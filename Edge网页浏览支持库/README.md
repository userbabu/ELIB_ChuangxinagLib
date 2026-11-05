# Edge网页浏览支持库

原生 C++ 支持库（`.fne`）2.0.0，32 位、静态 CRT。两个对外类型：
**可视化组件「网页浏览框」**（可直接拖进窗口）与**进程级类「网页环境」**，内核是系统自带的 Edge WebView2 Runtime。
对外出口 = 108 条命名命令 + 20 个组件事件 + 140 条按名反射成员 + 60 个可按名订阅的内核事件（带参数 JSON 快照）。

约定与流程见 `docs\易语言支持库开发手册.md` 与项目 skill `epl-support-lib`。

```
lib.cpp                     全部实现：宿主窗口 + 组件接口（ITF_*）+ 命令 + 库表
lib.def                     只导出 GetNewInf（组件接口用 dllexport，见下）
edgewv.rc + res\webview.bmp 组件箱图标（24x24 24bpp，背景 RGB(192,192,192)）→ m_dwUnitBmpID=1000
include\{lib2.h,lang.h}      SDK 头，已从 GBK 转成 UTF-8（见「本机踩过的坑」）
third_party\webview2\        WebView2 SDK 1.0.4258.31：include\ 头 + x86\WebView2Loader.dll + 许可
gen_reflect.mjs                解析 WebView2.h → reflect_gen.cpp（按名读写/调用派发）
gen_events.mjs                 解析 WebView2.h → reflect_events.cpp（事件按名订阅 + 参数 JSON 快照）
gen_static_trampolines.mjs     从 g_commandFuncNames 生成静态编译转发（改命令表后必须重跑）
gen_matrix.py                  汇总对接面 → 覆盖矩阵.md（改命令/重跑生成器后同步）
reflect_events.h               事件层契约：WvEvtSubscribe / WvDescribeObject
host_test.cpp                离线宿主：装载 .fne、校验四张表、真开一个网页
build.bat / build_static.bat / build_host.bat
```

## 组件用法

1. 「工具 → 支持库配置」勾选 **Edge网页浏览支持库**（首次勾选那一次才需要重启 IDE；之后替换文件无需重启即生效）。
2. 窗口设计界面里把 **网页浏览框** 拖到窗口上，右侧属性填「网址」，运行即可显示页面。
3. 缓存目录（Edge 用户数据目录，放 cookie / 缓存 / localStorage）默认是**程序运行目录下的 `edge_cache`**；
   属性「缓存目录」可改成绝对路径、相对路径（按程序运行目录解释）或 `%APPDATA%\xxx`，
   **只能在页面第一次打开前设置**，之后属性面板会置灰。

| 属性 | 说明 |
| --- | --- |
| 网址 | 启动后打开的地址，支持 http / https / file / data |
| 缓存目录 | 留空 = 运行目录\edge_cache |
| 允许打开新窗口 | 默认假：`window.open` / `target=_blank` 被拦；真则在当前框内打开 |
| 允许开发者工具 | 默认真：F12 / 右键检查 |
| 缩放比例 | 50 / 75 / 100 / 125 / 150 / 200 % |
| 载入提示 | 默认真：初始化和跳转期间用载入状态盖住页面，显示滚动进度 + 居中「载入中...」 |

| 事件 | 触发时机 |
| --- | --- |
| 内核事件 | 任何按名订阅的 WebView2 事件都从这里出来，配 `取事件名称` / `取事件数据` |
| 网页消息 / 下载开始 / 权限请求 / 页面对话框请求 / 身份验证请求 / 客户端证书请求 / 服务器证书错误 / 右键菜单请求 / 文档结构就绪 / 请求关闭窗口 / 键盘按键 / 外部协议请求 / 另存为界面 | 高频事件，配对应的 `应答xxx` 命令回话 |
| 页面开始载入 / 页面载入完毕（是否成功） / 页面载入出错（错误码） | 导航生命周期 |
| 地址被改变 / 标题被改变 | 页面切换、`document.title` 变化 |
| 新窗口请求 | 页面请求开新窗口时（允许时先在本框打开） |

成员命令 93 条：打开网址 · 载入网页文本 · 刷新页面 · 后退 · 前进 · 停止载入 · 取页面地址 ·
取页面标题 · 取页面源码 · 是否载入中 · 能否后退 · 能否前进 · 执行脚本 · 计算脚本 · 取缓存目录 ·
设置缩放 · 打开开发者工具 · 清除缓存 · 取窗口句柄 · 是否有效 · 销毁组件 ·
**取内核属性 · 置内核属性 · 调用内核方法 · 取内核成员列表**；
另有隐含的构造/复制/析构三件套，全局命令 3 条（测试命令 · 检测网页内核 · 取网页内核版本）。
命令总数 123（全局 4 + 组件成员 108 + 环境成员 11），三张表长度与成员命令条数全部用 `static_assert` 绑死，
宿主 `CheckTables` 再核一遍（含「每个类型各且仅一个构造/复制/析构」）。
GUID：`A3CA48C25E65958618B66F2A48DA138F`（发布后不可更改）。

