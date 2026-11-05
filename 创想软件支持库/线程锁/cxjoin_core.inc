// 一轮会合：到齐前阻塞，最后一个到达者返回真，其余参与者返回假。
// 复制对象共享同一个会合点；等待调用持有独立引用，取消/析构不会提前释放事件。
struct CxJoin
{
    CRITICAL_SECTION cs;
    HANDLE ready;
    volatile LONG refs;
    int owners;
    int target;
    int arrived;
    int active;
    int state; // 0=未创建，1=等待到齐，2=到齐，3=取消，4=系统失败
};

static CxJoin* JoinNew()
{
    CxJoin* p = (CxJoin*)LocalAlloc(LPTR, sizeof(CxJoin));
    if (p == NULL) { ReportError("线程会合：内存不足"); return NULL; }
    p->ready = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (p->ready == NULL) { LocalFree(p); ReportError("线程会合：创建等待事件失败"); return NULL; }
    InitializeCriticalSection(&p->cs);
    p->refs = 1;
    p->owners = 1;
    return p;
}

static void JoinRelease(CxJoin* p)
{
    if (p != NULL && InterlockedDecrement(&p->refs) == 0)
    {
        CloseHandle(p->ready);
        DeleteCriticalSection(&p->cs);
        LocalFree(p);
    }
}

static bool JoinSignal(CxJoin* p, int state)
{
    p->state = state;
    if (SetEvent(p->ready)) return true;
    p->state = 4;
    return false;
}

static void JoinDropOwner(CxJoin* p)
{
    if (p == NULL) return;
    EnterCriticalSection(&p->cs);
    const bool wakeFailed = --p->owners == 0 && p->state == 1 && !JoinSignal(p, 3);
    LeaveCriticalSection(&p->cs);
    if (wakeFailed) ReportError("线程会合：取消等待事件失败");
    JoinRelease(p);
}

static CxJoin* JoinSelf(PMDATA_INF args)
{
    return args && args[0].m_pCompoundData ? (CxJoin*)*(PINT)args[0].m_pCompoundData : NULL;
}

void CxJoinConstruct(PMDATA_INF ret, INT, PMDATA_INF)
{
    ret->m_int = (INT)JoinNew();
}

void CxJoinDestruct(PMDATA_INF, INT, PMDATA_INF args)
{
    SyncLifecycleGuard lifecycleGuard;
    CxJoin* p = JoinSelf(args);
    if (args && args[0].m_pCompoundData) *(PINT)args[0].m_pCompoundData = 0;
    JoinDropOwner(p);
}

void CxJoinCopy(PMDATA_INF, INT, PMDATA_INF args)
{
    SyncLifecycleGuard lifecycleGuard;
    if (args[0].m_pCompoundData == args[1].m_pCompoundData) return;
    // 第0参是未初始化的目标对象，第1参是源；SDK已释放目标的旧内容，不能再读目标槽。
    CxJoin* src = (CxJoin*)*(PINT)args[1].m_pCompoundData;
    if (src != NULL)
    {
        EnterCriticalSection(&src->cs);
        InterlockedIncrement(&src->refs);
        src->owners++;
        LeaveCriticalSection(&src->cs);
    }
    *(PINT)args[0].m_pCompoundData = (INT)src;
}

static void JoinConfigure(PMDATA_INF ret, PMDATA_INF args, bool onlyIfAbsent)
{
    SyncLifecycleGuard lifecycleGuard;
    ret->m_bool = BL_FALSE;
    const int total = args[1].m_int;
    if (total <= 0) { ReportError("线程会合：线程总数必须大于零"); return; }
    CxJoin* p = JoinSelf(args);
    if (p == NULL)
    {
        p = JoinNew();
        if (p == NULL) return;
        *(PINT)args[0].m_pCompoundData = (INT)p;
    }
    EnterCriticalSection(&p->cs);
    if (onlyIfAbsent && p->target > 0)
    {
        const bool sameTotal = p->target == total;
        const bool valid = p->state != 4 && sameTotal;
        LeaveCriticalSection(&p->cs);
        ret->m_bool = valid ? BL_TRUE : BL_FALSE;
        if (!sameTotal) ReportError("线程会合：确保创建的线程总数与本轮配置不一致");
        else if (!valid) ReportError("线程会合：内部事件已失败，不能确保创建");
        return;
    }
    const bool busy = p->active != 0;
    const bool reset = !busy && ResetEvent(p->ready);
    if (reset)
    {
        p->target = total;
        p->arrived = 0;
        p->state = 1;
        ret->m_bool = BL_TRUE;
    }
    LeaveCriticalSection(&p->cs);
    if (busy) ReportError("线程会合：上一轮等待尚未全部返回，不能重新创建");
    else if (!reset) ReportError("线程会合：重置等待事件失败");
}
void CxJoin_Create(PMDATA_INF ret, INT, PMDATA_INF args) { JoinConfigure(ret, args, false); }
void CxJoin_EnsureCreated(PMDATA_INF ret, INT, PMDATA_INF args) { JoinConfigure(ret, args, true); }

