// 正则表达式四个类的原生实现：用 IDispatch 驱动 VBScript.RegExp，与模块原版同一 COM 引擎。
// CoCreateInstance / SysAllocString / VariantClear / CoInitialize 全部运行期解析，
// 静态编译时不引入 ole32/oleaut32 导入库；BSTR 长度自己从前置 4 字节读，避免 SysStringLen 导入。

#include <cx_uni.h>
#include <ole2.h>
#include <oleauto.h>

namespace cx {

extern PFN_NOTIFY_SYS g_pfnNotifySys;
INT NotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2);
char* CloneTextData(LPCSTR pszText, INT nLen);
LPBYTE CloneBinData(LPCSTR pData, INT nDataSize);

static const GUID g_clsidRegExp = { 0x3F4DACA4, 0x160D, 0x11D2, { 0xA8, 0xE9, 0x00, 0x10, 0x4B, 0x36, 0x5C, 0x9F } };static const GUID g_iidDispatch = { 0x00020400, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const WCHAR g_rxEmpty[1] = { 0 };

// ============================================================ IDispatch 小封装

static HRESULT RxInvoke(IDispatch* p, LPCWSTR szName, WORD wFlags, VARIANT* pArgs, int nArgs, VARIANT* pOut)
{
    if (p == NULL || szName == NULL) return E_INVALIDARG;
    DISPID id = DISPID_UNKNOWN;
    LPOLESTR pName = (LPOLESTR)szName;
    HRESULT hr = p->GetIDsOfNames(g_iidDispatch, &pName, 1, LOCALE_USER_DEFAULT, &id);
    if (FAILED(hr)) return hr;
    DISPPARAMS dp;
    memset(&dp, 0, sizeof(dp));
    DISPID dpPut = DISPID_PROPERTYPUT;
    dp.rgvarg = pArgs;
    dp.cArgs = nArgs;
    if (wFlags == DISPATCH_PROPERTYPUT)
    {
        dp.rgdispidNamedArgs = &dpPut;
        dp.cNamedArgs = 1;
    }
    EXCEPINFO ei;
    memset(&ei, 0, sizeof(ei));
    UINT nErr = 0;
    return p->Invoke(id, g_iidDispatch, LOCALE_USER_DEFAULT, wFlags, &dp, pOut, &ei, &nErr);
}

static void RxVarInit(VARIANT* v)
{
    memset(v, 0, sizeof(*v));                      // VARIANT 成员名随 SDK 版本变化，统一清零
    v->vt = VT_EMPTY;
}

static void RxVarClear(VARIANT* v)
{
    const UniApi& a = UniGetApi();
    if (v == NULL) return;
    if (v->vt != VT_EMPTY && a.VariantClear != NULL) a.VariantClear(v);
    memset(v, 0, sizeof(*v));
    v->vt = VT_EMPTY;
}

static VARIANT RxVarBstr(const WCHAR* pW)
{
    VARIANT v;
    RxVarInit(&v);
    const UniApi& a = UniGetApi();
    if (pW != NULL && a.SysAllocString != NULL)
    {
        v.bstrVal = reinterpret_cast<BSTR>(a.SysAllocString(pW));
        v.vt = (v.bstrVal != NULL) ? VT_BSTR : VT_NULL;
    }
    return v;
}

static VARIANT RxVarBool(BOOL b)
{
    VARIANT v;
    RxVarInit(&v);
    v.vt = VT_BOOL;
    v.boolVal = b ? VARIANT_TRUE : VARIANT_FALSE;
    return v;
}

static VARIANT RxVarInt(INT n)
{
    VARIANT v;
    RxVarInit(&v);
    v.vt = VT_I4;
    v.lVal = n;
    return v;
}

static INT RxVarToInt(const VARIANT& v, INT nDefault)
{
    if (v.vt == VT_I4) return v.lVal;
    if (v.vt == VT_BOOL) return v.boolVal ? 1 : 0;
    return nDefault;
}

static IDispatch* RxVarToDisp(const VARIANT& v)
{
    return (v.vt == VT_DISPATCH) ? v.pdispVal : NULL;
}

static INT RxBstrChars(BSTR bs)
{
    if (bs == NULL) return 0;
    INT nBytes = ((INT*)bs)[-1];                  // x86 下 BSTR 前置 4 字节是字节长度
    if (nBytes < 0) return 0;
    return nBytes / 2;
}

static WCHAR* RxVarDupWide(const VARIANT& v, INT* pnChars)
{
    if (pnChars) *pnChars = 0;
    if (v.vt != VT_BSTR || v.bstrVal == NULL) return NULL;
    INT n = RxBstrChars(v.bstrVal);
    WCHAR* p = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)(n + 1) * sizeof(WCHAR));
    if (p == NULL) return NULL;
    if (n > 0) memcpy(p, v.bstrVal, (SIZE_T)n * sizeof(WCHAR));
    p[n] = 0;
    if (pnChars) *pnChars = n;
    return p;
}

