// HTTP访问 族：原模块「HTTP访问_WinHTTP访问类」「HTTP访问_WinInet访问类」两条访问路线
// 与全局命令「网页_访问_WinHTTP」「网页_访问_WinInet」的 C++ 实现。
// 由 lib.cpp 在 namespace cx 内、cxhttp_access_curl.inc 之后 #include；
// 派发写法与 cxhttp_extra.inc 的 HExtra、cxhttp_access_curl.inc 的 HAccessCurlInvoke 同构。
// 参数名/顺序/类型/可空/参考/返回类型逐字照原源码；语义以 http访问\规格_访问类.md 第二、三、四节为准。
//
// 硬约束（本文件自己遵守）：
//  · 不 STL / 不 new / 不异常 / 不 sprintf / 不 #pragma comment(lib) / 不静态链接 winhttp、wininet；
//  · 两个 DLL 一律运行期 LoadLibraryA + GetProcAddress，入口指针缓存；
//  · 拿不到入口只 ReportError 并回空字节集，绝不解引用空指针；
//  · 回调子程序地址先 IsBadReadPtr 验过再按 cdecl 调，任何一步失败都不许让宿主崩。
//
// 相对原模块的处理（对照规格第六节）：
//  照抄：Cookie 三点优先级与三条路线各自差异（WinInet 的合并只看 是否自动更新COOKIE、不用系统默认时清空提交串）、
//        访问方式按 提交信息 自动定 GET/POST、自适应接收块大小的短路判断链、每个状态回调的时机与参数、
//        发送 5 次重试、WinInet 无 站点登录 参数、InternetOpenA 的 accessType 字面量 + INTERNET_OPTION_PROXY 双设、
//        返回COOKIE 不写回（原声明缺 参考，坑 10）。
//  修复：句柄统一按 请求→连接→会话 在每条出口关闭（坑 12 的泄漏与两条路线顺序不一致）；
//        WinHttpOpenRequest 按 NULL 判失败（坑 11 的 <0 漏判）；超时只在「秒→毫秒」处乘一次，
//        WinInet 的 301 改成真解析 Location 的有限次循环（坑 20 的二次×1000 与不换网址的死循环）；
//        响应头缓冲按 API 回读的需求长度扩容重试（坑 13）；UA 不再带 "User-Agent: " 头名（坑 17）。
#ifndef CXHTTP_ACCESS_WIN_INC
#define CXHTTP_ACCESS_WIN_INC

#include "cxhttp_util.h"     // 自带 include guard，重复包含无害；保证 Ux* 声明一定可见

// ============================================================ 常量（规格 第五节）
// ---- WinHTTP ----
#define CWH_ACCESS_TYPE_DEFAULT_PROXY      0
#define CWH_ACCESS_TYPE_NAMED_PROXY        3
#define CWH_ACCESS_TYPE_AUTOMATIC_PROXY    4
#define CWH_FLAG_SECURE                    0x00800000
#define CWH_OPTION_PROXY                   38
#define CWH_OPTION_REDIRECT_POLICY         88
#define CWH_REDIRECT_POLICY_NEVER          0
#define CWH_REDIRECT_POLICY_ALWAYS         2
#define CWH_QUERY_RAW_HEADERS_CRLF         22
#define CWH_ADDREQ_FLAG_ADD                0x20000000
#define CWH_ADDREQ_FLAG_REPLACE            0x80000000
#define CWH_AUTH_TARGET_SERVER             0
#define CWH_AUTH_TARGET_PROXY              1
#define CWH_AUTH_SCHEME_BASIC              1
// ---- WinInet ----
// 注：INTERNET_OPEN_TYPE_DIRECT 就是 1（规格坑 18 把源码里的字面量 1 误标成 PRECONFIG，值照源码）
#define CWI_OPEN_TYPE_DIRECT               1
#define CWI_OPEN_TYPE_PROXY                3
#define CWI_OPTION_CONNECT_TIMEOUT         2
#define CWI_OPTION_SEND_TIMEOUT            5
#define CWI_OPTION_RECEIVE_TIMEOUT         6
#define CWI_OPTION_PROXY                   38
#define CWI_OPTION_PROXY_USERNAME          43
#define CWI_OPTION_PROXY_PASSWORD          44
#define CWI_FLAG_RELOAD                    0x80000000
#define CWI_COOKIE_THIRD_PARTY             0x10
#define CWI_FLAG_NO_COOKIES                0x80000
#define CWI_FLAG_NO_AUTO_REDIRECT          0x200000
#define CWI_FLAG_SECURE                    0x800000
#define CWI_FLAG_IGNORE_REDIRECT_TO_HTTPS  0x4000
#define CWI_SERVICE_HTTP                   3
#define CWI_QUERY_RAW_HEADERS_CRLF         22
// ---- HTTP状态_*（回调第一参数；错误_接收不到数据 与 处理完成 同值 11，靠参数区分——照原模块）----
#define CWS_ERR_IFACE_UNAVAILABLE          0
#define CWS_SESSION                        1
#define CWS_CONNECT                        2
#define CWS_REQUEST                        3
#define CWS_SENDING                        4
#define CWS_WAITING                        5
#define CWS_QUERY_HEAD                     6
#define CWS_CONTENT_LEN                    7
#define CWS_BEGIN_DATA                     8
#define CWS_RECV_LEN                       9
#define CWS_RECV_DATA                      10
#define CWS_DONE                           11
#define CWS_ERR_NO_DATA                    11
#define CWS_ERR_WAIT_TIMEOUT               12
#define CWS_ERR_SEND_FAILED                13
#define CWS_ERR_MAKE_REQUEST               14
#define CWS_ERR_MAKE_CONNECT               15
#define CWS_ERR_MAKE_SESSION               16
#define CWS_ERR_SEND_RETRY                 17

#define CWH_DEFAULT_CHUNK                  512        // #HTTP配置_默认每次接收数据长度
#define CWH_HEAD_BUF                       50000      // WinHttpQueryHeaders 缓冲（字节，照原模块）
#define CWI_HEAD_BUF                       5000       // HttpQueryInfoA 缓冲（字节，照原模块）
#define CWH_HEAD_BUF_MAX                   262144     // 扩容重试上限（修规格坑 13）
#define CWIN_REDIR_MAXHOP                  3          // WinInet 手动 301 的最大跳数（修坑 20 的死循环风险）

#define CWIN_RET_DONE                      0          // 一次访问正常结束
#define CWIN_RET_FAIL                      1          // 一次访问失败（已回调、已关句柄、正文空）
#define CWIN_RET_301                       2          // 需要按 Location 再访问一次

// #HTTP配置_默认浏览器UA 的原值含 "User-Agent: " 头名，传给 lpszAgent/pwszUserAgent 会发出不合法
// UA（规格坑 17），这里只存纯值；协议头里的 User-Agent 行仍由 UxBuildHeaders 负责补全。
static const char* const s_szCxDefaultUa =
    "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
    "Chrome/50.0.2661.87 Safari/537.36";

// ============================================================ 运行期入口表
// 句柄统一用 void*，不引 winhttp.h / wininet.h（免得拖进它们的 #pragma comment(lib) 与链接依赖）。
typedef void* CWH_HIN;

typedef struct
{
    DWORD   dwAccessType;
    LPCWSTR lpszProxy;
    LPCWSTR lpszProxyBypass;
} CWH_PROXY_INFO;                          // WINHTTP_PROXY_INFO（x86 12 字节）

typedef struct
{
    DWORD dwAccessType;
    LPSTR lpszProxy;
    LPSTR lpszProxyBypass;
} CWI_PROXY_INFO;                          // INTERNET_PROXY_INFO 的 ANSI 版（x86 12 字节）

typedef struct
{
    HMODULE hModule;
    int     nTried;                        // 试过且失败过就别反复敲 LoadLibrary
    char    szErr[96];
    // 必需入口
    CWH_HIN (WINAPI *Open)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
    CWH_HIN (WINAPI *Connect)(CWH_HIN, LPCWSTR, DWORD, DWORD);
    CWH_HIN (WINAPI *OpenRequest)(CWH_HIN, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD);
    BOOL (WINAPI *SendRequest)(CWH_HIN, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR);
    BOOL (WINAPI *ReceiveResponse)(CWH_HIN, LPVOID);
    BOOL (WINAPI *QueryHeaders)(CWH_HIN, DWORD, LPCWSTR, LPVOID, LPDWORD, LPDWORD);
    BOOL (WINAPI *QueryDataAvailable)(CWH_HIN, LPDWORD);
    BOOL (WINAPI *ReadData)(CWH_HIN, LPVOID, DWORD, LPDWORD);
    BOOL (WINAPI *CloseHandle)(CWH_HIN);
    // 可选入口：为 NULL 时调用点判空跳过
    BOOL (WINAPI *CheckPlatform)(void);
    BOOL (WINAPI *SetTimeouts)(CWH_HIN, int, int, int, int);
    BOOL (WINAPI *SetOption)(CWH_HIN, DWORD, LPVOID, DWORD);
    BOOL (WINAPI *SetCredentials)(CWH_HIN, DWORD, DWORD, LPCWSTR, LPCWSTR, LPVOID);
    BOOL (WINAPI *AddRequestHeaders)(CWH_HIN, LPCWSTR, DWORD, DWORD);
} CxWinHttpApi;

typedef struct
{
    HMODULE hModule;
    int     nTried;
    char    szErr[96];
    CWH_HIN (WINAPI *OpenA)(LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD);
    CWH_HIN (WINAPI *ConnectA)(CWH_HIN, LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD, DWORD, DWORD_PTR);
    CWH_HIN (WINAPI *OpenRequestA)(CWH_HIN, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR, DWORD, DWORD_PTR);
    BOOL (WINAPI *SendRequestA)(CWH_HIN, LPCSTR, DWORD, LPCVOID, DWORD);
    BOOL (WINAPI *QueryInfoA)(CWH_HIN, DWORD, LPVOID, LPDWORD, LPDWORD);
    BOOL (WINAPI *ReadFile)(CWH_HIN, LPVOID, DWORD, LPDWORD);
    BOOL (WINAPI *SetOptionA)(CWH_HIN, DWORD, LPCVOID, DWORD);
    BOOL (WINAPI *CloseHandle)(CWH_HIN);
} CxWinInetApi;

static CxWinHttpApi g_cxWinHttp;
static CxWinInetApi g_cxWinInet;

#define CWH_BIND(field, cast, name) \
    g_cxWinHttp.field = (cast)GetProcAddress(g_cxWinHttp.hModule, name)
#define CWI_BIND(field, cast, name) \
    g_cxWinInet.field = (cast)GetProcAddress(g_cxWinInet.hModule, name)

