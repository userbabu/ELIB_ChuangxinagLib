// 宽文本全局命令（迁移自 创想软件UNICODE模块\文本处理模块 56 条 + 扩展模块 19 条）。
// 位置值一律从 1 开始（与模块原版一致）；宽文本按 UTF-16 字节集进出，文本型参数按 UTF-8 解释。

#include <cx_uni.h>

namespace cx {

extern PFN_NOTIFY_SYS g_pfnNotifySys;
INT NotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2);
char* CloneTextData(LPCSTR pszText, INT nLen);
char* CloneTextData(LPCSTR pszText);
LPBYTE CloneBinData(LPCSTR pData, INT nDataSize);
void ReportError(LPCSTR szErrText);

static const WCHAR g_wEmpty[1] = { 0 };
static const WCHAR g_wSpace[2] = { ' ', 0 };

// ============================================================ 局部工具

static void RetRawLeft(PMDATA_INF pRet, const WCHAR* pRaw, INT nAvailChars, INT nWantChars)
{
    if (nWantChars < 0) nWantChars = 0;
    if (nWantChars > nAvailChars) nWantChars = nAvailChars;
    pRet->m_dtDataType = SDT_BIN;
    pRet->m_pBin = (nWantChars <= 0) ? NULL : CloneBinData((LPCSTR)pRaw, nWantChars * 2);
}

static void ViewFromBinElem(LPBYTE b, UniWView* pView)
{
    if (b != NULL) pView->Set(b + sizeof(INT) * 2, *(LPINT)(b + sizeof(INT)));
}

// WF(标准Unicode编码=假)：去掉末尾 {0,0}，其余原样保留（含中间的空字符）。
static void AddWFRaw(UniWBuf* pOut, const UniWView& v)
{
    INT n = v.nRawChars - 1;
    if (v.nOrigBytes <= 0 || n <= 0) return;
    pOut->Add(v.pRaw, n);
}

static void WriteBackBin(const MDATA_INF& arg, const WCHAR* pW, INT nChars)
{
    if (UniIsNull(arg) || arg.m_ppBin == NULL) return;
    LPBYTE p = CloneBinData((LPCSTR)pW, nChars * 2 + 2);
    if (*arg.m_ppBin != NULL) NotifySys(NRS_MFREE, (DWORD)(INT_PTR)*arg.m_ppBin, 0);
    *arg.m_ppBin = p;
}

// 参考(传址) 字节集参数：联合体槽里放的是「数据指针变量」的地址，读取要先解一层引用。
static void ViewFromRefBin(const MDATA_INF& arg, UniWView* pView)
{
    if (UniIsNull(arg) || arg.m_ppBin == NULL || *arg.m_ppBin == NULL) { pView->Set(NULL, 0); return; }
    LPBYTE b = *arg.m_ppBin;
    pView->Set(b + sizeof(INT) * 2, *(LPINT)(b + sizeof(INT)));
}

static INT CountOccur(const WCHAR* pSrc, INT nSrcChars, const WCHAR* pNeedle, INT nNeedleChars, BOOL bCaseInsensitive)
{
    INT n = 0, nPos = 1;
    for (;;)
    {
        INT nAt = UniFindWide(pSrc, nSrcChars, pNeedle, nPos, bCaseInsensitive);
        if (nAt < 0) break;
        n++;
        nPos = nAt + ((nNeedleChars > 0) ? nNeedleChars : 1);
    }
    return n;
}

// 寻找文本W：保留原版全部前置检查（奇数字节、位置越界、终止符之后不可搜）。
static INT FindTextWView(const UniWView& src, const UniWView& find, INT nPos1, BOOL bCaseInsensitive)
{
    if ((src.nOrigBytes & 1) != 0) return -1;
    if ((find.nOrigBytes & 1) != 0) return -1;
    if (nPos1 == 0) nPos1 = 1;
    if (nPos1 < 1) return -1;
    if (nPos1 > src.nChars + 1) return -1;
    if (find.nChars > 0 && nPos1 > src.nChars) return -1;
    return UniFindWide(src.p, src.nChars, find.p, nPos1, bCaseInsensitive);
}

// ============================================================ 取段与构造

void UwTextLen(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = v.nChars;
}

void UwLeft(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    INT n = UniArgInt(pArgInf[1], 0);
    if (n < 0) n = 0;
    if (n > v.nRawChars) n = v.nRawChars;
    UniRetWC(pRetData, v.pRaw, n);
}

void UwRight(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    INT n = UniArgInt(pArgInf[1], 0) + 1;         // 原版取右边 长度*2+2 字节
    if (n < 1) n = 1;
    if (n > v.nRawChars) n = v.nRawChars;
    UniRetWC(pRetData, v.pRaw + (v.nRawChars - n), n);
}

void UwMid(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    INT nStart = UniArgInt(pArgInf[1], 1);
    INT n = UniArgInt(pArgInf[2], 0);
    if (nStart < 1) nStart = 1;
    INT nIdx = nStart - 1;
    if (nIdx > v.nRawChars) nIdx = v.nRawChars;
    if (n < 0) n = 0;
    if (nIdx + n > v.nRawChars) n = v.nRawChars - nIdx;
    UniRetWC(pRetData, v.pRaw + nIdx, n);
}

void UwCharW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    INT buf[1];
    buf[0] = UniArgInt(pArgInf[0], 0);
    UniRetRawBin(pRetData, (LPCSTR)buf, 4);       // 到字节集(整数型)：4 字节小端序
}

void UwGetCode(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    INT nPos = UniArgInt(pArgInf[1], 0);
    if (nPos == 0) nPos = 1;
    pRetData->m_dtDataType = SDT_INT;
    // 原版不做越界检查，这里补上以免读到非法内存。
    if (nPos < 1 || (nPos - 1) >= v.nRawChars) pRetData->m_int = 0;
    else pRetData->m_int = (INT)(unsigned short)v.pRaw[nPos - 1];
}

void UwBlankText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    INT n = UniArgInt(pArgInf[0], 0);
    pRetData->m_dtDataType = SDT_BIN;
    pRetData->m_pBin = NULL;
    if (n < 1) return;
    WCHAR* q = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)(n / 2 + 2) * sizeof(WCHAR));
    if (q == NULL) return;
    UniRetRawBin(pRetData, (LPCSTR)q, n);         // 字节集_取空白：n 个 0 字节
    LocalFree(q);
}

void UwRepeatText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    INT n = UniArgInt(pArgInf[0], 0);
    UniWView v; v.FromBin(pArgInf[1]);
    UniWBuf out;
    if (n > 0 && out.Reserve(v.nChars * n + 1))
        for (INT i = 0; i < n; i++) out.Add(v.p, v.nChars);
    UniRetWideBin(pRetData, (out.p != NULL) ? out.p : g_wEmpty, out.nLen);
}

// ============================================================ 查找

void UwFind(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView find; find.FromBin(pArgInf[1]);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = FindTextWView(src, find, UniArgInt(pArgInf[2], 0), UniArgBool(pArgInf[3], FALSE));
}

