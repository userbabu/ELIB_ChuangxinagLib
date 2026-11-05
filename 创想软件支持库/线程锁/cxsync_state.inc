// 查询只观察既有状态，不触发创建；与创建、绑定、复制、销毁使用同一生命周期锁。
static void SyncIsCreated(PMDATA_INF ret, PMDATA_INF args, int kind)
{
    SyncLifecycleGuard guard;
    void* p = LkSelfQuiet(args, kind);
    ret->m_bool = p != NULL ? BL_TRUE : BL_FALSE;
}
void CxLock_IsCreated(PMDATA_INF ret, INT, PMDATA_INF args) { SyncIsCreated(ret,args,LK_OBJ_LOCK); }
void CxSem_IsCreated(PMDATA_INF ret, INT, PMDATA_INF args) { SyncIsCreated(ret,args,LK_OBJ_SEM); }
void CxCond_IsCreated(PMDATA_INF ret, INT, PMDATA_INF args) { SyncIsCreated(ret,args,LK_OBJ_COND); }
