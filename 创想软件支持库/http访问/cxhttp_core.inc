// HTTP访问 族核心实现：原 zyCurl 模块 curl_easy / curl_http 两类的同名门面。
// 由 lib.cpp 在 namespace cx 内 #include，复用本库已加载的 libcurl-impersonate。
// 与 lib.cpp 里 2.0 的 浏览器会话/请求/响应 完全隔离：自己持有 easy 句柄与响应缓冲。
// 参数名/顺序/类型/可空/数组/参考/返回类型逐字照原源码（见 cxhttp_api.json）。
//
// 有意修掉原模块两处隐患（只在原实现出错时行为不同）：
//  1) 响应缓冲改为对象私有，克隆对象不会读到别人的正文；
//  2) 句柄为空时统一报运行时错误，不再直接调 curl 造成崩溃。
#ifndef CXHTTP_CORE_INC
#define CXHTTP_CORE_INC

// uni_core.cpp 提供：易语言内存池分配
void* UniMalloc(INT nSize);   // 注意是 INT，不是 SIZE_T

#define CURL_STATICLIB
#include <curl/curl.h>

// 运行期再解析的入口（不进 CurlApi，免得动 lib.cpp 的加载器）
typedef void* (*fn_duphandle_t)(void*);
typedef char* (*fn_escape_t)(void*, const char*, int);
typedef char* (*fn_unescape_t)(void*, const char*, int, int*);
typedef void (*fn_free_t)(void*);
typedef void* (*fn_multi_init_t)();
typedef int (*fn_multi_cleanup_t)(void*);
typedef int (*fn_multi_add_t)(void*, void*);
typedef int (*fn_multi_perform_t)(void*, int*);
typedef int (*fn_multi_wait_t)(void*, void*, unsigned int, int, int*);
typedef void* (*fn_multi_info_read_t)(void*, int*);
typedef int (*fn_multi_setopt_t)(void*, int, long);
#define H_PROC(n) ((void*)GetProcAddress(g_curl.hModule, n))

// 对象种类（cxhttp_api.json 里每个类的 对象 字段，生成器把它写成 CxHttpInvoke 的 nKind）
#define CXH_KIND_EASY      0      // curl_easy
#define CXH_KIND_HTTP      1      // curl_http（自动挂响应收集回调）
#define CXH_KIND_SMTP      2      // curl_smtp（easy + 邮件正文）
#define CXH_KIND_MULTI     3      // curl_multi（h 是 CURLM*）
#define CXH_KIND_URLPARAM  4      // URL参数类（纯文本）
#define CXH_KIND_ACC_CURL  5      // HTTP访问_Curl访问类
#define CXH_KIND_ACC_HTTP  6      // HTTP访问_WinHTTP访问类
#define CXH_KIND_ACC_INET  7      // HTTP访问_WinInet访问类

// 访问类对象的状态位（原模块的成员变量，逐条对应）
#define CXH_F_AUTO_COOKIE  1      // 是否自动更新COOKIE（默认开）
#define CXH_F_CUSTOM_COOKIE 2     // 当前是否为自定义设置COOKIE
#define CXH_F_SYS_COOKIE   4      // 是否初始使用系统默认COOKIE（默认关）

struct CxWinAsync;
struct CxHttpObj
{
    CX_CURL h;        // curl_easy 句柄；种类为 MULTI 时是 curl_multi 句柄
    CxBuf body;       // 响应正文
    CxBuf head;       // 响应头
    void* pSlist;     // 置请求头 建的 curl_slist 链
    void* pRcptList;  // 置邮件内容 建的收件人 curl_slist
    CxBuf post;       // 置POST数据 的副本：curl 要求实参在 执行 期间存活
    CxBuf mail;       // 置邮件内容 组好的 MIME 正文
    char* szUrl;      // URL参数类 的 内部网址
    void* pFn;        // 进度回调
    void* pUser;      // 进度回调用户数据
    int nHttp;        // 对象种类（CXH_KIND_*）
    // ---- 以下三个访问类（Curl / WinHTTP / WinInet）共用 ----
    char* szCookie;       // 提交Cookies
    char* szLastCookie;   // 上次提交Cookies
    char* szRetCookie;    // 返回Cookies
    char* szHeadExtra;    // 提交的协议头 / 附加协议头缓存
    char* szUser;         // 代理或站点用户名（WinInet 设句柄级选项要留存）
    char* szPass;         // 对应密码
    void* hSession;       // WinHttpOpen / InternetOpen 句柄
    void* hConnect;       // WinHttpConnect / InternetConnect 句柄
    void* hRequest;       // WinHttpOpenRequest / InternetOpenUrl 句柄
    int nChunk;           // 每次接收字节（默认 #HTTP配置_默认每次接收数据长度）
    int nFlags;           // CXH_F_*
    volatile LONG* pStop; // 借用调用方停止变量，只有 Interlocked 读写
    CxCancelState* pToken; // 绑定共享状态并持有引用，不借用易语言变量地址。
    int nLastError;       // 0=成功，1223=调用方取消，其余为后端错误码
    CxWinAsync* pAsync;   // 只在请求线程创建/关闭；系统回调不访问对象
    CRITICAL_SECTION accessCs; // 访问类方法按实例串行，避免绑定与请求同时修改状态
    bool requestActive;   // 拒绝回调内重入请求或改绑停止变量
};