void UwRFind(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView find; find.FromBin(pArgInf[1]);
    INT nPos = UniArgInt(pArgInf[2], 0);
    if (nPos == 0) nPos = src.nChars + 1;
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = UniRFindWide(src.p, src.nChars, find.p, nPos, UniArgBool(pArgInf[3], FALSE));
}

void UwTextExists(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView needle; needle.FromBin(pArgInf[1]);
    // 原版把「区分大小写」直接送进「不区分大小写」位，保持同一取值方式。
    UniRetBool(pRetData, UniFindWide(src.p, src.nChars, needle.p, 1, UniArgBool(pArgInf[2], FALSE)) >= 0 ? TRUE : FALSE);
}

// ============================================================ 大小写与全半角

static void RetCaseConverted(PMDATA_INF pRet, const UniWView& v, BOOL bUpper, BOOL bKeepNull)
{
    WCHAR* q = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)(v.nChars + 1) * sizeof(WCHAR));
    if (q == NULL)
    {
        if (bKeepNull) UniRetWideBin(pRet, g_wEmpty, 0);
        else UniRetWideBinNoNull(pRet, g_wEmpty, 0);
        return;
    }
    memcpy(q, v.p, (SIZE_T)v.nChars * sizeof(WCHAR));
    q[v.nChars] = 0;
    if (bUpper) CharUpperW(q); else CharLowerW(q);
    INT n = lstrlenW(q);
    if (bKeepNull) UniRetWideBin(pRet, q, n);
    else UniRetWideBinNoNull(pRet, q, n);          // 到小写W 原版对结果做了格式化宽字符W（去末尾 0,0）
    LocalFree(q);
}

void UwUpper(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    RetCaseConverted(pRetData, v, TRUE, TRUE);
}

void UwLower(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    RetCaseConverted(pRetData, v, FALSE, FALSE);
}

static void RetMapWidth(PMDATA_INF pRet, const UniWView& v, DWORD dwMap)
{
    INT nOut = 0;
    WCHAR* q = UniMapWidth(v.p, v.nChars, dwMap, &nOut);
    if (q == NULL)
    {
        WCHAR* z = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)(v.nChars + 1) * sizeof(WCHAR));
        if (z != NULL) { UniRetWideBin(pRet, z, v.nChars); LocalFree(z); }   // 与原版一致返回清零缓冲
        else UniRetWideBin(pRet, g_wEmpty, v.nChars);
        return;
    }
    UniRetWideBin(pRet, q, nOut);
    LocalFree(q);
}

void UwFullWidth(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    RetMapWidth(pRetData, v, LCMAP_FULLWIDTH);
}

void UwHalfWidth(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    RetMapWidth(pRetData, v, LCMAP_HALFWIDTH);
}

// ============================================================ 连接/比较/裁剪

void UwCat(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView a; a.FromBin(pArgInf[0]);
    UniWView b; b.FromBin(pArgInf[1]);
    if (b.nOrigBytes <= 0) { UniRetWideBin(pRetData, a.p, a.nChars); return; }
    UniWBuf out;
    if (!out.Reserve(a.nChars + b.nChars + 1)) { UniRetWideBin(pRetData, a.p, a.nChars); return; }
    out.Add(a.p, a.nChars);
    out.Add(b.p, b.nChars);
    UniRetWideBin(pRetData, out.p, out.nLen);
}

void UwCompare(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView a; a.FromBin(pArgInf[0]);
    UniWView b; b.FromBin(pArgInf[1]);
    BOOL bIns = UniArgBool(pArgInf[2], FALSE);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = bIns ? lstrcmpiW(a.p, b.p) : lstrcmpW(a.p, b.p);
}

void UwTrim(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    UniWView t; t.FromBin(pArgInf[1]);
    const UniApi& a = UniGetApi();
    WCHAR* q = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)(v.nChars + 1) * sizeof(WCHAR));
    if (q == NULL) { UniRetWideBin(pRetData, v.p, v.nChars); return; }
    memcpy(q, v.p, (SIZE_T)v.nChars * sizeof(WCHAR));
    q[v.nChars] = 0;
    if (a.StrTrimW != NULL) a.StrTrimW(q, t.p);
    UniRetWideBin(pRetData, q, lstrlenW(q));
    LocalFree(q);
}

void UwTrimSpace(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    const UniApi& a = UniGetApi();
    WCHAR* q = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)(v.nChars + 1) * sizeof(WCHAR));
    if (q == NULL) { UniRetWideBin(pRetData, v.p, v.nChars); return; }
    memcpy(q, v.p, (SIZE_T)v.nChars * sizeof(WCHAR));
    q[v.nChars] = 0;
    if (a.StrTrimW != NULL) a.StrTrimW(q, g_wSpace);
    UniRetWideBin(pRetData, q, lstrlenW(q));
    LocalFree(q);
}

// ============================================================ 替换

void UwReplace(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    INT nStart = UniArgInt(pArgInf[1], 1);
    INT nRepLen = UniArgInt(pArgInf[2], 0);
    UniWView nw; nw.FromBin(pArgInf[3]);
    if (nStart < 1) nStart = 1;
    INT nIdx = nStart - 1;
    if (nIdx > v.nRawChars) nIdx = v.nRawChars;
    if (nRepLen < 0) nRepLen = 0;
    if (nIdx + nRepLen > v.nRawChars) nRepLen = v.nRawChars - nIdx;
    INT nTail = v.nRawChars - nIdx - nRepLen;
    UniWBuf out;
    if (!out.Reserve(nIdx + nw.nChars + nTail + 1)) { RetRawLeft(pRetData, v.pRaw, v.nRawChars, nIdx); return; }
    out.Add(v.pRaw, nIdx);
    out.Add(nw.p, nw.nChars);
    out.Add(v.pRaw + nIdx + nRepLen, nTail);       // 字节集替换保留原始尾部（含终止符）
    pRetData->m_dtDataType = SDT_BIN;
    pRetData->m_pBin = CloneBinData((LPCSTR)out.p, out.nLen * 2);
}

static void SubReplaceRet(PMDATA_INF pRetData, const UniWView& v, const UniWView& oldText, const UniWView& newText,
                          INT nStart, INT nCount, BOOL bCaseSensitive)
{
    if (nCount < 0 || oldText.nChars <= 0 || v.nChars <= 0)
    { UniRetWideBin(pRetData, v.p, v.nChars); return; }
    UniWBuf out;
    if (!UniSubReplaceWide(v.p, v.nChars, oldText.p, newText.p, nStart, nCount, bCaseSensitive, &out))
    { UniRetWideBin(pRetData, v.p, v.nChars); return; }
    UniRetWideBin(pRetData, out.p, out.nLen);
}

void UwSubReplace(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    UniWView o; o.FromBin(pArgInf[1]);
    UniWView n; n.FromBin(pArgInf[2]);
    INT nStart = UniIsNull(pArgInf[3]) ? 1 : UniArgInt(pArgInf[3], 1);
    SubReplaceRet(pRetData, v, o, n, nStart, UniArgInt(pArgInf[4], 0), UniArgBool(pArgInf[5], TRUE));
}

