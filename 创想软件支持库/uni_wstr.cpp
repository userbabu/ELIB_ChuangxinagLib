// 宽文本类（迁移自 创想软件UNICODE模块\宽文本类 + 宽文本操作对象的 zyString 实现）。
// 原生 CxWStr 缓冲（UTF-16 + 长度 + 容量），位置从 0 开始、索引从 0 开始，与模块原版一致。

#include <cx_uni.h>

namespace cx {

extern PFN_NOTIFY_SYS g_pfnNotifySys;
INT NotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2);
char* CloneTextData(LPCSTR pszText, INT nLen);
char* CloneTextData(LPCSTR pszText);
LPBYTE CloneBinData(LPCSTR pData, INT nDataSize);
HMODULE UniSelfModule();

static CxWStr* WsArgSelf(const MDATA_INF& arg)
{
    if (arg.m_pCompoundData == NULL) return NULL;
    return (CxWStr*)(INT_PTR)*(PINT)arg.m_pCompoundData;
}

// 原版所有写命令在对象指针为 0 时自动 创建()。
static CxWStr* WsEnsure(const MDATA_INF& arg)
{
    if (arg.m_pCompoundData == NULL) return NULL;
    CxWStr* p = WsArgSelf(arg);
    if (p == NULL)
    {
        p = WsNew(11);
        *(PINT)arg.m_pCompoundData = (INT)(INT_PTR)p;
    }
    return p;
}

static CxWStr* WsOther(const MDATA_INF& arg)
{
    return WsArgSelf(arg);
}

// 返回一个新的宽文本对象（对象数据 = 一个 INT，存 CxWStr 指针）。
static void RetWStrObject(PMDATA_INF pRet, CxWStr* p)
{
    pRet->m_dtDataType = MAKELONG(CX_TYPE_WSTR, 0);
    INT* pBlock = (INT*)NotifySys(NRS_MALLOC, sizeof(INT), 0);
    if (pBlock == NULL) { pRet->m_pCompoundData = NULL; return; }
    if (p == NULL) p = WsNew(11);
    *pBlock = (INT)(INT_PTR)p;
    pRet->m_pCompoundData = pBlock;
}

// ============================================================ 生命周期

void WsConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    *(PINT)pArgInf[0].m_pCompoundData = 0;         // 创建() 时才分配缓冲区
}

void WsCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    *(PINT)pArgInf[0].m_pCompoundData = (INT)(INT_PTR)WsClone(WsOther(pArgInf[1]));
}

void WsDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    if (p != NULL)
    {
        WsFree(p);
        *(PINT)pArgInf[0].m_pCompoundData = 0;
    }
}

// ============================================================ 指针互转

void WsGetPtr(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (INT)(INT_PTR)WsArgSelf(pArgInf[0]);
}

void WsSetPtr(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    INT ptr = UniArgInt(pArgInf[1], 0);
    if (ptr == 0) return;                          // 原版：0 直接返回
    if (pArgInf[0].m_pCompoundData == NULL) return;
    *(PINT)pArgInf[0].m_pCompoundData = ptr;
}

void WsCreate(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniRetBool(pRetData, FALSE);
    INT nCap = UniIsNull(pArgInf[1]) ? 11 : UniArgInt(pArgInf[1], 11);
    if (nCap <= 0) nCap = 12;
    CxWStr* p = WsNew(nCap);
    if (p == NULL) return;
    if (pArgInf[0].m_pCompoundData != NULL)
    {
        CxWStr* pOld = WsArgSelf(pArgInf[0]);
        if (pOld != NULL) WsFree(pOld);
        *(PINT)pArgInf[0].m_pCompoundData = (INT)(INT_PTR)p;
    }
    UniRetBool(pRetData, TRUE);
}

void WsAddRes(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniRetBool(pRetData, FALSE);
    CxWStr* p = WsEnsure(pArgInf[0]);
    if (p == NULL) return;
    INT nId = UniArgInt(pArgInf[1], 0);
    INT nBuf = UniArgInt(pArgInf[2], 0);
    if (nBuf <= 0) return;
    WCHAR* q = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)(nBuf + 1) * sizeof(WCHAR));
    if (q == NULL) { UniRetBool(pRetData, FALSE); return; }
    int nLen = LoadStringW(UniSelfModule(), (UINT)nId, q, nBuf);
    if (nLen > 0) UniRetBool(pRetData, WsAppend(p, q, nLen) ? TRUE : FALSE);
    LocalFree(q);
}