void CxJoin_Wait(PMDATA_INF ret, INT, PMDATA_INF args)
{
    ret->m_bool = BL_FALSE;
    CxJoin* p;
    {
        // 必须在读取对象槽与增加引用之间阻止销毁，之后才可以离开生命周期锁去等待。
        SyncLifecycleGuard lifecycleGuard;
        p = JoinSelf(args);
        if (p != NULL) InterlockedIncrement(&p->refs);
    }
    if (p == NULL) { ReportError("线程会合：请先创建并设置线程总数"); return; }
    EnterCriticalSection(&p->cs);
    if (p->state != 1)
    {
        const bool unconfigured = p->state == 0;
        LeaveCriticalSection(&p->cs);
        JoinRelease(p);
        if (unconfigured) ReportError("线程会合：请先创建并设置线程总数");
        return; // 到齐或取消的这一轮不再接受新参与者。
    }
    p->active++;
    const bool last = ++p->arrived == p->target;
    const bool signalFailed = last && !JoinSignal(p, 2);
    LeaveCriticalSection(&p->cs);

    // 事件在到齐/取消后保持有信号，避免唤醒早于前面的线程真正进入系统等待时丢失通知。
    const DWORD wait = last ? WAIT_OBJECT_0 : WaitForSingleObject(p->ready, INFINITE);
    EnterCriticalSection(&p->cs);
    bool waitFailed = wait != WAIT_OBJECT_0;
    if (waitFailed) JoinSignal(p, 4);
    ret->m_bool = last && p->state == 2 ? BL_TRUE : BL_FALSE;
    p->active--;
    LeaveCriticalSection(&p->cs);
    JoinRelease(p);
    if (signalFailed || waitFailed) ReportError("线程会合：系统等待或唤醒失败");
}

void CxJoin_Cancel(PMDATA_INF ret, INT, PMDATA_INF args)
{
    SyncLifecycleGuard lifecycleGuard;
    ret->m_bool = BL_FALSE;
    CxJoin* p = JoinSelf(args);
    if (p == NULL) return;
    EnterCriticalSection(&p->cs);
    const bool cancel = p->state == 1;
    const bool ok = cancel && JoinSignal(p, 3);
    LeaveCriticalSection(&p->cs);
    if (cancel && !ok) ReportError("线程会合：取消等待失败");
    ret->m_bool = ok ? BL_TRUE : BL_FALSE;
}

void CxJoin_Destroy(PMDATA_INF ret, INT n, PMDATA_INF args) { CxJoinDestruct(ret, n, args); }

static int JoinRead(CxJoin* p, int property)
{
    if (p == NULL) return 0;
    EnterCriticalSection(&p->cs);
    const int value = property == 0 ? p->arrived : p->state;
    LeaveCriticalSection(&p->cs);
    return value;
}
void CxJoin_Arrived(PMDATA_INF ret, INT, PMDATA_INF args) { SyncLifecycleGuard guard; ret->m_int = JoinRead(JoinSelf(args), 0); }
void CxJoin_IsComplete(PMDATA_INF ret, INT, PMDATA_INF args) { SyncLifecycleGuard guard; ret->m_bool = JoinRead(JoinSelf(args), 1) == 2 ? BL_TRUE : BL_FALSE; }
void CxJoin_IsCancelled(PMDATA_INF ret, INT, PMDATA_INF args) { SyncLifecycleGuard guard; ret->m_bool = JoinRead(JoinSelf(args), 1) == 3 ? BL_TRUE : BL_FALSE; }
void CxJoin_IsCreated(PMDATA_INF ret, INT, PMDATA_INF args)
{
    SyncLifecycleGuard guard;
    CxJoin* p = JoinSelf(args);
    ret->m_bool = BL_FALSE;
    if (p == NULL) return;
    EnterCriticalSection(&p->cs);
    ret->m_bool = p->target > 0 && p->state != 4 ? BL_TRUE : BL_FALSE;
    LeaveCriticalSection(&p->cs);
}