static bool RxPutFlag(IDispatch* pReg, LPCWSTR szName, BOOL bVal)
{
    VARIANT arg = RxVarBool(bVal);
    VARIANT out;
    RxVarInit(&out);
    bool b = SUCCEEDED(RxInvoke(pReg, szName, DISPATCH_PROPERTYPUT, &arg, 1, &out));
    RxVarClear(&arg);
    RxVarClear(&out);
    return b;
}

static bool RxPutPattern(IDispatch* pReg, const WCHAR* pPattern)
{
    VARIANT arg = RxVarBstr((pPattern != NULL) ? pPattern : g_rxEmpty);
    VARIANT out;
    RxVarInit(&out);
    bool b = SUCCEEDED(RxInvoke(pReg, L"Pattern", DISPATCH_PROPERTYPUT, &arg, 1, &out));
    RxVarClear(&arg);
    RxVarClear(&out);
    return b;
}

// ============================================================ CxRegex 生命周期

void RxClear(CxRegex* p)
{
    if (p == NULL) return;
    if (p->pMatches != NULL) { ((IDispatch*)p->pMatches)->Release(); p->pMatches = NULL; }
    if (p->pReg != NULL) { ((IDispatch*)p->pReg)->Release(); p->pReg = NULL; }
    if (p->pSubject != NULL) { LocalFree(p->pSubject); p->pSubject = NULL; }
    p->nSubjectChars = 0;
}

void RxFree(CxRegex* p)
{
    if (p == NULL) return;
    RxClear(p);
    if (p->pTmpVal != NULL) { LocalFree(p->pTmpVal); p->pTmpVal = NULL; }
    p->nTmpValChars = 0;
    const UniApi& a = UniGetApi();
    if (p->bComInited && a.CoUninitialize != NULL)
    {
        a.CoUninitialize();
        p->bComInited = FALSE;                     // 只在成功初始化过的线程上成对取消
    }
}

bool RxExecute(CxRegex* p, const WCHAR* pSubject, INT nChars)
{
    if (p == NULL || p->pReg == NULL) return false;
    if (nChars < 0) nChars = (pSubject != NULL) ? lstrlenW(pSubject) : 0;
    if (nChars < 0) nChars = 0;
    if (p->pSubject != NULL) { LocalFree(p->pSubject); p->pSubject = NULL; }
    p->pSubject = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)(nChars + 1) * sizeof(WCHAR));
    if (p->pSubject == NULL) return false;
    if (pSubject != NULL && nChars > 0) memcpy(p->pSubject, pSubject, (SIZE_T)nChars * sizeof(WCHAR));
    p->pSubject[nChars] = 0;
    p->nSubjectChars = nChars;

    if (p->pMatches != NULL) { ((IDispatch*)p->pMatches)->Release(); p->pMatches = NULL; }
    VARIANT arg = RxVarBstr(p->pSubject);
    VARIANT out;
    RxVarInit(&out);
    HRESULT hr = RxInvoke((IDispatch*)p->pReg, L"Execute", DISPATCH_METHOD, &arg, 1, &out);
    RxVarClear(&arg);
    if (SUCCEEDED(hr))
    {
        IDispatch* pDisp = RxVarToDisp(out);
        if (pDisp != NULL)
        {
            pDisp->AddRef();
            p->pMatches = (void*)pDisp;
        }
    }
    RxVarClear(&out);
    return p->pMatches != NULL;
}

