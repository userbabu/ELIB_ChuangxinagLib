// 线程锁族实现：线程锁（Windows 命名互斥量 + 持有者记账）、信号量、条件变量。
// 由 lib.cpp 在 namespace cx 内 #include：先 cxlock_api.inc（内核入口），再本文件。
//
// 为什么不用易语言自带的锁支持库：它不记录持有者，误解锁 / 解别人持有的锁 / 销毁后再用
// 都会直接踩内核对象，表现是随机崩溃。这里每个入口先验对象、再验持有者，最坏只报错不崩。
//
// 三种对象共用一个头部（nObj 决定种类），所以 复制 / 析构 只拿到指针也能正确分派：
//   0 = 线程锁   2 = 信号量   3 = 条件变量
#ifndef CXLOCK_CORE_INC
#define CXLOCK_CORE_INC

#define LK_OBJ_LOCK 0
#define LK_OBJ_SEM  2
#define LK_OBJ_COND 3
#define LK_KIND_MUTEX 0
#define LK_KIND_SPIN  2

// 每个等待者一个自动重置事件：先登记再放锁，因此不会丢唤醒
struct LkWaitNode
{
    HANDLE hEv;
    DWORD dwThread;
    LkWaitNode* pNext;
};

struct CxLock
{
    int nObj;               // 必须是首字段：= LK_OBJ_LOCK
    HANDLE hMutex;
    DWORD dwOwner;          // 持有线程 id，0 = 无人持有
    LONG nRecursion;        // 持有线程的嵌套层数
    int nKind;              // LK_KIND_*
    int nWaiters;
    LkWaitNode* pHead;
    LkWaitNode* pTail;
    CRITICAL_SECTION cs;    // 保护上面的记账与等待队列
    volatile LONG nRefs;    // 引用计数：置句柄 让多个对象共用一把锁时靠它决定谁真正释放
};

struct CxSem
{
    int nObj;               // = LK_OBJ_SEM
    HANDLE hSem;
    LONG nMax;
    volatile LONG nCur;     // 本对象观测到的可用计数（取当前计数用）
};

struct CxCond
{
    int nObj;               // = LK_OBJ_COND
    CRITICAL_SECTION cs;
    LkWaitNode* pHead;
    LkWaitNode* pTail;
    int nWaiters;
};

// ---------------- 命名锁注册表（进程内同名一把锁；对象销毁不删，注册表常驻） ----------------
typedef struct LkNameItem { char szName[128]; CxLock* pLock; struct LkNameItem* pNext; } LkNameItem;
static LkNameItem* g_pNameList = NULL;
static CRITICAL_SECTION g_lkNameCs;
static LONG g_lkNameInit = 0;
static void LkNameEnter()
{
    if (InterlockedCompareExchange(&g_lkNameInit, 1, 0) == 0) InitializeCriticalSection(&g_lkNameCs);
    EnterCriticalSection(&g_lkNameCs);
}
static void LkNameLeave() { if (g_lkNameInit != 0) LeaveCriticalSection(&g_lkNameCs); }

// ---------------- 通用小工具 ----------------
static void* LkMalloc(int nSize) { return LocalAlloc(LPTR, (SIZE_T)nSize); }
static void LkFreeMem(void* p) { if (p != NULL) LocalFree(p); }

// 等任意内核对象（事件 / 互斥量 / 信号量）；入口缺失时按失败返回
static DWORD LkWaitHandle(void* h, DWORD dwMs)
{
    const LkApi& a = LkGet();
    if (h == NULL || a.WaitForSingleObject == NULL) return 0xFFFFFFFFu;
    return (DWORD)a.WaitForSingleObject(h, dwMs);
}

