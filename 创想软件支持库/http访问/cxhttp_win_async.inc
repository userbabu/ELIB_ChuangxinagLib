// 仅绑定停止变量的请求启用系统异步模式；调用接口仍等待并返回完整结果。
typedef void (CALLBACK *CxWinStatusCallback)(void*, DWORD_PTR, DWORD, void*, DWORD);
typedef CxWinStatusCallback (WINAPI *CxWhSetCallback)(void*, CxWinStatusCallback, DWORD, DWORD_PTR);
typedef CxWinStatusCallback (WINAPI *CxWiSetCallback)(void*, CxWinStatusCallback);

struct CxWinAsync
{
    HANDLE done;
    HANDLE closed;
    DWORD error;
    DWORD bytes;
    bool registered;
};

static bool HWinAsyncInit(CxHttpObj* p)
{
    if (p->pStop == NULL && p->pToken == NULL) return true;
    CxWinAsync* a = (CxWinAsync*)LocalAlloc(LPTR, sizeof(CxWinAsync));
    if (a == NULL) { p->nLastError = ERROR_NOT_ENOUGH_MEMORY; return false; }
    a->done = CreateEvent(NULL, TRUE, FALSE, NULL);
    a->closed = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!a->done || !a->closed)
    {
        p->nLastError = GetLastError();
        if (a->done) CloseHandle(a->done);
        if (a->closed) CloseHandle(a->closed);
        LocalFree(a);
        return false;
    }
    p->pAsync = a;
    return true;
}

static void CALLBACK HWinHttpStatus(void*, DWORD_PTR ctx, DWORD status, void* info, DWORD len)
{
    CxWinAsync* a = (CxWinAsync*)ctx;
    if (a == NULL) return;
    if (status == 0x800) { SetEvent(a->closed); return; } // HANDLE_CLOSING 是最终通知
    if (status == 0x200000) // REQUEST_ERROR: WINHTTP_ASYNC_RESULT
    {
        struct Result { DWORD_PTR result; DWORD error; };
        a->error = info && len >= sizeof(Result) ? ((Result*)info)->error : ERROR_GEN_FAILURE;
    }
    else if (status == 0x40000) a->bytes = info && len >= sizeof(DWORD) ? *(DWORD*)info : 0;
    else if (status == 0x80000) a->bytes = len;
    else if (status != 0x400000 && status != 0x20000) return;
    SetEvent(a->done);
}

static void CALLBACK HWinInetStatus(void*, DWORD_PTR ctx, DWORD status, void* info, DWORD len)
{
    CxWinAsync* a = (CxWinAsync*)ctx;
    if (a == NULL) return;
    if (status == 70) { SetEvent(a->closed); return; }
    if (status != 100) return;
    struct Result { DWORD_PTR result; DWORD error; };
    a->error = info && len >= sizeof(Result) ? ((Result*)info)->error : ERROR_GEN_FAILURE;
    SetEvent(a->done);
}

static bool HWinAsyncAttach(CxHttpObj* p, bool http)
{
    CxWinAsync* a = p->pAsync;
    if (a == NULL) return true;
    const DWORD_PTR ctx = (DWORD_PTR)a;
    bool ok;
    if (http)
    {
        CxWhSetCallback set = (CxWhSetCallback)GetProcAddress(g_cxWinHttp.hModule, "WinHttpSetStatusCallback");
        ok = set && g_cxWinHttp.SetOption
            && g_cxWinHttp.SetOption(p->hRequest, 45, (LPVOID)&ctx, sizeof(ctx))
            && set(p->hRequest, HWinHttpStatus, 0x6e0800, 0) != (CxWinStatusCallback)(INT_PTR)-1;
    }
    else
    {
        CxWiSetCallback set = (CxWiSetCallback)GetProcAddress(g_cxWinInet.hModule, "InternetSetStatusCallbackA");
        ok = set && set(p->hRequest, HWinInetStatus) != (CxWinStatusCallback)(INT_PTR)-1;
    }
    if (!ok) { p->nLastError = GetLastError(); if (!p->nLastError) p->nLastError = ERROR_PROC_NOT_FOUND; }
    a->registered = ok;
    return ok;
}