// 访问类对象的字符串成员统一走这条：LocalAlloc，随对象一起释放
static void HSetStr(char** ppDst, LPCSTR szSrc)
{
    if (*ppDst != NULL) { LocalFree(*ppDst); *ppDst = NULL; }
    const int n = szSrc == NULL ? 0 : lstrlenA(szSrc);
    *ppDst = (char*)LocalAlloc(LPTR, n + 1);
    if (*ppDst == NULL) return;
    if (n > 0) memcpy(*ppDst, szSrc, n);
    (*ppDst)[n] = '\0';
}
static LPCSTR HGetStr(LPCSTR sz) { return sz == NULL ? "" : sz; }

// curl_multi / URL参数类 不吃 easy 句柄，实现体在 cxhttp_extra.inc
static bool HExtra(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf, int nOp, int nKind, CxHttpObj** ppObj);
// 三个访问类（HTTP访问_Curl访问类 / _WinHTTP访问类 / _WinInet访问类）：
// 实现体分别在 cxhttp_access_curl.inc 与 cxhttp_access_win.inc
static bool HAccessInvoke(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf, int nOp, int nKind, CxHttpObj** ppObj);
// 本族的全局命令（原模块的公开子程序）：生成器给的桩一律转到这里，见 cxhttp_glob.inc
void CxHttpGlobalInvoke(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf, int nOp);

static size_t CXH_COLLECT_CB(char* ptr, size_t size, size_t n, void* user)
{
    const size_t bytes = size * n;
    if (user != NULL && bytes > 0) BufAppend((CxBuf*)user, ptr, (int)bytes);
    return bytes;
}

static CxHttpObj* HNew(int nKind)
{
    CxHttpObj* p = (CxHttpObj*)LocalAlloc(LPTR, sizeof(CxHttpObj));
    if (p == NULL) return NULL;
    p->nHttp = nKind;
    InitializeCriticalSection(&p->accessCs);
    BufInit(&p->body, 4096);
    BufInit(&p->head, 1024);
    BufInit(&p->post, 1024);
    BufInit(&p->mail, 2048);
    p->nChunk = 512;                       // #HTTP配置_默认每次接收数据长度
    p->nFlags = CXH_F_AUTO_COOKIE;         // 是否自动更新COOKIE 默认开；系统默认COOKIE 默认关
    return p;
}

#include "cxhttp_cancel.inc"

static void HFreeObj(CxHttpObj* p)
{
    if (p == NULL) return;
    if (p->h != NULL)
    {
        if (p->nHttp == CXH_KIND_MULTI)
        {
            fn_multi_cleanup_t f = (fn_multi_cleanup_t)H_PROC("curl_multi_cleanup");
            if (f != NULL) f(p->h);
        }
        else if (g_curl.EasyCleanup != NULL) g_curl.EasyCleanup(p->h);
    }
    if (p->pSlist != NULL && g_curl.SlistFreeAll != NULL) g_curl.SlistFreeAll((CxSlist*)p->pSlist);
    if (p->pRcptList != NULL && g_curl.SlistFreeAll != NULL) g_curl.SlistFreeAll((CxSlist*)p->pRcptList);
    BufFree(&p->body);
    BufFree(&p->head);
    BufFree(&p->post);
    BufFree(&p->mail);
    if (p->szUrl != NULL) LocalFree(p->szUrl);
    // 访问类对象还带着 Cookie/协议头/账号 这几串文本
    if (p->szCookie != NULL) LocalFree(p->szCookie);
    if (p->szLastCookie != NULL) LocalFree(p->szLastCookie);
    if (p->szRetCookie != NULL) LocalFree(p->szRetCookie);
    if (p->szHeadExtra != NULL) LocalFree(p->szHeadExtra);
    if (p->szUser != NULL) LocalFree(p->szUser);
    if (p->szPass != NULL) LocalFree(p->szPass);
    CancelRelease(p->pToken);
    DeleteCriticalSection(&p->accessCs);
    LocalFree(p);
}

