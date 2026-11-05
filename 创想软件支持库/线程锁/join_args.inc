// 由 gen_join_table.mjs 生成，勿手改。
static ARG_INFO g_joinCopyArgs[] = {
    { _T("对象"), _T("复制后共享同一个会合点；每个副本独立持有引用。"), 0, 0, MAKELONG(CX_TYPE_JOIN, 0), 0, 0 },
};
static ARG_INFO g_joinCreateArgs[] = {
    { _T("线程总数"), _T("本轮需要到达的线程总数，必须大于0；每个线程每轮调用一次等待。"), 0, 0, SDT_INT, 0, 0 },
};