void WsAddPtrW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsEnsure(pArgInf[0]);
    INT ptr = UniArgInt(pArgInf[1], 0);
    INT nChars = UniIsNull(pArgInf[2]) ? ((ptr != 0) ? lstrlenW((LPCWSTR)(INT_PTR)ptr) : 0) : UniArgInt(pArgInf[2], 0);
    UniRetBool(pRetData, (p != NULL && ptr != 0) ? (WsAppend(p, (LPCWSTR)(INT_PTR)ptr, nChars) ? TRUE : FALSE) : FALSE);
}

void WsAddPtrA(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsEnsure(pArgInf[0]);
    UniRetBool(pRetData, FALSE);
    INT ptr = UniArgInt(pArgInf[1], 0);
    if (p == NULL || ptr == 0) return;
    INT nChars = UniIsNull(pArgInf[2]) ? lstrlenA((LPCSTR)(INT_PTR)ptr) : UniArgInt(pArgInf[2], 0);
    UINT uCP = (UINT)(UniIsNull(pArgInf[3]) ? 936 : UniArgInt(pArgInf[3], 936));
    INT nWide = MultiByteToWideChar(uCP, 0, (LPCSTR)(INT_PTR)ptr, nChars, NULL, 0);
    if (nWide <= 0) return;
    WCHAR* q = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)(nWide + 1) * sizeof(WCHAR));
    if (q == NULL) return;
    if (MultiByteToWideChar(uCP, 0, (LPCSTR)(INT_PTR)ptr, nChars, q, nWide) > 0)
        UniRetBool(pRetData, WsAppend(p, q, nWide) ? TRUE : FALSE);
    LocalFree(q);
}

// ============================================================ 添加/插入

void WsAdd(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsEnsure(pArgInf[0]);
    UniWView v; v.FromUtf8(UniArgText(pArgInf[1]));
    UniRetBool(pRetData, (p != NULL) ? (WsAppend(p, v.p, v.nChars) ? TRUE : FALSE) : FALSE);
}

void WsAddW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsEnsure(pArgInf[0]);
    UniWView v; v.FromBin(pArgInf[1]);
    UniRetBool(pRetData, (p != NULL) ? (WsAppend(p, v.p, v.nChars) ? TRUE : FALSE) : FALSE);
}

void WsAddSelf(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsEnsure(pArgInf[0]);
    CxWStr* o = WsOther(pArgInf[1]);
    UniRetBool(pRetData, (p != NULL && o != NULL) ? (WsAppend(p, o->pStr, o->nLen) ? TRUE : FALSE) : FALSE);
}

void WsAddLine(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsEnsure(pArgInf[0]);
    UniWView v; v.FromUtf8(UniArgText(pArgInf[1]));
    UniRetBool(pRetData, (p != NULL) ? (WsAppendLine(p, v.p, v.nChars) ? TRUE : FALSE) : FALSE);
}

void WsAddLineW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsEnsure(pArgInf[0]);
    UniWView v; v.FromBin(pArgInf[1]);
    UniRetBool(pRetData, (p != NULL) ? (WsAppendLine(p, v.p, v.nChars) ? TRUE : FALSE) : FALSE);
}

void WsAddLineSelf(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsEnsure(pArgInf[0]);
    CxWStr* o = WsOther(pArgInf[1]);
    UniRetBool(pRetData, (p != NULL && o != NULL) ? (WsAppendLine(p, o->pStr, o->nLen) ? TRUE : FALSE) : FALSE);
}

void WsAddInt(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsEnsure(pArgInf[0]);
    UniRetBool(pRetData, (p != NULL) ? (WsAppendInt(p, UniArgInt(pArgInf[1], 0)) ? TRUE : FALSE) : FALSE);
}

void WsInsertText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsEnsure(pArgInf[0]);
    UniWView v; v.FromUtf8(UniArgText(pArgInf[1]));
    UniRetBool(pRetData, (p != NULL) ? (WsInsert(p, UniArgInt(pArgInf[2], 0), v.p, v.nChars) ? TRUE : FALSE) : FALSE);
}

void WsInsertW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsEnsure(pArgInf[0]);
    UniWView v; v.FromBin(pArgInf[1]);
    UniRetBool(pRetData, (p != NULL) ? (WsInsert(p, UniArgInt(pArgInf[2], 0), v.p, v.nChars) ? TRUE : FALSE) : FALSE);
}

void WsInsertSelf(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsEnsure(pArgInf[0]);
    CxWStr* o = WsOther(pArgInf[1]);
    UniRetBool(pRetData, (p != NULL && o != NULL) ? (WsInsert(p, UniArgInt(pArgInf[2], 0), o->pStr, o->nLen) ? TRUE : FALSE) : FALSE);
}

