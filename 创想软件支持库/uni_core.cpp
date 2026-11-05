// 宽文本层公共设施：运行期系统函数解析、参数/返回编组、CxWStr 缓冲区。
// 与 lib.cpp 同一 namespace，命令实现分散在 uni_text.cpp / uni_wstr.cpp / uni_regex.cpp。

#include <cx_uni.h>

namespace cx {

// lib.cpp 里的易语言系统接口与内存池封装
extern PFN_NOTIFY_SYS g_pfnNotifySys;
INT NotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2);
char* CloneTextData(LPCSTR pszText, INT nLen);
char* CloneTextData(LPCSTR pszText);
LPBYTE CloneBinData(LPCSTR pData, INT nDataSize);
void ReportError(LPCSTR szErrText);

// ============================================================ 内存池

// 非 static：HTTP访问 族（http访问\cxhttp_core.inc）也要用易语言内存池分配数组元素
void* UniMalloc(INT nSize)
{
    return (void*)(INT_PTR)NotifySys(NRS_MALLOC, (DWORD)nSize, 0);
}

static void UniMFree(void* p)
{
    if (p != NULL) NotifySys(NRS_MFREE, (DWORD)(INT_PTR)p, 0);
}

// VC6 CRT 没有 wcsstr/wcstod/wcstol 之类宽字符函数，静态编译会找不到符号，这里自带。
static const WCHAR* UniWcsStr(const WCHAR* pSrc, const WCHAR* pFind)
{
    if (pSrc == NULL || pFind == NULL) return NULL;
    if (pFind[0] == 0) return pSrc;
    INT nNeed = lstrlenW(pFind);
    for (const WCHAR* p = pSrc; *p != 0; p++)
    {
        INT i = 0;
        while (i < nNeed && p[i] == pFind[i]) i++;
        if (i >= nNeed) return p;
    }
    return NULL;
}

// ============================================================ 运行期系统函数

static UniApi g_api;
static bool g_apiCsReady = false;
static CRITICAL_SECTION g_apiCs;

static FARPROC UniProc(HMODULE h, LPCSTR szName)
{
    return h == NULL ? NULL : GetProcAddress(h, szName);
}

HMODULE UniSelfModule()
{
    // 静态编译时没有 DllMain，不能用 GetModuleHandleEx（VC6 导入库没有该入口），用页信息取自身基址。
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((LPCSTR)&g_api, &mbi, sizeof(mbi)) != 0 && mbi.AllocationBase != NULL)
        return (HMODULE)mbi.AllocationBase;
    return GetModuleHandleA(NULL);
}

const UniApi& UniGetApi()
{
    if (g_api.bReady) return g_api;
    if (!g_apiCsReady) { InitializeCriticalSection(&g_apiCs); g_apiCsReady = true; }
    EnterCriticalSection(&g_apiCs);
    if (!g_api.bReady)
    {
        memset(&g_api, 0, sizeof(g_api));
        HMODULE hShell = LoadLibraryA("shlwapi.dll");
        g_api.StrStrW     = (const WCHAR* (WINAPI*)(const WCHAR*, const WCHAR*))UniProc(hShell, "StrStrW");
        g_api.StrStrIW    = (const WCHAR* (WINAPI*)(const WCHAR*, const WCHAR*))UniProc(hShell, "StrStrIW");
        g_api.StrRStrW    = (const WCHAR* (WINAPI*)(const WCHAR*, const WCHAR*, const WCHAR*))UniProc(hShell, "StrRStrW");
        g_api.StrRStrIW   = (const WCHAR* (WINAPI*)(const WCHAR*, const WCHAR*, const WCHAR*))UniProc(hShell, "StrRStrIW");
        g_api.StrTrimW    = (BOOL (WINAPI*)(WCHAR*, const WCHAR*))UniProc(hShell, "StrTrimW");
        g_api.StrToIntExW = (BOOL (WINAPI*)(const WCHAR*, UniStrToIntFlag, INT*))UniProc(hShell, "StrToIntExW");
        g_api.wvsprintfW  = (int (WINAPI*)(WCHAR*, const WCHAR*, const void*))UniProc(hShell, "wvsprintfW");

        HMODULE hK32 = GetModuleHandleA("kernel32.dll");
        if (hK32 == NULL) hK32 = LoadLibraryA("kernel32.dll");
        g_api.LCMapStringEx = (int (WINAPI*)(const WCHAR*, DWORD, const WCHAR*, int, WCHAR*, int, void*, void*, int))
            UniProc(hK32, "LCMapStringEx");

        HMODULE hOle32 = LoadLibraryA("ole32.dll");
        HMODULE hOleAut = LoadLibraryA("oleaut32.dll");
        g_api.CoCreateInstance = (HRESULT (WINAPI*)(const void*, void*, DWORD, const void*, void**))UniProc(hOle32, "CoCreateInstance");
        g_api.CoInitialize     = (HRESULT (WINAPI*)(void*))UniProc(hOle32, "CoInitialize");
        g_api.CoUninitialize   = (void (WINAPI*)(void))UniProc(hOle32, "CoUninitialize");
        g_api.SysAllocString   = (void* (WINAPI*)(const WCHAR*))UniProc(hOleAut, "SysAllocString");
        g_api.SysFreeString    = (void (WINAPI*)(void*))UniProc(hOleAut, "SysFreeString");
        g_api.VariantClear     = (HRESULT (WINAPI*)(void*))UniProc(hOleAut, "VariantClear");
        g_api.VarBstrFromR8    = (HRESULT (WINAPI*)(double, unsigned long, unsigned int, void**))UniProc(hOleAut, "VarBstrFromR8");
        g_api.bReady = true;      // 取不到的函数保持 NULL，调用处各自降级处理，避免反复重载
    }
    LeaveCriticalSection(&g_apiCs);
    return g_api;
}

