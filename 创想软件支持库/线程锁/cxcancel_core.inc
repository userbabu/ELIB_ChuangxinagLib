// 令牌变量、副本、等待调用及 HTTP 绑定各自持有引用；只有最后一份引用释放后才关闭事件。
struct CxCancelState
{
    CRITICAL_SECTION cs;
    volatile LONG refs;
    volatile LONG state; // 0=运行，1=暂停，2=取消；创建会为本变量替换一个新的运行状态。
    HANDLE cancelled;
    HANDLE runnable;
};

static CxCancelState* CancelNew()
{
    CxCancelState* p = (CxCancelState*)LocalAlloc(LPTR, sizeof(CxCancelState));
    if (!p) { ReportError("取消令牌：内存不足"); return NULL; }
    p->cancelled = CreateEventA(NULL, TRUE, FALSE, NULL);
    p->runnable = CreateEventA(NULL, TRUE, TRUE, NULL);
    if (!p->cancelled || !p->runnable)
    {
        if (p->cancelled) CloseHandle(p->cancelled);
        if (p->runnable) CloseHandle(p->runnable);
        LocalFree(p);
        ReportError("取消令牌：创建事件失败");
        return NULL;
    }
    InitializeCriticalSection(&p->cs);
    p->refs = 1;
    return p;
}
static void CancelRetain(CxCancelState* p) { if (p) InterlockedIncrement(&p->refs); }
static void CancelRelease(CxCancelState* p)
{
    if (p && InterlockedDecrement(&p->refs) == 0)
    {
        CloseHandle(p->cancelled);
        CloseHandle(p->runnable);
        DeleteCriticalSection(&p->cs);
        LocalFree(p);
    }
}
static LONG CancelRead(CxCancelState* p) { return InterlockedCompareExchange(&p->state, 0, 0); }
static CxCancelState* CancelSelf(const MDATA_INF& arg)
{
    return arg.m_pCompoundData ? (CxCancelState*)*(PINT)arg.m_pCompoundData : NULL;
}
static CxCancelState* CancelPin(const MDATA_INF& arg)
{
    SyncLifecycleGuard lifecycle;
    CxCancelState* p = CancelSelf(arg);
    CancelRetain(p);
    return p;
}
static CxCancelState* CancelPinReference(const MDATA_INF& arg)
{
    SyncLifecycleGuard lifecycle;
    // AS_RECEIVE_VAR接收复合变量地址；先取得对象数据，再读取对象保存的状态指针。
    void* data = arg.m_ppCompoundData ? *arg.m_ppCompoundData : NULL;
    CxCancelState* p = data ? (CxCancelState*)*(PINT)data : NULL;
    CancelRetain(p);
    return p;
}
struct CancelLease
{
    CxCancelState* p;
    explicit CancelLease(const MDATA_INF& a) : p(CancelPin(a)) {}
    ~CancelLease() { CancelRelease(p); }
};