// 原模块 DLL命令 段（Winhttp.dll）：WinHttpCheckPlatform / WinHttpOpen / ... / WinHttpCloseHandle
static bool HWinHttpLoadImpl(void)
{
    if (g_cxWinHttp.hModule != NULL) return true;
    if (g_cxWinHttp.nTried != 0) return false;
    g_cxWinHttp.nTried = 1;
    g_cxWinHttp.hModule = LoadLibraryA("winhttp.dll");
    if (g_cxWinHttp.hModule == NULL)
    {
        lstrcpynA(g_cxWinHttp.szErr, "访问网页_WinHTTP：winhttp.dll 加载失败", sizeof(g_cxWinHttp.szErr));
        return false;
    }
    CWH_BIND(Open, CWH_HIN (WINAPI *)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD), "WinHttpOpen");
    CWH_BIND(Connect, CWH_HIN (WINAPI *)(CWH_HIN, LPCWSTR, DWORD, DWORD), "WinHttpConnect");
    CWH_BIND(OpenRequest, CWH_HIN (WINAPI *)(CWH_HIN, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, DWORD), "WinHttpOpenRequest");
    CWH_BIND(SendRequest, BOOL (WINAPI *)(CWH_HIN, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR), "WinHttpSendRequest");
    CWH_BIND(ReceiveResponse, BOOL (WINAPI *)(CWH_HIN, LPVOID), "WinHttpReceiveResponse");
    CWH_BIND(QueryHeaders, BOOL (WINAPI *)(CWH_HIN, DWORD, LPCWSTR, LPVOID, LPDWORD, LPDWORD), "WinHttpQueryHeaders");
    CWH_BIND(QueryDataAvailable, BOOL (WINAPI *)(CWH_HIN, LPDWORD), "WinHttpQueryDataAvailable");
    CWH_BIND(ReadData, BOOL (WINAPI *)(CWH_HIN, LPVOID, DWORD, LPDWORD), "WinHttpReadData");
    CWH_BIND(CloseHandle, BOOL (WINAPI *)(CWH_HIN), "WinHttpCloseHandle");
    if (g_cxWinHttp.Open == NULL || g_cxWinHttp.Connect == NULL || g_cxWinHttp.OpenRequest == NULL
        || g_cxWinHttp.SendRequest == NULL || g_cxWinHttp.ReceiveResponse == NULL
        || g_cxWinHttp.QueryHeaders == NULL || g_cxWinHttp.QueryDataAvailable == NULL
        || g_cxWinHttp.ReadData == NULL || g_cxWinHttp.CloseHandle == NULL)
    {
        g_cxWinHttp.hModule = NULL;
        lstrcpynA(g_cxWinHttp.szErr, "访问网页_WinHTTP：winhttp.dll 缺少必需的导出入口", sizeof(g_cxWinHttp.szErr));
        return false;
    }
    CWH_BIND(CheckPlatform, BOOL (WINAPI *)(void), "WinHttpCheckPlatform");
    CWH_BIND(SetTimeouts, BOOL (WINAPI *)(CWH_HIN, int, int, int, int), "WinHttpSetTimeouts");
    CWH_BIND(SetOption, BOOL (WINAPI *)(CWH_HIN, DWORD, LPVOID, DWORD), "WinHttpSetOption");
    CWH_BIND(SetCredentials, BOOL (WINAPI *)(CWH_HIN, DWORD, DWORD, LPCWSTR, LPCWSTR, LPVOID), "WinHttpSetCredentials");
    CWH_BIND(AddRequestHeaders, BOOL (WINAPI *)(CWH_HIN, LPCWSTR, DWORD, DWORD), "WinHttpAddRequestHeaders");
    return true;
}

// 原模块 DLL命令 段（wininet.dll）：InternetOpenA / InternetConnectA / HttpOpenRequestA / ...（全 A 版）
static bool HWinInetLoadImpl(void)
{
    if (g_cxWinInet.hModule != NULL) return true;
    if (g_cxWinInet.nTried != 0) return false;
    g_cxWinInet.nTried = 1;
    g_cxWinInet.hModule = LoadLibraryA("wininet.dll");
    if (g_cxWinInet.hModule == NULL)
    {
        lstrcpynA(g_cxWinInet.szErr, "访问网页_WinInet：wininet.dll 加载失败", sizeof(g_cxWinInet.szErr));
        return false;
    }
    CWI_BIND(OpenA, CWH_HIN (WINAPI *)(LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD), "InternetOpenA");
    CWI_BIND(ConnectA, CWH_HIN (WINAPI *)(CWH_HIN, LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD, DWORD, DWORD_PTR), "InternetConnectA");
    CWI_BIND(OpenRequestA, CWH_HIN (WINAPI *)(CWH_HIN, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR, DWORD, DWORD_PTR), "HttpOpenRequestA");
    CWI_BIND(SendRequestA, BOOL (WINAPI *)(CWH_HIN, LPCSTR, DWORD, LPCVOID, DWORD), "HttpSendRequestA");
    CWI_BIND(QueryInfoA, BOOL (WINAPI *)(CWH_HIN, DWORD, LPVOID, LPDWORD, LPDWORD), "HttpQueryInfoA");
    CWI_BIND(ReadFile, BOOL (WINAPI *)(CWH_HIN, LPVOID, DWORD, LPDWORD), "InternetReadFile");
    CWI_BIND(SetOptionA, BOOL (WINAPI *)(CWH_HIN, DWORD, LPCVOID, DWORD), "InternetSetOptionA");
    CWI_BIND(CloseHandle, BOOL (WINAPI *)(CWH_HIN), "InternetCloseHandle");
    if (g_cxWinInet.OpenA == NULL || g_cxWinInet.ConnectA == NULL || g_cxWinInet.OpenRequestA == NULL
        || g_cxWinInet.SendRequestA == NULL || g_cxWinInet.QueryInfoA == NULL
        || g_cxWinInet.ReadFile == NULL || g_cxWinInet.CloseHandle == NULL)
    {
        g_cxWinInet.hModule = NULL;
        lstrcpynA(g_cxWinInet.szErr, "访问网页_WinInet：wininet.dll 缺少必需的导出入口", sizeof(g_cxWinInet.szErr));
        return false;
    }
    return true;
}

static volatile LONG s_httpLoadState = 0, s_inetLoadState = 0;
static bool HWinLoadOnce(volatile LONG* state, bool (*load)())
{
    LONG value = InterlockedCompareExchange(state, 1, 0);
    if (value == 0)
    {
        const bool ok = load();
        InterlockedExchange(state, ok ? 2 : -1);
        return ok;
    }
    while ((value = InterlockedCompareExchange(state, 0, 0)) == 1) Sleep(0);
    return value == 2;
}
static bool HWinHttpLoad() { return HWinLoadOnce(&s_httpLoadState, HWinHttpLoadImpl); }
static bool HWinInetLoad() { return HWinLoadOnce(&s_inetLoadState, HWinInetLoadImpl); }

#include "cxhttp_win_async.inc"

// ============================================================ 参数与文本小工具
// 「是否为空(参数)」：可空参数没传时 m_dtDataType 是 _SDT_NULL（lib2.h 对 MDATA_INF 的说明），
// 空文本/0 值算「传了」——原模块大量 选择(是否为空(x), 默认值, x) 就靠这个区分。
static bool HWinGiven(PMDATA_INF pArgInf, INT nArgCount, int i)
{
    return i < nArgCount && pArgInf[i].m_dtDataType != _SDT_NULL;
}
static int HWinIntAt(PMDATA_INF pArgInf, INT nArgCount, int i, int nDefault)
{
    return HWinGiven(pArgInf, nArgCount, i) ? pArgInf[i].m_int : nDefault;
}
static BOOL HWinBoolAt(PMDATA_INF pArgInf, INT nArgCount, int i, BOOL bDefault)
{
    if (!HWinGiven(pArgInf, nArgCount, i)) return bDefault;
    return pArgInf[i].m_bool == BL_TRUE ? TRUE : FALSE;
}
static LPCSTR HWinTextAt(PMDATA_INF pArgInf, INT nArgCount, int i)
{
    if (!HWinGiven(pArgInf, nArgCount, i) || pArgInf[i].m_pText == NULL) return "";
    return pArgInf[i].m_pText;
}
// 参考 文本型 出参槽：没传就不回写（原模块 返回头 的声明是 参考 可空）
static PMDATA_INF HWinRefAt(PMDATA_INF pArgInf, INT nArgCount, int i)
{
    return HWinGiven(pArgInf, nArgCount, i) ? &pArgInf[i] : NULL;
}

// 字节集实参的数据与长度（块 = [保留=1][长度][数据]），与 cxhttp_access_curl.inc 同规则、不同名
static LPCSTR HWinBinPtr(const MDATA_INF& a)
{
    if (a.m_dtDataType != SDT_BIN || a.m_pBin == NULL) return NULL;
    return (LPCSTR)((LPBYTE)a.m_pBin + sizeof(INT) * 2);
}
static int HWinBinLen(const MDATA_INF& a)
{
    if (a.m_dtDataType != SDT_BIN || a.m_pBin == NULL) return 0;
    const int n = *(INT*)((LPBYTE)a.m_pBin + sizeof(INT));
    return (n < 0 || n > 0x08000000) ? 0 : n;
}

static void HWinCopy(CxBuf* pOut, LPCSTR sz)
{
    if (pOut != NULL && sz != NULL) BufAppend(pOut, sz, lstrlenA(sz));
}

