// 线程锁族：27 条成员命令的派发。由 lib.cpp 在 cxlock_core.inc 之后 #include。
#ifndef CXLOCK_IMPL_INC
#define CXLOCK_IMPL_INC

// 实参取对象：种类不符只报错，绝不解引用野指针
static void* LkSelfAny(PMDATA_INF pArgInf, int nWant)
{
    if (pArgInf == NULL || pArgInf[0].m_pCompoundData == NULL) { ReportError("对象槽无效：请先调用 创建()"); return NULL; }
    void* p = (void*)(INT_PTR)(*(PINT)pArgInf[0].m_pCompoundData);
    if (p == NULL) { ReportError("对象尚未创建：请先调用 创建()"); return NULL; }
    if (IsBadReadPtr(p, sizeof(int)) || *(int*)p != nWant)
    {
        ReportError("对象无效：不是本库创建的对象，或已被销毁");
        return NULL;
    }
    return p;
}
// 超时毫秒：省略（易语言传进来的空值实参是 _SDT_NULL + 0）与 -1 都是无限等待；0 = 只探一次不等待。
// 只看 m_int 会把「省略」当成 0 毫秒立即超时，正好和命令说明相反 —— 必须先判空值标记。
static DWORD LkMs(PMDATA_INF pArgInf, INT nArgCount, int i)
{
    if (pArgInf == NULL || i >= nArgCount) return WAIT_INFINITE;
    if (pArgInf[i].m_dtDataType == _SDT_NULL) return WAIT_INFINITE;
    const INT n = pArgInf[i].m_int;
    if (n < 0) return WAIT_INFINITE;                 // -1 = 无限等待
    return (DWORD)n;
}
// 只读类查询（取句柄/取持有线程id/…）：未创建不报错，按 0/假 返回；只有内存被写坏才报
static void* LkSelfQuiet(PMDATA_INF pArgInf, int nWant)
{
    if (pArgInf == NULL || pArgInf[0].m_pCompoundData == NULL) return NULL;
    if (IsBadReadPtr(pArgInf[0].m_pCompoundData, sizeof(INT))) return NULL;
    void* p = (void*)(INT_PTR)(*(PINT)pArgInf[0].m_pCompoundData);
    if (p == NULL) return NULL;
    if (IsBadReadPtr(p, sizeof(int)) || *(int*)p != nWant) return NULL;
    return p;
}

