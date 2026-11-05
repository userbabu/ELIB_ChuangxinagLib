// 通用工具：文本缓冲、UTF-8 处理、极简 JSON 解析、对接日志。
#include "bridge.h"
#include <stdarg.h>
#include <stdio.h>

namespace cx {

BrCfg  g_cfg;
BrStat g_stat;
CRITICAL_SECTION g_cs;
CRITICAL_SECTION g_csDispatch;   // 只圈住「跑用户子程序」这一段，保证事件串行派发
static INT g_csBuilt = 0;
static INT g_evSeq = 0;

void BrInitOnce()
{
    // 两个线程同时首次调用会各自 Initialize 一遍，把前一份锁对象覆盖掉（派发锁就失效了）；
    // 用一次性的原子占位保证只有一方建。
    if (g_csBuilt) return;
    if (InterlockedCompareExchange((volatile long*)&g_csBuilt, 1, 0) != 0) return;
    InitializeCriticalSection(&g_cs);
    InitializeCriticalSection(&g_csDispatch);
}

INT BrNextSeq()
{
    g_evSeq = ATOM_INC(&g_evSeq);
    return g_evSeq;
}

// ===================== 自带格式化 =====================
// 新 CRT 的 _snprintf/_vsnprintf 会引用 ___stdio_common_vsprintf，易语言静态编译用的
// VC6 LIBC 里没有这个符号（动态 .fne 毫无征兆，只有静态链接才炸），库内一律用这两个实现。

static INT UIntToText(unsigned long v, INT nBase, char* out, INT cchOut)
{
    char tmp[24];
    INT n = 0;
    if (v == 0) tmp[n++] = '0';
    while (v != 0 && n < (INT)sizeof(tmp))
    {
        const INT d = (INT)(v % (unsigned long)nBase);
        tmp[n++] = (char)((d < 10) ? ('0' + d) : ('a' + (d - 10)));
        v /= (unsigned long)nBase;
    }
    INT i = 0;
    for (INT k = n - 1; k >= 0 && i < cchOut - 1; --k) out[i++] = tmp[k];
    out[i] = '\0';
    return i;
}

INT CxVSnprintf(char* dst, INT cchDst, const char* pszFmt, va_list ap)
{
    if (dst == NULL || cchDst <= 0 || pszFmt == NULL) return 0;
    INT j = 0;
    for (INT i = 0; pszFmt[i] != '\0'; ++i)
    {
        if (pszFmt[i] != '%')
        {
            if (j < cchDst - 1) dst[j++] = pszFmt[i];
            continue;
        }
        ++i;
        if (pszFmt[i] == '\0') break;
        if (pszFmt[i] == '%')
        {
            if (j < cchDst - 1) dst[j++] = '%';
            continue;
        }
        INT nZero = 0, nWidth = 0;
        if (pszFmt[i] == '0') { nZero = 1; ++i; }
        while (pszFmt[i] >= '0' && pszFmt[i] <= '9') { nWidth = nWidth * 10 + (pszFmt[i] - '0'); ++i; }
        const char c = pszFmt[i];
        char item[512];
        INT nItem = 0;
        if (c == 's')
        {
            const char* psz = va_arg(ap, const char*);
            if (psz == NULL) psz = "(null)";
            nItem = (INT)strlen(psz);
            if (nItem > (INT)sizeof(item) - 1) nItem = (INT)sizeof(item) - 1;
            memcpy(item, psz, nItem);
        }
        else if (c == 'd')
        {
            const INT v = va_arg(ap, INT);
            unsigned long uv;
            if (v < 0)
            {
                item[nItem++] = '-';
                uv = (unsigned long)(-(v + 1)) + 1UL;   // 避开 INT_MIN 取负溢出
            }
            else uv = (unsigned long)v;
            nItem += UIntToText(uv, 10, item + nItem, (INT)sizeof(item) - nItem);
        }
        else if (c == 'u' || c == 'x' || c == 'X')
        {
            nItem = UIntToText((unsigned long)va_arg(ap, unsigned int), (c == 'x' || c == 'X') ? 16 : 10,
                item, (INT)sizeof(item));
            if (c == 'X')
            {
                for (INT k = 0; k < nItem; ++k)
                {
                    if (item[k] >= 'a' && item[k] <= 'f') item[k] = (char)(item[k] - 'a' + 'A');
                }
            }
        }
        else
        {
            // 不认识的格式符原样留下，方便发现用错的地方
            if (j < cchDst - 1) dst[j++] = '%';
            if (j < cchDst - 1) dst[j++] = c;
            continue;
        }
        for (INT k = nItem; k < nWidth && j < cchDst - 1; ++k) dst[j++] = nZero ? '0' : ' ';
        for (INT k = 0; k < nItem && j < cchDst - 1; ++k) dst[j++] = item[k];
    }
    dst[j] = '\0';
    return j;
}

INT CxSnprintf(char* dst, INT cchDst, const char* pszFmt, ...)
{
    va_list ap;
    va_start(ap, pszFmt);
    const INT n = CxVSnprintf(dst, cchDst, pszFmt, ap);
    va_end(ap);
    return n;
}

// ===================== Buf =====================

void BufInit(Buf* b) { b->p = NULL; b->nLen = 0; b->nCap = 0; }

void BufFree(Buf* b)
{
    if (b->p) { free(b->p); b->p = NULL; }
    b->nLen = 0; b->nCap = 0;
}

void BufReset(Buf* b) { b->nLen = 0; if (b->p) b->p[0] = '\0'; }

void BufReserve(Buf* b, INT nNeed)
{
    if (nNeed <= 0) nNeed = 1;
    if (b->nCap >= nNeed + 1) return;
    INT nCap = b->nCap ? b->nCap : 64;
    while (nCap < nNeed + 1) nCap *= 2;
    char* p = (char*)realloc(b->p, nCap);
    if (p == NULL) { BufFree(b); return; }
    b->p = p; b->nCap = nCap;
}

void BufAdd(Buf* b, const char* psz, INT nLen)
{
    if (psz == NULL || nLen <= 0) return;
    BufReserve(b, b->nLen + nLen);
    if (b->p == NULL) return;
    memcpy(b->p + b->nLen, psz, nLen);
    b->nLen += nLen;
    b->p[b->nLen] = '\0';
}

void BufAddCh(Buf* b, char c) { BufAdd(b, &c, 1); }

void BufAddSz(Buf* b, const char* psz)
{
    if (psz != NULL) BufAdd(b, psz, (INT)strlen(psz));
}

void BufAddF(Buf* b, const char* pszFmt, ...)
{
    char tmp[1024];
    va_list ap;
    va_start(ap, pszFmt);
    INT n = CxVSnprintf(tmp, sizeof(tmp) - 1, pszFmt, ap);
    va_end(ap);
    if (n < 0) n = (INT)strlen(tmp);
    if (n > (INT)sizeof(tmp) - 1) n = (INT)sizeof(tmp) - 1;
    BufAdd(b, tmp, n);
}

void BufAddJsonStr(Buf* b, const char* psz)
{
    BufAddCh(b, '"');
    for (const unsigned char* p = (const unsigned char*)psz; p != NULL && *p != '\0'; ++p)
    {
        const unsigned char c = *p;
        if (c == '"' || c == '\\') { BufAddCh(b, '\\'); BufAddCh(b, (char)c); }
        else if (c == '\n') BufAddSz(b, "\\n");
        else if (c == '\r') BufAddSz(b, "\\r");
        else if (c == '\t') BufAddSz(b, "\\t");
        else if (c < 0x20) BufAddF(b, "\\u%04x", (INT)c);
        else BufAddCh(b, (char)c);
    }
    BufAddCh(b, '"');
}

// ===================== JSON 组装 =====================

void JsonKey(Buf* b, const char* pszKey, BOOL bComma)
{
    if (bComma) BufAddCh(b, ',');
    BufAddCh(b, '"');
    BufAdd(b, pszKey, StrLen(pszKey));
    BufAddSz(b, "\":");
}

void JsonStr(Buf* b, const char* pszKey, const char* pszVal, BOOL bComma)
{
    JsonKey(b, pszKey, bComma);
    BufAddJsonStr(b, pszVal == NULL ? "" : pszVal);
}

void JsonBool(Buf* b, const char* pszKey, INT nVal, BOOL bComma)
{
    JsonKey(b, pszKey, bComma);
    BufAdd(b, nVal ? "true" : "false", nVal ? 4 : 5);
}

void JsonInt(Buf* b, const char* pszKey, INT nVal, BOOL bComma)
{
    JsonKey(b, pszKey, bComma);
    BufAddF(b, "%d", nVal);
}

// ===================== 文本工具 =====================

INT StrLen(const char* psz) { return psz == NULL ? 0 : (INT)strlen(psz); }
static int LowerOf(int c) { return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c; }

BOOL StrEqI(const char* a, const char* b)
{
    if (a == NULL || b == NULL) return FALSE;
    while (*a != '\0' && *b != '\0')
    {
        if (LowerOf((unsigned char)*a) != LowerOf((unsigned char)*b)) return FALSE;
        ++a; ++b;
    }
    return *a == '\0' && *b == '\0';
}

const char* StrFindI(const char* hay, const char* needle)
{
    if (hay == NULL || needle == NULL) return NULL;
    const INT n = (INT)strlen(needle);
    if (n == 0) return hay;
    for (const char* p = hay; *p != '\0'; ++p)
    {
        INT i = 0;
        while (i < n && p[i] != '\0' && LowerOf((unsigned char)p[i]) == LowerOf((unsigned char)needle[i])) ++i;
        if (i == n) return p;
    }
    return NULL;
}

BOOL StrHasI(const char* hay, const char* needle) { return StrFindI(hay, needle) != NULL; }

void StrCpyN(char* dst, INT cchDst, const char* src)
{
    if (dst == NULL || cchDst <= 0) return;
    if (src == NULL) { dst[0] = '\0'; return; }
    INT i = 0;
    for (; i < cchDst - 1 && src[i] != '\0'; ++i) dst[i] = src[i];
    dst[i] = '\0';
}

void StrTrim(char* psz)
{
    if (psz == NULL) return;
    while (*psz == ' ' || *psz == '\t' || *psz == '\r' || *psz == '\n') ++psz;
    // 首部空白只能移动调用方指针，这里只截尾
    INT n = (INT)strlen(psz);
    while (n > 0 && (psz[n - 1] == ' ' || psz[n - 1] == '\t' || psz[n - 1] == '\r' || psz[n - 1] == '\n')) psz[--n] = '\0';
}

const char* Zh(const char* pszGbk)
{
    static char szBuf[8][512];
    static INT nSlot = 0;
    char* dst = szBuf[nSlot];
    nSlot = (nSlot + 1) & 7;
    dst[0] = '\0';
    if (pszGbk == NULL) return dst;

    // 已是纯 ASCII 直接搬走，省两次系统调用
    BOOL bAscii = TRUE;
    for (const unsigned char* p = (const unsigned char*)pszGbk; *p != '\0'; ++p)
    {
        if (*p >= 0x80) { bAscii = FALSE; break; }
    }
    if (bAscii)
    {
        StrCpyN(dst, 512, pszGbk);
        return dst;
    }

    const INT nW = MultiByteToWideChar(936, 0, pszGbk, -1, NULL, 0);
    if (nW <= 0) { StrCpyN(dst, 512, pszGbk); return dst; }
    WCHAR wTmp[512];
    LPWSTR pW = wTmp;
    if (nW > 512) { pW = (LPWSTR)malloc(sizeof(WCHAR) * nW); if (pW == NULL) { StrCpyN(dst, 512, pszGbk); return dst; } }
    const BOOL bOk = MultiByteToWideChar(936, 0, pszGbk, -1, pW, nW) > 0;
    if (bOk)
    {
        const INT nU = WideCharToMultiByte(CP_UTF8, 0, pW, -1, dst, 512, NULL, NULL);
        if (nU <= 0) dst[0] = '\0';
    }
    else StrCpyN(dst, 512, pszGbk);
    if (pW != wTmp) free(pW);
    return dst;
}

const char* ToUtf8(const char* psz)
{
    static char szBuf[8][4096];
    static INT nSlot = 0;
    char* dst = szBuf[nSlot];
    nSlot = (nSlot + 1) & 7;
    dst[0] = '\0';
    if (psz == NULL || psz[0] == '\0') return dst;
    // 已经是合法 UTF-8（浏览器来的数据、库自己 Zh 过的常量）就原样走，别二次转码
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, psz, -1, NULL, 0) > 0) return psz;
    const INT nW = MultiByteToWideChar(936, 0, psz, -1, NULL, 0);
    if (nW <= 0) return psz;
    WCHAR wTmp[512];
    LPWSTR pW = (nW <= 512) ? wTmp : (LPWSTR)malloc(sizeof(WCHAR) * nW);
    if (pW == NULL) return psz;
    if (MultiByteToWideChar(936, 0, psz, -1, pW, nW) > 0)
    {
        if (WideCharToMultiByte(CP_UTF8, 0, pW, -1, dst, 4096, NULL, NULL) <= 0) dst[0] = '\0';
    }
    if (pW != wTmp) free(pW);
    return dst[0] != '\0' ? dst : psz;   // 转不动就退回原样，至少不丢信息
}

