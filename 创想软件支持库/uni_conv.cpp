// 编码转换命令（迁移自 编码转换通用模块 / 创想编码模块）。
// 这一族的职责就是"在编码之间转换"，因此代码页按原版语义生效：ANSI 侧默认 936；
// 库内其它命令的文本型仍是 UTF-8 原样透传，不受这里影响。

#include <cx_uni.h>

namespace cx {

extern PFN_NOTIFY_SYS g_pfnNotifySys;
INT NotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2);
char* CloneTextData(LPCSTR pszText, INT nLen);
LPBYTE CloneBinData(LPCSTR pData, INT nDataSize);

static const WCHAR g_cvEmpty[1] = { 0 };
#define CV_DEFAULT_CP 936

// ============================================================ 转换核心

// 任意代码页文本 → 宽字符（LocalAlloc，调用者释放）。
static WCHAR* CvCpToWide(LPCSTR sz, UINT uCP, INT* pnChars)
{
    if (pnChars) *pnChars = 0;
    if (sz == NULL) return NULL;
    INT n = MultiByteToWideChar(uCP, 0, sz, -1, NULL, 0);
    if (n <= 1) return (WCHAR*)LocalAlloc(LPTR, sizeof(WCHAR));
    WCHAR* p = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)n * sizeof(WCHAR));
    if (p == NULL) return NULL;
    MultiByteToWideChar(uCP, 0, sz, -1, p, n);
    *pnChars = n - 1;
    return p;
}

// 按给定长度转换，不依赖结尾 0（参数文本可能指向程序常量段，就地补 0 会写坏常量）。
static WCHAR* CvCpToWideN(LPCSTR sz, INT nBytes, UINT uCP, INT* pnChars)
{
    if (pnChars) *pnChars = 0;
    if (sz == NULL || nBytes <= 0) return NULL;
    INT n = MultiByteToWideChar(uCP, 0, sz, nBytes, NULL, 0);
    if (n <= 0) return NULL;
    WCHAR* p = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)n * sizeof(WCHAR));
    if (p == NULL) return NULL;
    MultiByteToWideChar(uCP, 0, sz, nBytes, p, n);
    *pnChars = n;
    return p;
}

// 宽字符 → 任意代码页文本（LocalAlloc，调用者释放）。
static char* CvWideToCp(const WCHAR* pW, INT nChars, UINT uCP)
{
    if (pW == NULL || nChars <= 0) nChars = 0;
    INT n = WideCharToMultiByte(uCP, 0, pW, nChars, NULL, 0, NULL, NULL);
    char* p = (char*)LocalAlloc(LPTR, (SIZE_T)n + 1);
    if (p == NULL) return NULL;
    if (n > 0) WideCharToMultiByte(uCP, 0, pW, nChars, p, n, NULL, NULL);
    p[n] = 0;
    return p;
}

static char* CvCpToCp(LPCSTR sz, UINT uFrom, UINT uTo)
{
    INT nW = 0;
    WCHAR* w = CvCpToWide(sz, uFrom, &nW);
    char* out = CvWideToCp(w, nW, uTo);
    if (w != NULL) LocalFree(w);
    return out;
}

static void CvRetText(PMDATA_INF pRet, LPCSTR sz)
{
    pRet->m_dtDataType = SDT_TEXT;
    pRet->m_pText = (sz != NULL) ? CloneTextData(sz, (INT)lstrlenA(sz)) : CloneTextData("", 0);
}

// 宽文本 → UTF-8 字节集 / 文本。
static void CvRetWideAsUtf8(PMDATA_INF pRet, const WCHAR* pW, INT nChars, BOOL bAsText)
{
    char* sz = CvWideToCp(pW, nChars, CP_UTF8);
    if (bAsText) CvRetText(pRet, sz);
    else UniRetRawBin(pRet, sz, (sz != NULL) ? (INT)lstrlenA(sz) : 0);
    if (sz != NULL) LocalFree(sz);
}

// UTF-8 文本 → 宽文本字节集（末尾带 {0,0}）。
static void CvRetUtf8AsWide(PMDATA_INF pRet, LPCSTR szUtf8)
{
    INT nChars = 0;
    WCHAR* w = CvCpToWide(szUtf8, CP_UTF8, &nChars);
    if (w == NULL) { UniRetWideBin(pRet, g_cvEmpty, 0); return; }
    UniRetWideBin(pRet, w, nChars);
    LocalFree(w);
}