bool RxNew(CxRegex* p, const WCHAR* pPattern, const WCHAR* pSubject, INT nSubjectChars,
           BOOL bIgnoreCase, BOOL bMultiline, BOOL bSingleline, BOOL bGlobal)
{
    const UniApi& a = UniGetApi();
    if (p == NULL || a.CoCreateInstance == NULL) return false;
    RxClear(p);
    if (!p->bComInited && a.CoInitialize != NULL && SUCCEEDED(a.CoInitialize(NULL))) p->bComInited = TRUE;
    IUnknown* pUnk = NULL;
    const DWORD CLSCTX_INPROC_SERVER = 1;
    if (FAILED(a.CoCreateInstance(&g_clsidRegExp, NULL, CLSCTX_INPROC_SERVER, &g_iidDispatch, (void**)&pUnk)) || pUnk == NULL)
        return false;
    p->pReg = (void*)pUnk;
    IDispatch* pReg = (IDispatch*)pUnk;
    p->bIgnoreCase = bIgnoreCase;
    RxPutFlag(pReg, L"IgnoreCase", bIgnoreCase);
    RxPutFlag(pReg, L"Multiline", bMultiline);
    if (bSingleline) RxPutFlag(pReg, L"Singleline", TRUE);
    RxPutFlag(pReg, L"Global", bGlobal);
    if (!RxPutPattern(pReg, pPattern)) return false;
    return RxExecute(p, pSubject, nSubjectChars);
}

void RxCopyObject(CxRegex* pDst, const CxRegex* pSrc)
{
    if (pDst == NULL || pSrc == NULL) return;
    memset(pDst, 0, sizeof(CxRegex));
    if (pSrc->pReg == NULL) return;
    WCHAR* pPat = NULL;
    INT nPat = 0;
    RxGetPattern(const_cast<CxRegex*>(pSrc), &pPat, &nPat);
    pDst->bIgnoreCase = pSrc->bIgnoreCase;
    if (pPat != NULL)
    {
        RxNew(pDst, pPat, pSrc->pSubject, pSrc->nSubjectChars, pSrc->bIgnoreCase, TRUE, TRUE, TRUE);
        LocalFree(pPat);
    }
}

// ============================================================ 匹配结果读取

INT RxMatchCount(CxRegex* p)
{
    if (p == NULL || p->pMatches == NULL) return 0;
    VARIANT out;
    RxVarInit(&out);
    INT n = 0;
    if (SUCCEEDED(RxInvoke((IDispatch*)p->pMatches, L"Count", DISPATCH_PROPERTYGET, NULL, 0, &out)))
        n = RxVarToInt(out, 0);
    RxVarClear(&out);
    return n;
}

static IDispatch* RxMatchAt(CxRegex* p, INT nMatchIndex)
{
    if (p == NULL || p->pMatches == NULL || nMatchIndex < 1) return NULL;
    VARIANT arg = RxVarInt(nMatchIndex - 1);
    VARIANT out;
    RxVarInit(&out);
    IDispatch* pRet = NULL;
    if (SUCCEEDED(RxInvoke((IDispatch*)p->pMatches, L"Item", DISPATCH_PROPERTYGET, &arg, 1, &out)))
    {
        IDispatch* pDisp = RxVarToDisp(out);
        if (pDisp != NULL) { pDisp->AddRef(); pRet = pDisp; }
    }
    RxVarClear(&arg);
    RxVarClear(&out);
    return pRet;
}

static IDispatch* RxSubSetAt(CxRegex* p, INT nMatchIndex)
{
    IDispatch* pMatch = RxMatchAt(p, nMatchIndex);
    if (pMatch == NULL) return NULL;
    VARIANT out;
    RxVarInit(&out);
    IDispatch* pRet = NULL;
    if (SUCCEEDED(RxInvoke(pMatch, L"SubMatches", DISPATCH_PROPERTYGET, NULL, 0, &out)))
    {
        IDispatch* pDisp = RxVarToDisp(out);
        if (pDisp != NULL) { pDisp->AddRef(); pRet = pDisp; }
    }
    RxVarClear(&out);
    pMatch->Release();
    return pRet;
}

// 取文本属性到正则对象的临时缓冲（下次取值前有效）。
static const WCHAR* RxGetTextProp(CxRegex* p, IDispatch* pObj, LPCWSTR szProp, VARIANT* pArg, INT nArgs, INT* pnChars)
{
    if (pnChars) *pnChars = 0;
    if (pObj == NULL) return NULL;
    if (p->pTmpVal != NULL) { LocalFree(p->pTmpVal); p->pTmpVal = NULL; }
    VARIANT out;
    RxVarInit(&out);
    WCHAR* q = NULL;
    INT n = 0;
    if (SUCCEEDED(RxInvoke(pObj, szProp, DISPATCH_PROPERTYGET, pArg, nArgs, &out))) q = RxVarDupWide(out, &n);
    RxVarClear(&out);
    p->pTmpVal = q;
    p->nTmpValChars = n;
    if (pnChars) *pnChars = n;
    return q;
}