// 复制函数：只复制配置与句柄，响应缓冲各自新建（原模块共享静态缓冲是隐患）
static void HCopyObj(PMDATA_INF pArgInf, int nKind)
{
    if (pArgInf[0].m_pCompoundData == pArgInf[1].m_pCompoundData) return;
    // SDK复制参数为[未初始化目标,源]，访问类配置及令牌从源复制。
    CxHttpObj* pSrc = (CxHttpObj*)(*(PINT)pArgInf[1].m_pCompoundData);
    CxHttpObj* pDst = HNew(nKind);
    if (pDst == NULL) return;
    if (pSrc != NULL)
    {
        if (nKind == CXH_KIND_URLPARAM)
        {
            const int n = pSrc->szUrl == NULL ? 0 : lstrlenA(pSrc->szUrl);
            pDst->szUrl = (char*)LocalAlloc(LPTR, n + 1);
            if (pDst->szUrl != NULL && n > 0) memcpy(pDst->szUrl, pSrc->szUrl, n);
        }
        else if (nKind == CXH_KIND_MULTI && pSrc->h != NULL)
        {
            fn_multi_init_t init = (fn_multi_init_t)H_PROC("curl_multi_init");
            pDst->h = init ? init() : NULL; // multi不能共享所有权，否则任一副本析构会释放另一副本的句柄。
        }
        else if (nKind >= CXH_KIND_ACC_CURL)
        {
            HAccessLock lock(pSrc);
            // 访问类：Cookie/协议头/账号 这些状态跟着一起复制，句柄不带走（每次访问各自建链）
            HSetStr(&pDst->szCookie, pSrc->szCookie);
            HSetStr(&pDst->szLastCookie, pSrc->szLastCookie);
            HSetStr(&pDst->szRetCookie, pSrc->szRetCookie);
            HSetStr(&pDst->szHeadExtra, pSrc->szHeadExtra);
            HSetStr(&pDst->szUser, pSrc->szUser);
            HSetStr(&pDst->szPass, pSrc->szPass);
            pDst->nChunk = pSrc->nChunk;
            pDst->nFlags = pSrc->nFlags;
            pDst->pStop = pSrc->pStop;
            pDst->pToken = pSrc->pToken;
            CancelRetain(pDst->pToken);
        }
        else if (pSrc->h != NULL)
        {
            fn_duphandle_t f = (fn_duphandle_t)H_PROC("curl_easy_duphandle");
            pDst->h = f == NULL ? NULL : f(pSrc->h);
            if (pDst->h == NULL) ReportError("HTTP访问：复制curl句柄失败");
        }
    }
    *(PINT)pArgInf[0].m_pCompoundData = (INT)pDst;
}

static CxHttpObj* HSelf(PMDATA_INF pArgInf)
{
    CxHttpObj* p = (CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData);
    if (p == NULL) ReportError("HTTP访问对象未初始化：请先调用 创建()");
    return p;
}

// 挂响应收集回调（curl_http）
static void HBind(CxHttpObj* p)
{
    BufReset(&p->body);
    BufReset(&p->head);
    g_curl.EasySetopt(p->h, CURLOPT_WRITEFUNCTION, (CXH_COLLECT_CB));
    g_curl.EasySetopt(p->h, CURLOPT_WRITEDATA, &p->body);
    g_curl.EasySetopt(p->h, CURLOPT_HEADERFUNCTION, (CXH_COLLECT_CB));
    g_curl.EasySetopt(p->h, CURLOPT_HEADERDATA, &p->head);
}

// 取可用句柄；失败已报错并返回 NULL
static CX_CURL HOk(CxHttpObj* p)
{
    if (p == NULL) return NULL;
    if (p->h == NULL)
    {
        ReportError("curl 句柄无效：请先调用 创建()，或用 置句柄() 绑定句柄");
        return NULL;
    }
    return p->h;
}

// 指针型选项：文本/字节集两种实参都接
// 选项号合法性：0 是 CURLOPT_ERRORBUFFER（curl 会往里写），越界值会让 curl 按错宽度取参
#define H_OPT_OK(n)  ((n) > 0 && (n) < 100000)

static int HSetopt(CX_CURL h, int nOpt, const MDATA_INF& v)
{
    fn_easy_setopt_t so = g_curl.EasySetopt;
    // 选项号 0（CURLOPT_ERRORBUFFER）等一律拒收：原模块在这里会把实参当缓冲区让 curl 写，
    // 传 0 就是往空指针写错误文本直接崩；支持库不该给调用方这个自毁入口。
    if (nOpt <= 0 || nOpt >= 100000) return CURLE_UNKNOWN_OPTION;
    switch (nOpt)
    {
    case CURLOPT_URL: case CURLOPT_PROXY: case CURLOPT_USERPWD: case CURLOPT_PROXYUSERPWD:
    case CURLOPT_USERNAME: case CURLOPT_PASSWORD: case CURLOPT_COOKIE: case CURLOPT_COOKIEFILE:
    case CURLOPT_COOKIEJAR: case CURLOPT_REFERER: case CURLOPT_USERAGENT: case CURLOPT_CAINFO:
    case CURLOPT_ACCEPT_ENCODING: case CURLOPT_CUSTOMREQUEST: case CURLOPT_POSTFIELDS:
    case CURLOPT_RANGE: case CURLOPT_SSLCERT: case CURLOPT_SSLKEY: case CURLOPT_INTERFACE:
    case CURLOPT_KRBLEVEL: case CURLOPT_DNS_SERVERS:
        return so(h, nOpt, v.m_dtDataType == SDT_BIN ? (const void*)v.m_pBin : (const void*)v.m_pText);
    case CURLOPT_HTTPHEADER: case CURLOPT_RESOLVE: case CURLOPT_HTTP200ALIASES:
        return so(h, nOpt, (const void*)(curl_slist*)v.m_int);
    default:
        if (v.m_dtDataType == SDT_INT64) return so(h, nOpt, (curl_off_t)v.m_int64);
        if (v.m_dtDataType == SDT_TEXT) return so(h, nOpt, v.m_pText);
        if (v.m_dtDataType == SDT_BIN) return so(h, nOpt, v.m_pBin);
        return so(h, nOpt, v.m_int);
    }
}

