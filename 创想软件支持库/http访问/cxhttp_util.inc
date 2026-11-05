// 创想HTTP访问类【新CURL版本】「全局公开命令」用到的工具子程序，逐字迁移成 C++。
// 由 lib.cpp 在 namespace cx 内 #include（cxhttp_core.inc / cxhttp_extra.inc 之后）。
// 语义一律以 http访问\规格_全局命令.md 为准（§1~§24、P1~P15）；函数清单见 cxhttp_util.h。
// 编码口径：全部 GBK/CP936 单字节串；%XX 大写；行分隔 CRLF。
// 硬约束：无 STL / 无 new / 无异常 / 无 sprintf / 无 CRT 浮点与 64 位整数互转；
//         wininet 等入口一律运行期 LoadLibraryA + GetProcAddress（照 H_PROC 的做法）。
// 出参约定：所有写 pOut 的函数都是「追加」，调用方负责先 BufReset；
//           返回 LPBYTE 的两个函数给的是 LocalAlloc 缓冲（调用方 LocalFree），失败返回 NULL。
// GBK 安全：文本搜索/替换/分割一律按双字节边界推进（首字节 0x81~0xFE 且后继非 0 时成对跳过），
//           绝不在双字节的尾字节（0x40~0xFE 可能落在 ASCII 区）上起匹配。
#ifndef CXHTTP_UTIL_INC
#define CXHTTP_UTIL_INC

// SDK 里没有 LPCBYTE 这个 typedef，字节流的只读指针自己补一个
typedef const BYTE* LPCBYTE;

#include "cxhttp_util.h"