LPWSTR Utf8ToWide(const char* pszUtf8)
{
    if (pszUtf8 == NULL || *pszUtf8 == '\0') return NULL;
    const INT nW = MultiByteToWideChar(CP_UTF8, 0, pszUtf8, -1, NULL, 0);
    if (nW <= 0) return NULL;
    LPWSTR pW = (LPWSTR)malloc(sizeof(WCHAR) * nW);
    if (pW == NULL) return NULL;
    if (MultiByteToWideChar(CP_UTF8, 0, pszUtf8, -1, pW, nW) <= 0) { free(pW); return NULL; }
    return pW;
}

void NowText(char* dst, INT cchDst)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    CxSnprintf(dst, cchDst, "%04d-%02d-%02d %02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    dst[cchDst - 1] = '\0';
}

// ===================== 极简 JSON 解析 =====================
// 只处理扩展发来的扁平对象；键查找限定在顶层大括号内，值可能是字符串/数字/布尔/数组/对象。

static const char* JsonSkipSpace(const char* p, const char* pEnd)
{
    while (p < pEnd && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) ++p;
    return p;
}

// 从 p 开始跳过一个值，返回值的结束位置
static const char* JsonSkipValue(const char* p, const char* pEnd)
{
    p = JsonSkipSpace(p, pEnd);
    if (p >= pEnd) return p;
    if (*p == '"' || *p == '\'')
    {
        const char q = *p;
        ++p;
        while (p < pEnd)
        {
            if (*p == '\\') { p += 2; continue; }
            if (*p == q) return p + 1;
            ++p;
        }
        return p;
    }
    if (*p == '{' || *p == '[')
    {
        INT nDepth = 0;
        BOOL bInStr = FALSE;
        for (; p < pEnd; ++p)
        {
            const char c = *p;
            if (bInStr)
            {
                if (c == '\\') { ++p; continue; }
                if (c == '"') bInStr = FALSE;
                continue;
            }
            if (c == '"') bInStr = TRUE;
            else if (c == '{' || c == '[') ++nDepth;
            else if (c == '}' || c == ']') { --nDepth; if (nDepth <= 0) return p + 1; }
        }
        return p;
    }
    while (p < pEnd && *p != ',' && *p != '}' && *p != ']' && *p != '\r' && *p != '\n') ++p;
    return p;
}