// 到大写（只动 ASCII，同易语言 到大写）
static void HWinUpper(CxBuf* pOut, LPCSTR sz)
{
    if (pOut == NULL || sz == NULL) return;
    for (int i = 0; sz[i] != '\0'; i++)
    {
        char c = sz[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        BufAppend(pOut, &c, 1);
    }
}

// 逐字节比较，可忽略 ASCII 大小写（不用 lstrnicmpA：SDK 头里不一定声明了它）
static int HWinNCmp(LPCSTR pA, LPCSTR pB, int n, BOOL bIgnoreCase)
{
    for (int i = 0; i < n; i++)
    {
        BYTE a = (BYTE)pA[i], b = (BYTE)pB[i];
        if (bIgnoreCase)
        {
            if (a >= 'A' && a <= 'Z') a = (BYTE)(a + 32);
            if (b >= 'A' && b <= 'Z') b = (BYTE)(b + 32);
        }
        if (a != b) return (int)a - (int)b;
        if (a == 0) return 0;
    }
    return 0;
}

// 寻找文本(被搜文本, 目标, , 真)：不区分大小写找子串，找不到返回 NULL
static LPCSTR HWinFindNi(LPCSTR szHay, LPCSTR szNeed)
{
    if (szHay == NULL || szNeed == NULL) return NULL;
    const int nNeed = lstrlenA(szNeed);
    if (nNeed <= 0) return NULL;
    for (LPCSTR p = szHay; *p != '\0'; p++)
        if (HWinNCmp(p, szNeed, nNeed, TRUE) == 0) return p;
    return NULL;
}

// 文本_取出中间文本(源, 左标记, 右标记, , 是否不区分大小写)；取不到返回假
static BOOL HWinBetween(CxBuf* pOut, LPCSTR szSrc, LPCSTR szLeft, LPCSTR szRight, BOOL bIgnoreCase)
{
    if (pOut == NULL || szSrc == NULL || szLeft == NULL) return FALSE;
    const int nLeft = lstrlenA(szLeft);
    LPCSTR pL = NULL;
    if (bIgnoreCase)
    {
        for (LPCSTR p = szSrc; *p != '\0'; p++)
            if (HWinNCmp(p, szLeft, nLeft, TRUE) == 0) { pL = p + nLeft; break; }
    }
    else
    {
        LPCSTR pHit = strstr(szSrc, szLeft);
        if (pHit != NULL) pL = pHit + nLeft;
    }
    if (pL == NULL) return FALSE;
    LPCSTR pR = (szRight == NULL || szRight[0] == '\0') ? NULL : strstr(pL, szRight);
    if (pR == NULL) pR = pL + lstrlenA(pL);
    if (pR > pL) BufAppend(pOut, pL, (int)(pR - pL));
    return TRUE;
}

// 删首尾空（空格/制表/CR/LF）
static void HWinTrim(CxBuf* pOut, LPCSTR sz)
{
    if (pOut == NULL || sz == NULL) return;
    const int nAll = lstrlenA(sz);
    int nBeg = 0;
    int nEnd = nAll;
    while (nBeg < nEnd && (sz[nBeg] == ' ' || sz[nBeg] == '\t' || sz[nBeg] == '\r' || sz[nBeg] == '\n')) nBeg++;
    while (nEnd > nBeg && (sz[nEnd - 1] == ' ' || sz[nEnd - 1] == '\t'
        || sz[nEnd - 1] == '\r' || sz[nEnd - 1] == '\n')) nEnd--;
    if (nEnd > nBeg) BufAppend(pOut, sz + nBeg, nEnd - nBeg);
}

// M2W(文本, #CP_ACP)：GBK → UTF-16 堆缓冲（调用者 LocalFree）；空文本返回 NULL，等同 #WINHTTP_NO_*
static LPWSTR HWinM2W(LPCSTR szGbk)
{
    if (szGbk == NULL || szGbk[0] == '\0') return NULL;
    const int nW = MultiByteToWideChar(CP_ACP, 0, szGbk, -1, NULL, 0);
    if (nW <= 1) return NULL;
    LPWSTR pW = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (SIZE_T)nW);
    if (pW == NULL) return NULL;
    if (MultiByteToWideChar(CP_ACP, 0, szGbk, -1, pW, nW) <= 0) { LocalFree(pW); return NULL; }
    return pW;
}

// W2M(UTF-16 字节集, #CP_ACP)：按字符数转回 GBK 写进 pOut
static void HWinW2M(CxBuf* pOut, LPCWSTR pW, int nChars)
{
    if (pOut == NULL || pW == NULL || nChars <= 0) return;
    while (nChars > 0 && pW[nChars - 1] == 0) nChars--;                          // 去掉尾部 NUL
    if (nChars <= 0) return;
    const int nA = WideCharToMultiByte(CP_ACP, 0, pW, nChars, NULL, 0, NULL, NULL);
    if (nA <= 0) return;
    LPSTR pA = (LPSTR)LocalAlloc(LPTR, (SIZE_T)nA + 1);
    if (pA == NULL) return;
    if (WideCharToMultiByte(CP_ACP, 0, pW, nChars, pA, nA, NULL, NULL) > 0) BufAppend(pOut, pA, nA);
    LocalFree(pA);
}

// 到数值(文本)：只认开头的数字（可带符号），等价于原模块解析 Content-Length 的那一步
static int HWinToInt(LPCSTR sz)
{
    if (sz == NULL) return 0;
    while (*sz == ' ' || *sz == '\t' || *sz == '\r' || *sz == '\n') sz++;
    int nSign = 1;
    if (*sz == '-') { nSign = -1; sz++; }
    else if (*sz == '+') sz++;
    INT64 v = 0;
    while (*sz >= '0' && *sz <= '9')
    {
        v = v * 10 + (*sz - '0');
        if (v > 0x40000000) break;
        sz++;
    }
    if (v > 0x7FFFFFFF) v = 0x7FFFFFFF;
    return (int)(v * nSign);
}

// 网页_取页面地址（原模块 通用模块/WinHTTP专用 两套实现都是「域名之后的整段，含 ?query」）。
// 不用 UxPageOf：它按规格_全局命令 P15 实现成「? 之前」，会把查询串丢掉。
static void HWinObjectOf(CxBuf* pOut, LPCSTR szUrl)
{
    if (pOut == NULL || szUrl == NULL) return;
    LPCSTR pHost = szUrl;
    LPCSTR pSep = strstr(szUrl, "://");
    if (pSep != NULL) pHost = pSep + 3;
    LPCSTR pSlash = strchr(pHost, '/');
    if (pSlash == NULL) BufAppend(pOut, "/", 1);                                 // 原模块：取不到就 "/"
    else BufAppend(pOut, pSlash, lstrlenA(pSlash));
}

// 计算每次接收块大小(连接耗时ms) —— 判断链的短路顺序照原模块（规格坑 24：区间重叠勿重排）
static int HWinChunk(int nMs)
{
    if (nMs <= 50) return 20240;
    if (nMs <= 500 && nMs > 50) return 10240;
    if (nMs <= 1000 && nMs > 300) return 5120;
    if (nMs <= 2000 && nMs > 1000) return 2048;
    if (nMs <= 5000 && nMs > 2000) return 1024;
    return 128;
}

// 参考 文本型 出参写回（返回头）：直接用 cxhttp_core.inc 的 HSetRefText，
// 它对 union 槽、旧文本指针都做了可读/可写校验，并且先 MFree 旧值再换指针（lib2.h 的硬要求）。
static void HWinRefText(PMDATA_INF pArg, LPCSTR szText)
{
    if (pArg == NULL || pArg->m_dtDataType == _SDT_NULL) return;
    HSetRefText(pArg, szText == NULL ? "" : szText);
}

// 字节集返回值：零长度时 m_pBin 留 NULL，就是易语言的 { }（库内既有约定），不留悬空指针
static void HWinRetBin(PMDATA_INF pRet, LPCSTR pData, int nLen)
{
    if (pRet == NULL) return;
    pRet->m_dtDataType = SDT_BIN;
    pRet->m_pBin = NULL;
    if (pData == NULL || nLen <= 0) return;
    pRet->m_pBin = CloneBinData(pData, nLen);
}

// 回调子程序（原模块 调用子程序）：地址先验可读再按 cdecl 调；
// WinHTTP 类的回调声明 5 个整数参（1=HTTP状态_ 2=状态码/总大小 3=已接收/保留 4,5=保留），
// WinInet 类的回调声明 3 个整数参（1=HTTP状态_ 2=总大小 3=按状态数值）。
static void HWinCb5(INT nFn, int nState, int n1, int n2, int n3, int n4)
{
    if (nFn == 0 || IsBadReadPtr((LPCVOID)nFn, 4)) return;                       // 地址不可读就跳过回调
    int (__cdecl* pFn)(int, int, int, int, int) = (int (__cdecl*)(int, int, int, int, int))(INT_PTR)nFn;
    pFn(nState, n1, n2, n3, n4);
}
static void HWinCb3(INT nFn, int nState, int n1, int n2)
{
    if (nFn == 0 || IsBadReadPtr((LPCVOID)nFn, 4)) return;
    int (__cdecl* pFn)(int, int, int) = (int (__cdecl*)(int, int, int))(INT_PTR)nFn;
    pFn(nState, n1, n2);
}

// 设置COOKIE（两个访问类逐字同 Curl 版，但没有 CURL≤0 守卫，永远生效）
static void HWinSetCookieState(CxHttpObj* p, LPCSTR szCookie, BOOL bSysDefault, BOOL bAutoUpdate)
{
    if (p == NULL) return;
    p->nFlags = (p->nFlags & ~CXH_F_AUTO_COOKIE) | (bAutoUpdate ? CXH_F_AUTO_COOKIE : 0);
    p->nFlags = (p->nFlags & ~CXH_F_SYS_COOKIE) | (bSysDefault ? CXH_F_SYS_COOKIE : 0);
    if (!bSysDefault)
    {
        p->nFlags |= CXH_F_CUSTOM_COOKIE;                                        // 空串也算「自定义空COOKIE」
        HSetStr(&p->szCookie, szCookie);
    }
    else p->nFlags &= ~CXH_F_CUSTOM_COOKIE;                                      // 用默认时不改 提交Cookies（坑 25）
}

// 网页_取Cookie_内部：开了系统默认 + 没自定义 + 当前没Cookie 才去取 IE 默认（WinHTTP/Curl 路线口径）
static void HWinPickSysCookie(CxHttpObj* p, LPCSTR szUrl)
{
    if (p == NULL || (p->nFlags & CXH_F_SYS_COOKIE) == 0) return;
    if (HGetStr(p->szCookie)[0] != '\0') return;
    if ((p->nFlags & CXH_F_CUSTOM_COOKIE) != 0) return;
    CxBuf host, ck;
    BufInit(&host, 64);
    BufInit(&ck, 256);
    UxHostOf(&host, szUrl);
    UxCookieFromUrl(&ck, host.nLen == 0 ? "" : host.pData, NULL, HWinFindNi(szUrl, "https://") != NULL);
    HSetStr(&p->szCookie, ck.nLen == 0 ? "" : ck.pData);
    BufFree(&ck); BufFree(&host);
}

// 取 UA：在 附加协议头 里找 "User-Agent:"（不区分大小写），取不到用 #HTTP配置_默认浏览器UA 的值
static void HWinUaOf(CxBuf* pOut, LPCSTR szExtraHead)
{
    if (pOut == NULL) return;
    if (szExtraHead != NULL && HWinFindNi(szExtraHead, "User-Agent: ") != NULL)
    {
        CxBuf raw, val;
        BufInit(&raw, 128);
        BufInit(&val, 128);
        HWinBetween(&raw, szExtraHead, "User-Agent:", "\n", TRUE);
        HWinTrim(&val, raw.nLen > 0 ? raw.pData : "");
        if (val.nLen > 0) BufAppend(pOut, val.pData, val.nLen);
        BufFree(&val); BufFree(&raw);
    }
    if (pOut->nLen == 0) HWinCopy(pOut, s_szCxDefaultUa);
}

// 到数值(删首尾空(文本_取出中间文本(返回头,"Content-Length:",#换行符,,假)))
static int HWinContentLen(LPCSTR szHead)
{
    CxBuf one, trim;
    BufInit(&one, 32);
    BufInit(&trim, 32);
    if (szHead != NULL) HWinBetween(&one, szHead, "Content-Length:", "\n", FALSE);
    HWinTrim(&trim, one.nLen > 0 ? one.pData : "");
    const int n = HWinToInt(trim.nLen > 0 ? trim.pData : "");
    BufFree(&trim); BufFree(&one);
    return n < 0 ? 0 : n;
}

// 超时秒 → 毫秒：只在这一处换算（原 WinInet 的 301 递归把毫秒又×1000，坑 20）
static int HWinToMs(int nSec)
{
    if (nSec <= 0) nSec = 60;                                                    // 超时 省略→60
    if (nSec > 2147483) nSec = 2147483;
    return nSec * 1000;
}