const WCHAR* RxMatchValue(CxRegex* p, INT nMatchIndex)
{
    if (p == NULL) return NULL;
    IDispatch* pMatch = RxMatchAt(p, nMatchIndex);
    const WCHAR* r = RxGetTextProp(p, pMatch, L"Value", NULL, 0, NULL);
    if (pMatch != NULL) pMatch->Release();
    return r;
}

const WCHAR* RxSubValue(CxRegex* p, INT nMatchIndex, INT nSubIndex)
{
    if (p == NULL) return NULL;
    IDispatch* pSub = RxSubSetAt(p, nMatchIndex);
    const WCHAR* r = NULL;
    if (pSub != NULL)
    {
        VARIANT arg = RxVarInt(nSubIndex - 1);
        r = RxGetTextProp(p, pSub, L"Item", &arg, 1, NULL);
        RxVarClear(&arg);
        pSub->Release();
    }
    return r;
}

INT RxMatchFirstIndex(CxRegex* p, INT nMatchIndex)
{
    IDispatch* pMatch = RxMatchAt(p, nMatchIndex);
    if (pMatch == NULL) return -1;
    VARIANT out;
    RxVarInit(&out);
    INT n = -1;
    if (SUCCEEDED(RxInvoke(pMatch, L"FirstIndex", DISPATCH_PROPERTYGET, NULL, 0, &out))) n = RxVarToInt(out, -1);
    RxVarClear(&out);
    pMatch->Release();
    return n;
}

static INT RxCountOf(IDispatch* pDisp)
{
    if (pDisp == NULL) return 0;
    VARIANT out;
    RxVarInit(&out);
    INT n = 0;
    if (SUCCEEDED(RxInvoke(pDisp, L"Count", DISPATCH_PROPERTYGET, NULL, 0, &out))) n = RxVarToInt(out, 0);
    RxVarClear(&out);
    return n;
}

INT RxSubCount(CxRegex* p)
{
    IDispatch* pSub = RxSubSetAt(p, 1);
    INT n = RxCountOf(pSub);
    if (pSub != NULL) pSub->Release();
    return n;
}

INT RxSubCountAt(CxRegex* p, INT nMatchIndex)
{
    IDispatch* pSub = RxSubSetAt(p, nMatchIndex);
    INT n = RxCountOf(pSub);
    if (pSub != NULL) pSub->Release();
    return n;
}

void RxGetPattern(CxRegex* p, WCHAR** ppOut, INT* pnChars)
{
    if (ppOut) *ppOut = NULL;
    if (pnChars) *pnChars = 0;
    if (p == NULL || p->pReg == NULL) return;
    VARIANT out;
    RxVarInit(&out);
    WCHAR* q = NULL;
    INT n = 0;
    if (SUCCEEDED(RxInvoke((IDispatch*)p->pReg, L"Pattern", DISPATCH_PROPERTYGET, NULL, 0, &out))) q = RxVarDupWide(out, &n);
    RxVarClear(&out);
    if (ppOut) *ppOut = q;
    if (pnChars) *pnChars = n;
}

WCHAR* RxReplace(CxRegex* p, const WCHAR* pRepl, INT* pnChars)
{
    if (pnChars) *pnChars = 0;
    if (p == NULL || p->pReg == NULL) return NULL;
    // DISPPARAMS 的 rgvarg 为逆序：Replace(被搜索文本, 替换文本) → [1]=被搜索文本 [0]=替换文本
    VARIANT args[2];
    args[1] = RxVarBstr(p->pSubject);
    args[0] = RxVarBstr((pRepl != NULL) ? pRepl : g_rxEmpty);
    VARIANT out;
    RxVarInit(&out);
    WCHAR* q = NULL;
    INT n = 0;
    if (SUCCEEDED(RxInvoke((IDispatch*)p->pReg, L"Replace", DISPATCH_METHOD, args, 2, &out))) q = RxVarDupWide(out, &n);
    RxVarClear(&args[0]);
    RxVarClear(&args[1]);
    RxVarClear(&out);
    if (pnChars) *pnChars = n;
    return q;
}