static void CvRetWideBinOfCp(PMDATA_INF pRet, LPCSTR sz, UINT uCP, BOOL bKeepNull)
{
    INT nChars = 0;
    WCHAR* w = CvCpToWide(sz, uCP, &nChars);
    if (w == NULL) { UniRetWideBin(pRet, g_cvEmpty, 0); return; }
    if (bKeepNull) UniRetWideBin(pRet, w, nChars);
    else UniRetWideBinNoNull(pRet, w, nChars);
    LocalFree(w);
}

// 字节集参数按 C 串取用（原版这些参数装的都是文本内容）；返回 LocalAlloc 缓冲，调用者释放。
static char* CvBinAsCString(const MDATA_INF& arg)
{
    INT nBytes = 0;
    LPBYTE p = UniArgBinPtr(arg, &nBytes);
    INT n = 0;
    while (n < nBytes && p[n] != 0) n++;
    char* out = (char*)LocalAlloc(LPTR, (SIZE_T)n + 1);
    if (out == NULL) return NULL;
    if (n > 0) memcpy(out, p, (SIZE_T)n);
    out[n] = 0;
    return out;
}

// ============================================================ ANSI / UTF-8 / UTF-16 互转

void UcW2A(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    char* sz = CvWideToCp(v.p, v.nChars, (UINT)UniArgInt(pArgInf[1], CV_DEFAULT_CP));
    CvRetText(pRetData, sz);
    if (sz != NULL) LocalFree(sz);
}

void UcA2W(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CvRetWideBinOfCp(pRetData, UniArgText(pArgInf[0]), (UINT)UniArgInt(pArgInf[1], CV_DEFAULT_CP), TRUE);
}

void UcA2WNo(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CvRetWideBinOfCp(pRetData, UniArgText(pArgInf[0]), (UINT)UniArgInt(pArgInf[1], CV_DEFAULT_CP), FALSE);
}

void UcW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UcA2W(pRetData, nArgCount, pArgInf);
}

void UcA(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UcW2A(pRetData, nArgCount, pArgInf);
}

void UcW2Utf8(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    CvRetWideAsUtf8(pRetData, v.p, v.nChars, FALSE);
}

void UcW2Utf8T(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    CvRetWideAsUtf8(pRetData, v.p, v.nChars, TRUE);
}

void UcUtf82W(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    char* buf = CvBinAsCString(pArgInf[0]);
    CvRetUtf8AsWide(pRetData, buf);
    if (buf != NULL) LocalFree(buf);
}

void UcUtf82WT(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CvRetUtf8AsWide(pRetData, UniArgText(pArgInf[0]));
}

void UcUtf82A(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    char* buf = CvBinAsCString(pArgInf[0]);
    UINT cp = (UINT)UniArgInt(pArgInf[1], CV_DEFAULT_CP);
    char* sz = CvCpToCp(buf, CP_UTF8, cp);
    CvRetText(pRetData, sz);
    if (sz != NULL) LocalFree(sz);
    if (buf != NULL) LocalFree(buf);
}

void UcUtf82AT(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UINT cp = (UINT)UniArgInt(pArgInf[1], CV_DEFAULT_CP);
    char* sz = CvCpToCp(UniArgText(pArgInf[0]), CP_UTF8, cp);
    CvRetText(pRetData, sz);
    if (sz != NULL) LocalFree(sz);
}

void UcA2Utf8(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UINT cp = (UINT)UniArgInt(pArgInf[1], CV_DEFAULT_CP);
    LPCSTR sz = UniArgText(pArgInf[0]);
    INT nW = 0;
    WCHAR* w = CvCpToWide(sz, cp, &nW);
    CvRetWideAsUtf8(pRetData, w, nW, FALSE);
    if (w != NULL) LocalFree(w);
}

void UcA2Utf8T(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UINT cp = (UINT)UniArgInt(pArgInf[1], CV_DEFAULT_CP);
    char* sz = CvCpToCp(UniArgText(pArgInf[0]), cp, CP_UTF8);
    CvRetText(pRetData, sz);
    if (sz != NULL) LocalFree(sz);
}