void UwSubReplace2(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    UniWView o; o.FromUtf8(UniArgText(pArgInf[1]));
    UniWView n; n.FromUtf8(UniArgText(pArgInf[2]));
    INT nStart = UniIsNull(pArgInf[3]) ? 1 : UniArgInt(pArgInf[3], 1);
    SubReplaceRet(pRetData, v, o, n, nStart, UniArgInt(pArgInf[4], 0), UniArgBool(pArgInf[5], TRUE));
}

// ============================================================ 分割

static void AddSegToAry(UniAryList* pAry, const WCHAR* p, INT nChars)
{
    UniWBuf b;
    if (nChars > 0 && p != NULL)
    {
        b.Reserve(nChars + 1);
        for (INT i = 0; i < nChars; i++) if (p[i] != 0) b.AddChar(p[i]);
    }
    pAry->Add(CloneBinData((LPCSTR)((b.p != NULL) ? b.p : g_wEmpty), b.nLen * 2 + 2));
}

// 切分口径：源为空给一个空片段；bRemoveEmpty 只去掉"相邻分隔符之间的空片段"与末尾空片段。
static void CollectSplitRanges(const UniWView& src, const UniWView& sep, BOOL bRemoveEmpty, UniWRangeList* pOut)
{
    if (src.nChars <= 0) { pOut->Add(0, 0); return; }
    INT i = 0, nHit = 0;
    INT nSep = (sep.nChars > 0) ? sep.nChars : 1;
    for (;;)
    {
        INT nAt = UniFindWide(src.p, src.nChars, sep.p, i + 1, FALSE);
        if (nAt < 0) break;
        nHit++;
        if (!(nAt - 1 == i && bRemoveEmpty)) pOut->Add(i, nAt - 1 - i);
        i = nAt - 1 + nSep;
        if (i > src.nChars) break;
    }
    if (nHit == 0) pOut->Add(0, src.nChars);
    else if (i <= src.nChars && !(bRemoveEmpty && i == src.nChars)) pOut->Add(i, src.nChars - i);
}

static INT SplitToBinAry(const UniWView& src, const UniWView& sep, BOOL bRemoveEmpty,
                         const MDATA_INF& argAry, INT bAppend)
{
    UniAryList ary;
    UniWRangeList rng;
    CollectSplitRanges(src, sep, bRemoveEmpty, &rng);
    for (INT k = 0; k < rng.nCount; k++) AddSegToAry(&ary, src.p + rng.pStart[k], rng.pChars[k]);
    UniAryAppend(argAry, &ary, bAppend);
    return ary.nCount;
}

// 同一套切分，元素直接编成 UTF-8 文本并由命令返回数组（不要求调用方先声明数组变量）。
void UwSplitAry(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView sep; sep.FromBin(pArgInf[1]);
    UniWRangeList rng;
    CollectSplitRanges(src, sep, UniArgBool(pArgInf[2], FALSE), &rng);
    UniAryList list;
    for (INT k = 0; k < rng.nCount; k++)
    {
        char* sz = UniWideToUtf8(src.p + rng.pStart[k], rng.pChars[k]);
        list.Add(CloneTextData((sz != NULL) ? sz : "", (sz != NULL) ? (INT)lstrlenA(sz) : 0));
        if (sz != NULL) LocalFree(sz);
    }
    UniRetAryData(pRetData, &list);
}

void UwSplit2(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView sep; sep.FromBin(pArgInf[1]);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = SplitToBinAry(src, sep, FALSE, pArgInf[2], 0);   // 原版先清空数组
}

void UwSplit(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UwSplit2(pRetData, nArgCount, pArgInf);       // 内存方式与寻找方式分割结果一致
    UNREFERENCED_PARAMETER(nArgCount);
}

// ============================================================ 指针与数值转换

void UwPtrToText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    INT ptr = UniArgInt(pArgInf[0], 0);
    pRetData->m_dtDataType = SDT_BIN;
    pRetData->m_pBin = NULL;
    if (ptr == 0) return;
    INT n = lstrlenW((LPCWSTR)(INT_PTR)ptr);
    UniRetRawBin(pRetData, (LPCSTR)(INT_PTR)ptr, n * 2 + 2);
}

void UwTextPtr(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    INT nBytes = 0;
    LPBYTE p = UniArgBinPtr(pArgInf[0], &nBytes);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (INT)(INT_PTR)p;            // 空字节集按原版返回 0
}

void UwTextToInt(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    BOOL bHex = UniArgBool(pArgInf[1], FALSE);
    const UniApi& a = UniGetApi();
    INT nRet = 0;
    if (a.StrToIntExW != NULL) a.StrToIntExW(v.p, bHex ? UNI_STIF_SUPPORT_HEX : UNI_STIF_DEFAULT, &nRet);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = nRet;
}

void UwIntToText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    WCHAR buf[24];
    buf[0] = 0;
    INT n = UniArgInt(pArgInf[0], 0);
    const UniApi& a = UniGetApi();
    INT nRet;
    if (a.wvsprintfW != NULL) { INT args[1] = { n }; nRet = a.wvsprintfW(buf, L"%d", args); }
    else nRet = wsprintfW(buf, L"%d", n);
    if (nRet < 0) nRet = 0;
    UniRetWC(pRetData, buf, nRet);
}

// ============================================================ 文件