static void LkQueuePush(LkWaitNode** ppHead, LkWaitNode** ppTail, int* pnWaiters, LkWaitNode* pNode)
{
    pNode->pNext = NULL;
    if (*ppTail == NULL) *ppHead = pNode;
    else (*ppTail)->pNext = pNode;
    *ppTail = pNode;
    (*pnWaiters)++;
}
// 摘除指定节点（只动链表；nWaiters 只在还在队里时减）
static void LkQueueRemove(LkWaitNode** ppHead, LkWaitNode** ppTail, int* pnWaiters, LkWaitNode* pNode)
{
    LkWaitNode** pp = ppHead;
    while (*pp != NULL && *pp != pNode) pp = &(*pp)->pNext;
    if (*pp == NULL) return;
    *pp = pNode->pNext;
    if (*ppTail == pNode) *ppTail = (pNode->pNext == NULL) ? *ppHead : pNode->pNext;
    if (*ppHead == NULL) *ppTail = NULL;
    if (*pnWaiters > 0) (*pnWaiters)--;
}
static void LkQueueWake(LkWaitNode** ppHead, LkWaitNode** ppTail, int* pnWaiters, BOOL bAll)
{
    int n = bAll ? *pnWaiters : (*pnWaiters > 0 ? 1 : 0);
    for (int i = 0; i < n && *ppHead != NULL; i++)
    {
        LkWaitNode* pHit = *ppHead;
        *ppHead = pHit->pNext;
        if (*ppHead == NULL) *ppTail = NULL;
        if (*pnWaiters > 0) (*pnWaiters)--;
        if (pHit->hEv != NULL) SetEvent(pHit->hEv);
    }
}
static HANDLE LkNewEvent() { return CreateEventA(NULL, FALSE, FALSE, NULL); }   // 自动重置 + 初始无信号
static void LkCloseEvent(HANDLE h) { if (h != NULL) CloseHandle(h); }

// ---------------- 构造 / 析构 ----------------
static void LkFreeLock(CxLock* p);
static CxLock* LkNewLock(int nKind)
{
    const LkApi& a = LkGet();
    if (a.CreateMutexA == NULL) { ReportError(a.LoadError[0] ? a.LoadError : "kernel32 互斥量入口不可用"); return NULL; }
    CxLock* p = (CxLock*)LkMalloc(sizeof(CxLock));
    if (p == NULL) return NULL;
    p->nObj = LK_OBJ_LOCK;
    p->nKind = (nKind == LK_KIND_SPIN) ? LK_KIND_SPIN : LK_KIND_MUTEX;
    InitializeCriticalSection(&p->cs);
    p->hMutex = a.CreateMutexA(NULL, FALSE, NULL);
    if (p->hMutex == NULL)
    {
        DeleteCriticalSection(&p->cs);
        LkFreeMem(p);
        ReportError("CreateMutex 失败：系统资源不足");
        return NULL;
    }
    p->nRefs = 1;
    return p;
}
// 引用计数：置句柄 让多个易语言变量共用同一把锁，谁最后放手谁真正销毁
static void LkRetain(CxLock* p) { if (p != NULL) InterlockedIncrement((LPLONG)&p->nRefs); }
static void LkReleaseRef(CxLock* p)
{
    if (p == NULL) return;
    if (InterlockedDecrement((LPLONG)&p->nRefs) == 0) LkFreeLock(p);
}
static CxSem* LkNewSem(LONG nInit, LONG nMax)
{
    const LkApi& a = LkGet();
    if (a.CreateSemaphoreA == NULL) { ReportError(a.LoadError[0] ? a.LoadError : "kernel32 信号量入口不可用"); return NULL; }
    if (nInit < 0) nInit = 0;
    if (nMax <= 0 || nMax < nInit) nMax = 0x7FFFFFFF;
    if (nInit > nMax) nInit = nMax;
    CxSem* p = (CxSem*)LkMalloc(sizeof(CxSem));
    if (p == NULL) return NULL;
    p->nObj = LK_OBJ_SEM;
    p->nMax = nMax;
    p->nCur = nInit;
    p->hSem = a.CreateSemaphoreA(NULL, nInit, nMax, NULL);
    if (p->hSem == NULL) { LkFreeMem(p); ReportError("CreateSemaphore 失败"); return NULL; }
    return p;
}
static CxCond* LkNewCond()
{
    CxCond* p = (CxCond*)LkMalloc(sizeof(CxCond));
    if (p == NULL) return NULL;
    p->nObj = LK_OBJ_COND;
    InitializeCriticalSection(&p->cs);
    return p;
}
static void LkFreeLock(CxLock* p)
{
    if (p == NULL) return;
    const LkApi& a = LkGet();
    if (p->dwOwner != 0 && a.ReleaseMutex != NULL && p->hMutex != NULL)
    {
        OutputDebugStringA("[cxsoft] 线程锁 被销毁时仍被持有，已强制释放\n");
        while (p->nRecursion > 0) { a.ReleaseMutex(p->hMutex); p->nRecursion--; }
        p->dwOwner = 0;
    }
    while (p->pHead != NULL) { LkWaitNode* q = p->pHead; p->pHead = q->pNext; if (q->hEv) SetEvent(q->hEv); LkFreeMem(q); }
    if (p->hMutex != NULL && a.CloseHandle != NULL) a.CloseHandle(p->hMutex);
    DeleteCriticalSection(&p->cs);
    LkFreeMem(p);
}
static void LkFreeSem(CxSem* p)
{
    if (p == NULL) return;
    const LkApi& a = LkGet();
    if (p->hSem != NULL && a.CloseHandle != NULL) a.CloseHandle(p->hSem);
    LkFreeMem(p);
}
static void LkFreeCond(CxCond* p)
{
    if (p == NULL) return;
    while (p->pHead != NULL) { LkWaitNode* q = p->pHead; p->pHead = q->pNext; if (q->hEv) SetEvent(q->hEv); LkFreeMem(q); }
    DeleteCriticalSection(&p->cs);
    LkFreeMem(p);
}