void UcUtf8ToGb(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    char* sz = CvCpToCp(UniArgText(pArgInf[0]), CP_UTF8, CV_DEFAULT_CP);
    CvRetText(pRetData, sz);
    if (sz != NULL) LocalFree(sz);
}

void UcGbToUtf8(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    char* sz = CvCpToCp(UniArgText(pArgInf[0]), CV_DEFAULT_CP, CP_UTF8);
    CvRetText(pRetData, sz);
    if (sz != NULL) LocalFree(sz);
}

// ============================================================ 宽文本末尾 {0,0} 处理（= WF）

void UcWideTrimNull(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    BOOL bStd = UniArgBool(pArgInf[1], FALSE);
    pRetData->m_dtDataType = SDT_BIN;
    pRetData->m_pBin = NULL;
    if (v.nOrigBytes <= 0 || (v.nOrigBytes == 2 && v.nChars <= 0)) return;   // 空 / {0,0}
    if (!bStd)
    {
        INT n = v.nRawChars - 1;
        pRetData->m_pBin = (n <= 0) ? NULL : CloneBinData((LPCSTR)v.pRaw, n * 2);
        return;
    }
    if (v.nChars <= 0) return;
    UniRetWideBin(pRetData, v.pRaw, v.nChars);
}

// ============================================================ URL 编解码

static BOOL CvIsUnreserved(BYTE b)
{
    if (b >= 'A' && b <= 'Z') return TRUE;
    if (b >= 'a' && b <= 'z') return TRUE;
    if (b >= '0' && b <= '9') return TRUE;
    return (b == '-' || b == '_' || b == '.' || b == '!' || b == '~' || b == '*' || b == '\'' || b == '(' || b == ')') ? TRUE : FALSE;
}

static const char* CvHexUpper(void) { return "0123456789ABCDEF"; }

static BOOL CvIsReservedUri(BYTE b)
{
    return (b == ';' || b == '/' || b == '?' || b == ':' || b == '@' || b == '&' || b == '=' ||
            b == '+' || b == '$' || b == ',' || b == '#') ? TRUE : FALSE;
}

static INT CvHexVal(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// 宽文本 → UTF-8 → 百分号转义（encodeURIComponent 口径）。
void UcUrlEncodeW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    char* utf8 = CvWideToCp(v.p, v.nChars, CP_UTF8);
    if (utf8 == NULL) { CvRetText(pRetData, ""); return; }
    UniWBuf out;
    out.Reserve(16);
    char* o = (char*)LocalAlloc(LPTR, (SIZE_T)lstrlenA(utf8) * 3 + 1);
    INT nOut = 0;
    if (o != NULL)
    {
        for (const char* p = utf8; *p; p++)
        {
            BYTE b = (BYTE)*p;
            if (CvIsUnreserved(b)) o[nOut++] = (char)b;
            else
            {
                o[nOut++] = '%';
                o[nOut++] = CvHexUpper()[(b >> 4) & 0xF];
                o[nOut++] = CvHexUpper()[b & 0xF];
            }
        }
        o[nOut] = 0;
        CvRetText(pRetData, o);
        LocalFree(o);
    }
    else CvRetText(pRetData, "");
    LocalFree(utf8);
}

