// 由 gen_join_table.mjs 生成，勿手改。
    CX_LIFECYCLE("线程会合", "线程会合", "CxJoin", g_joinCopyArgs, "人数到齐后仅最后到达者返回真，其余返回假；复制对象共享同一个会合点。"),
    CX_OBJ("创建", "CxJoin_Create", "设置本轮线程总数（必须大于0）；只能在上一轮等待全部返回后重新创建。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_joinCreateArgs),
    CX_OBJ("等待", "CxJoin_Wait", "到齐前一直等待；仅最后到达者返回真，其余返回假。取消后均返回假，本轮不会再次选出最后到达者。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    CX_OBJ("取消", "CxJoin_Cancel", "取消未到齐的一轮并唤醒所有等待者；取消成功返回真，已到齐或已取消返回假。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    CX_OBJ("销毁", "CxJoin_Destroy", "解除本变量的引用；最后一个变量销毁时取消未完成会合，等待调用返回后释放内部事件。", _CMD_OS(__OS_WIN), _SDT_NULL, 0, NULL),
    CX_OBJ("取到达数量", "CxJoin_Arrived", "本轮已经到达会合点的线程数量，供观察使用。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("是否已完成", "CxJoin_IsComplete", "线程是否已经到齐；不代表工作线程已经结束，也不代表最后一个线程的后续操作已经完成。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    CX_OBJ("是否已取消", "CxJoin_IsCancelled", "本轮是否已被取消。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
