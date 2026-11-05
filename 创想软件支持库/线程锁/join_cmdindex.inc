// 由 gen_join_table.mjs 生成，勿手改。
INT g_CxJoinCmdIndex[] = { 623, 624, 625, 626, 627, 628, 629, 630, 631, 632, 636, 637 };
static_assert(sizeof(g_CxJoinCmdIndex) / sizeof(INT) == 12, "线程会合命令数不一致");
static_assert(CX_TYPE_JOIN == 24, "线程会合数据类型序号不一致");
static_assert(sizeof(g_commands) / sizeof(CMD_INFO) >= 633, "线程会合追加命令位置不一致");
