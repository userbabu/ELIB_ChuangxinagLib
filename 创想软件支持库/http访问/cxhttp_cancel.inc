// 令牌绑定持有共享状态引用；整数绑定借用四字节变量，两种控制源互斥。
static bool HStopRequested(CxHttpObj* p)
{
    if (p->nLastError == ERROR_CANCELLED) return true;
    if (p->pToken != NULL)
    {
        const DWORD status = CancelContinue(p->pToken, INFINITE);
        if (status != ERROR_SUCCESS) { p->nLastError = (int)status; return true; }
    }
    if (p->pStop != NULL && InterlockedCompareExchange(p->pStop, 0, 0) == 1)
    {
        p->nLastError = ERROR_CANCELLED;
        return true;
    }
    return false;
}

static bool HStopAddressValid(volatile LONG* p)
{
    // Interlocked 操作需要自然对齐且内存可写；此检查不代替调用方的生命周期保证。
    return p != NULL && ((ULONG_PTR)p & 3) == 0 && !IsBadWritePtr((LPVOID)p, sizeof(LONG));
}

// 访问类命令按对象串行执行；取消线程操作令牌或原子写整数，不争抢请求对象锁。
struct HAccessLock
{
    CxHttpObj* p;
    explicit HAccessLock(CxHttpObj* obj) : p(obj) { EnterCriticalSection(&p->accessCs); }
    ~HAccessLock() { LeaveCriticalSection(&p->accessCs); }
};

// 全局快捷请求只固定本次调用的令牌状态；省略参数不分配令牌、不启用额外等待。
static bool HOptionalToken(CxHttpObj* p, INT n, PMDATA_INF a, int index)
{
    if (index >= n || a[index].m_dtDataType == _SDT_NULL) return true;
    p->pToken = CancelPinReference(a[index]);
    if (p->pToken != NULL) return true;
    ReportError("网页访问：取消令牌尚未创建");
    return false;
}

static void HCancelCommand(PMDATA_INF r, INT n, PMDATA_INF a, int kind, int op)
{
    CxHttpObj* p = (CxHttpObj*)*(PINT)a[0].m_pCompoundData;
    if (p == NULL)
    {
        p = HNew(kind);
        if (p == NULL) { ReportError("HTTP访问：创建对象失败"); return; }
        *(PINT)a[0].m_pCompoundData = (INT)p;
    }
    HAccessLock lock(p);
    if (op == 2) { r->m_int = p->nLastError; return; }
    if (p->requestActive) { r->m_bool = BL_FALSE; ReportError("HTTP访问：请求回调内不能改绑停止变量"); return; }
    volatile LONG* addr = op == 0 ? (volatile LONG*)a[1].m_pInt
        : (volatile LONG*)(INT_PTR)a[1].m_int;
    r->m_bool = BL_FALSE;
    if (addr != NULL && !HStopAddressValid(addr))
    {
        ReportError("HTTP访问：停止变量必须是四字节对齐且可写的整数变量");
        return;
    }
    if (op == 0 && addr == NULL) { ReportError("HTTP访问：停止变量引用不能为空"); return; }
    p->pStop = addr; // 指针传 0 解绑；不读取或重置调用方的变量。
    CancelRelease(p->pToken);
    p->pToken = NULL;
    r->m_bool = BL_TRUE;
    UNREFERENCED_PARAMETER(n);
}

static void HBindToken(PMDATA_INF r, PMDATA_INF a, int kind, bool unbind)
{
    r->m_bool = BL_FALSE;
    CxHttpObj* p = (CxHttpObj*)*(PINT)a[0].m_pCompoundData;
    if (!p)
    {
        p = HNew(kind);
        if (!p) { ReportError("HTTP访问：创建对象失败"); return; }
        *(PINT)a[0].m_pCompoundData = (INT)p;
    }
    HAccessLock access(p);
    if (p->requestActive) { ReportError("HTTP访问：请求回调内不能改绑取消令牌"); return; }
    CxCancelState* state = unbind ? NULL : CancelPinReference(a[1]);
    if (!unbind && !state) { ReportError("HTTP访问：取消令牌尚未创建"); return; }
    CxCancelState* old = p->pToken;
    p->pToken = state;
    if (!unbind) p->pStop = NULL;
    CancelRelease(old);
    r->m_bool = BL_TRUE;
}
#define HTOKEN_METHODS(tag, kind) \
void tag##_BindToken(PMDATA_INF r, INT, PMDATA_INF a) { HBindToken(r,a,kind,false); } \
void tag##_UnbindToken(PMDATA_INF r, INT, PMDATA_INF a) { HBindToken(r,a,kind,true); }
HTOKEN_METHODS(AccessCurl, CXH_KIND_ACC_CURL)
HTOKEN_METHODS(AccessWinHttp, CXH_KIND_ACC_HTTP)
HTOKEN_METHODS(AccessWinInet, CXH_KIND_ACC_INET)
#undef HTOKEN_METHODS