// ============================================================ 宽文本小工具

static INT WStrLenN(const WCHAR* p, INT nMaxChars)
{
    INT i = 0;
    while (i < nMaxChars && p[i] != 0) i++;
    return i;
}

bool UniIsNull(const MDATA_INF& arg)
{
    return arg.m_dtDataType == _SDT_NULL;
}

INT UniArgInt(const MDATA_INF& arg, INT nDefault)
{
    if (UniIsNull(arg)) return nDefault;
    if (arg.m_dtDataType == SDT_BOOL) return arg.m_bool ? 1 : 0;
    return arg.m_int;
}

BOOL UniArgBool(const MDATA_INF& arg, BOOL bDefault)
{
    if (UniIsNull(arg)) return bDefault;
    if (arg.m_dtDataType == SDT_INT) return arg.m_int ? TRUE : FALSE;
    return arg.m_bool ? TRUE : FALSE;
}

LPCSTR UniArgText(const MDATA_INF& arg)
{
    if (UniIsNull(arg)) return NULL;
    return arg.m_pText;
}

LPBYTE UniArgBinPtr(const MDATA_INF& arg, INT* pnBytes)
{
    if (pnBytes) *pnBytes = 0;
    if (UniIsNull(arg) || arg.m_pBin == NULL) return NULL;
    LPBYTE p = arg.m_pBin;
    INT nDim = *(LPINT)p;
    INT nLen = *(LPINT)(p + sizeof(INT));
    if (nDim <= 0 || nLen < 0) return NULL;
    if (pnBytes) *pnBytes = nLen;
    return p + sizeof(INT) * 2;
}

// ============================================================ UniWView

UniWView::~UniWView()
{
    if (pTmp != NULL) { LocalFree(pTmp); pTmp = NULL; }
}

void UniWView::Set(LPBYTE pData, INT nBytes)
{
    p = (const WCHAR*)L"";
    nChars = 0;
    if (pData == NULL || nBytes <= 0) return;
    nOrigBytes = nBytes;
    INT nMax = nBytes / 2;                       // 奇数字节时丢掉尾字节，宽字符必须成双
    if (nMax <= 0) return;
    if (((const BYTE*)pData)[nBytes - 2] == 0 && ((const BYTE*)pData)[nBytes - 1] == 0)
    {
        p = (const WCHAR*)pData;
        nChars = WStrLenN(p, nMax);
        pRaw = p;
        nRawChars = nMax;                        // 末尾那对 0 已包含在区域内
        return;
    }
    pTmp = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)(nMax + 1) * sizeof(WCHAR));
    if (pTmp == NULL) return;
    memcpy(pTmp, pData, (SIZE_T)nMax * sizeof(WCHAR));
    pTmp[nMax] = 0;
    p = pTmp;
    nChars = WStrLenN(p, nMax);
    pRaw = pTmp;
    nRawChars = nMax + 1;                        // 相当于原字节集 + {0,0}
}

void UniWView::FromBin(const MDATA_INF& arg)
{
    INT nBytes = 0;
    Set(UniArgBinPtr(arg, &nBytes), nBytes);
}

void UniWView::FromUtf8(LPCSTR szUtf8)
{
    p = (const WCHAR*)L"";
    nChars = 0;
    if (szUtf8 == NULL) return;
    INT n = MultiByteToWideChar(CP_UTF8, 0, szUtf8, -1, NULL, 0);
    if (n <= 1) return;
    pTmp = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)n * sizeof(WCHAR));
    if (pTmp == NULL) return;
    MultiByteToWideChar(CP_UTF8, 0, szUtf8, -1, pTmp, n);
    p = pTmp;
    nChars = n - 1;
}

void UniWView::FromPtr(INT ptr, INT nWant)
{
    p = (const WCHAR*)L"";
    nChars = 0;
    if (ptr == 0) return;
    if (nWant <= 0)
    {
        p = (const WCHAR*)(INT_PTR)ptr;
        nChars = lstrlenW(p);
        return;
    }
    pTmp = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)(nWant + 1) * sizeof(WCHAR));
    if (pTmp == NULL) return;
    memcpy(pTmp, (LPCSTR)(INT_PTR)ptr, (SIZE_T)nWant * sizeof(WCHAR));
    pTmp[nWant] = 0;
    p = pTmp;
    nChars = WStrLenN(p, nWant);
}

// ============================================================ 编码转换