// 取信息：0=长整数 1=双精度 2=文本。值直接落进 pOut 的 union，故不再回传错误码——
// 调用方若再写 pOut->m_int，会把刚分配的字符串指针覆盖成错误码（同一块 union）。
static void HGetinfo(CX_CURL h, int nInfo, PMDATA_INF pOut, int nMode)
{
    // CURLINFO_* 都带类型基址（0x100000 起），越界值会让 curl 按错宽度写栈
    if (nInfo < 0x100000 || nInfo > 0x700000)
    {
        // 非法信息码：按声明的返回类型给同型零值，不能改类型（否则调用方拿到的数据类型和声明不符）
        if (nMode == 1) { pOut->m_double = 0; }
        else if (nMode == 2) { SetRetText(pOut, ""); }
        else { pOut->m_int = 0; }
        return;
    }
    switch (nMode)
    {
    case 0: { long v = 0; g_curl.EasyGetinfo(h, nInfo, &v); pOut->m_int = (int)v; break; }
    case 1: { double v = 0; g_curl.EasyGetinfo(h, nInfo, &v); pOut->m_double = v; break; }
    default: { char* v = NULL; g_curl.EasyGetinfo(h, nInfo, &v); SetRetText(pOut, v == NULL ? "" : v); break; }
    }
}

// 把缓冲按分隔符切开发进 文本型 参考数组。
// 元素与数据块一律走易语言内存池（UniAryWrite 内部 UniMalloc / NRS_FREE_ARY），
// 用 LocalAlloc 会让易语言释放时跨堆崩。
static void HFillTextAry(const MDATA_INF& arg, CxBuf* pSrc, char chSep)
{
    UniAryList list;
    int nStart = 0;
    for (int i = 0; i <= pSrc->nLen; i++)
    {
        if (i != pSrc->nLen && pSrc->pData[i] != chSep) continue;
        const int nLen = i - nStart;
        char* p = (char*)UniMalloc(nLen + 1);
        if (p != NULL)
        {
            if (nLen > 0) memcpy(p, pSrc->pData + nStart, nLen);
            p[nLen] = '\0';
            list.Add(p);
        }
        nStart = i + 1;
    }
    UniAryWrite(arg, &list);
}

// 文本型数组 -> curl_slist，旧链一并释放
static curl_slist* HBuildSlist(CxHttpObj* p, const MDATA_INF& ary)
{
    if (p->pSlist != NULL && g_curl.SlistFreeAll != NULL)
    {
        g_curl.SlistFreeAll((CxSlist*)p->pSlist);
        p->pSlist = NULL;
    }
    curl_slist* pList = NULL;
    // 数组块 = [维数][个数][元素指针...]，个数在块头第二个 INT
    const int nRaw = ary.m_pAryData == NULL ? 0 : *(INT*)((LPBYTE)ary.m_pAryData + sizeof(INT));
    const int n = (nRaw < 0 || nRaw > 4096) ? 0 : nRaw;      // 块头被写坏时不越界
    if (g_curl.SlistAppend != NULL)
    {
        for (int i = 0; i < n; i++)
        {
            LPCSTR sz = ((LPCSTR*)ary.m_pAryData)[i];
            // 元素可能是调用方给的野指针：先验可读再取串，否则 lstrlenA 直接崩在库里
            if (sz == NULL || IsBadReadPtr(sz, 1) || sz[0] == '\0') continue;
            curl_slist* pNext = (curl_slist*)g_curl.SlistAppend((CxSlist*)pList, sz);
            if (pNext == NULL) break;
            pList = pNext;
        }
    }
    p->pSlist = pList;
    return pList;
}

// 「参考」（传址）出入参：union 里放的是调用方变量的地址，不是值本身。
// 写文本/字节集之前必须把旧值还给易语言内存池（lib2.h 对 m_ppText/m_ppBin 的硬性要求），
// 否则宿主按自己的分配器释放时会跨堆崩。
static LPCSTR HGetRefText(const MDATA_INF& a)
{
    // slot 本身或里面的文本指针都可能是调用方给的野指针
    if (a.m_ppText == NULL || IsBadReadPtr(a.m_ppText, sizeof(void*))) return "";
    if (*a.m_ppText == NULL || IsBadReadPtr(*a.m_ppText, 1)) return "";
    return *a.m_ppText;
}
static void HSetRefText(PMDATA_INF pArg, LPCSTR sz)
{
    // 出参标了「可空」：调用方整个省略时运行时给的数据类型是 _SDT_NULL，这时不写回
    if (pArg == NULL || pArg->m_dtDataType == _SDT_NULL) return;
    if (pArg->m_ppText == NULL || IsBadReadPtr(pArg->m_ppText, sizeof(void*))) return;
    if (*pArg->m_ppText != NULL && !IsBadWritePtr(*pArg->m_ppText, 1))
        NotifySys(NRS_MFREE, (DWORD)(INT_PTR)*pArg->m_ppText, 0);
    const int n = sz == NULL ? 0 : lstrlenA(sz);
    *pArg->m_ppText = CloneTextData(sz == NULL ? "" : sz, n);
}
static int HGetRefInt(const MDATA_INF& a)
{
    return (a.m_pInt == NULL || IsBadReadPtr(a.m_pInt, sizeof(INT))) ? 0 : *a.m_pInt;
}
static void HSetRefInt(PMDATA_INF pArg, INT v)
{
    if (pArg == NULL || pArg->m_pInt == NULL || pArg->m_dtDataType == _SDT_NULL) return;
    if (!IsBadWritePtr(pArg->m_pInt, sizeof(INT))) *pArg->m_pInt = v;
}
// 按值接收的字节集入参（不强制变量）：union 里直接是块指针，按块头核验可读范围
static LPCSTR HGetValueBin(const MDATA_INF& a, int* pLen)
{
    if (pLen != NULL) *pLen = 0;
    if (a.m_dtDataType != SDT_BIN) return NULL;
    const LPBYTE p = (const LPBYTE)a.m_pBin;
    if (p == NULL || IsBadReadPtr(p, sizeof(INT) * 2)) return NULL;
    const int n = *(INT*)(p + sizeof(INT));
    if (n < 0 || n > 0x08000000) return NULL;
    if (IsBadReadPtr(p, (SIZE_T)sizeof(INT) * 2 + (SIZE_T)n)) return NULL;
    if (pLen != NULL) *pLen = n;
    return (LPCSTR)(p + sizeof(INT) * 2);
}

