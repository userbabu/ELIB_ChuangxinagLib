// 由 线程锁/gen_lock_table.mjs 生成，勿手改。
    CX_TYPE("线程锁", "CxLock", "稳定的线程锁：递归进入、超时等待、误用只报错不崩溃；条件等待与命名锁都挂在它上面。", g_CxLockCmdIndex),
    CX_TYPE("信号量", "CxSem", "计数信号量：限制并发数量、跨线程投递计数。", g_CxSemCmdIndex),
    CX_TYPE("条件变量", "CxCond", "独立条件变量：配任意 线程锁 使用，支持超时等待与广播。", g_CxCondCmdIndex),