// 关闭句柄：统一 请求→连接→会话（修坑 12：WinHTTP 原是 连接→会话→请求、WinInet 原是 会话→连接→请求）
static void HWinHttpCloseAll(CxHttpObj* p)
{
    if (p == NULL || g_cxWinHttp.CloseHandle == NULL) return;
    HWinAsyncCloseRequest(p, true);
    if (p->hConnect != NULL) { g_cxWinHttp.CloseHandle(p->hConnect); p->hConnect = NULL; }
    if (p->hSession != NULL) { g_cxWinHttp.CloseHandle(p->hSession); p->hSession = NULL; }
    HWinAsyncFree(p);
}
static void HWinInetCloseAll(CxHttpObj* p)
{
    if (p == NULL || g_cxWinInet.CloseHandle == NULL) return;
    HWinAsyncCloseRequest(p, false);
    if (p->hConnect != NULL) { g_cxWinInet.CloseHandle(p->hConnect); p->hConnect = NULL; }
    if (p->hSession != NULL) { g_cxWinInet.CloseHandle(p->hSession); p->hSession = NULL; }
    HWinAsyncFree(p);
}

// 一次访问的全部入参（两条路线共用一份，各自取用；WinInet 路线不读 网站登录 两项）
struct CxWinAccessArg
{
    LPCSTR     szUrl;
    LPCSTR     szVerb;
    INT        nCbFn;
    LPCSTR     pPost;
    int        nPostLen;
    int        nTimeoutSec;
    LPCSTR     szExtraHead;
    PMDATA_INF pHeadRef;
    BOOL       bNoRedirect;
    LPCSTR     szSiteUser;
    LPCSTR     szSitePass;
    LPCSTR     szProxy;
    LPCSTR     szProxyUser;
    LPCSTR     szProxyPass;
    BOOL       bGzip;
};

static void HWinArgClear(CxWinAccessArg* pA)
{
    pA->szUrl = ""; pA->szVerb = ""; pA->nCbFn = 0;
    pA->pPost = NULL; pA->nPostLen = 0; pA->nTimeoutSec = 60;
    pA->szExtraHead = ""; pA->pHeadRef = NULL; pA->bNoRedirect = FALSE;
    pA->szSiteUser = ""; pA->szSitePass = "";
    pA->szProxy = ""; pA->szProxyUser = ""; pA->szProxyPass = "";
    pA->bGzip = TRUE;
}