bool RxTest(CxRegex* p, const WCHAR* pText)
{
    if (p == NULL || p->pReg == NULL) return false;
    VARIANT arg = RxVarBstr((pText != NULL) ? pText : g_rxEmpty);
    VARIANT out;
    RxVarInit(&out);
    bool b = false;
    if (SUCCEEDED(RxInvoke((IDispatch*)p->pReg, L"Test", DISPATCH_METHOD, &arg, 1, &out))) b = (RxVarToInt(out, 0) != 0);
    RxVarClear(&arg);
    RxVarClear(&out);
    return b;
}

// 原版 起始位置 写回值：寻找文本W(被搜索文本, 匹配文本, FirstIndex, 与 IgnoreCase 取反) - 1
INT RxStartPosAt(CxRegex* p, INT nMatchIndex, const WCHAR* pVal)
{
    if (p == NULL || p->pSubject == NULL || pVal == NULL) return -1;
    INT nFirst = RxMatchFirstIndex(p, nMatchIndex);
    if (nFirst <= 0) nFirst = 1;                   // 原版把 0 基 FirstIndex 当 1 基位置传入，0 会被当作 1
    return UniFindWide(p->pSubject, p->nSubjectChars, pVal, nFirst, p->bIgnoreCase ? FALSE : TRUE) - 1;
}

// ============================================================ 命令层公共件

static CxRegex* RxArgSelf(const MDATA_INF& arg)
{
    if (arg.m_pCompoundData == NULL) return NULL;
    return (CxRegex*)(INT_PTR)*(PINT)arg.m_pCompoundData;
}

static CxRegex* RxNewSelf(const MDATA_INF& arg)
{
    if (arg.m_pCompoundData == NULL) return NULL;
    CxRegex* p = RxArgSelf(arg);
    if (p == NULL)
    {
        p = (CxRegex*)LocalAlloc(LPTR, sizeof(CxRegex));
        *(PINT)arg.m_pCompoundData = (INT)(INT_PTR)p;
    }
    return p;
}

static void RxRetWideBin(PMDATA_INF pRet, const WCHAR* pW, INT nChars)
{
    UniRetWideBin(pRet, (pW != NULL) ? pW : g_rxEmpty, nChars);
}

static void RxRetUtf8Text(PMDATA_INF pRet, const WCHAR* pW, INT nChars)
{
    UniRetWideText(pRet, (pW != NULL) ? pW : g_rxEmpty, nChars);
}

static void RxWriteStartPos(const MDATA_INF& arg, INT nPos)
{
    if (UniIsNull(arg) || arg.m_pInt == NULL) return;
    *arg.m_pInt = nPos;
}

// 自动替换引号：#引号 → "
static const WCHAR* RxApplyQuote(const WCHAR* pPattern, UniWBuf* pBuf)
{
    UniWView token; token.FromUtf8("#\xe5\xbc\x95\xe5\x8f\xb7");
    UniWView q; q.FromUtf8("\"");
    if (pPattern != NULL && UniSubReplaceWide(pPattern, lstrlenW(pPattern), token.p, q.p, 1, 0, TRUE, pBuf)
        && pBuf->p != NULL) return pBuf->p;
    return pPattern;
}

// ============================================================ 生命周期（四个类同构）

#define RX_LIFECYCLE(fnConstruct, fnCopy, fnDtor)                                       \
void fnConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)                \
{                                                                                       \
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);                \
    if (pArgInf[0].m_pCompoundData != NULL) *(PINT)pArgInf[0].m_pCompoundData = 0;      \
}                                                                                       \
void fnCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)                     \
{                                                                                       \
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);                \
    CxRegex* pDst = (CxRegex*)LocalAlloc(LPTR, sizeof(CxRegex));                        \
    if (pArgInf[0].m_pCompoundData != NULL) *(PINT)pArgInf[0].m_pCompoundData = (INT)(INT_PTR)pDst; \
    RxCopyObject(pDst, RxArgSelf(pArgInf[1]));                                          \
}                                                                                       \
void fnDtor(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)                     \
{                                                                                       \
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);                \
    CxRegex* p = RxArgSelf(pArgInf[0]);                                                 \
    if (p != NULL) { RxFree(p); LocalFree(p); *(PINT)pArgInf[0].m_pCompoundData = 0; }  \
}