## 编码约定：对外文本一律 UTF-8

**所有命令的入参与返回、事件参数、脚本与页面文本都是 UTF-8**，库内不做 GBK/ANSI 转换
（与创想软件支持库同一约定）。要在 GBK 界面上显示中文，另外调 `UTF8转GBK()`。

- 内部只存一种编码：`GetExeDirectory` 走 `GetModuleFileNameW` 再转 UTF-8，缓存目录/URL 全部 UTF-8 → UTF-16 交给 WebView2。
- 唯一例外是**易语言属性面板**（IDE 给的是 GBK）：`StorePropText()` 先按 UTF-8 严格解，解不动才按本机 ANSI 转一次，
  所以面板里填中文路径也不会花屏，落库后仍是 UTF-8。
- 宿主测试里看到 `E:\dev\a鎴戠...` 这类"乱码"是正常的 —— 那是 UTF-8 字节打在 GBK 控制台上，存在性断言走 `GetFileAttributesW` 已通过。

## 反射层：把 WebView2 全部属性/长尾方法按名打通

`WebView2.h` 实测有 **273 个接口、56 个可注册事件、262 个属性**，逐条包成中文命令不现实，所以：

- `gen_reflect.mjs` 解析 SDK 头，生成 `reflect_gen.cpp`：**140 条**类型安全的按名派发
  （`get_/put_/无参或单参方法`），分 15 个目标组：内核 / 设置 / 控制器 / 环境 / 环境选项 / Cookie管理器 /
  Cookie / 配置文件 / 框架 / 下载 / 请求 / 响应 / 进程信息 / 协议事件接收器 / 证书，
  外加 `事件对象`（最近一次事件带出来的对象，逐组试派）。
  每条都显式 `QueryInterface(声明接口) → 调用 → Release`，签名由编译器校验，**不靠 vtable 序号硬算**。
- 组件命令：`取内核属性(组名, 成员名)`（按属性原形回 逻辑/整数/小数/文本，取不到回空白数据）、
  `置内核属性(组名, 成员名, 值)`、`调用内核方法(组名, 成员名[, 参数])`、`取内核成员列表(组名)`。
- 剩下 **108 条**参数类型无法干净映射的成员记在 `uncovered.txt`，逐条判定去向写进 `覆盖矩阵.md`
  （命名命令 / 事件快照 / 内部解析 / 待补），不静默丢弃。
- 内核版本不够时对应组的 `QueryInterface` 直接失败 → 命令回空白/假，不会崩。

## 事件层：60 个内核事件按名订阅 + 参数 JSON 快照

`gen_events.mjs` 把 SDK 里所有 `add_/remove_` 事件对生成成一张「组名 + 事件名 → 订阅函数」表
（`reflect_events.cpp`，实测 60 个），并给每个事件的参数对象生成一个 `Desc_` 快照函数：
逐属性展开，嵌套对象递归，列表型按 `Count + ValueAt` 展开前 32 项，统一输出 UTF-8 JSON。

- 组件侧出口：`订阅内核事件(事件名[, 组名])` / `退订内核事件` / `取可订阅事件列表([组名])`，
  事件产生时抛易语言事件「内核事件」，正文用 `取事件名称` / `取事件数据` 读，
  带对象的再用 `取事件对象快照`（下载、帧、请求、扩展…）。
- 13 个高频事件另给了**命名事件**（网页消息 / 下载开始 / 权限请求 / 页面对话框请求 / 身份验证请求 /
  客户端证书请求 / 服务器证书错误 / 右键菜单请求 / 文档结构就绪 / 请求关闭窗口 / 键盘按键 /
  外部协议请求 / 另存为界面），配对应的 `应答xxx` 命令写回选择。
- **订阅不改变默认行为**：`OnKernelEvent` 抛完事件后，若用户没应答，`WvApplyEventFallback` 会把内核
  默认值放回去（权限 → DEFAULT、证书错误 → DEFAULT）。`ScriptDialogOpening` 不在自动订阅里，
  因为一订阅就会吞掉页面弹窗，必须用户显式 `订阅内核事件("ScriptDialogOpening")`。
- 事件参数对象跨回调持有必须自己 `AddRef`（宿主按槽位存 `m_pPending[]`，`ReleaseWebview` 时统一放掉）。

## 网页环境（WebView2环境 类）

进程级环境对象，一个用户数据目录一个环境，多个浏览框可共用：