// ============================================================ 访问网页_WinHTTP
// 类 HTTP访问_WinHTTP访问类 的 访问网页_WinHTTP：正文写进 pBody，返回头按 参考 回写。
static void HWinHttpAccess(CxHttpObj* p, const CxWinAccessArg* pArg, CxBuf* pBody)
{
    p->nLastError = 0;
    if (HStopRequested(p)) { HWinRefText(pArg->pHeadRef, ""); return; }
    if (!HWinHttpLoad()) { ReportError(g_cxWinHttp.szErr); return; }

    CxBuf verb, ua, proxy, url, host, page, head, retHead;
    BufInit(&verb, 16); BufInit(&ua, 128); BufInit(&proxy, 64); BufInit(&url, 128);
    BufInit(&host, 64); BufInit(&page, 128); BufInit(&head, 512); BufInit(&retHead, 1024);
    LPWSTR pHeadW = NULL;
    int nFail = 0, nFailA = 0, nFailB = 0;                                       // nFail=0 表示没走失败出口
    const int nMs = HWinToMs(pArg->nTimeoutSec);

    do
    {
        // 1) 访问方式：省略时按 提交信息 自动定 GET/POST（这条路线与 Curl 路线不同，照原样保留）
        if (pArg->szVerb == NULL || pArg->szVerb[0] == '\0') HWinCopy(&verb, pArg->nPostLen > 0 ? "POST" : "GET");
        else HWinUpper(&verb, pArg->szVerb);
        if (verb.nLen == 0) HWinCopy(&verb, "GET");
        const BOOL bPost = (lstrcmpA(verb.pData, "POST") == 0);
        const int nContentLen = bPost ? pArg->nPostLen : 0;    // 原模块复用 请求标记，这里拆成两个（坑 15）

        // 2) IE 默认 COOKIE
        HWinPickSysCookie(p, pArg->szUrl);

        // 3) 上次残留句柄先清（原模块 内部销毁方法）
        HWinHttpCloseAll(p);
        if (HStopRequested(p) || !HWinAsyncInit(p)) break;

        // 4) 平台检查：WinHttpCheckPlatform 在新 SDK 已废弃，入口缺失就当可用；返回假才中止
        if (g_cxWinHttp.CheckPlatform != NULL && g_cxWinHttp.CheckPlatform() == FALSE)
        {
            nFail = CWS_ERR_IFACE_UNAVAILABLE;
            break;
        }

        // 5) UA
        HWinUaOf(&ua, pArg->szExtraHead);

        // 6) 回调·建立会话 + 系统代理解析（'*' 先 网页_取系统代理EX；读不到仍留 '*' 走自动代理）
        //    原模块这次回调把静态 返回_整数型 塞进第 3 槽（坑 14），那是噪声参数，这里不传。
        HWinCb5(pArg->nCbFn, CWS_SESSION, 0, 0, 0, 0);
        BOOL bProxyFromSys = FALSE;
        if (pArg->szProxy != NULL && pArg->szProxy[0] == '*' && pArg->szProxy[1] == '\0')
        {
            UxSystemProxy(&proxy);
            if (proxy.nLen > 0) bProxyFromSys = TRUE;
            else HWinCopy(&proxy, "*");
        }
        else HWinCopy(&proxy, pArg->szProxy == NULL ? "" : pArg->szProxy);

        // 7) WinHttpOpen（访问类型依代理地址：'*'→自动、空→默认、其它→命名代理；末位 0 = 同步）
        LPWSTR pUaW = HWinM2W(ua.nLen > 0 ? ua.pData : "");
        LPWSTR pProxyW = NULL;
        DWORD dwAccess;
        if (proxy.nLen == 0) dwAccess = CWH_ACCESS_TYPE_DEFAULT_PROXY;
        else if (lstrcmpA(proxy.pData, "*") == 0) dwAccess = CWH_ACCESS_TYPE_AUTOMATIC_PROXY;
        else { dwAccess = CWH_ACCESS_TYPE_NAMED_PROXY; pProxyW = HWinM2W(proxy.pData); }
        p->hSession = g_cxWinHttp.Open(pUaW, dwAccess, pProxyW, NULL, p->pAsync ? 0x10000000 : 0);
        if (pUaW != NULL) LocalFree(pUaW);
        if (pProxyW != NULL) LocalFree(pProxyW);
        if (p->hSession == NULL) { nFail = CWS_ERR_MAKE_SESSION; break; }

        // 自动代理只在 Win8.1+ 真生效；从注册表读到代理时再显式压一份 WINHTTP_OPTION_PROXY
        if (bProxyFromSys && g_cxWinHttp.SetOption != NULL)
        {
            CWH_PROXY_INFO pi;
            pi.dwAccessType = CWH_ACCESS_TYPE_NAMED_PROXY;
            pi.lpszProxy = HWinM2W(proxy.pData);
            pi.lpszProxyBypass = NULL;
            if (pi.lpszProxy != NULL)
            {
                g_cxWinHttp.SetOption(p->hSession, CWH_OPTION_PROXY, &pi, (DWORD)sizeof(pi));
                LocalFree((HLOCAL)(INT_PTR)pi.lpszProxy);   // lpszProxy 是 LPCWSTR，须剥 const 再交给 LocalFree
            }
        }

        // 8) 网址拆分：没有协议头就补 http://；域名/页面地址/端口走 Ux* 工具
        if (HWinNCmp(pArg->szUrl, "http://", 7, TRUE) != 0 && HWinNCmp(pArg->szUrl, "https://", 8, TRUE) != 0)
            HWinCopy(&url, "http://");
        HWinCopy(&url, pArg->szUrl);
        const BOOL bHttps = (HWinNCmp(url.pData, "https://", 8, TRUE) == 0);
        UxHostOf(&host, url.pData);
        HWinObjectOf(&page, url.pData);
        const int nPort = UxPortOf(url.pData);

        // 9) WinHttpSetTimeouts（resolve/connect/send/receive 四段同值，单位毫秒）
        if (g_cxWinHttp.SetTimeouts != NULL)
            g_cxWinHttp.SetTimeouts(p->hSession, nMs, nMs, nMs, nMs);

        // 10) 回调·建立连接 + WinHttpConnect（域名 + 端口）
        HWinCb5(pArg->nCbFn, CWS_CONNECT, 0, 0, 0, 0);
        LPWSTR pHostW = HWinM2W(host.nLen > 0 ? host.pData : "");
        p->hConnect = pHostW == NULL ? NULL
            : g_cxWinHttp.Connect(p->hSession, pHostW,
                (DWORD)(nPort <= 0 ? (bHttps ? 443 : 80) : nPort), 0);
        if (pHostW != NULL) LocalFree(pHostW);
        if (p->hConnect == NULL) { nFail = CWS_ERR_MAKE_CONNECT; break; }

        // 11) 回调·建立请求 + WinHttpOpenRequest（动词用 访问方式 大写，https 加 WINHTTP_FLAG_SECURE）
        HWinCb5(pArg->nCbFn, CWS_REQUEST, 0, 0, 0, 0);
        LPWSTR pVerbW = HWinM2W(verb.pData);
        LPWSTR pPageW = HWinM2W(page.nLen > 0 ? page.pData : "/");
        p->hRequest = g_cxWinHttp.OpenRequest(p->hConnect, pVerbW, pPageW, NULL, NULL, NULL,
            bHttps ? CWH_FLAG_SECURE : 0);
        if (pVerbW != NULL) LocalFree(pVerbW);
        if (pPageW != NULL) LocalFree(pPageW);
        // 修坑 11：失败判据按 NULL（原模块用 <0，漏判返回 0 的情况，之后全在空句柄上跑）
        if (p->hRequest == NULL) { nFail = CWS_ERR_MAKE_REQUEST; break; }
        if (!HWinAsyncAttach(p, true)) break;

        // 12) 凭据：站点用 AUTH_TARGET_SERVER、代理用 AUTH_TARGET_PROXY，都只声明 BASIC。
        //     站点账号同时存进对象（szUser/szPass），下次访问不传也还在。
        if (g_cxWinHttp.SetCredentials != NULL)
        {
            if (pArg->szSiteUser != NULL && pArg->szSiteUser[0] != '\0')
            {
                HSetStr(&p->szUser, pArg->szSiteUser);
                HSetStr(&p->szPass, pArg->szSitePass);
                LPWSTR pU = HWinM2W(pArg->szSiteUser);
                LPWSTR pP = (pArg->szSitePass != NULL && pArg->szSitePass[0] != '\0')
                    ? HWinM2W(pArg->szSitePass) : NULL;
                g_cxWinHttp.SetCredentials(p->hRequest, CWH_AUTH_TARGET_SERVER, CWH_AUTH_SCHEME_BASIC, pU, pP, NULL);
                if (pU != NULL) LocalFree(pU);
                if (pP != NULL) LocalFree(pP);
            }
            if (pArg->szProxyUser != NULL && pArg->szProxyUser[0] != '\0')
            {
                LPWSTR pU = HWinM2W(pArg->szProxyUser);
                LPWSTR pP = (pArg->szProxyPass != NULL && pArg->szProxyPass[0] != '\0')
                    ? HWinM2W(pArg->szProxyPass) : NULL;
                g_cxWinHttp.SetCredentials(p->hRequest, CWH_AUTH_TARGET_PROXY, CWH_AUTH_SCHEME_BASIC, pU, pP, NULL);
                if (pU != NULL) LocalFree(pU);
                if (pP != NULL) LocalFree(pP);
            }
        }

        // 13) 重定向策略：禁止→WINHTTP_OPTION_REDIRECT_POLICY_NEVER，否则→ALWAYS
        if (g_cxWinHttp.SetOption != NULL)
        {
            DWORD dwPolicy = pArg->bNoRedirect ? CWH_REDIRECT_POLICY_NEVER : CWH_REDIRECT_POLICY_ALWAYS;
            g_cxWinHttp.SetOption(p->hRequest, CWH_OPTION_REDIRECT_POLICY, &dwPolicy, 4);
        }

        // 14) 协议头：网页_构建协议头 补全；整串存进 提交的协议头 供 取上次提交请求头
        UxBuildHeaders(&head, pArg->szExtraHead, url.pData, HGetStr(p->szCookie),
            bPost, pArg->bGzip, nContentLen);
        HSetStr(&p->szHeadExtra, head.nLen == 0 ? "" : head.pData);
        BufFree(&url); BufFree(&host); BufFree(&page); BufFree(&proxy); BufFree(&ua); BufFree(&verb);

        // 15) 回调·正在发送请求 + WinHttpAddRequestHeaders
        //     原模块把整串头交给 WinHttpSendRequest 的 pwszHeaders；这里显式走 WinHttpAddRequestHeaders
        //     （二者等价，且不会被塞两遍），该入口缺失时退回原写法。
        HWinCb5(pArg->nCbFn, CWS_SENDING, 0, 0, 0, 0);
        pHeadW = HWinM2W(head.nLen > 0 ? head.pData : "");
        BOOL bHeadAdded = FALSE;
        if (g_cxWinHttp.AddRequestHeaders != NULL && pHeadW != NULL)
            bHeadAdded = g_cxWinHttp.AddRequestHeaders(p->hRequest, pHeadW, (DWORD)-1,
                CWH_ADDREQ_FLAG_ADD | CWH_ADDREQ_FLAG_REPLACE);

        // 16) WinHttpSendRequest：失败重试 5 次（OptionalLength/TotalLength 都用内容长度，GET 时为 0）
        BOOL bSent = FALSE;
        for (int nTry = 1; nTry <= 5 && !HStopRequested(p); nTry++)
        {
            bSent = HWhSend(p, bHeadAdded ? NULL : pHeadW, 0,
                (bPost && pArg->pPost != NULL && pArg->nPostLen > 0) ? (LPVOID)pArg->pPost : NULL,
                (DWORD)nContentLen, (DWORD)nContentLen);
            if (bSent) break;
            HWinCb5(pArg->nCbFn, CWS_ERR_SEND_RETRY, 5, nTry, 0, 0);
            if (nTry >= 5) { nFail = CWS_ERR_SEND_FAILED; nFailA = 5; nFailB = nTry; break; }
        }
        if (nFail != 0 || HStopRequested(p)) break;

        // 响应失败后不能继续查询头或读取正文，异步等待负责取消与完成同步。
        HWinCb5(pArg->nCbFn, CWS_WAITING, 0, 0, 0, 0);
        {
            if (!HWhReceive(p)) { nFail = CWS_ERR_WAIT_TIMEOUT; break; }
        }

        if (HStopRequested(p)) break;

        // 18) WinHttpQueryHeaders 取响应头（UTF-16 → GBK，W2M(...,#CP_ACP)）
        HWinCb5(pArg->nCbFn, CWS_QUERY_HEAD, 0, 0, 0, 0);
        {
            DWORD nCap = CWH_HEAD_BUF;
            for (; !HStopRequested(p);)
            {
                LPWSTR pBufW = (LPWSTR)LocalAlloc(LPTR, nCap);
                if (pBufW == NULL) break;
                DWORD nLen = nCap;
                const BOOL bOk = g_cxWinHttp.QueryHeaders(p->hRequest, CWH_QUERY_RAW_HEADERS_CRLF,
                    NULL, pBufW, &nLen, NULL);
                if (bOk)
                {
                    pBufW[nCap / sizeof(WCHAR) - 1] = 0;                         // 保证有结束符
                    HWinW2M(&retHead, pBufW, (int)(nLen / sizeof(WCHAR)));
                }
                LocalFree(pBufW);
                if (bOk) break;
                // 修坑 13：固定 50000 字节会把超长头截断 → 按 API 回读的需求长度扩容再试
                if (nLen > nCap && nLen <= CWH_HEAD_BUF_MAX) { nCap = nLen; continue; }
                break;
            }
        }
        if (pHeadW != NULL) { LocalFree(pHeadW); pHeadW = NULL; }
        BufFree(&head);
        LPCSTR szHead = retHead.nLen > 0 ? retHead.pData : "";
        HWinRefText(pArg->pHeadRef, szHead);

        // 19) COOKIE：上次提交Cookies 回写 + 返回Cookies 合并
        //     WinHTTP 路线要求 提交Cookies≠"" 才合并（与 WinInet 路线的差异，照原模块勿统一）
        HSetStr(&p->szLastCookie, p->szCookie);
        {
            CxBuf retCk, merged;
            BufInit(&retCk, 128);
            BufInit(&merged, 128);
            UxCookieFromHead(&retCk, szHead);
            if (HGetStr(p->szCookie)[0] != '\0' && (p->nFlags & CXH_F_AUTO_COOKIE))
            {
                UxCookieMerge(&merged, p->szCookie, retCk.nLen == 0 ? "" : retCk.pData);
                BufReset(&retCk);
                BufAppend(&retCk, merged.pData, merged.nLen);
            }
            HSetStr(&p->szRetCookie, retCk.nLen == 0 ? "" : retCk.pData);
            BufFree(&merged); BufFree(&retCk);
        }

        // 20) Content-Length + 两个进度回调
        const int nTotalLen = HWinContentLen(szHead);
        HWinCb5(pArg->nCbFn, CWS_CONTENT_LEN, nTotalLen, 0, 0, 0);
        HWinCb5(pArg->nCbFn, CWS_BEGIN_DATA, nTotalLen, 0, 0, 0);

        // 21) WinHttpQueryDataAvailable + WinHttpReadData 循环（每次读 每次接收字节，块大小自适应）
        {
            int nEmpty = 0;
            for (; !HStopRequested(p);)
            {
                const DWORD tRound = GetTickCount();
                DWORD dwAvail = 0;
                if (HWhAvailable(p, &dwAvail) == FALSE) break;
                if (dwAvail < 1) break;
                int nChunk = p->nChunk > 0 ? p->nChunk : CWH_DEFAULT_CHUNK;
                if (nChunk > (1 << 20)) nChunk = 1 << 20;
                LPSTR pBuf = (LPSTR)LocalAlloc(LPTR, (SIZE_T)nChunk);
                if (pBuf == NULL) break;
                DWORD nRead = 0;
                const BOOL bRead = HWhRead(p, pBuf, (DWORD)nChunk, &nRead);
                if (bRead == FALSE || nRead == 0)
                {
                    // 原模块：连续读到空 5 次以上才发「接收不到数据」并跳出（已收到的正文照旧返回）
                    LocalFree(pBuf);
                    if (HStopRequested(p)) break;
                    if (nEmpty > 5)
                    {
                        HWinCb5(pArg->nCbFn, CWS_ERR_NO_DATA, nTotalLen,
                            pBody->pData == NULL ? 0 : (int)(INT_PTR)pBody->pData, 0, 0);
                        break;
                    }
                    nEmpty++;
                    continue;
                }
                nEmpty = 0;
                BufAppend(pBody, pBuf, (int)nRead);
                LocalFree(pBuf);
                HWinCb5(pArg->nCbFn, CWS_RECV_LEN, nTotalLen, pBody->nLen, 0, 0);
                HWinCb5(pArg->nCbFn, CWS_RECV_DATA, nTotalLen, (int)(INT_PTR)pBody->pData, 0, 0);
                p->nChunk = HWinChunk((int)(DWORD)(GetTickCount() - tRound));
            }
        }

        if (HStopRequested(p)) break;

        // 22) 修泄漏：读完就按 请求→连接→会话 关句柄（原模块要等下次访问或对象销毁才关）
        HWinHttpCloseAll(p);

        // 23) gzip：请求要了 gzip 或响应声明 gzip，且正文真是 1F 8B 08 才解；解不出来回退原始正文
        CxBuf ce;
        BufInit(&ce, 32);
        UxPickHeader(&ce, szHead, "Content-Encoding");
        const BOOL bWantGzip = pArg->bGzip || (ce.nLen > 0 && HWinFindNi(ce.pData, "gzip") != NULL);
        BufFree(&ce);
        if (bWantGzip && pBody->nLen > 3
            && (BYTE)pBody->pData[0] == 31 && (BYTE)pBody->pData[1] == 139 && (BYTE)pBody->pData[2] == 8)
        {
            int nOut = 0;
            LPBYTE pRaw = UxGzipInflate(pBody->pData, pBody->nLen, &nOut);
            if (pRaw != NULL && nOut > 0)
            {
                BufReset(pBody);
                BufAppend(pBody, (LPCSTR)pRaw, nOut);
                LocalFree(pRaw);                        // UxGzipInflate 给的是 LocalAlloc 缓冲，调用方归还
                BufFree(&retHead);
                HWinCb5(pArg->nCbFn, CWS_DONE, nTotalLen, (int)(INT_PTR)pBody->pData, 0, 0);
                return;
            }
            if (pRaw != NULL) LocalFree(pRaw);
        }
        BufFree(&retHead);
        HWinCb5(pArg->nCbFn, CWS_DONE, nTotalLen, (int)(INT_PTR)pBody->pData, 0, 0);
        return;
    } while (0);

    // 失败出口：关齐三条句柄 + 回调错误状态 + 正文留空（原模块中途返回不关句柄，靠下次访问回收）
    if (pHeadW != NULL) { LocalFree(pHeadW); pHeadW = NULL; }
    HWinHttpCloseAll(p);
    BufFree(&head); BufFree(&retHead);
    BufFree(&verb); BufFree(&ua); BufFree(&proxy); BufFree(&url); BufFree(&host); BufFree(&page);
    if (HStopRequested(p))
    {
        BufReset(pBody);
        HWinRefText(pArg->pHeadRef, "");
        return;
    }
    if (nFail != 0)
    {
        HWinCb5(pArg->nCbFn, nFail, nFailA, nFailB, 0, 0);
        BufReset(pBody);
    }
}