RX_LIFECYCLE(RxConstruct, RxCopy, RxDestruct)
RX_LIFECYCLE(RxcConstruct, RxcCopy, RxcDestruct)
RX_LIFECYCLE(Rxc2Construct, Rxc2Copy, Rxc2Destruct)
RX_LIFECYCLE(RxsConstruct, RxsCopy, RxsDestruct)

// ============================================================ 类_正则表达式类W（字节集入出）

void RxCmdCreate(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniRetBool(pRetData, FALSE);
    CxRegex* p = RxNewSelf(pArgInf[0]);
    if (p == NULL) return;
    UniWView pat; pat.FromBin(pArgInf[1]);
    UniWView src; src.FromBin(pArgInf[2]);
    BOOL bCase = UniArgBool(pArgInf[3], FALSE);
    BOOL bMulti = UniIsNull(pArgInf[4]) ? TRUE : UniArgBool(pArgInf[4], TRUE);
    BOOL bGlobal = UniIsNull(pArgInf[5]) ? TRUE : UniArgBool(pArgInf[5], TRUE);
    UniWBuf buf;
    const WCHAR* pPat = pat.p;
    if (UniArgBool(pArgInf[6], FALSE)) pPat = RxApplyQuote(pat.p, &buf);
    if (RxNew(p, pPat, src.p, src.nChars, bCase ? FALSE : TRUE, bMulti, TRUE, bGlobal))
        UniRetBool(pRetData, TRUE);
}

void RxCmdView(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
}

void RxCmdPattern(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxRegex* p = RxArgSelf(pArgInf[0]);
    WCHAR* q = NULL;
    INT n = 0;
    if (p != NULL) RxGetPattern(p, &q, &n);
    RxRetWideBin(pRetData, q, n);
    if (q != NULL) LocalFree(q);
}

void RxCmdReplace(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxRegex* p = RxArgSelf(pArgInf[0]);
    UniWView rep; rep.FromBin(pArgInf[1]);
    INT n = 0;
    WCHAR* q = (p != NULL) ? RxReplace(p, rep.p, &n) : NULL;
    RxRetWideBin(pRetData, q, n);
    if (q != NULL) LocalFree(q);
}

void RxCmdCount(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = RxMatchCount(RxArgSelf(pArgInf[0]));
}

void RxCmdValueW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxRegex* p = RxArgSelf(pArgInf[0]);
    INT nIdx = UniArgInt(pArgInf[1], 1);
    const WCHAR* val = (p != NULL) ? RxMatchValue(p, nIdx) : NULL;
    INT nChars = (val != NULL) ? lstrlenW(val) : 0;
    if (val != NULL && !UniIsNull(pArgInf[2])) RxWriteStartPos(pArgInf[2], RxStartPosAt(p, nIdx, val));
    RxRetWideBin(pRetData, val, nChars);
}

void RxCmdSubValueW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxRegex* p = RxArgSelf(pArgInf[0]);
    INT nIdx = UniArgInt(pArgInf[1], 1);
    const WCHAR* val = (p != NULL) ? RxSubValue(p, nIdx, UniArgInt(pArgInf[2], 1)) : NULL;
    INT nChars = (val != NULL) ? lstrlenW(val) : 0;
    if (val != NULL && !UniIsNull(pArgInf[3])) RxWriteStartPos(pArgInf[3], RxStartPosAt(p, nIdx, val));
    RxRetWideBin(pRetData, val, nChars);
}

void RxCmdSubCount(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = RxSubCount(RxArgSelf(pArgInf[0]));
}

// ============================================================ 兼容版W

void RxcCreate(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UniRetBool(pRetData, FALSE);
    CxRegex* p = RxNewSelf(pArgInf[0]);
    if (p == NULL) return;
    UniWView pat; pat.FromUtf8(UniArgText(pArgInf[1]));
    UniWView src; src.FromUtf8(UniArgText(pArgInf[2]));
    BOOL bCase = UniArgBool(pArgInf[3], FALSE);
    BOOL bMulti = UniIsNull(pArgInf[4]) ? TRUE : UniArgBool(pArgInf[4], TRUE);
    BOOL bGlobal = UniIsNull(pArgInf[5]) ? TRUE : UniArgBool(pArgInf[5], TRUE);
    UniWBuf buf;
    const WCHAR* pPat = pat.p;
    if (UniArgBool(pArgInf[6], FALSE)) pPat = RxApplyQuote(pat.p, &buf);
    if (RxNew(p, pPat, src.p, src.nChars, bCase ? FALSE : TRUE, bMulti, TRUE, bGlobal))
        UniRetBool(pRetData, TRUE);
}