WCHAR* UniUtf8ToWide(LPCSTR szUtf8, INT* pnChars)
{
    if (pnChars) *pnChars = 0;
    if (szUtf8 == NULL) return NULL;
    INT n = MultiByteToWideChar(CP_UTF8, 0, szUtf8, -1, NULL, 0);
    if (n <= 1) return NULL;
    WCHAR* p = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)n * sizeof(WCHAR));
    if (p == NULL) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, szUtf8, -1, p, n);
    if (pnChars) *pnChars = n - 1;
    return p;
}

char* UniWideToUtf8(const WCHAR* pW, INT nChars)
{
    if (pW == NULL || nChars < 0) nChars = 0;
    INT n = WideCharToMultiByte(CP_UTF8, 0, pW, nChars, NULL, 0, NULL, NULL);
    char* p = (char*)LocalAlloc(LPTR, (SIZE_T)n + 1);
    if (p == NULL) return NULL;
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, pW, nChars, p, n, NULL, NULL);
    p[n] = '\0';
    return p;
}

// ============================================================ 返回值写入

static void UniRetUtf8Text(PMDATA_INF pRet, LPCSTR szUtf8)
{
    pRet->m_dtDataType = SDT_TEXT;
    pRet->m_pText = (szUtf8 != NULL && szUtf8[0] != '\0') ? CloneTextData(szUtf8) : CloneTextData("", 0);
}

void UniRetWideBin(PMDATA_INF pRet, const WCHAR* pW, INT nChars)
{
    if (nChars < 0) nChars = 0;
    pRet->m_dtDataType = SDT_BIN;
    pRet->m_pBin = CloneBinData((LPCSTR)pW, nChars * 2 + 2);
}

void UniRetWideBinNoNull(PMDATA_INF pRet, const WCHAR* pW, INT nChars)
{
    if (nChars < 0) nChars = 0;
    pRet->m_dtDataType = SDT_BIN;
    pRet->m_pBin = (nChars == 0) ? NULL : CloneBinData((LPCSTR)pW, nChars * 2);
}

void UniRetRawBin(PMDATA_INF pRet, LPCSTR pData, INT nBytes)
{
    pRet->m_dtDataType = SDT_BIN;
    pRet->m_pBin = (nBytes <= 0 || pData == NULL) ? NULL : CloneBinData(pData, nBytes);
}

void UniRetWideText(PMDATA_INF pRet, const WCHAR* pW, INT nChars)
{
    if (nChars < 0) nChars = 0;
    char* sz = UniWideToUtf8(pW, nChars);
    UniRetUtf8Text(pRet, sz);
    if (sz != NULL) LocalFree(sz);
}

void UniRetBool(PMDATA_INF pRet, BOOL b)
{
    pRet->m_dtDataType = SDT_BOOL;
    pRet->m_bool = b ? TRUE : FALSE;
}

// ============================================================ UniWBuf

UniWBuf::~UniWBuf()
{
    if (p != NULL) { LocalFree(p); p = NULL; }
}

void UniWBuf::Clear()
{
    nLen = 0;
    if (p != NULL && nCap > 0) p[0] = 0;
}

void UniWBuf::Steal(UniWBuf& other)
{
    if (p != NULL) LocalFree(p);
    p = other.p;
    nLen = other.nLen;
    nCap = other.nCap;
    other.p = NULL;
    other.nLen = 0;
    other.nCap = 0;
}

bool UniWBuf::Reserve(INT nAddChars)
{
    if (nAddChars < 0) nAddChars = 0;
    if (p != NULL && nLen + nAddChars + 1 <= nCap) return true;
    INT nNeed = nLen + nAddChars + 1;
    if (nNeed < 16) nNeed = 16;
    INT nNewCap = (nCap > 0) ? nCap : 16;
    while (nNewCap < nNeed) nNewCap *= 2;
    WCHAR* q = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)nNewCap * sizeof(WCHAR));
    if (q == NULL) return false;
    if (p != NULL)
    {
        memcpy(q, p, (SIZE_T)nLen * sizeof(WCHAR));
        LocalFree(p);
    }
    p = q;
    nCap = nNewCap;
    return true;
}

bool UniWBuf::Add(const WCHAR* s, INT nChars)
{
    if (nChars <= 0 || s == NULL) return p != NULL || Reserve(0);
    if (!Reserve(nChars)) return false;
    memcpy(p + nLen, s, (SIZE_T)nChars * sizeof(WCHAR));
    nLen += nChars;
    p[nLen] = 0;
    return true;
}

bool UniWBuf::AddChar(WCHAR c)
{
    return Add(&c, 1);
}

// ============================================================ 参考数组参数

UniAryList::~UniAryList()
{
    if (pItem != NULL) LocalFree(pItem);
}

void UniAryList::Add(void* pElement)
{
    if (nCount >= nCap)
    {
        INT nNewCap = (nCap == 0) ? 16 : nCap * 2;
        void** pNew = (void**)LocalAlloc(LPTR, (SIZE_T)nNewCap * sizeof(void*));
        if (pNew == NULL) return;
        if (pItem != NULL)
        {
            memcpy(pNew, pItem, (SIZE_T)nCount * sizeof(void*));
            LocalFree(pItem);
        }
        pItem = pNew;
        nCap = nNewCap;
    }
    pItem[nCount++] = pElement;
}

