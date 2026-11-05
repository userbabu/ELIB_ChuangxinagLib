// 由 gen_cancel_table.mjs 生成，勿手改。
    CX_LIFECYCLE("取消令牌", "取消令牌", "CxCancel", g_cancelCopyArgs, "构造后即可使用；创建用于重新开始。支持共享取消、暂停、恢复，复制共享当前状态，可绑定多个HTTP访问类。"),
    CX_OBJ("创建", "CxCancel_Create", "每次调用都重新开始，生成未取消、未暂停的新状态。先等旧任务结束，再为访问类重新绑定令牌。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    CX_OBJ("取消", "CxCancel_Cancel", "取消本轮共享任务和绑定请求，并唤醒暂停等待；重复取消返回真。需要重新开始时调用创建。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    CX_OBJ("暂停", "CxCancel_Pause", "暂停共享本轮的协作检查点；已取消返回假。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    CX_OBJ("恢复", "CxCancel_Resume", "恢复暂停的本轮任务；本轮已取消时返回假，需要调用创建重新开始。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    CX_OBJ("是否已取消", "CxCancel_IsCancelled", "线程安全查询本轮是否已取消。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    CX_OBJ("是否已暂停", "CxCancel_IsPaused", "线程安全查询本轮是否暂停；取消后返回假。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    CX_OBJ("等待取消", "CxCancel_WaitCancel", "等待取消，超时返回假；省略或-1无限等待，0只查询。可用作可取消延时。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_cancelWaitArgs),
    CX_OBJ("等待继续", "CxCancel_WaitContinue", "运行时返回真；暂停时等待恢复，取消或超时返回假。省略或-1无限等待。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_cancelWaitArgs),
    CX_OBJ("是否已创建", "CxCancel_IsCreated", "查询是否持有有效状态；取消或暂停后仍返回真，查询本身不会创建或重置状态。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    CX_OBJ("销毁", "CxCancel_Destroy", "释放本变量引用，不强杀线程、不自动取消；绑定请求和其他副本保留其状态。", _CMD_OS(__OS_WIN), _SDT_NULL, 0, NULL),
// CX_APPEND_OWNER HTTP访问_Curl访问类
    CX_OBJ("绑定取消令牌", "AccessCurl_BindToken", "绑定令牌当前状态；令牌调用创建后需要重新绑定。内部保留引用并替换整数绑定，请求期间改绑会等待返回，回调内禁止改绑。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_bindCancelArgs),
    CX_OBJ("解绑取消令牌", "AccessCurl_UnbindToken", "解除令牌绑定，不取消令牌，也不影响其他请求；请求期间调用会等待返回，不能用来停止请求。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
// CX_APPEND_OWNER HTTP访问_WinHTTP访问类
    CX_OBJ("绑定取消令牌", "AccessWinHttp_BindToken", "绑定令牌当前状态；令牌调用创建后需要重新绑定。内部保留引用并替换整数绑定，请求期间改绑会等待返回，回调内禁止改绑。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_bindCancelArgs),
    CX_OBJ("解绑取消令牌", "AccessWinHttp_UnbindToken", "解除令牌绑定，不取消令牌，也不影响其他请求；请求期间调用会等待返回，不能用来停止请求。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
// CX_APPEND_OWNER HTTP访问_WinInet访问类
    CX_OBJ("绑定取消令牌", "AccessWinInet_BindToken", "绑定令牌当前状态；令牌调用创建后需要重新绑定。内部保留引用并替换整数绑定，请求期间改绑会等待返回，回调内禁止改绑。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_bindCancelArgs),
    CX_OBJ("解绑取消令牌", "AccessWinInet_UnbindToken", "解除令牌绑定，不取消令牌，也不影响其他请求；请求期间调用会等待返回，不能用来停止请求。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
// CX_APPEND_OWNER -