void RxcCreateW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);             // 本命令只有 5 个参数，不能按 6 参数版取 pArgInf[6]
    UniRetBool(pRetData, FALSE);
    CxRegex* p = RxNewSelf(pArgInf[0]);
    if (p == NULL) return;
    UniWView pat; pat.FromBin(pArgInf[1]);
    UniWView src; src.FromBin(pArgInf[2]);
    BOOL bCase = UniArgBool(pArgInf[3], FALSE);
    BOOL bMulti = UniIsNull(pArgInf[4]) ? TRUE : UniArgBool(pArgInf[4], TRUE);
    BOOL bGlobal = UniIsNull(pArgInf[5]) ? TRUE : UniArgBool(pArgInf[5], TRUE);
    if (RxNew(p, pat.p, src.p, src.nChars, bCase ? FALSE : TRUE, bMulti, TRUE, bGlobal))
        UniRetBool(pRetData, TRUE);
}

void RxcView(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
}

void RxcPattern(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxRegex* p = RxArgSelf(pArgInf[0]);
    WCHAR* q = NULL;
    INT n = 0;
    if (p != NULL) RxGetPattern(p, &q, &n);
    RxRetUtf8Text(pRetData, q, n);
    if (q != NULL) LocalFree(q);
}

void RxcReplace(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxRegex* p = RxArgSelf(pArgInf[0]);
    UniWView rep; rep.FromUtf8(UniArgText(pArgInf[1]));
    INT n = 0;
    WCHAR* q = (p != NULL) ? RxReplace(p, rep.p, &n) : NULL;
    RxRetUtf8Text(pRetData, q, n);
    if (q != NULL) LocalFree(q);
}

void RxcCount(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = RxMatchCount(RxArgSelf(pArgInf[0]));
}

void RxcValue(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxRegex* p = RxArgSelf(pArgInf[0]);
    INT nIdx = UniArgInt(pArgInf[1], 1);
    const WCHAR* val = (p != NULL) ? RxMatchValue(p, nIdx) : NULL;
    INT nChars = (val != NULL) ? lstrlenW(val) : 0;
    if (val != NULL && !UniIsNull(pArgInf[2])) RxWriteStartPos(pArgInf[2], RxStartPosAt(p, nIdx, val));
    RxRetUtf8Text(pRetData, val, nChars);
}

void RxcValueW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RxCmdValueW(pRetData, nArgCount, pArgInf);
}

void RxcSubValue(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxRegex* p = RxArgSelf(pArgInf[0]);
    INT nIdx = UniArgInt(pArgInf[1], 1);
    const WCHAR* val = (p != NULL) ? RxSubValue(p, nIdx, UniArgInt(pArgInf[2], 1)) : NULL;
    INT nChars = (val != NULL) ? lstrlenW(val) : 0;
    if (val != NULL && !UniIsNull(pArgInf[3])) RxWriteStartPos(pArgInf[3], RxStartPosAt(p, nIdx, val));
    RxRetUtf8Text(pRetData, val, nChars);
}

void RxcSubValueW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RxCmdSubValueW(pRetData, nArgCount, pArgInf);
}

void RxcSubCount(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = RxSubCount(RxArgSelf(pArgInf[0]));
}

// ============================================================ 兼容版2
// 差别：不设 Singleline，Global 恒为真，Multiline 取参数反义，创建总是返回真。

static void Rxc2Apply(PMDATA_INF pRetData, PMDATA_INF pArgInf, INT bWide)
{
    CxRegex* p = RxNewSelf(pArgInf[0]);
    UniWView pat, src;
    if (bWide) { pat.FromBin(pArgInf[1]); src.FromBin(pArgInf[2]); }
    else { pat.FromUtf8(UniArgText(pArgInf[1])); src.FromUtf8(UniArgText(pArgInf[2])); }
    BOOL bCase = UniArgBool(pArgInf[3], FALSE);
    BOOL bMulti = UniArgBool(pArgInf[4], FALSE);
    if (p != NULL) RxNew(p, pat.p, src.p, src.nChars, bCase ? FALSE : TRUE, bMulti ? FALSE : TRUE, FALSE, TRUE);
    UniRetBool(pRetData, TRUE);                     // 原版无论是否命中都返回真
}

