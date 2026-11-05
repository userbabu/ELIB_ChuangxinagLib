// 由 gen_cancel_table.mjs 生成，勿手改。
static ARG_INFO g_cancelCopyArgs[] = { { _T("对象"), _T("副本共享同一轮取消与暂停状态。"), 0, 0, MAKELONG(CX_TYPE_CANCEL, 0), 0, 0 } };
static ARG_INFO g_cancelWaitArgs[] = { { _T("超时毫秒"), _T("省略或-1无限等待，0立即检查。"), 0, 0, SDT_INT, -1, AS_HAS_DEFAULT_VALUE } };
static ARG_INFO g_bindCancelArgs[] = { { _T("令牌"), _T("引用绑定取消令牌当前轮次，内部保留引用，无需整数或指针。"), 0, 0, MAKELONG(CX_TYPE_CANCEL, 0), 0, AS_RECEIVE_VAR } };