// 找到顶层键对应的值起始位置
static const char* JsonFindValue(const char* pszJson, const char* pszKey, const char** ppValEnd)
{
    if (pszJson == NULL || pszKey == NULL) return NULL;
    const INT nJson = StrLen(pszJson);
    const char* pEnd = pszJson + nJson;
    const char* p = JsonSkipSpace(pszJson, pEnd);
    if (p >= pEnd || (*p != '{' && *p != '[')) return NULL;
    ++p;
    const INT nKeyLen = StrLen(pszKey);
    INT nDepth = 1;
    while (p < pEnd && nDepth > 0)
    {
        p = JsonSkipSpace(p, pEnd);
        if (p >= pEnd) break;
        if (*p == '"' )
        {
            const char* pName = p + 1;
            const char* pNameEnd = pName;
            while (pNameEnd < pEnd && *pNameEnd != '"')
            {
                if (*pNameEnd == '\\') ++pNameEnd;
                ++pNameEnd;
            }
            const char* pVal = JsonSkipValue(p, pEnd);   // 跳过键文本
            p = JsonSkipSpace(pVal, pEnd);
            if (p < pEnd && *p == ':')
            {
                ++p;
                p = JsonSkipSpace(p, pEnd);
                const char* pVEnd = JsonSkipValue(p, pEnd);
                if (pNameEnd - pName == nKeyLen && StrNCmpI(pName, pszKey, nKeyLen))
                {
                    if (ppValEnd) *ppValEnd = pVEnd;
                    return p;
                }
                p = pVEnd;
                continue;
            }
            // 不是键（比如数组里的裸字符串）
            p = pVal;
            continue;
        }
        if (*p == ',' || *p == ':') { ++p; continue; }
        if (*p == '}' || *p == ']') { --nDepth; ++p; continue; }
        p = JsonSkipValue(p, pEnd);
    }
    return NULL;
}

