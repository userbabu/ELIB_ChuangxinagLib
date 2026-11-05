创想软件支持库 源码包 —— 构建与维护说明
版本 3.21.0 / 2026-11-05 / 产物 657 条命令、25 个数据类型、6 个命令类别、1384 条常量

【环境与不变量】
- 本机 VS 的 32 位 vcvars32.bat（当前 C:\Program Files\Microsoft Visual Studio\18\Enterprise）；32 位、静态 CRT(/MT)、零 CRT 导入依赖。
- 源码固定 UTF-8，编译必须带 /source-charset:utf-8 /execution-charset:gbk，
  于是标识符/注释是 UTF-8、写进 .fne 的字符串是 GBK（IDE 才显示得出中文）。
- 必须带 /D__GCC_（本机 SDK 头对 C 语言的补丁）与 /wd4828 /wd4838；链接必须带 /DEF:lib.def（只导出 GetNewInf）。
- lib.cpp 的命令行参数编组按 pack(1) 的 MDATA_INF ABI 写死：sizeof(MDATA_INF)==12，BL_TRUE==-1、BL_FALSE==0。

【目录】
  lib.cpp                     库信息 + 三张表（命令 / 实现 / 函数名）+ 网络与旧族实现；末尾是被 include 进来的生成表
  uni_*.cpp / json_*.cpp       宽文本 / 正则 / 编码转换 / Json2 各族实现
  http访问\                    HTTP访问 族（生成器 + .inc + 常量表 + 规格 + 易语言验收代码）
  线程锁\                      线程锁 / 信号量 / 条件变量 族（生成器 + .inc + 易语言验收代码）
  include\                     从 <易语言根>\sdk\cpp\elib 拷来的 lib2.h / lang.h（lib2.h 已打 __GCC_ 补丁）
  third_party\                 libcurl-impersonate 32 位包（include / lib / LICENSE*）
  out\                         编译产物，不入库；临时脚本与验证文件放项目 temp\

【构建顺序】
1. 改过任何一族的命令表/参数表，先重跑该族的生成器：
     node http访问/gen_http_table.mjs      # 同时生成末尾追加的停止绑定接口表
     node 线程锁/gen_lock_table.mjs 577    # 行基址 = 该族第一行在 g_commands 里的下标
     node 线程锁/gen_join_table.mjs        # 线程会合及同步类创建状态追加表
   再跑：
     node gen_static_trampolines.mjs
   它做两件事：按 g_commandFuncNames 生成 extern "C" 转发（静态编译按函数名挑符号）；
   按 g_commands 重算所有类成员下标表 g_*CmdIndex（表的中间删/插一条会把后面整体挪位）。
   新加一族时要登记进它的 genFiles / srcX 内联列表，否则会静默少一族转发（打印的转发数比命令总数小）。
2. cmd /c build_local.bat   只编到 out\，不落库（自查用）
   cmd /c publish.bat       动态 + 静态一次做完并落库：<易语言根>\lib\ 与 <易语言根>\static_lib\，
                            同时拷 third_party\lib\libcurl-impersonate.dll；
                            IDE 开着会锁 .fne，脚本会重试后暂存成 目标.new 并打 [PENDING]。
3. cmd /c build_host.bat    依次编译并运行六个离线宿主（不占 IDE，任一失败就退出码非 0）：
     host_const   .fne 自报的常量表 vs MSVC 编译探针，逐条核值
     host_cover   全命令覆盖：读 ARG_INFO 自动造实参，每条命令真调一次，断言 不崩 / 返回类型与声明一致 /
                  无运行时错误 / 数组返回块格式正确，并核对「跑过或有理由跳过 == 命令总数」
     host_uni     宽文本 / 正则 / 编码转换 / 数组返回的逐条语义比对（期望值用 Win32 现算）
     host_json    Json2 115 条成员命令语义断言 + 成员命令全覆盖
     host_test    真发 HTTP 请求（指纹复用、会话/请求/响应、访问类、全局命令）
     host_lock    线程锁族多线程真跑：8 线程 × 20000 轮非原子累加不丢数、跨线程 等待/信号、超时、
                  命名锁同址、假句柄拒收、销毁后重建、信号量限并发、条件变量广播
   只改某个宿主时用 run_uni.bat / run_cover.bat / run_json.bat / run_lock_host.bat 单跑。