// 百分号解码。bPlusAsSpace：'+' 是否当空格；bKeepReserved：保留字符的转义原样留着（decodeURI）；
// bUtf8ToGbk：解出的字节按 UTF-8 再转 936（原版 是否UTF8 行为）。
static void UrlDecodeCore(PMDATA_INF pRetData, LPCSTR szIn, BOOL bPlusAsSpace, BOOL bKeepReserved, BOOL bUtf8ToGbk)
{
    if (szIn == NULL) { CvRetText(pRetData, ""); return; }
    INT n = lstrlenA(szIn);
    char* out = (char*)LocalAlloc(LPTR, (SIZE_T)n + 1);
    if (out == NULL) { CvRetText(pRetData, ""); return; }
    INT j = 0;
    for (INT i = 0; i < n; )
    {
        char c = szIn[i];
        if (c == '%' && i + 2 < n)
        {
            INT h = CvHexVal(szIn[i + 1]), l = CvHexVal(szIn[i + 2]);
            if (h >= 0 && l >= 0)
            {
                BYTE b = (BYTE)(h * 16 + l);
                if (bKeepReserved && CvIsReservedUri(b))
                {
                    out[j++] = '%'; out[j++] = szIn[i + 1]; out[j++] = szIn[i + 2];
                }
                else out[j++] = (char)b;
                i += 3;
                continue;
            }
        }
        if (c == '+' && bPlusAsSpace) { out[j++] = ' '; i++; continue; }
        out[j++] = c; i++;
    }
    out[j] = 0;
    if (bUtf8ToGbk)
    {
        char* gb = CvCpToCp(out, CP_UTF8, CV_DEFAULT_CP);
        CvRetText(pRetData, gb);
        if (gb != NULL) LocalFree(gb);
    }
    else CvRetText(pRetData, out);
    LocalFree(out);
}

void UcUrlDecode(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UrlDecodeCore(pRetData, UniArgText(pArgInf[0]), TRUE, FALSE, UniArgBool(pArgInf[1], FALSE) ? TRUE : FALSE);
}

void UcUrlDecodeJs(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    // 0 = decodeURI（保留字符的转义原样保留，'+' 不当空格），1 = decodeURIComponent（全解）。
    INT nType = UniArgInt(pArgInf[1], 0);
    UrlDecodeCore(pRetData, UniArgText(pArgInf[0]), FALSE, nType == 0 ? TRUE : FALSE, FALSE);
}

// ============================================================ HTML 实体解码

struct CvEntity { const char* szName; UINT uCode; };
static const CvEntity g_cvEntities[] = {
    { "amp", '&' }, { "lt", '<' }, { "gt", '>' }, { "quot", '"' }, { "apos", '\'' },
    { "nbsp", 0xA0 }, { "copy", 0xA9 }, { "reg", 0xAE }, { "trade", 0x2122 },
    { "middot", 0xB7 }, { "bull", 0x2022 }, { "hellip", 0x2026 }, { "mdash", 0x2014 },
    { "ndash", 0x2013 }, { "lsquo", 0x2018 }, { "rsquo", 0x2019 }, { "ldquo", 0x201C },
    { "rdquo", 0x201D }, { "laquo", 0xAB }, { "raquo", 0xBB }, { "times", 0xD7 },
    { "divide", 0xF7 }, { "plusmn", 0xB1 }, { "deg", 0xB0 }, { "micro", 0xB5 },
    { "para", 0xB6 }, { "sect", 0xA7 }, { "cedil", 0xB8 }, { "ordm", 0xBA }, { "ordf", 0xAA },
    { "frac12", 0xBD }, { "frac14", 0xBC }, { "frac34", 0xBE }, { "sup1", 0xB9 },
    { "sup2", 0xB2 }, { "sup3", 0xB3 }, { "sub2", 0x2082 }, { "radic", 0x221A },
    { "infin", 0x221E }, { "ne", 0x2260 }, { "le", 0x2264 }, { "ge", 0x2265 },
    { "pm", 0xB1 }, { "alpha", 0x3B1 }, { "beta", 0x3B2 }, { "gamma", 0x3B3 },
    { "delta", 0x3B4 }, { "pi", 0x3C0 }, { "sigma", 0x3C3 }, { "omega", 0x3C9 },
    { "mu", 0xB5 }, { "euro", 0x20AC }, { "pound", 0xA3 }, { "yen", 0xA5 },
    { "curren", 0xA4 }, { "cent", 0xA2 }, { "shy", 0xAD }, { "dagger", 0x2020 },
    { "Dagger", 0x2021 }, { "permil", 0x2030 }, { "prime", 0x2032 }, { "Prime", 0x2033 },
    { "larr", 0x2190 }, { "uarr", 0x2191 }, { "rarr", 0x2192 }, { "darr", 0x2193 },
    { "harr", 0x2194 }, { "hearts", 0x2665 }, { "diams", 0x2666 }, { "clubs", 0x2663 },
    { "spades", 0x2660 }, { NULL, 0 }
};