BOOL JsonGetRaw(const char* pszJson, const char* pszKey, char* dst, INT cchDst)
{
    const char* pEnd = NULL;
    const char* p = JsonFindValue(pszJson, pszKey, &pEnd);
    if (p == NULL || pEnd == NULL || pEnd <= p) return FALSE;
    INT n = (INT)(pEnd - p);
    if (n > cchDst - 1) n = cchDst - 1;
    memcpy(dst, p, n);
    dst[n] = '\0';
    return TRUE;
}

static INT HexOf(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// 把 \uXXXX 编成 UTF-8
static INT AppendUtf16(char* dst, INT cchLeft, INT nCode)
{
    if (nCode < 0x80) { if (cchLeft < 1) return 0; dst[0] = (char)nCode; return 1; }
    if (nCode < 0x800)
    {
        if (cchLeft < 2) return 0;
        dst[0] = (char)(0xC0 | (nCode >> 6));
        dst[1] = (char)(0x80 | (nCode & 0x3F));
        return 2;
    }
    if (cchLeft < 3) return 0;
    dst[0] = (char)(0xE0 | (nCode >> 12));
    dst[1] = (char)(0x80 | ((nCode >> 6) & 0x3F));
    dst[2] = (char)(0x80 | (nCode & 0x3F));
    return 3;
}

BOOL JsonGetString(const char* pszJson, const char* pszKey, char* dst, INT cchDst)
{
    char raw[2048];
    raw[0] = '\0';
    if (!JsonGetRaw(pszJson, pszKey, raw, sizeof(raw))) return FALSE;
    StrTrim(raw);
    const INT n = StrLen(raw);
    INT j = 0;
    dst[0] = '\0';
    if (n >= 2 && raw[0] == '"' && raw[n - 1] == '"')
    {
        for (INT i = 1; i < n - 1 && j < cchDst - 1; ++i)
        {
            char c = raw[i];
            if (c != '\\') { dst[j++] = c; continue; }
            ++i;
            if (i >= n - 1) break;
            const char e = raw[i];
            switch (e)
            {
            case 'n': dst[j++] = '\n'; break;
            case 'r': dst[j++] = '\r'; break;
            case 't': dst[j++] = '\t'; break;
            case 'b': dst[j++] = '\b'; break;
            case 'f': dst[j++] = '\f'; break;
            case '/': dst[j++] = '/'; break;
            case '"': dst[j++] = '"'; break;
            case '\\': dst[j++] = '\\'; break;
            case 'u':
            {
                if (i + 4 < n - 1)
                {
                    INT h1 = HexOf(raw[i + 1]), h2 = HexOf(raw[i + 2]), h3 = HexOf(raw[i + 3]), h4 = HexOf(raw[i + 4]);
                    if (h1 >= 0 && h2 >= 0 && h3 >= 0 && h4 >= 0)
                    {
                        INT code = (h1 << 12) | (h2 << 8) | (h3 << 4) | h4;
                        // 代理对：拼成一个码点，避免写出半个
                        if (code >= 0xD800 && code <= 0xDBFF && i + 10 < n - 1 &&
                            raw[i + 5] == '\\' && raw[i + 6] == 'u')
                        {
                            INT l1 = HexOf(raw[i + 7]), l2 = HexOf(raw[i + 8]), l3 = HexOf(raw[i + 9]), l4 = HexOf(raw[i + 10]);
                            if (l1 >= 0 && l2 >= 0 && l3 >= 0 && l4 >= 0)
                            {
                                const INT low = (l1 << 12) | (l2 << 8) | (l3 << 4) | l4;
                                if (low >= 0xDC00 && low <= 0xDFFF)
                                {
                                    code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                                    i += 6;
                                    if (j + 4 >= cchDst) { dst[j] = '\0'; return j > 0; }
                                    dst[j++] = (char)(0xF0 | (code >> 18));
                                    dst[j++] = (char)(0x80 | ((code >> 12) & 0x3F));
                                    dst[j++] = (char)(0x80 | ((code >> 6) & 0x3F));
                                    dst[j++] = (char)(0x80 | (code & 0x3F));
                                    continue;
                                }
                            }
                        }
                        const INT w = AppendUtf16(dst + j, cchDst - 1 - j, code);
                        if (w == 0) { dst[j] = '\0'; return j > 0; }
                        j += w;
                        i += 4;
                        break;
                    }
                }
                dst[j++] = 'u';
                break;
            }
            default: dst[j++] = e; break;
            }
        }
    }
    else
    {
        // 数字 / 布尔 / null 也当文本交回，方便易语言直接判
        INT nCopy = n;
        if (nCopy > cchDst - 1) nCopy = cchDst - 1;
        memcpy(dst, raw, nCopy);
        j = nCopy;
    }
    dst[j] = '\0';
    return j > 0 || dst[0] == '\0';
}

BOOL JsonGetInt(const char* pszJson, const char* pszKey, INT* pOut)
{
    char tmp[64];
    if (!JsonGetString(pszJson, pszKey, tmp, sizeof(tmp))) return FALSE;
    StrTrim(tmp);
    char* pEnd = NULL;
    const long v = strtol(tmp, &pEnd, 10);
    if (pEnd == tmp) return FALSE;
    *pOut = (INT)v;
    return TRUE;
}

BOOL JsonGetBool(const char* pszJson, const char* pszKey, INT* pOut)
{
    char tmp[32];
    if (!JsonGetString(pszJson, pszKey, tmp, sizeof(tmp))) return FALSE;
    StrTrim(tmp);
    if (StrEqI(tmp, "true") || StrEqI(tmp, "1")) { *pOut = 1; return TRUE; }
    if (StrEqI(tmp, "false") || StrEqI(tmp, "0")) { *pOut = 0; return TRUE; }
    return FALSE;
}

// 大小写无关的定长比较（键名比较用）
BOOL StrNCmpI(const char* a, const char* b, INT n)
{
    for (INT i = 0; i < n; ++i)
    {
        if (LowerOf((unsigned char)a[i]) != LowerOf((unsigned char)b[i])) return FALSE;
        if (a[i] == '\0') return b[i] == '\0';
    }
    return TRUE;
}

// ===================== 日志 =====================

// 运行目录到底是不是 UTF-8，只能就地判定：本库对外是 UTF-8，但 取运行目录() 在 GBK 口径的
// 易语言程序里传进来的是 ANSI 字节，按 UTF-8 解出来的目录不存在。两种都试一次再定。
static void ResolveRunDir()
{
    if (g_cfg.nDirMode != 0) return;
    if (g_cfg.szRunDir[0] == '\0') { g_cfg.nDirMode = 1; return; }
    LPWSTR pW = Utf8ToWide(g_cfg.szRunDir);
    if (pW != NULL)
    {
        const BOOL bOk = (GetFileAttributesW(pW) != INVALID_FILE_ATTRIBUTES);
        free(pW);
        if (bOk) { g_cfg.nDirMode = 1; return; }
    }
    g_cfg.nDirMode = 2;
}

static LPWSTR RunDirToWide()
{
    ResolveRunDir();
    if (g_cfg.szRunDir[0] == '\0') return NULL;
    if (g_cfg.nDirMode == 1) return Utf8ToWide(g_cfg.szRunDir);
    const INT nW = MultiByteToWideChar(936, 0, g_cfg.szRunDir, -1, NULL, 0);
    if (nW <= 0) return NULL;
    LPWSTR pW = (LPWSTR)malloc(sizeof(WCHAR) * nW);
    if (pW == NULL) return NULL;
    if (MultiByteToWideChar(936, 0, g_cfg.szRunDir, -1, pW, nW) <= 0) { free(pW); return NULL; }
    return pW;
}

// 日志文件名的窄字节：跟着目录的判定结果走，两者拼在一起才是能打开的路径
static const char* LogNameBytes()
{
    static const char* pszUtf8 = NULL;
    if (pszUtf8 == NULL) pszUtf8 = Zh("浏览器插件对接.log");
    ResolveRunDir();
    return (g_cfg.nDirMode == 2) ? "cxbrow.log" : pszUtf8;
}

static const WCHAR* LogNameWide()
{
    static WCHAR wsz[32] = { 0 };
    if (wsz[0] == L'\0')
    {
        const char* psz = (g_cfg.nDirMode == 2) ? "cxbrow.log" : Zh("浏览器插件对接.log");
        const INT n = (g_cfg.nDirMode == 2) ? 936 : CP_UTF8;
        MultiByteToWideChar(n, 0, psz, -1, wsz, 32);
    }
    return wsz;
}

void BrLog(const char* pszFmt, ...)
{
    char msg[2048];
    va_list ap;
    va_start(ap, pszFmt);
    CxVSnprintf(msg, sizeof(msg) - 1, pszFmt, ap);
    va_end(ap);
    msg[sizeof(msg) - 1] = '\0';

    char line[2300];
    char szTime[32];
    NowText(szTime, sizeof(szTime));
    CxSnprintf(line, sizeof(line) - 3, "[%s] %s\r\n", szTime, msg);
    line[sizeof(line) - 3] = '\0';

    if (g_cfg.szRunDir[0] == '\0') return;
    LPWSTR pDir = RunDirToWide();
    if (pDir == NULL)
    {
        // 目录解析不出来：把原因留下，否则整份对接日志会无声消失
        if (g_stat.szLogErr[0] == '\0')
            CxSnprintf(g_stat.szLogErr, sizeof(g_stat.szLogErr) - 1, "%s", Zh("运行目录解析失败，日志没有写盘。"));
        return;
    }
    WCHAR wszPath[600];
    wszPath[0] = L'\0';
    lstrcpynW(wszPath, pDir, 520);
    const INT nBase = (INT)lstrlenW(wszPath);
    if (nBase > 0 && wszPath[nBase - 1] != L'\\') lstrcpynW(wszPath + nBase, L"\\", 560 - nBase);
    lstrcatW(wszPath, LogNameWide());
    free(pDir);

    HANDLE hFile = CreateFileW(wszPath, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        // 权限不足 / 目录不存在 / 被策略挡掉：只记一次，交给「取对接错误」说出来
        if (g_stat.szLogErr[0] == '\0')
        {
            char szUtf8[600];
            szUtf8[0] = '\0';
            WideCharToMultiByte(CP_UTF8, 0, wszPath, -1, szUtf8, (INT)sizeof(szUtf8) - 1, NULL, NULL);
            CxSnprintf(g_stat.szLogErr, sizeof(g_stat.szLogErr) - 1, Zh("日志写不进去（错误码 %u）：%s"),
                GetLastError(), szUtf8);
        }
        return;
    }
    // 正文原样落成 UTF-8：库自己的中文都过了 Zh()，程序传进来的文本按约定就是 UTF-8；
    // 整行再判一次编码会把已经正确的中文二次转码，反而毁掉整份日志。
    DWORD dwWritten = 0;
    WriteFile(hFile, line, (DWORD)strlen(line), &dwWritten, NULL);
    CloseHandle(hFile);
}

// 给 取对接日志路径 用：按目录判定结果拼出能直接打开的路径字节
void LogPathBytes(char* dst, INT cchDst)
{
    dst[0] = '\0';
    if (g_cfg.szRunDir[0] == '\0') return;
    CxSnprintf(dst, cchDst - 1, "%s\\%s", g_cfg.szRunDir, LogNameBytes());
    dst[cchDst - 1] = '\0';
}

void BrLogOnce(const char* pszKey, const char* pszFmt, ...)
{
    static char szLast[8][160];
    static INT nSlot = 0;
    char msg[256];
    va_list ap;
    va_start(ap, pszFmt);
    CxVSnprintf(msg, sizeof(msg) - 1, pszFmt, ap);
    va_end(ap);
    msg[sizeof(msg) - 1] = '\0';
    for (INT i = 0; i < 8; ++i)
    {
        if (szLast[i][0] != '\0' && StrEqI(szLast[i], msg)) return;   // 同一句只记一次
    }
    StrCpyN(szLast[nSlot], 160, msg);
    nSlot = (nSlot + 1) % 8;
    BrLog("%s: %s", pszKey, msg);
}

void BrSetError(const char* pszFmt, ...)
{
    va_list ap;
    va_start(ap, pszFmt);
    EnterCriticalSection(&g_cs);
    CxVSnprintf(g_stat.szErr, sizeof(g_stat.szErr) - 1, pszFmt, ap);
    LeaveCriticalSection(&g_cs);
    va_end(ap);
    g_stat.szErr[sizeof(g_stat.szErr) - 1] = '\0';
}

void BrAppendError(const char* pszFmt, ...)
{
    va_list ap;
    va_start(ap, pszFmt);
    EnterCriticalSection(&g_cs);
    const INT nLen = (INT)strlen(g_stat.szErr);
    if (nLen < (INT)sizeof(g_stat.szErr) - 2)
    {
        g_stat.szErr[nLen] = ' ';
        g_stat.szErr[nLen + 1] = '\0';
        CxVSnprintf(g_stat.szErr + nLen + 1, (INT)sizeof(g_stat.szErr) - nLen - 2, pszFmt, ap);
    }
    LeaveCriticalSection(&g_cs);
    va_end(ap);
    g_stat.szErr[sizeof(g_stat.szErr) - 1] = '\0';
}

}  // namespace cx