static LPBYTE UniAryDataFromPtr(LPBYTE p, INT* pnCount)
{
    if (pnCount) *pnCount = 0;
    if (p == NULL) return NULL;
    INT nDim = *(LPINT)p;
    if (nDim <= 0) return NULL;
    INT nCount = 1;
    for (INT i = 0; i < nDim; i++) nCount *= *(LPINT)(p + sizeof(INT) * (i + 1));
    if (nCount <= 0) return NULL;
    if (pnCount) *pnCount = nCount;
    return p + sizeof(INT) * (nDim + 1);
}

static LPBYTE UniAryData(const MDATA_INF& arg, INT* pnCount)
{
    if (UniIsNull(arg) || arg.m_ppAryData == NULL) return NULL;
    return UniAryDataFromPtr((LPBYTE)*arg.m_ppAryData, pnCount);
}

LPBYTE UniAryRead(const MDATA_INF& arg, INT bByRef, INT* pnCount)
{
    if (pnCount) *pnCount = 0;
    if (UniIsNull(arg)) return NULL;
    if (bByRef) return UniAryData(arg, pnCount);
    return UniAryDataFromPtr((LPBYTE)arg.m_pAryData, pnCount);
}

INT UniAryCount(const MDATA_INF& arg)
{
    INT n = 0;
    UniAryData(arg, &n);
    return n;
}

void UniAryCopyFrom(const MDATA_INF& arg, UniAryList* pList)
{
    INT n = 0;
    LPBYTE p = UniAryData(arg, &n);
    if (p == NULL) return;
    DATA_TYPE dt = arg.m_dtDataType & ~DT_IS_ARY;
    if (dt == SDT_TEXT)
    {
        for (INT i = 0; i < n; i++)
        {
            LPCSTR s = ((LPCSTR*)p)[i];
            pList->Add(s != NULL ? CloneTextData(s) : CloneTextData("", 0));
        }
    }
    else if (dt == SDT_BIN)
    {
        for (INT i = 0; i < n; i++)
        {
            LPBYTE b = ((LPBYTE*)p)[i];
            INT nBytes = 0;
            if (b != NULL)
            {
                nBytes = *(LPINT)(b + sizeof(INT));
                pList->Add(CloneBinData((LPCSTR)(b + sizeof(INT) * 2), nBytes));
            }
            else pList->Add(NULL);
        }
    }
    else
    {
        ReportError("创想软件支持库：该命令只支持文本或字节集数组");
    }
}

void UniAryAppend(const MDATA_INF& arg, UniAryList* pList, INT bAppend)
{
    if (bAppend) UniAryCopyFrom(arg, pList);
    UniAryWrite(arg, pList);
}

// 数组数据块：[INT 维数=1][INT 元素数][元素指针...]，元素归易语言内存池所有。
static LPBYTE UniBuildAryBlock(UniAryList* pList)
{
    INT n = pList->nCount;
    if (n <= 0) return NULL;
    LPBYTE p = (LPBYTE)UniMalloc(sizeof(INT) * 2 + (INT)((SIZE_T)n * sizeof(void*)));
    if (p == NULL) return NULL;
    *(LPINT)p = 1;
    *(LPINT)(p + sizeof(INT)) = n;
    memcpy(p + sizeof(INT) * 2, pList->pItem, (SIZE_T)n * sizeof(void*));
    return p;
}

void UniAryWrite(const MDATA_INF& arg, UniAryList* pList)
{
    if (UniIsNull(arg) || arg.m_ppAryData == NULL) return;
    void* pOld = *arg.m_ppAryData;
    if (pOld != NULL) NotifySys(NRS_FREE_ARY, (DWORD)arg.m_dtDataType, (DWORD)(INT_PTR)pOld);
    *arg.m_ppAryData = UniBuildAryBlock(pList);   // 空数组写回 NULL，与易语言自身一致
}

// 命令直接返回数组（命令表带 CT_RETRUN_ARY_TYPE_DATA）：只填 m_pAryData，块与元素由易语言接管释放。
void UniRetAryData(PMDATA_INF pRet, UniAryList* pList)
{
    pRet->m_pAryData = UniBuildAryBlock(pList);
}

// ============================================================ 全角/半角映射

static const WCHAR g_szLocaleZhCN[] = { 'z', 'h', '-', 'C', 'N', 0 };