// 取实参指向的对象种类：槽无效 / 对象不是本库的都返回 -1
static int LkSelfKind(const MDATA_INF& arg)
{
    if (arg.m_pCompoundData == NULL || IsBadReadPtr(arg.m_pCompoundData, sizeof(INT))) return -1;
    void* p = (void*)(INT_PTR)(*(PINT)arg.m_pCompoundData);
    if (p == NULL || IsBadReadPtr(p, sizeof(int))) return -1;
    return *(int*)p;
}
static void* LkSelfObj(const MDATA_INF& arg, int nWant)
{
    if (LkSelfKind(arg) != nWant) return NULL;
    return (void*)(INT_PTR)(*(PINT)arg.m_pCompoundData);
}
static void LkFree(PMDATA_INF pSelfArg);
static void LkCopy(PMDATA_INF pArgInf, int nKind)
{
    SyncLifecycleGuard lifecycleGuard;
    // SDK第0参是未初始化的目标、第1参是源；复制未创建变量时仍保持未创建。
    if (pArgInf == NULL || pArgInf[0].m_pCompoundData == NULL) return;
    if (pArgInf[0].m_pCompoundData == pArgInf[1].m_pCompoundData) return;
    PINT pDstSlot = (PINT)pArgInf[0].m_pCompoundData;
    const int nSrcObj = LkSelfKind(pArgInf[1]);
    if (nSrcObj == -1) { *pDstSlot = 0; return; }
    if (nSrcObj != nKind) { *pDstSlot = 0; ReportError("复制：同步对象类型不一致"); return; }
    void* pNew = NULL;
    if (nSrcObj == LK_OBJ_SEM)
    {
        CxSem* pSrc = (CxSem*)LkSelfObj(pArgInf[1], LK_OBJ_SEM);
        pNew = LkNewSem(InterlockedCompareExchange(&pSrc->nCur, 0, 0), pSrc->nMax);
    }
    else if (nSrcObj == LK_OBJ_COND) pNew = LkNewCond();
    else if (nSrcObj == LK_OBJ_LOCK)
    {
        CxLock* pSrc = (CxLock*)LkSelfObj(pArgInf[1], LK_OBJ_LOCK);
        pNew = LkNewLock(pSrc->nKind);
    }
    *pDstSlot = (INT)(INT_PTR)pNew;
}
// 析构：入参是对象实参本身，对象槽 = 它的 m_pCompoundData 指向的那一格
static void LkFree(PMDATA_INF pSelfArg)
{
    SyncLifecycleGuard lifecycleGuard;
    if (pSelfArg == NULL || pSelfArg->m_pCompoundData == NULL) return;
    PINT pSlot = (PINT)pSelfArg->m_pCompoundData;
    if (IsBadWritePtr(pSlot, sizeof(INT)))
    {
        ReportError("析构：对象槽地址不可写，已忽略（不崩进程）");
        return;
    }
    void* pObj = (void*)(INT_PTR)(*pSlot);
    *pSlot = 0;
    if (pObj == NULL) return;
    if (IsBadReadPtr(pObj, sizeof(int)))
    {
        ReportError("析构：锁对象内存不可读，已忽略（不崩进程）");
        return;
    }
    const int nObj = *(int*)pObj;
    if (nObj == LK_OBJ_SEM) LkFreeSem((CxSem*)pObj);
    else if (nObj == LK_OBJ_COND) LkFreeCond((CxCond*)pObj);
    else if (nObj == LK_OBJ_LOCK) LkReleaseRef((CxLock*)pObj);
    else ReportError("析构：锁对象内存已被写坏，已忽略（不崩进程）");
}