void Rxc2Create(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    Rxc2Apply(pRetData, pArgInf, 0);
}

void Rxc2CreateW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    Rxc2Apply(pRetData, pArgInf, 1);
}

void Rxc2Pattern(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RxcPattern(pRetData, nArgCount, pArgInf);
}

void Rxc2PatternW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RxCmdPattern(pRetData, nArgCount, pArgInf);
}

void Rxc2Replace(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RxcReplace(pRetData, nArgCount, pArgInf);
}

void Rxc2ReplaceW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RxCmdReplace(pRetData, nArgCount, pArgInf);
}

void Rxc2Count(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RxcCount(pRetData, nArgCount, pArgInf);
}

void Rxc2Value(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RxcValue(pRetData, nArgCount, pArgInf);
}

void Rxc2SubValue(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RxcSubValue(pRetData, nArgCount, pArgInf);
}

void Rxc2SubCount(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RxcSubCount(pRetData, nArgCount, pArgInf);
}

// ============================================================ 简单版W
// 创建只设 pattern/ignorecase/global；匹配由 匹配()/搜索全部() 现场执行。

static void RxsCreateCommon(PMDATA_INF pRetData, PMDATA_INF pArgInf, INT bWide)
{
    UniRetBool(pRetData, FALSE);
    CxRegex* p = RxNewSelf(pArgInf[0]);
    if (p == NULL) return;
    UniWView pat;
    if (bWide) pat.FromBin(pArgInf[1]);
    else pat.FromUtf8(UniArgText(pArgInf[1]));
    BOOL bCase = UniArgBool(pArgInf[2], FALSE);
    BOOL bGlobal = UniArgBool(pArgInf[3], FALSE);
    if (RxNew(p, pat.p, g_rxEmpty, 0, bCase ? FALSE : TRUE, FALSE, FALSE, bGlobal))
        UniRetBool(pRetData, TRUE);
}

void RxsCreate(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    RxsCreateCommon(pRetData, pArgInf, 0);
}

void RxsCreateW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    RxsCreateCommon(pRetData, pArgInf, 1);
}

void RxsMatch(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxRegex* p = RxArgSelf(pArgInf[0]);
    UniWView t; t.FromUtf8(UniArgText(pArgInf[1]));
    UniRetBool(pRetData, (p != NULL && RxTest(p, t.p)) ? TRUE : FALSE);
}

static void RxsSearch(PMDATA_INF pRetData, PMDATA_INF pArgInf, INT bWide)
{
    CxRegex* p = RxArgSelf(pArgInf[0]);
    UniWView t;
    if (bWide) t.FromBin(pArgInf[1]);
    else t.FromUtf8(UniArgText(pArgInf[1]));
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (p != NULL && RxExecute(p, t.p, t.nChars)) ? RxMatchCount(p) : 0;
}

void RxsSearchAll(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    RxsSearch(pRetData, pArgInf, 0);
}

void RxsSearchAllW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    RxsSearch(pRetData, pArgInf, 1);
}

static void RxsValueCommon(PMDATA_INF pRetData, PMDATA_INF pArgInf, INT bSub, INT bWide)
{
    CxRegex* p = RxArgSelf(pArgInf[0]);
    INT nIdx = UniArgInt(pArgInf[1], 1);
    const WCHAR* val = NULL;
    if (p != NULL)
        val = bSub ? RxSubValue(p, nIdx, UniArgInt(pArgInf[2], 1)) : RxMatchValue(p, nIdx);
    INT nChars = (val != NULL) ? lstrlenW(val) : 0;
    if (bWide) RxRetWideBin(pRetData, val, nChars);
    else RxRetUtf8Text(pRetData, val, nChars);
}

void RxsValue(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    RxsValueCommon(pRetData, pArgInf, 0, 0);
}

void RxsValueW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    RxsValueCommon(pRetData, pArgInf, 0, 1);
}

void RxsSubValue(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    RxsValueCommon(pRetData, pArgInf, 1, 0);
}

void RxsSubValueW(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    RxsValueCommon(pRetData, pArgInf, 1, 1);
}

void RxsSubCount(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxRegex* p = RxArgSelf(pArgInf[0]);
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = (p != NULL) ? RxSubCountAt(p, UniArgInt(pArgInf[1], 1)) : 0;
}

}  // namespace cx