// 访问网页_WinInet 的单次访问（301 跳转由外层 HWinInetAccess 循环驱动）
// 返回 CWIN_RET_DONE / CWIN_RET_FAIL / CWIN_RET_301；301 时把下一跳网址写进 pOutNext。
static int HWinInetAccessOnce(CxHttpObj* p, const CxWinAccessArg* pArg, CxBuf* pBody, CxBuf* pOutNext)
{
    CxBuf verb, ua, proxy, url, host, page, head, retHead;
    BufInit(&verb, 16); BufInit(&ua, 128); BufInit(&proxy, 64); BufInit(&url, 128);
    BufInit(&host, 64); BufInit(&page, 128); BufInit(&head, 512); BufInit(&retHead, 1024);
    int nRet = CWIN_RET_DONE;
    int nFail = 0, nFailA = 0, nFailB = 0;
    const int nMs = HWinToMs(pArg->nTimeoutSec);                                 // 秒→毫秒只这一处

    do
    {
        // 1) 访问方式
        if (pArg->szVerb == NULL || pArg->szVerb[0] == '\0') HWinCopy(&verb, pArg->nPostLen > 0 ? "POST" : "GET");
        else HWinUpper(&verb, pArg->szVerb);
        if (verb.nLen == 0) HWinCopy(&verb, "GET");
        const BOOL bPost = (lstrcmpA(verb.pData, "POST") == 0);

        // 2) COOKIE 初值分支（与 Curl/WinHTTP 不同：不用系统默认时把 提交Cookies 清空）
        if (p->nFlags & CXH_F_SYS_COOKIE)
        {
            if (HGetStr(p->szCookie)[0] == '\0' && (p->nFlags & CXH_F_CUSTOM_COOKIE) == 0)
                HWinPickSysCookie(p, pArg->szUrl);
        }
        else if ((p->nFlags & CXH_F_CUSTOM_COOKIE) == 0) HSetStr(&p->szCookie, "");

        // 3) 内部销毁方法（先清上次残留句柄）
        HWinInetCloseAll(p);
        if (HStopRequested(p) || !HWinAsyncInit(p)) break;

        // 4) 回调·建立会话 + UA + 系统代理（'*' → 网页_取系统代理EX）
        HWinCb3(pArg->nCbFn, CWS_SESSION, 0, 0);
        HWinUaOf(&ua, pArg->szExtraHead);
        if (pArg->szProxy != NULL && pArg->szProxy[0] == '*' && pArg->szProxy[1] == '\0') UxSystemProxy(&proxy);
        else HWinCopy(&proxy, pArg->szProxy == NULL ? "" : pArg->szProxy);

        // 5) InternetOpenA：无代理传字面量 1（DIRECT，Cookie 容器保持默认开着）；
        //    有代理传 3（PROXY）+ "http=IP:端口"（原模块就是这两个字面量，坑 18 的双设照抄）
        CxBuf proxyArg;
        BufInit(&proxyArg, proxy.nLen + 8);
        if (proxy.nLen > 0) { BufAppend(&proxyArg, "http=", 5); BufAppend(&proxyArg, proxy.pData, proxy.nLen); }
        p->hSession = g_cxWinInet.OpenA(ua.nLen > 0 ? ua.pData : s_szCxDefaultUa,
            proxy.nLen == 0 ? CWI_OPEN_TYPE_DIRECT : CWI_OPEN_TYPE_PROXY,
            proxy.nLen == 0 ? NULL : proxyArg.pData, NULL, p->pAsync ? 0x10000000 : 0);
        BufFree(&proxyArg);
        if (p->hSession == NULL) { nFail = CWS_ERR_MAKE_SESSION; break; }

        // 6) 会话级超时三段（DWORD 毫秒）+ INTERNET_OPTION_PROXY
        if (g_cxWinInet.SetOptionA != NULL)
        {
            DWORD dwMs = (DWORD)nMs;
            g_cxWinInet.SetOptionA(p->hSession, CWI_OPTION_CONNECT_TIMEOUT, &dwMs, 4);
            g_cxWinInet.SetOptionA(p->hSession, CWI_OPTION_SEND_TIMEOUT, &dwMs, 4);
            g_cxWinInet.SetOptionA(p->hSession, CWI_OPTION_RECEIVE_TIMEOUT, &dwMs, 4);
            CWI_PROXY_INFO op;
            op.dwAccessType = proxy.nLen == 0 ? CWI_OPEN_TYPE_DIRECT : CWI_OPEN_TYPE_PROXY;
            op.lpszProxy = proxy.nLen == 0 ? (LPSTR)"" : proxy.pData;
            op.lpszProxyBypass = (LPSTR)"";
            g_cxWinInet.SetOptionA(p->hSession, CWI_OPTION_PROXY, &op, 12);
        }

        // 7) 回调·建立连接 + InternetConnectA（域名与端口分开取）
        HWinCb3(pArg->nCbFn, CWS_CONNECT, 0, 0);
        if (HWinNCmp(pArg->szUrl, "http://", 7, TRUE) != 0 && HWinNCmp(pArg->szUrl, "https://", 8, TRUE) != 0)
            HWinCopy(&url, "http://");
        HWinCopy(&url, pArg->szUrl);
        const BOOL bHttps = (HWinNCmp(url.pData, "https://", 8, TRUE) == 0);
        UxHostOf(&host, url.pData);
        const int nPort = UxPortOf(url.pData);
        p->hConnect = g_cxWinInet.ConnectA(p->hSession, host.nLen > 0 ? host.pData : "",
            (DWORD)(nPort <= 0 ? (bHttps ? 443 : 80) : nPort), NULL, NULL, CWI_SERVICE_HTTP, 0, 0);
        if (p->hConnect == NULL) { nFail = CWS_ERR_MAKE_CONNECT; break; }

        // 8) 回调·建立请求 + 请求标志：强制读源 + 第三方 Cookie；不用系统默认 Cookie 就禁 Cookie 引擎；
        //    禁止重定向靠 INTERNET_FLAG_NO_AUTO_REDIRECT；https 加 SECURE，否则允许 HTTP→HTTPS
        HWinCb3(pArg->nCbFn, CWS_REQUEST, 0, 0);
        DWORD dwFlags = CWI_FLAG_RELOAD | CWI_COOKIE_THIRD_PARTY;
        if ((p->nFlags & CXH_F_SYS_COOKIE) == 0) dwFlags |= CWI_FLAG_NO_COOKIES;
        if (pArg->bNoRedirect) dwFlags |= CWI_FLAG_NO_AUTO_REDIRECT;
        dwFlags |= bHttps ? CWI_FLAG_SECURE : CWI_FLAG_IGNORE_REDIRECT_TO_HTTPS;
        HWinObjectOf(&page, url.pData);
        p->hRequest = g_cxWinInet.OpenRequestA(p->hConnect, verb.pData,
            page.nLen > 0 ? page.pData : "/", "HTTP/1.1", NULL, NULL, dwFlags, (DWORD_PTR)p->pAsync);

        // 9) 请求级代理凭据（INTERNET_OPTION_PROXY_USERNAME/PASSWORD = 43/44）
        //    修坑 19：先判空句柄再设选项；凭据同时存进对象，句柄级选项跨次访问留存。
        if (p->hRequest != NULL && proxy.nLen > 0 && g_cxWinInet.SetOptionA != NULL)
        {
            if (pArg->szProxyUser != NULL && pArg->szProxyUser[0] != '\0') HSetStr(&p->szUser, pArg->szProxyUser);
            if (pArg->szProxyPass != NULL && pArg->szProxyPass[0] != '\0') HSetStr(&p->szPass, pArg->szProxyPass);
            LPCSTR szPu = HGetStr(p->szUser);
            LPCSTR szPp = HGetStr(p->szPass);
            g_cxWinInet.SetOptionA(p->hRequest, CWI_OPTION_PROXY_USERNAME, szPu, (DWORD)lstrlenA(szPu));
            g_cxWinInet.SetOptionA(p->hRequest, CWI_OPTION_PROXY_PASSWORD, szPp, (DWORD)lstrlenA(szPp));
        }
        if (p->hRequest == NULL) { nFail = CWS_ERR_MAKE_REQUEST; break; }
        if (!HWinAsyncAttach(p, false)) break;

        // 10) 协议头（GBK 直传，WinInet 路线全程 ANSI，无编码转换点）
        UxBuildHeaders(&head, pArg->szExtraHead, url.pData, HGetStr(p->szCookie),
            bPost, pArg->bGzip, pArg->nPostLen);
        HSetStr(&p->szHeadExtra, head.nLen == 0 ? "" : head.pData);

        // 11) 回调·正在发送请求 + HttpSendRequestA 重试 5 次（GET 不带正文，其它走 提交信息）
        HWinCb3(pArg->nCbFn, CWS_SENDING, 0, 0);
        BOOL bSent = FALSE;
        for (int nTry = 1; nTry <= 5 && !HStopRequested(p); nTry++)
        {
            const BOOL bGet = (lstrcmpA(verb.pData, "GET") == 0);
            bSent = HWiSend(p, head.nLen > 0 ? head.pData : "", (DWORD)head.nLen,
                (bGet || pArg->pPost == NULL) ? NULL : (LPCVOID)pArg->pPost,
                bGet ? 0 : (DWORD)pArg->nPostLen);
            if (bSent) break;
            HWinCb3(pArg->nCbFn, CWS_ERR_SEND_RETRY, 5, nTry);
            if (nTry >= 5) { nFail = CWS_ERR_SEND_FAILED; nFailA = 5; nFailB = nTry; break; }
        }
        if (nFail != 0 || HStopRequested(p)) break;

        // 12) 回调·等待响应 与 正在请求头信息（原模块连着发这两个）+ HttpQueryInfoA 自旋带超时
        HWinCb3(pArg->nCbFn, CWS_WAITING, 0, 0);
        HWinCb3(pArg->nCbFn, CWS_QUERY_HEAD, 0, 0);
        {
            const DWORD t0 = GetTickCount();
            DWORD nCap = CWI_HEAD_BUF;
            for (; !HStopRequested(p);)
            {
                LPSTR pBufA = (LPSTR)LocalAlloc(LPTR, nCap);
                if (pBufA == NULL) break;
                DWORD nLen = nCap;
                const BOOL bOk = g_cxWinInet.QueryInfoA(p->hRequest, CWI_QUERY_RAW_HEADERS_CRLF,
                    pBufA, &nLen, NULL);
                if (bOk)
                {
                    pBufA[nCap - 1] = '\0';                                      // 保证有结束符再取串
                    BufAppend(&retHead, pBufA, lstrlenA(pBufA));
                }
                LocalFree(pBufA);
                if (bOk) break;
                if (nLen > nCap && nLen <= CWH_HEAD_BUF_MAX) { nCap = nLen; continue; }   // 修坑 13：5000 太短会截头
                if ((DWORD)(GetTickCount() - t0) > (DWORD)nMs)
                {
                    HWinCb3(pArg->nCbFn, CWS_ERR_WAIT_TIMEOUT, 0, 0);
                    break;
                }
            }
        }
        LPCSTR szHead = retHead.nLen > 0 ? retHead.pData : "";
        HWinRefText(pArg->pHeadRef, szHead);

        // 13) COOKIE：本路线的合并只看 是否自动更新COOKIE，不要求 提交Cookies≠""（照原差异，勿统一）
        HSetStr(&p->szLastCookie, p->szCookie);
        {
            CxBuf retCk, merged;
            BufInit(&retCk, 128);
            BufInit(&merged, 128);
            UxCookieFromHead(&retCk, szHead);
            if (p->nFlags & CXH_F_AUTO_COOKIE)
            {
                UxCookieMerge(&merged, HGetStr(p->szCookie), retCk.nLen == 0 ? "" : retCk.pData);
                BufReset(&retCk);
                BufAppend(&retCk, merged.pData, merged.nLen);
            }
            HSetStr(&p->szRetCookie, retCk.nLen == 0 ? "" : retCk.pData);
            BufFree(&merged); BufFree(&retCk);
        }

        // 14) Content-Length + 两个进度回调
        const int nTotalLen = HWinContentLen(szHead);
        HWinCb3(pArg->nCbFn, CWS_CONTENT_LEN, nTotalLen, 0);
        HWinCb3(pArg->nCbFn, CWS_BEGIN_DATA, nTotalLen, 0);

        // 15) InternetReadFile 循环（每次读 每次接收字节，块大小自适应）
        BOOL bNoData = FALSE;
        for (; !HStopRequested(p);)
        {
            const DWORD tRound = GetTickCount();
            int nChunk = p->nChunk > 0 ? p->nChunk : CWH_DEFAULT_CHUNK;
            if (nChunk > (1 << 20)) nChunk = 1 << 20;
            LPSTR pBuf = (LPSTR)LocalAlloc(LPTR, (SIZE_T)nChunk);
            if (pBuf == NULL) break;
            DWORD nRead = 0;
            const BOOL bRead = HWiRead(p, pBuf, (DWORD)nChunk, &nRead);
            if (bRead == FALSE || nRead == 0)
            {
                LocalFree(pBuf);
                if (pBody->nLen == 0) bNoData = TRUE;            // 一个字节都没读到 → 接收不到数据
                break;
            }
            BufAppend(pBody, pBuf, (int)nRead);
            LocalFree(pBuf);
            HWinCb3(pArg->nCbFn, CWS_RECV_LEN, nTotalLen, pBody->nLen);
            HWinCb3(pArg->nCbFn, CWS_RECV_DATA, nTotalLen, (int)(INT_PTR)pBody->pData);
            p->nChunk = HWinChunk((int)(DWORD)(GetTickCount() - tRound));
        }

        if (HStopRequested(p)) break;

        // 16) 修泄漏：本条出口统一按 请求→连接→会话 关句柄
        HWinInetCloseAll(p);
        if (bNoData)
        {
            nFail = CWS_ERR_NO_DATA;
            nFailA = nTotalLen;
            nFailB = pBody->pData == NULL ? 0 : (int)(INT_PTR)pBody->pData;
            break;
        }

        // 17) 手动 301：原模块算完 局_重定向地址 就丢、递归不换网址、还把毫秒再×1000（坑 20）。
        //     这里真解析 Location 交给外层换网址重来，超时不重复缩放，禁止重定向时不跟。
        {
            CxBuf code;
            BufInit(&code, 8);
            UxStatusCode(&code, szHead);
            const BOOL b301 = (code.nLen == 3 && code.pData[0] == '3'
                && code.pData[1] == '0' && code.pData[2] == '1');
            BufFree(&code);
            if (b301 && !pArg->bNoRedirect)
            {
                CxBuf loc, locTrim;
                BufInit(&loc, 256);
                BufInit(&locTrim, 256);
                UxPickHeader(&loc, szHead, "Location");
                HWinTrim(&locTrim, loc.nLen > 0 ? loc.pData : "");
                BufFree(&loc);
                if (locTrim.nLen > 0)
                {
                    if (HWinNCmp(locTrim.pData, "http://", 7, TRUE) == 0
                        || HWinNCmp(locTrim.pData, "https://", 8, TRUE) == 0)
                    {
                        BufAppend(pOutNext, locTrim.pData, locTrim.nLen);        // 绝对地址：直接用
                    }
                    else
                    {
                        LPCSTR pScheme = bHttps ? "https://" : "http://";
                        BufAppend(pOutNext, pScheme, lstrlenA(pScheme));
                        LPCSTR pHostStart = strstr(url.pData, "://");
                        if (pHostStart != NULL)
                        {
                            pHostStart += 3;
                            LPCSTR pSlash = strchr(pHostStart, '/');
                            BufAppend(pOutNext, pHostStart, (int)(pSlash == NULL
                                ? lstrlenA(pHostStart) : (pSlash - pHostStart)));
                        }
                        else BufAppend(pOutNext, host.nLen > 0 ? host.pData : "", host.nLen);
                        if (locTrim.pData[0] != '/') BufAppend(pOutNext, "/", 1);
                        BufAppend(pOutNext, locTrim.pData, locTrim.nLen);
                    }
                    nRet = CWIN_RET_301;
                }
                BufFree(&locTrim);
            }
        }
        if (nRet != CWIN_RET_301)
        {
            // 18) 回调·处理完成（原模块在 gzip 之后还会再发一次同状态回调，照抄）
            HWinCb3(pArg->nCbFn, CWS_DONE, nTotalLen, (int)(INT_PTR)pBody->pData);

            // 19) gzip：同样先按魔术字节判；解压失败返回原始正文
            CxBuf ce;
            BufInit(&ce, 32);
            UxPickHeader(&ce, szHead, "Content-Encoding");
            const BOOL bWantGzip = pArg->bGzip || (ce.nLen > 0 && HWinFindNi(ce.pData, "gzip") != NULL);
            BufFree(&ce);
            if (bWantGzip && pBody->nLen > 3
                && (BYTE)pBody->pData[0] == 31 && (BYTE)pBody->pData[1] == 139 && (BYTE)pBody->pData[2] == 8)
            {
                int nOut = 0;
                LPBYTE pRaw = UxGzipInflate(pBody->pData, pBody->nLen, &nOut);
                if (pRaw != NULL && nOut > 0)
                {
                    BufReset(pBody);
                    BufAppend(pBody, (LPCSTR)pRaw, nOut);
                    LocalFree(pRaw);
                    HWinCb3(pArg->nCbFn, CWS_DONE, nTotalLen, (int)(INT_PTR)pBody->pData);
                }
                else if (pRaw != NULL) LocalFree(pRaw);                           // 解压失败：回原始正文
            }
        }
        BufFree(&retHead); BufFree(&head); BufFree(&verb); BufFree(&ua);
        BufFree(&proxy); BufFree(&url); BufFree(&host); BufFree(&page);
        return nRet;
    } while (0);

    // 失败出口：关句柄 + 回调 + 正文留空
    HWinInetCloseAll(p);
    BufFree(&retHead); BufFree(&head); BufFree(&verb); BufFree(&ua);
    BufFree(&proxy); BufFree(&url); BufFree(&host); BufFree(&page);
    if (HStopRequested(p))
    {
        BufReset(pBody);
        HWinRefText(pArg->pHeadRef, "");
        return CWIN_RET_FAIL;
    }
    if (nFail != 0)
    {
        HWinCb3(pArg->nCbFn, nFail, nFailA, nFailB);
        BufReset(pBody);
        nRet = CWIN_RET_FAIL;
    }
    return nRet;
}