```
.局部变量 环境, 网页环境
环境.创建网页环境 ()                        // 留空 = 运行目录\edge_cache；可传内核目录 / 数据目录
环境.取环境内核版本 ()                      // 139.0.3405.86
环境.取环境进程信息列表 ()                  // JSON
浏览框1.使用网页环境 (环境)                 // 必须在页面第一次打开前绑定
浏览框2.使用网页环境 (环境)
```

成员命令 11 条：创建网页环境 · 环境是否有效 · 取环境内核版本 · 取环境缓存目录 · 取环境进程信息列表 ·
取环境属性 · 调用环境方法 · 取环境成员列表 + 构造/复制/析构三件套。复制只共享 `ICoreWebView2Environment`
引用，副本析构不影响原对象（宿主有断言）。


## 事件参数对象（2.1.0）

事件不再只给一个 JSON 串：抛「内核事件 / 网页消息 / 下载开始 / 权限请求 …」时**带一个整数参数「对象号」**，
参数对象进了库内的对象表，易语言侧按名读写：

| 命令 | 用途 |
| --- | --- |
| 取事件对象号 / 取事件对象类型 / 取事件属性列表 | 先搞清楚手上是什么对象、有哪些属性 |
| 取事件属性文本 / 整数 / 逻辑 / 小数(属性名[, 对象号]) | 按属性名读，枚举按整数出 |
| 置事件属性(属性名, 值[, 对象号]) | 通用应答通道：`Cancel` / `Handled` / `State` / `ResultFilePath` … |
| 取事件子对象(属性名[, 对象号]) | 顺着对象往下钻（下载事件里的 `DownloadOperation`、请求事件里的 `Request`） |
| 取事件列表数量 / 取事件列表成员(属性名, 序号[, 对象号]) | 列表属性（JSON 里叫 `Items`）逐项取 |
| 取得事件延后 / 完成事件延后 | `GetDeferral` / `Complete`：先返回去做别的事，回来再应答 |
| 取事件对象JSON / 释放事件对象 | 整颗快照；对象号可显式回收，组件销毁与库卸载时自动回收 |

对象号 = `(槽位 + 1) * 4096 + 代数`，槽位复用时代数递增，**旧号读不到东西而不是指错对象**。
留空对象号一律用「最近一次事件」的对象。派发层由 `gen_events.mjs` 按参数对象逐个生成，
属性读写不靠 vtable 序号。

## 库常量（2.1.0）

`gen_consts.mjs` 从 `WebView2.h` 抽全部 `enum COREWEBVIEW2_*` 与数值宏，落成 121 条库常量
（`wv_consts.h`，挂在 `LIB_INFO.m_pLibConst`）：`COREWEBVIEW2_PERMISSION_STATE_ALLOW`、
`COREWEBVIEW2_WEB_ERROR_STATUS_...`、`COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_PNG`、
`COREWEBVIEW2_WEB_RESOURCE_CONTEXT_*` 等，IDE 里直接选，不用记数字。

## 已知缺陷

- **`WebResourceRequested` 的参数快照会让宿主测试崩溃**（离线宿主里「开启请求拦截 + 订阅该事件 + 跳转」
  三步必崩，0xC0000005；已定位到该事件的参数对象快照路径，其余 59 个事件正常）。
  该事件**不在自动订阅清单里**，不主动 `订阅内核事件("WebResourceRequested")` 就不会碰到；
  宿主回归里对应断言已摘除并注明。修好后要把「子对象 + 列表」端到端断言补回去。
- 排查时踩到两件事：① `.fne` 里 `printf` 到不了宿主控制台（各自一份静态 CRT），探针要落文件；
  ② 探针字符串里写 `
` 会被写成真实换行导致 `C2001 常量中有换行符`，**构建失败但旧产物还在**，
  于是连着几轮在测旧库、误判「崩溃没修」。构建后必须确认 `[OK] built`，别只看测试输出。

## 编译与落库

```
cd libs\Edge网页浏览支持库
cmd /c build.bat            :: out\edgewv.fne → <易语根>\lib\，同时落 WebView2Loader.dll
cmd /c build_static.bat     :: out\static\edgewv_static.lib → <易语根>\static_lib\
cmd /c build_host.bat       :: 离线跑一遍库（不需要易语言）
node gen_static_trampolines.mjs   :: 改过命令表后重新生成转发段
```

宿主还有第二条用例：把 `edgewv.fne` + `host_test.exe`（**不带** `WebView2Loader.dll`）放进一个空目录，
在该目录里跑 `host_test.exe edgewv.fne noedge hint.bmp`，验的就是「探测不到内核 → 不载入、零弹窗、不崩、
只剩提示文字」，截图可直接肉眼看文案。

要求：VS 2022 BuildTools（32 位 `vcvars32.bat`）。关键开关：`/MT`、`/source-charset:utf-8 /execution-charset:gbk`、
`/D__GCC_`、`/wd4828 /wd4838 /wd4005`。