void UwReadFile(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView name; name.FromBin(pArgInf[0]);
    pRetData->m_dtDataType = SDT_BIN;
    pRetData->m_pBin = NULL;
    if (name.nChars <= 0) return;
    HANDLE h = CreateFileW(name.p, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD dwSize = GetFileSize(h, NULL);
    if (dwSize == 0 || dwSize == INVALID_FILE_SIZE) { CloseHandle(h); return; }
    LPBYTE pData = (LPBYTE)LocalAlloc(LPTR, dwSize);
    if (pData == NULL) { CloseHandle(h); return; }
    DWORD dwOff = 0, dwRead = 0;
    while (dwOff < dwSize)
    {
        if (!ReadFile(h, pData + dwOff, dwSize - dwOff, &dwRead, NULL) || dwRead == 0) break;
        dwOff += dwRead;
    }
    CloseHandle(h);
    UniRetRawBin(pRetData, (LPCSTR)pData, (INT)dwOff);
    LocalFree(pData);
}

void UwWriteFile(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView name; name.FromBin(pArgInf[0]);
    INT nBytes = 0;
    LPBYTE pData = UniArgBinPtr(pArgInf[1], &nBytes);
    UniRetBool(pRetData, FALSE);
    if (name.nChars <= 0) return;
    HANDLE h = CreateFileW(name.p, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    INT nOff = 0;
    BOOL bOk = TRUE;
    while (pData != NULL && nOff < nBytes)
    {
        DWORD dwWritten = 0;
        if (!WriteFile(h, pData + nOff, nBytes - nOff, &dwWritten, NULL) || dwWritten == 0) { bOk = FALSE; break; }
        nOff += (INT)dwWritten;
    }
    CloseHandle(h);
    UniRetBool(pRetData, bOk);
}

// ============================================================ 取中间/取左右

static void MidBetweenRet(PMDATA_INF pRetData, const UniWView& src, const UniWView& pre, const UniWView& post,
                          INT nStart, BOOL bCaseInsensitive)
{
    INT nPre = UniFindWide(src.p, src.nChars, pre.p, (nStart == 0) ? 1 : nStart, bCaseInsensitive);
    if (nPre < 1) { UniRetWideBin(pRetData, g_wEmpty, 0); return; }
    INT nFrom = nPre + pre.nChars;
    INT nPost = UniFindWide(src.p, src.nChars, post.p, nFrom, bCaseInsensitive);
    if (nPost < nFrom) { UniRetWideBin(pRetData, g_wEmpty, 0); return; }
    UniRetWC(pRetData, src.p + (nFrom - 1), nPost - nFrom);
}

void UwMidBetween(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView pre; pre.FromBin(pArgInf[1]);
    UniWView post; post.FromBin(pArgInf[2]);
    MidBetweenRet(pRetData, src, pre, post, UniArgInt(pArgInf[3], 0), UniArgBool(pArgInf[4], FALSE));
}

void UwMidBetween2(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView pre; pre.FromUtf8(UniArgText(pArgInf[1]));
    UniWView post; post.FromUtf8(UniArgText(pArgInf[2]));
    MidBetweenRet(pRetData, src, pre, post, UniArgInt(pArgInf[3], 0), UniArgBool(pArgInf[4], FALSE));
}

static void RMidBetweenRet(PMDATA_INF pRetData, const UniWView& src, const UniWView& right, const UniWView& left,
                           INT nStart, BOOL bCaseInsensitive)
{
    INT nPos = nStart;
    if (nPos <= 0) nPos = src.nChars + 1;
    else if (nPos < src.nChars) nPos += right.nChars;
    INT nRight = UniRFindWide(src.p, src.nChars, right.p, nPos, bCaseInsensitive);
    if (nRight < 0) { UniRetWideBin(pRetData, g_wEmpty, 0); return; }
    INT nLeft = UniRFindWide(src.p, src.nChars, left.p, nRight, bCaseInsensitive);
    if (nLeft < 0) { UniRetWideBin(pRetData, g_wEmpty, 0); return; }
    nLeft += left.nChars;
    UniRetWC(pRetData, src.p + (nLeft - 1), nRight - nLeft);
}

void UwRMidBetween(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView right; right.FromBin(pArgInf[1]);
    UniWView left; left.FromBin(pArgInf[2]);
    RMidBetweenRet(pRetData, src, right, left, UniArgInt(pArgInf[3], 0), UniArgBool(pArgInf[4], FALSE));
}

void UwRMidBetween2(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView right; right.FromUtf8(UniArgText(pArgInf[1]));
    UniWView left; left.FromUtf8(UniArgText(pArgInf[2]));
    RMidBetweenRet(pRetData, src, right, left, UniArgInt(pArgInf[3], 0), UniArgBool(pArgInf[4], FALSE));
}

static void TextBeforeRet(PMDATA_INF pRetData, const UniWView& src, const UniWView& needle,
                          INT nStart, BOOL bCaseInsensitive)
{
    if (nStart <= 0) nStart = 1;
    INT nAt = UniFindWide(src.p, src.nChars, needle.p, nStart, bCaseInsensitive);
    if (nAt < 0) { UniRetWideBin(pRetData, g_wEmpty, 0); return; }
    UniRetWC(pRetData, src.pRaw, nAt - 1);        // 取文本左边W(文本, 位置-1)
}

void UwTextBefore(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView needle; needle.FromBin(pArgInf[1]);
    TextBeforeRet(pRetData, src, needle, UniArgInt(pArgInf[2], 0), UniArgBool(pArgInf[3], FALSE));
}

void UwTextBefore2(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView needle; needle.FromUtf8(UniArgText(pArgInf[1]));
    TextBeforeRet(pRetData, src, needle, UniArgInt(pArgInf[2], 0), UniArgBool(pArgInf[3], FALSE));
}

static void TextAfterRet(PMDATA_INF pRetData, const UniWView& src, const UniWView& needle, BOOL bCaseInsensitive)
{
    INT nAt = UniRFindWide(src.p, src.nChars, needle.p, 0, bCaseInsensitive);
    if (nAt < 0) { UniRetWideBin(pRetData, g_wEmpty, 0); return; }
    INT nKeep = src.nChars - needle.nChars - nAt + 1;
    if (nKeep < 0) nKeep = 0;
    INT nFrom = src.nRawChars - nKeep;
    if (nFrom < 0) nFrom = 0;
    UniRetWC(pRetData, src.pRaw + nFrom, nKeep);
}

void UwTextAfter(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView needle; needle.FromBin(pArgInf[1]);
    TextAfterRet(pRetData, src, needle, UniArgBool(pArgInf[3], FALSE));
}

void UwTextAfter2(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView needle; needle.FromUtf8(UniArgText(pArgInf[1]));
    TextAfterRet(pRetData, src, needle, UniArgBool(pArgInf[3], FALSE));
}

void UwCountOccur(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView needle; needle.FromBin(pArgInf[1]);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = CountOccur(src.p, src.nChars, needle.p, needle.nChars, !UniArgBool(pArgInf[2], FALSE));
}

void UwCountOccur2(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromUtf8(UniArgText(pArgInf[0]));
    UniWView needle; needle.FromUtf8(UniArgText(pArgInf[1]));
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = CountOccur(src.p, src.nChars, needle.p, needle.nChars, !UniArgBool(pArgInf[2], FALSE));
}

// ============================================================ 数组

void UwAryJoin(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    INT n = 0;
    LPBYTE pElem = UniAryRead(pArgInf[0], FALSE, &n);
    INT nSepBytes = 0;
    LPBYTE pSep = UniArgBinPtr(pArgInf[1], &nSepBytes);
    UniWBuf out;
    for (INT i = 0; i < n; i++)
    {
        UniWView one;
        ViewFromBinElem(((LPBYTE*)pElem)[i], &one);
        if (i > 0 && pSep != NULL && nSepBytes > 0) out.Add((LPCWSTR)(pSep + sizeof(INT) * 2), nSepBytes / 2);
        if (one.nRawChars > 0) out.Add(one.pRaw, one.nRawChars);   // 数组_输出为文本W 原样拼接
    }
    pRetData->m_dtDataType = SDT_BIN;
    pRetData->m_pBin = (out.nLen <= 0 || out.p == NULL) ? NULL : CloneBinData((LPCSTR)out.p, out.nLen * 2);
}

void UwAryJoinNull(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    INT n = 0;
    LPBYTE pElem = UniAryRead(pArgInf[0], TRUE, &n);
    UniWView sep; sep.FromBin(pArgInf[1]);
    UniWBuf out;
    for (INT i = 0; i < n; i++)
    {
        UniWView one;
        ViewFromBinElem(((LPBYTE*)pElem)[i], &one);
        if (i > 0) AddWFRaw(&out, sep);
        AddWFRaw(&out, one);
    }
    UniRetWideBin(pRetData, (out.p != NULL) ? out.p : g_wEmpty, out.nLen);
}

void UwSubReplaceAry(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWBuf chk;
    chk.Reserve(src.nChars + 1);
    chk.Add(src.p, src.nChars);
    const UniApi& a = UniGetApi();
    if (a.StrTrimW != NULL && chk.p != NULL) { a.StrTrimW(chk.p, g_wSpace); chk.nLen = lstrlenW(chk.p); }
    if (chk.p == NULL || chk.nLen <= 0) { UniRetWideBin(pRetData, src.p, src.nChars); return; }

    UniWBuf cur;
    cur.Reserve(src.nChars + 1);
    cur.Add(src.p, src.nChars);
    UniWView rep; rep.FromBin(pArgInf[2]);
    BOOL bCaseSensitive = UniArgBool(pArgInf[3], TRUE);
    INT n = 0;
    LPBYTE pElem = UniAryRead(pArgInf[1], FALSE, &n);
    for (INT i = 0; i < n; i++)
    {
        UniWView one;
        ViewFromBinElem(((LPBYTE*)pElem)[i], &one);
        if (one.nChars <= 0) continue;
        UniWBuf next;
        if (UniSubReplaceWide(cur.p, cur.nLen, one.p, rep.p, 1, 0, bCaseSensitive, &next) && next.p != NULL)
            cur.Steal(next);
    }
    UniRetWideBin(pRetData, (cur.p != NULL) ? cur.p : g_wEmpty, cur.nLen);
}

void UwAryUnique(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    INT n = 0;
    LPBYTE pElem = UniAryRead(pArgInf[0], TRUE, &n);
    BOOL bIns = UniArgBool(pArgInf[1], FALSE);
    UniAryList keep;
    for (INT i = 0; i < n; i++)
    {
        UniWView v;
        ViewFromBinElem(((LPBYTE*)pElem)[i], &v);
        INT nDup = -1;
        for (INT k = 0; k < keep.nCount; k++)
        {
            UniWView w;
            ViewFromBinElem((LPBYTE)keep.pItem[k], &w);
            if ((bIns ? lstrcmpiW(v.p, w.p) : lstrcmpW(v.p, w.p)) == 0) { nDup = k; break; }
        }
        if (nDup < 0) keep.Add(CloneBinData((LPCSTR)v.p, v.nChars * 2 + 2));
    }
    UniAryWrite(pArgInf[0], &keep);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = keep.nCount;
}

void UwAryShuffle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData);
    UNREFERENCED_PARAMETER(nArgCount);
    INT n = 0;
    LPBYTE pElem = UniAryRead(pArgInf[0], FALSE, &n);
    UniAryList out;
    if (pElem == NULL || n <= 0) { UniAryWrite(pArgInf[1], &out); return; }
    void** pool = (void**)LocalAlloc(LPTR, (SIZE_T)n * sizeof(void*));
    if (pool == NULL) { UniAryWrite(pArgInf[1], &out); return; }
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    srand((unsigned int)(li.u.LowPart ^ (unsigned int)GetTickCount()));
    for (INT i = 0; i < n; i++) pool[i] = ((void**)pElem)[i];
    INT nLeft = n;
    for (INT i = 0; i < n; i++)
    {
        INT k = (nLeft > 0) ? (rand() % nLeft) : 0;
        UniWView v;
        ViewFromBinElem((LPBYTE)pool[k], &v);
        out.Add(CloneBinData((LPCSTR)v.p, v.nChars * 2 + 2));
        pool[k] = pool[nLeft - 1];
        nLeft--;
    }
    LocalFree(pool);
    UniAryWrite(pArgInf[1], &out);
}