// 访问网页_WinInet：带 Location 的有限次 301 循环（替代原模块不换网址的死循环递归）
static void HWinInetAccess(CxHttpObj* p, const CxWinAccessArg* pArg, CxBuf* pBody)
{
    p->nLastError = 0;
    if (HStopRequested(p)) { HWinRefText(pArg->pHeadRef, ""); return; }
    if (!HWinInetLoad()) { ReportError(g_cxWinInet.szErr); return; }

    CxBuf cur;
    BufInit(&cur, lstrlenA(pArg->szUrl) + 32);
    HWinCopy(&cur, pArg->szUrl);
    CxWinAccessArg arg = *pArg;
    arg.szUrl = cur.pData;

    for (int nHop = 0; nHop <= CWIN_REDIR_MAXHOP && !HStopRequested(p); nHop++)
    {
        CxBuf next;
        BufInit(&next, 256);
        const int nRet = HWinInetAccessOnce(p, &arg, pBody, &next);
        const bool bFollow = (!HStopRequested(p) && nRet == CWIN_RET_301 && next.nLen > 0 && nHop < CWIN_REDIR_MAXHOP);
        if (bFollow)
        {
            BufReset(pBody);                                     // 301 那一轮的正文不作为结果
            BufReset(&cur);
            BufAppend(&cur, next.pData, next.nLen);
            arg.szUrl = cur.pData;                               // cur 可能重分配过，重新指向
        }
        BufFree(&next);
        if (!bFollow) break;
    }
    // 最后一轮完成与停止可能同时发生；既已记录取消，就不能返回完整或部分正文。
    if (HStopRequested(p))
    {
        BufReset(pBody);
        HWinRefText(pArg->pHeadRef, "");
    }
    BufFree(&cur);
}

// ============================================================ 单入口派发
// 与 cxhttp_extra.inc 的 HExtra 同构：只认自己这些操作码，其余返回假交回核心派发。

// 取访问类对象：pArgInf[0] 的复合数据槽里是对象指针（可能为 NULL），
// 没建过就就地 HNew(nKind) 并写回该槽，同时更新 *ppObj。
static CxHttpObj* HWinSelf(PMDATA_INF pArgInf, CxHttpObj** ppObj, int nKind)
{
    if (ppObj != NULL && *ppObj != NULL) return *ppObj;
    if (pArgInf == NULL || pArgInf[0].m_pCompoundData == NULL) return NULL;
    CxHttpObj* p = (CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData);
    if (p != NULL) { if (ppObj != NULL) *ppObj = p; return p; }
    p = HNew(nKind);
    if (p == NULL) { ReportError("创建 HTTP访问 对象失败：内存不足"); return NULL; }
    *(PINT)pArgInf[0].m_pCompoundData = (INT)p;
    if (ppObj != NULL) *ppObj = p;
    return p;
}