// ============================================================ 取内容

void WsGetText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniRetWideText(pRetData, (p != NULL && p->pStr != NULL) ? p->pStr : L"", (p != NULL) ? p->nLen : 0);
}

void WsGetTextW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    if (p == NULL || p->pStr == NULL) { UniRetWideBin(pRetData, L"", 0); return; }
    UniRetWideBin(pRetData, p->pStr, p->nLen);
}

void WsGetTextWN(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    if (p == NULL || p->pStr == NULL) { pRetData->m_dtDataType = SDT_BIN; pRetData->m_pBin = NULL; return; }
    UniRetWideBinNoNull(pRetData, p->pStr, p->nLen);
}

void WsGetStrPtr(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (p != NULL && p->pStr != NULL) ? (INT)(INT_PTR)p->pStr : 0;
}

void WsCmdClear(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    WsClear(WsArgSelf(pArgInf[0]));
}

void WsCmdDestroy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    if (p != NULL)
    {
        WsFree(p);
        if (pArgInf[0].m_pCompoundData != NULL) *(PINT)pArgInf[0].m_pCompoundData = 0;
    }
}

// ============================================================ 字符级

void WsAddChar(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsEnsure(pArgInf[0]);
    WCHAR c = (WCHAR)(unsigned short)UniArgInt(pArgInf[1], 0);
    UniRetBool(pRetData, (p != NULL) ? (WsAppend(p, &c, 1) ? TRUE : FALSE) : FALSE);
}

void WsGetChar(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    INT nIdx = UniArgInt(pArgInf[1], 0);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = 0;
    if (p == NULL || p->pStr == NULL || nIdx < 0 || nIdx >= p->nLen) return;
    pRetData->m_int = (INT)(unsigned short)p->pStr[nIdx];
}

void WsSetChar(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    INT nIdx = UniArgInt(pArgInf[1], 0);
    UniRetBool(pRetData, FALSE);
    if (p == NULL || p->pStr == NULL || nIdx < 0 || nIdx >= p->nLen) return;
    p->pStr[nIdx] = (WCHAR)(unsigned short)UniArgInt(pArgInf[2], 0);
    UniRetBool(pRetData, TRUE);
}

// ============================================================ 比较与长度

void WsCmdCompare(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniWView v; v.FromUtf8(UniArgText(pArgInf[1]));
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (p != NULL) ? WsCompare(p, v.p) : 0;
}

void WsCmdCompareW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniWView v; v.FromBin(pArgInf[1]);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (p != NULL) ? WsCompare(p, v.p) : 0;
}

void WsCmdCompareSelf(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    CxWStr* o = WsOther(pArgInf[1]);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (p != NULL) ? WsCompare(p, (o != NULL && o->pStr != NULL) ? o->pStr : L"") : 0;
}

void WsCmdLength(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (p != NULL) ? p->nLen : 0;
}

// ============================================================ 派生对象

void WsCmdClone(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    RetWStrObject(pRetData, WsClone(WsArgSelf(pArgInf[0])));
}

void WsCmdUpper(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    CxWStr* q = WsClone(p);
    WsToUpper(q);
    RetWStrObject(pRetData, q);
}

void WsCmdLower(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    CxWStr* q = WsClone(p);
    WsToLower(q);
    RetWStrObject(pRetData, q);
}

void WsCmdFull(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    CxWStr* q = WsClone(p);
    WsToFullWidth(q);
    RetWStrObject(pRetData, q);
}

void WsCmdHalf(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    CxWStr* q = WsClone(p);
    WsToHalfWidth(q);
    RetWStrObject(pRetData, q);
}

// ============================================================ 寻找

void WsCmdFindW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniWView v; v.FromBin(pArgInf[1]);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (p != NULL) ? WsFind(p, v.p, UniArgInt(pArgInf[2], 0), UniArgBool(pArgInf[3], FALSE)) : -1;
}

void WsCmdFind(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniWView v; v.FromUtf8(UniArgText(pArgInf[1]));
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (p != NULL) ? WsFind(p, v.p, UniArgInt(pArgInf[2], 0), UniArgBool(pArgInf[3], FALSE)) : -1;
}

void WsCmdFindSelf(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    CxWStr* o = WsOther(pArgInf[1]);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (p != NULL) ? WsFind(p, (o != NULL && o->pStr != NULL) ? o->pStr : L"",
        UniArgInt(pArgInf[2], 0), UniArgBool(pArgInf[3], FALSE)) : -1;
}