// ============================================================ GBK 感知的搜索原语
// 对应内置 寻找文本/倒找文本（第 4 参「不区分大小写」语义）；大小写折叠只作用 ASCII A-Z/a-z。
// 匹配一律只允许落在 GBK 字符边界上：首字节 0x81~0xFE 且后继非 0 时整对跳过，
// 绝不在尾字节（0x40~0xFE，可能长得像 ASCII）上起匹配。
static char UxFoldAscii(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

// szNeed 为空串时：按 krnln 寻找文本 的空 needle 语义，命中 nStart 位置（P2 坑注）。
// 返回 0 基字节下标，找不到返回 -1。匹配只允许落在 GBK 字符边界上。
static int UxStrIdx(LPCSTR szHay, LPCSTR szNeed, BOOL bNoCase, int nStart)
{
    if (szHay == NULL || szNeed == NULL) return -1;
    const int nNeed = lstrlenA(szNeed);
    const int nHay = lstrlenA(szHay);
    if (nStart < 0) nStart = 0;
    if (nNeed == 0) return (nStart <= nHay) ? nStart : -1;
    int i = 0;
    while (i < nHay)
    {
        const BYTE b = (BYTE)szHay[i];
        const int clen = (b >= 0x81 && b <= 0xFE && szHay[i + 1] != '\0') ? 2 : 1;
        if (i >= nStart)
        {
            BOOL bHit = TRUE;
            for (int k = 0; k < nNeed; k++)
            {
                char a = szHay[i + k];
                char c = szNeed[k];
                if (a == '\0') { bHit = FALSE; break; }
                if (bNoCase) { a = UxFoldAscii(a); c = UxFoldAscii(c); }
                if (a != c) { bHit = FALSE; break; }
            }
            if (bHit) return i;
        }
        i += clen;
    }
    return -1;
}

static LPCSTR UxStrFind(LPCSTR szHay, LPCSTR szNeed, BOOL bNoCase, int nStart)
{
    const int p = UxStrIdx(szHay, szNeed, bNoCase, nStart);
    return p < 0 ? NULL : szHay + p;
}

// 倒找文本：最后一次出现；找不到 NULL
static LPCSTR UxStrRFind(LPCSTR szHay, LPCSTR szNeed, BOOL bNoCase)
{
    LPCSTR pLast = NULL;
    int nPos = 0;
    for (;;)
    {
        LPCSTR p = UxStrFind(szHay, szNeed, bNoCase, nPos);
        if (p == NULL) break;
        pLast = p;
        nPos = (int)(p - szHay) + 1;
    }
    return pLast;
}

// 文本是否存在（P4）：默认区分大小写；要找的串空 → 假
static BOOL UxHasI(LPCSTR szHay, LPCSTR szNeed)
{
    if (szNeed == NULL || szNeed[0] == '\0') return FALSE;
    return UxStrFind(szHay, szNeed, TRUE, 0) != NULL;
}

// ============================================================ 分段（分割文本）
// 语义（规格 0.2）：src 空串 → 0 段；分隔符空串 → 1 段（整串）；其余 = 边界数 + 1（含首尾空段）。
static int UxSegCountSz(LPCSTR szSrc, LPCSTR szDelim)
{
    if (szSrc == NULL || szSrc[0] == '\0') return 0;
    if (szDelim == NULL || szDelim[0] == '\0') return 1;
    int n = 1, nPos = 0;
    for (;;)
    {
        const int p = UxStrIdx(szSrc, szDelim, FALSE, nPos);
        if (p < 0) break;
        n++;
        nPos = p + lstrlenA(szDelim);
    }
    return n;
}

typedef struct { LPCSTR pData; int nLen; } UxItem;
typedef struct { UxItem* pItems; int nCount; int nCap; } UxList;

static BOOL UxlAdd(UxList* L, LPCSTR p, int n)
{
    if (L->nCount >= L->nCap)
    {
        const int nCap = L->nCap == 0 ? 32 : L->nCap * 2;
        UxItem* pNew = (UxItem*)LocalAlloc(LPTR, (SIZE_T)nCap * sizeof(UxItem));
        if (pNew == NULL) return FALSE;
        if (L->nCount > 0) memcpy(pNew, L->pItems, (SIZE_T)L->nCount * sizeof(UxItem));
        if (L->pItems != NULL) LocalFree(L->pItems);
        L->pItems = pNew;
        L->nCap = nCap;
    }
    L->pItems[L->nCount].pData = p;
    L->pItems[L->nCount].nLen = n;
    L->nCount++;
    return TRUE;
}
static void UxlFree(UxList* L)
{
    if (L->pItems != NULL) LocalFree(L->pItems);
    L->pItems = NULL; L->nCount = 0; L->nCap = 0;
}
static void UxSplitList(LPCSTR szSrc, LPCSTR szDelim, UxList* L)
{
    L->pItems = NULL; L->nCount = 0; L->nCap = 0;
    const int nSrc = szSrc == NULL ? 0 : lstrlenA(szSrc);
    if (nSrc == 0) return;
    const int nD = szDelim == NULL ? 0 : lstrlenA(szDelim);
    if (nD == 0) { UxlAdd(L, szSrc, nSrc); return; }
    int nStart = 0;
    for (;;)
    {
        const int p = UxStrIdx(szSrc, szDelim, FALSE, nStart);
        if (p < 0) { UxlAdd(L, szSrc + nStart, nSrc - nStart); break; }
        UxlAdd(L, szSrc + nStart, p - nStart);
        nStart = p + nD;
    }
}

// ============================================================ 文本原子
// 追加字面量串：长度一律现场 lstrlenA，杜绝手写数字差一（会把 '\0' 当正文塞进缓冲，
// 缓冲内出现嵌入 NUL 后所有 C 串操作都看不见后面的内容）。
static void UxAddLit(CxBuf* b, LPCSTR szLit) { BufAppend(b, szLit, lstrlenA(szLit)); }

// 原模块：连接文本（内部工具，逐项加分隔符）
void UxAryJoin(CxBuf* pOut, LPCSTR szSep, LPCSTR const* pItems, int nCount)
{
    const int nSep = szSep == NULL ? 0 : lstrlenA(szSep);
    for (int i = 0; i < nCount; i++)
    {
        if (i > 0 && nSep > 0) BufAppend(pOut, szSep, nSep);
        LPCSTR sz = (pItems == NULL || pItems[i] == NULL) ? "" : pItems[i];
        BufAppend(pOut, sz, lstrlenA(sz));
    }
}

// 原模块：子文本替换（全量、区分大小写；被替串空 → 原样拷贝，照 krnln 保守处理）
static void UxReplaceCore(CxBuf* pOut, LPCSTR szSrc, LPCSTR szFrom, LPCSTR szTo, BOOL bNoCase)
{
    if (szSrc == NULL) return;
    const int nSrc = lstrlenA(szSrc);
    const int nFrom = szFrom == NULL ? 0 : lstrlenA(szFrom);
    if (nFrom <= 0) { BufAppend(pOut, szSrc, nSrc); return; }
    const int nTo = szTo == NULL ? 0 : lstrlenA(szTo);
    int nPos = 0;
    for (;;)
    {
        const int p = UxStrIdx(szSrc, szFrom, bNoCase, nPos);
        if (p < 0) { BufAppend(pOut, szSrc + nPos, nSrc - nPos); break; }
        BufAppend(pOut, szSrc + nPos, p - nPos);
        if (nTo > 0) BufAppend(pOut, szTo, nTo);
        nPos = p + nFrom;
    }
}
void UxReplaceAll(CxBuf* pOut, LPCSTR szSrc, LPCSTR szFrom, LPCSTR szTo)
{
    UxReplaceCore(pOut, szSrc, szFrom, szTo, FALSE);
}
void UxReplaceAllNoCase(CxBuf* pOut, LPCSTR szSrc, LPCSTR szFrom, LPCSTR szTo)
{
    UxReplaceCore(pOut, szSrc, szFrom, szTo, TRUE);
}

// 原模块：到小写 / 到大写（仅 ASCII A-Z/a-z，与 krnln 一致）
void UxLower(CxBuf* pOut, LPCSTR sz)
{
    if (sz == NULL) return;
    for (int i = 0; sz[i] != '\0'; i++)
    {
        char c = sz[i];
        if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
        BufAppend(pOut, &c, 1);
    }
}
void UxUpper(CxBuf* pOut, LPCSTR sz)
{
    if (sz == NULL) return;
    for (int i = 0; sz[i] != '\0'; i++)
    {
        char c = sz[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        BufAppend(pOut, &c, 1);
    }
}

static BOOL UxIsBlank(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

// 原模块：删首尾空（0x20/09/0D/0A；这些字节都 <0x40，绝不会是 GBK 尾字节，逐字节安全）
void UxTrim(CxBuf* pOut, LPCSTR sz)
{
    if (sz == NULL) return;
    const int n = lstrlenA(sz);
    int a = 0, b = n;
    while (a < b && UxIsBlank(sz[a])) a++;
    while (b > a && UxIsBlank(sz[b - 1])) b--;
    if (b > a) BufAppend(pOut, sz + a, b - a);
}
static void UxTrimSlice(LPCSTR* pp, int* pn)
{
    while (*pn > 0 && UxIsBlank(*pp[0])) { (*pp)++; (*pn)--; }
    while (*pn > 0 && UxIsBlank((*pp)[*pn - 1])) (*pn)--;
}

// 原模块：删全部空 —— 有意比 krnln 多删 制表/CR/LF（规格 §1 坑「可修 bug」，任务书拍板）；
// 全角空格 GBK A1A1 只在双字节边界上识别，尾字节 A1 不与下一字形首字节 A1 误拼。
void UxNoSpace(CxBuf* pOut, LPCSTR sz)
{
    if (sz == NULL) return;
    for (int i = 0; sz[i] != '\0'; )
    {
        const BYTE b = (BYTE)sz[i];
        if (b == ' ' || b == '\t' || b == '\r' || b == '\n') { i++; continue; }
        if (b >= 0x81 && b <= 0xFE && sz[i + 1] != '\0')
        {
            if (b == 0xA1 && (BYTE)sz[i + 1] == 0xA1) { i += 2; continue; }
            BufAppend(pOut, sz + i, 2);
            i += 2;
            continue;
        }
        BufAppend(pOut, sz + i, 1);
        i++;
    }
}

// 原模块：文本是否存在（P4，区分大小写）
BOOL UxContains(LPCSTR szHay, LPCSTR szNeed)
{
    if (szHay == NULL || szNeed == NULL || szNeed[0] == '\0') return FALSE;
    return UxStrFind(szHay, szNeed, FALSE, 0) != NULL;
}

// 原模块：文本_取右边（P3）：最后一个 标记 之后的全部内容；找不到空串
void UxRightOf(CxBuf* pOut, LPCSTR szSrc, LPCSTR szMark)
{
    if (szSrc == NULL || szMark == NULL || szMark[0] == '\0') return;
    LPCSTR pLast = UxStrRFind(szSrc, szMark, FALSE);
    if (pLast != NULL) BufAppend(pOut, pLast + lstrlenA(szMark), (int)lstrlenA(pLast + lstrlenA(szMark)));
}
static void UxRightOfI(CxBuf* pOut, LPCSTR szSrc, LPCSTR szMark)   // 不区分大小写版（§5 用）
{
    if (szSrc == NULL || szMark == NULL || szMark[0] == '\0') return;
    LPCSTR pLast = UxStrRFind(szSrc, szMark, TRUE);
    if (pLast != NULL) BufAppend(pOut, pLast + lstrlenA(szMark), (int)lstrlenA(pLast + lstrlenA(szMark)));
}

// 原模块：文本_取出中间文本（P2）：第一个 左标记 之后到其后第一个 右标记 之前；缺一即空
BOOL UxBetween(CxBuf* pOut, LPCSTR szSrc, LPCSTR szLeft, LPCSTR szRight)
{
    if (szSrc == NULL) return FALSE;
    int pL = UxStrIdx(szSrc, szLeft, FALSE, 0);
    if (pL < 0) return FALSE;
    pL += lstrlenA(szLeft);
    const int pR = UxStrIdx(szSrc, szRight, FALSE, pL);
    if (pR < 0) return FALSE;
    if (pR > pL) BufAppend(pOut, szSrc + pL, pR - pL);
    return TRUE;
}
static BOOL UxBetweenI(CxBuf* pOut, LPCSTR szSrc, LPCSTR szLeft, LPCSTR szRight) // 不区分（§5 用）
{
    if (szSrc == NULL) return FALSE;
    int pL = UxStrIdx(szSrc, szLeft, TRUE, 0);
    if (pL < 0) return FALSE;
    pL += lstrlenA(szLeft);
    const int pR = UxStrIdx(szSrc, szRight, TRUE, pL);
    if (pR < 0) return FALSE;
    if (pR > pL) BufAppend(pOut, szSrc + pL, pR - pL);
    return TRUE;
}

// 原模块：分割文本 取第 nWant 段（0 基）写进 pOutItem；越界 → 什么都不写。
// chSep=='\0' 对应 分割文本 分隔符为空 → 整串是唯一的 1 段。
void UxSplit(CxBuf* pOutItem, LPCSTR szSrc, char chSep, int nWant)
{
    if (nWant < 0 || szSrc == NULL || pOutItem == NULL) return;
    const int nSrc = lstrlenA(szSrc);
    if (nSrc == 0) return;                       // 分割文本("") → 0 成员，任何下标都越界
    if (chSep == '\0')
    {
        if (nWant == 0) BufAppend(pOutItem, szSrc, nSrc);
        return;
    }
    char szSep[2];
    szSep[0] = chSep; szSep[1] = '\0';
    int nStart = 0;
    for (int k = 0; k < nWant; k++)
    {
        const int p = UxStrIdx(szSrc, szSep, FALSE, nStart);
        if (p < 0) return;                        // 段数不够 nWant
        nStart = p + 1;
    }
    const int pNext = UxStrIdx(szSrc, szSep, FALSE, nStart);
    const int nEnd = pNext < 0 ? nSrc : pNext;
    if (nEnd > nStart) BufAppend(pOutItem, szSrc + nStart, nEnd - nStart);
}
int UxSplitCount(LPCSTR szSrc, char chSep)
{
    if (chSep == '\0')
        return (szSrc != NULL && szSrc[0] != '\0') ? 1 : 0;
    char szSep[2];
    szSep[0] = chSep; szSep[1] = '\0';
    return UxSegCountSz(szSrc, szSep);
}

// 原模块：编码_转Unicode / Unicode到Ansi（P10）：GBK(936) 与 UTF-8(65001) 互转。
// 有意修正 P10 的坑：长度一律按目标码页本身求（原模块按 ACP 求长度再按 原始编码 转换，缓冲可能偏小）。
static void UxConvText(CxBuf* pOut, LPCSTR sz, UINT uSrcCp, UINT uDstCp)
{
    if (sz == NULL || sz[0] == '\0' || pOut == NULL) return;
    const int nW = MultiByteToWideChar(uSrcCp, 0, sz, -1, NULL, 0);
    if (nW <= 1) return;
    LPWSTR pW = (LPWSTR)LocalAlloc(LPTR, (SIZE_T)nW * sizeof(WCHAR));
    if (pW == NULL) return;
    if (MultiByteToWideChar(uSrcCp, 0, sz, -1, pW, nW) > 0)
    {
        const int nA = WideCharToMultiByte(uDstCp, 0, pW, -1, NULL, 0, NULL, NULL);
        if (nA > 1)
        {
            char* pA = (char*)LocalAlloc(LPTR, nA);
            if (pA != NULL && WideCharToMultiByte(uDstCp, 0, pW, -1, pA, nA, NULL, NULL) > 0)
                BufAppend(pOut, pA, nA - 1);       // 去掉结尾 NUL
            if (pA != NULL) LocalFree(pA);
        }
    }
    LocalFree(pW);
}
void UxGbkToUtf8(CxBuf* pOut, LPCSTR szGbk) { UxConvText(pOut, szGbk, 936, 65001); }
void UxUtf8ToGbk(CxBuf* pOut, LPCSTR szUtf8) { UxConvText(pOut, szUtf8, 65001, 936); }

// 原模块：编码_URL编码（P1，逐字）：unreserved = '-' '.' '0-9' 'A-Z' 'a-z'，'/' 也编码；
// bKeepAlnum 假 → 逐字节全百分号编码；%XX 十六进制大写；bToUtf8 真 → 先 GBK→UTF-8 再编码。
void UxUrlEncode(CxBuf* pOut, LPCSTR szText, BOOL bKeepAlnum, BOOL bToUtf8)
{
    if (szText == NULL || szText[0] == '\0' || pOut == NULL) return;
    static const char* const kHex = "0123456789ABCDEF";
    CxBuf src;
    BufInit(&src, lstrlenA(szText) * 3 + 1);
    if (bToUtf8)
    {
        UxConvText(&src, szText, 936, 65001);
        // 转换失败（空结果）时照原模块兜不住的情况回退：直接按 GBK 原字节编码
        if (src.pData == NULL || src.nLen == 0)
        {
            BufReset(&src);
            BufAppend(&src, szText, lstrlenA(szText));
        }
    }
    else
    {
        BufAppend(&src, szText, lstrlenA(szText));
    }
    for (int i = 0; i < src.nLen; i++)
    {
        const BYTE b = (BYTE)src.pData[i];
        // P1 保留集判定原样：b∈(44,58) 且 ≠47，或 A-Z，或 a-z
        const BOOL bKeep = bKeepAlnum &&
            ((b > 44 && b < 58 && b != 47) || (b > 64 && b < 91) || (b > 96 && b < 123));
        if (bKeep) BufAppend(pOut, (LPCSTR)&b, 1);
        else
        {
            char szEsc[4];
            szEsc[0] = '%'; szEsc[1] = kHex[b >> 4]; szEsc[2] = kHex[b & 0xF]; szEsc[3] = '\0';
            BufAppend(pOut, szEsc, 3);
        }
    }
    BufFree(&src);
}

// 原模块：文本_取出现次数（§18）：不重叠计数。原模块可空的「是否区分大小写」省略→假→
// 取反→**默认不区分大小写**（坑注：命名易误解）；本库把该参显式化为 bCase，
// 调用方在参数省略时按原语义传 FALSE（=不区分）。
int UxCountOccur(LPCSTR szSrc, LPCSTR szSub, BOOL bCase)
{
    if (szSrc == NULL || szSub == NULL || szSrc[0] == '\0' || szSub[0] == '\0') return 0;
    const int nSub = lstrlenA(szSub);
    int nCount = 0, nPos = 0;
    for (;;)
    {
        // UxStrIdx 第 3 参是「不区分大小写」：bCase=真(区分) 时必须传 FALSE
        const int p = UxStrIdx(szSrc, szSub, bCase ? FALSE : TRUE, nPos);
        if (p < 0) break;
        nCount++;
        nPos = p + nSub;                          // 命中位置+子串长度继续 → 不重叠
    }
    return nCount;
}

// 原模块：文本_取随机字符（P13）：从 0-9A-Za-z 均匀取 nLen 个。
// 熵源：GetTickCount + QueryPerformanceCounter + 栈地址，xorshift32 推进；不用 CRT rand（静态序列问题）。
void UxRandChars(CxBuf* pOut, int nLen)
{
    static const char kPool[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    if (pOut == NULL || nLen <= 0) return;
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    unsigned int s = (unsigned int)GetTickCount()
        ^ ((unsigned int)li.LowPart << 1) ^ ((unsigned int)li.HighPart << 5)
        ^ (unsigned int)(UINT_PTR)&li ^ 0x9E3779B9u;
    if (s == 0) s = 0x1234ABCDu;
    for (int i = 0; i < nLen; i++)
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;  // xorshift32
        BufAppend(pOut, &kPool[s % 62], 1);
    }
}

// ============================================================ 网址拆解
// 原模块：网页_取域名（P5，"/"方式：小写、补斜杠、去协议、截到第一个 /、过滤端口）。
// bStripPort=假 时保留 :端口（供 网页_取端口 用，对应原模块 不过滤端口 传真的调用）。
static void UxHostRaw(CxBuf* pOut, LPCSTR szUrl, BOOL bStripPort)
{
    if (szUrl == NULL || szUrl[0] == '\0') return;
    CxBuf low, host;
    BufInit(&low, lstrlenA(szUrl) + 16);
    BufInit(&host, 64);
    UxLower(&low, szUrl);
    if (low.pData == NULL || low.nLen == 0) { BufFree(&host); BufFree(&low); return; }
    if (low.pData[low.nLen - 1] != '/') BufAppend(&low, "/", 1);
    if (low.nLen >= 9 && memcmp(low.pData, "https://", 8) == 0)
        UxBetween(&host, low.pData, "https://", "/");
    else
    {
        if (UxStrFind(low.pData, "http://", FALSE, 0) == NULL)
        {
            CxBuf t; BufInit(&t, low.nLen + 10);
            BufAppend(&t, "http://", 7);
            BufAppend(&t, low.pData, low.nLen);
            BufFree(&low);
            if (t.pData != NULL) { low = t; t.pData = NULL; t.nLen = t.nCap = 0; }
            else { BufFree(&host); BufFree(&low); return; }
        }
        UxBetween(&host, low.pData, "http://", "/");
    }
    if (bStripPort && host.pData != NULL && host.nLen > 0)
    {
        const int p = UxStrIdx(host.pData, ":", FALSE, 0);
        if (p >= 0 && p < host.nLen) host.nLen = p;   // 截断到冒号左
        if (host.nLen > 0) BufAppend(pOut, host.pData, host.nLen);
    }
    else if (host.pData != NULL && host.nLen > 0)
    {
        BufAppend(pOut, host.pData, host.nLen);
    }
    BufFree(&host); BufFree(&low);
}

// 原模块：网页_取域名（去协议头/路径/端口；异常输入返回空串）
void UxHostOf(CxBuf* pOut, LPCSTR szUrl)
{
    UxHostRaw(pOut, szUrl, TRUE);
}

// 原模块：网页_取页面地址——本库按 cxhttp_util.h 定稿语义：? 之前的部分（含协议与域名），无 ? 原样
void UxPageOf(CxBuf* pOut, LPCSTR szUrl)
{
    if (szUrl == NULL) return;
    const int n = lstrlenA(szUrl);
    const int p = UxStrIdx(szUrl, "?", FALSE, 0);
    if (p < 0) BufAppend(pOut, szUrl, n);
    else if (p > 0) BufAppend(pOut, szUrl, p);
}

// 原模块：网页_取端口（P15）：先按 不过滤端口 取域名段，倒找 ':' 有则返回其右侧数字
// （到整数("abc")=0，命中冒号就返回、不再落 443/80，照抄原模块）；无冒号时
// 取文本左边(到大写(网址),5)="HTTPS" → 443，其余 → 80。
int UxPortOf(LPCSTR szUrl)
{
    if (szUrl == NULL) return 80;
    CxBuf hp;
    BufInit(&hp, 64);
    UxHostRaw(&hp, szUrl, FALSE);
    BOOL bHasColon = FALSE;
    int nPort = 0;
    if (hp.pData != NULL && hp.nLen > 0)
    {
        LPCSTR pColon = UxStrRFind(hp.pData, ":", FALSE);
        if (pColon != NULL)
        {
            bHasColon = TRUE;
            LPCSTR p = pColon + 1;
            while (*p >= '0' && *p <= '9') { nPort = nPort * 10 + (*p - '0'); p++; }
        }
    }
    BufFree(&hp);
    if (bHasColon) return nPort;
    static const char kHttps[] = "HTTPS";   // 到大写后比前 5 字符；不足 5 字符 → 80（取文本左边原样短串不等于 HTTPS）
    int k = 0;
    while (k < 5 && szUrl[k] != '\0')
    {
        char c = szUrl[k];
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        if (c != kHttps[k]) return 80;
        k++;
    }
    return (k == 5) ? 443 : 80;
}

// ============================================================ 协议头
// 原模块：网页_构建协议头（§7，顺序/默认值/大小写判定完全照抄）：
//  · 末尾无 CRLF 先补 CRLF；Referer:*** 两段删除（先带换行再裸串，均不区分大小写）；
//  · Accept/Host/Connection/User-Agent/Accept-Language/Cookie/Accept-Encoding 逐项「不含才补」，
//    存在性判定 = 整段里不区分大小写地找 "XXX:" 子串；
//  · Referer 的默认补全原模块就注释掉了——这里不补（别自作主张）；
//  · POST 补 Content-Type: application/x-www-form-urlencoded；POST 长度 >0 才补 Content-Length；
//  · 行分隔一律 CRLF，返回 GBK 文本。
void UxBuildHeaders(CxBuf* pOut, LPCSTR szExtra, LPCSTR szUrl, LPCSTR szCookie,
    BOOL bPost, BOOL bGzip, int nPostLen)
{
    if (pOut == NULL) return;
    CxBuf w, t;
    BufInit(&w, szExtra == NULL ? 512 : lstrlenA(szExtra) + 512);
    BufInit(&t, 256);
    if (szExtra != NULL) BufAppend(&w, szExtra, lstrlenA(szExtra));
    if (w.pData == NULL) { BufFree(&t); BufFree(&w); return; }
    // 2. 末尾换行：取文本右边(x,2) ≠ CRLF → 补
    if (!(w.nLen >= 2 && w.pData[w.nLen - 2] == '\r' && w.pData[w.nLen - 1] == '\n'))
        BufAppend(&w, "\r\n", 2);
    // 3. Referer:*** 魔法值（亚马逊防访问错误：代表不提交 Referer）
    if (UxHasI(w.pData, "Referer:***"))
    {
        BufReset(&t);
        UxReplaceAllNoCase(&t, w.pData, "Referer:***\r\n", "");
        BufReset(&w);
        BufAppend(&w, t.pData, t.nLen);
        BufReset(&t);
        UxReplaceAllNoCase(&t, w.pData, "Referer:***", "");
        BufReset(&w);
        BufAppend(&w, t.pData, t.nLen);
    }
    // 4. Accept
    if (!UxHasI(w.pData, "Accept:")) UxAddLit(&w, "Accept: */*\r\n");
    // 5. Host（网页_取域名 单参：/方式、不去二级、过滤端口、小写）
    if (!UxHasI(w.pData, "Host:"))
    {
        CxBuf host; BufInit(&host, 64);
        UxHostOf(&host, szUrl);
        UxAddLit(&w, "Host: ");
        if (host.pData != NULL && host.nLen > 0) BufAppend(&w, host.pData, host.nLen);
        UxAddLit(&w, "\r\n");
        BufFree(&host);
    }
    // 6. Connection
    if (!UxHasI(w.pData, "Connection:")) UxAddLit(&w, "Connection: Keep-Alive\r\n");
    // 7. User-Agent（#HTTP配置_默认浏览器UA，自带前缀）
    if (!UxHasI(w.pData, "User-Agent:"))
    {
        UxAddLit(&w, "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/50.0.2661.87 Safari/537.36\r\n");
    }
    // 8. Accept-Language
    if (!UxHasI(w.pData, "Accept-Language:")) UxAddLit(&w, "Accept-Language: zh-CN\r\n");
    // 9. Cookie（提交COOKIE 非空且头里没有才补）
    if (szCookie != NULL && szCookie[0] != '\0' && !UxHasI(w.pData, "Cookie:"))
    {
        UxAddLit(&w, "Cookie: ");
        BufAppend(&w, szCookie, lstrlenA(szCookie));
        UxAddLit(&w, "\r\n");
    }
    // 10. Accept-Encoding（是否采用GZIP 为真才补）
    if (bGzip && !UxHasI(w.pData, "Accept-Encoding:")) UxAddLit(&w, "Accept-Encoding: gzip\r\n");
    // 11. POST
    if (bPost)
    {
        if (!UxHasI(w.pData, "Content-Type:"))
            UxAddLit(&w, "Content-Type: application/x-www-form-urlencoded\r\n");
        if (nPostLen > 0 && !UxHasI(w.pData, "Content-Length:"))
        {
            char szT[48];
            wsprintfA(szT, "Content-Length: %d\r\n", nPostLen);   // 到文本(整数)
            BufAppend(&w, szT, lstrlenA(szT));
        }
    }
    BufAppend(pOut, w.pData, w.nLen);
    BufFree(&t); BufFree(&w);
}

// 原模块：网页_提取头（§6）：行分割 CRLF→(≤1段)CR→(≤1段)LF；
// 命中第一个「不区分大小写含字段名」的行 → 该行最后一个 ':' 右侧并 trim；全不命中空串。
void UxPickHeader(CxBuf* pOut, LPCSTR szHead, LPCSTR szName)
{
    if (szHead == NULL || szName == NULL || szName[0] == '\0') return;
    LPCSTR delim = "\r\n";
    if (UxSegCountSz(szHead, delim) <= 1)
    {
        delim = "\r";
        if (UxSegCountSz(szHead, delim) <= 1) delim = "\n";
    }
    UxList L;
    UxSplitList(szHead, delim, &L);
    CxBuf line, right;
    BufInit(&line, 256);
    BufInit(&right, 128);
    for (int i = 0; i < L.nCount; i++)
    {
        BufReset(&line);
        if (L.pItems[i].nLen > 0) BufAppend(&line, L.pItems[i].pData, L.pItems[i].nLen);
        if (line.pData == NULL) continue;
        if (UxHasI(line.pData, szName))
        {
            BufReset(&right);
            UxRightOf(&right, line.pData, ":");
            if (right.pData != NULL)
            {
                CxBuf val; BufInit(&val, right.nLen + 1);
                UxTrim(&val, right.pData);
                if (val.pData != NULL && val.nLen > 0) BufAppend(pOut, val.pData, val.nLen);
                BufFree(&val);
            }
            break;
        }
    }
    BufFree(&right); BufFree(&line); UxlFree(&L);
}

// 原模块：判断响应头是否有效（§9）：文本是否存在(响应头,"200 OK")，默参区分大小写 → 照抄。
BOOL UxHeadIsOk(LPCSTR szHead)
{
    if (szHead == NULL) return FALSE;
    return UxContains(szHead, "200 OK");
}

// 原模块：取响应头代码（§10）：HTTP/1.1→1.0→2.0→3.0 前缀逐级降级找「前缀+空格」之间的码。
// 原模块声明返回逻辑型却返回文本，属源码 bug；本库按文本返回（cxhttp_util.h 已注明）。
void UxHeadCodeText(CxBuf* pOut, LPCSTR szHead)
{
    if (szHead == NULL) return;
    static const char* const kPref[] = { "HTTP/1.1 ", "HTTP/1.0 ", "HTTP/2.0 ", "HTTP/3.0 " };
    for (int i = 0; i < 4; i++)
    {
        CxBuf t; BufInit(&t, 16);
        UxBetween(&t, szHead, kPref[i], " ");       // 区分大小写（第5参省略→假）
        BOOL bHit = (t.pData != NULL && t.nLen > 0);
        if (bHit) BufAppend(pOut, t.pData, t.nLen);
        BufFree(&t);
        if (bHit) return;
    }
}

// 原模块：网页_取返回协议代码（§11）：正则 (\d{3})\s+([A-Za-z ]+)? 的首个匹配第 1 子匹配。
// 手写等价匹配：最左的「边界处 3 位 ASCII 数字 + 至少 1 个空白」；不区分大小写对数字无意义。
// 返回三位数码文本；原因短语（第 2 子匹配）不在本函数出参内（cxhttp_util.h 未给参考槽，调用方另取）。
void UxStatusCode(CxBuf* pOut, LPCSTR szHead)
{
    if (szHead == NULL || pOut == NULL) return;
    int i = 0;
    while (szHead[i] != '\0')
    {
        const BYTE b = (BYTE)szHead[i];
        const int clen = (b >= 0x81 && b <= 0xFE && szHead[i + 1] != '\0') ? 2 : 1;
        if (clen == 1 && b >= '0' && b <= '9'
            && szHead[i + 1] >= '0' && szHead[i + 1] <= '9'
            && szHead[i + 2] >= '0' && szHead[i + 2] <= '9'
            && (szHead[i + 3] == ' ' || szHead[i + 3] == '\t' || szHead[i + 3] == '\r'
                || szHead[i + 3] == '\n' || szHead[i + 3] == '\v' || szHead[i + 3] == '\f'))
        {
            BufAppend(pOut, szHead + i, 3);
            return;
        }
        i += clen;
    }
}

// ============================================================ Cookie
// 原模块：网页_提取返回头COOKIE（§5）：行分割 CRLF→(=1段)LF→(=1段)CR（注意与 §6 顺序不同）；
// 行内不区分大小写找 "Set-Cookie"；有 ';' 取「Set-Cookie: … ;」中间（不区分），否则取最后一个
// "Set-Cookie:" 右边（不区分）；trim 后丢弃 ""、"-"、以 "=-" 结尾的伪条目；每项带 ';'。
void UxCookieFromHead(CxBuf* pOut, LPCSTR szHead)
{
    if (szHead == NULL || pOut == NULL) return;
    LPCSTR delim = "\r\n";
    if (UxSegCountSz(szHead, delim) == 1)
    {
        delim = "\n";
        if (UxSegCountSz(szHead, delim) == 1) delim = "\r";
    }
    UxList L;
    UxSplitList(szHead, delim, &L);
    CxBuf line, raw;
    BufInit(&line, 256);
    BufInit(&raw, 128);
    for (int i = 0; i < L.nCount; i++)
    {
        BufReset(&line);
        if (L.pItems[i].nLen > 0) BufAppend(&line, L.pItems[i].pData, L.pItems[i].nLen);
        if (line.pData == NULL || line.nLen == 0) continue;
        if (UxStrFind(line.pData, "Set-Cookie", TRUE, 0) == NULL) continue;   // 寻找文本 不区分
        BufReset(&raw);
        if (UxContains(line.pData, ";"))                                      // 文本是否存在 区分大小写
            UxBetweenI(&raw, line.pData, "Set-Cookie:", ";");
        else
            UxRightOfI(&raw, line.pData, "Set-Cookie:");
        if (raw.pData == NULL || raw.nLen == 0) continue;
        LPCSTR p = raw.pData; int n = raw.nLen;
        UxTrimSlice(&p, &n);                                                  // 删首尾空
        BOOL bBad = FALSE;
        if (n == 0) bBad = TRUE;
        else if (n == 1 && p[0] == '-') bBad = TRUE;                          // 值 ≠ "-"
        else if (n >= 2 && p[n - 2] == '=' && p[n - 1] == '-') bBad = TRUE;   // 右边2 ≠ "=-"
        if (!bBad)
        {
            BufAppend(pOut, p, n);
            BufAppend(pOut, ";", 1);
        }
    }
    BufFree(&raw); BufFree(&line); UxlFree(&L);
}

// 取第一个 '=' 之前的 Cookie 名（内部_Cookie取名，P6）：去首尾空；无 '=' 整串。
static void UxCookieNameOf(LPCSTR p, int n, LPCSTR* ppName, int* pNameLen)
{
    int i = 0;
    while (i < n)
    {
        const BYTE b = (BYTE)p[i];
        if (b == '=') break;
        i += (b >= 0x81 && b <= 0xFE && i + 1 < n && p[i + 1] != '\0') ? 2 : 1;
    }
    *ppName = p;
    *pNameLen = i;
    UxTrimSlice(ppName, pNameLen);
}

static BOOL UxItemEq(const UxItem& a, const UxItem& b)
{
    if (a.nLen != b.nLen) return FALSE;
    if (a.nLen == 0) return TRUE;
    return memcmp(a.pData, b.pData, (SIZE_T)a.nLen) == 0;
}
// 取文本右边(item,N) 的等值判断：长度不足 N 时 krnln 返回整串 → 只有恰等长才可能命中
static BOOL UxItemSuffixEq(const UxItem& it, LPCSTR szSfx)
{
    const int k = lstrlenA(szSfx);
    return it.nLen >= k && memcmp(it.pData + it.nLen - k, szSfx, (SIZE_T)k) == 0;
}

// 原模块：网页_Cookie合并更新_内部（§1 + P6~P8）。规则：
//  · 先对旧/新串做「删全部空」清洗；按 ';' 分割；
//  · 旧组里「新组没有同名（只看 cookie 名、大小写敏感、域/路径不参与）」的项补进新组；
//  · 丢弃结尾恰为 "=delete"/"=deleted" 的项；每项后带 ';'；
//  · 最后按【整项文本】去重（保留首现顺序，P8），并去掉拼接串末尾的一个 ';'
//    （因每段都以 ';' 进串、尾段为空段，净效果仍是「每项都带 ';'」）。
// 本函数实现的是「内部版」规则（§23 网页_取Cookie_内部 依赖的引用回写语义由调用方完成）。
// 另一条公开命令 COOKIE合并更新（§22）规则不同：不清洗空格、本次值为空即删除该项、
// 新名追加到串尾、且「定位旧项不区分大小写、替换却区分大小写」——由调用方按 §22 另行处理，
// 且注意 §22 依赖「本次COOKIE 以 ; 结尾」否则丢最后一段（源码 bug，规格里写明）。
void UxCookieMerge(CxBuf* pOut, LPCSTR szOld, LPCSTR szNew)
{
    if (pOut == NULL) return;
    if (szOld == NULL) szOld = "";
    if (szNew == NULL) szNew = "";
    // 1. 删全部空
    CxBuf oN, nN;
    BufInit(&oN, lstrlenA(szOld) + 1);
    BufInit(&nN, lstrlenA(szNew) + 1);
    UxNoSpace(&oN, szOld);
    UxNoSpace(&nN, szNew);
    if (oN.pData == NULL || nN.pData == NULL) { BufFree(&nN); BufFree(&oN); return; }
    // 2. 分割
    UxList A, B;
    UxSplitList(oN.pData, ";", &A);
    UxSplitList(nN.pData, ";", &B);
    // 3. 旧组里名字不在新组的补进来（大小写敏感，只看名）
    for (int i = 0; i < A.nCount; i++)
    {
        LPCSTR pNameA; int nNameA;
        UxCookieNameOf(A.pItems[i].pData, A.pItems[i].nLen, &pNameA, &nNameA);
        BOOL bExist = FALSE;
        for (int j = 0; j < B.nCount; j++)
        {
            LPCSTR pNameB; int nNameB;
            UxCookieNameOf(B.pItems[j].pData, B.pItems[j].nLen, &pNameB, &nNameB);
            if (nNameA == nNameB && (nNameA == 0 || memcmp(pNameA, pNameB, (SIZE_T)nNameA) == 0))
            { bExist = TRUE; break; }
        }
        if (!bExist) UxlAdd(&B, A.pItems[i].pData, A.pItems[i].nLen);
    }
    // 4~5. 剔除删除标记，每项带 ';'
    CxBuf mid;
    BufInit(&mid, nN.nLen + oN.nLen + B.nCount + 1);
    for (int j = 0; j < B.nCount; j++)
    {
        if (UxItemSuffixEq(B.pItems[j], "=deleted") || UxItemSuffixEq(B.pItems[j], "=delete"))
            continue;
        if (B.pItems[j].nLen > 0) BufAppend(&mid, B.pItems[j].pData, B.pItems[j].nLen);
        BufAppend(&mid, ";", 1);
    }
    // 6. 整项去重（P8：保留首现顺序；末段空串也会作为「首见」进串，故尾 ';' 得以保留）
    UxlFree(&A);
    if (mid.pData != NULL)
    {
        UxList D, S;
        UxSplitList(mid.pData, ";", &D);
        S.pItems = NULL; S.nCount = 0; S.nCap = 0;
        for (int i = 0; i < D.nCount; i++)
        {
            BOOL bSeen = FALSE;
            for (int j = 0; j < S.nCount; j++)
                if (UxItemEq(S.pItems[j], D.pItems[i])) { bSeen = TRUE; break; }
            if (bSeen) continue;
            UxlAdd(&S, D.pItems[i].pData, D.pItems[i].nLen);
            if (D.pItems[i].nLen > 0) BufAppend(pOut, D.pItems[i].pData, D.pItems[i].nLen);
            BufAppend(pOut, ";", 1);
        }
        // 取文本左边(局_文本, 长度-1)：去掉最后一个 ';'；空串不动
        if (pOut->pData != NULL && pOut->nLen > 0)
        {
            pOut->nLen--;
            pOut->pData[pOut->nLen] = '\0';
        }
        UxlFree(&S); UxlFree(&D);
    }
    BufFree(&mid); UxlFree(&B); BufFree(&nN); BufFree(&oN);
}

// 原模块：网页_取单条Cookie（§24）：补尾 ';' 后按「名称=」区分大小写找第一个命中；
// 「不附带名称」可省略→假，故本函数返回 名称=值；未找到（或值为空）返回空串。
void UxCookieOne(CxBuf* pOut, LPCSTR szCookie, LPCSTR szName)
{
    if (pOut == NULL || szCookie == NULL || szName == NULL || szName[0] == '\0') return;
    CxBuf c, key, t;
    BufInit(&c, lstrlenA(szCookie) + 2);
    UxTrim(&c, szCookie);
    if (c.pData == NULL) { BufFree(&t); BufFree(&key); BufFree(&c); return; }
    if (c.nLen == 0 || c.pData[c.nLen - 1] != ';') BufAppend(&c, ";", 1);
    BufInit(&key, lstrlenA(szName) + 2);
    BufAppend(&key, szName, lstrlenA(szName));
    BufAppend(&key, "=", 1);
    BufInit(&t, 64);
    if (UxBetween(&t, c.pData, key.pData, ";") && t.pData != NULL && t.nLen > 0)
    {
        BufAppend(pOut, szName, lstrlenA(szName));
        BufAppend(pOut, "=", 1);
        BufAppend(pOut, t.pData, t.nLen);
    }
    BufFree(&t); BufFree(&key); BufFree(&c);
}

// wininet 入口运行期解析（照 cxhttp_core.inc H_PROC 的路子；缓存 HMODULE，拿不到不报错不崩）
typedef BOOL (WINAPI *fn_InternetGetCookieA_t)(LPCSTR, LPCSTR, LPSTR, LPDWORD);
typedef BOOL (WINAPI *fn_InternetGetCookieExA_t)(LPCSTR, LPCSTR, LPSTR, LPDWORD, DWORD, LPVOID);
static HMODULE g_hUxWinInet = NULL;
static BOOL g_bUxWinInetTried = FALSE;
static HMODULE UxWinInetDll(void)
{
    if (!g_bUxWinInetTried)
    {
        g_bUxWinInetTried = TRUE;
        g_hUxWinInet = LoadLibraryA("wininet.dll");
    }
    return g_hUxWinInet;
}

// 原模块：网页_取Cookie_内部（§23）：http(s)://+域名，InternetGetCookieA + InternetGetCookieExA
// （flag 8192 = INTERNET_COOKIE_HTTPONLY）各 3000 字节缓冲，结果按 §1 内部版规则合并（Ex 赢），
// 名称非空再走 网页_取单条Cookie。与原版差异（均为规格坑注里点名的修法）：
//  · lpszCookieName 传 NULL（原版传 字符(0)，MSDN 语义里 NULL 才是「取全部」）；
//  · 缓冲先清零而不是清空格，按 API 真实的 in/out DWORD 传地址；
//  · 拿不到 wininet 或 InternetGetCookieA 入口 → 返回空串，不报错；只有 Ex 缺失时降级用 A 的结果。
void UxCookieFromUrl(CxBuf* pOut, LPCSTR szUrlHost, LPCSTR szName, BOOL bHttps)
{
    if (pOut == NULL) return;
    if (szUrlHost == NULL || szUrlHost[0] == '\0') return;
    if (lstrlenA(szUrlHost) > 1900) return;      // 防 wsprintfA 越界：域名超长直接给空串
    HMODULE h = UxWinInetDll();
    if (h == NULL) return;
    fn_InternetGetCookieA_t pfnA =
        (fn_InternetGetCookieA_t)GetProcAddress(h, "InternetGetCookieA");
    fn_InternetGetCookieExA_t pfnEx =
        (fn_InternetGetCookieExA_t)GetProcAddress(h, "InternetGetCookieExA");
    if (pfnA == NULL && pfnEx == NULL) return;

    char szUrl[2048];
    wsprintfA(szUrl, "%s%s", bHttps ? "https://" : "http://", szUrlHost);

    char* pBufA = (char*)LocalAlloc(LPTR, 3001);
    char* pBufE = (char*)LocalAlloc(LPTR, 3001);
    if (pBufA == NULL || pBufE == NULL)
    {
        if (pBufA != NULL) LocalFree(pBufA);
        if (pBufE != NULL) LocalFree(pBufE);
        return;
    }
    if (pfnA != NULL)
    {
        DWORD nSize = 3000;
        pBufA[0] = '\0';
        pfnA(szUrl, NULL, pBufA, &nSize);
    }
    if (pfnEx != NULL)
    {
        DWORD nSize = 3000;
        pBufE[0] = '\0';
        pfnEx(szUrl, NULL, pBufE, &nSize, 8192, NULL);   // INTERNET_COOKIE_HTTPONLY
    }
    else
    {
        // 只有 A 可用：Ex 那份当作空
    }
    if (szName == NULL || szName[0] == '\0')
    {
        UxCookieMerge(pOut, pBufA, pBufE);
    }
    else
    {
        CxBuf merged;
        BufInit(&merged, 256);
        UxCookieMerge(&merged, pBufA, pBufE);
        if (merged.pData != NULL) UxCookieOne(pOut, merged.pData, szName);
        BufFree(&merged);
    }
    LocalFree(pBufE); LocalFree(pBufA);
}

// ============================================================ 其它
// 原模块：网页_取常见ContentType（§13）：34 项映射表按源码判定顺序原样实现，
// 「取文本右边(名字,N)==".ext"」= 结尾定长比较、区分大小写（.JPG 不命中→octet-stream，照抄）。
void UxContentType(CxBuf* pOut, LPCSTR szNameOrExt)
{
    static const char* const kMap[][2] = {
        { ".jpg",  "image/jpeg" },
        { ".jpeg", "image/jpeg" },
        { ".png",  "image/png" },
        { ".gif",  "image/gif" },
        { ".bmp",  "image/bmp" },
        { ".webp", "image/webp" },
        { ".svg",  "image/svg+xml" },
        { ".txt",  "text/plain" },
        { ".html", "text/html" },
        { ".css",  "text/css" },
        { ".js",   "application/javascript" },
        { ".json", "application/json" },
        { ".xml",  "application/xml" },
        { ".doc",  "application/msword" },
        { ".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document" },
        { ".xls",  "application/vnd.ms-excel" },
        { ".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" },
        { ".ppt",  "application/vnd.ms-powerpoint" },
        { ".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation" },
        { ".pdf",  "application/pdf" },
        { ".zip",  "application/zip" },
        { ".rar",  "application/x-rar-compressed" },
        { ".7z",   "application/x-7z-compressed" },
        { ".tar",  "application/x-tar" },
        { ".gz",   "application/gzip" },
        { ".mp3",  "audio/mpeg" },
        { ".wav",  "audio/wav" },
        { ".ogg",  "audio/ogg" },
        { ".mp4",  "video/mp4" },
        { ".avi",  "video/x-msvideo" },
        { ".mov",  "video/quicktime" },
        { ".wmv",  "video/x-ms-wmv" },
        { ".flv",  "video/x-flv" }
    };
    LPCSTR szResult = "application/octet-stream";
    if (szNameOrExt != NULL)
    {
        const int n = lstrlenA(szNameOrExt);
        for (int i = 0; i < 33; i++)
        {
            const int k = lstrlenA(kMap[i][0]);
            if (n >= k && memcmp(szNameOrExt + n - k, kMap[i][0], (SIZE_T)k) == 0)
            { szResult = kMap[i][1]; break; }
        }
    }
    BufAppend(pOut, szResult, lstrlenA(szResult));
}

// 原模块：网页_构建提交表单数据（§12）：边界=6连字符 WebKitFormBoundary+10随机字符，
// 每 part：边界行+CRLF+Content-Disposition(+文件项的 filename/Content-Type 行)+CRLF+CRLF+数据；
// part 间 CRLF；最后保留项之后 CRLF+收尾边界+"--"。声明的 boundary 带 4 连字符（----WebKit...）。
// 有意修掉两处原模块 bug（规格里点名的）：
//  1) filename="…" 原填的是 表单名称，这里改填真正的 文件名；
//  2) 文件格式="" 时应在拼 part 之前先用 网页_取常见ContentType(文件名) 补好（原在之后，永远空）。
// 返回请求头文本没有出参槽（cxhttp_util.h 未给），调用方可从 pOut 首个边界行截取，
// 拼接格式：Content-Type: multipart/form-data; boundary=----WebKitFormBoundary<10随机>。
void UxBuildForm(CxBuf* pOut, LPCSTR* pNames, LPCSTR* pValues, LPCSTR* pFiles, int nCount)
{
    if (pOut == NULL || nCount <= 0) return;
    CxBuf bnd;
    BufInit(&bnd, 16);
    UxRandChars(&bnd, 10);
    if (bnd.pData == NULL || bnd.nLen == 0) { BufFree(&bnd); return; }
    // 找最后一个保留项（表单名称非空）：修复「跳过空名项后收尾边界永不追加」的隐患
    int nLast = -1;
    for (int i = 0; i < nCount; i++)
    {
        LPCSTR nm = (pNames == NULL || pNames[i] == NULL) ? "" : pNames[i];
        if (nm[0] != '\0') nLast = i;
    }
    if (nLast < 0) { BufFree(&bnd); return; }
    for (int i = 0; i < nCount; i++)
    {
        LPCSTR szName = (pNames == NULL || pNames[i] == NULL) ? "" : pNames[i];
        if (szName[0] == '\0') continue;                       // 到循环尾
        LPCSTR szVal  = (pValues == NULL || pValues[i] == NULL) ? "" : pValues[i];
        LPCSTR szFile = (pFiles == NULL || pFiles[i] == NULL) ? "" : pFiles[i];
        // 边界行：------WebKitFormBoundary + rand + CRLF
        UxAddLit(pOut, "------WebKitFormBoundary");
        BufAppend(pOut, bnd.pData, bnd.nLen);
        UxAddLit(pOut, "\r\n");
        // Content-Disposition 行
        UxAddLit(pOut, "Content-Disposition: form-data; name=\"");
        BufAppend(pOut, szName, lstrlenA(szName));
        UxAddLit(pOut, "\"");
        if (szFile[0] != '\0')
        {
            UxAddLit(pOut, "; filename=\"");
            BufAppend(pOut, szFile, lstrlenA(szFile));         // 修 bug 1：用文件名
            UxAddLit(pOut, "\"\r\nContent-Type: ");
            CxBuf mime;
            BufInit(&mime, 64);
            UxContentType(&mime, szFile);                      // 修 bug 2：拼之前先补 MIME
            if (mime.pData != NULL) BufAppend(pOut, mime.pData, mime.nLen);
            BufFree(&mime);
        }
        UxAddLit(pOut, "\r\n\r\n");
        BufAppend(pOut, szVal, lstrlenA(szVal));               // 提交数据（GBK/ANSI 原字节）
        if (i != nLast) UxAddLit(pOut, "\r\n");
        else
        {
            UxAddLit(pOut, "\r\n------WebKitFormBoundary");
            BufAppend(pOut, bnd.pData, bnd.nLen);
            UxAddLit(pOut, "--");
        }
    }
    BufFree(&bnd);
}

// 注册表读取（对应 krnln 取文本注册项/取数值注册项）
// advapi32 的注册表入口一律运行期解析：静态编译时不能把 advapi32.lib 挂进易语言的链接行
typedef long (WINAPI *fn_regopen_t)(HKEY, LPCSTR, DWORD, REGSAM, HKEY*);
typedef long (WINAPI *fn_regquery_t)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef long (WINAPI *fn_regclose_t)(HKEY);
static fn_regopen_t  g_pRegOpen = NULL;
static fn_regquery_t g_pRegQuery = NULL;
static fn_regclose_t g_pRegClose = NULL;
static BOOL UxRegApi(void)
{
    // 三个入口必须同时可用；只判一个会在部分解析失败后放行空指针调用
    if (g_pRegOpen != NULL && g_pRegQuery != NULL && g_pRegClose != NULL) return TRUE;
    const HMODULE h = LoadLibraryA("advapi32.dll");
    if (h == NULL) return FALSE;
    g_pRegOpen = (fn_regopen_t)GetProcAddress(h, "RegOpenKeyExA");
    g_pRegQuery = (fn_regquery_t)GetProcAddress(h, "RegQueryValueExA");
    g_pRegClose = (fn_regclose_t)GetProcAddress(h, "RegCloseKey");
    return (g_pRegOpen != NULL && g_pRegQuery != NULL && g_pRegClose != NULL) ? TRUE : FALSE;
}

static BOOL UxRegGetStr(HKEY hRoot, LPCSTR szKey, LPCSTR szValue, char* pBuf, int nBuf)
{
    pBuf[0] = '\0';
    HKEY h = NULL;
    if (!UxRegApi() || g_pRegOpen(hRoot, szKey, 0, KEY_READ, &h) != ERROR_SUCCESS) return FALSE;
    BOOL b = FALSE;
    DWORD dt = REG_SZ, n = (DWORD)nBuf;
    if (g_pRegQuery(h, szValue, NULL, &dt, (LPBYTE)pBuf, &n) == ERROR_SUCCESS
        && (dt == REG_SZ || dt == REG_EXPAND_SZ))
    {
        pBuf[nBuf - 1] = '\0';
        if (n < (DWORD)nBuf) pBuf[n] = '\0';
        b = TRUE;
    }
    g_pRegClose(h);
    return b;
}
static BOOL UxRegGetDword(HKEY hRoot, LPCSTR szKey, LPCSTR szValue, DWORD* pOut)
{
    HKEY h = NULL;
    if (!UxRegApi() || g_pRegOpen(hRoot, szKey, 0, KEY_READ, &h) != ERROR_SUCCESS) return FALSE;
    BOOL b = FALSE;
    DWORD dt = REG_DWORD, n = sizeof(DWORD);
    if (g_pRegQuery(h, szValue, NULL, &dt, (LPBYTE)pOut, &n) == ERROR_SUCCESS && dt == REG_DWORD)
        b = TRUE;
    g_pRegClose(h);
    return b;
}

// 从 "…http=<ip:port>;…" 抽 ip:port（§8 抽取逻辑：区分大小写，';' 右界；取不到试 https=）
static void UxProxyExtract(LPCSTR szInfo, CxBuf* pOut)
{
    BufReset(pOut);
    if (UxContains(szInfo, "http=") || UxContains(szInfo, "https="))
    {
        UxBetween(pOut, szInfo, "http=", ";");
        if (pOut->pData == NULL || pOut->nLen == 0)
        {
            BufReset(pOut);
            UxBetween(pOut, szInfo, "https=", ";");
        }
    }
}

// 原模块：网页_取系统代理EX（§8）：先读 HKLM NlaSvc\...\ManualProxies 默认值，
// 不中再走 HKCU Internet Settings 的 ProxyEnable(必须==1)/ProxyServer；返回单个 ip:port 或空串。
// 方式1 的「否则」分支在原模块是恒假死逻辑（ip 那时必为空串），按规格省略不实现（永不生效）。
// 照抄的不一致：ProxyServer 只有 sock=… 前缀时原样整串返回（保留协议前缀）。
void UxSystemProxy(CxBuf* pOut)
{
    if (pOut == NULL) return;
    char szInfo[4096];
    CxBuf ip;
    BufInit(&ip, 64);
    // 方式1
    if (UxRegGetStr(HKEY_LOCAL_MACHINE,
            "SYSTEM\\ControlSet001\\Services\\NlaSvc\\Parameters\\Internet\\ManualProxies\\",
            "", szInfo, sizeof(szInfo)))
    {
        UxProxyExtract(szInfo, &ip);
        if (ip.pData != NULL && ip.nLen > 0)
        {
            BufAppend(pOut, ip.pData, ip.nLen);
            BufFree(&ip);
            return;
        }
    }
    BufFree(&ip);
    // 方式2
    DWORD dwEnable = 0;
    if (!UxRegGetDword(HKEY_CURRENT_USER,
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
            "ProxyEnable", &dwEnable) || dwEnable != 1)
        return;                                     // 取数值注册项默认 0，≠1 → 空串
    if (!UxRegGetStr(HKEY_CURRENT_USER,
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
            "ProxyServer", szInfo, sizeof(szInfo)))
        szInfo[0] = '\0';
    CxBuf ip2;
    BufInit(&ip2, 64);
    UxProxyExtract(szInfo, &ip2);
    if (ip2.pData != NULL && ip2.nLen > 0)
    {
        BufAppend(pOut, ip2.pData, ip2.nLen);
    }
    else
    {
        // 判 http=/https= 都不含（或含但抽取为空后原样兜底？——原模块「否则」才是整串，
        // 含 http= 但抽不到 ';' 段落时原模块返回空串，这里保持同一行为：只有整串无 http=/https= 才回落）
        if (!UxContains(szInfo, "http=") && !UxContains(szInfo, "https="))
            BufAppend(pOut, szInfo, lstrlenA(szInfo));
    }
    BufFree(&ip2);
}

// ============================================================ 最小 INFLATE（RFC1951）
// 原模块：网页_GZIP解压（P9，依赖第三方 gzip.dll）。本库不引 zlib、不加载 gzip.dll，
// 自带最小 inflate：支持 stored/fixed/dynamic 三类块；先剥 gzip 头（1F 8B、CM、FLG、
// MTIME(4)、XFL、OS，处理 FHCRC/FEXTRA/FNAME/FCOMMENT 可选段），也顺手认 zlib 壳(0x78 …)；
// 都不符合则按裸 deflate 流处理。失败返回 NULL，绝不返回未定义指针。
// 返回缓冲为 LocalAlloc，容量可能大于 *pOutLen，调用方用长度取数、LocalFree 释放。
#define UZ_MAX_OUT (64 * 1024 * 1024)

typedef struct
{
    LPCBYTE      pIn;
    int          nIn;
    int          nPos;
    unsigned int uBuf;      // 位缓冲：低位是下一个要取的位（deflate 为 LSB-first）
    int          nBits;
    LPBYTE       pOut;
    int          nOut;
    int          nCap;
} UxZStream;

typedef struct { unsigned short Counts[16]; unsigned short Symbols[320]; } UxZHuff;

static int UxzBits(UxZStream* z, int need)        // need 1..16
{
    unsigned int val;
    while (z->nBits < need)
    {
        if (z->nPos >= z->nIn) return -1;
        z->uBuf |= (unsigned int)z->pIn[z->nPos++] << z->nBits;
        z->nBits += 8;
    }
    val = z->uBuf & ((1u << need) - 1u);
    z->uBuf >>= need;
    z->nBits -= need;
    return (int)val;
}

static BOOL UxzOut(UxZStream* z, int need)
{
    if (need <= 0) return TRUE;
    if (z->nOut + need > UZ_MAX_OUT) return FALSE;             // 压缩炸弹/坏数据护栏
    if (z->nOut + need <= z->nCap) return TRUE;
    int nCap = z->nCap;
    while (nCap < z->nOut + need) nCap += (nCap >> 1) + 64;
    LPBYTE p = (LPBYTE)LocalAlloc(LPTR, (SIZE_T)nCap);
    if (p == NULL) return FALSE;
    if (z->nOut > 0 && z->pOut != NULL) memcpy(p, z->pOut, (SIZE_T)z->nOut);
    if (z->pOut != NULL) LocalFree(z->pOut);
    z->pOut = p;
    z->nCap = nCap;
    return TRUE;
}

// 正则（canonical）Huffman 构建：Counts[len]=该长度码个数，Symbols 按 (长度,符号升序) 排。
// 只查「超订阅」，允许不完备码（fixed 距离表 30/32 就是不完备的）。
static BOOL UzxBuild(UxZHuff* h, const unsigned short* lens, int n)
{
    unsigned int count[16], offs[17];
    int i, len, left = 1;
    for (i = 0; i < 16; i++) count[i] = 0;
    for (i = 0; i < n; i++)
    {
        if (lens[i] > 15) return FALSE;
        count[lens[i]]++;
    }
    for (len = 1; len <= 15; len++)
    {
        left <<= 1;
        left -= (int)count[len];
        if (left < 0) return FALSE;                            // 超订阅：坏流
    }
    offs[1] = 0;
    for (len = 1; len < 15; len++) offs[len + 1] = offs[len] + count[len];
    for (len = 0; len <= 15; len++) h->Counts[len] = (unsigned short)count[len];
    for (i = 0; i < n; i++) if (lens[i] != 0) h->Symbols[offs[lens[i]]++] = (unsigned short)i;
    return TRUE;
}

static int UzxDecode(UxZStream* z, const UxZHuff* h)
{
    int code = 0, first = 0, index = 0;
    for (int len = 1; len <= 15; len++)
    {
        const int bit = UxzBits(z, 1);
        if (bit < 0) return -1;
        code |= bit;
        const int count = h->Counts[len];
        if (code - first < count) return h->Symbols[index + (code - first)];
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    return -1;
}

static const unsigned short UZ_LENS[29] =
    { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258 };
static const unsigned char UZ_LBITS[29] =
    { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0 };
static const unsigned short UZ_DISTS[30] =
    { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,
      8193,12289,16385,24577 };
static const unsigned char UZ_DBITS[30] =
    { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13 };
static const unsigned char UZ_CORDER[19] =
    { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };

static BOOL UxzInflate(UxZStream* z)
{
    BOOL bLast = FALSE;
    while (!bLast)
    {
        int bit = UxzBits(z, 1);
        if (bit < 0) return FALSE;
        bLast = (bit != 0);
        const int type = UxzBits(z, 2);
        if (type < 0) return FALSE;
        if (type == 0)
        {
            // stored：丢掉不足一字的余位（本读位器保证 surplus<8，之后 nBits 必为 0）
            const int drop = z->nBits & 7;
            z->uBuf >>= drop;
            z->nBits -= drop;
            const int lo = UxzBits(z, 8), hi = UxzBits(z, 8);
            const int lo2 = UxzBits(z, 8), hi2 = UxzBits(z, 8);
            if (lo < 0 || hi < 0 || lo2 < 0 || hi2 < 0) return FALSE;
            const int len = lo | (hi << 8);
            const int nlen = lo2 | (hi2 << 8);
            if (((len ^ 0xFFFF) & 0xFFFF) != nlen) return FALSE;
            if (len > 0)
            {
                if (z->nIn - z->nPos < len) return FALSE;
                if (!UxzOut(z, len)) return FALSE;
                memcpy(z->pOut + z->nOut, z->pIn + z->nPos, (SIZE_T)len);
                z->nPos += len;
                z->nOut += len;
            }
            continue;
        }
        UxZHuff hLit, hDist;
        unsigned short litLens[288];
        unsigned short distLens[32];
        if (type == 1)
        {
            int i;
            for (i = 0; i < 144; i++) litLens[i] = 8;
            for (; i < 256; i++) litLens[i] = 9;
            for (; i < 280; i++) litLens[i] = 7;
            for (; i < 288; i++) litLens[i] = 8;
            for (i = 0; i < 30; i++) distLens[i] = 5;
            if (!UzxBuild(&hLit, litLens, 288)) return FALSE;
            if (!UzxBuild(&hDist, distLens, 30)) return FALSE;
        }
        else if (type == 2)
        {
            int hlit = UxzBits(z, 5), hdist = UxzBits(z, 5), hclen = UxzBits(z, 4);
            if (hlit < 0 || hdist < 0 || hclen < 0) return FALSE;
            hlit += 257; hdist += 1; hclen += 4;
            if (hlit > 286 || hdist > 30) return FALSE;       // 286/287 长度码本身非法
            unsigned short cl[19];
            int i;
            for (i = 0; i < 19; i++) cl[i] = 0;
            for (i = 0; i < hclen; i++)
            {
                const int v = UxzBits(z, 3);
                if (v < 0) return FALSE;
                cl[UZ_CORDER[i]] = (unsigned short)v;
            }
            UxZHuff hLen;
            if (!UzxBuild(&hLen, cl, 19)) return FALSE;
            unsigned short lens[288 + 32];
            const int total = hlit + hdist;
            int n = 0;
            while (n < total)
            {
                const int sym = UzxDecode(z, &hLen);
                if (sym < 0 || sym > 18) return FALSE;
                if (sym < 16) { lens[n++] = (unsigned short)sym; }
                else
                {
                    int rep, v;
                    if (sym == 16)
                    {
                        if (n == 0) return FALSE;
                        v = UxzBits(z, 2); if (v < 0) return FALSE;
                        rep = 3 + v;
                        if (n + rep > total) return FALSE;
                        const unsigned short prev = lens[n - 1];
                        for (; rep > 0; rep--) lens[n++] = prev;
                    }
                    else if (sym == 17)
                    {
                        v = UxzBits(z, 3); if (v < 0) return FALSE;
                        rep = 3 + v;
                        if (n + rep > total) return FALSE;
                        for (; rep > 0; rep--) lens[n++] = 0;
                    }
                    else
                    {
                        v = UxzBits(z, 7); if (v < 0) return FALSE;
                        rep = 11 + v;
                        if (n + rep > total) return FALSE;
                        for (; rep > 0; rep--) lens[n++] = 0;
                    }
                }
            }
            for (int k = 0; k < hdist; k++) distLens[k] = lens[hlit + k];
            if (!UzxBuild(&hLit, lens, hlit)) return FALSE;
            if (!UzxBuild(&hDist, distLens, hdist)) return FALSE;
        }
        else return FALSE;
        for (;;)
        {
            const int sym = UzxDecode(z, &hLit);
            if (sym < 0) return FALSE;
            if (sym < 256)
            {
                if (!UxzOut(z, 1)) return FALSE;
                z->pOut[z->nOut++] = (BYTE)sym;
            }
            else if (sym == 256) break;
            else
            {
                if (sym > 285) return FALSE;
                const int li = sym - 257;
                const int lext = UxzBits(z, UZ_LBITS[li]);
                if (lext < 0) return FALSE;
                const int length = UZ_LENS[li] + lext;
                const int dsym = UzxDecode(z, &hDist);
                if (dsym < 0 || dsym >= 30) return FALSE;
                const int dext = UxzBits(z, UZ_DBITS[dsym]);
                if (dext < 0) return FALSE;
                const int dist = UZ_DISTS[dsym] + dext;
                if (dist > z->nOut) return FALSE;
                if (!UxzOut(z, length)) return FALSE;
                int from = z->nOut - dist;
                for (int k = 0; k < length; k++) z->pOut[z->nOut++] = z->pOut[from++];
            }
        }
    }
    return TRUE;
}

// 剥 gzip 头：1；zlib 壳：2；裸流：0；坏头：-1
static int UxzSkipHeader(UxZStream* z)
{
    LPCBYTE p = z->pIn;
    if (z->nIn >= 2 && p[0] == 0x1F && p[1] == 0x8B)
    {
        if (z->nIn < 10 || p[2] != 8) return -1;              // CM 必须是 deflate
        const int flg = p[3];
        int pos = 10;                                          // 跳过 MTIME(4)+XFL(1)+OS(1)
        if (flg & 0x04)                                        // FEXTRA
        {
            if (pos + 2 > z->nIn) return -1;
            const int xlen = p[pos] | (p[pos + 1] << 8);
            pos += 2 + xlen;
            if (pos > z->nIn) return -1;
        }
        if (flg & 0x08)                                        // FNAME
        {
            while (pos < z->nIn && p[pos] != 0) pos++;
            if (pos >= z->nIn) return -1;
            pos++;
        }
        if (flg & 0x10)                                        // FCOMMENT
        {
            while (pos < z->nIn && p[pos] != 0) pos++;
            if (pos >= z->nIn) return -1;
            pos++;
        }
        if (flg & 0x02)                                        // FHCRC（CRC16 不校验，只跳）
        {
            pos += 2;
            if (pos > z->nIn) return -1;
        }
        z->nPos = pos;
        return 1;
    }
    if (z->nIn >= 2 && p[0] == 0x78 && (((p[0] << 8) | p[1]) % 31) == 0)
    {
        z->nPos = 2;
        return 2;
    }
    return 0;
}

LPBYTE UxGzipInflate(const void* pData, int nLen, int* pOutLen)
{
    if (pOutLen != NULL) *pOutLen = 0;
    if (pData == NULL || nLen <= 0 || pOutLen == NULL) return NULL;
    UxZStream z;
    z.pIn = (LPCBYTE)pData; z.nIn = nLen; z.nPos = 0;
    z.uBuf = 0; z.nBits = 0;
    z.nCap = nLen < 8192 ? 8192 : nLen;                        // 经验：输出 ≥ 输入的不少场景，先给足
    z.pOut = (LPBYTE)LocalAlloc(LPTR, (SIZE_T)z.nCap);
    if (z.pOut == NULL) return NULL;
    z.nOut = 0;
    if (UxzSkipHeader(&z) < 0) { LocalFree(z.pOut); return NULL; }
    if (!UxzInflate(&z)) { LocalFree(z.pOut); *pOutLen = 0; return NULL; }
    *pOutLen = z.nOut;
    return z.pOut;                                             // LocalAlloc 缓冲，调用方 LocalFree
}

// 原模块：CURL取错误文本（curl_easy_strerror）。入口只从「已加载」的 curl 模块取
// （运行期 GetProcAddress，不触发加载）；拿不到就输出 "curl error <码>"。
void UxCurlErrorText(CxBuf* pOut, int nCode)
{
    if (pOut == NULL) return;
    fn_easy_strerror_t f = g_curl.EasyStrerror;
    if (f == NULL && g_curl.hModule != NULL)
        f = (fn_easy_strerror_t)GetProcAddress(g_curl.hModule, "curl_easy_strerror");
    if (f != NULL)
    {
        const char* sz = f(nCode);
        if (sz != NULL && sz[0] != '\0')
        {
            BufAppend(pOut, sz, lstrlenA(sz));
            return;
        }
    }
    char szT[48];
    wsprintfA(szT, "curl error %d", nCode);
    BufAppend(pOut, szT, lstrlenA(szT));
}

// 字节级查找（压缩亚马逊页面数据用；页面字节不做 GBK 边界判断，逐字节匹配照原模块）
static int UxMemFind(LPCBYTE p, int n, LPCSTR szNeed)
{
    const int m = lstrlenA(szNeed);
    if (m <= 0 || n < m) return -1;
    for (int i = 0; i + m <= n; i++)
        if (memcmp(p + i, szNeed, (SIZE_T)m) == 0) return i;
    return -1;
}
static int UxMemRFind(LPCBYTE p, int n, LPCSTR szNeed)
{
    const int m = lstrlenA(szNeed);
    if (m <= 0 || n < m) return -1;
    for (int i = n - m; i >= 0; i--)
        if (memcmp(p + i, szNeed, (SIZE_T)m) == 0) return i;
    return -1;
}

// 原模块：压缩亚马逊页面数据（§19 + P11）。步骤：
//  1) 不含小写 "<!doctype html>"（大小写敏感，原模块就这样，亚马逊大写 DOCTYPE 常判不到——照抄）
//     且长度 > 1000000 → 原样返回；
//  2) 找到 <meta http-equiv="content-type" 就把之前的头部裁掉；
//  3) 找到最后一次 <!-- footer pilu --> 就把它之后的页脚尾裁掉；
// 有意修掉 P11/§19 的空 needle 隐患：标记找不到（字节集_取左边/取右边 返回 { } → 空 needle 替换
// 是未定义行为）时直接跳过该步，不做替换。返回 LocalAlloc 缓冲（调用方 LocalFree），失败 NULL。
LPBYTE UxCompressAmazonPage(const void* pData, int nLen, int* pOutLen)
{
    if (pOutLen != NULL) *pOutLen = 0;
    if (pData == NULL || nLen <= 0 || pOutLen == NULL) return NULL;
    LPCBYTE p = (LPCBYTE)pData;
    int nStart = 0, nEnd = nLen;
    const BOOL bNoDoctype = (UxMemFind(p, nLen, "<!doctype html>") == -1);
    if (bNoDoctype && nLen > 1000000)
    {
        // 原样返回：复制一份（不能把调用方字节集的头指针当结果往外送）
        LPBYTE q = (LPBYTE)LocalAlloc(LPTR, (SIZE_T)nLen);
        if (q == NULL) return NULL;
        memcpy(q, p, (SIZE_T)nLen);
        *pOutLen = nLen;
        return q;
    }
    const int iMeta = UxMemFind(p, nLen, "<meta http-equiv=\"content-type\"");
    if (iMeta >= 0) nStart = iMeta;
    static const char kFoot[] = "<!-- footer pilu -->";
    const int iFoot = UxMemRFind(p, nLen, kFoot);
    if (iFoot >= 0)
    {
        const int nStop = iFoot + lstrlenA(kFoot);
        if (nStop < nEnd) nEnd = nStop;
    }
    const int nOut = nEnd - nStart;
    if (nOut <= 0)
    {
        // 极端输入（标记错位）：兜底原样返回，绝不给未定义指针
        LPBYTE q = (LPBYTE)LocalAlloc(LPTR, (SIZE_T)nLen);
        if (q == NULL) return NULL;
        memcpy(q, p, (SIZE_T)nLen);
        *pOutLen = nLen;
        return q;
    }
    LPBYTE q = (LPBYTE)LocalAlloc(LPTR, (SIZE_T)nOut);
    if (q == NULL) return NULL;
    memcpy(q, p + nStart, (SIZE_T)nOut);
    *pOutLen = nOut;
    return q;
}

#endif  // CXHTTP_UTIL_INC