static bool HAccessWinInvoke(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf,
    int nOp, int nKind, CxHttpObj** ppObj)
{
    // ---------------- 两条全局命令：没有对象自身，pArgInf[0] 就是 完整网址 ----------------
    if (nOp == HOP_G_WebAccessWinHttp || nOp == HOP_G_WebAccessWinInet)
    {
        const bool bHttp = (nOp == HOP_G_WebAccessWinHttp);
        CxHttpObj* pTmp = HNew(bHttp ? CXH_KIND_ACC_HTTP : CXH_KIND_ACC_INET);
        if (pTmp == NULL) { ReportError("创建 HTTP访问 对象失败：内存不足"); HWinRetBin(pRetData, NULL, 0); return true; }
        if (!HOptionalToken(pTmp, nArgCount, pArgInf, bHttp ? 16 : 14))
        {
            HFreeObj(pTmp);
            HWinRetBin(pRetData, NULL, 0);
            return true;
        }

        // 全局命令的 Cookie 分支：提交COOKIE 留空(未传) → 用系统默认COOKIE；传了 → 自定义；自动更新恒真
        const bool bCookieGiven = HWinGiven(pArgInf, nArgCount, 4);
        HWinSetCookieState(pTmp, bCookieGiven ? HWinTextAt(pArgInf, nArgCount, 4) : "",
            bCookieGiven ? FALSE : TRUE, TRUE);

        CxWinAccessArg a;
        HWinArgClear(&a);
        a.szUrl = HWinTextAt(pArgInf, nArgCount, 0);
        a.szVerb = HWinTextAt(pArgInf, nArgCount, 1);
        a.nCbFn = HWinIntAt(pArgInf, nArgCount, 2, 0);
        if (HWinGiven(pArgInf, nArgCount, 3)) { a.pPost = HWinBinPtr(pArgInf[3]); a.nPostLen = HWinBinLen(pArgInf[3]); }
        a.nTimeoutSec = HWinIntAt(pArgInf, nArgCount, 6, 60);
        a.szExtraHead = HWinTextAt(pArgInf, nArgCount, 7);
        a.pHeadRef = HWinRefAt(pArgInf, nArgCount, 8);
        a.bNoRedirect = HWinBoolAt(pArgInf, nArgCount, 9, FALSE);
        if (bHttp)
        {
            a.szSiteUser = HWinTextAt(pArgInf, nArgCount, 10);
            a.szSitePass = HWinTextAt(pArgInf, nArgCount, 11);
            a.szProxy = HWinTextAt(pArgInf, nArgCount, 12);
            a.szProxyUser = HWinTextAt(pArgInf, nArgCount, 13);
            a.szProxyPass = HWinTextAt(pArgInf, nArgCount, 14);
            a.bGzip = HWinBoolAt(pArgInf, nArgCount, 15, TRUE);
        }
        else
        {
            a.szProxy = HWinTextAt(pArgInf, nArgCount, 10);
            a.szProxyUser = HWinTextAt(pArgInf, nArgCount, 11);
            a.szProxyPass = HWinTextAt(pArgInf, nArgCount, 12);
            a.bGzip = HWinBoolAt(pArgInf, nArgCount, 13, TRUE);
        }

        CxBuf body;
        BufInit(&body, 4096);
        if (bHttp) HWinHttpAccess(pTmp, &a, &body);
        else HWinInetAccess(pTmp, &a, &body);
        HWinRetBin(pRetData, body.pData, body.nLen);
        BufFree(&body);
        // 返回COOKIE：原模块声明是 文本型, 可空（没有 参考），写不回调用方（规格第六节坑 10）——
        // 照抄该行为；要拿返回 COOKIE 请用 返回头 里的 Set-Cookie，或访问类对象的 获取当前返回的COOKIE()。
        HFreeObj(pTmp);                              // 临时对象：句柄已在访问里关过，这里只回收对象本身
        return true;
    }

    if (nKind != CXH_KIND_ACC_HTTP && nKind != CXH_KIND_ACC_INET) return false;
    switch (nOp)
    {
    case HOP_AccessWinHttp_GetLastHeader:
    case HOP_AccessWinHttp_SetRecvChunk:
    case HOP_AccessWinHttp_SetCookie:
    case HOP_AccessWinHttp_GetSubmitCookie:
    case HOP_AccessWinHttp_GetRetCookie:
    case HOP_AccessWinHttp_AccessPageWinHttp:
    case HOP_AccessWinInet_SetRecvChunk:
    case HOP_AccessWinInet_SetCookie:
    case HOP_AccessWinInet_GetSubmitCookie:
    case HOP_AccessWinInet_GetRetCookie:
    case HOP_AccessWinInet_AccessPageWinInet:
        break;
    default:
        return false;                                     // 不是本文件的命令，交回核心派发
    }

    CxHttpObj* p = HWinSelf(pArgInf, ppObj, nKind);
    const bool bTextRet = (nOp == HOP_AccessWinHttp_GetLastHeader || nOp == HOP_AccessWinHttp_GetSubmitCookie
        || nOp == HOP_AccessWinHttp_GetRetCookie || nOp == HOP_AccessWinInet_GetSubmitCookie
        || nOp == HOP_AccessWinInet_GetRetCookie);
    const bool bBinRet = (nOp == HOP_AccessWinHttp_AccessPageWinHttp || nOp == HOP_AccessWinInet_AccessPageWinInet);
    if (p == NULL)
    {
        if (bTextRet) SetRetText(pRetData, "");
        else if (bBinRet) HWinRetBin(pRetData, NULL, 0);
        else { pRetData->m_dtDataType = SDT_BOOL; pRetData->m_bool = BL_FALSE; }
        return true;
    }

    switch (nOp)
    {
    // ---------------- 类 HTTP访问_WinHTTP访问类 ----------------
    case HOP_AccessWinHttp_GetLastHeader:                                        // 取上次提交请求头
        SetRetText(pRetData, HGetStr(p->szHeadExtra));
        return true;
    case HOP_AccessWinHttp_SetRecvChunk:                                         // 设置每次接收长度（字节）
    {
        int n = pArgInf[1].m_int;
        if (n <= 0) n = CWH_DEFAULT_CHUNK;                     // #HTTP配置_默认每次接收数据长度
        p->nChunk = n;
        // 声明无返回值：返回槽一律不写，易语言按空处理
        return true;
    }
    case HOP_AccessWinHttp_SetCookie:                                            // 设置COOKIE
        HWinSetCookieState(p, HWinTextAt(pArgInf, nArgCount, 1),
            HWinBoolAt(pArgInf, nArgCount, 2, FALSE), HWinBoolAt(pArgInf, nArgCount, 3, TRUE));
        // 声明无返回值：返回槽一律不写
        return true;
    case HOP_AccessWinHttp_GetSubmitCookie:                                      // 获取当前提交的COOKIE
        SetRetText(pRetData, HGetStr(p->szLastCookie));
        return true;
    case HOP_AccessWinHttp_GetRetCookie:                                         // 获取当前返回的COOKIE
        SetRetText(pRetData, HGetStr(p->szRetCookie));
        return true;
    case HOP_AccessWinHttp_AccessPageWinHttp:                                    // 访问网页_WinHTTP
    {
        CxWinAccessArg a;
        HWinArgClear(&a);
        a.szUrl = HWinTextAt(pArgInf, nArgCount, 1);
        a.szVerb = HWinTextAt(pArgInf, nArgCount, 2);
        a.nCbFn = HWinIntAt(pArgInf, nArgCount, 3, 0);
        if (HWinGiven(pArgInf, nArgCount, 4)) { a.pPost = HWinBinPtr(pArgInf[4]); a.nPostLen = HWinBinLen(pArgInf[4]); }
        a.nTimeoutSec = HWinIntAt(pArgInf, nArgCount, 5, 60);
        a.szExtraHead = HWinTextAt(pArgInf, nArgCount, 6);
        a.pHeadRef = HWinRefAt(pArgInf, nArgCount, 7);
        a.bNoRedirect = HWinBoolAt(pArgInf, nArgCount, 8, FALSE);
        a.szSiteUser = HWinTextAt(pArgInf, nArgCount, 9);
        a.szSitePass = HWinTextAt(pArgInf, nArgCount, 10);
        a.szProxy = HWinTextAt(pArgInf, nArgCount, 11);
        a.szProxyUser = HWinTextAt(pArgInf, nArgCount, 12);
        a.szProxyPass = HWinTextAt(pArgInf, nArgCount, 13);
        a.bGzip = HWinBoolAt(pArgInf, nArgCount, 14, TRUE);
        CxBuf body;
        BufInit(&body, 4096);
        HWinHttpAccess(p, &a, &body);
        HWinRetBin(pRetData, body.pData, body.nLen);
        BufFree(&body);
        return true;
    }

    // ---------------- 类 HTTP访问_WinInet访问类 ----------------
    case HOP_AccessWinInet_SetRecvChunk:                                         // 设置每次接收长度（字节）
    {
        int n = pArgInf[1].m_int;
        if (n <= 0) n = CWH_DEFAULT_CHUNK;
        p->nChunk = n;
        // 声明无返回值：返回槽一律不写
        return true;
    }
    case HOP_AccessWinInet_SetCookie:                                            // 设置COOKIE
        HWinSetCookieState(p, HWinTextAt(pArgInf, nArgCount, 1),
            HWinBoolAt(pArgInf, nArgCount, 2, FALSE), HWinBoolAt(pArgInf, nArgCount, 3, TRUE));
        // 声明无返回值：返回槽一律不写
        return true;
    case HOP_AccessWinInet_GetSubmitCookie:                                      // 获取当前提交的COOKIE
        SetRetText(pRetData, HGetStr(p->szLastCookie));
        return true;
    case HOP_AccessWinInet_GetRetCookie:                                         // 获取当前返回的COOKIE
        SetRetText(pRetData, HGetStr(p->szRetCookie));
        return true;
    case HOP_AccessWinInet_AccessPageWinInet:                                    // 访问网页_WinInet
    {
        CxWinAccessArg a;
        HWinArgClear(&a);
        a.szUrl = HWinTextAt(pArgInf, nArgCount, 1);
        a.szVerb = HWinTextAt(pArgInf, nArgCount, 2);
        a.nCbFn = HWinIntAt(pArgInf, nArgCount, 3, 0);
        if (HWinGiven(pArgInf, nArgCount, 4)) { a.pPost = HWinBinPtr(pArgInf[4]); a.nPostLen = HWinBinLen(pArgInf[4]); }
        a.nTimeoutSec = HWinIntAt(pArgInf, nArgCount, 5, 60);
        a.szExtraHead = HWinTextAt(pArgInf, nArgCount, 6);
        a.pHeadRef = HWinRefAt(pArgInf, nArgCount, 7);
        a.bNoRedirect = HWinBoolAt(pArgInf, nArgCount, 8, FALSE);
        a.szProxy = HWinTextAt(pArgInf, nArgCount, 9);                    // 本类没有 网站登录用户名/密码
        a.szProxyUser = HWinTextAt(pArgInf, nArgCount, 10);
        a.szProxyPass = HWinTextAt(pArgInf, nArgCount, 11);
        a.bGzip = HWinBoolAt(pArgInf, nArgCount, 12, TRUE);
        CxBuf body;
        BufInit(&body, 4096);
        HWinInetAccess(p, &a, &body);
        HWinRetBin(pRetData, body.pData, body.nLen);
        BufFree(&body);
        return true;
    }
    default: break;
    }
    return false;
}

#endif  // CXHTTP_ACCESS_WIN_INC