// ---------------- 加锁 / 解锁 ----------------
static BOOL LkAcquire(CxLock* p, DWORD dwMs, BOOL bThrowOnFail)
{
    const LkApi& a = LkGet();
    if (p == NULL || a.WaitForSingleObject == NULL) return FALSE;
    const DWORD dwThread = GetCurrentThreadId();
    EnterCriticalSection(&p->cs);
    const DWORD dwOwner = p->dwOwner;
    const LONG nRec = p->nRecursion;
    LeaveCriticalSection(&p->cs);
    if (dwOwner == dwThread && nRec > 0)             // 同一线程重入
    {
        if (p->nKind == LK_KIND_SPIN)
        {
            if (bThrowOnFail) ReportError("自旋型 线程锁 不能嵌套加锁：请把 创建 的锁类型改回 0");
            return FALSE;
        }
        EnterCriticalSection(&p->cs);
        p->nRecursion++;
        LeaveCriticalSection(&p->cs);
        return TRUE;
    }
    if (dwOwner != 0 && dwOwner != dwThread && p->nKind == LK_KIND_SPIN)
    {
        // 自旋锁：先有界自旋，避免被调度的线程抢不到锁而整段睡死
        DWORD t0 = GetTickCount();
        for (int i = 0; i < 64; i++)
        {
            if (a.WaitForSingleObject(p->hMutex, 0) == 0L) { break; }
            SwitchToThread();
            if (GetTickCount() - t0 > dwMs && dwMs != WAIT_INFINITE) { return FALSE; }
        }
    }
    const DWORD r = (DWORD)a.WaitForSingleObject(p->hMutex, dwMs);
    if (r != 0L)
    {
        if (bThrowOnFail && r != 0x00000102L)       // 不是 WAIT_TIMEOUT 才报，超时属正常返回假
            ReportError("进入 线程锁 失败：内核对象无效或已被销毁");
        return FALSE;
    }
    EnterCriticalSection(&p->cs);
    p->dwOwner = dwThread;
    p->nRecursion = 1;
    LeaveCriticalSection(&p->cs);
    return TRUE;
}
static BOOL LkRelease(CxLock* p)
{
    const LkApi& a = LkGet();
    if (p == NULL || a.ReleaseMutex == NULL) return FALSE;
    const DWORD dwThread = GetCurrentThreadId();
    EnterCriticalSection(&p->cs);
    if (p->dwOwner != dwThread || p->nRecursion <= 0)
    {
        LeaveCriticalSection(&p->cs);
        ReportError("离开 线程锁 无效：本线程没有持有这把锁（只有持有者能解锁）");
        return FALSE;
    }
    p->nRecursion--;
    const LONG nLeft = p->nRecursion;
    if (nLeft == 0) p->dwOwner = 0;
    LeaveCriticalSection(&p->cs);
    if (nLeft == 0 && a.ReleaseMutex(p->hMutex) == 0L)
    {
        ReportError("离开 线程锁：内核解锁失败（句柄可能已被销毁）");
        return FALSE;
    }
    return TRUE;
}
// 条件等待：先登记再放锁，醒来后重新拿锁
static BOOL LkLockWait(CxLock* p, DWORD dwMs)
{
    const LkApi& a = LkGet();
    if (p == NULL || a.WaitForSingleObject == NULL) return FALSE;
    LkWaitNode node;
    memset(&node, 0, sizeof(node));
    node.hEv = LkNewEvent();
    node.dwThread = GetCurrentThreadId();
    if (node.hEv == NULL) { ReportError("等待 线程锁：创建等待事件失败"); return FALSE; }
    EnterCriticalSection(&p->cs);
    LkQueuePush(&p->pHead, &p->pTail, &p->nWaiters, &node);
    LeaveCriticalSection(&p->cs);

    if (p->nRecursion > 0 && p->dwOwner == node.dwThread) LkRelease(p);   // 没持有就不强解
    const DWORD r = (DWORD)a.WaitForSingleObject(node.hEv, dwMs);
    EnterCriticalSection(&p->cs);
    LkQueueRemove(&p->pHead, &p->pTail, &p->nWaiters, &node);
    LeaveCriticalSection(&p->cs);
    LkCloseEvent(node.hEv);
    const BOOL bOk = (r == 0L);
    LkAcquire(p, WAIT_INFINITE, FALSE);        // 无论是否超时都要把锁拿回来，避免调用方裸奔
    return bOk;
}

