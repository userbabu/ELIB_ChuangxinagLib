// 由 gen_http_cancel_table.mjs 生成，勿手改。
static ARG_INFO g_AccessCurl_BindStopArgs[] = { { _T("停止变量"), _T("参考传入四字节对齐的整数；跨线程原子置1取消，变量需存活到解绑且所有请求返回。"), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR } };
static ARG_INFO g_AccessCurl_BindStopPointerArgs[] = { { _T("停止变量指针"), _T("传入四字节对齐的整数变量地址；0解绑。变量需存活到解绑且所有请求返回。"), 0, 0, SDT_INT, 0, 0 } };
static ARG_INFO g_AccessWinHttp_BindStopArgs[] = { { _T("停止变量"), _T("参考传入四字节对齐的整数；跨线程原子置1取消，变量需存活到解绑且所有请求返回。"), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR } };
static ARG_INFO g_AccessWinHttp_BindStopPointerArgs[] = { { _T("停止变量指针"), _T("传入四字节对齐的整数变量地址；0解绑。变量需存活到解绑且所有请求返回。"), 0, 0, SDT_INT, 0, 0 } };
static ARG_INFO g_AccessWinInet_BindStopArgs[] = { { _T("停止变量"), _T("参考传入四字节对齐的整数；跨线程原子置1取消，变量需存活到解绑且所有请求返回。"), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR } };
static ARG_INFO g_AccessWinInet_BindStopPointerArgs[] = { { _T("停止变量指针"), _T("传入四字节对齐的整数变量地址；0解绑。变量需存活到解绑且所有请求返回。"), 0, 0, SDT_INT, 0, 0 } };
static ARG_INFO g_HttpExchangeStopArgs[] = {
    { _T("停止变量"), _T("参考传入四字节对齐且存活的整数变量。"), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR },
    { _T("值"), _T("1取消；请求全部返回后可置0复用。返回原值。"), 0, 0, SDT_INT, 0, 0 },
};