void CxLockInvoke(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf, int nOp, int nKind)
{
    UNREFERENCED_PARAMETER(nKind);
    pRetData->m_dtDataType = 0;
    const bool lifecycle = nOp == LOP_LK_CxLock_Create || nOp == LOP_LK_CxLock_Destroy
        || nOp == LOP_LK_CxSem_Create || nOp == LOP_LK_CxSem_Destroy
        || nOp == LOP_LK_CxCond_Create || nOp == LOP_LK_CxCond_Destroy
        || nOp == LOP_LK_CxLock_SetHandle || nOp == LOP_LK_CxLock_Named
        || nOp == LOP_LK_CxLock_IsLocked || nOp == LOP_LK_CxLock_Owner
        || nOp == LOP_LK_CxLock_Recursion || nOp == LOP_LK_CxLock_Waiters
        || nOp == LOP_LK_CxLock_GetHandle || nOp == LOP_LK_CxSem_Value || nOp == LOP_LK_CxCond_Waiters;
    SyncLifecycleGuard lifecycleGuard(lifecycle);
    switch (nOp)
    {
    // ---------------- 线程锁 ----------------
    case LOP_LK_CxLock_Create:
    {
        if (pArgInf == NULL || pArgInf[0].m_pCompoundData == NULL ||
            IsBadWritePtr(pArgInf[0].m_pCompoundData, sizeof(INT))) { ReportError("线程锁 对象槽无效"); return; }
        CxLock* p = (CxLock*)(INT_PTR)(*(PINT)pArgInf[0].m_pCompoundData);
        const int nType = nArgCount > 1 ? pArgInf[1].m_int : LK_KIND_MUTEX;
        if (p != NULL && !IsBadReadPtr(p, sizeof(int)) && *(int*)p == LK_OBJ_LOCK)
        {
            // 已存在的实例不重建、不改配置，避免并发按需创建改变其他线程正在使用的锁。
            pRetData->m_bool = BL_TRUE;
            return;
        }
        p = LkNewLock(nType);
        if (p == NULL) { pRetData->m_bool = BL_FALSE; return; }
        *(PINT)pArgInf[0].m_pCompoundData = (INT)p;
        pRetData->m_bool = BL_TRUE;
        return;
    }
    case LOP_LK_CxLock_Destroy:
    {
        if (pArgInf == NULL || pArgInf[0].m_pCompoundData == NULL) return;
        LkFree(&pArgInf[0]);
        return;
    }
    case LOP_LK_CxLock_Enter:
    {
        CxLock* p = (CxLock*)LkSelfAny(pArgInf, LK_OBJ_LOCK);
        pRetData->m_bool = (p != NULL && LkAcquire(p, LkMs(pArgInf, nArgCount, 1), TRUE)) ? BL_TRUE : BL_FALSE;
        return;
    }
    case LOP_LK_CxLock_TryEnter:
    {
        CxLock* p = (CxLock*)LkSelfAny(pArgInf, LK_OBJ_LOCK);
        pRetData->m_bool = (p != NULL && LkAcquire(p, 0, FALSE)) ? BL_TRUE : BL_FALSE;
        return;
    }
    case LOP_LK_CxLock_Leave:
    {
        CxLock* p = (CxLock*)LkSelfAny(pArgInf, LK_OBJ_LOCK);
        pRetData->m_bool = (p != NULL && LkRelease(p)) ? BL_TRUE : BL_FALSE;
        return;
    }
    case LOP_LK_CxLock_Wait:
    {
        CxLock* p = (CxLock*)LkSelfAny(pArgInf, LK_OBJ_LOCK);
        pRetData->m_bool = (p != NULL && LkLockWait(p, LkMs(pArgInf, nArgCount, 1))) ? BL_TRUE : BL_FALSE;
        return;
    }
    case LOP_LK_CxLock_Signal:
    {
        CxLock* p = (CxLock*)LkSelfAny(pArgInf, LK_OBJ_LOCK);
        if (p == NULL) return;
        if (p->dwOwner != GetCurrentThreadId()) { ReportError("信号/唤醒全部 只能由持有锁的线程调用"); return; }
        EnterCriticalSection(&p->cs);
        LkQueueWake(&p->pHead, &p->pTail, &p->nWaiters, FALSE);
        LeaveCriticalSection(&p->cs);
        return;
    }
    case LOP_LK_CxLock_Broadcast:
    {
        CxLock* p = (CxLock*)LkSelfAny(pArgInf, LK_OBJ_LOCK);
        if (p == NULL) return;
        if (p->dwOwner != GetCurrentThreadId()) { ReportError("信号/唤醒全部 只能由持有锁的线程调用"); return; }
        EnterCriticalSection(&p->cs);
        LkQueueWake(&p->pHead, &p->pTail, &p->nWaiters, TRUE);
        LeaveCriticalSection(&p->cs);
        return;
    }
    case LOP_LK_CxLock_IsLocked:
    {
        CxLock* p = (CxLock*)LkSelfQuiet(pArgInf, LK_OBJ_LOCK);
        if (p == NULL) { pRetData->m_bool = BL_FALSE; return; }
        EnterCriticalSection(&p->cs);
        const BOOL b = (p->dwOwner != 0) ? TRUE : FALSE;
        LeaveCriticalSection(&p->cs);
        pRetData->m_bool = b ? BL_TRUE : BL_FALSE;
        return;
    }
    case LOP_LK_CxLock_Owner:
    {
        CxLock* p = (CxLock*)LkSelfQuiet(pArgInf, LK_OBJ_LOCK);
        if (p == NULL) { pRetData->m_int = 0; return; }
        EnterCriticalSection(&p->cs);
        const INT v = (INT)p->dwOwner;
        LeaveCriticalSection(&p->cs);
        pRetData->m_int = v;
        return;
    }
    case LOP_LK_CxLock_Recursion:
    {
        CxLock* p = (CxLock*)LkSelfQuiet(pArgInf, LK_OBJ_LOCK);
        if (p == NULL) { pRetData->m_int = 0; return; }
        EnterCriticalSection(&p->cs);
        const INT v = (INT)p->nRecursion;
        LeaveCriticalSection(&p->cs);
        pRetData->m_int = v;
        return;
    }
    case LOP_LK_CxLock_Waiters:
    {
        CxLock* p = (CxLock*)LkSelfQuiet(pArgInf, LK_OBJ_LOCK);
        if (p == NULL) { pRetData->m_int = 0; return; }
        EnterCriticalSection(&p->cs);
        const INT v = p->nWaiters;
        LeaveCriticalSection(&p->cs);
        pRetData->m_int = v;
        return;
    }
    case LOP_LK_CxLock_Named:
    {
        LPCSTR szName = HTextAt(pArgInf, nArgCount, 1);
        if (szName[0] == '\0') { ReportError("按名取锁：名字不能为空"); pRetData->m_int = 0; return; }
        const LkApi& a = LkGet();
        LkNameEnter();
        LkNameItem* pHit = NULL;
        for (LkNameItem* q = g_pNameList; q != NULL; q = q->pNext)
            if (lstrcmpA(q->szName, szName) == 0) { pHit = q; break; }
        if (pHit == NULL)
        {
            CxLock* pNew = LkNewLock(LK_KIND_MUTEX);
            if (pNew == NULL) { LkNameLeave(); pRetData->m_int = 0; return; }
            // 换成命名内核对象，让别的进程/后来的对象也能按名接上同一把锁
            if (a.CreateMutexA != NULL)
            {
                HANDLE hNamed = a.CreateMutexA(NULL, FALSE, szName);
                if (hNamed != NULL && a.CloseHandle != NULL && pNew->hMutex != NULL)
                { a.CloseHandle(pNew->hMutex); pNew->hMutex = hNamed; }
                else if (hNamed != NULL && a.CloseHandle != NULL) a.CloseHandle(hNamed);
            }
            LkNameItem* pItem = (LkNameItem*)LocalAlloc(LPTR, sizeof(LkNameItem));
            if (pItem == NULL) { LkFreeLock(pNew); LkNameLeave(); pRetData->m_int = 0; return; }
            lstrcpynA(pItem->szName, szName, 128);
            pItem->pLock = pNew;
            pItem->pNext = g_pNameList;
            g_pNameList = pItem;
            pHit = pItem;
        }
        const INT hRet = (INT)(INT_PTR)pHit->pLock;
        LkNameLeave();
        pRetData->m_int = hRet;
        return;
    }
    case LOP_LK_CxLock_SetHandle:
    {
        if (pArgInf == NULL || pArgInf[0].m_pCompoundData == NULL) { ReportError("对象槽无效"); return; }
        if (IsBadWritePtr(pArgInf[0].m_pCompoundData, sizeof(INT))) { ReportError("置句柄：对象槽不可写，已拒收"); return; }
        CxLock* pOld = (CxLock*)(INT_PTR)(*(PINT)pArgInf[0].m_pCompoundData);
        if (pOld != NULL && IsBadReadPtr(pOld, sizeof(INT))) pOld = NULL;      // 旧值已是野指针：不再放手，直接覆盖
        CxLock* pNew = (CxLock*)(INT_PTR)(nArgCount > 1 ? pArgInf[1].m_int : 0);
        if (pNew != NULL && (IsBadReadPtr(pNew, sizeof(INT)) || *(int*)pNew != LK_OBJ_LOCK))
        {
            ReportError("置句柄：该地址不是本库的 线程锁 对象（请用 按名取锁 或 取句柄 的返回值）");
            return;
        }
        if (pNew == pOld) return;                     // 本来就绑着同一把锁：引用数不动
        LkRetain(pNew);                               // 新锁多一个持有者（传 0 就是纯解绑）
        *(PINT)pArgInf[0].m_pCompoundData = (INT)pNew;
        LkReleaseRef(pOld);                           // 原锁少一个持有者：没人用了才真正销毁
        return;
    }
    case LOP_LK_CxLock_GetHandle:
    {
        CxLock* p = (CxLock*)LkSelfQuiet(pArgInf, LK_OBJ_LOCK);
        pRetData->m_int = p == NULL ? 0 : (INT)(INT_PTR)p;
        return;
    }

    // ---------------- 信号量 ----------------
    case LOP_LK_CxSem_Create:
    {
        if (pArgInf == NULL || pArgInf[0].m_pCompoundData == NULL) { ReportError("对象槽无效"); return; }
        if (IsBadWritePtr(pArgInf[0].m_pCompoundData, sizeof(INT))) { ReportError("信号量 对象槽不可写，已拒收"); return; }
        CxSem* p = (CxSem*)(INT_PTR)(*(PINT)pArgInf[0].m_pCompoundData);
        if (p != NULL && !IsBadReadPtr(p, sizeof(int)) && *(int*)p == LK_OBJ_SEM) { pRetData->m_bool = BL_TRUE; return; }
        p = LkNewSem(nArgCount > 1 ? pArgInf[1].m_int : 0, nArgCount > 2 ? pArgInf[2].m_int : 0);
        if (p == NULL) { pRetData->m_bool = BL_FALSE; return; }
        *(PINT)pArgInf[0].m_pCompoundData = (INT)p;
        pRetData->m_bool = BL_TRUE;
        return;
    }
    case LOP_LK_CxSem_Destroy:
        if (pArgInf != NULL && pArgInf[0].m_pCompoundData != NULL) LkFree(&pArgInf[0]);
        return;
    case LOP_LK_CxSem_Wait:
    {
        CxSem* p = (CxSem*)LkSelfAny(pArgInf, LK_OBJ_SEM);
        pRetData->m_bool = (p != NULL && LkSemWait(p, LkMs(pArgInf, nArgCount, 1))) ? BL_TRUE : BL_FALSE;
        return;
    }
    case LOP_LK_CxSem_TryWait:
    {
        CxSem* p = (CxSem*)LkSelfAny(pArgInf, LK_OBJ_SEM);
        pRetData->m_bool = (p != NULL && LkSemWait(p, 0)) ? BL_TRUE : BL_FALSE;
        return;
    }
    case LOP_LK_CxSem_Release:
    {
        CxSem* p = (CxSem*)LkSelfAny(pArgInf, LK_OBJ_SEM);
        pRetData->m_bool = (p != NULL && LkSemRelease(p)) ? BL_TRUE : BL_FALSE;
        return;
    }
    case LOP_LK_CxSem_Value:
    {
        CxSem* p = (CxSem*)LkSelfQuiet(pArgInf, LK_OBJ_SEM);
        pRetData->m_int = p == NULL ? 0 : (INT)p->nCur;
        return;
    }

    // ---------------- 条件变量 ----------------
    case LOP_LK_CxCond_Create:
    {
        if (pArgInf == NULL || pArgInf[0].m_pCompoundData == NULL) { ReportError("对象槽无效"); return; }
        if (IsBadWritePtr(pArgInf[0].m_pCompoundData, sizeof(INT))) { ReportError("条件变量 对象槽不可写，已拒收"); return; }
        CxCond* p = (CxCond*)(INT_PTR)(*(PINT)pArgInf[0].m_pCompoundData);
        if (p != NULL && !IsBadReadPtr(p, sizeof(int)) && *(int*)p == LK_OBJ_COND) { pRetData->m_bool = BL_TRUE; return; }
        p = LkNewCond();
        if (p == NULL) { pRetData->m_bool = BL_FALSE; return; }
        *(PINT)pArgInf[0].m_pCompoundData = (INT)p;
        pRetData->m_bool = BL_TRUE;
        return;
    }
    case LOP_LK_CxCond_Destroy:
        if (pArgInf != NULL && pArgInf[0].m_pCompoundData != NULL) LkFree(&pArgInf[0]);
        return;
    case LOP_LK_CxCond_Wait:
    {
        CxCond* p = (CxCond*)LkSelfAny(pArgInf, LK_OBJ_COND);
        if (p == NULL) { pRetData->m_bool = BL_FALSE; return; }
        // 第 1 参是 线程锁 对象（参考）：校验种类后传进去，等待期间会放/取这把锁
        CxLock* pLock = NULL;
        if (nArgCount > 1 && pArgInf[1].m_pCompoundData != NULL)
        {
            void* pGiven = (void*)(INT_PTR)(*(PINT)pArgInf[1].m_pCompoundData);
            if (pGiven == NULL) ReportError("等待 条件变量：传入的 线程锁 尚未创建");
            else if (IsBadReadPtr(pGiven, sizeof(int)) || *(int*)pGiven != LK_OBJ_LOCK)
                ReportError("等待 条件变量：传入的对象不是 线程锁");
            else pLock = (CxLock*)pGiven;
        }
        pRetData->m_bool = LkCondWait(p, pLock, LkMs(pArgInf, nArgCount, 2)) ? BL_TRUE : BL_FALSE;
        return;
    }
    case LOP_LK_CxCond_Signal:
    case LOP_LK_CxCond_Broadcast:
    {
        CxCond* p = (CxCond*)LkSelfAny(pArgInf, LK_OBJ_COND);
        if (p == NULL) return;
        EnterCriticalSection(&p->cs);
        LkQueueWake(&p->pHead, &p->pTail, &p->nWaiters, nOp == LOP_LK_CxCond_Broadcast ? TRUE : FALSE);
        LeaveCriticalSection(&p->cs);
        return;
    }
    case LOP_LK_CxCond_Waiters:
    {
        CxCond* p = (CxCond*)LkSelfQuiet(pArgInf, LK_OBJ_COND);
        if (p == NULL) { pRetData->m_int = 0; return; }
        EnterCriticalSection(&p->cs);
        const INT v = p->nWaiters;
        LeaveCriticalSection(&p->cs);
        pRetData->m_int = v;
        return;
    }
    default: break;
    }
    ReportError("线程锁：未知的成员命令");
}

#endif  // CXLOCK_IMPL_INC