// ============================================================ 正则全局命令

static void RegMatchRet(PMDATA_INF pRetData, const UniWView& src, const UniWView& pat, INT nWant,
                        const UniWView& sep, BOOL bCaseSensitive, const MDATA_INF* pArgAry)
{
    CxRegex rx;
    memset(&rx, 0, sizeof(rx));
    if (!RxNew(&rx, pat.p, src.p, src.nChars, bCaseSensitive ? FALSE : TRUE, TRUE, TRUE, TRUE))
    {
        RxFree(&rx);
        pRetData->m_dtDataType = SDT_BIN;
        pRetData->m_pBin = NULL;                  // 原版创建失败返回空字节集
        return;
    }
    UniAryList ary;
    INT bAppend = 0;
    if (pArgAry != NULL && !UniIsNull(*pArgAry)) { UniAryCopyFrom(*pArgAry, &ary); bAppend = 1; }
    UniWBuf out;
    INT nTotal = RxMatchCount(&rx);
    for (INT i = 1; i <= nTotal; i++)
    {
        const WCHAR* val = RxMatchValue(&rx, i);
        INT nChars = (val != NULL) ? lstrlenW(val) : 0;
        if (nChars <= 0)
        {
            val = RxSubValue(&rx, i, 1);          // 主匹配为空时退回第 1 个子匹配（原版行为）
            nChars = (val != NULL) ? lstrlenW(val) : 0;
        }
        if (out.nLen > 0) out.Add(sep.p, sep.nChars);
        out.Add(val, nChars);
        if (bAppend) ary.Add(CloneBinData((LPCSTR)((val != NULL) ? val : g_wEmpty), nChars * 2 + 2));
        if (nWant != 0 && i == nWant) break;
    }
    if (bAppend) UniAryWrite(*pArgAry, &ary);
    UniRetWideBin(pRetData, (out.p != NULL) ? out.p : g_wEmpty, out.nLen);
    RxFree(&rx);
}

void UwRegMatch(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView pat; pat.FromBin(pArgInf[1]);
    INT nWant = UniIsNull(pArgInf[2]) ? 1 : UniArgInt(pArgInf[2], 1);
    UniWView sep; sep.FromBin(pArgInf[3]);
    RegMatchRet(pRetData, src, pat, nWant, sep, UniArgBool(pArgInf[4], TRUE),
                UniIsNull(pArgInf[5]) ? NULL : &pArgInf[5]);
}

void UwRegMatch2(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView pat; pat.FromUtf8(UniArgText(pArgInf[1]));
    INT nWant = UniIsNull(pArgInf[2]) ? 1 : UniArgInt(pArgInf[2], 1);
    UniWView sep; sep.FromBin(pArgInf[3]);
    RegMatchRet(pRetData, src, pat, nWant, sep, UniArgBool(pArgInf[4], TRUE),
                UniIsNull(pArgInf[5]) ? NULL : &pArgInf[5]);
}

void UwPickDigits(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView pat; pat.FromUtf8("[0-9]+");
    UniWView sep;
    RegMatchRet(pRetData, src, pat, 1, sep, TRUE, NULL);
}