void WsCmdRFindW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniWView v; v.FromBin(pArgInf[1]);
    INT nPos = UniIsNull(pArgInf[2]) ? ((p != NULL) ? p->nLen : 0) : UniArgInt(pArgInf[2], 0);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (p != NULL) ? WsRFind(p, v.p, nPos, UniArgBool(pArgInf[3], FALSE)) : -1;
}

void WsCmdRFind(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniWView v; v.FromUtf8(UniArgText(pArgInf[1]));
    INT nPos = UniIsNull(pArgInf[2]) ? ((p != NULL) ? p->nLen : 0) : UniArgInt(pArgInf[2], 0);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (p != NULL) ? WsRFind(p, v.p, nPos, UniArgBool(pArgInf[3], FALSE)) : -1;
}

void WsCmdRFindSelf(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    CxWStr* o = WsOther(pArgInf[1]);
    INT nPos = UniIsNull(pArgInf[2]) ? ((p != NULL) ? p->nLen : 0) : UniArgInt(pArgInf[2], 0);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (p != NULL) ? WsRFind(p, (o != NULL && o->pStr != NULL) ? o->pStr : L"",
        nPos, UniArgBool(pArgInf[3], FALSE)) : -1;
}

// ============================================================ 替换

void WsCmdReplaceW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniWView v; v.FromBin(pArgInf[3]);
    UniRetBool(pRetData, (p != NULL)
        ? (WsReplaceAt(p, UniArgInt(pArgInf[1], 0), UniArgInt(pArgInf[2], 0), v.p, v.nChars) ? TRUE : FALSE) : FALSE);
}

void WsCmdReplace(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniWView v; v.FromUtf8(UniArgText(pArgInf[3]));
    UniRetBool(pRetData, (p != NULL)
        ? (WsReplaceAt(p, UniArgInt(pArgInf[1], 0), UniArgInt(pArgInf[2], 0), v.p, v.nChars) ? TRUE : FALSE) : FALSE);
}

void WsCmdReplaceSelf(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    CxWStr* o = WsOther(pArgInf[3]);
    UniRetBool(pRetData, (p != NULL)
        ? (WsReplaceAt(p, UniArgInt(pArgInf[1], 0), UniArgInt(pArgInf[2], 0),
              (o != NULL) ? o->pStr : NULL, (o != NULL) ? o->nLen : 0) ? TRUE : FALSE) : FALSE);
}

// 原版的最后一个参数名义是「是否区分大小写」，实际当作「不区分大小写」传给 zyString，这里保持同一行为。
static BOOL SubInsensitive(const MDATA_INF& arg)
{
    return UniArgBool(arg, TRUE);
}

void WsCmdSubW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniWView o; o.FromBin(pArgInf[1]);
    UniWView n; n.FromBin(pArgInf[2]);
    UniRetBool(pRetData, (p != NULL)
        ? (WsSubReplace(p, o.p, n.p, UniArgInt(pArgInf[3], 0), UniArgInt(pArgInf[4], 0), SubInsensitive(pArgInf[5])) ? TRUE : FALSE) : FALSE);
}

void WsCmdSub(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniWView o; o.FromUtf8(UniArgText(pArgInf[1]));
    UniWView n; n.FromUtf8(UniArgText(pArgInf[2]));
    UniRetBool(pRetData, (p != NULL)
        ? (WsSubReplace(p, o.p, n.p, UniArgInt(pArgInf[3], 0), UniArgInt(pArgInf[4], 0), SubInsensitive(pArgInf[5])) ? TRUE : FALSE) : FALSE);
}

void WsCmdSubSelf(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    CxWStr* o = WsOther(pArgInf[1]);
    CxWStr* n = WsOther(pArgInf[2]);
    UniRetBool(pRetData, (p != NULL && o != NULL)
        ? (WsSubReplace(p, o->pStr, (n != NULL) ? n->pStr : NULL,
              UniArgInt(pArgInf[3], 0), UniArgInt(pArgInf[4], 0), SubInsensitive(pArgInf[5])) ? TRUE : FALSE) : FALSE);
}

// ============================================================ 取子段

void WsCmdMid(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    RetWStrObject(pRetData, WsSub(WsArgSelf(pArgInf[0]), UniArgInt(pArgInf[1], 0), UniArgInt(pArgInf[2], 0)));
}

void WsCmdLeft(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    RetWStrObject(pRetData, WsSub(p, 0, UniArgInt(pArgInf[1], 0)));
}