WCHAR* UniMapWidth(const WCHAR* pSrc, INT nChars, DWORD dwMap, INT* pnOut)
{
    if (pnOut) *pnOut = 0;
    if (pSrc == NULL || nChars <= 0) return NULL;
    const UniApi& a = UniGetApi();
    INT nNeed;
    if (a.LCMapStringEx != NULL)
        nNeed = a.LCMapStringEx(g_szLocaleZhCN, dwMap, pSrc, nChars, NULL, 0, NULL, NULL, 0);
    else
        nNeed = LCMapStringW(0x0804, dwMap, pSrc, nChars, NULL, 0);
    if (nNeed <= 0) return NULL;
    WCHAR* p = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)(nNeed + 1) * sizeof(WCHAR));
    if (p == NULL) return NULL;
    INT nRet;
    if (a.LCMapStringEx != NULL)
        nRet = a.LCMapStringEx(g_szLocaleZhCN, dwMap, pSrc, nChars, p, nNeed, NULL, NULL, 0);
    else
        nRet = LCMapStringW(0x0804, dwMap, pSrc, nChars, p, nNeed);
    if (nRet <= 0) { LocalFree(p); return NULL; }
    p[nRet] = 0;
    if (pnOut) *pnOut = nRet;
    return p;
}

// ============================================================ 文本查找

INT WsFind(const CxWStr* p, const WCHAR* s, INT nPos, BOOL bCaseInsensitive)
{
    const UniApi& a = UniGetApi();
    if (p == NULL || p->pStr == NULL || s == NULL) return -1;
    if (nPos < 0) nPos = 0;
    if (nPos > p->nLen) return -1;
    const WCHAR* r = (bCaseInsensitive && a.StrStrIW != NULL)
        ? a.StrStrIW(p->pStr + nPos, s)
        : (a.StrStrW != NULL ? a.StrStrW(p->pStr + nPos, s) : UniWcsStr(p->pStr + nPos, s));
    if (r == NULL) return -1;
    return (INT)(r - p->pStr);
}

INT WsRFind(const CxWStr* p, const WCHAR* s, INT nPos, BOOL bCaseInsensitive)
{
    const UniApi& a = UniGetApi();
    if (p == NULL || p->pStr == NULL || s == NULL || a.StrRStrW == NULL || a.StrRStrIW == NULL) return -1;
    if (nPos < 0) nPos = p->nLen;
    if (nPos > p->nLen) nPos = p->nLen;
    const WCHAR* r = bCaseInsensitive
        ? a.StrRStrIW(p->pStr, p->pStr + nPos, s)
        : a.StrRStrW(p->pStr, p->pStr + nPos, s);
    if (r == NULL) return -1;
    return (INT)(r - p->pStr);
}

// ============================================================ CxWStr

CxWStr* WsNew(INT nInitCapChars)
{
    if (nInitCapChars < 8) nInitCapChars = 8;
    CxWStr* p = (CxWStr*)LocalAlloc(LPTR, sizeof(CxWStr));
    if (p == NULL) return NULL;
    p->pStr = (WCHAR*)LocalAlloc(LPTR, (SIZE_T)(nInitCapChars + 1) * sizeof(WCHAR));
    if (p->pStr == NULL) { LocalFree(p); return NULL; }
    p->nLen = 0;
    p->nCap = (nInitCapChars + 1) * 2;
    return p;
}

void WsFree(CxWStr* p)
{
    if (p == NULL) return;
    if (p->pStr != NULL) LocalFree(p->pStr);
    LocalFree(p);
}

CxWStr* WsClone(const CxWStr* pSrc)
{
    CxWStr* p = WsNew(pSrc != NULL ? pSrc->nCap / 2 : 8);
    if (p == NULL || pSrc == NULL) return p;
    if (pSrc->nLen > 0)
    {
        if (!WsReserve(p, pSrc->nLen)) { WsFree(p); return NULL; }
        memcpy(p->pStr, pSrc->pStr, (SIZE_T)pSrc->nLen * sizeof(WCHAR));
        p->pStr[pSrc->nLen] = 0;
        p->nLen = pSrc->nLen;
    }
    return p;
}

void WsClear(CxWStr* p)
{
    if (p == NULL) return;
    if (p->pStr != NULL) { p->pStr[0] = 0; }
    p->nLen = 0;
}

bool WsReserve(CxWStr* p, INT nAddChars)
{
    if (p == NULL) return false;
    INT nNeedBytes = (p->nLen + nAddChars + 1) * 2;
    if (p->nCap >= nNeedBytes) return true;
    INT nNewChars = nNeedBytes / 2;
    if (p->nCap / 2 * 2 > 0 && p->nCap / 2 > nNewChars) nNewChars = p->nCap / 2;
    INT nTry = (p->nCap / 2) * 2 + nAddChars + 1;
    if (nTry > nNewChars) nNewChars = nTry;
    SIZE_T nNewBytes = (SIZE_T)(nNewChars + 1) * sizeof(WCHAR);
    WCHAR* pNew = (WCHAR*)LocalAlloc(LPTR, nNewBytes);
    if (pNew == NULL) return false;
    if (p->pStr != NULL)
    {
        memcpy(pNew, p->pStr, (SIZE_T)(p->nLen + 1) * sizeof(WCHAR));
        LocalFree(p->pStr);
    }
    p->pStr = pNew;
    p->nCap = (INT)nNewBytes;
    return true;
}

bool WsAppend(CxWStr* p, const WCHAR* s, INT nChars)
{
    if (p == NULL) return false;
    if (s == NULL || nChars <= 0) return p->pStr != NULL;
    if (!WsReserve(p, nChars)) return false;
    memcpy(p->pStr + p->nLen, s, (SIZE_T)nChars * sizeof(WCHAR));
    p->nLen += nChars;
    p->pStr[p->nLen] = 0;
    return true;
}