// 不跨系统等待持有生命周期锁或状态锁；取消同时置位两个事件，暂停中的调用也会返回。
static DWORD CancelContinue(CxCancelState* p, DWORD timeout)
{
    // 请求正常运行时只读一次原子状态；不计时、不分配、不进入生命周期锁。
    LONG state = CancelRead(p);
    if (state != 1) return state == 2 ? ERROR_CANCELLED : ERROR_SUCCESS;
    const DWORD begin = timeout == INFINITE ? 0 : GetTickCount();
    for (;;)
    {
        DWORD remaining = timeout;
        if (timeout != INFINITE)
        {
            const DWORD elapsed = GetTickCount() - begin;
            if (elapsed >= timeout) return WAIT_TIMEOUT;
            remaining = timeout - elapsed;
        }
        const DWORD wait = WaitForSingleObject(p->runnable, remaining);
        if (wait == WAIT_OBJECT_0)
        {
            state = CancelRead(p);
            if (state != 1) return state == 2 ? ERROR_CANCELLED : ERROR_SUCCESS;
            continue; // 恢复后可能再次暂停，始终以状态为准。
        }
        if (wait == WAIT_TIMEOUT) return wait;
        ReportError("取消令牌：等待继续失败");
        return GetLastError() ? GetLastError() : ERROR_GEN_FAILURE;
    }
}
static DWORD CancelTimeout(INT n, PMDATA_INF a)
{
    return n < 2 || a[1].m_dtDataType == _SDT_NULL || a[1].m_int < 0 ? INFINITE : (DWORD)a[1].m_int;
}
void CxCancelConstruct(PMDATA_INF r, INT, PMDATA_INF) { r->m_int = (INT)CancelNew(); }
void CxCancelCopy(PMDATA_INF, INT, PMDATA_INF a)
{
    SyncLifecycleGuard lifecycle;
    if (a[0].m_pCompoundData == a[1].m_pCompoundData) return;
    CxCancelState* p = CancelSelf(a[1]);
    CancelRetain(p);
    *(PINT)a[0].m_pCompoundData = (INT)p;
}
void CxCancelDestruct(PMDATA_INF, INT, PMDATA_INF a)
{
    SyncLifecycleGuard lifecycle;
    CxCancelState* p = CancelSelf(a[0]);
    *(PINT)a[0].m_pCompoundData = 0;
    CancelRelease(p);
}
void CxCancel_Create(PMDATA_INF r, INT, PMDATA_INF a)
{
    r->m_bool = BL_FALSE;
    CxCancelState* p = CancelNew();
    if (!p) return;
    SyncLifecycleGuard lifecycle;
    CxCancelState* old = CancelSelf(a[0]);
    *(PINT)a[0].m_pCompoundData = (INT)p;
    // 创建仅替换本变量的引用；旧副本与请求保留其状态，避免旧任务被意外恢复。
    CancelRelease(old);
    r->m_bool = BL_TRUE;
}
void CxCancel_Destroy(PMDATA_INF r, INT n, PMDATA_INF a) { CxCancelDestruct(r, n, a); }
static void CancelChange(PMDATA_INF r, PMDATA_INF a, LONG state)
{
    r->m_bool = BL_FALSE;
    CancelLease lease(a[0]);
    CxCancelState* p = lease.p;
    if (!p) { ReportError("取消令牌：对象尚未创建"); return; }
    EnterCriticalSection(&p->cs);
    const bool allowed = CancelRead(p) != 2 || state == 2;
    bool ok = allowed;
    if (allowed)
    {
        // 事件与状态在同一临界区更新，防止暂停与恢复竞争造成状态已恢复但事件未置位。
        InterlockedExchange(&p->state, state);
        if (state == 2) ok = SetEvent(p->cancelled) != 0;
        if (state == 1) ok = ResetEvent(p->runnable) != 0 && ok;
        else ok = SetEvent(p->runnable) != 0 && ok;
    }
    LeaveCriticalSection(&p->cs);
    r->m_bool = ok ? BL_TRUE : BL_FALSE;
    if (allowed && !ok) ReportError("取消令牌：更新等待事件失败");
}
void CxCancel_Cancel(PMDATA_INF r, INT, PMDATA_INF a) { CancelChange(r, a, 2); }
void CxCancel_Pause(PMDATA_INF r, INT, PMDATA_INF a) { CancelChange(r, a, 1); }
void CxCancel_Resume(PMDATA_INF r, INT, PMDATA_INF a) { CancelChange(r, a, 0); }
void CxCancel_IsCancelled(PMDATA_INF r, INT, PMDATA_INF a)
{
    CancelLease lease(a[0]); r->m_bool = lease.p && CancelRead(lease.p) == 2 ? BL_TRUE : BL_FALSE;
}
void CxCancel_IsPaused(PMDATA_INF r, INT, PMDATA_INF a)
{
    CancelLease lease(a[0]); r->m_bool = lease.p && CancelRead(lease.p) == 1 ? BL_TRUE : BL_FALSE;
}
void CxCancel_IsCreated(PMDATA_INF r, INT, PMDATA_INF a)
{
    CancelLease lease(a[0]); r->m_bool = lease.p ? BL_TRUE : BL_FALSE;
}
void CxCancel_WaitCancel(PMDATA_INF r, INT n, PMDATA_INF a)
{
    r->m_bool = BL_FALSE;
    CancelLease lease(a[0]);
    if (!lease.p) { ReportError("取消令牌：对象尚未创建"); return; }
    if (CancelRead(lease.p) == 2) { r->m_bool = BL_TRUE; return; }
    const DWORD wait = WaitForSingleObject(lease.p->cancelled, CancelTimeout(n, a));
    if (wait == WAIT_OBJECT_0) r->m_bool = BL_TRUE;
    else if (wait != WAIT_TIMEOUT) ReportError("取消令牌：等待取消失败");
}
void CxCancel_WaitContinue(PMDATA_INF r, INT n, PMDATA_INF a)
{
    r->m_bool = BL_FALSE;
    CancelLease lease(a[0]);
    if (!lease.p) { ReportError("取消令牌：对象尚未创建"); return; }
    r->m_bool = CancelContinue(lease.p, CancelTimeout(n, a)) == ERROR_SUCCESS ? BL_TRUE : BL_FALSE;
}