static void HWinAsyncCloseRequest(CxHttpObj* p, bool http)
{
    void* h = p->hRequest;
    p->hRequest = NULL; // 只有请求线程能关闭，关闭后不再调用任何句柄 API。
    if (h != NULL)
    {
        if (http) g_cxWinHttp.CloseHandle(h); else g_cxWinInet.CloseHandle(h);
        // 最终回调之前，系统仍可访问上下文、发送数据和读取缓冲，不能提前释放。
        if (p->pAsync && p->pAsync->registered) WaitForSingleObject(p->pAsync->closed, INFINITE);
    }
}

static void HWinAsyncFree(CxHttpObj* p)
{
    if (p->pAsync == NULL) return;
    CloseHandle(p->pAsync->done);
    CloseHandle(p->pAsync->closed);
    LocalFree(p->pAsync);
    p->pAsync = NULL;
}

static bool HWinAsyncPrepare(CxHttpObj* p)
{
    if (HStopRequested(p)) { SetLastError(ERROR_CANCELLED); return false; }
    if (p->hRequest == NULL) { SetLastError(ERROR_INVALID_HANDLE); return false; }
    if (p->pAsync)
    {
        ResetEvent(p->pAsync->done);
        p->pAsync->error = 0;
        p->pAsync->bytes = 0;
    }
    return true;
}

static BOOL HWinAsyncFinish(CxHttpObj* p, BOOL started, bool http, DWORD* bytes)
{
    DWORD err = started ? 0 : GetLastError();
    if (!p->pAsync)
    {
        p->nLastError = err;
        return started;
    }
    // WinHTTP 异步成功也必须等完成通知；WinInet 同步完成则不会发 REQUEST_COMPLETE。
    if ((!started && err != ERROR_IO_PENDING) || (!http && started))
    {
        p->nLastError = err;
        return started;
    }
    for (;;)
    {
        if (HStopRequested(p))
        {
            HWinAsyncCloseRequest(p, http);
            SetLastError(ERROR_CANCELLED);
            return FALSE;
        }
        const DWORD wait = WaitForSingleObject(p->pAsync->done, 20);
        if (wait == WAIT_TIMEOUT) continue;
        if (wait != WAIT_OBJECT_0) err = GetLastError();
        else err = p->pAsync->error;
        if (bytes && http) *bytes = p->pAsync->bytes;
        p->nLastError = err;
        SetLastError(err);
        return err == 0;
    }
}

static BOOL HWhSend(CxHttpObj* p, LPCWSTR head, DWORD headLen, LPVOID data, DWORD len, DWORD total)
{
    if (!HWinAsyncPrepare(p)) return FALSE;
    return HWinAsyncFinish(p, g_cxWinHttp.SendRequest(p->hRequest, head, headLen, data, len, total,
        (DWORD_PTR)p->pAsync), true, NULL);
}
static BOOL HWhReceive(CxHttpObj* p)
{
    if (!HWinAsyncPrepare(p)) return FALSE;
    return HWinAsyncFinish(p, g_cxWinHttp.ReceiveResponse(p->hRequest, NULL), true, NULL);
}
static BOOL HWhAvailable(CxHttpObj* p, DWORD* bytes)
{
    if (!HWinAsyncPrepare(p)) return FALSE;
    return HWinAsyncFinish(p, g_cxWinHttp.QueryDataAvailable(p->hRequest, p->pAsync ? NULL : bytes), true, bytes);
}
static BOOL HWhRead(CxHttpObj* p, LPVOID data, DWORD len, DWORD* bytes)
{
    if (!HWinAsyncPrepare(p)) return FALSE;
    return HWinAsyncFinish(p, g_cxWinHttp.ReadData(p->hRequest, data, len, p->pAsync ? NULL : bytes), true, bytes);
}
static BOOL HWiSend(CxHttpObj* p, LPCSTR head, DWORD headLen, LPCVOID data, DWORD len)
{
    if (!HWinAsyncPrepare(p)) return FALSE;
    return HWinAsyncFinish(p, g_cxWinInet.SendRequestA(p->hRequest, head, headLen, data, len), false, NULL);
}
static BOOL HWiRead(CxHttpObj* p, LPVOID data, DWORD len, DWORD* bytes)
{
    if (!HWinAsyncPrepare(p)) return FALSE;
    return HWinAsyncFinish(p, g_cxWinInet.ReadFile(p->hRequest, data, len, bytes), false, bytes);
}