bool WsAppendLine(CxWStr* p, const WCHAR* s, INT nChars)
{
    if (!WsAppend(p, s, nChars)) return false;
    return WsAppend(p, L"\r\n", 2);
}

bool WsAppendInt(CxWStr* p, INT n)
{
    WCHAR buf[24];
    if (!WsReserve(p, 20)) return false;
    if (!p->pStr) return false;
    WCHAR* pDst = p->pStr + p->nLen;
    INT nRet = 0;
    const UniApi& a = UniGetApi();
    if (a.wvsprintfW != NULL)
    {
        INT args[1] = { n };
        nRet = a.wvsprintfW(buf, L"%d", args);
    }
    else nRet = wsprintfW(buf, L"%d", n);
    if (nRet <= 0) return false;
    return WsAppend(p, buf, nRet);
}

bool WsInsert(CxWStr* p, INT nPos, const WCHAR* s, INT nChars)
{
    if (p == NULL || p->pStr == NULL) return false;
    if (nPos < 0 || nPos > p->nLen) nPos = p->nLen;
    if (s == NULL || nChars <= 0) return true;
    if (!WsReserve(p, nChars)) return false;
    memmove(p->pStr + nPos + nChars, p->pStr + nPos, (SIZE_T)(p->nLen - nPos) * sizeof(WCHAR));
    memcpy(p->pStr + nPos, s, (SIZE_T)nChars * sizeof(WCHAR));
    p->nLen += nChars;
    p->pStr[p->nLen] = 0;
    return true;
}

bool WsReplaceAt(CxWStr* p, INT nPos, INT nLen, const WCHAR* s, INT nNewChars)
{
    if (p == NULL || p->pStr == NULL) return false;
    if (nPos < 0 || nPos > p->nLen) return false;
    if (nLen < 0 || nPos + nLen > p->nLen) nLen = p->nLen - nPos;
    if (s == NULL) nNewChars = 0;
    if (nNewChars < 0) nNewChars = 0;
    if (nNewChars > 0 && !WsReserve(p, nNewChars)) return false;
    memmove(p->pStr + nPos + nNewChars, p->pStr + nPos + nLen,
        (SIZE_T)(p->nLen - nPos - nLen) * sizeof(WCHAR));
    if (nNewChars > 0) memcpy(p->pStr + nPos, s, (SIZE_T)nNewChars * sizeof(WCHAR));
    p->nLen = p->nLen - nLen + nNewChars;
    p->pStr[p->nLen] = 0;
    return true;
}

bool WsSubReplace(CxWStr* p, const WCHAR* pOld, const WCHAR* pNew, INT nPos, INT nCount, BOOL bCaseInsensitive)
{
    if (p == NULL || p->pStr == NULL || pOld == NULL) return false;
    if (nCount < 0 || nPos < 0) return false;
    INT nOldLen = lstrlenW(pOld);
    if (nOldLen <= 0) return true;                       // 空子文本无可替换，直接返回避免死循环
    INT nNewLen = (pNew != NULL) ? lstrlenW(pNew) : 0;
    if (nPos > p->nLen) return true;

    const UniApi& a = UniGetApi();
    UniWBuf out;
    if (!out.Reserve(p->nLen + 1)) return false;
    INT i = nPos;                                        // 已复制到的源位置
    INT nHit = 0;
    bool bFound = false;
    for (;;)
    {
        const WCHAR* hit = NULL;
        if (bCaseInsensitive && a.StrStrIW != NULL) hit = a.StrStrIW(p->pStr + i, pOld);
        else if (a.StrStrW != NULL) hit = a.StrStrW(p->pStr + i, pOld);
        else hit = UniWcsStr(p->pStr + i, pOld);
        if (hit == NULL) break;
        INT nPre = (INT)(hit - p->pStr);
        bFound = true;
        if (!out.Add(p->pStr + i, nPre - i)) return false;
        if (nNewLen > 0 && !out.Add(pNew, nNewLen)) return false;
        i = nPre + nOldLen;
        if (++nHit >= nCount && nCount != 0) break;
    }
    if (!bFound) return false;
    if (!out.Add(p->pStr + i, p->nLen - i)) return false;

    WsClear(p);
    return WsAppend(p, out.p, out.nLen);
}

bool WsTrimChars(CxWStr* p, const WCHAR* pTrim)
{
    if (p == NULL || p->pStr == NULL || pTrim == NULL) return false;
    const UniApi& a = UniGetApi();
    if (a.StrTrimW == NULL) return false;
    if (!a.StrTrimW(p->pStr, pTrim)) return false;
    p->nLen = lstrlenW(p->pStr);
    return true;
}

void WsToUpper(CxWStr* p)
{
    if (p != NULL && p->pStr != NULL) CharUpperW(p->pStr);
}

void WsToLower(CxWStr* p)
{
    if (p != NULL && p->pStr != NULL) CharLowerW(p->pStr);
}