void WsCmdRight(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    INT nLen = UniArgInt(pArgInf[1], 0);
    INT nStart = (p != NULL ? p->nLen : 0) - nLen;
    if (nStart < 0) nStart = 0;
    RetWStrObject(pRetData, WsSub(p, nStart, nLen));
}

// ============================================================ 裁剪与数值

void WsCmdTrimW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniWView v; v.FromBin(pArgInf[1]);
    UniRetBool(pRetData, (p != NULL) ? (WsTrimChars(p, v.p) ? TRUE : FALSE) : FALSE);
}

void WsCmdTrim(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniWView v; v.FromUtf8(UniArgText(pArgInf[1]));
    UniRetBool(pRetData, (p != NULL) ? (WsTrimChars(p, v.p) ? TRUE : FALSE) : FALSE);
}

void WsCmdTrimSelf(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    CxWStr* o = WsOther(pArgInf[1]);
    UniRetBool(pRetData, (p != NULL && o != NULL) ? (WsTrimChars(p, o->pStr) ? TRUE : FALSE) : FALSE);
}

void WsCmdTrimSpace(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    const UniApi& a = UniGetApi();
    static const WCHAR szSpace[2] = { ' ', 0 };
    UniRetBool(pRetData, (p != NULL && p->pStr != NULL && a.StrTrimW != NULL)
        ? (a.StrTrimW(p->pStr, szSpace) ? (p->nLen = lstrlenW(p->pStr), TRUE) : FALSE) : FALSE);
}

void WsCmdToInt(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = WsToInt(WsArgSelf(pArgInf[0]), UniArgBool(pArgInf[1], FALSE));
}

// ============================================================ 分割

void WsCmdSplitW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniWView sep; sep.FromBin(pArgInf[1]);
    BOOL bRemoveEmpty = UniIsNull(pArgInf[3]) ? TRUE : UniArgBool(pArgInf[3], TRUE);
    UniWRangeList list;
    if (p != NULL) WsSplit(p, sep.p, sep.nChars, bRemoveEmpty, &list);
    UniAryList ary;
    for (INT i = 0; i < list.nCount; i++)
    {
        CxWStr* seg = WsSub(p, list.pStart[i], list.pChars[i]);
        INT nChars = (seg != NULL) ? seg->nLen : 0;
        ary.Add(CloneBinData((LPCSTR)((seg != NULL && seg->pStr != NULL) ? seg->pStr : L""), nChars * 2 + 2));
        WsFree(seg);
    }
    UniAryWrite(pArgInf[2], &ary);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = ary.nCount;
}

void WsCmdSplit(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    UniWView sep; sep.FromUtf8(UniArgText(pArgInf[1]));
    BOOL bRemoveEmpty = UniIsNull(pArgInf[3]) ? TRUE : UniArgBool(pArgInf[3], TRUE);
    UniWRangeList list;
    if (p != NULL) WsSplit(p, sep.p, sep.nChars, bRemoveEmpty, &list);
    UniAryList ary;
    for (INT i = 0; i < list.nCount; i++)
    {
        CxWStr* seg = WsSub(p, list.pStart[i], list.pChars[i]);
        char* sz = (seg != NULL && seg->pStr != NULL) ? UniWideToUtf8(seg->pStr, seg->nLen) : NULL;
        ary.Add((sz != NULL) ? CloneTextData(sz) : CloneTextData("", 0));
        if (sz != NULL) LocalFree(sz);
        WsFree(seg);
    }
    UniAryWrite(pArgInf[2], &ary);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = ary.nCount;
}

void WsCmdSplitSelf(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxWStr* p = WsArgSelf(pArgInf[0]);
    CxWStr* sep = WsOther(pArgInf[1]);
    BOOL bRemoveEmpty = UniIsNull(pArgInf[3]) ? TRUE : UniArgBool(pArgInf[3], TRUE);
    UniWRangeList list;
    if (p != NULL) WsSplit(p, (sep != NULL) ? sep->pStr : L"", (sep != NULL) ? sep->nLen : 0, bRemoveEmpty, &list);
    // 元素是宽文本对象：每个元素是一块「存 CxWStr 指针」的对象数据
    INT n = list.nCount;
    UniAryList ary;
    for (INT i = 0; i < n; i++)
    {
        INT* pBlock = (INT*)NotifySys(NRS_MALLOC, sizeof(INT), 0);
        if (pBlock == NULL) continue;
        *pBlock = (INT)(INT_PTR)WsSub(p, list.pStart[i], list.pChars[i]);
        ary.Add(pBlock);
    }
    UniAryWrite(pArgInf[2], &ary);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = ary.nCount;
}

}  // namespace cx
