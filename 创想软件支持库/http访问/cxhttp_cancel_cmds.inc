// 由 gen_http_cancel_table.mjs 生成，勿手改。
// CX_APPEND_OWNER HTTP访问_Curl访问类
    CX_OBJ("绑定停止变量", "AccessCurl_BindStop", "参考传入四字节对齐的整数；跨线程原子置1取消，变量需存活到解绑且所有请求返回。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_AccessCurl_BindStopArgs),
    CX_OBJ("绑定停止变量指针", "AccessCurl_BindStopPointer", "传入四字节对齐的整数变量地址；0解绑。变量需存活到解绑且所有请求返回。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_AccessCurl_BindStopPointerArgs),
    CX_OBJ("取上次请求错误码", "AccessCurl_GetLastError", "0成功；1223调用方取消；其余为curl或Windows错误码。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
// CX_APPEND_OWNER HTTP访问_WinHTTP访问类
    CX_OBJ("绑定停止变量", "AccessWinHttp_BindStop", "参考传入四字节对齐的整数；跨线程原子置1取消，变量需存活到解绑且所有请求返回。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_AccessWinHttp_BindStopArgs),
    CX_OBJ("绑定停止变量指针", "AccessWinHttp_BindStopPointer", "传入四字节对齐的整数变量地址；0解绑。变量需存活到解绑且所有请求返回。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_AccessWinHttp_BindStopPointerArgs),
    CX_OBJ("取上次请求错误码", "AccessWinHttp_GetLastError", "0成功；1223调用方取消；其余为curl或Windows错误码。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
// CX_APPEND_OWNER HTTP访问_WinInet访问类
    CX_OBJ("绑定停止变量", "AccessWinInet_BindStop", "参考传入四字节对齐的整数；跨线程原子置1取消，变量需存活到解绑且所有请求返回。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_AccessWinInet_BindStopArgs),
    CX_OBJ("绑定停止变量指针", "AccessWinInet_BindStopPointer", "传入四字节对齐的整数变量地址；0解绑。变量需存活到解绑且所有请求返回。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_AccessWinInet_BindStopPointerArgs),
    CX_OBJ("取上次请求错误码", "AccessWinInet_GetLastError", "0成功；1223调用方取消；其余为curl或Windows错误码。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
// CX_APPEND_OWNER -
    CX_CMD("HTTP_原子置停止变量", "HttpExchangeStop", "原子设置停止变量，返回原值；请求运行期间不得将已置1的变量重新置0。", 6, SDT_INT, 2, g_HttpExchangeStopArgs),