## 发布清单

- `edgewv.fne` + **`WebView2Loader.dll`（必须同目录）**；静态编译用户还要 `static_lib\edgewv_static.lib`，
  且 `WebView2Loader.dll` 要放 **EXE 同目录**。
- 第三方许可：`third_party\webview2\LICENSE.txt`、`NOTICE.txt`。
- 目标机需要 Edge WebView2 Runtime（Win10/11 一般自带）。**初始化时会先探测一次**（进程内缓存），
  探测不到就不去创建内核：组件不载入、不弹运行时错误、不闪退，框里居中显示一行灰字
  「您的电脑未安装edge浏览器，请安装后再试」，此时所有成员命令安静返回假/空。
  另两条降级文案：缺 `WebView2Loader.dll` → 「缺少 WebView2Loader.dll，请放到程序目录」；
  内核中途挂掉 → 「网页内核启动失败，请重新打开程序」。用户代码可用 `检测网页内核()` 提前判断。

## 本机踩过的坑

1. **SDK 头必须转 UTF-8**：`lib2.h` 原文是 GBK，而 build.bat 用 `/source-charset:utf-8` 读所有源文件，
   GBK 双字节会被当非法 UTF-8 再回写成执行字符集，`FIXED_WIN_UNIT_PROPERTY` 里的「左边」变成
   `0xb6 0xa5 …`，报 C2018 + C3688「文本后缀 left 无效」。纯命令库不碰这个宏所以一直没暴露。
2. **组件接口要 `dllexport`**：`.def` 只导 `GetNewInf`；`wv_CreateUnit / wv_GetInterface_WebView` 等
   用 `__declspec(dllexport)` 导出（HtmlView 范例同款），静态库按名字挑符号也靠这层。
3. **`sizeof(m_pWebview = m_pController = m_pEnvironment = NULL)`** 这类链式赋值在 C++ 里类型不一致，
   构造函数必须拆开写。
4. **hUnit 不能解引用**：运行时报的是 MFC `CWnd*`，组件销毁后那块内存随时可能被复用；
   `HostFromHunit` 只查自建登记表 + `IsWindow`，销毁时先置空 `m_hWnd/m_blDeleted`。
5. **WebView2 是异步起动的**：`CreateCoreWebView2EnvironmentWithOptions` → 控制器 → core 三段回调，
   「打开网址」在 core 就绪前只登记待跳转地址，回调里再补 `Navigate`，否则首次调用返回假。
6. **`_T()` 里别放 GBK 汉字串接**：跨宏拼接要用 `_T("纯 ASCII")` 或整串 `_T("中文")`，
   `WV_DEFAULT_CACHE_DIR "中文"` 这种窄字符 + 宽字符混排编不过。
7. **组件箱不列无图标的组件**：`lib2.h` 注释说 `m_dwUnitBmpID` 填 0 表示「无」，实测 IDE 照样装载库、
   左边树里事件命令全都在，但**组件箱里根本不出现这个组件**，等于拖不出来。补了 24x24 位图资源后才正常。
8. **属性备选串被八进制转义吃掉**：`_T("50%\075%\0100%…")` 里 `\0` 后紧跟数字，`\075` 被当八进制 `=`、
   `\010` 当退格 —— 产物里选项串全废，IDE 登记组件时拿到坏数据。写成 `_T("50%\0" "75%\0" …)` 拆开。
9. **ITF 属性接口下标不含固定属性**：`nPropertyIndex` 是「自定义属性序号」（表下标 - 8）。
   我按表下标写 `case 8: 网址`，结果改「网址」被当成改「左边」去 `MoveWindow`，
   而 `ITF_GET_PROPERTY_DATA` 只填 `m_int` 不清零 union，系统把残留整数当 `m_szText` 野指针读 →
   **在属性格里改一下就崩，异常 0xc0000005、故障模块 ntdll（堆）**。现在三个接口都按自定义下标分派，
   且 `GetPropertyData` 进门先 `memset` 整个 union；宿主按接口下标做了回归断言。
10. **遮罩要在自己的心跳里重申 Z 序**：WebView2 的视图是宿主客户区里的一个子窗口，
    只在创建遮罩时 `SetWindowPos(HWND_TOP)` 一次会被它重新压回去。现在遮罩窗口的 `WM_TIMER`（40ms）
    里每一跳都 `SetWindowPos(hWnd, HWND_TOP, …, SWP_NOMOVE|SWP_NOSIZE)`，并顺带检查载入是否已结束。
11. **本 SDK 头没有 `get_IsLoading`**：`WebView2.h` 里搜不到 `IsLoading`，「是否载入中」只能由库自己按
    `NavigationStarting` / `NavigationCompleted` / `停止载入` 维护；`Navigate()` 返回成功后立刻置位，
    否则遮罩要等异步回调才出现，会闪一下。