void UwRegMatchAllSub(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    if (src.nOrigBytes <= 0 || (src.nOrigBytes == 2 && src.nChars <= 0))
    { UniRetWideBin(pRetData, g_wEmpty, 0); return; }
    UniWView pat; pat.FromBin(pArgInf[1]);
    INT bWantSub = UniIsNull(pArgInf[2]) ? 0 : 1;
    INT nSubIndex = UniArgInt(pArgInf[2], 1);
    INT nWant = UniIsNull(pArgInf[3]) ? 1 : UniArgInt(pArgInf[3], 1);
    UniWView sep; sep.FromBin(pArgInf[4]);
    BOOL bCaseSensitive = UniArgBool(pArgInf[5], TRUE);
    const MDATA_INF* pAry = UniIsNull(pArgInf[6]) ? NULL : &pArgInf[6];

    CxRegex rx;
    memset(&rx, 0, sizeof(rx));
    if (!RxNew(&rx, pat.p, src.p, src.nChars, bCaseSensitive ? FALSE : TRUE, TRUE, TRUE, TRUE))
    { RxFree(&rx); UniRetWideBin(pRetData, g_wEmpty, 0); return; }

    if (nWant == 1)
    {
        const WCHAR* val = bWantSub ? RxSubValue(&rx, 1, nSubIndex) : RxMatchValue(&rx, 1);
        INT nChars = (val != NULL) ? lstrlenW(val) : 0;
        if (pAry != NULL)
        {
            UniAryList ary;
            if (nChars > 0) ary.Add(CloneBinData((LPCSTR)val, nChars * 2 + 2));
            UniAryWrite(*pAry, &ary);             // 原版先清空数组
        }
        UniRetWideBin(pRetData, (nChars > 0) ? val : g_wEmpty, nChars);
        RxFree(&rx);
        return;
    }
    UniAryList ary;
    UniWBuf out;
    INT nTotal = RxMatchCount(&rx);
    for (INT i = 1; i <= nTotal; i++)
    {
        const WCHAR* val = bWantSub ? RxSubValue(&rx, i, nSubIndex) : RxMatchValue(&rx, i);
        INT nChars = (val != NULL) ? lstrlenW(val) : 0;
        if (out.nLen > 0) out.Add(sep.p, sep.nChars);
        out.Add(val, nChars);
        if (pAry != NULL) ary.Add(CloneBinData((LPCSTR)((val != NULL) ? val : g_wEmpty), nChars * 2 + 2));
        if (nWant != 0 && i == nWant) break;
    }
    if (pAry != NULL) UniAryWrite(*pAry, &ary);
    UniRetWideBin(pRetData, (out.p != NULL) ? out.p : g_wEmpty, out.nLen);
    RxFree(&rx);
}

// 文本_替换_正则W / 文本_替换_正则_子匹配W：逐个定位替换，并按剩余文本重建正则（原版算法）。
static INT RegReplaceCore(PMDATA_INF pArgInf, INT nSubIndex, INT bUseSub)
{
    UniWView src0; ViewFromRefBin(pArgInf[0], &src0);
    UniWView pat; pat.FromBin(pArgInf[1]);
    UniWView rep; rep.FromBin(pArgInf[2]);
    INT iCase = bUseSub ? 5 : 4;
    INT iGlobal = bUseSub ? 6 : 5;
    BOOL bCaseSensitive = UniArgBool(pArgInf[iCase], TRUE);
    BOOL bGlobal = UniArgBool(pArgInf[iGlobal], FALSE);

    UniWBuf cur;
    cur.Reserve(src0.nChars + 1);
    cur.Add(src0.p, src0.nChars);

    CxRegex rx;
    memset(&rx, 0, sizeof(rx));
    if (!RxNew(&rx, pat.p, cur.p, cur.nLen, bCaseSensitive ? FALSE : TRUE, FALSE, TRUE, TRUE))
    { RxFree(&rx); return 0; }

    INT nStart2 = 0, nCount = 0;
    INT nTotal = RxMatchCount(&rx);
    for (INT i = 1; i <= nTotal; i++)
    {
        const WCHAR* val = bUseSub ? RxSubValue(&rx, 1, nSubIndex) : RxMatchValue(&rx, 1);
        INT nValChars = (val != NULL) ? lstrlenW(val) : 0;
        if (nValChars <= 0) break;
        INT nStart = RxStartPosAt(&rx, 1, val);
        if (nStart2 != 0) nStart = nStart2 + nStart;

        UniWBuf next;
        BOOL bOk;
        if (bGlobal) bOk = UniSubReplaceWide(cur.p, cur.nLen, val, rep.p, 1, 0, bCaseSensitive, &next);
        else bOk = UniSubReplaceWide(cur.p, cur.nLen, val, rep.p, (nStart < 1) ? 1 : nStart, 1, bCaseSensitive, &next);
        if (!bOk || next.p == NULL) break;
        cur.Steal(next);
        nCount++;
        nStart2 = nStart + rep.nChars;

        // 截断剩余文本后重建正则：原版长度用 (总长 - start)，起始用 start2
        INT nSubject = cur.nLen;
        INT nFrom = nStart2 - 1;
        if (nFrom < 0) nFrom = 0;
        if (nFrom > nSubject) nFrom = nSubject;
        INT nTake = nSubject - nStart;
        if (nTake < 0) nTake = 0;
        if (nFrom + nTake > nSubject) nTake = nSubject - nFrom;
        RxFree(&rx);
        memset(&rx, 0, sizeof(rx));
        if (!RxNew(&rx, pat.p, cur.p + nFrom, nTake, bCaseSensitive ? FALSE : TRUE, FALSE, TRUE, TRUE)) break;
        UNREFERENCED_PARAMETER(i);             // 每轮固定取新集合的第 1 个匹配（原版行为）
    }
    RxFree(&rx);
    WriteBackBin(pArgInf[0], (cur.p != NULL) ? cur.p : g_wEmpty, cur.nLen);
    return nCount;
}

void UwRegReplace(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = RegReplaceCore(pArgInf, 0, FALSE);
}

void UwRegSubReplace(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    INT nSub = UniIsNull(pArgInf[3]) ? 1 : UniArgInt(pArgInf[3], 1);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = RegReplaceCore(pArgInf, nSub, TRUE);
}

// ============================================================ 批量取中间

