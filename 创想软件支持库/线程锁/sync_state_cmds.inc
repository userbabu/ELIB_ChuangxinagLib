// 由 gen_sync_state_table.mjs 生成，勿手改。
// CX_APPEND_OWNER 线程锁
    CX_OBJ("是否已创建", "CxLock_IsCreated", "线程安全地查询是否已创建；未创建或销毁后返回假，查询本身不创建对象。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
// CX_APPEND_OWNER 信号量
    CX_OBJ("是否已创建", "CxSem_IsCreated", "线程安全地查询是否已创建；未创建或销毁后返回假，查询本身不创建对象。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
// CX_APPEND_OWNER 条件变量
    CX_OBJ("是否已创建", "CxCond_IsCreated", "线程安全地查询是否已创建；未创建或销毁后返回假，查询本身不创建对象。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
// CX_APPEND_OWNER 线程会合
    CX_OBJ("是否已创建", "CxJoin_IsCreated", "线程安全地查询是否已创建；未创建或销毁后返回假，查询本身不创建对象。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
// CX_APPEND_OWNER 线程会合
    CX_OBJ("确保创建", "CxJoin_EnsureCreated", "原子检查并按需配置会合；已配置直接返回真，不重置当前轮次，取消或完成后也不重新开启。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_syncEnsureJoinArgs),
// CX_APPEND_OWNER -