4. cmd /c dump_static.bat   导出静态库符号与 UNDEF 供体检（build_static.bat 里已内置 UNDEF 断言）。
   node ..\..\.qoder\skills\epl-support-lib\scripts\check-fne.mjs out\cxsoft.fne  做 PE 体检。

【必须记住的不变量】
- 命令只往末尾追加，编号永不重排；GUID CADE64471347478381C0C610A22C47D0 发布后不可改；改命令/类/常量要升版本号。
- 三张表（命令 / 实现 / 函数名）同点同序改：static_assert 只能绑住「同长」，顺序错位要靠离线宿主按中文名逐条断言语义才发现。
- 每个类恰好 1 条构造 / 1 条复制 / 1 条析构，成员命令 m_shtCategory = -1 且下标进 m_pnCmdsIndex，实现里 pArgInf[0] 是对象自身。
- CX_OBJ(...) 的第 4 个参数是 m_wState 不是类别，必须写 _CMD_OS(__OS_WIN)；误写 -1 会给每条成员命令带上构造/析构标志。
- 返回文本/字节集一律 NotifySys(NRS_MALLOC)；无返回值(_SDT_NULL)的命令不许回填 m_dtDataType。
- 参数可空 = AS_DEFAULT_VALUE_IS_EMPTY，它与 AS_HAS_DEFAULT_VALUE 互斥；参考参数（AS_RECEIVE_VAR）交出的 union 是变量地址，要二次解栈。
- 只读不回写的入参不要标 AS_RECEIVE_VAR / AS_RECEIVE_VAR_ARRAY：标了 IDE 就要求必须先声明变量，而实现按值读时拿到的是地址（判值永远不成立）。数组入参用 AS_RECEIVE_ARRAY_DATA。
- 库定义数据类型的 析构函数 收到的是实参本身，对象槽 = 它的 m_pCompoundData；复制函数是 pArgInf[0] 为源、pArgInf[1] 为目标。
- 运行期解析的系统函数，函数指针 typedef 必须带 WINAPI（stdcall），否则进内核 DLL 直接崩。
- 对外接口文本一律 UTF-8 原样进出（编码转换族的代码页参数是口径例外）。
- 静态库要能在 VC6 链接器下过：编译加 /GS-，链接前用 strip_drectve.mjs 抹掉新工具链的 .drectve 指令；
  double↔__int64 互转要插垫片；静态分支补的 operator new/delete 必须走 malloc/free（跨堆释放会让 EXE 一启动就崩）。
- 批处理与临时探针保持纯 ASCII 且 CRLF；cl 的对象目录写 /Foout\；宿主编译 flags 必须统一（别单独特造一行 /EHsc 版）。
- 静态编译出的 EXE 不再需要 .fne，但 libcurl-impersonate.dll 必须放 EXE 同目录（线程锁族只用 kernel32，无额外依赖）。

【易语言侧真机验收代码】
  http访问\测试源码.txt   → 结果写 程序目录\cxhttp_selftest.txt（常量带 # 引用）
  线程锁\测试源码.txt     → 结果写 程序目录\cxlock_selftest.txt（建议先勾 程序属性→忽略运行错误）

取消与暂停令牌：线程锁\cxcancel_core.inc，元数据生成器 线程锁\gen_cancel_table.mjs；HTTP访问类保留令牌引用。修改表后运行 node 线程锁\gen_cancel_table.mjs 和 node gen_static_trampolines.mjs。host_cancel.cpp 已纳入 build_host.bat，在本地服务器上运行真实取消、暂停、复制及并发回收验证。
