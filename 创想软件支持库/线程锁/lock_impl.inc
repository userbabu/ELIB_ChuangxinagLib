// 由 线程锁/gen_lock_table.mjs 生成，勿手改。
void CxLockConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    pRetData->m_int = 0; // 构造只初始化对象槽，显式创建才分配同步对象。
}

void CxLockCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    LkCopy(pArgInf, 0);
}

void CxLockDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    LkFree(&pArgInf[0]);
}

void CxLock_Create(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_Create, 0);
}

void CxLock_Destroy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_Destroy, 0);
}

void CxLock_Enter(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_Enter, 0);
}

void CxLock_Leave(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_Leave, 0);
}

void CxLock_TryEnter(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_TryEnter, 0);
}

void CxLock_Wait(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_Wait, 0);
}

void CxLock_Signal(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_Signal, 0);
}

void CxLock_Broadcast(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_Broadcast, 0);
}

void CxLock_IsLocked(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_IsLocked, 0);
}

void CxLock_Owner(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_Owner, 0);
}

void CxLock_Recursion(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_Recursion, 0);
}

void CxLock_Waiters(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_Waiters, 0);
}

void CxLock_Named(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_Named, 0);
}

void CxLock_SetHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_SetHandle, 0);
}

void CxLock_GetHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxLock_GetHandle, 0);
}

void CxSemConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    pRetData->m_int = 0; // 构造只初始化对象槽，显式创建才分配同步对象。
}

void CxSemCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    LkCopy(pArgInf, 2);
}

void CxSemDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    LkFree(&pArgInf[0]);
}

void CxSem_Create(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxSem_Create, 2);
}

void CxSem_Destroy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxSem_Destroy, 2);
}

void CxSem_Wait(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxSem_Wait, 2);
}

void CxSem_Release(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxSem_Release, 2);
}

void CxSem_TryWait(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxSem_TryWait, 2);
}

void CxSem_Value(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxSem_Value, 2);
}

void CxCondConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    pRetData->m_int = 0; // 构造只初始化对象槽，显式创建才分配同步对象。
}

void CxCondCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    LkCopy(pArgInf, 3);
}

void CxCondDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    LkFree(&pArgInf[0]);
}

void CxCond_Create(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxCond_Create, 3);
}

void CxCond_Destroy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxCond_Destroy, 3);
}

void CxCond_Wait(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxCond_Wait, 3);
}

void CxCond_Signal(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxCond_Signal, 3);
}

void CxCond_Broadcast(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxCond_Broadcast, 3);
}

void CxCond_Waiters(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxLockInvoke(pRetData, nArgCount, pArgInf, LOP_LK_CxCond_Waiters, 3);
}