// 信号量 / 条件变量的等待与通知
static BOOL LkSemWait(CxSem* p, DWORD dwMs)
{
    const LkApi& a = LkGet();
    if (p == NULL || p->hSem == NULL || a.WaitForSingleObject == NULL) return FALSE;
    if (a.WaitForSingleObject(p->hSem, dwMs) != 0L) return FALSE;
    if (p->nCur > 0) InterlockedDecrement((LPLONG)&p->nCur);
    return TRUE;
}
static BOOL LkSemRelease(CxSem* p)
{
    const LkApi& a = LkGet();
    if (p == NULL || p->hSem == NULL || a.ReleaseSemaphore == NULL) return FALSE;
    LONG nPrev = 0;
    if (a.ReleaseSemaphore(p->hSem, 1, &nPrev) == 0) return FALSE;   // 非 0 才算成功
    InterlockedIncrement((LPLONG)&p->nCur);
    return TRUE;
}
static BOOL LkCondWait(CxCond* p, CxLock* pLock, DWORD dwMs)
{
    if (p == NULL) return FALSE;
    LkWaitNode node;
    memset(&node, 0, sizeof(node));
    node.hEv = LkNewEvent();
    node.dwThread = GetCurrentThreadId();
    if (node.hEv == NULL) { ReportError("等待 条件变量：创建等待事件失败"); return FALSE; }
    EnterCriticalSection(&p->cs);
    LkQueuePush(&p->pHead, &p->pTail, &p->nWaiters, &node);
    LeaveCriticalSection(&p->cs);
    if (pLock != NULL) LkRelease(pLock);
    const DWORD r = LkWaitHandle(node.hEv, dwMs);
    EnterCriticalSection(&p->cs);
    LkQueueRemove(&p->pHead, &p->pTail, &p->nWaiters, &node);
    LeaveCriticalSection(&p->cs);
    LkCloseEvent(node.hEv);
    if (pLock != NULL) LkAcquire(pLock, WAIT_INFINITE, FALSE);
    return r == 0L ? TRUE : FALSE;
}

#endif  // CXLOCK_CORE_INC