static INT MidBatchRet(const UniWView& src, const UniWView& pre, const UniWView& post,
                       const MDATA_INF& argAry, INT bAryIsText, INT bAppend, BOOL bCaseSensitiveParam,
                       INT nLenLimit, const UniWView& prefix, const UniWView& suffix,
                       BOOL bWithPre, BOOL bWithPost, INT nStart, BOOL bTrim)
{
    BOOL bIns = bCaseSensitiveParam ? FALSE : TRUE;   // 原版对参数取反后使用
    UniAryList ary;
    if (bAppend) UniAryCopyFrom(argAry, &ary);
    INT nPos = nStart;
    for (;;)
    {
        INT nPre = UniFindWide(src.p, src.nChars, pre.p, (nPos == 0) ? 1 : nPos, bIns);
        INT nPost = UniFindWide(src.p, src.nChars, post.p, nPre + pre.nChars, bIns);
        if (nPre < 1 || nPost < 1) break;
        INT nFrom = nPre + pre.nChars;
        INT nChars = nPost - nFrom;
        if (nChars < 0) nChars = 0;

        UniWBuf seg;
        seg.Reserve(nChars + 1);
        for (INT i = 0; i < nChars; i++) if (src.p[nFrom - 1 + i] != 0) seg.AddChar(src.p[nFrom - 1 + i]);
        if (bTrim)
        {
            const UniApi& a = UniGetApi();
            if (a.StrTrimW != NULL && seg.p != NULL) { a.StrTrimW(seg.p, g_wSpace); seg.nLen = lstrlenW(seg.p); }
        }
        INT nSeg = (seg.p != NULL) ? lstrlenW(seg.p) : 0;
        if (nSeg <= 0) break;
        if (nLenLimit != 0 && nSeg > nLenLimit) break;

        UniWBuf item;
        item.Reserve(pre.nChars + prefix.nChars + nSeg + suffix.nChars + post.nChars + 2);
        if (bWithPre) item.Add(pre.p, pre.nChars);
        item.Add(prefix.p, prefix.nChars);
        item.Add(seg.p, nSeg);
        item.Add(suffix.p, suffix.nChars);
        if (bWithPost) item.Add(post.p, post.nChars);
        UniWBuf fixed;
        fixed.Reserve(item.nLen + 1);
        for (INT i = 0; i < item.nLen; i++) if (item.p[i] != 0) fixed.AddChar(item.p[i]);   // 宽文本_字符处理

        if (bAryIsText)
        {
            char* sz = UniWideToUtf8(fixed.p != NULL ? fixed.p : g_wEmpty, fixed.nLen);
            ary.Add((sz != NULL) ? CloneTextData(sz) : CloneTextData("", 0));
            if (sz != NULL) LocalFree(sz);
        }
        else ary.Add(CloneBinData((LPCSTR)(fixed.p != NULL ? fixed.p : g_wEmpty), fixed.nLen * 2 + 2));
        nPos = nPost;
    }
    UniAryWrite(argAry, &ary);
    return ary.nCount;
}

void UwMidBatch(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView pre; pre.FromBin(pArgInf[1]);
    UniWView post; post.FromBin(pArgInf[2]);
    UniWView prefix; prefix.FromBin(pArgInf[6]);
    UniWView suffix; suffix.FromBin(pArgInf[7]);
    BOOL bClear = UniIsNull(pArgInf[12]) ? TRUE : UniArgBool(pArgInf[12], TRUE);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = MidBatchRet(src, pre, post, pArgInf[3], FALSE, bClear ? 0 : 1,
        UniArgBool(pArgInf[4], FALSE), UniArgInt(pArgInf[5], 0), prefix, suffix,
        UniArgBool(pArgInf[8], FALSE), UniArgBool(pArgInf[9], FALSE), UniArgInt(pArgInf[10], 0),
        UniIsNull(pArgInf[11]) ? TRUE : UniArgBool(pArgInf[11], TRUE));
}

void UwMidBatch2(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView pre; pre.FromUtf8(UniArgText(pArgInf[1]));
    UniWView post; post.FromUtf8(UniArgText(pArgInf[2]));
    UniWView prefix; prefix.FromBin(pArgInf[6]);
    UniWView suffix; suffix.FromBin(pArgInf[7]);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = MidBatchRet(src, pre, post, pArgInf[3], FALSE, 0,
        UniArgBool(pArgInf[4], FALSE), UniArgInt(pArgInf[5], 0), prefix, suffix,
        UniArgBool(pArgInf[8], FALSE), UniArgBool(pArgInf[9], FALSE), UniArgInt(pArgInf[10], 0),
        UniIsNull(pArgInf[11]) ? TRUE : UniArgBool(pArgInf[11], TRUE));
}

void UwMidBatch3(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView pre; pre.FromBin(pArgInf[1]);
    UniWView post; post.FromBin(pArgInf[2]);
    UniWView prefix; prefix.FromBin(pArgInf[6]);
    UniWView suffix; suffix.FromBin(pArgInf[7]);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = MidBatchRet(src, pre, post, pArgInf[3], TRUE, 0,
        UniArgBool(pArgInf[4], FALSE), UniArgInt(pArgInf[5], 0), prefix, suffix,
        UniArgBool(pArgInf[8], FALSE), UniArgBool(pArgInf[9], FALSE), UniArgInt(pArgInf[10], 0),
        UniIsNull(pArgInf[11]) ? TRUE : UniArgBool(pArgInf[11], TRUE));
}

// ============================================================ HTML 标签闭合取中间

void UwHtmlMid(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView src; src.FromBin(pArgInf[0]);
    UniWView pre; pre.FromBin(pArgInf[1]);
    INT nStart = UniArgInt(pArgInf[2], 0);
    BOOL bIns = UniArgBool(pArgInf[3], FALSE);

    // 标签名称 = 子文本替换2W(取文本_正则2W(前面文本, "<\w+"), "<")
    UniWBuf nameBuf;
    {
        UniWView tagPat; tagPat.FromUtf8("<\\w+");
        CxRegex rx;
        memset(&rx, 0, sizeof(rx));
        if (RxNew(&rx, tagPat.p, pre.p, pre.nChars, FALSE, TRUE, TRUE, TRUE))
        {
            const WCHAR* val = RxMatchValue(&rx, 1);
            UniWView lt; lt.FromUtf8("<");
            if (val != NULL) UniSubReplaceWide(val, lstrlenW(val), lt.p, NULL, 1, 0, TRUE, &nameBuf);
        }
        RxFree(&rx);
    }
    const WCHAR* pName = (nameBuf.p != NULL) ? nameBuf.p : g_wEmpty;
    INT nName = (nameBuf.p != NULL) ? lstrlenW(nameBuf.p) : 0;

    UniWBuf closeTag, openTag;
    closeTag.Reserve(nName + 3);
    closeTag.Add(L"</", 2); closeTag.Add(pName, nName); closeTag.Add(L">", 1);
    openTag.Reserve(nName + 2);
    openTag.Add(L"<", 1); openTag.Add(pName, nName);

    INT nPrePos = UniFindWide(src.p, src.nChars, pre.p, (nStart == 0) ? 1 : nStart, bIns);
    if (nPrePos < 0) { UniRetWideBin(pRetData, g_wEmpty, 0); return; }
    nPrePos += pre.nChars;

    INT nAfter = UniFindWide(src.p, src.nChars, closeTag.p, nPrePos, bIns);
    INT nBefore = UniFindWide(src.p, src.nChars, openTag.p, nPrePos, bIns);
    if (nBefore > 0) nBefore += openTag.nLen;
    while (nBefore > 0 && nAfter > 0 && nAfter < nBefore)
    {
        nAfter = UniFindWide(src.p, src.nChars, closeTag.p, nBefore, bIns);
        if (nAfter < 0) break;
        nAfter += closeTag.nLen;
        nBefore = UniFindWide(src.p, src.nChars, openTag.p, nAfter, bIns);
    }
    INT nPostPos = (nAfter > 0) ? UniFindWide(src.p, src.nChars, closeTag.p, nAfter, bIns) : -1;
    if (nPostPos < 0) nPostPos = nAfter - closeTag.nLen;   // 内嵌标签缺闭合时用外层末尾
    if (nPostPos < nPrePos) { UniRetWideBin(pRetData, g_wEmpty, 0); return; }
    UniRetWC(pRetData, src.p + (nPrePos - 1), nPostPos - nPrePos);
}

