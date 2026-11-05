// 由 线程锁/gen_lock_table.mjs 生成，勿手改。
// 线程锁：生命周期 3 + 成员 15
INT g_CxLockCmdIndex[] = { 577, 578, 579, 580, 581, 582, 583, 584, 585, 586, 587, 588, 589, 590, 591, 592, 593, 594, 633 };
static_assert(sizeof(g_CxLockCmdIndex) / sizeof(INT) == 19, "线程锁 成员命令条数变了");
static_assert(CX_TYPE_LOCK == 21, "线程锁 数据类型序号与登记顺序不一致");
// 信号量：生命周期 3 + 成员 6
INT g_CxSemCmdIndex[] = { 595, 596, 597, 598, 599, 600, 601, 602, 603, 634 };
static_assert(sizeof(g_CxSemCmdIndex) / sizeof(INT) == 10, "信号量 成员命令条数变了");
static_assert(CX_TYPE_SEM == 22, "信号量 数据类型序号与登记顺序不一致");
// 条件变量：生命周期 3 + 成员 6
INT g_CxCondCmdIndex[] = { 604, 605, 606, 607, 608, 609, 610, 611, 612, 635 };
static_assert(sizeof(g_CxCondCmdIndex) / sizeof(INT) == 10, "条件变量 成员命令条数变了");
static_assert(CX_TYPE_COND == 23, "条件变量 数据类型序号与登记顺序不一致");
static_assert(sizeof(g_commands) / sizeof(CMD_INFO) >= 613, "线程锁 族的行基址 577 与 g_commands 实际长度不符");