// 可空实参：没传就按 0 / 空串处理
static int HIntAt(PMDATA_INF pArgInf, INT nArgCount, int i){
    return (i < nArgCount) ? pArgInf[i].m_int : 0;
}
static LPCSTR HTextAt(PMDATA_INF pArgInf, INT nArgCount, int i)
{
    if (i >= nArgCount) return "";
    LPCSTR p = pArgInf[i].m_pText;
    if (p == NULL) return "";
    if (!IsBadReadPtr(p, 1)) return p;                 // 普通文本实参
    // 传址文本：union 里装的是「变量地址」，真正的文本指针在那个变量里
    if (!IsBadReadPtr(p, sizeof(void*)))
    {
        void* q = *(void**)p;
        if (q != NULL && !IsBadReadPtr(q, 1)) return (LPCSTR)q;
    }
    return "";
}

// ============================================================ 单入口派发
void CxHttpInvoke(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf, int nOp, int nKind)
{
    if (!LoadCurlApi()) { ReportError(g_curl.LoadError); return; }

    CxHttpObj* p = (CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData);

    // curl_multi / URL参数类 不用 easy 句柄，走独立实现体
    if (HExtra(pRetData, nArgCount, pArgInf, nOp, nKind, &p)) return;
    // 三个访问类（Curl / WinHTTP / WinInet）自成一路，走完自己就不回来了
    if (nKind >= CXH_KIND_ACC_CURL)
    {
        if (p == NULL)
        {
            p = HNew(nKind);
            if (p == NULL) { ReportError("HTTP访问：创建对象失败"); return; }
            *(PINT)pArgInf[0].m_pCompoundData = (INT)p;
        }
        HAccessLock lock(p);
        if (p->requestActive) { ReportError("HTTP访问：请求期间不能重入访问类命令"); return; }
        const bool requesting = nOp == HOP_AccessCurl_AccessPageCurl
            || nOp == HOP_AccessWinHttp_AccessPageWinHttp || nOp == HOP_AccessWinInet_AccessPageWinInet;
        p->requestActive = requesting;
        const bool handled = HAccessInvoke(pRetData, nArgCount, pArgInf, nOp, nKind, &p);
        p->requestActive = false;
        if (handled) return;
        ReportError("HTTP访问：未知的访问类命令");
        return;
    }
    // ---- 生命周期与句柄：不要求句柄已存在 ----
    switch (nOp)
    {
    case HOP_CurlEasy_Create:
    {
        if (p == NULL)
        {
            p = HNew(nKind);
            if (p == NULL) { ReportError("创建 HTTP 对象失败：内存不足"); return; }
            *(PINT)pArgInf[0].m_pCompoundData = (INT)p;
        }
        if (p->h == NULL) p->h = g_curl.EasyInit();
        if (p->h == NULL) { ReportError("curl_easy_init 失败"); return; }
        if (p->nHttp != CXH_KIND_EASY) HBind(p);
        pRetData->m_bool = BL_TRUE;
        return;
    }
    case HOP_CurlEasy_Destroy:
        if (p != NULL) { HFreeObj(p); *(PINT)pArgInf[0].m_pCompoundData = 0; }
        pRetData->m_dtDataType = SDT_BOOL;      // 声明无返回值，也要把返回槽落成合法零值
        pRetData->m_bool = BL_FALSE;
        return;
    case HOP_CurlEasy_GetHandle:
        pRetData->m_int = p == NULL ? 0 : (INT)p->h;
        return;
    case HOP_CurlEasy_SetHandle:
    {
        if (p == NULL)
        {
            p = HNew(nKind);
            if (p == NULL) return;
            *(PINT)pArgInf[0].m_pCompoundData = (INT)p;
        }
        // 句柄是外部交进来的裸指针：不可读就拒收，否则后面每条命令都会在 curl 里解引用崩溃
        const INT nNew = pArgInf[1].m_int;
        if (nNew != 0 && IsBadReadPtr((LPCVOID)nNew, 8))
        {
            ReportError("置句柄：传入的句柄不可读，已拒收");
            return;
        }
        p->h = (CX_CURL)nNew;
        if (p->nHttp) HBind(p);
        return;
    }
    case HOP_CurlEasy_DupHandle:
    {
        CX_CURL h = HOk(p);
        if (h == NULL) return;
        fn_duphandle_t f = (fn_duphandle_t)H_PROC("curl_easy_duphandle");
        pRetData->m_int = f == NULL ? 0 : (INT)f(h);
        return;
    }
    case HOP_CurlEasy_Reset:
    {
        CX_CURL h = HOk(p);
        if (h == NULL) return;
        g_curl.EasyReset(h);
        p->pSlist = NULL;
        if (p->nHttp) HBind(p);
        return;
    }
    default: break;
    }

    CX_CURL h = HOk(p);
    if (h == NULL) return;
    fn_easy_setopt_t so = g_curl.EasySetopt;

    switch (nOp)
    {
    // ---- 通用 setopt / getinfo / perform / URL 编解码 ----
    case HOP_CurlEasy_SetoptInt:
        pRetData->m_int = HSetopt(h, pArgInf[1].m_int, pArgInf[2]); return;
    case HOP_CurlEasy_SetoptLong:
        if (!H_OPT_OK(pArgInf[1].m_int)) { pRetData->m_int = CURLE_UNKNOWN_OPTION; return; }
        // curl_off_t 型选项按值直传（cdecl 变参里 8 字节原样入栈）
        pRetData->m_int = so(h, pArgInf[1].m_int, (curl_off_t)pArgInf[2].m_int64); return;
    case HOP_CurlEasy_SetoptText:
        if (!H_OPT_OK(pArgInf[1].m_int)) { pRetData->m_int = CURLE_UNKNOWN_OPTION; return; }
        pRetData->m_int = so(h, pArgInf[1].m_int, pArgInf[2].m_pText); return;
    case HOP_CurlEasy_SetoptBin:
        if (!H_OPT_OK(pArgInf[1].m_int)) { pRetData->m_int = CURLE_UNKNOWN_OPTION; return; }
        pRetData->m_int = so(h, pArgInf[1].m_int, pArgInf[2].m_pBin); return;
    case HOP_CurlEasy_Perform:
        BufReset(&p->body); BufReset(&p->head);
        // 邮件正文可能在 置邮件内容 之后又扩容，指针要重新交给 curl
        if (p->nHttp == CXH_KIND_SMTP && p->mail.nLen > 0)
        {
            so(h, CURLOPT_POSTFIELDS, p->mail.pData);
            so(h, CURLOPT_POSTFIELDSIZE, (curl_off_t)p->mail.nLen);
        }
        pRetData->m_int = g_curl.EasyPerform(h); return;
    case HOP_CurlEasy_GetinfoInt:
        HGetinfo(h, pArgInf[1].m_int, pRetData, 0); return;
    case HOP_CurlEasy_GetinfoDouble:
        HGetinfo(h, pArgInf[1].m_int, pRetData, 1); return;
    case HOP_CurlEasy_GetinfoText:
        HGetinfo(h, pArgInf[1].m_int, pRetData, 2); return;
    case HOP_CurlEasy_GetinfoTextAry:
    {
        curl_slist* pList = NULL;
        const int r = g_curl.EasyGetinfo(h, pArgInf[1].m_int, &pList);
        BufReset(&p->head);
        for (curl_slist* q = pList; q != NULL; q = q->next)
        {
            if (q->data != NULL) BufAppend(&p->head, q->data, (int)lstrlenA(q->data));
            BufAppend(&p->head, "\n", 1);
        }
        HFillTextAry(pArgInf[2], &p->head, '\n');
        pRetData->m_int = r;
        return;
    }
    case HOP_CurlEasy_GetinfoCert:
    {
        so(h, CURLOPT_CERTINFO, 1L);
        struct curl_certinfo* ci = NULL;
        const int r = g_curl.EasyGetinfo(h, pArgInf[1].m_int, &ci);
        if (ci != NULL && pArgInf[2].m_pCompoundData != NULL)
        {
            // CURL_证书信息 = { 整数型 数量, 整数型 链表数组 }：数据块是裸整数
            LPINT pField = (LPINT)pArgInf[2].m_pCompoundData;
            pField[0] = ci->num_of_certs;
            pField[1] = (INT)(INT_PTR)ci->certinfo;
        }
        pRetData->m_int = r;
        return;
    }
    case HOP_CurlEasy_Escape:
    {
        fn_escape_t f = (fn_escape_t)H_PROC("curl_easy_escape");
        char* pOut = f == NULL ? NULL : f(h, pArgInf[1].m_pText, 0);
        SetRetText(pRetData, pOut);
        fn_free_t fr = (fn_free_t)H_PROC("curl_free");
        if (pOut != NULL && fr != NULL) fr(pOut);
        return;
    }
    case HOP_CurlEasy_Unescape:
    {
        fn_unescape_t f = (fn_unescape_t)H_PROC("curl_easy_unescape");
        int nLen = 0;
        char* pOut = f == NULL ? NULL : f(h, pArgInf[1].m_pText, 0, &nLen);
        SetRetText(pRetData, pOut);
        fn_free_t fr = (fn_free_t)H_PROC("curl_free");
        if (pOut != NULL && fr != NULL) fr(pOut);
        return;
    }

    // ---- curl_http 便捷设置 ----
    case HOP_CurlHttp_SetXferinfo: case HOP_CurlHttp_SetXferinfoAddr:
    {
        void* pFn = nOp == HOP_CurlHttp_SetXferinfo ? (void*)pArgInf[1].m_int : (void*)pArgInf[1].m_int;
        p->pFn = pFn;
        p->pUser = HIntAt(pArgInf, nArgCount, 2) ? (void*)pArgInf[2].m_int : NULL;
        so(h, CURLOPT_XFERINFOFUNCTION, pFn);
        so(h, CURLOPT_XFERINFODATA, p->pUser);
        so(h, CURLOPT_NOPROGRESS, 0L);
        pRetData->m_int = CURLE_OK;
        return;
    }
    case HOP_CurlHttp_SetUrl:
        pRetData->m_int = so(h, CURLOPT_URL, pArgInf[1].m_pText); return;
    case HOP_CurlHttp_SetUserPwd:
        so(h, CURLOPT_USERNAME, pArgInf[1].m_pText);
        so(h, CURLOPT_PASSWORD, pArgInf[2].m_pText);
        pRetData->m_int = so(h, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY); return;
    case HOP_CurlHttp_SetCookieFile:
        pRetData->m_int = so(h, CURLOPT_COOKIEFILE, pArgInf[1].m_pText); return;
    case HOP_CurlHttp_SetCookieList:
        pRetData->m_int = so(h, CURLOPT_COOKIEJAR, pArgInf[1].m_pText); return;
    case HOP_CurlHttp_SetCookieText:
        pRetData->m_int = so(h, CURLOPT_COOKIE, pArgInf[1].m_pText); return;
    case HOP_CurlHttp_SetPost:
        pRetData->m_int = so(h, CURLOPT_POST, pArgInf[1].m_bool ? 1L : 0L); return;
    case HOP_CurlHttp_SetPostBin:
    {
        // 字节集实参：块头是 [保留][长度]，正文在头之后；拷进对象自有缓冲再交给 curl
        LPBYTE pBin = (LPBYTE)pArgInf[1].m_pBin;
        const INT n = pBin == NULL ? 0 : *(INT*)(pBin + sizeof(INT));
        BufReset(&p->post);
        if (n > 0) BufAppend(&p->post, (LPCSTR)(pBin + sizeof(INT) * 2), n);
        so(h, CURLOPT_POSTFIELDSIZE, (long)p->post.nLen);
        pRetData->m_int = so(h, CURLOPT_POSTFIELDS, p->post.pData);
        return;
    }
    case HOP_CurlHttp_SetPostText:
    {
        BufReset(&p->post);
        if (pArgInf[1].m_pText != NULL) BufAppend(&p->post, pArgInf[1].m_pText, (int)lstrlenA(pArgInf[1].m_pText));
        so(h, CURLOPT_POSTFIELDSIZE, (long)p->post.nLen);
        pRetData->m_int = so(h, CURLOPT_POSTFIELDS, p->post.pData);
        return;
    }
    case HOP_CurlHttp_SetReferer:
        pRetData->m_int = so(h, CURLOPT_REFERER, pArgInf[1].m_pText); return;
    case HOP_CurlHttp_SetBufSize:
        pRetData->m_int = so(h, CURLOPT_BUFFERSIZE, (long)pArgInf[1].m_int); return;
    case HOP_CurlHttp_SetUserAgent:
        pRetData->m_int = so(h, CURLOPT_USERAGENT, pArgInf[1].m_pText); return;
    case HOP_CurlHttp_SetCaInfo:
        pRetData->m_int = so(h, CURLOPT_CAINFO, pArgInf[1].m_pText); return;
    case HOP_CurlHttp_SetHttpVersion:
        pRetData->m_int = so(h, CURLOPT_HTTP_VERSION, (long)pArgInf[1].m_int); return;
    case HOP_CurlHttp_SetProxy:
    {
        // 代理串形如 类型://服务器:端口；用户名非空时把 user:pass@ 插在 :// 之后
        char szBuf[1024], szOut[1280];
        lstrcpynA(szBuf, HTextAt(pArgInf, nArgCount, 1), sizeof(szBuf));
        LPCSTR szUser = HTextAt(pArgInf, nArgCount, 2);
        if (szUser[0] == '\0')
        {
            pRetData->m_int = so(h, CURLOPT_PROXY, szBuf);
            return;
        }
        wsprintfA(szOut, "%s:%s@", szUser, HTextAt(pArgInf, nArgCount, 3));
        LPCSTR sch = strstr(szBuf, "://");
        if (sch == NULL) lstrcatA(szOut, szBuf);
        else
        {
            char szHead[1024];
            const int nHead = (int)(sch - szBuf) + 3;
            memcpy(szHead, szBuf, nHead);
            szHead[nHead] = '\0';
            lstrcatA(szOut, szHead);
            lstrcatA(szOut, sch + 3);
        }
        pRetData->m_int = so(h, CURLOPT_PROXY, szOut);
        return;
    }
    case HOP_CurlHttp_Impersonate:
        pRetData->m_int = g_curl.Impersonate(h, pArgInf[1].m_pText,
                                             (nArgCount > 2 && pArgInf[2].m_bool) ? 1 : 0);
        return;
    case HOP_CurlHttp_SetTimeoutMs:
    {
        int r = CURLE_OK;
        if (HIntAt(pArgInf, nArgCount, 1) > 0) r = so(h, CURLOPT_CONNECTTIMEOUT_MS, (long)pArgInf[1].m_int);
        if (HIntAt(pArgInf, nArgCount, 2) > 0) r = so(h, CURLOPT_TIMEOUT_MS, (long)pArgInf[2].m_int);
        pRetData->m_int = r; return;
    }
    case HOP_CurlHttp_SetTimeout:
    {
        int r = CURLE_OK;
        if (HIntAt(pArgInf, nArgCount, 1) > 0) r = so(h, CURLOPT_CONNECTTIMEOUT, (long)pArgInf[1].m_int);
        if (HIntAt(pArgInf, nArgCount, 2) > 0) r = so(h, CURLOPT_TIMEOUT, (long)pArgInf[2].m_int);
        pRetData->m_int = r; return;
    }
    case HOP_CurlHttp_SetFollow:
    {
        int r = so(h, CURLOPT_FOLLOWLOCATION, pArgInf[1].m_bool ? 1L : 0L);
        const int nMax = HIntAt(pArgInf, nArgCount, 2);
        r = so(h, CURLOPT_MAXREDIRS, nMax > 0 ? (long)nMax : 5L);
        pRetData->m_int = r; return;
    }
    case HOP_CurlHttp_SetKeepAlive:
    {
        int r = so(h, CURLOPT_TCP_KEEPALIVE, pArgInf[1].m_bool ? 1L : 0L);
        const int nIdle = HIntAt(pArgInf, nArgCount, 2);
        if (nIdle > 0) r = so(h, CURLOPT_TCP_KEEPIDLE, (long)nIdle);
        pRetData->m_int = r; return;
    }
    case HOP_CurlHttp_SetEncoding:
    {
        LPCSTR sz = HTextAt(pArgInf, nArgCount, 1);
        pRetData->m_int = so(h, CURLOPT_ACCEPT_ENCODING, sz[0] == '\0' ? NULL : sz);
        return;
    }
    case HOP_CurlHttp_SetVerify:
    {
        int r = so(h, CURLOPT_SSL_VERIFYPEER, pArgInf[1].m_bool ? 1L : 0L);
        r = so(h, CURLOPT_SSL_VERIFYHOST, (nArgCount > 2 && pArgInf[2].m_bool) ? 2L : 0L);
        pRetData->m_int = r; return;
    }
    case HOP_CurlHttp_SetHeaders:
        pRetData->m_int = so(h, CURLOPT_HTTPHEADER, HBuildSlist(p, pArgInf[1])); return;

    // ---- curl_http 取结果 ----
    case HOP_CurlHttp_RespBody:  pRetData->m_pBin = CloneBinData(p->body.pData, p->body.nLen); return;
    case HOP_CurlHttp_RespHead:  SetRetText(pRetData, p->head.pData == NULL ? "" : p->head.pData); return;
    case HOP_CurlHttp_RespHeadBin: pRetData->m_pBin = CloneBinData(p->head.pData, p->head.nLen); return;
    case HOP_CurlHttp_RespCode:  HGetinfo(h, CURLINFO_RESPONSE_CODE, pRetData, 0); return;
    case HOP_CurlHttp_RespTime:  HGetinfo(h, CURLINFO_TOTAL_TIME, pRetData, 1); return;
    case HOP_CurlHttp_RespUrl:   HGetinfo(h, CURLINFO_EFFECTIVE_URL, pRetData, 2); return;
    case HOP_CurlHttp_RespRedirectUrl: HGetinfo(h, CURLINFO_REDIRECT_URL, pRetData, 2); return;
    case HOP_CurlHttp_RespRemoteIp:    HGetinfo(h, CURLINFO_PRIMARY_IP, pRetData, 2); return;
    case HOP_CurlHttp_RespLocalIp:     HGetinfo(h, CURLINFO_LOCAL_IP, pRetData, 2); return;
    case HOP_CurlHttp_RespRemotePort:  HGetinfo(h, CURLINFO_PRIMARY_PORT, pRetData, 0); return;
    case HOP_CurlHttp_RespLocalPort:   HGetinfo(h, CURLINFO_LOCAL_PORT, pRetData, 0); return;
    case HOP_CurlHttp_RespCookies:
    {
        char* pCookie = NULL;
        const int r = g_curl.EasyGetinfo(h, CURLINFO_COOKIELIST, &pCookie);
        BufReset(&p->head);
        if (pCookie != NULL) BufAppend(&p->head, pCookie, (int)lstrlenA(pCookie));
        HFillTextAry(pArgInf[1], &p->head, '\n');
        pRetData->m_int = r;
        return;
    }
    default:
        ReportError("HTTP访问：未知的成员命令");
        return;
    }
}

#endif  // CXHTTP_CORE_INC