// ============================================================ 扩展模块

void UexFormatW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    // 末尾确为 {0,0} 时删 2 字节，否则原样：两种情况都等于取前 nRawChars-1 个字符。
    RetRawLeft(pRetData, v.pRaw, v.nRawChars, v.nRawChars - 1);
}

void UexFormatW2(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; ViewFromRefBin(pArgInf[0], &v);
    INT n = v.nRawChars;                            // 含终止符的原始区域
    UniRetWideBin(pRetData, v.pRaw, n - 1);
    WriteBackBin(pArgInf[0], v.pRaw, n - 1);
}

void UexVarAddr(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = SDT_INT;
    // 通用型 传址 参数：联合体里放的就是变量自身的地址。
    pRetData->m_int = (INT)(INT_PTR)pArgInf[0].m_pCompoundData;
}

void UexStructPtr(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = SDT_INT;
    if (pArgInf[0].m_pCompoundData == NULL) pRetData->m_int = 0;
    else pRetData->m_int = *(PINT)pArgInf[0].m_pCompoundData;   // 自定义数据类型变量的数据指针
}

typedef INT (CDECL *PFN_CALL_WITH_THIS)(INT, INT, INT, INT, INT, INT, INT, INT, INT);

void UexThisCall(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = 0;
    INT thisPtr = UniArgInt(pArgInf[0], 0);
    INT index = UniArgInt(pArgInf[1], 0);
    if (thisPtr == 0) return;
    INT pVtbl = *(PINT)(INT_PTR)thisPtr;
    if (pVtbl == 0) return;
    INT pFunc = *(PINT)(pVtbl + index * 4);
    if (pFunc == 0) return;
    INT a[8];
    for (INT i = 0; i < 8; i++) a[i] = UniArgInt(pArgInf[2 + i], 0);
    pRetData->m_int = ((PFN_CALL_WITH_THIS)pFunc)(thisPtr, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
}

void UexWTextFix(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    UniRetWC(pRetData, v.pRaw, v.nRawChars);
}

void UexWC(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UexWTextFix(pRetData, nArgCount, pArgInf);
}

void UexWF(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    BOOL bStd = UniArgBool(pArgInf[1], FALSE);
    if (v.nOrigBytes <= 0 || (v.nOrigBytes == 2 && v.nChars <= 0))
    { pRetData->m_dtDataType = SDT_BIN; pRetData->m_pBin = NULL; return; }
    if (!bStd)
    {
        INT n = v.nRawChars - 1;                     // 去末尾 {0,0}
        RetRawLeft(pRetData, v.pRaw, v.nRawChars, (n > 0) ? n : 0);
        return;
    }
    INT nKeep = v.nChars;                            // 标准 Unicode：截到第一个 {0,0}
    if (nKeep <= 0) { pRetData->m_dtDataType = SDT_BIN; pRetData->m_pBin = NULL; return; }
    UniRetWideBin(pRetData, v.pRaw, nKeep);
}

static BOOL IsWTextEmpty(const UniWView& v)
{
    if (v.nOrigBytes <= 0) return TRUE;              // 空字节集
    if (v.nOrigBytes == 2 && ((const BYTE*)v.pRaw)[0] == 0 && ((const BYTE*)v.pRaw)[1] == 0) return TRUE;
    return FALSE;
}

void UexWTextIsEmpty(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    UniRetBool(pRetData, IsWTextEmpty(v) ? TRUE : FALSE);
}

void UexWIsNull(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UexWTextIsEmpty(pRetData, nArgCount, pArgInf);
}

void UexW_ISNULL(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UexWTextIsEmpty(pRetData, nArgCount, pArgInf);
}

void UexWTextEmpty(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniRetWideBin(pRetData, g_wEmpty, 0);
}

void UexWToInt(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    const UniApi& a = UniGetApi();
    INT nRet = 0;
    if (a.StrToIntExW != NULL) a.StrToIntExW(v.p, UNI_STIF_DEFAULT, &nRet);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = nRet;
}

void UexWToDouble(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView v; v.FromBin(pArgInf[0]);
    pRetData->m_dtDataType = SDT_DOUBLE;
    // VC6 CRT 没有 wcstod：先转 UTF-8 再用 strtod，且不做整数互转。
    char* sz = UniWideToUtf8(v.p, v.nChars);
    DOUBLE dRet = 0.0;
    if (sz != NULL)
    {
        dRet = strtod(sz, NULL);
        LocalFree(sz);
    }
    pRetData->m_double = dRet;
}

void UexDoubleToW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    DOUBLE d = pArgInf[0].m_double;
    // 不用 swprintf/_snwprintf：新 CRT 会把它们引到 ___stdio_common_vswprintf，VC6 静态链接取不到。
    // 走 oleaut32 的 VarBstrFromR8（运行期解析），格式化也与易语言 到文本() 一致。
    const UniApi& a = UniGetApi();
    void* pBstr = NULL;
    INT nChars = 0;
    if (a.VarBstrFromR8 != NULL && SUCCEEDED(a.VarBstrFromR8(d, 0x804, 0 /*VARFLAG_LOCAL*, 用 locale 0x804 的小数点*/, &pBstr)) && pBstr != NULL)
    {
        const WCHAR* w = (const WCHAR*)pBstr;
        nChars = ((INT*)w)[-1] / 2;
        UniRetWideBin(pRetData, w, nChars);
        if (a.SysFreeString != NULL) a.SysFreeString(pBstr);
        return;
    }
    UniRetWideBin(pRetData, g_wEmpty, 0);
}

void UexBinBlank(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UwBlankText(pRetData, nArgCount, pArgInf);
}

void UexWTextEqual(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniWView a; a.FromBin(pArgInf[0]);
    UniWView b; b.FromBin(pArgInf[1]);
    INT la = a.nOrigBytes, lb = b.nOrigBytes;
    if (la >= 2 && ((const BYTE*)a.pRaw)[la - 2] == 0 && ((const BYTE*)a.pRaw)[la - 1] == 0) la -= 2;
    if (lb >= 2 && ((const BYTE*)b.pRaw)[lb - 2] == 0 && ((const BYTE*)b.pRaw)[lb - 1] == 0) lb -= 2;
    BOOL bEq = FALSE;
    if (la == lb)
        bEq = (la <= 0) ? TRUE : ((memcmp(a.pRaw, b.pRaw, (SIZE_T)la) == 0) ? TRUE : FALSE);
    UniRetBool(pRetData, bEq);
}

void UexCoInit(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    const UniApi& a = UniGetApi();
    if (a.CoInitialize != NULL) a.CoInitialize(NULL);
}

void UexCoFree(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    const UniApi& a = UniGetApi();
    if (a.CoUninitialize != NULL) a.CoUninitialize();
}

}  // namespace cx
