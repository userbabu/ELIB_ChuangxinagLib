// 对象槽的读取、发布和销毁共用此锁；初始化完成状态通过 Interlocked 发布。
// 只用于短生命周期操作，绝不能跨进入/等待等阻塞调用持有。
static CRITICAL_SECTION g_syncLifecycleCs;
static volatile LONG g_syncLifecycleReady = 0;
struct SyncLifecycleGuard
{
    bool enabled;
    explicit SyncLifecycleGuard(bool take = true) : enabled(take)
    {
        if (!enabled) return;
        if (InterlockedCompareExchange(&g_syncLifecycleReady, 1, 0) == 0)
        {
            InitializeCriticalSection(&g_syncLifecycleCs);
            InterlockedExchange(&g_syncLifecycleReady, 2);
        }
        else while (InterlockedCompareExchange(&g_syncLifecycleReady, 0, 0) != 2) Sleep(0);
        EnterCriticalSection(&g_syncLifecycleCs);
    }
    ~SyncLifecycleGuard() { if (enabled) LeaveCriticalSection(&g_syncLifecycleCs); }
};