static UINT CvEntityCode(LPCSTR szName)
{
    for (const CvEntity* e = g_cvEntities; e->szName != NULL; e++)
        if (lstrcmpA(e->szName, szName) == 0) return e->uCode;
    return 0xFFFFFFFF;         // 未知
}

// 解一个实体：从 p 指向 '&' 开始。*pnUsed 返回消耗的源字符数（0 表示不是实体，原样保留）。
static UINT CvParseEntity(LPCSTR p, INT nAvail, INT* pnUsed)
{
    *pnUsed = 0;
    if (p[0] != '&' || nAvail < 3) return 0;
    INT i = 1;
    if (p[i] == '#')
    {
        i++;
        INT nHex = 0;
        if (i < nAvail && (p[i] == 'x' || p[i] == 'X')) { nHex = 1; i++; }
        LONG v = 0;
        INT nDigits = 0;
        for (; i < nAvail; i++)
        {
            if (nHex)
            {
                INT h = CvHexVal(p[i]);
                if (h < 0) break;
                v = v * 16 + h;
            }
            else
            {
                if (p[i] < '0' || p[i] > '9') break;
                v = v * 10 + (p[i] - '0');
            }
            nDigits++;
        }
        if (nDigits > 0 && i < nAvail && p[i] == ';' && v > 0)
        {
            *pnUsed = i + 1;
            return (UINT)v;
        }
        return 0;                                  // 号码缺失/无分号/负数 → 原样保留
    }
    INT nStart = i;
    while (i < nAvail && ((p[i] >= 'a' && p[i] <= 'z') || (p[i] >= 'A' && p[i] <= 'Z') ||
                          (p[i] >= '0' && p[i] <= '9'))) i++;
    if (i >= nAvail || p[i] != ';' || i == nStart) return 0;
    char name[64];
    INT nLen = i - nStart;
    if (nLen >= (INT)sizeof(name)) return 0;
    memcpy(name, p + nStart, (SIZE_T)nLen);
    name[nLen] = 0;
    UINT uCode = CvEntityCode(name);
    if (uCode == 0xFFFFFFFF) return 0;             // 未知实体名 → 原样保留
    *pnUsed = i + 1;
    return uCode;
}

// HTML 实体解码。bOne 为真时只解第一个实体、其余原样保留（原版 单字 版）；
// bWide 为真返回宽文本字节集，否则按原版转成 936 文本。输入文本按 UTF-8 解释。
// HTML 实体解码，返回 936 文本；输入文本按 UTF-8 解释。解不动的片段一律原样保留。
static void HtmlDecodeToAnsi(PMDATA_INF pRetData, LPCSTR szIn)
{
    UniWBuf w;
    if (szIn == NULL) { CvRetText(pRetData, ""); return; }
    INT n = lstrlenA(szIn);
    if (!w.Reserve(n + 1)) { CvRetText(pRetData, ""); return; }
    INT i = 0;
    while (i < n)
    {
        if (szIn[i] == '&')
        {
            INT nUsed = 0;
            UINT uCode = CvParseEntity(szIn + i, n - i, &nUsed);
            if (nUsed > 0)
            {
                WCHAR unit[2];
                unit[0] = (WCHAR)uCode; unit[1] = 0;
                w.Add(unit, 1);
                i += nUsed;
                continue;
            }
        }
        INT nRun = i;
        while (nRun < n && szIn[nRun] != '&') nRun++;
        if (nRun == i) nRun = i + 1;   // 解不动的 '&'（未知实体）按字面吃一个字节，否则原地死循环
        INT nWide = 0;
        // 参数文本可能指向程序常量段，不能就地补 0，按长度解码。
        WCHAR* frag = CvCpToWideN(szIn + i, nRun - i, CP_UTF8, &nWide);
        if (frag != NULL)
        {
            w.Add(frag, nWide);
            LocalFree(frag);
        }
        i = nRun;
    }
    char* sz = CvWideToCp(w.p, w.nLen, CV_DEFAULT_CP);
    CvRetText(pRetData, sz);
    if (sz != NULL) LocalFree(sz);
}

void UcHtmlDecodeA(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    HtmlDecodeToAnsi(pRetData, UniArgText(pArgInf[0]));
}

}  // namespace cx