#define HCANCEL_METHODS(tag, kind) \
void tag##_BindStop(PMDATA_INF r, INT n, PMDATA_INF a) { HCancelCommand(r,n,a,kind,0); } \
void tag##_BindStopPointer(PMDATA_INF r, INT n, PMDATA_INF a) { HCancelCommand(r,n,a,kind,1); } \
void tag##_GetLastError(PMDATA_INF r, INT n, PMDATA_INF a) { HCancelCommand(r,n,a,kind,2); }
HCANCEL_METHODS(AccessCurl, CXH_KIND_ACC_CURL)
HCANCEL_METHODS(AccessWinHttp, CXH_KIND_ACC_HTTP)
HCANCEL_METHODS(AccessWinInet, CXH_KIND_ACC_INET)
#undef HCANCEL_METHODS

void G_HttpExchangeStop(PMDATA_INF r, INT n, PMDATA_INF a)
{
    volatile LONG* p = (volatile LONG*)a[0].m_pInt;
    if (!HStopAddressValid(p)) { ReportError("HTTP访问：停止变量必须是四字节对齐且可写的整数变量"); return; }
    r->m_int = InterlockedExchange(p, a[1].m_int);
    UNREFERENCED_PARAMETER(n);
}

// multi 只在请求线程驱动与移除句柄，取消线程不接触 curl 内部状态。
static int HCancelableCurlPerform(CxHttpObj* p)
{
    if (HStopRequested(p)) return CURLE_ABORTED_BY_CALLBACK;
    if (p->pStop == NULL && p->pToken == NULL) return g_curl.EasyPerform(p->h);
    fn_multi_init_t init = (fn_multi_init_t)H_PROC("curl_multi_init");
    fn_multi_cleanup_t cleanup = (fn_multi_cleanup_t)H_PROC("curl_multi_cleanup");
    fn_multi_add_t add = (fn_multi_add_t)H_PROC("curl_multi_add_handle");
    fn_multi_add_t remove = (fn_multi_add_t)H_PROC("curl_multi_remove_handle");
    fn_multi_perform_t perform = (fn_multi_perform_t)H_PROC("curl_multi_perform");
    fn_multi_wait_t poll = (fn_multi_wait_t)H_PROC("curl_multi_poll");
    fn_multi_info_read_t read = (fn_multi_info_read_t)H_PROC("curl_multi_info_read");
    if (!init || !cleanup || !add || !remove || !perform || !poll || !read) return CURLE_FAILED_INIT;
    void* multi = init();
    if (multi == NULL) return CURLE_OUT_OF_MEMORY;
    int result = CURLE_FAILED_INIT;
    if (add(multi, p->h) == CURLM_OK)
    {
        int running = 1;
        while (!HStopRequested(p))
        {
            const int code = perform(multi, &running);
            if (code == CURLM_CALL_MULTI_PERFORM) continue;
            if (code != CURLM_OK) break;
            if (!running)
            {
                int queued = 0;
                CURLMsg* msg;
                while ((msg = (CURLMsg*)read(multi, &queued)) != NULL)
                    if (msg->msg == CURLMSG_DONE && msg->easy_handle == p->h) result = msg->data.result;
                break;
            }
            int ready = 0;
            if (poll(multi, NULL, 0, 20, &ready) != CURLM_OK) break;
        }
        if (HStopRequested(p)) result = CURLE_ABORTED_BY_CALLBACK;
        remove(multi, p->h);
    }
    cleanup(multi);
    return result;
}