void WsToFullWidth(CxWStr* p)
{
    if (p == NULL || p->pStr == NULL || p->nLen <= 0) return;
    INT nOut = 0;
    WCHAR* q = UniMapWidth(p->pStr, p->nLen, LCMAP_FULLWIDTH, &nOut);
    if (q == NULL) return;
    WsClear(p);
    WsAppend(p, q, nOut);
    LocalFree(q);
}

void WsToHalfWidth(CxWStr* p)
{
    if (p == NULL || p->pStr == NULL || p->nLen <= 0) return;
    INT nOut = 0;
    WCHAR* q = UniMapWidth(p->pStr, p->nLen, LCMAP_HALFWIDTH, &nOut);
    if (q == NULL) return;
    WsClear(p);
    WsAppend(p, q, nOut);
    LocalFree(q);
}

INT WsToInt(const CxWStr* p, BOOL bHex)
{
    if (p == NULL || p->pStr == NULL) return 0;
    const UniApi& a = UniGetApi();
    INT nRet = 0;
    if (a.StrToIntExW != NULL)
        a.StrToIntExW(p->pStr, bHex ? UNI_STIF_SUPPORT_HEX : UNI_STIF_DEFAULT, &nRet);
    else
    {
        // 取不到 shlwapi 时手工解析，避免依赖 VC6 没有的宽字符转换函数。
        const WCHAR* q = p->pStr;
        while (*q == ' ' || *q == '	') q++;
        INT nSign = 1;
        if (*q == '-') { nSign = -1; q++; } else if (*q == '+') q++;
        INT nBase = bHex ? 16 : 10;
        if (nBase == 10 && q[0] == '0' && (q[1] == 'x' || q[1] == 'X')) { nBase = 16; q += 2; }
        INT nVal = 0;
        while (*q != 0)
        {
            WCHAR c = *q;
            INT d;
            if (c >= '0' && c <= '9') d = c - '0';
            else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
            else break;
            if (d >= nBase) break;
            nVal = nVal * nBase + d;
            q++;
        }
        nRet = nSign * nVal;
    }
    return nRet;
}

INT WsCompare(const CxWStr* p, const WCHAR* pOther)
{
    if (p == NULL || p->pStr == NULL || pOther == NULL) return -1;
    return lstrcmpiW(p->pStr, pOther);
}

CxWStr* WsSub(const CxWStr* p, INT nStart, INT nLen)
{
    if (p == NULL) return NULL;
    if (nLen < 0) nLen = (p->pStr != NULL) ? lstrlenW(p->pStr) : 0;
    if (p->pStr == NULL || nStart < 0 || nStart > p->nLen) return WsNew(8);
    if (nStart + nLen > p->nLen) nLen = p->nLen - nStart;
    CxWStr* r = WsNew(nLen > 0 ? nLen : 8);
    if (r != NULL && nLen > 0) WsAppend(r, p->pStr + nStart, nLen);
    return r;
}

// ============================================================ 查找与替换基础函数

INT UniFindWide(const WCHAR* pSrc, INT nSrcChars, const WCHAR* pFind, INT nPos1, BOOL bCaseInsensitive)
{
    const UniApi& a = UniGetApi();
    if (pSrc == NULL || pFind == NULL) return -1;
    if (nPos1 < 1) return -1;
    if (nPos1 - 1 > nSrcChars) return -1;          // 允许指向终止符
    const WCHAR* r;
    if (bCaseInsensitive && a.StrStrIW != NULL) r = a.StrStrIW(pSrc + nPos1 - 1, pFind);
    else if (a.StrStrW != NULL) r = a.StrStrW(pSrc + nPos1 - 1, pFind);
    else r = UniWcsStr(pSrc + nPos1 - 1, pFind);
    if (r == NULL) return -1;
    return (INT)(r - pSrc) + 1;
}

INT UniRFindWide(const WCHAR* pSrc, INT nSrcChars, const WCHAR* pFind, INT nPos1, BOOL bCaseInsensitive)
{
    const UniApi& a = UniGetApi();
    if (pSrc == NULL || pFind == NULL) return -1;
    if (nPos1 <= 0) nPos1 = nSrcChars + 1;
    if (nPos1 > nSrcChars + 1) nPos1 = nSrcChars + 1;
    if (bCaseInsensitive && a.StrRStrIW != NULL)
    {
        const WCHAR* r = a.StrRStrIW(pSrc, pSrc + nPos1 - 1, pFind);
        if (r == NULL) return -1;
        return (INT)(r - pSrc) + 1;
    }
    // shlwapi 只导出 StrRStrIA/StrRStrIW，没有 StrRStrW；区分大小写时自己正向扫到最后一次命中。
    INT nLimit = nPos1 - 1;
    INT nFind = lstrlenW(pFind);
    if (nFind <= 0 || nLimit <= 0) return -1;
    INT best = -1, pos = 1;
    for (;;)
    {
        INT nAt = UniFindWide(pSrc, nLimit, pFind, pos, FALSE);
        if (nAt < 0) break;
        best = nAt;
        pos = nAt + nFind;
    }
    return best;
}

bool UniSubReplaceWide(const WCHAR* pSrc, INT nSrcChars, const WCHAR* pOld, const WCHAR* pNew,
                       INT nPos1, INT nCount, BOOL bCaseSensitive, UniWBuf* pOut)
{
    if (pSrc == NULL || pOld == NULL || pOut == NULL) return false;
    INT nOldLen = lstrlenW(pOld);
    if (nOldLen <= 0) return false;
    if (nPos1 < 1 || nPos1 > nSrcChars) return false;
    INT nNewLen = (pNew != NULL) ? lstrlenW(pNew) : 0;

    const UniApi& a = UniGetApi();
    if (!pOut->Reserve(nSrcChars + 1)) return false;
    INT i = nPos1 - 1;                              // 起始位置之前的内容原样保留
    if (i > 0 && !pOut->Add(pSrc, i)) return false;
    const WCHAR* hit;
    if (bCaseSensitive && a.StrStrW != NULL) hit = a.StrStrW(pSrc + i, pOld);
    else if (!bCaseSensitive && a.StrStrIW != NULL) hit = a.StrStrIW(pSrc + i, pOld);
    else hit = UniWcsStr(pSrc + i, pOld);
    if (hit == NULL) { pOut->nLen = 0; return false; }

    INT nRep = 0;
    while (hit != NULL)
    {
        INT nPre = (INT)(hit - pSrc);
        if (!pOut->Add(pSrc + i, nPre - i)) return false;
        if (nNewLen > 0 && !pOut->Add(pNew, nNewLen)) return false;
        i = nPre + nOldLen;
        if (++nRep >= nCount && nCount != 0) break;
        if (bCaseSensitive && a.StrStrW != NULL) hit = a.StrStrW(pSrc + i, pOld);
        else if (!bCaseSensitive && a.StrStrIW != NULL) hit = a.StrStrIW(pSrc + i, pOld);
        else hit = UniWcsStr(pSrc + i, pOld);
    }
    if (i < nSrcChars && !pOut->Add(pSrc + i, nSrcChars - i)) return false;
    return true;
}

void UniRetWC(PMDATA_INF pRet, const WCHAR* p, INT nChars)
{
    static const WCHAR szEmpty[1] = { 0 };
    UniWBuf b;
    if (nChars > 0 && p != NULL)
    {
        if (!b.Reserve(nChars + 1)) { UniRetWideBin(pRet, szEmpty, 0); return; }
        for (INT i = 0; i < nChars; i++) if (p[i] != 0) b.AddChar(p[i]);
    }
    UniRetWideBin(pRet, (b.p != NULL) ? b.p : szEmpty, b.nLen);
}

// ============================================================ 分割

UniWRangeList::~UniWRangeList()
{
    if (pStart != NULL) LocalFree(pStart);
    if (pChars != NULL) LocalFree(pChars);
}

void UniWRangeList::Add(INT nStart, INT nChars)
{
    if (nCount >= nCap)
    {
        INT nNewCap = (nCap == 0) ? 16 : nCap * 2;
        INT* q1 = (INT*)LocalAlloc(LPTR, (SIZE_T)nNewCap * sizeof(INT));
        INT* q2 = (INT*)LocalAlloc(LPTR, (SIZE_T)nNewCap * sizeof(INT));
        if (q1 == NULL || q2 == NULL)
        {
            if (q1 != NULL) LocalFree(q1);
            if (q2 != NULL) LocalFree(q2);
            return;
        }
        if (pStart != NULL)
        {
            memcpy(q1, pStart, (SIZE_T)nCount * sizeof(INT));
            memcpy(q2, pChars, (SIZE_T)nCount * sizeof(INT));
            LocalFree(pStart);
            LocalFree(pChars);
        }
        pStart = q1;
        pChars = q2;
        nCap = nNewCap;
    }
    pStart[nCount] = nStart;
    pChars[nCount] = nChars;
    nCount++;
}

void WsSplit(const CxWStr* p, const WCHAR* pSep, INT nSepChars, BOOL bRemoveEmpty, UniWRangeList* pOut)
{
    if (p == NULL || p->pStr == NULL || pSep == NULL || pOut == NULL || p->nLen <= 0) return;
    if (nSepChars <= 0) nSepChars = lstrlenW(pSep);
    if (nSepChars <= 0) { pOut->Add(0, p->nLen); return; }

    const UniApi& a = UniGetApi();
    INT i = 0;
    bool bFound = false;
    for (;;)
    {
        const WCHAR* hit = NULL;
        if (a.StrStrW != NULL) hit = a.StrStrW(p->pStr + i, pSep);
        else hit = UniWcsStr(p->pStr + i, pSep);
        if (hit == NULL) break;
        INT nPre = (INT)(hit - p->pStr);
        bFound = true;
        if (!(nPre == i && bRemoveEmpty)) pOut->Add(i, nPre - i);
        i = nPre + nSepChars;
    }
    if (!bFound) { pOut->Add(0, p->nLen); return; }
    if (i <= p->nLen)
    {
        if (bRemoveEmpty && i == p->nLen) return;
        pOut->Add(i, p->nLen - i);
    }
}

}  // namespace cx
