// Edge网页浏览支持库 —— 原生 C++ 支持库（.fne），32 位、静态 CRT、GBK 输出。
// 源码 UTF-8 保存，由 /source-charset:utf-8 /execution-charset:gbk 转出 GBK 产物。
//
// 内容：可视化窗口组件「网页浏览框」，内核是系统自带的 Edge WebView2 Runtime。
// 组件从工具箱拖进窗口即可用，属性里填网址就显示页面；缓存（用户数据目录）默认落在
// 程序运行目录下的 edge_cache，也可指定任意可写目录（支持相对路径与 %变量%）。
//
// 文件分四段：① 基础设施 ② WebView2 宿主窗口 ③ 组件对外接口（ITF_*）④ 命令与库表。
// 静态编译只需要命令实现，表与库信息用 __E_STATIC_LIB 排除，见文件末尾转发段。
// 约定见 docs\易语言支持库开发手册.md 与项目 skill epl-support-lib。

// 包含前奏与 reflect_gen.cpp 共用，见 wv_pre.h。
// WebView2 只用接口定义，实现靠运行时加载 WebView2Loader.dll，避免 .fne 产生导入依赖。
#include "wv_pre.h"
#include "reflect.h"
#include "reflect_events.h"

#define WV_LIB_NAME            _T("Edge网页浏览支持库")
#define WV_NOTIFY_FUNC_NAME    "edgewv_ProcessNotifyLib"
#define WV_GUID                _T("A3CA48C25E65958618B66F2A48DA138F")   // 发布后不可更改

#define WV_HOST_CLASS          "EdgeWv_WebViewHost"
#define WV_OVERLAY_CLASS       "EdgeWv_LoadingOverlay"
#define WV_OVERLAY_TIMER       ((UINT_PTR)0x5751)
#define WV_MARQUEE_BLOCK_W     48
#define WV_MARQUEE_TRACK_W     240
#define WV_LOADING_TICK_MS     40
#define WV_LOADING_TEXT        "载入中..."
#define WV_PROP_NAME           "EdgeWvHost"          // 宿主窗口属性表里存的指针对象
#define WV_LOADER_DLL          "WebView2Loader.dll"
#define WV_DEFAULT_CACHE_DIR   "edge_cache"
#define WV_DEFAULT_CACHE_DIR_W _T("edge_cache")

// 组件载入不了时在框里画的提示（宁可显示文字，也不弹窗、不闪退）
#define WV_HINT_NO_EDGE     "您的电脑未安装edge浏览器，请安装后再试"
#define WV_HINT_NO_LOADER   "缺少 WebView2Loader.dll，请放到程序目录"
#define WV_HINT_START_FAIL  "网页内核启动失败，请重新打开程序"
#define WV_LIB_DESIGN_TIP   "Edge 网页浏览"

#define WV_TEXT_MAX            4096                  // 序列化文本属性缓冲区（字节，UTF-8）
#define WV_TYPE_PAGEVIEW       1                     // m_pDataType 里的序号，从 1 开始
#define WV_DT_PAGEVIEW         MAKELONG(WV_TYPE_PAGEVIEW, 0)
#define WV_TYPE_ENV              2                     // WebView2环境
#define WV_DT_ENV                MAKELONG(WV_TYPE_ENV, 0)
#define WV_UNIT_BMP_ID         1000                  // 工具箱图标资源 ID，须与 edgewv.rc 一致

// 组件属性序号：前 8 项是系统固定属性，顺序与名称都不能改。
#define WVPROP_LEFT            0
#define WVPROP_TOP             1
#define WVPROP_WIDTH           2
#define WVPROP_HEIGHT          3
#define WVPROP_TAG             4
#define WVPROP_VISIBLE         5
#define WVPROP_DISABLE         6
#define WVPROP_MOUSE_POINTER   7
#define WVPROP_CUSTOM_FIRST    8                     // 以下由库自己维护，接口下标 = 表下标 - 8
#define WVPROP_ADDRESS         8                     // 网址
#define WVPROP_CACHE_FOLDER    9                     // 缓存目录
#define WVPROP_NEW_WINDOW      10                    // 允许打开新窗口
#define WVPROP_DEV_TOOLS       11                    // 允许开发者工具
#define WVPROP_ZOOM            12                    // 缩放比例
#define WVPROP_LOADING         13                    // 载入提示
#define WVPROP_COUNT           14

// 事件序号（决定 IDE 里事件列表的下标，只能往末尾追加）
#define WVEVENT_LOAD_START     0                    // 页面开始载入
#define WVEVENT_LOAD_DONE      1                    // 页面载入完毕
#define WVEVENT_TITLE_CHANGED  2                    // 标题被改变
#define WVEVENT_ADDRESS_CHANGED 3                   // 地址被改变
#define WVEVENT_NEW_WINDOW     4                    // 新窗口请求
#define WVEVENT_PAGE_ERROR     5                    // 页面载入出错
#define WVEVENT_KERNEL_EVENT   6                    // 内核事件（通用）
#define WVEVENT_WEB_MESSAGE    7                    // 网页消息
#define WVEVENT_DOWNLOAD       8                    // 下载开始
#define WVEVENT_PERMISSION     9                    // 权限请求
#define WVEVENT_DIALOG         10                   // 页面对话框请求
#define WVEVENT_BASE_AUTH      11                   // 身份验证请求
#define WVEVENT_CLIENT_CERT    12                   // 客户端证书请求
#define WVEVENT_SERVER_CERT    13                   // 服务器证书错误
#define WVEVENT_CONTEXT_MENU   14                   // 右键菜单请求
#define WVEVENT_DOM_READY      15                   // 文档结构就绪
#define WVEVENT_CLOSE_REQ      16                   // 请求关闭窗口
#define WVEVENT_KEY            17                   // 键盘按键
#define WVEVENT_EXT_URI        18                   // 外部协议请求
#define WVEVENT_SAVEAS_UI      19                   // 另存为界面
#define WVEVENT_COUNT          20

#ifdef __E_STATIC_LIB
// 易语言静态编译会带 nafxcw.lib，它已经定义了普通 new / delete（再定义就是 LNK2005）；
// 但它没有数组版和带长度的 delete，这几个得我们自己补。
// 必须走 malloc/free（链接期解析到 VC6 的 libcmt，与 nafxcw 的 new 同一个堆）：
// 接进程堆的话，new 来的块被 delete 时就是跨堆释放，RtlFreeHeap 直接报无效地址崩掉。
void* operator new[](size_t n) { return malloc(n ? n : 1); }
void  operator delete[](void* p) throw() { free(p); }
void  operator delete(void* p, size_t) throw() { free(p); }
void  operator delete[](void* p, size_t) throw() { free(p); }
#endif

namespace wv {

// ============================================================ ① 基础设施

PFN_NOTIFY_SYS g_pfnNotifySys = NULL;
HMODULE        g_hModule = NULL;

INT NotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    if (g_pfnNotifySys == NULL) return 0;
    return g_pfnNotifySys(nMsg, dwParam1, dwParam2);
}

void ReportError(LPCSTR szErrText)
{
    if (g_pfnNotifySys == NULL || szErrText == NULL) return;
    NotifySys(NRS_RUNTIME_ERR, (DWORD)szErrText, 0);
}

// 在易语言内存池上建立文本数据，返回后由系统释放。
// 必须走 NRS_MALLOC：返回静态缓冲区或 LocalAlloc 指针会让系统释放时踩坏堆。
char* CloneTextData(LPCSTR szUtf8Text)
{
    if (szUtf8Text == NULL || g_pfnNotifySys == NULL) return NULL;

    const INT nLen = (INT)lstrlenA(szUtf8Text);
    if (nLen <= 0) return NULL;

    char* pd = (char*)NotifySys(NRS_MALLOC, (DWORD)(nLen + 1), 0);
    if (pd == NULL) return NULL;

    memcpy(pd, szUtf8Text, nLen);
    pd[nLen] = '\0';
    return pd;
}

void SetRetText(PMDATA_INF pRet, LPCSTR szUtf8OrNull)
{
    pRet->m_dtDataType = SDT_TEXT;
    pRet->m_pText = (szUtf8OrNull != NULL) ? CloneTextData(szUtf8OrNull) : NULL;
}

// 字节集：INT 维数(=1) + INT 成员数 + 数据本体，内存必须来自易语言内存池。
LPBYTE CloneBinData(const void* pData, INT nLen)
{
    if (nLen < 0) nLen = 0;
    LPBYTE p = (LPBYTE)NotifySys(NRS_MALLOC, (DWORD)(sizeof(INT) * 2 + nLen), 0);
    if (p == NULL) return NULL;
    *(INT*)p = 1;
    *(INT*)(p + sizeof(INT)) = nLen;
    if (nLen > 0 && pData != NULL) memcpy(p + sizeof(INT) * 2, pData, (size_t)nLen);
    return p;
}

void SetRetBin(PMDATA_INF pRet, const void* pData, INT nLen)
{
    pRet->m_dtDataType = SDT_BIN;
    pRet->m_pBin = CloneBinData(pData, nLen);
}

// IStream <-> 内存：截图取回、应答请求正文都要过流。
LPBYTE StreamToBin(IStream* pStream, INT* pnLen)
{
    if (pnLen != NULL) *pnLen = 0;
    if (pStream == NULL || pnLen == NULL) return NULL;
    HGLOBAL hMem = NULL;
    if (FAILED(GetHGlobalFromStream(pStream, &hMem)) || hMem == NULL) return NULL;
    const SIZE_T sz = GlobalSize(hMem);
    LPBYTE pSrc = (LPBYTE)GlobalLock(hMem);
    if (pSrc == NULL) return NULL;
    LPBYTE pCopy = (LPBYTE)malloc(sz == 0 ? 1 : (size_t)sz);
    if (pCopy != NULL) memcpy(pCopy, pSrc, (size_t)sz);
    GlobalUnlock(hMem);
    *pnLen = (INT)sz;
    return pCopy;
}

IStream* BinToStream(const void* pData, INT nLen)
{
    IStream* pStream = NULL;
    if (FAILED(CreateStreamOnHGlobal(NULL, TRUE, &pStream)) || pStream == NULL) return NULL;
    if (nLen > 0 && pData != NULL)
    {
        ULONG nWritten = 0;
        if (FAILED(pStream->Write(pData, (ULONG)nLen, &nWritten))) { pStream->Release(); return NULL; }
    }
    return pStream;
}

void SetRetBool(PMDATA_INF pRet, BOOL blValue)
{
    pRet->m_dtDataType = SDT_BOOL;
    pRet->m_bool = blValue ? TRUE : FALSE;
}

void SetRetInt(PMDATA_INF pRet, INT nValue)
{
    pRet->m_dtDataType = SDT_INT;
    pRet->m_int = nValue;
}

// 本库所有对外文本一律 UTF-8：进来的是 UTF-8，出去的也是 UTF-8，不做 ANSI/GBK 转换。
// （与创想软件支持库同一约定；要在 GBK 界面上显示中文，请另外调 UTF8转GBK。）

// UTF8 -> UTF16，缓冲区不足时按完整字符截断，保证结尾 0。
void Utf8ToWide(LPCSTR szUtf8, LPWSTR pwszDst, INT nDstChars)
{
    if (pwszDst == NULL || nDstChars <= 0) return;
    pwszDst[0] = L'\0';
    if (szUtf8 == NULL) return;
    if (MultiByteToWideChar(CP_UTF8, 0, szUtf8, -1, pwszDst, nDstChars) != 0) return;

    INT nLen = (INT)lstrlenA(szUtf8);
    while (nLen > 0 && MultiByteToWideChar(CP_UTF8, 0, szUtf8, nLen, pwszDst, nDstChars - 1) == 0) nLen--;
    MultiByteToWideChar(CP_UTF8, 0, szUtf8, nLen, pwszDst, nDstChars - 1);
    pwszDst[nDstChars - 1] = L'\0';
}

// UTF16 -> UTF8，缓冲区不足时截断，保证结尾 0。
void WideToUtf8(LPCWSTR pwszWide, LPSTR szDst, INT nDstCap)
{
    if (szDst == NULL || nDstCap <= 0) return;
    szDst[0] = '\0';
    if (pwszWide == NULL) return;
    if (WideCharToMultiByte(CP_UTF8, 0, pwszWide, -1, szDst, nDstCap, NULL, NULL) != 0) return;

    INT nLen = (INT)lstrlenW(pwszWide);
    while (nLen > 0 && WideCharToMultiByte(CP_UTF8, 0, pwszWide, nLen, szDst, nDstCap - 1, NULL, NULL) == 0) nLen--;
    WideCharToMultiByte(CP_UTF8, 0, pwszWide, nLen, szDst, nDstCap - 1, NULL, NULL);
    szDst[nDstCap - 1] = '\0';
}

// 把外部文本规范化成内部 UTF-8 存进定长缓冲：本身是合法 UTF-8 的原样收；
// 不是（易语言属性面板给的是 GBK）就按本机 ANSI 转一次。库内只存 UTF-8。
void StorePropText(LPSTR szDst, INT nDstCap, LPCSTR szSrc)
{
    if (szDst == NULL || nDstCap <= 0) return;
    szDst[0] = '\0';
    if (szSrc == NULL) return;

    WCHAR wz[4096];
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szSrc, -1, wz, 4096) != 0)
    {
        lstrcpynA(szDst, szSrc, nDstCap);
        return;
    }
    if (MultiByteToWideChar(CP_ACP, 0, szSrc, -1, wz, 4096) == 0)
    {
        lstrcpynA(szDst, szSrc, nDstCap);
        return;
    }
    WideToUtf8(wz, szDst, nDstCap);
}

// 属性面板里填的是易语言文本（GBK），只有这一条路径需要 ANSI 兜底：
// 先按 UTF-8 严格解，解不动再按本机 ANSI 解，两边都不会花屏。
void AnsiOrUtf8ToWide(LPCSTR szText, LPWSTR pwszDst, INT nDstChars)
{
    if (pwszDst == NULL || nDstChars <= 0) return;
    pwszDst[0] = L'\0';
    if (szText == NULL) return;
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szText, -1, pwszDst, nDstChars) != 0) return;
    pwszDst[0] = L'\0';
    MultiByteToWideChar(CP_ACP, 0, szText, -1, pwszDst, nDstChars);
}

// 取执行程序所在目录（UTF-8，含结尾反斜杠）。
void GetExeDirectory(LPSTR szDst, INT nDstCap)
{
    if (szDst == NULL || nDstCap <= 0) return;
    szDst[0] = '\0';

    // 本库内部只承认 UTF-8，所以一律走宽字符 API 再转，绝不让 ANSI 路径混进来。
    WCHAR wzPath[MAX_PATH];
    wzPath[0] = L'\0';
    BOOL blGot = FALSE;

    // 运行环境下 NAS_GET_PATH 的 2001 = 用户 EXE 所在目录（给的是本机码页文本，转一次）
    if (g_pfnNotifySys != NULL)
    {
        CHAR szAnsi[MAX_PATH];
        szAnsi[0] = '\0';
        if (NotifySys(NAS_GET_PATH, 2001, (DWORD)szAnsi) > 0 && szAnsi[0] != '\0')
        {
            AnsiOrUtf8ToWide(szAnsi, wzPath, MAX_PATH);
            blGot = wzPath[0] != L'\0';
        }
    }
    if (!blGot && GetModuleFileNameW(NULL, wzPath, MAX_PATH) > 0 && wzPath[0] != L'\0')
    {
        LPWSTR pszLast = wcsrchr(wzPath, L'\\');
        if (pszLast != NULL)
        {
            pszLast[1] = L'\0';
            blGot = TRUE;
        }
    }
    if (!blGot) return;
    WideToUtf8(wzPath, szDst, nDstCap);
}

// ------------------------------------------------------------ WebView2Loader

typedef HRESULT (STDMETHODCALLTYPE *PFN_WV_CREATE_ENV) (PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions*,
                                                        ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);
typedef HRESULT (STDMETHODCALLTYPE *PFN_WV_GET_VERSION) (PCWSTR, LPWSTR*);
typedef HRESULT (STDMETHODCALLTYPE *PFN_WV_COMPARE_VERSIONS) (PCWSTR, PCWSTR, int*);

PFN_WV_CREATE_ENV          g_pfnCreateEnv  = NULL;
PFN_WV_GET_VERSION         g_pfnGetVersion = NULL;
PFN_WV_COMPARE_VERSIONS    g_pfnCompareVersions = NULL;
WCHAR              g_wzRuntimeVersion[64] = { 0 };   // 探测到的内核版本号，空表示不可用
HMODULE            g_hLoaderModule = NULL;

CRITICAL_SECTION g_wvCs;
BOOL             g_wvCsInited = FALSE;

void EnsureCs()
{
    if (!g_wvCsInited)
    {
        InitializeCriticalSection(&g_wvCs);
        g_wvCsInited = TRUE;
    }
}

// 查找顺序：.fne 所在目录 → 宿主 EXE 目录 → 系统路径。
BOOL LoadWebViewLoader()
{
    if (g_pfnCreateEnv != NULL) return TRUE;
    EnsureCs();
    EnterCriticalSection(&g_wvCs);
    if (g_pfnCreateEnv != NULL) { LeaveCriticalSection(&g_wvCs); return TRUE; }

    HMODULE hMod = NULL;
    TCHAR szTry[MAX_PATH * 4];

    if (GetModuleFileName(g_hModule, szTry, (DWORD)(sizeof(szTry) / sizeof(szTry[0]))) > 0)
    {
        LPTSTR pszLast = _tcsrchr(szTry, _T('\\'));
        if (pszLast != NULL)
        {
            pszLast[1] = _T('\0');
            _tcscat(szTry, _T(WV_LOADER_DLL));
            hMod = LoadLibrary(szTry);
        }
    }
    if (hMod == NULL && GetModuleFileName(NULL, szTry, (DWORD)(sizeof(szTry) / sizeof(szTry[0]))) > 0)
    {
        LPTSTR pszLast = _tcsrchr(szTry, _T('\\'));
        if (pszLast != NULL)
        {
            pszLast[1] = _T('\0');
            _tcscat(szTry, _T(WV_LOADER_DLL));
            hMod = LoadLibrary(szTry);
        }
    }
    if (hMod == NULL) hMod = LoadLibrary(_T(WV_LOADER_DLL));

    if (hMod != NULL)
    {
        g_pfnCreateEnv = (PFN_WV_CREATE_ENV)GetProcAddress(hMod, "CreateCoreWebView2EnvironmentWithOptions");
        if (g_pfnCreateEnv != NULL)
        {
            g_pfnGetVersion = (PFN_WV_GET_VERSION)GetProcAddress(hMod, "GetAvailableCoreWebView2BrowserVersionString");
            g_pfnCompareVersions = (PFN_WV_COMPARE_VERSIONS)GetProcAddress(hMod, "CompareBrowserVersions");
            g_hLoaderModule = hMod;
        }
        else
        {
            FreeLibrary(hMod);
            hMod = NULL;
        }
    }
    LeaveCriticalSection(&g_wvCs);
    return g_pfnCreateEnv != NULL;
}

BOOL EnsureCom()
{
    const HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    return SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
}

// 系统里到底有没有 Edge WebView2 运行环境：探测一次就缓存，之后所有组件共用结论。
// 返回假表示不该再尝试创建内核，组件退化成一段提示文字。
BOOL ProbeRuntime()
{
    static BOOL s_blProbed = FALSE;
    static BOOL s_blOk = FALSE;
    static WCHAR s_wzVersion[64];

    if (s_blProbed)
    {
        if (s_wzVersion[0] != L'\0' && g_wzRuntimeVersion[0] == L'\0')
            lstrcpynW(g_wzRuntimeVersion, s_wzVersion, (INT)(sizeof(g_wzRuntimeVersion) / sizeof(WCHAR)));
        return s_blOk;
    }
    s_blProbed = TRUE;

    if (!LoadWebViewLoader()) return FALSE;

    if (g_pfnGetVersion != NULL)
    {
        LPWSTR pwsz = NULL;
        if (g_pfnGetVersion(NULL, &pwsz) == S_OK && pwsz != NULL && pwsz[0] != L'\0')
        {
            lstrcpynW(s_wzVersion, pwsz, (INT)(sizeof(s_wzVersion) / sizeof(WCHAR)));
            CoTaskMemFree(pwsz);
            s_blOk = TRUE;
        }
    }
    else
    {
        // 老版 loader 没有版本查询接口，只能等真正创建环境时再判定。
        s_blOk = TRUE;
    }

    if (s_wzVersion[0] != L'\0')
        lstrcpynW(g_wzRuntimeVersion, s_wzVersion, (INT)(sizeof(g_wzRuntimeVersion) / sizeof(WCHAR)));
    return s_blOk;
}

// ------------------------------------------------------------ 有效性锚
// 宿主窗口销毁后，在途的 WebView2 异步回调必须安全失效，不能碰已释放的对象。

struct WV_ALIVE
{
    volatile LONG m_ref;       // 窗口 + 每个在途回调各持一份
    volatile LONG m_alive;
};

WV_ALIVE* AliveNew()
{
    WV_ALIVE* p = new WV_ALIVE;
    p->m_ref = 1;
    p->m_alive = 1;
    return p;
}

void AliveAddRef(WV_ALIVE* p) { if (p != NULL) InterlockedIncrement(&p->m_ref); }

void AliveRelease(WV_ALIVE* p)
{
    if (p == NULL) return;
    if (InterlockedDecrement(&p->m_ref) == 0) delete p;
}

void AliveKill(WV_ALIVE* p) { if (p != NULL) InterlockedExchange(&p->m_alive, 0); }

BOOL AliveOk(WV_ALIVE* p) { return p != NULL && p->m_alive != 0; }

// ------------------------------------------------------------ 环境缓存
// 同一用户数据目录只能有一个环境（多个浏览框共享同一个浏览器进程），按目录复用。

struct WV_ENV_ITEM
{
    WCHAR                     m_wzFolder[MAX_PATH * 4];
    ICoreWebView2Environment* m_pEnvironment;
    INT                       m_nUsers;      // 正在用这个环境的浏览框数量
};

WV_ENV_ITEM g_envCache[8];
INT         g_envCacheCount = 0;

ICoreWebView2Environment* EnvCacheFind(LPCWSTR pwszFolder)
{
    ICoreWebView2Environment* pFound = NULL;
    EnsureCs();
    EnterCriticalSection(&g_wvCs);
    for (INT i = 0; i < g_envCacheCount; i++)
    {
        if (g_envCache[i].m_pEnvironment == NULL || lstrcmpW(g_envCache[i].m_wzFolder, pwszFolder) != 0)
            continue;
        // 环境会随最后一个视图关闭而失效，本 SDK 没有 IsDisposed 可查，
        // 所以靠用户数归零时丢条目（见 EnvCacheRelease），这里命中即可直接用。
        g_envCache[i].m_nUsers++;
        pFound = g_envCache[i].m_pEnvironment;
        pFound->AddRef();
        break;
    }
    LeaveCriticalSection(&g_wvCs);
    return pFound;
}

// 浏览框放手：最后一个使用者退出时把缓存条目一并释放，
// 否则下次拿到的是已经随浏览器进程一起死掉的旧环境。
void EnvCacheRelease(LPCWSTR pwszFolder)
{
    if (pwszFolder == NULL || pwszFolder[0] == L'\0') return;
    if (!g_wvCsInited) return;
    EnterCriticalSection(&g_wvCs);
    for (INT i = 0; i < g_envCacheCount; i++)
    {
        if (g_envCache[i].m_pEnvironment != NULL && lstrcmpW(g_envCache[i].m_wzFolder, pwszFolder) == 0)
        {
            if (g_envCache[i].m_nUsers > 0) g_envCache[i].m_nUsers--;
            if (g_envCache[i].m_nUsers == 0)
            {
                g_envCache[i].m_pEnvironment->Release();
                g_envCache[i].m_pEnvironment = NULL;
                g_envCache[i].m_wzFolder[0] = L'\0';
            }
            break;
        }
    }
    LeaveCriticalSection(&g_wvCs);
}

void EnvCacheAdd(LPCWSTR pwszFolder, ICoreWebView2Environment* pEnvironment)
{
    if (pwszFolder == NULL || pEnvironment == NULL) return;
    EnsureCs();
    EnterCriticalSection(&g_wvCs);
    for (INT i = 0; i < g_envCacheCount; i++)
    {
        if (lstrcmpW(g_envCache[i].m_wzFolder, pwszFolder) == 0) { LeaveCriticalSection(&g_wvCs); return; }
    }
    INT nSlot = -1;
    for (INT i = 0; i < g_envCacheCount; i++)
    {
        if (g_envCache[i].m_pEnvironment == NULL) { nSlot = i; break; }
    }
    if (nSlot < 0 && g_envCacheCount < 8) nSlot = g_envCacheCount++;
    if (nSlot >= 0)
    {
        lstrcpynW(g_envCache[nSlot].m_wzFolder, pwszFolder,
                  (INT)(sizeof(g_envCache[nSlot].m_wzFolder) / sizeof(WCHAR)));
        g_envCache[nSlot].m_pEnvironment = pEnvironment;
        pEnvironment->AddRef();
        g_envCache[nSlot].m_nUsers = 0;
    }
    LeaveCriticalSection(&g_wvCs);
}

void EnvCacheReleaseAll()
{
    if (!g_wvCsInited) return;
    EnterCriticalSection(&g_wvCs);
    for (INT i = 0; i < 8; i++)
    {
        if (g_envCache[i].m_pEnvironment != NULL)
        {
            g_envCache[i].m_pEnvironment->Release();
            g_envCache[i].m_pEnvironment = NULL;
            g_envCache[i].m_nUsers = 0;
            g_envCache[i].m_wzFolder[0] = L'\0';
        }
    }
    g_envCacheCount = 0;
    LeaveCriticalSection(&g_wvCs);
}

// ============================================================ ② 数据结构

// ITF_GET_ALL_PROPERTY_DATA 与 ITF_CREATE_UNIT 之间传递的数据块：
// 只含本库自定义属性，固定属性（位置尺寸可视等）由系统自己维护。
struct WV_PROP_DATA
{
    CHAR m_szAddress[WV_TEXT_MAX];
    CHAR m_szCacheFolder[WV_TEXT_MAX];
    BOOL m_blAllowNewWindow;
    BOOL m_blAllowDevTools;
    INT  m_nZoomIndex;
    BOOL m_blShowLoading;
};

const INT g_zoomPercent[] = { 50, 75, 100, 125, 150, 200 };
#define WV_ZOOM_COUNT 6

class CWebViewHost;

// 宿主窗口过程（实现在后面，注册窗口类时要先见到声明）
LRESULT CALLBACK WvHostWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// 事件参数挂起槽：需要「应答」的 WebView2 事件按名字落到固定槽位，供应答命令取用。
enum WvPendingSlot
{
    WVP_NEWWINDOW = 0,    // NewWindowRequested
    WVP_PERMISSION,       // PermissionRequested
    WVP_DIALOG,           // ScriptDialogOpening
    WVP_DOWNLOAD,         // DownloadStarting
    WVP_BASEAUTH,         // BasicAuthenticationRequested
    WVP_CLIENCERT,        // ClientCertificateRequested
    WVP_SRVCERT,          // ServerCertificateErrorDetected
    WVP_CTXMENU,          // ContextMenuRequested
    WVP_KEY,              // AcceleratorKeyPressed
    WVP_EXTURI,           // LaunchingExternalUriScheme
    WVP_SAVEASUI,         // SaveAsUIShowing
    WVP_COUNT
};
int  WvEvtSlotOf(LPCSTR szEventName);
int  WvNamedEventOf(LPCSTR szEventName);
void WvApplyEventFallback(int nSlot, IUnknown* pArgs);
void WvEventThunk(void* pCtx, const char* szName, const char* szJson, void* pArgs);

// 组件上各族对象的反射目标解析（实现见反射层一节），宿主类里的订阅逻辑要用它。
BOOL ReflResolveTarget(CWebViewHost* pH, WvReflTarget t, IUnknown** ppObj);

// ------------------------------------------------------------ 事件参数对象表
// 易语言侧拿不到 COM 指针，所以给每个参数对象发一个「对象号」：
// 号 = (槽位 + 1) * 4096 + 代数，代数在槽位复用时递增，旧号自动失效而不是指错对象。
#define WV_OBJ_MAX 64

struct WV_OBJ_SLOT
{
    IUnknown*                 pUnk;
    ICoreWebView2Deferral*    pDeferral;
    CHAR                      szIface[80];
    INT                       nGen;
};

WV_OBJ_SLOT g_wvObjs[WV_OBJ_MAX];
INT         g_wvObjGen = 0;

INT WvObjAdd(IUnknown* pUnk, LPCSTR szIface)
{
    if (pUnk == NULL) return 0;
    for (INT i = 0; i < WV_OBJ_MAX; i++)
    {
        if (g_wvObjs[i].pUnk != NULL) continue;
        g_wvObjGen = (g_wvObjGen % 4095) + 1;
        g_wvObjs[i].pUnk = pUnk;
        pUnk->AddRef();
        g_wvObjs[i].pDeferral = NULL;
        g_wvObjs[i].nGen = g_wvObjGen;
        lstrcpynA(g_wvObjs[i].szIface, (szIface != NULL) ? szIface : "", (INT)sizeof(g_wvObjs[i].szIface));
        return (i + 1) * 4096 + g_wvObjGen;
    }
    return 0;
}

WV_OBJ_SLOT* WvObjSlot(INT nId)
{
    if (nId <= 4096) return NULL;
    const INT i = nId / 4096 - 1;
    const INT nGen = nId % 4096;
    if (i < 0 || i >= WV_OBJ_MAX) return NULL;
    if (g_wvObjs[i].pUnk == NULL || g_wvObjs[i].nGen != nGen) return NULL;
    return &g_wvObjs[i];
}

BOOL WvObjFree(INT nId)
{
    WV_OBJ_SLOT* p = WvObjSlot(nId);
    if (p == NULL) return FALSE;
    if (p->pDeferral != NULL) p->pDeferral->Release();
    p->pUnk->Release();
    p->pUnk = NULL;
    p->pDeferral = NULL;
    p->szIface[0] = '\0';
    p->nGen = 0;
    return TRUE;
}

void WvObjFreeAll()
{
    for (INT i = 0; i < WV_OBJ_MAX; i++)
        if (g_wvObjs[i].pUnk != NULL) WvObjFree((i + 1) * 4096 + g_wvObjs[i].nGen);
}
LRESULT CALLBACK WvOverlayWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ------------------------------------------------------------ COM 回调桩
// 各接口的 Invoke 槽位都在 vtable 第 4 项；QueryInterface 对本接口与 IUnknown
// 返回自身即可，宿主失效时 Invoke 直接吞掉。

#define WV_HANDLER(cls, base, IID_EXPR, params)                                     \
class cls : public base                                                             \
{                                                                                   \
public:                                                                             \
    cls(CWebViewHost* pHost, WV_ALIVE* pState)                                      \
        : m_ref(1), m_pHost(pHost), m_pState(pState) { AliveAddRef(pState); }       \
    virtual ~cls() { AliveRelease(m_pState); }                                      \
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject)                      \
    {                                                                               \
        if (ppvObject == NULL) return E_POINTER;                                    \
        if (riid == __uuidof(IUnknown)) *ppvObject = (base*)this;                   \
        else if (riid == IID_EXPR) *ppvObject = (base*)this;                        \
        else { *ppvObject = NULL; return E_NOINTERFACE; }                           \
        AddRef();                                                                   \
        return S_OK;                                                                \
    }                                                                               \
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }   \
    STDMETHODIMP_(ULONG) Release()                                                  \
    {                                                                               \
        const ULONG n = (ULONG)InterlockedDecrement(&m_ref);                        \
        if (n == 0) delete this;                                                    \
        return n;                                                                   \
    }                                                                               \
    STDMETHODIMP Invoke params;                                                     \
private:                                                                            \
    LONG m_ref;                                                                     \
    CWebViewHost* m_pHost;                                                          \
    WV_ALIVE* m_pState;                                                             \
};

WV_HANDLER(WvEnvHandler, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler,
    __uuidof(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler),
    (HRESULT errorCode, ICoreWebView2Environment* result));
WV_HANDLER(WvCtrlHandler, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler,
    __uuidof(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler),
    (HRESULT errorCode, ICoreWebView2Controller* result));
WV_HANDLER(WvEvalHandler, ICoreWebView2ExecuteScriptCompletedHandler,
    __uuidof(ICoreWebView2ExecuteScriptCompletedHandler),
    (HRESULT errorCode, LPCWSTR result));
WV_HANDLER(WvJsonHandler, ICoreWebView2CallDevToolsProtocolMethodCompletedHandler,
    __uuidof(ICoreWebView2CallDevToolsProtocolMethodCompletedHandler),
    (HRESULT errorCode, LPCWSTR resultAsJson));
WV_HANDLER(WvNavStartHandler, ICoreWebView2NavigationStartingEventHandler,
    __uuidof(ICoreWebView2NavigationStartingEventHandler),
    (ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args));
WV_HANDLER(WvNavDoneHandler, ICoreWebView2NavigationCompletedEventHandler,
    __uuidof(ICoreWebView2NavigationCompletedEventHandler),
    (ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args));
WV_HANDLER(WvTitleHandler, ICoreWebView2DocumentTitleChangedEventHandler,
    __uuidof(ICoreWebView2DocumentTitleChangedEventHandler),
    (ICoreWebView2* sender, IUnknown* args));
WV_HANDLER(WvSourceHandler, ICoreWebView2SourceChangedEventHandler,
    __uuidof(ICoreWebView2SourceChangedEventHandler),
    (ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs* args));
WV_HANDLER(WvNewWindowHandler, ICoreWebView2NewWindowRequestedEventHandler,
    __uuidof(ICoreWebView2NewWindowRequestedEventHandler),
    (ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args));
WV_HANDLER(WvProcessFailedHandler, ICoreWebView2ProcessFailedEventHandler,
    __uuidof(ICoreWebView2ProcessFailedEventHandler),
    (ICoreWebView2* sender, ICoreWebView2ProcessFailedEventArgs* args));
WV_HANDLER(WvTrySuspendHandler, ICoreWebView2TrySuspendCompletedHandler,
    __uuidof(ICoreWebView2TrySuspendCompletedHandler),
    (HRESULT errorCode, BOOL result));
WV_HANDLER(WvBoolDoneHandler, ICoreWebView2PrintToPdfCompletedHandler,
    __uuidof(ICoreWebView2PrintToPdfCompletedHandler),
    (HRESULT errorCode, BOOL result));
WV_HANDLER(WvPrintDoneHandler, ICoreWebView2PrintCompletedHandler,
    __uuidof(ICoreWebView2PrintCompletedHandler),
    (HRESULT errorCode, COREWEBVIEW2_PRINT_STATUS result));
WV_HANDLER(WvSaveAsDoneHandler, ICoreWebView2ShowSaveAsUICompletedHandler,
    __uuidof(ICoreWebView2ShowSaveAsUICompletedHandler),
    (HRESULT errorCode, COREWEBVIEW2_SAVE_AS_UI_RESULT result));
WV_HANDLER(WvVoidDoneHandler, ICoreWebView2CapturePreviewCompletedHandler,
    __uuidof(ICoreWebView2CapturePreviewCompletedHandler),
    (HRESULT errorCode));
WV_HANDLER(WvIdDoneHandler, ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler,
    __uuidof(ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler),
    (HRESULT errorCode, LPCWSTR id));
WV_HANDLER(WvCookieListHandler, ICoreWebView2GetCookiesCompletedHandler,
    __uuidof(ICoreWebView2GetCookiesCompletedHandler),
    (HRESULT errorCode, ICoreWebView2CookieList* result));
WV_HANDLER(WvExtListHandler, ICoreWebView2ProfileGetBrowserExtensionsCompletedHandler,
    __uuidof(ICoreWebView2ProfileGetBrowserExtensionsCompletedHandler),
    (HRESULT errorCode, ICoreWebView2BrowserExtensionList* result));
WV_HANDLER(WvExtItemHandler, ICoreWebView2ProfileAddBrowserExtensionCompletedHandler,
    __uuidof(ICoreWebView2ProfileAddBrowserExtensionCompletedHandler),
    (HRESULT errorCode, ICoreWebView2BrowserExtension* result));
WV_HANDLER(WvVoidHandler2, ICoreWebView2ClearBrowsingDataCompletedHandler,
    __uuidof(ICoreWebView2ClearBrowsingDataCompletedHandler),
    (HRESULT errorCode));
WV_HANDLER(WvVoidHandler3, ICoreWebView2BrowserExtensionRemoveCompletedHandler,
    __uuidof(ICoreWebView2BrowserExtensionRemoveCompletedHandler),
    (HRESULT errorCode));
WV_HANDLER(WvJsonDoneHandler, ICoreWebView2CallDevToolsProtocolMethodCompletedHandler,
    __uuidof(ICoreWebView2CallDevToolsProtocolMethodCompletedHandler),
    (HRESULT errorCode, LPCWSTR resultAsJson));
WV_HANDLER(WvVoidHandler4, ICoreWebView2BrowserExtensionEnableCompletedHandler,
    __uuidof(ICoreWebView2BrowserExtensionEnableCompletedHandler),
    (HRESULT errorCode));
WV_HANDLER(WvNoopHandler, ICoreWebView2ExecuteScriptCompletedHandler,
    __uuidof(ICoreWebView2ExecuteScriptCompletedHandler),
    (HRESULT errorCode, LPCWSTR result));

// ============================================================ ② 宿主窗口

class CWebViewHost
{
public:
    HWND                  m_hWnd;         // 本模块创建的宿主窗口
    HWND                  m_hWrapper;     // 运行时给出的 CWnd*，即 HUNIT
    WV_ALIVE*             m_pState;
    DWORD                 m_dwFormID;
    DWORD                 m_dwUnitID;
    BOOL                  m_blInDesignMode;

    WV_PROP_DATA          m_prop;
    CHAR                  m_szTag[WV_TEXT_MAX];

    ICoreWebView2*                m_pWebview;
    ICoreWebView2Controller*      m_pController;
    ICoreWebView2Environment*     m_pEnvironment;
    ICoreWebView2Environment*     m_pBoundEnv;      // 由「使用网页环境」绑来的进程级环境，非空时优先用它
    WCHAR                         m_wzEnvFolder[MAX_PATH * 4];   // 自己在缓存里占了哪一条
    WCHAR                         m_pBoundEnvFolder[MAX_PATH * 4];   // 绑定环境的目录串
    // ---- 内核事件 ----
    IUnknown*             m_pLastEventObj;        // 最近一次事件带出来的对象（帧/下载/请求…）
    CHAR                  m_szLastEventName[64];
    INT                   m_nLastEventObjId;
    CHAR                  m_szLastEventJson[WV_EVT_JSON_MAX];
    IUnknown*             m_pPending[WVP_COUNT];  // 等用户应答的事件参数
    BOOL                  m_blAnswered[WVP_COUNT];
    struct WV_SUB
    {
        IUnknown*         pTarget;
        CHAR              szGroup[20];
        CHAR              szName[48];
        INT64             tk;
    }                     m_sub[40];

    HWND                  m_hOverlay;             // 载入遮罩，盖在 WebView2 视图之上
    INT                   m_nMarqueePos;        // 滚动进度条偏移
    BOOL                  m_blInitializing;   // 环境/控制器创建中
    BOOL                  m_blNavLoading;
    BOOL                  m_blDeleted;        // 已由 Destroy() 接管释放
    CHAR                  m_szHint[128];      // 非空 = 框里显示这段提示，不再尝试载入内核
    BOOL                  m_blHintPermanent;  // 假 = 可重试

    WCHAR                 m_wzUrl[4096];
    WCHAR                 m_wzTitle[4096];
    WCHAR                 m_wzEvalResult[8192];
    BOOL                  m_blEvalDone;

    // 异步命令的结果槽：同一线程内串行使用，命令进门先 WaitOp() 复位再泵消息等回调。
    BOOL                  m_blOpDone;
    HRESULT               m_hrOp;
    INT                   m_nOpBool;
    CHAR                  m_szOpText[WV_TEXT_MAX];
    CHAR                  m_szOpJson[WV_EVT_JSON_MAX];
    HRESULT               m_hrLastError;

    CWebViewHost()
    {
        m_hWnd = NULL;
        m_hWrapper = NULL;
        m_pState = AliveNew();
        m_dwFormID = m_dwUnitID = 0;
        m_blInDesignMode = FALSE;
        m_pWebview = NULL; m_pController = NULL; m_pEnvironment = NULL;
        m_pBoundEnv = NULL;
        m_wzEnvFolder[0] = L'\0';
        m_pBoundEnvFolder[0] = L'\0';
        m_hOverlay = NULL;
        m_nMarqueePos = 0;
        m_pLastEventObj = NULL;
        m_szLastEventJson[0] = '\0';
        m_szLastEventName[0] = '\0';
        m_nLastEventObjId = 0;
        for (INT i = 0; i < WVP_COUNT; i++) { m_pPending[i] = NULL; m_blAnswered[i] = FALSE; }
        for (INT i = 0; i < (INT)(sizeof(m_sub) / sizeof(m_sub[0])); i++)
        {
            m_sub[i].pTarget = NULL; m_sub[i].tk = 0;
            m_sub[i].szGroup[0] = '\0'; m_sub[i].szName[0] = '\0';
        }
        m_blInitializing = FALSE;
        m_blNavLoading = FALSE;
        m_blDeleted = FALSE;
        m_szHint[0] = '\0';
        m_blHintPermanent = TRUE;
        m_blEvalDone = FALSE;
        m_hrLastError = S_OK;
        m_wzUrl[0] = m_wzTitle[0] = m_wzEvalResult[0] = L'\0';
        memset(&m_prop, 0, sizeof(m_prop));
        memset(m_szTag, 0, sizeof(m_szTag));
        m_prop.m_blAllowNewWindow = FALSE;
        m_prop.m_blAllowDevTools = TRUE;
        m_prop.m_nZoomIndex = 2;
        m_prop.m_blShowLoading = TRUE;
        m_blEvalDone = FALSE;
        m_blOpDone = FALSE;
        m_hrOp = S_OK;
        m_nOpBool = 0;
        m_szOpText[0] = '\0';
        m_szOpJson[0] = '\0';
    }

    virtual ~CWebViewHost()
    {
        ReleaseWebview();
        AliveRelease(m_pState);
        m_pState = NULL;
    }

    static CWebViewHost* FromHwnd(HWND hWnd)
    {
        if (hWnd == NULL || !IsWindow(hWnd)) return NULL;
        return (CWebViewHost*)GetPropA(hWnd, WV_PROP_NAME);
    }

    static CWebViewHost* Create(HWND hParent, DWORD dwStyle, UINT nID, INT x, INT y, INT cx, INT cy)
    {
        if (!RegisterClass()) return NULL;

        CWebViewHost* pH = new CWebViewHost;
        if (pH == NULL) return NULL;

        const DWORD dwAllStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP | dwStyle;
        HWND hWnd = CreateWindowExA(0, WV_HOST_CLASS, "", dwAllStyle,
                                    x, y, (cx < 1) ? 1 : cx, (cy < 1) ? 1 : cy,
                                    hParent, (HMENU)(UINT_PTR)nID, g_hModule, NULL);
        if (hWnd == NULL)
        {
            delete pH;
            return NULL;
        }
        pH->m_hWnd = hWnd;
        SetPropA(hWnd, WV_PROP_NAME, pH);
        return pH;
    }

    static BOOL RegisterClass()
    {
        static BOOL s_blRegistered = FALSE;
        if (s_blRegistered) return TRUE;

        WNDCLASSEXA wc;
        memset(&wc, 0, sizeof(wc));
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = WvHostWndProc;
        wc.hInstance     = g_hModule;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = WV_HOST_CLASS;

        if (RegisterClassExA(&wc) != 0) s_blRegistered = TRUE;
        else if (GetClassInfoExA(g_hModule, WV_HOST_CLASS, &wc)) s_blRegistered = TRUE;
        if (!s_blRegistered) return FALSE;

        static BOOL s_blOverlayRegistered = FALSE;
        if (!s_blOverlayRegistered)
        {
            WNDCLASSEXA oc;
            memset(&oc, 0, sizeof(oc));
            oc.cbSize = sizeof(oc);
            oc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
            oc.lpfnWndProc = WvOverlayWndProc;
            oc.hInstance = g_hModule;
            oc.hCursor = LoadCursor(NULL, IDC_ARROW);
            oc.lpszClassName = WV_OVERLAY_CLASS;
            if (RegisterClassExA(&oc) != 0) s_blOverlayRegistered = TRUE;
            else if (GetClassInfoExA(g_hModule, WV_OVERLAY_CLASS, &oc)) s_blOverlayRegistered = TRUE;
            if (!s_blOverlayRegistered) return FALSE;
        }
        return TRUE;
    }

    // ---------- 属性 ----------

    void SetPropData(const WV_PROP_DATA* pProp)
    {
        if (pProp == NULL) return;
        memcpy(&m_prop, pProp, sizeof(WV_PROP_DATA));
        m_prop.m_szAddress[sizeof(m_prop.m_szAddress) - 1] = '\0';
        m_prop.m_szCacheFolder[sizeof(m_prop.m_szCacheFolder) - 1] = '\0';
    }

    void GetPropData(WV_PROP_DATA* pProp) const { memcpy(pProp, &m_prop, sizeof(WV_PROP_DATA)); }

    BOOL IsWebviewStarted() const { return m_pWebview != NULL || m_pController != NULL || m_blInitializing; }

    // 缓存目录：相对路径按程序运行目录解释；留空回到默认 edge_cache。
    void ResolveCacheFolder(LPWSTR pwszDst, INT nDstChars) const
    {
        if (pwszDst == NULL || nDstChars <= 0) return;
        pwszDst[0] = L'\0';

        CHAR szFolder[WV_TEXT_MAX];
        szFolder[0] = '\0';
        lstrcpynA(szFolder, m_prop.m_szCacheFolder, (int)sizeof(szFolder));
        if (szFolder[0] == '\0') lstrcpynA(szFolder, WV_DEFAULT_CACHE_DIR, (int)sizeof(szFolder));

        CHAR szExpanded[WV_TEXT_MAX];
        szExpanded[0] = '\0';
        if (ExpandEnvironmentStringsA(szFolder, szExpanded, (DWORD)sizeof(szExpanded)) == 0)
            lstrcpynA(szExpanded, szFolder, (int)sizeof(szExpanded));

        WCHAR wzPath[MAX_PATH * 4];
        Utf8ToWide(szExpanded, wzPath, (INT)(sizeof(wzPath) / sizeof(WCHAR)));

        const BOOL blAbsolute = (wzPath[0] != L'\0' && wzPath[1] == L':') ||
                                (wzPath[0] == L'\\' && wzPath[1] == L'\\') ||
                                (wzPath[0] == L'\\');
        if (!blAbsolute)
        {
            CHAR szExe[MAX_PATH * 2];
            GetExeDirectory(szExe, (INT)sizeof(szExe));
            WCHAR wzExe[MAX_PATH * 4];
            Utf8ToWide(szExe, wzExe, (INT)(sizeof(wzExe) / sizeof(WCHAR)));
            lstrcpynW(pwszDst, wzExe, nDstChars);
            INT n = (INT)lstrlenW(pwszDst);
            if (n > 0 && pwszDst[n - 1] != L'\\' && n + (INT)lstrlenW(wzPath) + 2 < nDstChars)
            {
                pwszDst[n] = L'\\';
                pwszDst[n + 1] = L'\0';
            }
        }
        if ((INT)lstrlenW(pwszDst) + (INT)lstrlenW(wzPath) + 1 < nDstChars) lstrcatW(pwszDst, wzPath);

        INT nLen = (INT)lstrlenW(pwszDst);
        while (nLen > 1 && (pwszDst[nLen - 1] == L'\\' || pwszDst[nLen - 1] == L'/')) pwszDst[--nLen] = L'\0';
    }

    // ---------- 生命周期 ----------

    // blPermanent = 假 时只是暂时失败（比如环境刚随上一个窗口退出），下次调用允许重试
    void ShowHint(LPCSTR szText, BOOL blPermanent = TRUE)
    {
        if (szText == NULL) return;
        if (lstrcmpA(m_szHint, szText) == 0 && (blPermanent || m_blHintPermanent)) return;
        lstrcpynA(m_szHint, szText, (int)sizeof(m_szHint));
        m_blHintPermanent = blPermanent ? TRUE : FALSE;
        m_blInitializing = FALSE;
        if (m_hWnd != NULL) InvalidateRect(m_hWnd, NULL, TRUE);
        RefreshLoading();
    }

    BOOL EnsureStarted()
    {
        if (m_blInDesignMode) return FALSE;                       // 设计期不起浏览器进程
        if (m_hWnd == NULL) return FALSE;
        if (m_szHint[0] != '\0')
        {
            if (m_blHintPermanent) return FALSE;                  // 环境缺失，不再反复尝试
            m_szHint[0] = '\0';                                   // 暂时性失败：清掉提示再试一次
            if (m_hWnd != NULL) InvalidateRect(m_hWnd, NULL, TRUE);
        }
        if (IsWebviewStarted()) return TRUE;

        // 没有 Edge 运行环境就安静地退化成提示文字：不弹运行时错误、不崩。
        if (!ProbeRuntime())
        {
            ShowHint(g_wzRuntimeVersion[0] == L'\0' && g_pfnCreateEnv == NULL ? WV_HINT_NO_LOADER : WV_HINT_NO_EDGE);
            return FALSE;
        }
        if (!EnsureCom())
        {
            ShowHint(WV_HINT_START_FAIL, FALSE);
            return FALSE;
        }

        WCHAR wzFolder[MAX_PATH * 4];
        ResolveCacheFolder(wzFolder, (INT)(sizeof(wzFolder) / sizeof(WCHAR)));
        if (wzFolder[0] == L'\0') lstrcpynW(wzFolder, L"edge_cache", (INT)(sizeof(wzFolder) / sizeof(WCHAR)));

        m_blInitializing = TRUE;
        RefreshLoading();

        if (m_pBoundEnv != NULL)
        {
            // 环境由 WebView2环境 对象持有，本组件只借一份引用。
            m_pEnvironment = m_pBoundEnv;
            m_pEnvironment->AddRef();
            lstrcpynW(m_wzEnvFolder, m_pBoundEnvFolder, (INT)(sizeof(m_wzEnvFolder) / sizeof(WCHAR)));
            return RequestController();
        }
        m_pEnvironment = EnvCacheFind(wzFolder);
        if (m_pEnvironment != NULL)
        {
            lstrcpynW(m_wzEnvFolder, wzFolder, (INT)(sizeof(m_wzEnvFolder) / sizeof(WCHAR)));
            return RequestController();
        }

        WvEnvHandler* pHandler = new WvEnvHandler(this, m_pState);
        const HRESULT hr = g_pfnCreateEnv(NULL, wzFolder, NULL, pHandler);
        pHandler->Release();
        if (FAILED(hr))
        {
            m_blInitializing = FALSE;
            m_hrLastError = hr;
            ShowHint(WV_HINT_NO_EDGE, TRUE);
            return FALSE;
        }
        return TRUE;
    }

    BOOL RequestController()
    {
        if (m_pEnvironment == NULL || m_hWnd == NULL) { m_blInitializing = FALSE; return FALSE; }
        WvCtrlHandler* pHandler = new WvCtrlHandler(this, m_pState);
        const HRESULT hr = m_pEnvironment->CreateCoreWebView2Controller(m_hWnd, pHandler);
        pHandler->Release();
        if (FAILED(hr))
        {
            m_blInitializing = FALSE;
            m_hrLastError = hr;
            ShowHint(WV_HINT_START_FAIL, FALSE);
            return FALSE;
        }
        return TRUE;
    }

    void OnEnvironmentCreated(HRESULT hr, ICoreWebView2Environment* pEnvironment)
    {
        if (m_hWnd == NULL || !AliveOk(m_pState) || FAILED(hr) || pEnvironment == NULL)
        {
            if (pEnvironment != NULL) pEnvironment->Release();
            m_blInitializing = FALSE;
            m_hrLastError = hr;
            if (m_hWnd != NULL && AliveOk(m_pState)) ShowHint(WV_HINT_NO_EDGE, TRUE);
            return;
        }

        WCHAR wzFolder[MAX_PATH * 4];
        ResolveCacheFolder(wzFolder, (INT)(sizeof(wzFolder) / sizeof(WCHAR)));
        EnvCacheAdd(wzFolder, pEnvironment);
        pEnvironment->Release();                 // 缓存里已持有一份

        if (m_pEnvironment != NULL) { m_pEnvironment->Release(); m_pEnvironment = NULL; }
        m_pEnvironment = EnvCacheFind(wzFolder);
        if (m_pEnvironment != NULL) lstrcpynW(m_wzEnvFolder, wzFolder, (INT)(sizeof(m_wzEnvFolder) / sizeof(WCHAR)));
        RequestController();
    }

    void OnControllerCreated(HRESULT hr, ICoreWebView2Controller* pController)
    {
        m_blInitializing = FALSE;
        if (m_hWnd == NULL || !AliveOk(m_pState) || FAILED(hr) || pController == NULL)
        {
            if (pController != NULL) pController->Release();
            if (m_hWnd != NULL && AliveOk(m_pState)) ShowHint(WV_HINT_START_FAIL, FALSE);
            return;
        }

        if (m_pController != NULL) { m_pController->Close(); m_pController->Release(); }
        m_pController = pController;
        m_pController->AddRef();

        RECT rc;
        GetClientRect(m_hWnd, &rc);
        m_pController->put_Bounds(rc);
        m_pController->put_IsVisible(IsWindowVisible(m_hWnd) ? TRUE : FALSE);

        if (m_pController->get_CoreWebView2(&m_pWebview) != S_OK) m_pWebview = NULL;
        if (m_pWebview == NULL) { RefreshLoading(); return; }

        // 事件处理器注册后由 WebView2 持有引用，本对象销毁时窗口先死，回调靠 m_pState 失效。
        EventRegistrationToken tk;
        m_pWebview->add_NavigationStarting(new WvNavStartHandler(this, m_pState), &tk);
        m_pWebview->add_NavigationCompleted(new WvNavDoneHandler(this, m_pState), &tk);
        m_pWebview->add_DocumentTitleChanged(new WvTitleHandler(this, m_pState), &tk);
        m_pWebview->add_SourceChanged(new WvSourceHandler(this, m_pState), &tk);
        m_pWebview->add_NewWindowRequested(new WvNewWindowHandler(this, m_pState), &tk);
        m_pWebview->add_ProcessFailed(new WvProcessFailedHandler(this, m_pState), &tk);

        // 命名事件靠通用订阅层驱动：内核版本不够的订阅会安静失败，不影响其它功能。
        static const char* const szAutoSub[] = {
            "WebMessageReceived", "DownloadStarting", "PermissionRequested", "ScriptDialogOpening",
            "BasicAuthenticationRequested", "ClientCertificateRequested", "ServerCertificateErrorDetected",
            "ContextMenuRequested", "DOMContentLoaded", "WindowCloseRequested", "AcceleratorKeyPressed",
            "LaunchingExternalUriScheme", "SaveAsUIShowing",
        };
        for (INT i = 0; i < (INT)(sizeof(szAutoSub) / sizeof(szAutoSub[0])); i++)
            SubscribeKernelAuto(szAutoSub[i]);

        ApplyDevTools();
        ApplyZoom();

        if (m_prop.m_szAddress[0] != '\0') NavigateUtf8(m_prop.m_szAddress);
        RefreshLoading();
    }

    void ReleaseWebview()
    {
        if (m_pWebview != NULL) { m_pWebview->Release(); m_pWebview = NULL; }
        if (m_pController != NULL) { m_pController->Close(); m_pController->Release(); m_pController = NULL; }
        if (m_pEnvironment != NULL) { m_pEnvironment->Release(); m_pEnvironment = NULL; }
        if (m_pBoundEnv != NULL) { m_pBoundEnv->Release(); m_pBoundEnv = NULL; }
        if (m_wzEnvFolder[0] != L'\0')
        {
            EnvCacheRelease(m_wzEnvFolder);
            m_wzEnvFolder[0] = L'\0';
        }
        UnsubscribeAllKernel();
        ClearAllPending();
        m_blInitializing = FALSE;
        DropOverlay();
    }

    // ---------- 异步命令等待 ----------

    // 回调都在本线程消息队列里，泵消息等结果；超时返回假，调用方按「未完成」降级处理。
    BOOL BeginOp()
    {
        m_blOpDone = FALSE;
        m_hrOp = S_OK;
        m_nOpBool = 0;
        m_szOpText[0] = '\0';
        m_szOpJson[0] = '\0';
        return TRUE;
    }

    BOOL WaitOp(INT nMs)
    {
        if (nMs < 1000) nMs = 1000;
        const DWORD dwEnd = GetTickCount() + (DWORD)nMs;
        while (!m_blOpDone && GetTickCount() < dwEnd && m_hWnd != NULL && AliveOk(m_pState))
        {
            MSG msg;
            if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            else
                Sleep(1);
        }
        return m_blOpDone ? TRUE : FALSE;
    }

    void OpDone(HRESULT hr, INT nBool, LPCWSTR pwszText, void* pObj, LPCSTR szObjIface)
    {
        if (m_hWnd == NULL || !AliveOk(m_pState)) return;
        m_hrOp = hr;
        m_nOpBool = nBool;
        m_szOpText[0] = '\0';
        if (pwszText != NULL) WideToUtf8(pwszText, m_szOpText, (INT)sizeof(m_szOpText));
        m_szOpJson[0] = '\0';
        if (pObj != NULL && szObjIface != NULL)
            WvDescribeObject(szObjIface, pObj, m_szOpJson, (INT)sizeof(m_szOpJson));
        m_blOpDone = TRUE;
    }

    // ---------- 内核事件 ----------

    IUnknown* Pending(INT nSlot) const
    {
        return (nSlot >= 0 && nSlot < WVP_COUNT) ? m_pPending[nSlot] : NULL;
    }

    void SetPending(INT nSlot, IUnknown* pArgs)
    {
        if (nSlot < 0 || nSlot >= WVP_COUNT) return;
        if (m_pPending[nSlot] == pArgs) { m_blAnswered[nSlot] = FALSE; return; }
        if (m_pPending[nSlot] != NULL) m_pPending[nSlot]->Release();
        m_pPending[nSlot] = pArgs;
        if (pArgs != NULL) pArgs->AddRef();          // 跨回调持有，应答命令用完再放
        m_blAnswered[nSlot] = FALSE;
    }

    void MarkAnswered(INT nSlot) { if (nSlot >= 0 && nSlot < WVP_COUNT) m_blAnswered[nSlot] = TRUE; }
    BOOL WasAnswered(INT nSlot) const { return (nSlot >= 0 && nSlot < WVP_COUNT) ? m_blAnswered[nSlot] : TRUE; }

    LPCSTR LastEventJson() const { return m_szLastEventJson; }

    // 事件回调：存快照 → 记挂起参数 → 发对应命名事件 → 发通用「内核事件」。
    void OnKernelEvent(LPCSTR szName, LPCSTR szJson, IUnknown* pArgs)
    {
        if (m_hWnd == NULL || !AliveOk(m_pState)) return;

        lstrcpynA(m_szLastEventName, szName, (INT)sizeof(m_szLastEventName));
        lstrcpynA(m_szLastEventJson, (szJson != NULL) ? szJson : "", (INT)sizeof(m_szLastEventJson));

        if (m_pLastEventObj != pArgs)
        {
            if (m_pLastEventObj != NULL) m_pLastEventObj->Release();
            m_pLastEventObj = pArgs;
            if (pArgs != NULL) pArgs->AddRef();
        }

        const INT nSlot = WvEvtSlotOf(szName);
        SetPending(nSlot, pArgs);

        // 参数对象进表：用户拿对象号按属性读、按子对象钻、置属性应答、挂延后。
        if (m_nLastEventObjId != 0) { WvObjFree(m_nLastEventObjId); m_nLastEventObjId = 0; }
        if (pArgs != NULL)
            m_nLastEventObjId = WvObjAdd(pArgs, WvEvtArgsIface(szName));
        INT nObjId = m_nLastEventObjId;

        switch (WvNamedEventOf(szName))
        {
        case WVEVENT_WEB_MESSAGE:  FireEvent(WVEVENT_WEB_MESSAGE, 1, &nObjId); break;    // 正文走 取事件数据
        case WVEVENT_DOWNLOAD:     FireEvent(WVEVENT_DOWNLOAD, 1, &nObjId); break;
        case WVEVENT_PERMISSION:   FireEvent(WVEVENT_PERMISSION, 1, &nObjId); break;
        case WVEVENT_DIALOG:       FireEvent(WVEVENT_DIALOG, 1, &nObjId); break;
        case WVEVENT_BASE_AUTH:    FireEvent(WVEVENT_BASE_AUTH, 1, &nObjId); break;
        case WVEVENT_CLIENT_CERT:  FireEvent(WVEVENT_CLIENT_CERT, 1, &nObjId); break;
        case WVEVENT_SERVER_CERT:  FireEvent(WVEVENT_SERVER_CERT, 1, &nObjId); break;
        case WVEVENT_CONTEXT_MENU: FireEvent(WVEVENT_CONTEXT_MENU, 1, &nObjId); break;
        case WVEVENT_DOM_READY:    FireEvent(WVEVENT_DOM_READY, 1, &nObjId); break;
        case WVEVENT_CLOSE_REQ:    FireEvent(WVEVENT_CLOSE_REQ, 1, &nObjId); break;
        case WVEVENT_KEY:          FireEvent(WVEVENT_KEY, 1, &nObjId); break;
        case WVEVENT_EXT_URI:      FireEvent(WVEVENT_EXT_URI, 1, &nObjId); break;
        case WVEVENT_SAVEAS_UI:    FireEvent(WVEVENT_SAVEAS_UI, 1, &nObjId); break;
        default: break;
        }
        FireEvent(WVEVENT_KERNEL_EVENT, 1, &nObjId);   // 参数 = 事件参数对象号；名称/JSON 另有命令可读

        // 用户没应答就恢复内核默认行为，避免「一订阅就把功能关掉」。
        if (nSlot >= 0 && !WasAnswered(nSlot)) WvApplyEventFallback(nSlot, pArgs);
    }

    BOOL SubscribeKernel(LPCSTR szGroup, LPCSTR szName)
    {
        if (szName == NULL || szName[0] == '\0') return FALSE;
        WvReflTarget t = RF_T_CORE;
        if (szGroup != NULL && szGroup[0] != '\0' && !ReflTargetFromName(szGroup, &t)) return FALSE;

        IUnknown* pTarget = NULL;
        if (!ReflResolveTarget(this, t, &pTarget) || pTarget == NULL) return FALSE;

        INT64 tk = 0;
        const BOOL bl = WvEvtSubscribe(pTarget, ReflTargetName(t), szName, this, WvEventThunk, &tk);
        if (!bl) { pTarget->Release(); return FALSE; }

        for (INT i = 0; i < (INT)(sizeof(m_sub) / sizeof(m_sub[0])); i++)
        {
            if (m_sub[i].pTarget != NULL) continue;
            m_sub[i].pTarget = pTarget;                 // 表里持有一份引用
            lstrcpynA(m_sub[i].szGroup, ReflTargetName(t), (INT)sizeof(m_sub[i].szGroup));
            lstrcpynA(m_sub[i].szName, szName, (INT)sizeof(m_sub[i].szName));
            m_sub[i].tk = tk;
            return TRUE;
        }
        WvEvtUnsubscribe(pTarget, ReflTargetName(t), szName, tk);
        pTarget->Release();
        return FALSE;
    }

    // 组名留空时逐组试：事件对象自己属于哪一族，用户不必关心。
    BOOL SubscribeKernelAuto(LPCSTR szName)
    {
        if (szName == NULL || szName[0] == '\0') return FALSE;
        IUnknown* pTarget = NULL;
        if (!ReflResolveTarget(this, RF_T_CORE, &pTarget) || pTarget == NULL) return FALSE;
        INT64 tk = 0;
        const BOOL bl = WvEvtSubscribe(pTarget, "CORE", szName, this, WvEventThunk, &tk);
        if (bl)
        {
            for (INT i = 0; i < (INT)(sizeof(m_sub) / sizeof(m_sub[0])); i++)
            {
                if (m_sub[i].pTarget != NULL) continue;
                m_sub[i].pTarget = pTarget;
                lstrcpynA(m_sub[i].szGroup, "core", (INT)sizeof(m_sub[i].szGroup));
                lstrcpynA(m_sub[i].szName, szName, (INT)sizeof(m_sub[i].szName));
                m_sub[i].tk = tk;
                return TRUE;
            }
            WvEvtUnsubscribe(pTarget, "CORE", szName, tk);
        }
        pTarget->Release();
        return FALSE;
    }

    BOOL UnsubscribeKernel(LPCSTR szGroup, LPCSTR szName)
    {
        for (INT i = 0; i < (INT)(sizeof(m_sub) / sizeof(m_sub[0])); i++)
        {
            if (m_sub[i].pTarget == NULL) continue;
            if (szName != NULL && szName[0] != '\0' && _stricmp(m_sub[i].szName, szName) != 0) continue;
            WvEvtUnsubscribe(m_sub[i].pTarget, m_sub[i].szGroup, m_sub[i].szName, m_sub[i].tk);
            m_sub[i].pTarget->Release();
            m_sub[i].pTarget = NULL;
            m_sub[i].tk = 0;
            m_sub[i].szName[0] = '\0';
            return TRUE;
        }
        UNREFERENCED_PARAMETER(szGroup);
        return FALSE;
    }

    void UnsubscribeAllKernel()
    {
        for (INT i = 0; i < (INT)(sizeof(m_sub) / sizeof(m_sub[0])); i++)
        {
            if (m_sub[i].pTarget == NULL) continue;
            WvEvtUnsubscribe(m_sub[i].pTarget, m_sub[i].szGroup, m_sub[i].szName, m_sub[i].tk);
            m_sub[i].pTarget->Release();
            m_sub[i].pTarget = NULL;
            m_sub[i].tk = 0;
        }
    }

    void ClearAllPending()
    {
        for (INT i = 0; i < WVP_COUNT; i++)
        {
            if (m_pPending[i] != NULL) { m_pPending[i]->Release(); m_pPending[i] = NULL; }
            m_blAnswered[i] = FALSE;
        }
        if (m_pLastEventObj != NULL) { m_pLastEventObj->Release(); m_pLastEventObj = NULL; }
    }

    // ---------- 载入遮罩 ----------

    // 「载入提示」属性为真时，初始化与跳转期间用遮罩盖住浏览器，显示滚动进度。
    BOOL WantsLoading() const
    {
        if (m_prop.m_blShowLoading == FALSE) return FALSE;
        if (m_blInDesignMode) return FALSE;
        if (m_hWnd == NULL || m_blDeleted) return FALSE;
        if (m_szHint[0] != '\0') return FALSE;          // 已经退化成提示文字，不再叠一层
        return (m_blInitializing || m_blNavLoading) ? TRUE : FALSE;
    }

    void RefreshLoading()
    {
        if (m_hWnd == NULL) return;

        if (!WantsLoading())
        {
            if (m_hOverlay != NULL && IsWindow(m_hOverlay))
            {
                KillTimer(m_hOverlay, WV_OVERLAY_TIMER);
                ShowWindow(m_hOverlay, SW_HIDE);
            }
            return;
        }

        if (m_hOverlay == NULL || !IsWindow(m_hOverlay))
        {
            m_hOverlay = CreateWindowExA(0, WV_OVERLAY_CLASS, "",
                                         WS_CHILD | WS_CLIPSIBLINGS | WS_DISABLED,
                                         0, 0, 1, 1, m_hWnd, NULL, g_hModule, NULL);
            if (m_hOverlay == NULL) return;
            SetPropA(m_hOverlay, WV_PROP_NAME, this);
            m_nMarqueePos = -WV_MARQUEE_BLOCK_W;
        }

        RECT rc;
        GetClientRect(m_hWnd, &rc);
        SetWindowPos(m_hOverlay, HWND_TOP, 0, 0, rc.right, rc.bottom, SWP_NOACTIVATE);
        ShowWindow(m_hOverlay, SW_SHOW);
        SetTimer(m_hOverlay, WV_OVERLAY_TIMER, WV_LOADING_TICK_MS, NULL);
    }

    // 宿主正在消失（父窗口已空）时不能再 DestroyWindow，遮罩会随父窗口一起销毁。
    void DropOverlay()
    {
        if (m_hOverlay == NULL) return;
        HWND h = m_hOverlay;
        m_hOverlay = NULL;
        if (m_hWnd != NULL && !m_blDeleted)
        {
            KillTimer(h, WV_OVERLAY_TIMER);
            DestroyWindow(h);
        }
    }

    void BoundsChanged()
    {
        if (m_pController == NULL || m_hWnd == NULL) return;
        RECT rc;
        GetClientRect(m_hWnd, &rc);
        m_pController->put_Bounds(rc);
        if (m_hOverlay != NULL && IsWindow(m_hOverlay) && IsWindowVisible(m_hOverlay))
            SetWindowPos(m_hOverlay, HWND_TOP, 0, 0, rc.right, rc.bottom, SWP_NOACTIVATE);
    }

    // 主动销毁：窗口过程里会通知运行时组件已销毁并释放本对象，调用后不得再碰 this。
    // 主动销毁：先标记失效，之后任何按句柄的查找都会拿到空；对象由 WM_NCDESTROY 释放。
    void Destroy()
    {
        if (m_hWnd == NULL)
        {
            AliveKill(m_pState);
            delete this;
            return;
        }
        HWND hWnd = m_hWnd;
        m_hWnd = NULL;                        // 之后所有查找都判空返回
        m_blDeleted = TRUE;
        ReleaseWebview();
        AliveKill(m_pState);
        DestroyWindow(hWnd);                  // WM_NCDESTROY 里 delete this
    }

    void ApplyZoom()
    {
        if (m_pController == NULL) return;
        INT n = m_prop.m_nZoomIndex;
        if (n < 0) n = 0;
        if (n >= WV_ZOOM_COUNT) n = WV_ZOOM_COUNT - 1;
        m_pController->put_ZoomFactor((double)g_zoomPercent[n] / 100.0);
    }

    void ApplyDevTools()
    {
        ICoreWebView2Settings* pSettings = NULL;
        if (m_pWebview != NULL && m_pWebview->get_Settings(&pSettings) == S_OK && pSettings != NULL)
        {
            pSettings->put_AreDevToolsEnabled(m_prop.m_blAllowDevTools ? TRUE : FALSE);
            pSettings->Release();
        }
    }

    // ---------- 事件抛出 ----------

    void FireEvent(INT nEventIndex, INT nArgCount, const INT* pnArgValue)
    {
        if (m_hWnd == NULL || g_pfnNotifySys == NULL) return;
        EVENT_NOTIFY ev;
        memset(&ev, 0, sizeof(ev));
        ev.m_dwFormID = m_dwFormID;
        ev.m_dwUnitID = m_dwUnitID;
        ev.m_nEventIndex = nEventIndex;
        ev.m_nArgCount = (nArgCount > 5) ? 5 : nArgCount;
        for (INT i = 0; i < ev.m_nArgCount; i++) ev.m_nArgValue[i] = (pnArgValue != NULL) ? pnArgValue[i] : 0;
        NotifySys(NRS_EVENT_NOTIFY, (DWORD)&ev, 0);
    }

    void FireTextEvent(INT nEventIndex, LPCWSTR pwszText)
    {
        if (m_hWnd == NULL || g_pfnNotifySys == NULL) return;
        CHAR szText[WV_TEXT_MAX];
        szText[0] = '\0';
        if (pwszText != NULL)
        {
            CHAR szRaw[WV_TEXT_MAX * 2];
            WideToUtf8(pwszText, szRaw, (INT)sizeof(szRaw));
            lstrcpynA(szText, szRaw, (int)sizeof(szText));
        }
        EVENT_NOTIFY ev;
        memset(&ev, 0, sizeof(ev));
        ev.m_dwFormID = m_dwFormID;
        ev.m_dwUnitID = m_dwUnitID;
        ev.m_nEventIndex = nEventIndex;
        ev.m_nArgCount = 1;
        ev.m_nArgValue[0] = (INT)(DWORD_PTR)(LPCSTR)szText;
        NotifySys(NRS_EVENT_NOTIFY, (DWORD)&ev, 0);
    }

    // ---------- 回调实现 ----------

    void OnNavigationStarting(ICoreWebView2NavigationStartingEventArgs* args)
    {
        m_blNavLoading = TRUE;
        if (args != NULL)
        {
            LPWSTR pwszUri = NULL;
            if (args->get_Uri(&pwszUri) == S_OK && pwszUri != NULL)
            {
                lstrcpynW(m_wzUrl, pwszUri, (INT)(sizeof(m_wzUrl) / sizeof(WCHAR)));
                CoTaskMemFree(pwszUri);
            }
        }
        FireTextEvent(WVEVENT_ADDRESS_CHANGED, m_wzUrl);
        RefreshLoading();
    }

    void OnNavigationCompleted(ICoreWebView2NavigationCompletedEventArgs* args)
    {
        m_blNavLoading = FALSE;
        INT nArg = 1;
        if (args != NULL)
        {
            BOOL blOk = FALSE;
            args->get_IsSuccess(&blOk);
            nArg = blOk ? 1 : 0;
            if (!blOk)
            {
                COREWEBVIEW2_WEB_ERROR_STATUS st = COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
                args->get_WebErrorStatus(&st);
                INT nErr = (INT)st;
                FireEvent(WVEVENT_PAGE_ERROR, 1, &nErr);
            }
        }
        FireEvent(WVEVENT_LOAD_DONE, 1, &nArg);
        RefreshLoading();
    }

    void OnTitleChanged(ICoreWebView2* pSender)
    {
        if (pSender == NULL) return;
        LPWSTR pwszTitle = NULL;
        if (pSender->get_DocumentTitle(&pwszTitle) == S_OK)
        {
            m_wzTitle[0] = L'\0';
            if (pwszTitle != NULL)
            {
                lstrcpynW(m_wzTitle, pwszTitle, (INT)(sizeof(m_wzTitle) / sizeof(WCHAR)));
                CoTaskMemFree(pwszTitle);
            }
            FireTextEvent(WVEVENT_TITLE_CHANGED, m_wzTitle);
        }
    }

    void OnSourceChanged(ICoreWebView2SourceChangedEventArgs* args)
    {
        UNREFERENCED_PARAMETER(args);
        FireTextEvent(WVEVENT_ADDRESS_CHANGED, m_wzUrl);
    }

    void OnNewWindowRequested(ICoreWebView2* pSender, ICoreWebView2NewWindowRequestedEventArgs* args)
    {
        if (args == NULL) return;
        LPWSTR pwszUri = NULL;
        args->get_Uri(&pwszUri);

        if (!m_prop.m_blAllowNewWindow)
        {
            args->put_Handled(TRUE);
            if (pwszUri != NULL) CoTaskMemFree(pwszUri);
            return;
        }
        // 允许：在当前浏览框里打开，避免产生脱离窗口的浏览器窗口。
        if (pSender != NULL && pwszUri != NULL) pSender->Navigate(pwszUri);
        args->put_Handled(TRUE);
        FireTextEvent(WVEVENT_NEW_WINDOW, pwszUri);
        if (pwszUri != NULL) CoTaskMemFree(pwszUri);
    }

    void OnProcessFailed(ICoreWebView2ProcessFailedEventArgs* args)
    {
        UNREFERENCED_PARAMETER(args);
        m_blInitializing = FALSE;
        INT nArg = 0;
        FireEvent(WVEVENT_PAGE_ERROR, 1, &nArg);
        ShowHint(WV_HINT_START_FAIL, FALSE);
    }

    void OnEvalDone(HRESULT hr, LPCWSTR pwszResult)
    {
        m_hrLastError = hr;
        m_wzEvalResult[0] = L'\0';
        if (pwszResult != NULL) lstrcpynW(m_wzEvalResult, pwszResult, (INT)(sizeof(m_wzEvalResult) / sizeof(WCHAR)));
        m_blEvalDone = TRUE;
    }

    // ---------- 方法 ----------

    BOOL WaitWebviewReady(INT nTimeoutMs)
    {
        if (m_pWebview != NULL) return TRUE;
        if (!EnsureStarted()) return FALSE;
        if (m_blInDesignMode) return FALSE;

        if (nTimeoutMs < 500) nTimeoutMs = 500;
        const DWORD dwEnd = GetTickCount() + (DWORD)nTimeoutMs;
        while (m_pWebview == NULL && GetTickCount() < dwEnd)
        {
            MSG msg;
            if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            else
                Sleep(1);
        }
        return (m_pWebview != NULL) ? TRUE : FALSE;
    }

    // 跳转类命令调用成功后统一进入载入态：NavigationStarting 是异步的，先自己置位免得遮罩闪一下。
    BOOL NavStarted(HRESULT hr)
    {
        if (FAILED(hr)) return FALSE;
        m_blNavLoading = TRUE;
        RefreshLoading();
        return TRUE;
    }

    BOOL NavigateUtf8(LPCSTR szUtf8Url)
    {
        if (szUtf8Url == NULL) return FALSE;
        if (szUtf8Url[0] == '\0') szUtf8Url = "about:blank";

        // 记住地址：内核还没起来时，等控制器建好后由 OnControllerCreated 补上这次跳转。
        lstrcpynA(m_prop.m_szAddress, szUtf8Url, (int)sizeof(m_prop.m_szAddress));

        if (m_pWebview == NULL)
            return EnsureStarted() ? TRUE : FALSE;

        WCHAR wzUrl[WV_TEXT_MAX * 2];
        Utf8ToWide(m_prop.m_szAddress, wzUrl, (INT)(sizeof(wzUrl) / sizeof(WCHAR)));
        if (wzUrl[0] == L'\0') return FALSE;

        lstrcpynW(m_wzUrl, wzUrl, (INT)(sizeof(m_wzUrl) / sizeof(WCHAR)));
        return NavStarted(m_pWebview->Navigate(wzUrl));
    }

    BOOL NavigateHtmlUtf8(LPCSTR szUtf8Html)
    {
        if (szUtf8Html == NULL) return FALSE;
        if (!WaitWebviewReady(8000)) return FALSE;
        static WCHAR s_wzHtml[WV_TEXT_MAX * 2];
        s_wzHtml[0] = L'\0';
        Utf8ToWide(szUtf8Html, s_wzHtml, (INT)(sizeof(s_wzHtml) / sizeof(WCHAR)));
        if (s_wzHtml[0] == L'\0') return FALSE;
        return NavStarted(m_pWebview->NavigateToString(s_wzHtml));
    }

    BOOL Reload(BOOL blSkipCache)
    {
        if (!WaitWebviewReady(8000)) return FALSE;
        if (blSkipCache) RunDevTools("Network.setCacheDisabled", "{\"cacheDisabled\":true}");
        return NavStarted(m_pWebview->Reload());
    }

    BOOL GoBack()    { return (m_pWebview != NULL && CanGoBack()) ? NavStarted(m_pWebview->GoBack()) : FALSE; }
    BOOL GoForward() { return (m_pWebview != NULL && CanGoForward()) ? NavStarted(m_pWebview->GoForward()) : FALSE; }

    BOOL Stop()
    {
        if (m_pWebview == NULL) return FALSE;
        if (FAILED(m_pWebview->Stop())) return FALSE;
        m_blNavLoading = FALSE;
        RefreshLoading();
        return TRUE;
    }

    BOOL CanGoBack()
    {
        BOOL bl = FALSE;
        if (m_pWebview != NULL) m_pWebview->get_CanGoBack(&bl);
        return bl ? TRUE : FALSE;
    }

    BOOL CanGoForward()
    {
        BOOL bl = FALSE;
        if (m_pWebview != NULL) m_pWebview->get_CanGoForward(&bl);
        return bl ? TRUE : FALSE;
    }

    BOOL SetZoomPercent(INT nPercent)
    {
        if (nPercent < 25) nPercent = 25;
        if (nPercent > 500) nPercent = 500;
        if (m_pController == NULL) return FALSE;
        return SUCCEEDED(m_pController->put_ZoomFactor((double)nPercent / 100.0));
    }

    BOOL ExecuteScriptUtf8(LPCSTR szUtf8Script)
    {
        if (szUtf8Script == NULL) return FALSE;
        if (!WaitWebviewReady(8000)) return FALSE;
        WCHAR* pwsz = new WCHAR[WV_TEXT_MAX * 2];
        if (pwsz == NULL) return FALSE;
        pwsz[0] = L'\0';
        Utf8ToWide(szUtf8Script, pwsz, WV_TEXT_MAX * 2);
        WvNoopHandler* pHandler = new WvNoopHandler(this, m_pState);
        const HRESULT hr = m_pWebview->ExecuteScript(pwsz, pHandler);
        pHandler->Release();
        delete[] pwsz;
        return SUCCEEDED(hr);
    }

    BOOL EvaluateScriptUtf8(LPCSTR szUtf8Script, INT nTimeoutMs, LPSTR szUtf8Out, INT nOutCap)
    {
        if (szUtf8Out != NULL && nOutCap > 0) szUtf8Out[0] = '\0';
        if (szUtf8Script == NULL) return FALSE;
        if (!WaitWebviewReady(8000)) return FALSE;

        WCHAR* pwsz = new WCHAR[WV_TEXT_MAX * 2];
        if (pwsz == NULL) return FALSE;
        pwsz[0] = L'\0';
        Utf8ToWide(szUtf8Script, pwsz, WV_TEXT_MAX * 2);

        m_blEvalDone = FALSE;
        m_wzEvalResult[0] = L'\0';

        WvEvalHandler* pHandler = new WvEvalHandler(this, m_pState);
        const HRESULT hr = m_pWebview->ExecuteScript(pwsz, pHandler);
        pHandler->Release();
        delete[] pwsz;
        if (FAILED(hr)) return FALSE;

        if (nTimeoutMs < 100) nTimeoutMs = 100;
        const DWORD dwEnd = GetTickCount() + (DWORD)nTimeoutMs;
        while (!m_blEvalDone && GetTickCount() < dwEnd)
        {
            MSG msg;
            if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            else
                Sleep(1);
        }
        if (!m_blEvalDone) return FALSE;

        if (szUtf8Out != NULL && nOutCap > 0) WideToUtf8(m_wzEvalResult, szUtf8Out, nOutCap);
        return SUCCEEDED(m_hrLastError);
    }

    BOOL RunDevTools(LPCSTR szUtf8Method, LPCSTR szUtf8Json)
    {
        if (m_pWebview == NULL || szUtf8Method == NULL) return FALSE;
        WCHAR wzMethod[256], wzJson[1024];
        Utf8ToWide(szUtf8Method, wzMethod, (INT)(sizeof(wzMethod) / sizeof(WCHAR)));
        wzJson[0] = L'\0';
        if (szUtf8Json != NULL) Utf8ToWide(szUtf8Json, wzJson, (INT)(sizeof(wzJson) / sizeof(WCHAR)));
        WvJsonHandler* pHandler = new WvJsonHandler(this, m_pState);
        const HRESULT hr = m_pWebview->CallDevToolsProtocolMethod(wzMethod, wzJson, pHandler);
        pHandler->Release();
        return SUCCEEDED(hr);
    }

    BOOL ClearCache() { return RunDevTools("Network.clearBrowserCache", "{}"); }

    void OpenDevTools() { if (m_pWebview != NULL) m_pWebview->OpenDevToolsWindow(); }

    // ---------- 文本读取 ----------

    void GetUrlUtf8(LPSTR szDst, INT nDstCap) const { WideToUtf8(m_wzUrl, szDst, nDstCap); }
    void GetTitleUtf8(LPSTR szDst, INT nDstCap) const { WideToUtf8(m_wzTitle, szDst, nDstCap); }

    void ReadSourceSync(LPSTR szDst, INT nDstCap)
    {
        if (szDst == NULL || nDstCap <= 0) return;
        szDst[0] = '\0';
        CHAR szScript[] = "document.documentElement.outerHTML";
        EvaluateScriptUtf8(szScript, 8000, szDst, nDstCap);
    }
};

HRESULT WvJsonDoneHandler::Invoke(HRESULT errorCode, LPCWSTR resultAsJson)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OpDone(errorCode, 0, resultAsJson, NULL, NULL);
    return S_OK;
}

// ============================================================ 回调桩实现

HRESULT WvEnvHandler::Invoke(HRESULT errorCode, ICoreWebView2Environment* result)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OnEnvironmentCreated(errorCode, result);
    else if (result != NULL) result->Release();
    return S_OK;
}

HRESULT WvCtrlHandler::Invoke(HRESULT errorCode, ICoreWebView2Controller* result)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OnControllerCreated(errorCode, result);
    else if (result != NULL) result->Release();
    return S_OK;
}

HRESULT WvEvalHandler::Invoke(HRESULT errorCode, LPCWSTR result)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OnEvalDone(errorCode, result);
    return S_OK;
}

HRESULT WvNoopHandler::Invoke(HRESULT errorCode, LPCWSTR result)
{
    UNREFERENCED_PARAMETER(errorCode);
    UNREFERENCED_PARAMETER(result);
    return S_OK;
}

HRESULT WvJsonHandler::Invoke(HRESULT errorCode, LPCWSTR resultAsJson)
{
    UNREFERENCED_PARAMETER(errorCode);
    UNREFERENCED_PARAMETER(resultAsJson);
    return S_OK;
}

HRESULT WvNavStartHandler::Invoke(ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args)
{
    UNREFERENCED_PARAMETER(sender);
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OnNavigationStarting(args);
    return S_OK;
}

HRESULT WvNavDoneHandler::Invoke(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args)
{
    UNREFERENCED_PARAMETER(sender);
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OnNavigationCompleted(args);
    return S_OK;
}

HRESULT WvTitleHandler::Invoke(ICoreWebView2* sender, IUnknown* args)
{
    UNREFERENCED_PARAMETER(args);
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OnTitleChanged(sender);
    return S_OK;
}

HRESULT WvSourceHandler::Invoke(ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs* args)
{
    UNREFERENCED_PARAMETER(sender);
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OnSourceChanged(args);
    return S_OK;
}

HRESULT WvNewWindowHandler::Invoke(ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OnNewWindowRequested(sender, args);
    return S_OK;
}

HRESULT WvProcessFailedHandler::Invoke(ICoreWebView2* sender, ICoreWebView2ProcessFailedEventArgs* args)
{
    UNREFERENCED_PARAMETER(sender);
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OnProcessFailed(args);
    return S_OK;
}

// 上面那批异步回调只做一件事：把结果塞进宿主的结果槽并结束等待。
HRESULT WvTrySuspendHandler::Invoke(HRESULT errorCode, BOOL result)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OpDone(errorCode, result, NULL, NULL, NULL);
    return S_OK;
}
HRESULT WvBoolDoneHandler::Invoke(HRESULT errorCode, BOOL result)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OpDone(errorCode, result, NULL, NULL, NULL);
    return S_OK;
}
HRESULT WvPrintDoneHandler::Invoke(HRESULT errorCode, COREWEBVIEW2_PRINT_STATUS result)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OpDone(errorCode, (INT)result, NULL, NULL, NULL);
    return S_OK;
}
HRESULT WvSaveAsDoneHandler::Invoke(HRESULT errorCode, COREWEBVIEW2_SAVE_AS_UI_RESULT result)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OpDone(errorCode, (INT)result, NULL, NULL, NULL);
    return S_OK;
}
HRESULT WvVoidDoneHandler::Invoke(HRESULT errorCode)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OpDone(errorCode, 0, NULL, NULL, NULL);
    return S_OK;
}
HRESULT WvIdDoneHandler::Invoke(HRESULT errorCode, LPCWSTR id)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OpDone(errorCode, 0, id, NULL, NULL);
    return S_OK;
}
HRESULT WvCookieListHandler::Invoke(HRESULT errorCode, ICoreWebView2CookieList* result)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OpDone(errorCode, 0, NULL, (void*)result, "ICoreWebView2CookieList");
    return S_OK;
}
HRESULT WvExtListHandler::Invoke(HRESULT errorCode, ICoreWebView2BrowserExtensionList* result)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OpDone(errorCode, 0, NULL, (void*)result, "ICoreWebView2BrowserExtensionList");
    return S_OK;
}
HRESULT WvExtItemHandler::Invoke(HRESULT errorCode, ICoreWebView2BrowserExtension* result)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OpDone(errorCode, 0, NULL, (void*)result, "ICoreWebView2BrowserExtension");
    return S_OK;
}
HRESULT WvVoidHandler2::Invoke(HRESULT errorCode)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OpDone(errorCode, 0, NULL, NULL, NULL);
    return S_OK;
}
HRESULT WvVoidHandler3::Invoke(HRESULT errorCode)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OpDone(errorCode, 0, NULL, NULL, NULL);
    return S_OK;
}
HRESULT WvVoidHandler4::Invoke(HRESULT errorCode)
{
    if (m_pHost != NULL && AliveOk(m_pState)) m_pHost->OpDone(errorCode, 0, NULL, NULL, NULL);
    return S_OK;
}

// ============================================================ hUnit 登记表
// hUnit 由易语言运行时给出（MFC CWnd* 或宿主 HWND）。组件销毁后那个值可能变成
// 已释放内存的地址，解引用会直接崩；所以按值登记，查找只走这张表。

struct WV_HUNIT_ITEM
{
    HUNIT          m_hUnit;
    CWebViewHost*  m_pHost;
};

WV_HUNIT_ITEM g_hunitMap[128];
INT           g_hunitCount = 0;

void HunitRegister(HUNIT hUnit, CWebViewHost* pHost)
{
    if (hUnit == 0 || pHost == NULL) return;
    EnsureCs();
    EnterCriticalSection(&g_wvCs);
    for (INT i = 0; i < g_hunitCount; i++)
    {
        if (g_hunitMap[i].m_hUnit == hUnit) { g_hunitMap[i].m_pHost = pHost; LeaveCriticalSection(&g_wvCs); return; }
    }
    INT nSlot = -1;
    for (INT i = 0; i < g_hunitCount; i++)
    {
        if (g_hunitMap[i].m_hUnit == 0) { nSlot = i; break; }
    }
    if (nSlot < 0 && g_hunitCount < 128) nSlot = g_hunitCount++;
    if (nSlot >= 0) { g_hunitMap[nSlot].m_hUnit = hUnit; g_hunitMap[nSlot].m_pHost = pHost; }
    LeaveCriticalSection(&g_wvCs);
}

CWebViewHost* HunitLookup(HUNIT hUnit)
{
    if (hUnit == 0) return NULL;
    CWebViewHost* pFound = NULL;
    EnsureCs();
    EnterCriticalSection(&g_wvCs);
    for (INT i = 0; i < g_hunitCount; i++)
    {
        if (g_hunitMap[i].m_hUnit != hUnit) continue;
        CWebViewHost* pH = g_hunitMap[i].m_pHost;
        if (pH == NULL || pH->m_hWnd == NULL || pH->m_blDeleted)
        {
            g_hunitMap[i].m_hUnit = 0;
            g_hunitMap[i].m_pHost = NULL;
        }
        else
            pFound = pH;
        break;
    }
    LeaveCriticalSection(&g_wvCs);
    return pFound;
}

void HunitUnlink(CWebViewHost* pHost)
{
    if (pHost == NULL) return;
    EnsureCs();
    EnterCriticalSection(&g_wvCs);
    for (INT i = 0; i < g_hunitCount; i++)
    {
        if (g_hunitMap[i].m_pHost == pHost)
        {
            g_hunitMap[i].m_hUnit = 0;
            g_hunitMap[i].m_pHost = NULL;
        }
    }
    LeaveCriticalSection(&g_wvCs);
}

// ============================================================ 事件名 → 槽位 / 命名事件
// 只有「需要应答」的事件占槽位；其余事件走通用「内核事件」+ 取事件数据。
int WvEvtSlotOf(LPCSTR szEventName)
{
    if (szEventName == NULL) return -1;
    if (lstrcmpA(szEventName, "PermissionRequested") == 0)            return WVP_PERMISSION;
    if (lstrcmpA(szEventName, "ScriptDialogOpening") == 0)            return WVP_DIALOG;
    if (lstrcmpA(szEventName, "DownloadStarting") == 0)               return WVP_DOWNLOAD;
    if (lstrcmpA(szEventName, "BasicAuthenticationRequested") == 0)   return WVP_BASEAUTH;
    if (lstrcmpA(szEventName, "ClientCertificateRequested") == 0)     return WVP_CLIENCERT;
    if (lstrcmpA(szEventName, "ServerCertificateErrorDetected") == 0) return WVP_SRVCERT;
    if (lstrcmpA(szEventName, "ContextMenuRequested") == 0)           return WVP_CTXMENU;
    if (lstrcmpA(szEventName, "AcceleratorKeyPressed") == 0)          return WVP_KEY;
    if (lstrcmpA(szEventName, "LaunchingExternalUriScheme") == 0)     return WVP_EXTURI;
    if (lstrcmpA(szEventName, "SaveAsUIShowing") == 0)                return WVP_SAVEASUI;
    if (lstrcmpA(szEventName, "NewWindowRequested") == 0)             return WVP_NEWWINDOW;
    return -1;
}

int WvNamedEventOf(LPCSTR szEventName)
{
    if (szEventName == NULL) return -1;
    if (lstrcmpA(szEventName, "WebMessageReceived") == 0)            return WVEVENT_WEB_MESSAGE;
    if (lstrcmpA(szEventName, "DownloadStarting") == 0)              return WVEVENT_DOWNLOAD;
    if (lstrcmpA(szEventName, "PermissionRequested") == 0)           return WVEVENT_PERMISSION;
    if (lstrcmpA(szEventName, "ScriptDialogOpening") == 0)           return WVEVENT_DIALOG;
    if (lstrcmpA(szEventName, "BasicAuthenticationRequested") == 0)  return WVEVENT_BASE_AUTH;
    if (lstrcmpA(szEventName, "ClientCertificateRequested") == 0)    return WVEVENT_CLIENT_CERT;
    if (lstrcmpA(szEventName, "ServerCertificateErrorDetected") == 0) return WVEVENT_SERVER_CERT;
    if (lstrcmpA(szEventName, "ContextMenuRequested") == 0)          return WVEVENT_CONTEXT_MENU;
    if (lstrcmpA(szEventName, "DOMContentLoaded") == 0)              return WVEVENT_DOM_READY;
    if (lstrcmpA(szEventName, "WindowCloseRequested") == 0)          return WVEVENT_CLOSE_REQ;
    if (lstrcmpA(szEventName, "AcceleratorKeyPressed") == 0)         return WVEVENT_KEY;
    if (lstrcmpA(szEventName, "LaunchingExternalUriScheme") == 0)    return WVEVENT_EXT_URI;
    if (lstrcmpA(szEventName, "SaveAsUIShowing") == 0)               return WVEVENT_SAVEAS_UI;
    return -1;
}

// 用户没应答时把内核默认行为放回去：订阅本身不该让功能消失。
void WvApplyEventFallback(int nSlot, IUnknown* pArgs)
{
    if (pArgs == NULL) return;
    switch (nSlot)
    {
    case WVP_PERMISSION:
        {
            ICoreWebView2PermissionRequestedEventArgs* p = NULL;
            if (SUCCEEDED(pArgs->QueryInterface(__uuidof(ICoreWebView2PermissionRequestedEventArgs), (void**)&p)) && p)
            {
                p->put_State(COREWEBVIEW2_PERMISSION_STATE_DEFAULT);   // 交回内核弹默认授权条
                p->Release();
            }
            break;
        }
    case WVP_SRVCERT:
        {
            ICoreWebView2ServerCertificateErrorDetectedEventArgs* p = NULL;
            if (SUCCEEDED(pArgs->QueryInterface(__uuidof(ICoreWebView2ServerCertificateErrorDetectedEventArgs), (void**)&p)) && p)
            {
                p->put_Action(COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION_DEFAULT);
                p->Release();
            }
            break;
        }
    default:
        break;      // 其余事件的不应答就是内核默认值
    }
}

void WvEventThunk(void* pCtx, const char* szName, const char* szJson, void* pArgs)
{
    CWebViewHost* pH = (CWebViewHost*)pCtx;
    if (pH == NULL) return;
    pH->OnKernelEvent(szName, szJson, (IUnknown*)pArgs);
}

// ============================================================ 载入遮罩
// 不透明地盖在 WebView2 视图上：滚动进度条 + 居中提示，避免跳转期间停在旧页面上。

// 进度条轨道宽度：够宽就固定 240，组件很窄时按客户区留边算。
static INT OverlayTrackWidth(INT nClientW)
{
    INT nW = nClientW - 48;
    if (nW > WV_MARQUEE_TRACK_W) nW = WV_MARQUEE_TRACK_W;
    return nW;
}

LRESULT CALLBACK WvOverlayWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CWebViewHost* pH = (CWebViewHost*)GetPropA(hWnd, WV_PROP_NAME);

    switch (uMsg)
    {
    case WM_TIMER:
        if (pH != NULL)
        {
            // WebView2 会在宿主窗口里建自己的子窗口，心跳里顺手把遮罩按回最上层。
            SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            if (!pH->WantsLoading()) { pH->RefreshLoading(); return 0; }   // 载入已结束：自己收起来
            RECT rc;
            GetClientRect(hWnd, &rc);
            const INT nTrackW = OverlayTrackWidth(rc.right);
            if (nTrackW > 0)
            {
                pH->m_nMarqueePos += 8;
                if (pH->m_nMarqueePos > nTrackW) pH->m_nMarqueePos = -WV_MARQUEE_BLOCK_W;
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        return 0;

    case WM_ERASEBKGND:
        return 1;                       // 全部交给 WM_PAINT，避免闪烁

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);

            HBRUSH hVeil = (HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
            FillRect(hDC, &rc, hVeil);

            const INT nTrackW = OverlayTrackWidth(rc.right);
            const INT nTrackH = 10;
            const INT nTrackX = (rc.right - nTrackW) / 2;
            const INT nTrackY = rc.bottom / 2 + 16;

            if (nTrackW > 0)
            {
                RECT rcTrack = { nTrackX, nTrackY, nTrackX + nTrackW, nTrackY + nTrackH };
                HBRUSH hTrack = CreateSolidBrush(RGB(225, 225, 225));
                FillRect(hDC, &rcTrack, hTrack);
                DeleteObject(hTrack);
                FrameRect(hDC, &rcTrack, (HBRUSH)GetStockObject(GRAY_BRUSH));

                INT nPos = (pH != NULL) ? pH->m_nMarqueePos : 0;
                if (nPos < 0) nPos = 0;
                if (nPos > nTrackW) nPos = 0;
                INT bx0 = nTrackX + nPos;
                INT bx1 = bx0 + WV_MARQUEE_BLOCK_W;
                if (bx0 < nTrackX + 1) bx0 = nTrackX + 1;
                if (bx1 > nTrackX + nTrackW) bx1 = nTrackX + nTrackW;
                if (bx1 > bx0)
                {
                    RECT rcBlock = { bx0, nTrackY + 1, bx1, nTrackY + nTrackH - 1 };
                    FillRect(hDC, &rcBlock, (HBRUSH)GetSysColorBrush(COLOR_HIGHLIGHT));
                }
            }

            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            HFONT hOld = (HFONT)SelectObject(hDC, hFont);
            SIZE szText;
            GetTextExtentPoint32A(hDC, WV_LOADING_TEXT, (int)lstrlenA(WV_LOADING_TEXT), &szText);
            SetBkMode(hDC, TRANSPARENT);
            SetTextColor(hDC, GetSysColor(COLOR_GRAYTEXT));
            TextOutA(hDC, (rc.right - szText.cx) / 2, rc.bottom / 2 - 28,
                     WV_LOADING_TEXT, (int)lstrlenA(WV_LOADING_TEXT));
            SelectObject(hDC, hOld);
            EndPaint(hWnd, &ps);
            return 0;
        }

    case WM_DESTROY:
        KillTimer(hWnd, WV_OVERLAY_TIMER);
        SetPropA(hWnd, WV_PROP_NAME, NULL);
        return 0;

    default:
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
}

// ============================================================ 宿主窗口过程

LRESULT CALLBACK WvHostWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CWebViewHost* pH = CWebViewHost::FromHwnd(hWnd);

    switch (uMsg)
    {
    case WM_SIZE:
        if (pH != NULL) pH->BoundsChanged();
        return 0;

    case WM_SETFOCUS:
        if (pH != NULL && pH->m_pController != NULL)
            pH->m_pController->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        return 0;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(hWnd, &ps);

            LPCSTR pszTip = NULL;
            if (pH != NULL)
            {
                if (pH->m_szHint[0] != '\0') pszTip = pH->m_szHint;           // 载入不了：整框只剩这句话
                else if (pH->m_blInDesignMode) pszTip = WV_LIB_DESIGN_TIP;
            }
            if (pszTip != NULL)
            {
                RECT rc;
                GetClientRect(hWnd, &rc);
                HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
                HFONT hOld = (HFONT)SelectObject(hDC, hFont);
                SIZE szText;
                GetTextExtentPoint32A(hDC, pszTip, (int)lstrlenA(pszTip), &szText);
                INT x = (rc.right - szText.cx) / 2;
                if (x < 4) x = 4;
                SetBkMode(hDC, TRANSPARENT);
                SetTextColor(hDC, GetSysColor(COLOR_GRAYTEXT));
                TextOutA(hDC, x, (rc.bottom - szText.cy) / 2, pszTip, (int)lstrlenA(pszTip));
                SelectObject(hDC, hOld);
            }
            EndPaint(hWnd, &ps);
            return 0;
        }

    case WM_DESTROY:
        if (pH != NULL)
        {
            pH->m_hWnd = NULL;
            pH->ReleaseWebview();
            NotifySys(NRS_UNIT_DESTROIED, pH->m_dwFormID, pH->m_dwUnitID);
        }
        return 0;

    case WM_NCDESTROY:
        if (pH != NULL)
        {
            HunitUnlink(pH);
            SetPropA(hWnd, WV_PROP_NAME, NULL);   // 摘掉属性后，本对象再不可能被查找到
            pH->m_hWnd = NULL;
            AliveKill(pH->m_pState);
            delete pH;
        }
        return 0;

    default:
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
}

// ============================================================ 反射层入口
// 把「组名 + 成员名」解析到具体 COM 对象上；返回的指针由调用方 Release。
BOOL ReflResolveTarget(CWebViewHost* pH, WvReflTarget t, IUnknown** ppObj)
{
    if (ppObj == NULL) return FALSE;
    *ppObj = NULL;
    if (pH == NULL) return FALSE;

    switch (t)
    {
    case RF_T_CORE:
        if (pH->m_pWebview == NULL) return FALSE;
        *ppObj = pH->m_pWebview;
        break;
    case RF_T_CONTROLLER:
        if (pH->m_pController == NULL) return FALSE;
        *ppObj = pH->m_pController;
        break;
    case RF_T_ENV:
        if (pH->m_pEnvironment == NULL) return FALSE;
        *ppObj = pH->m_pEnvironment;
        break;
    case RF_T_SETTINGS:
        {
            ICoreWebView2Settings* p = NULL;
            if (pH->m_pWebview == NULL || pH->m_pWebview->get_Settings(&p) != S_OK || p == NULL) return FALSE;
            *ppObj = p;
            return TRUE;
        }
    case RF_T_COOKIEMGR:
        {
            if (pH->m_pWebview == NULL) return FALSE;
            ICoreWebView2_2* p2 = NULL;
            if (FAILED(pH->m_pWebview->QueryInterface(__uuidof(ICoreWebView2_2), (void**)&p2)) || p2 == NULL) return FALSE;
            ICoreWebView2CookieManager* p = NULL;
            const HRESULT hr = p2->get_CookieManager(&p);
            p2->Release();
            if (FAILED(hr) || p == NULL) return FALSE;
            *ppObj = p;
            return TRUE;
        }
    case RF_T_PROFILE:
        {
            // get_Profile 挂在 ICoreWebView2_13 上（不是环境对象）
            if (pH->m_pWebview == NULL) return FALSE;
            ICoreWebView2_13* p13 = NULL;
            if (FAILED(pH->m_pWebview->QueryInterface(__uuidof(ICoreWebView2_13), (void**)&p13)) || p13 == NULL) return FALSE;
            ICoreWebView2Profile* p = NULL;
            const HRESULT hr = p13->get_Profile(&p);
            p13->Release();
            if (FAILED(hr) || p == NULL) return FALSE;
            *ppObj = p;
            return TRUE;
        }
    case RF_T_EVENT:
        if (pH->m_pLastEventObj == NULL) return FALSE;
        *ppObj = pH->m_pLastEventObj;
        break;
    case RF_T_OPTIONS:
        return FALSE;      // 环境选项属于 WebView2环境 对象，不在组件上
    default:
        return FALSE;
    }
    if (*ppObj == NULL) return FALSE;
    (*ppObj)->AddRef();    // 与 get_Settings 等自带引用的分支统一由调用方 Release
    return TRUE;
}

int ReflDispatch(WvReflTarget t, IUnknown* pObj, INT mode, LPCSTR name,
                 INT inInt, DOUBLE inDouble, LPCSTR inText,
                 INT* outInt, DOUBLE* outDouble, LPSTR outText, INT cap)
{
    #define WV_REFLECT_TRIPLE(g) \
        return mode == 0 ? ReflGet_##g(pObj, name, outInt, outDouble, outText, cap) \
             : mode == 1 ? ReflSet_##g(pObj, name, inInt, inDouble, inText) \
                         : ReflCall_##g(pObj, name, inInt, inDouble, inText);
    // mode: 0 读 1 写 2 调用。返回 RF_*，属性值经 outInt/outDouble/outText 带出。
    switch (t)
    {
    case RF_T_CORE:       return mode == 0 ? ReflGet_CORE(pObj, name, outInt, outDouble, outText, cap)
                                     : mode == 1 ? ReflSet_CORE(pObj, name, inInt, inDouble, inText)
                                                 : ReflCall_CORE(pObj, name, inInt, inDouble, inText);
    case RF_T_SETTINGS:   return mode == 0 ? ReflGet_SETTINGS(pObj, name, outInt, outDouble, outText, cap)
                                     : mode == 1 ? ReflSet_SETTINGS(pObj, name, inInt, inDouble, inText)
                                                 : ReflCall_SETTINGS(pObj, name, inInt, inDouble, inText);
    case RF_T_CONTROLLER: return mode == 0 ? ReflGet_CONTROLLER(pObj, name, outInt, outDouble, outText, cap)
                                     : mode == 1 ? ReflSet_CONTROLLER(pObj, name, inInt, inDouble, inText)
                                                 : ReflCall_CONTROLLER(pObj, name, inInt, inDouble, inText);
    case RF_T_ENV:        return mode == 0 ? ReflGet_ENV(pObj, name, outInt, outDouble, outText, cap)
                                     : mode == 1 ? ReflSet_ENV(pObj, name, inInt, inDouble, inText)
                                                 : ReflCall_ENV(pObj, name, inInt, inDouble, inText);
    case RF_T_OPTIONS:    return mode == 0 ? ReflGet_OPTIONS(pObj, name, outInt, outDouble, outText, cap)
                                     : mode == 1 ? ReflSet_OPTIONS(pObj, name, inInt, inDouble, inText)
                                                 : ReflCall_OPTIONS(pObj, name, inInt, inDouble, inText);
    case RF_T_COOKIEMGR:  return mode == 0 ? ReflGet_COOKIEMGR(pObj, name, outInt, outDouble, outText, cap)
                                     : mode == 1 ? ReflSet_COOKIEMGR(pObj, name, inInt, inDouble, inText)
                                                 : ReflCall_COOKIEMGR(pObj, name, inInt, inDouble, inText);
    case RF_T_PROFILE:    return mode == 0 ? ReflGet_PROFILE(pObj, name, outInt, outDouble, outText, cap)
                                     : mode == 1 ? ReflSet_PROFILE(pObj, name, inInt, inDouble, inText)
                                                 : ReflCall_PROFILE(pObj, name, inInt, inDouble, inText);
    case RF_T_COOKIE:       WV_REFLECT_TRIPLE(COOKIE)
    case RF_T_FRAME:        WV_REFLECT_TRIPLE(FRAME)
    case RF_T_DOWNLOAD:     WV_REFLECT_TRIPLE(DOWNLOAD)
    case RF_T_REQUEST:      WV_REFLECT_TRIPLE(REQUEST)
    case RF_T_RESPONSE:     WV_REFLECT_TRIPLE(RESPONSE)
    case RF_T_PROCESSINFO:  WV_REFLECT_TRIPLE(PROCESSINFO)
    case RF_T_RECEIVER:     WV_REFLECT_TRIPLE(RECEIVER)
    case RF_T_CERT:         WV_REFLECT_TRIPLE(CERT)
    case RF_T_EVENT:
        return ReflDispatchAny(pObj, mode, name, inInt, inDouble, inText, outInt, outDouble, outText, cap, NULL);
    default: return RF_ERR;
    }
}

// ============================================================ ③ 组件对外接口

CWebViewHost* HostFromHunit(HUNIT hUnit)
{
    if (hUnit == 0) return NULL;

    // hUnit 是运行时给出的 CWnd*，组件销毁后它可能只是块已释放的内存，
    // 所以一律先查登记表；登记表里没有时，才把它当作窗口句柄试一次。
    CWebViewHost* pH = HunitLookup(hUnit);
    if (pH != NULL) return pH;

    if (IsWindow((HWND)hUnit))
    {
        pH = CWebViewHost::FromHwnd((HWND)hUnit);
        if (pH != NULL && pH->m_hWnd != NULL && !pH->m_blDeleted)
        {
            pH->m_hWrapper = (HWND)hUnit;
            HunitRegister(hUnit, pH);
            return pH;
        }
    }
    return NULL;
}

// 组件接口（ITF）实现：静态编译同样要用，别用 __E_STATIC_LIB 挡掉 ——
// 易语言按 "<短名>_GetInterface_<组件英文名>" 挑符号，挡掉就是 LNK2001。
extern "C" HUNIT WINAPI pvi_CreateUnit(LPBYTE pAllData, INT nAllDataSize,
    DWORD dwStyle, HWND hParentWnd, UINT uID, HMENU hMenu, INT x, INT y, INT cx, INT cy,
    DWORD dwWinFormID, DWORD dwUnitID, HWND hDesignWnd, BOOL blInDesignMode)
{
    UNREFERENCED_PARAMETER(hMenu);
    UNREFERENCED_PARAMETER(hDesignWnd);

    CWebViewHost* pH = CWebViewHost::Create(hParentWnd, dwStyle, uID, x, y, cx, cy);
    if (pH == NULL) return 0;

    pH->m_dwFormID = dwWinFormID;
    pH->m_dwUnitID = dwUnitID;
    pH->m_blInDesignMode = blInDesignMode;
    if (pAllData != NULL && nAllDataSize == (INT)sizeof(WV_PROP_DATA))
        pH->SetPropData((const WV_PROP_DATA*)pAllData);

    if (!blInDesignMode)
    {
        ShowWindow(pH->m_hWnd, SW_SHOW);
        if (!ProbeRuntime())
        {
            // 没有 Edge 环境：直接显示提示，不去创建内核，也不弹任何错误
            pH->ShowHint(g_pfnCreateEnv == NULL ? WV_HINT_NO_LOADER : WV_HINT_NO_EDGE);
        }
        else if (pH->m_prop.m_szAddress[0] != '\0') pH->EnsureStarted();
    }

    // HUNIT 必须是易语言运行时认可的 CWnd*，用运行时服务把宿主窗口包一层。
    HUNIT hUnit = (HUNIT)(DWORD_PTR)NotifySys(NAS_CREATE_CWND_OBJECT_FROM_HWND, (DWORD)pH->m_hWnd, 0);
    if (hUnit == 0) hUnit = (HUNIT)(DWORD_PTR)pH->m_hWnd;     // 兜底：按窗口句柄处理
    pH->m_hWrapper = (HWND)hUnit;
    HunitRegister(hUnit, pH);
    return hUnit;
}

// 关键约定：ITF 属性接口（PROPERTY_UPDATE_UI / NOTIFY_PROPERTY_CHANGED / GET_PROPERTY_DATA）
// 的下标是从**第一个自定义属性**开始数的，不含系统自己维护的 8 个固定属性。
// 官方范例 GetPropertyData_HtmlViewer 里 case 3 才是「地址」，就是这个道理。
// 固定属性（位置/尺寸/可视/禁止/标记/鼠标指针）由系统直接作用在组件窗口上，库不要插手。
#define WVCP_ADDRESS       0     // 网址
#define WVCP_CACHE_FOLDER  1     // 缓存目录
#define WVCP_NEW_WINDOW    2     // 允许打开新窗口
#define WVCP_DEV_TOOLS     3     // 允许开发者工具
#define WVCP_ZOOM          4     // 缩放比例
#define WVCP_LOADING       5     // 载入提示
#define WVCP_COUNT         6

extern "C" BOOL WINAPI pvi_PropertyUpdateUI(HUNIT hUnit, INT nPropertyIndex)
{
    CWebViewHost* pH = HostFromHunit(hUnit);
    if (pH == NULL) return TRUE;
    if (nPropertyIndex == WVCP_CACHE_FOLDER && pH->IsWebviewStarted()) return FALSE;
    return TRUE;
}

extern "C" BOOL WINAPI pvi_NotifyPropertyChanged(HUNIT hUnit, INT nPropertyIndex,
    PUNIT_PROPERTY_VALUE pPropertyValue, LPTSTR* ppszTipText)
{
    if (ppszTipText != NULL) *ppszTipText = NULL;
    CWebViewHost* pH = HostFromHunit(hUnit);
    if (pH == NULL || pPropertyValue == NULL) return FALSE;

    switch (nPropertyIndex)
    {
    case WVCP_ADDRESS:
        if (pPropertyValue->m_szText == NULL) break;
        StorePropText(pH->m_prop.m_szAddress, (INT)sizeof(pH->m_prop.m_szAddress), (LPCSTR)pPropertyValue->m_szText);
        if (!pH->m_blInDesignMode) pH->NavigateUtf8(pH->m_prop.m_szAddress);
        break;

    case WVCP_CACHE_FOLDER:
        if (pPropertyValue->m_szText == NULL) break;
        if (pH->IsWebviewStarted())
        {
            ReportError("缓存目录只能在页面第一次打开前设置。");
            break;
        }
        StorePropText(pH->m_prop.m_szCacheFolder, (INT)sizeof(pH->m_prop.m_szCacheFolder), (LPCSTR)pPropertyValue->m_szText);
        break;

    case WVCP_NEW_WINDOW:
        pH->m_prop.m_blAllowNewWindow = pPropertyValue->m_bool ? TRUE : FALSE;
        break;

    case WVCP_DEV_TOOLS:
        pH->m_prop.m_blAllowDevTools = pPropertyValue->m_bool ? TRUE : FALSE;
        pH->ApplyDevTools();
        break;

    case WVCP_ZOOM:
        pH->m_prop.m_nZoomIndex = pPropertyValue->m_int;
        pH->ApplyZoom();
        break;

    case WVCP_LOADING:
        pH->m_prop.m_blShowLoading = pPropertyValue->m_bool ? TRUE : FALSE;
        pH->RefreshLoading();
        break;

    default:
        return FALSE;
    }
    return FALSE;       // 返回真表示需要重新创建窗口才能生效
}

extern "C" BOOL WINAPI pvi_GetPropertyData(HUNIT hUnit, INT nPropertyIndex, PUNIT_PROPERTY_VALUE pPropertyValue)
{
    CWebViewHost* pH = HostFromHunit(hUnit);
    if (pH == NULL || pPropertyValue == NULL) return FALSE;

    // 必须先清零：属性值是 union，留着上一次的野指针给系统当字符串用就是堆破坏。
    memset(pPropertyValue, 0, sizeof(UNIT_PROPERTY_VALUE));

    switch (nPropertyIndex)
    {
    case WVCP_ADDRESS:
        pPropertyValue->m_szText = pH->m_prop.m_szAddress;
        break;
    case WVCP_CACHE_FOLDER:
        pPropertyValue->m_szText = pH->m_prop.m_szCacheFolder;
        break;
    case WVCP_NEW_WINDOW:
        pPropertyValue->m_bool = pH->m_prop.m_blAllowNewWindow;
        break;
    case WVCP_DEV_TOOLS:
        pPropertyValue->m_bool = pH->m_prop.m_blAllowDevTools;
        break;
    case WVCP_ZOOM:
        pPropertyValue->m_int = pH->m_prop.m_nZoomIndex;
        break;
    case WVCP_LOADING:
        pPropertyValue->m_bool = pH->m_prop.m_blShowLoading;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

extern "C" HGLOBAL WINAPI pvi_GetAllPropertyData(HUNIT hUnit)
{
    CWebViewHost* pH = HostFromHunit(hUnit);
    if (pH == NULL) return NULL;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(WV_PROP_DATA));
    if (hMem == NULL) return NULL;

    WV_PROP_DATA* pData = (WV_PROP_DATA*)GlobalLock(hMem);
    if (pData == NULL) { GlobalFree(hMem); return NULL; }
    pH->GetPropData(pData);
    GlobalUnlock(hMem);
    return hMem;
}

extern "C" PFN_INTERFACE WINAPI pvi_GetInterface_WebView(INT nInterfaceNO)
{
    switch (nInterfaceNO)
    {
    case ITF_CREATE_UNIT:              return (PFN_INTERFACE)pvi_CreateUnit;
    case ITF_PROPERTY_UPDATE_UI:       return (PFN_INTERFACE)pvi_PropertyUpdateUI;
    case ITF_NOTIFY_PROPERTY_CHANGED:  return (PFN_INTERFACE)pvi_NotifyPropertyChanged;
    case ITF_GET_PROPERTY_DATA:        return (PFN_INTERFACE)pvi_GetPropertyData;
    case ITF_GET_ALL_PROPERTY_DATA:    return (PFN_INTERFACE)pvi_GetAllPropertyData;
    default:                           return NULL;
    }
}

// ============================================================ ④ 命令实现

CWebViewHost* Self(PMDATA_INF pArgInf)
{
    if (pArgInf == NULL) return NULL;
    return HostFromHunit((HUNIT)(DWORD_PTR)pArgInf[0].m_unit.m_dwUnitID);
}

void WvConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData);
    UNREFERENCED_PARAMETER(nArgCount);
    if (pArgInf != NULL)
    {
        pArgInf[0].m_unit.m_dwFormID = 0;
        pArgInf[0].m_unit.m_dwUnitID = 0;
    }
}

void WvCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData);
    UNREFERENCED_PARAMETER(nArgCount);
    if (pArgInf == NULL) return;
    pArgInf[0].m_unit.m_dwFormID = pArgInf[1].m_unit.m_dwFormID;
    pArgInf[0].m_unit.m_dwUnitID = pArgInf[1].m_unit.m_dwUnitID;
}

void WvDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData);
    UNREFERENCED_PARAMETER(nArgCount);
    if (pArgInf != NULL)
    {
        pArgInf[0].m_unit.m_dwFormID = 0;
        pArgInf[0].m_unit.m_dwUnitID = 0;
    }
}

void WvTest(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);

    CHAR szOut[512];
    szOut[0] = '\0';
    const BOOL blRuntime = ProbeRuntime();

    if (blRuntime && g_wzRuntimeVersion[0] != L'\0')
    {
        CHAR szVer[128];
        szVer[0] = '\0';
        WideToUtf8(g_wzRuntimeVersion, szVer, (INT)sizeof(szVer));
        wsprintfA(szOut, "Edge WebView2 Runtime %s", szVer);
    }
    else if (blRuntime)
        lstrcpynA(szOut, "已加载 WebView2Loader，但未取到内核版本", (int)sizeof(szOut));
    else if (g_pfnCreateEnv == NULL)
        lstrcpynA(szOut, "未找到 WebView2Loader.dll（请放到程序目录或易语言 lib 目录）", (int)sizeof(szOut));
    else
        lstrcpynA(szOut, "系统里没有 Edge 网页内核（WebView2 Runtime）", (int)sizeof(szOut));

    SetRetText(pRetData, szOut);
}

void WvDetectRuntime(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    SetRetBool(pRetData, ProbeRuntime() ? TRUE : FALSE);
}

void WvGetRuntimeVersion(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);

    if (ProbeRuntime() && g_wzRuntimeVersion[0] != L'\0')
    {
        CHAR szVer[128];
        szVer[0] = '\0';
        WideToUtf8(g_wzRuntimeVersion, szVer, (INT)sizeof(szVer));
        SetRetText(pRetData, szVer);
        return;
    }
    SetRetText(pRetData, "");
}

void WvIsValid(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    SetRetBool(pRetData, Self(pArgInf) != NULL ? TRUE : FALSE);
}

void WvNavigate(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    if (pH == NULL || pArgInf == NULL || pArgInf[1].m_dtDataType != SDT_TEXT)
    {
        SetRetBool(pRetData, FALSE);
        return;
    }
    SetRetBool(pRetData, pH->NavigateUtf8(pArgInf[1].m_pText) ? TRUE : FALSE);
}

void WvLoadHtml(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    if (pH == NULL || pArgInf == NULL || pArgInf[1].m_dtDataType != SDT_TEXT)
    {
        SetRetBool(pRetData, FALSE);
        return;
    }
    SetRetBool(pRetData, pH->NavigateHtmlUtf8(pArgInf[1].m_pText) ? TRUE : FALSE);
}

void WvRefresh(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    BOOL blSkip = FALSE;
    if (pArgInf != NULL && pArgInf[1].m_dtDataType == SDT_BOOL) blSkip = pArgInf[1].m_bool;
    SetRetBool(pRetData, (pH != NULL && pH->Reload(blSkip)) ? TRUE : FALSE);
}

void WvGoBack(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRetData, (pH != NULL && pH->GoBack()) ? TRUE : FALSE);
}

void WvGoForward(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRetData, (pH != NULL && pH->GoForward()) ? TRUE : FALSE);
}

void WvStop(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRetData, (pH != NULL && pH->Stop()) ? TRUE : FALSE);
}

void WvGetPageAddress(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    CHAR szText[WV_TEXT_MAX];
    szText[0] = '\0';
    if (pH != NULL) pH->GetUrlUtf8(szText, (INT)sizeof(szText));
    SetRetText(pRetData, szText);
}

void WvGetPageTitle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    CHAR szText[WV_TEXT_MAX];
    szText[0] = '\0';
    if (pH != NULL) pH->GetTitleUtf8(szText, (INT)sizeof(szText));
    SetRetText(pRetData, szText);
}

void WvGetPageSource(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    if (pH == NULL) { SetRetText(pRetData, ""); return; }

    char* pd = NULL;
    INT nCap = 0;
    // 先用 256KB 池缓冲取回，再按实际长度精确复制一份
    char* pRaw = (char*)NotifySys(NRS_MALLOC, 256 * 1024, 0);
    if (pRaw != NULL)
    {
        pRaw[0] = '\0';
        pH->ReadSourceSync(pRaw, 256 * 1024 - 1);
        nCap = (INT)lstrlenA(pRaw) + 1;
        pd = (char*)NotifySys(NRS_MALLOC, (DWORD)nCap, 0);
        if (pd != NULL) memcpy(pd, pRaw, nCap);
        NotifySys(NRS_MFREE, (DWORD)pRaw, 0);
    }
    pRetData->m_dtDataType = SDT_TEXT;
    pRetData->m_pText = pd;
}

void WvIsLoading(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRetData, (pH != NULL && pH->m_blNavLoading) ? TRUE : FALSE);
}

void WvCanGoBack(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRetData, (pH != NULL && pH->CanGoBack()) ? TRUE : FALSE);
}

void WvCanGoForward(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRetData, (pH != NULL && pH->CanGoForward()) ? TRUE : FALSE);
}

void WvExecuteScript(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    if (pH == NULL || pArgInf == NULL || pArgInf[1].m_dtDataType != SDT_TEXT)
    {
        SetRetBool(pRetData, FALSE);
        return;
    }
    SetRetBool(pRetData, pH->ExecuteScriptUtf8(pArgInf[1].m_pText) ? TRUE : FALSE);
}

void WvEvaluateScript(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    if (pH == NULL || pArgInf == NULL || pArgInf[1].m_dtDataType != SDT_TEXT)
    {
        SetRetText(pRetData, "");
        return;
    }

    INT nTimeout = 5000;
    if (pArgInf[2].m_dtDataType == SDT_INT && pArgInf[2].m_int > 0) nTimeout = pArgInf[2].m_int;

    CHAR szOut[WV_TEXT_MAX * 4];
    szOut[0] = '\0';
    if (!pH->EvaluateScriptUtf8(pArgInf[1].m_pText, nTimeout, szOut, (INT)sizeof(szOut)))
        lstrcpynA(szOut, "", (int)sizeof(szOut));
    SetRetText(pRetData, szOut);
}

void WvGetCacheFolder(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    WCHAR wzFolder[MAX_PATH * 4];
    wzFolder[0] = L'\0';
    if (pH != NULL) pH->ResolveCacheFolder(wzFolder, (INT)(sizeof(wzFolder) / sizeof(WCHAR)));

    CHAR szText[WV_TEXT_MAX];
    WideToUtf8(wzFolder, szText, (INT)sizeof(szText));
    SetRetText(pRetData, szText);
}

void WvSetZoom(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    if (pH == NULL || pArgInf == NULL || pArgInf[1].m_dtDataType != SDT_INT)
    {
        SetRetBool(pRetData, FALSE);
        return;
    }
    SetRetBool(pRetData, pH->SetZoomPercent(pArgInf[1].m_int) ? TRUE : FALSE);
}

void WvOpenDevTools(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pRetData);
    CWebViewHost* pH = Self(pArgInf);
    if (pH != NULL) pH->OpenDevTools();
}

void WvClearCache(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRetData, (pH != NULL && pH->ClearCache()) ? TRUE : FALSE);
}

void WvGetHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetInt(pRetData, (pH != NULL) ? (INT)(DWORD_PTR)pH->m_hWnd : 0);
}

void WvDestroyUnit(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    if (pH == NULL) { SetRetBool(pRetData, FALSE); return; }

    pH->Destroy();            // 对象在 Destroy() 内释放，这里不能再碰 pH
    SetRetBool(pRetData, TRUE);
}

// ---------- 反射层命令：把 262 个属性与长尾方法全部按名打通 ----------

// 从 _SDT_ALL 参数里取一个通用值
static BOOL ReflReadAllArg(PMDATA_INF pArg, INT* pInt, DOUBLE* pDbl, CHAR* pText, INT textCap)
{
    if (pInt) *pInt = 0;
    if (pDbl) *pDbl = 0;
    if (pText && textCap > 0) pText[0] = '\0';
    if (pArg == NULL) return FALSE;
    switch (pArg->m_dtDataType)
    {
    case SDT_BOOL:  if (pInt) *pInt = pArg->m_bool ? 1 : 0; return TRUE;
    case SDT_INT:   case SDT_SHORT: case SDT_BYTE: case SDT_DATE_TIME:
                    if (pInt) *pInt = pArg->m_int; return TRUE;
    case SDT_FLOAT: if (pArg->m_pFloat != NULL)
                    {
                        const DOUBLE d = (DOUBLE)*pArg->m_pFloat;
                        if (pDbl) *pDbl = d;
                        if (pInt) *pInt = (INT)d;
                    }
                    return TRUE;
    case SDT_DOUBLE:if (pDbl) *pDbl = pArg->m_double;
                    if (pInt) *pInt = (INT)pArg->m_double; return TRUE;
    case SDT_TEXT:  if (pText && textCap > 0 && pArg->m_pText) lstrcpynA(pText, pArg->m_pText, textCap);
                    if (pInt && pText) *pInt = (INT)(DWORD_PTR)(LPCSTR)pText; return TRUE;
    default: return FALSE;
    }
}

static BOOL ReflGetTargetFromArgs(CWebViewHost* pH, PMDATA_INF pArgInf, IUnknown** ppObj, WvReflTarget* pT)
{
    if (pH == NULL || pArgInf == NULL || pArgInf[1].m_dtDataType != SDT_TEXT) return FALSE;
    if (!ReflTargetFromName(pArgInf[1].m_pText, pT)) return FALSE;
    return ReflResolveTarget(pH, *pT, ppObj);
}

// 取内核属性(组名, 成员名) -> 通用型
void WvReflGet(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = _SDT_NULL;
    CWebViewHost* pH = Self(pArgInf);
    IUnknown* pObj = NULL;
    WvReflTarget t;
    if (!ReflGetTargetFromArgs(pH, pArgInf, &pObj, &t)) return;
    if (pArgInf[2].m_dtDataType != SDT_TEXT) { pObj->Release(); return; }

    INT nInt = 0; DOUBLE dbl = 0;
    CHAR szText[WV_TEXT_MAX];
    szText[0] = '\0';
    const int rc = ReflDispatch(t, pObj, 0, pArgInf[2].m_pText, 0, 0, NULL, &nInt, &dbl, szText, (INT)sizeof(szText));
    pObj->Release();

    if (rc == RF_OK_TEXT) { SetRetText(pRetData, szText); return; }
    if (rc != RF_OK) return;      // 取不到就返回空白数据，不打断主流程、不弹错
    // 属性原形是小数就按小数回，否则按整数回；逻辑值由成员名带 Is/Are/Can 前缀时给逻辑型
    LPCSTR name = pArgInf[2].m_pText;
    const BOOL blBool = (name != NULL && (_strnicmp(name, "Is", 2) == 0 || _strnicmp(name, "Are", 3) == 0
                        || _strnicmp(name, "Can", 3) == 0 || _strnicmp(name, "Show", 4) == 0));
    if (blBool) SetRetBool(pRetData, nInt ? TRUE : FALSE);
    else if (dbl != 0) { pRetData->m_dtDataType = SDT_DOUBLE; pRetData->m_double = dbl; }
    else { pRetData->m_dtDataType = SDT_INT; pRetData->m_int = nInt; }
}

// 置内核属性(组名, 成员名, 值) -> 逻辑型
void WvReflSet(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    IUnknown* pObj = NULL;
    WvReflTarget t;
    if (!ReflGetTargetFromArgs(pH, pArgInf, &pObj, &t)) { SetRetBool(pRetData, FALSE); return; }
    if (pArgInf[2].m_dtDataType != SDT_TEXT) { pObj->Release(); SetRetBool(pRetData, FALSE); return; }

    INT nInt = 0; DOUBLE dbl = 0;
    CHAR szText[WV_TEXT_MAX];
    ReflReadAllArg(&pArgInf[3], &nInt, &dbl, szText, (INT)sizeof(szText));
    const int rc = ReflDispatch(t, pObj, 1, pArgInf[2].m_pText, nInt, dbl, szText, NULL, NULL, NULL, 0);
    pObj->Release();
    SetRetBool(pRetData, rc == RF_OK ? TRUE : FALSE);
}

// 调用内核方法(组名, 成员名[, 参数]) -> 逻辑型
void WvReflCall(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    IUnknown* pObj = NULL;
    WvReflTarget t;
    if (!ReflGetTargetFromArgs(pH, pArgInf, &pObj, &t)) { SetRetBool(pRetData, FALSE); return; }
    if (pArgInf[2].m_dtDataType != SDT_TEXT) { pObj->Release(); SetRetBool(pRetData, FALSE); return; }

    INT nInt = 0; DOUBLE dbl = 0;
    CHAR szText[WV_TEXT_MAX];
    szText[0] = '\0';
    if (nArgCount >= 4) ReflReadAllArg(&pArgInf[3], &nInt, &dbl, szText, (INT)sizeof(szText));
    const int rc = ReflDispatch(t, pObj, 2, pArgInf[2].m_pText, nInt, dbl, szText, NULL, NULL, NULL, 0);
    pObj->Release();
    SetRetBool(pRetData, rc == RF_OK ? TRUE : FALSE);
}

// 取内核成员列表(组名) -> 文本型（每行一个成员名）
void WvReflNames(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    SetRetText(pRetData, NULL);
    if (pArgInf == NULL || pArgInf[1].m_dtDataType != SDT_TEXT) return;
    WvReflTarget t;
    if (!ReflTargetFromName(pArgInf[1].m_pText, &t)) return;

    static char s_buf[24 * 1024];
    switch (t)
    {
    case RF_T_CORE: ReflNames_CORE(s_buf, (INT)sizeof(s_buf)); break;
    case RF_T_SETTINGS: ReflNames_SETTINGS(s_buf, (INT)sizeof(s_buf)); break;
    case RF_T_CONTROLLER: ReflNames_CONTROLLER(s_buf, (INT)sizeof(s_buf)); break;
    case RF_T_ENV: ReflNames_ENV(s_buf, (INT)sizeof(s_buf)); break;
    case RF_T_OPTIONS: ReflNames_OPTIONS(s_buf, (INT)sizeof(s_buf)); break;
    case RF_T_COOKIEMGR: ReflNames_COOKIEMGR(s_buf, (INT)sizeof(s_buf)); break;
    case RF_T_PROFILE: ReflNames_PROFILE(s_buf, (INT)sizeof(s_buf)); break;
    default: s_buf[0] = '\0'; break;
    }
    SetRetText(pRetData, s_buf);
}


// ============================================================ ④-2 全量对接命令层
// 三类命令：
//   桥接命令 —— 固定落到反射层的某个 (组, 成员)，一行一个；
//   应答命令 —— 取 OnKernelEvent 挂起的事件参数对象，写回用户的选择；
//   异步命令 —— 需要回调的（截图 / PDF / Cookie / 扩展…），统一 BeginOp + WaitOp 等结果。

static void ArgReadAt(PMDATA_INF pArgInf, INT nIdx, INT* pInt, DOUBLE* pDbl, LPSTR pText, INT nCap)
{
    if (pInt != NULL) *pInt = 0;
    if (pDbl != NULL) *pDbl = 0;
    if (pText != NULL && nCap > 0) pText[0] = '\0';
    if (pArgInf == NULL || nIdx < 0) return;

    PMDATA_INF p = &pArgInf[nIdx];
    switch (p->m_dtDataType)
    {
    case SDT_BOOL:  if (pInt) *pInt = p->m_bool ? 1 : 0; break;
    case SDT_INT:
    case SDT_INT64: if (pInt) *pInt = p->m_int; break;
    case SDT_DOUBLE:
    case SDT_FLOAT:
        if (pDbl) *pDbl = p->m_double;
        if (pInt) *pInt = (INT)p->m_double;
        break;
    case SDT_TEXT:
        if (pText != NULL && nCap > 0 && p->m_pText != NULL) lstrcpynA(pText, p->m_pText, nCap);
        break;
    default: break;
    }
}

static BOOL ArgGiven(PMDATA_INF pArgInf, INT nArgCount, INT nIdx)
{
    return (pArgInf != NULL && nArgCount > nIdx && pArgInf[nIdx].m_dtDataType != _SDT_NULL) ? TRUE : FALSE;
}

// out: 0 逻辑型 1 文本型 2 整数型 3 小数型
static void BridgeRet(PMDATA_INF pRet, CWebViewHost* pH, WvReflTarget t, INT mode, LPCSTR member,
                      PMDATA_INF pArgInf, INT nArgIdx, INT nOut, INT nArgCount)
{
    SetRetBool(pRet, FALSE);
    IUnknown* pObj = NULL;
    if (!ReflResolveTarget(pH, t, &pObj) || pObj == NULL) return;

    INT nInt = 0; DOUBLE dbl = 0;
    CHAR szIn[WV_TEXT_MAX];
    szIn[0] = '\0';
    if (nArgIdx > 0) ArgReadAt(pArgInf, nArgIdx, &nInt, &dbl, szIn, (INT)sizeof(szIn));

    INT nOutInt = 0; DOUBLE dOut = 0;
    CHAR szOut[WV_TEXT_MAX];
    szOut[0] = '\0';
    const int rc = ReflDispatch(t, pObj, mode, member, nInt, dbl, szIn, &nOutInt, &dOut, szOut, (INT)sizeof(szOut));
    pObj->Release();

    if (rc != RF_OK && rc != RF_OK_TEXT) return;
    if (mode != 0) { SetRetBool(pRet, TRUE); return; }   // 写 / 调 只关心成没成，成功值不在 outInt 里
    switch (nOut)
    {
    case 1: SetRetText(pRet, szOut); break;
    case 2: SetRetInt(pRet, nOutInt); break;
    case 3: pRet->m_dtDataType = SDT_DOUBLE; pRet->m_double = dOut; break;
    default: SetRetBool(pRet, nOutInt ? TRUE : FALSE); break;
    }
}

#define WV_BRIDGE_CALL(fn, grp, member) \
    void fn(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf) { \
        UNREFERENCED_PARAMETER(nArgCount); \
        BridgeRet(pRet, Self(pArgInf), RF_T_##grp, 2, member, pArgInf, 0, 0, nArgCount); }
#define WV_BRIDGE_GET(fn, grp, member, out) \
    void fn(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf) { \
        UNREFERENCED_PARAMETER(nArgCount); \
        BridgeRet(pRet, Self(pArgInf), RF_T_##grp, 0, member, pArgInf, 0, out, nArgCount); }
#define WV_BRIDGE_SET(fn, grp, member) \
    void fn(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf) { \
        BridgeRet(pRet, Self(pArgInf), RF_T_##grp, 1, member, pArgInf, 1, 0, nArgCount); }

template<class Iface>
static Iface* PendingArgs(CWebViewHost* pH, INT nSlot)
{
    Iface* p = NULL;
    IUnknown* pUnk = (pH != NULL) ? pH->Pending(nSlot) : NULL;
    if (pUnk == NULL) return NULL;
    if (FAILED(pUnk->QueryInterface(__uuidof(Iface), (void**)&p)) || p == NULL) return NULL;
    return p;
}

// nCapBytes 按字节传（调用点一律 sizeof(wzXxx)）：按字符数解释会把宽缓冲区写穿一半。
static void TextToWideBuf(LPCSTR szUtf8, LPWSTR pwszDst, INT nCapBytes)
{
    pwszDst[0] = L'\0';
    if (szUtf8 != NULL && szUtf8[0] != '\0') Utf8ToWide(szUtf8, pwszDst, nCapBytes / (INT)sizeof(WCHAR));
}

// ---------------- 事件订阅 / 数据 ----------------

void WvSubEvt(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    CHAR szName[WV_TEXT_MAX], szGroup[WV_TEXT_MAX];
    szName[0] = szGroup[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szName, (INT)sizeof(szName));
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, NULL, NULL, szGroup, (INT)sizeof(szGroup));
    SetRetBool(pRet, (pH != NULL && szName[0] != '\0')
                     ? (szGroup[0] != '\0' ? pH->SubscribeKernel(szGroup, szName) : pH->SubscribeKernelAuto(szName))
                     : FALSE);
}

void WvUnsubEvt(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    CHAR szName[WV_TEXT_MAX];
    szName[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szName, (INT)sizeof(szName));
    SetRetBool(pRet, (pH != NULL && szName[0] != '\0') ? pH->UnsubscribeKernel(NULL, szName) : FALSE);
}

void WvEvtList(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CHAR szGroup[WV_TEXT_MAX];
    szGroup[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szGroup, (INT)sizeof(szGroup));

    static CHAR szBuf[16 * 1024];
    INT nUsed = 0;
    szBuf[0] = '\0';
    if (szGroup[0] != '\0')
    {
        WvReflTarget t;
        if (ReflTargetFromName(szGroup, &t))
        {
            WvEvtNames(ReflTargetName(t), szBuf, (INT)sizeof(szBuf));
        }
    }
    else
    {
        for (INT i = 0; i < WvEvtGroupCount(); i++)
        {
            CHAR szOne[8 * 1024];
            WvEvtNames(WvEvtGroupName(i), szOne, (INT)sizeof(szOne));
            if (szOne[0] == '\0') continue;
            nUsed += snprintf(szBuf + nUsed, (size_t)(sizeof(szBuf) - nUsed), "%s[%s]\n%s",
                              nUsed ? "\n" : "", WvEvtGroupName(i), szOne);
            if (nUsed >= (INT)sizeof(szBuf)) break;
        }
    }
    SetRetText(pRet, szBuf);
}

void WvEvtName(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetText(pRet, (pH != NULL) ? pH->m_szLastEventName : "");
}

void WvEvtData(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetText(pRet, (pH != NULL) ? pH->LastEventJson() : "");
}

// ---------------- 应答命令 ----------------

void WvAnsPermission(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    INT nState = 0;
    ArgReadAt(pArgInf, 1, &nState, NULL, NULL, 0);
    ICoreWebView2PermissionRequestedEventArgs* p =
        PendingArgs<ICoreWebView2PermissionRequestedEventArgs>(pH, WVP_PERMISSION);
    if (p == NULL) return;
    const HRESULT hr = p->put_State((COREWEBVIEW2_PERMISSION_STATE)nState);
    p->Release();
    if (SUCCEEDED(hr) && pH != NULL) { pH->MarkAnswered(WVP_PERMISSION); SetRetBool(pRet, TRUE); }
}

void WvAnsDialog(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    INT nAccept = 0;
    CHAR szText[WV_TEXT_MAX];
    ArgReadAt(pArgInf, 1, &nAccept, NULL, NULL, 0);
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, NULL, NULL, szText, (INT)sizeof(szText));
    else szText[0] = '\0';

    ICoreWebView2ScriptDialogOpeningEventArgs* p =
        PendingArgs<ICoreWebView2ScriptDialogOpeningEventArgs>(pH, WVP_DIALOG);
    if (p == NULL) return;

    WCHAR wzText[WV_TEXT_MAX];
    TextToWideBuf(szText, wzText, (INT)sizeof(wzText));
    HRESULT hr = S_OK;
    if (szText[0] != '\0') hr = p->put_ResultText(wzText);
    if (SUCCEEDED(hr) && nAccept) hr = p->Accept();
    p->Release();
    if (SUCCEEDED(hr) && pH != NULL) { pH->MarkAnswered(WVP_DIALOG); SetRetBool(pRet, TRUE); }
}

void WvAnsDownload(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    INT nCancel = 0, nHandled = 1;
    CHAR szPath[WV_TEXT_MAX];
    szPath[0] = '\0';
    ArgReadAt(pArgInf, 1, &nCancel, NULL, NULL, 0);
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, NULL, NULL, szPath, (INT)sizeof(szPath));
    if (ArgGiven(pArgInf, nArgCount, 3)) ArgReadAt(pArgInf, 3, &nHandled, NULL, NULL, 0);

    ICoreWebView2DownloadStartingEventArgs* p =
        PendingArgs<ICoreWebView2DownloadStartingEventArgs>(pH, WVP_DOWNLOAD);
    if (p == NULL) return;
    HRESULT hr = S_OK;
    if (szPath[0] != '\0')
    {
        WCHAR wzPath[WV_TEXT_MAX];
        TextToWideBuf(szPath, wzPath, (INT)sizeof(wzPath));
        hr = p->put_ResultFilePath(wzPath);
    }
    if (SUCCEEDED(hr)) hr = p->put_Cancel(nCancel ? TRUE : FALSE);
    if (SUCCEEDED(hr)) hr = p->put_Handled(nHandled ? TRUE : FALSE);
    p->Release();
    if (SUCCEEDED(hr) && pH != NULL) { pH->MarkAnswered(WVP_DOWNLOAD); SetRetBool(pRet, TRUE); }
}

void WvAnsBaseAuth(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    INT nCancel = 0;
    CHAR szUser[WV_TEXT_MAX], szPass[WV_TEXT_MAX];
    szUser[0] = szPass[0] = '\0';
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, NULL, NULL, szUser, (INT)sizeof(szUser));
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, NULL, NULL, szPass, (INT)sizeof(szPass));
    if (ArgGiven(pArgInf, nArgCount, 3)) ArgReadAt(pArgInf, 3, &nCancel, NULL, NULL, 0);

    ICoreWebView2BasicAuthenticationRequestedEventArgs* p =
        PendingArgs<ICoreWebView2BasicAuthenticationRequestedEventArgs>(pH, WVP_BASEAUTH);
    if (p == NULL) return;
    HRESULT hr = S_OK;
    if (szUser[0] != '\0' || szPass[0] != '\0')
    {
        ICoreWebView2BasicAuthenticationResponse* pResp = NULL;
        if (SUCCEEDED(p->get_Response(&pResp)) && pResp != NULL)
        {
            WCHAR wzUser[WV_TEXT_MAX], wzPass[WV_TEXT_MAX];
            TextToWideBuf(szUser, wzUser, (INT)sizeof(wzUser));
            TextToWideBuf(szPass, wzPass, (INT)sizeof(wzPass));
            if (szUser[0] != '\0') pResp->put_UserName(wzUser);
            if (szPass[0] != '\0') pResp->put_Password(wzPass);
            pResp->Release();
        }
    }
    if (SUCCEEDED(hr)) hr = p->put_Cancel(nCancel ? TRUE : FALSE);
    p->Release();
    if (SUCCEEDED(hr) && pH != NULL) { pH->MarkAnswered(WVP_BASEAUTH); SetRetBool(pRet, TRUE); }
}

void WvAnsClientCert(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    INT nIndex = -1, nCancel = 0;
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, &nIndex, NULL, NULL, 0);
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, &nCancel, NULL, NULL, 0);

    ICoreWebView2ClientCertificateRequestedEventArgs* p =
        PendingArgs<ICoreWebView2ClientCertificateRequestedEventArgs>(pH, WVP_CLIENCERT);
    if (p == NULL) return;
    HRESULT hr = S_OK;
    if (nIndex >= 0)
    {
        ICoreWebView2ClientCertificateCollection* pList = NULL;
        ICoreWebView2ClientCertificate* pCert = NULL;
        if (SUCCEEDED(p->get_MutuallyTrustedCertificates(&pList)) && pList != NULL)
        {
            UINT32 nCnt = 0;
            pList->get_Count(&nCnt);   // 列表取法：Count + GetValueAtIndex
            if ((UINT32)nIndex < nCnt) pList->GetValueAtIndex(nIndex, &pCert);
            pList->Release();
        }
        if (pCert != NULL) { hr = p->put_SelectedCertificate(pCert); pCert->Release(); }
        else hr = E_FAIL;
    }
    if (SUCCEEDED(hr) || nCancel) hr = p->put_Cancel(nCancel ? TRUE : FALSE);
    if (SUCCEEDED(hr)) hr = p->put_Handled(TRUE);
    p->Release();
    if (SUCCEEDED(hr) && pH != NULL) { pH->MarkAnswered(WVP_CLIENCERT); SetRetBool(pRet, TRUE); }
}

void WvAnsSvcCert(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    INT nAction = 0;
    ArgReadAt(pArgInf, 1, &nAction, NULL, NULL, 0);
    ICoreWebView2ServerCertificateErrorDetectedEventArgs* p =
        PendingArgs<ICoreWebView2ServerCertificateErrorDetectedEventArgs>(pH, WVP_SRVCERT);
    if (p == NULL) return;
    const HRESULT hr = p->put_Action((COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION)nAction);
    p->Release();
    if (SUCCEEDED(hr) && pH != NULL) { pH->MarkAnswered(WVP_SRVCERT); SetRetBool(pRet, TRUE); }
}

void WvAnsMenu(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    INT nCmd = -1, nHandled = 1;
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, &nCmd, NULL, NULL, 0);
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, &nHandled, NULL, NULL, 0);
    ICoreWebView2ContextMenuRequestedEventArgs* p =
        PendingArgs<ICoreWebView2ContextMenuRequestedEventArgs>(pH, WVP_CTXMENU);
    if (p == NULL) return;
    HRESULT hr = S_OK;
    if (nCmd >= 0) hr = p->put_SelectedCommandId(nCmd);
    if (SUCCEEDED(hr)) hr = p->put_Handled(nHandled ? TRUE : FALSE);
    p->Release();
    if (SUCCEEDED(hr) && pH != NULL) { pH->MarkAnswered(WVP_CTXMENU); SetRetBool(pRet, TRUE); }
}

void WvAnsKey(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    INT nBlock = 1;
    ArgReadAt(pArgInf, 1, &nBlock, NULL, NULL, 0);
    ICoreWebView2AcceleratorKeyPressedEventArgs* p =
        PendingArgs<ICoreWebView2AcceleratorKeyPressedEventArgs>(pH, WVP_KEY);
    if (p == NULL) return;
    const HRESULT hr = p->put_Handled(nBlock ? TRUE : FALSE);
    p->Release();
    if (SUCCEEDED(hr) && pH != NULL) { pH->MarkAnswered(WVP_KEY); SetRetBool(pRet, TRUE); }
}

void WvAnsExtUri(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    INT nCancel = 1;
    ArgReadAt(pArgInf, 1, &nCancel, NULL, NULL, 0);
    ICoreWebView2LaunchingExternalUriSchemeEventArgs* p =
        PendingArgs<ICoreWebView2LaunchingExternalUriSchemeEventArgs>(pH, WVP_EXTURI);
    if (p == NULL) return;
    const HRESULT hr = p->put_Cancel(nCancel ? TRUE : FALSE);
    p->Release();
    if (SUCCEEDED(hr) && pH != NULL) { pH->MarkAnswered(WVP_EXTURI); SetRetBool(pRet, TRUE); }
}

void WvAnsSaveAsUI(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    INT nCancel = 0, nReplace = 0, nSuppress = 0;
    CHAR szPath[WV_TEXT_MAX];
    szPath[0] = '\0';
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, NULL, NULL, szPath, (INT)sizeof(szPath));
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, &nCancel, NULL, NULL, 0);
    if (ArgGiven(pArgInf, nArgCount, 3)) ArgReadAt(pArgInf, 3, &nReplace, NULL, NULL, 0);
    if (ArgGiven(pArgInf, nArgCount, 4)) ArgReadAt(pArgInf, 4, &nSuppress, NULL, NULL, 0);
    ICoreWebView2SaveAsUIShowingEventArgs* p =
        PendingArgs<ICoreWebView2SaveAsUIShowingEventArgs>(pH, WVP_SAVEASUI);
    if (p == NULL) return;
    HRESULT hr = S_OK;
    if (szPath[0] != '\0')
    {
        WCHAR wzPath[WV_TEXT_MAX];
        TextToWideBuf(szPath, wzPath, (INT)sizeof(wzPath));
        hr = p->put_SaveAsFilePath(wzPath);
    }
    if (SUCCEEDED(hr)) hr = p->put_AllowReplace(nReplace ? TRUE : FALSE);
    if (SUCCEEDED(hr)) hr = p->put_SuppressDefaultDialog(nSuppress ? TRUE : FALSE);
    if (SUCCEEDED(hr)) hr = p->put_Cancel(nCancel ? TRUE : FALSE);
    p->Release();
    if (SUCCEEDED(hr) && pH != NULL) { pH->MarkAnswered(WVP_SAVEASUI); SetRetBool(pRet, TRUE); }
}

void WvAnsNewWindow(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    INT nHandled = 1;
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, &nHandled, NULL, NULL, 0);
    ICoreWebView2NewWindowRequestedEventArgs* p =
        PendingArgs<ICoreWebView2NewWindowRequestedEventArgs>(pH, WVP_NEWWINDOW);
    if (p == NULL) return;
    const HRESULT hr = p->put_Handled(nHandled ? TRUE : FALSE);
    p->Release();
    if (SUCCEEDED(hr) && pH != NULL) { pH->MarkAnswered(WVP_NEWWINDOW); SetRetBool(pRet, TRUE); }
}

void WvDownloadInfo(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetText(pRet, "");
    ICoreWebView2DownloadStartingEventArgs* p =
        PendingArgs<ICoreWebView2DownloadStartingEventArgs>(pH, WVP_DOWNLOAD);
    if (p == NULL) return;
    ICoreWebView2DownloadOperation* pOp = NULL;
    CHAR szJson[WV_EVT_JSON_MAX];
    szJson[0] = '\0';
    if (SUCCEEDED(p->get_DownloadOperation(&pOp)) && pOp != NULL)
        WvDescribeObject("ICoreWebView2DownloadOperation", (void*)pOp, szJson, (INT)sizeof(szJson));
    if (pOp != NULL) pOp->Release();
    p->Release();
    SetRetText(pRet, szJson);
}

// ---------------- 异步命令 ----------------

void WvTrySuspend(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    if (pH == NULL || pH->m_pWebview == NULL) return;
    INT nMs = 5000;
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, &nMs, NULL, NULL, 0);

    ICoreWebView2_3* p3 = NULL;
    if (FAILED(pH->m_pWebview->QueryInterface(__uuidof(ICoreWebView2_3), (void**)&p3)) || p3 == NULL) return;
    pH->BeginOp();
    WvTrySuspendHandler h(pH, pH->m_pState);
    const HRESULT hr = p3->TrySuspend(&h);
    p3->Release();
    if (FAILED(hr) || !pH->WaitOp(nMs)) return;
    SetRetBool(pRet, pH->m_nOpBool ? TRUE : FALSE);
}

WV_BRIDGE_CALL(WvResume, CORE, "Resume")
WV_BRIDGE_SET(WvSetMuted, CORE, "IsMuted")
WV_BRIDGE_GET(WvIsPlayingAudio, CORE, "IsDocumentPlayingAudio", 0)
WV_BRIDGE_GET(WvBrowserPid, CORE, "BrowserProcessId", 2)
WV_BRIDGE_GET(WvFavicon, CORE, "FaviconUri", 1)
WV_BRIDGE_GET(WvStatusText, CORE, "StatusBarText", 1)
WV_BRIDGE_SET(WvSetStatusText, CORE, "StatusBarText")
WV_BRIDGE_GET(WvGetZoom, CONTROLLER, "ZoomFactor", 3)
WV_BRIDGE_SET(WvSetZoomD, CONTROLLER, "ZoomFactor")
WV_BRIDGE_GET(WvFullScreen, CORE, "ContainsFullScreenElement", 0)
WV_BRIDGE_SET(WvSetMemLevel, CORE, "MemoryUsageTargetLevel")
WV_BRIDGE_GET(WvGetMemLevel, CORE, "MemoryUsageTargetLevel", 2)
WV_BRIDGE_CALL(WvOpenTaskMgr, CORE, "OpenTaskManagerWindow")
WV_BRIDGE_CALL(WvShowDownloadDlg, CORE, "OpenDefaultDownloadDialog")
WV_BRIDGE_CALL(WvCloseDownloadDlg, CORE, "CloseDefaultDownloadDialog")
WV_BRIDGE_CALL(WvPostMsg, CORE, "PostWebMessageAsString")
WV_BRIDGE_CALL(WvPostMsgJson, CORE, "PostWebMessageAsJson")
WV_BRIDGE_CALL(WvDelInitScript, CORE, "RemoveScriptToExecuteOnDocumentCreated")
WV_BRIDGE_CALL(WvUnmapHostName, CORE, "ClearVirtualHostNameToFolderMapping")
WV_BRIDGE_CALL(WvDelReqFilter, CORE, "RemoveWebResourceRequestedFilter")
WV_BRIDGE_GET(WvBrowserVersion, ENV, "BrowserVersionString", 1)
WV_BRIDGE_GET(WvProfileName, PROFILE, "ProfileName", 1)

void WvScreenshot(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    pRet->m_dtDataType = _SDT_NULL;
    if (pH == NULL || pH->m_pWebview == NULL) return;
    INT nFormat = 0, nMs = 8000;
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, &nFormat, NULL, NULL, 0);
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, &nMs, NULL, NULL, 0);

    IStream* pStream = BinToStream(NULL, 0);
    if (pStream == NULL) return;
    pH->BeginOp();
    WvVoidDoneHandler h(pH, pH->m_pState);
    const HRESULT hr = pH->m_pWebview->CapturePreview(
        (nFormat == 1) ? COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_JPEG
                       : COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_PNG, pStream, &h);
    if (SUCCEEDED(hr))
    {
        if (pH->WaitOp(nMs))
        {
            INT nLen = 0;
            LPBYTE pBits = StreamToBin(pStream, &nLen);
            if (pBits != NULL)
            {
                SetRetBin(pRet, pBits, nLen);
                free(pBits);
            }
        }
    }
    pStream->Release();
}

void WvPrintPdf(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    if (pH == NULL || pH->m_pWebview == NULL) return;
    CHAR szPath[WV_TEXT_MAX];
    szPath[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szPath, (INT)sizeof(szPath));
    INT nMs = 15000;
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, &nMs, NULL, NULL, 0);
    if (szPath[0] == '\0') return;

    ICoreWebView2_7* p7 = NULL;
    if (FAILED(pH->m_pWebview->QueryInterface(__uuidof(ICoreWebView2_7), (void**)&p7)) || p7 == NULL) return;
    WCHAR wzPath[WV_TEXT_MAX];
    TextToWideBuf(szPath, wzPath, (INT)sizeof(wzPath));
    pH->BeginOp();
    WvBoolDoneHandler h(pH, pH->m_pState);
    const HRESULT hr = p7->PrintToPdf(wzPath, NULL, &h);
    p7->Release();
    if (FAILED(hr) || !pH->WaitOp(nMs)) return;
    SetRetBool(pRet, pH->m_nOpBool ? TRUE : FALSE);
}

void WvPrintUI(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    if (pH == NULL || pH->m_pWebview == NULL) return;
    INT nKind = 0;
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, &nKind, NULL, NULL, 0);
    ICoreWebView2_16* p16 = NULL;
    if (FAILED(pH->m_pWebview->QueryInterface(__uuidof(ICoreWebView2_16), (void**)&p16)) || p16 == NULL) return;
    const HRESULT hr = p16->ShowPrintUI((COREWEBVIEW2_PRINT_DIALOG_KIND)nKind);
    p16->Release();
    SetRetBool(pRet, SUCCEEDED(hr) ? TRUE : FALSE);
}

void WvPrintPage(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    if (pH == NULL || pH->m_pWebview == NULL) return;
    INT nMs = 20000;
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, &nMs, NULL, NULL, 0);
    ICoreWebView2_16* p16 = NULL;
    if (FAILED(pH->m_pWebview->QueryInterface(__uuidof(ICoreWebView2_16), (void**)&p16)) || p16 == NULL) return;
    pH->BeginOp();
    WvPrintDoneHandler h(pH, pH->m_pState);
    const HRESULT hr = p16->Print(NULL, &h);
    p16->Release();
    if (FAILED(hr) || !pH->WaitOp(nMs)) return;
    SetRetBool(pRet, pH->m_nOpBool ? TRUE : FALSE);
}

void WvShowSaveAs(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetInt(pRet, 0);
    if (pH == NULL || pH->m_pWebview == NULL) return;
    INT nMs = 60000;
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, &nMs, NULL, NULL, 0);
    ICoreWebView2_25* p25 = NULL;
    if (FAILED(pH->m_pWebview->QueryInterface(__uuidof(ICoreWebView2_25), (void**)&p25)) || p25 == NULL) return;
    pH->BeginOp();
    WvSaveAsDoneHandler h(pH, pH->m_pState);
    const HRESULT hr = p25->ShowSaveAsUI(&h);
    p25->Release();
    if (FAILED(hr) || !pH->WaitOp(nMs)) return;
    SetRetInt(pRet, pH->m_nOpBool);
}

void WvAddInitScript(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetText(pRet, "");
    if (pH == NULL || pH->m_pWebview == NULL) return;
    CHAR szCode[WV_TEXT_MAX * 4];
    szCode[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szCode, (INT)sizeof(szCode));
    INT nMs = 5000;
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, &nMs, NULL, NULL, 0);
    if (szCode[0] == '\0') return;

    WCHAR wzCode[WV_TEXT_MAX * 4];
    wzCode[0] = L'\0';
    MultiByteToWideChar(CP_UTF8, 0, szCode, -1, wzCode, (INT)(sizeof(wzCode) / sizeof(WCHAR)));
    pH->BeginOp();
    WvIdDoneHandler h(pH, pH->m_pState);
    const HRESULT hr = pH->m_pWebview->AddScriptToExecuteOnDocumentCreated(wzCode, &h);
    if (FAILED(hr) || !pH->WaitOp(nMs)) return;
    SetRetText(pRet, pH->m_szOpText);
}

void WvDevToolsCall(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetText(pRet, "");
    if (pH == NULL || pH->m_pWebview == NULL) return;
    CHAR szMethod[WV_TEXT_MAX], szArgs[WV_TEXT_MAX * 4];
    szMethod[0] = '\0';
    szArgs[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szMethod, (INT)sizeof(szMethod));
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, NULL, NULL, szArgs, (INT)sizeof(szArgs));
    INT nMs = 8000;
    if (ArgGiven(pArgInf, nArgCount, 3)) ArgReadAt(pArgInf, 3, &nMs, NULL, NULL, 0);
    if (szMethod[0] == '\0') return;

    WCHAR wzMethod[WV_TEXT_MAX], wzArgs[WV_TEXT_MAX * 4];
    TextToWideBuf(szMethod, wzMethod, (INT)sizeof(wzMethod));
    TextToWideBuf(szArgs[0] ? szArgs : "{}", wzArgs, (INT)sizeof(wzArgs));
    pH->BeginOp();
    WvJsonDoneHandler h(pH, pH->m_pState);
    const HRESULT hr = pH->m_pWebview->CallDevToolsProtocolMethod(wzMethod, wzArgs, &h);
    if (FAILED(hr) || !pH->WaitOp(nMs)) return;
    SetRetText(pRet, pH->m_szOpText);
}

void WvDevToolsSub(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    if (pH == NULL || pH->m_pWebview == NULL) return;
    CHAR szName[WV_TEXT_MAX], szParams[WV_TEXT_MAX];
    szName[0] = szParams[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szName, (INT)sizeof(szName));
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, NULL, NULL, szParams, (INT)sizeof(szParams));
    if (szName[0] == '\0') return;

    WCHAR wzName[WV_TEXT_MAX], wzParams[WV_TEXT_MAX];
    TextToWideBuf(szName, wzName, (INT)sizeof(wzName));
    TextToWideBuf(szParams[0] ? szParams : "{}", wzParams, (INT)sizeof(wzParams));

    ICoreWebView2DevToolsProtocolEventReceiver* pRecv = NULL;
    const HRESULT hr = pH->m_pWebview->GetDevToolsProtocolEventReceiver(wzName, &pRecv);
    if (FAILED(hr) || pRecv == NULL) return;

    // 事件名统一加前缀，通用「内核事件」里就能区分出是协议事件。
    CHAR szEvt[96];
    snprintf(szEvt, sizeof(szEvt), "Protocol.%s", szName);
    INT64 tk = 0;
    const BOOL bl = WvEvtSubscribe((IUnknown*)pRecv, "RECEIVER", "DevToolsProtocolEventReceived",
                                   pH, WvEventThunk, &tk);
    if (bl)
    {
        for (INT i = 0; i < (INT)(sizeof(pH->m_sub) / sizeof(pH->m_sub[0])); i++)
        {
            if (pH->m_sub[i].pTarget != NULL) continue;
            pH->m_sub[i].pTarget = (IUnknown*)pRecv;
            lstrcpynA(pH->m_sub[i].szGroup, "receiver", (INT)sizeof(pH->m_sub[i].szGroup));
            lstrcpynA(pH->m_sub[i].szName, szName, (INT)sizeof(pH->m_sub[i].szName));
            pH->m_sub[i].tk = tk;
            break;
        }
    }
    else pRecv->Release();
    SetRetBool(pRet, bl);
    UNREFERENCED_PARAMETER(wzParams);
}

void WvMapHostName(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    if (pH == NULL || pH->m_pWebview == NULL) return;
    CHAR szHost[WV_TEXT_MAX], szFolder[WV_TEXT_MAX];
    szHost[0] = szFolder[0] = '\0';
    INT nLevel = 1;
    ArgReadAt(pArgInf, 1, NULL, NULL, szHost, (INT)sizeof(szHost));
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, NULL, NULL, szFolder, (INT)sizeof(szFolder));
    if (ArgGiven(pArgInf, nArgCount, 3)) ArgReadAt(pArgInf, 3, &nLevel, NULL, NULL, 0);
    if (szHost[0] == '\0' || szFolder[0] == '\0') return;

    WCHAR wzHost[WV_TEXT_MAX], wzFolder[WV_TEXT_MAX];
    TextToWideBuf(szHost, wzHost, (INT)sizeof(wzHost));
    TextToWideBuf(szFolder, wzFolder, (INT)sizeof(wzFolder));
    ICoreWebView2_3* p3 = NULL;
    if (FAILED(pH->m_pWebview->QueryInterface(__uuidof(ICoreWebView2_3), (void**)&p3)) || p3 == NULL) return;
    const HRESULT hr = p3->SetVirtualHostNameToFolderMapping(
        wzHost, wzFolder, (COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND)nLevel);
    p3->Release();
    SetRetBool(pRet, SUCCEEDED(hr) ? TRUE : FALSE);
}

void WvAddReqFilter(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    if (pH == NULL || pH->m_pWebview == NULL) return;
    CHAR szUri[WV_TEXT_MAX];
    szUri[0] = '\0';
    INT nKind = 0;
    ArgReadAt(pArgInf, 1, NULL, NULL, szUri, (INT)sizeof(szUri));
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, &nKind, NULL, NULL, 0);
    if (szUri[0] == '\0') return;
    WCHAR wzUri[WV_TEXT_MAX];
    TextToWideBuf(szUri, wzUri, (INT)sizeof(wzUri));
    SetRetBool(pRet, SUCCEEDED(pH->m_pWebview->AddWebResourceRequestedFilter(
        wzUri, (COREWEBVIEW2_WEB_RESOURCE_CONTEXT)nKind)) ? TRUE : FALSE);
}

void WvAnsRequest(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    if (pH == NULL || pH->m_pEnvironment == NULL) return;
    INT nStatus = 200;
    CHAR szReason[WV_TEXT_MAX], szHeaders[WV_TEXT_MAX * 2], szBody[WV_TEXT_MAX * 4];
    szReason[0] = szHeaders[0] = szBody[0] = '\0';
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, &nStatus, NULL, NULL, 0);
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, NULL, NULL, szReason, (INT)sizeof(szReason));
    if (ArgGiven(pArgInf, nArgCount, 3)) ArgReadAt(pArgInf, 3, NULL, NULL, szHeaders, (INT)sizeof(szHeaders));
    if (ArgGiven(pArgInf, nArgCount, 4)) ArgReadAt(pArgInf, 4, NULL, NULL, szBody, (INT)sizeof(szBody));

    IStream* pStream = NULL;
    INT nBodyLen = (INT)lstrlenA(szBody);
    if (nBodyLen > 0)
    {
        WCHAR wzBody[WV_TEXT_MAX * 4];
        wzBody[0] = L'\0';
        MultiByteToWideChar(CP_UTF8, 0, szBody, -1, wzBody, (INT)(sizeof(wzBody) / sizeof(WCHAR)));
        const INT nBytes = (INT)(lstrlenW(wzBody) * sizeof(WCHAR));
        pStream = BinToStream(wzBody, nBytes);   // 交给内核的是 UTF-16 文本
    }
    else pStream = BinToStream("", 0);

    WCHAR wzReason[WV_TEXT_MAX], wzHeaders[WV_TEXT_MAX * 2];
    TextToWideBuf(szReason[0] ? szReason : "OK", wzReason, (INT)sizeof(wzReason));
    TextToWideBuf(szHeaders, wzHeaders, (INT)sizeof(wzHeaders));

    ICoreWebView2WebResourceResponse* pResp = NULL;
    const HRESULT hr = pH->m_pEnvironment->CreateWebResourceResponse(
        pStream, nStatus, wzReason, wzHeaders, &pResp);
    pStream->Release();
    if (FAILED(hr) || pResp == NULL) return;

    IUnknown* pReqArgs = pH->m_pLastEventObj;
    if (pReqArgs != NULL)
    {
        ICoreWebView2WebResourceRequestedEventArgs* p = NULL;
        if (SUCCEEDED(pReqArgs->QueryInterface(__uuidof(ICoreWebView2WebResourceRequestedEventArgs), (void**)&p)) && p)
        {
            const HRESULT hr2 = p->put_Response(pResp);
            p->Release();
            pResp->Release();
            if (SUCCEEDED(hr2)) { SetRetBool(pRet, TRUE); return; }
        }
    }
    pResp->Release();
}

void WvGetCookies(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetText(pRet, "");
    IUnknown* pMgr = NULL;
    if (!ReflResolveTarget(pH, RF_T_COOKIEMGR, &pMgr) || pMgr == NULL) return;
    ICoreWebView2CookieManager* pCM = (ICoreWebView2CookieManager*)pMgr;

    CHAR szUrl[WV_TEXT_MAX];
    szUrl[0] = '\0';
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, NULL, NULL, szUrl, (INT)sizeof(szUrl));
    INT nMs = 5000;
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, &nMs, NULL, NULL, 0);

    WCHAR wzUrl[WV_TEXT_MAX];
    TextToWideBuf(szUrl[0] ? szUrl : "https://", wzUrl, (INT)sizeof(wzUrl));
    pH->BeginOp();
    WvCookieListHandler h(pH, pH->m_pState);
    const HRESULT hr = pCM->GetCookies(wzUrl, &h);
    pCM->Release();
    if (FAILED(hr) || !pH->WaitOp(nMs)) return;
    SetRetText(pRet, pH->m_szOpJson);
}

void WvSetCookie(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    IUnknown* pMgr = NULL;
    if (!ReflResolveTarget(pH, RF_T_COOKIEMGR, &pMgr) || pMgr == NULL) return;
    ICoreWebView2CookieManager* pCM = (ICoreWebView2CookieManager*)pMgr;

    CHAR szName[WV_TEXT_MAX], szValue[WV_TEXT_MAX], szDomain[WV_TEXT_MAX], szPath[WV_TEXT_MAX];
    szName[0] = szValue[0] = szDomain[0] = szPath[0] = '\0';
    INT nDays = 0, nSecure = 0, nHttpOnly = 0, nSameSite = 2;
    ArgReadAt(pArgInf, 1, NULL, NULL, szName, (INT)sizeof(szName));
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, NULL, NULL, szValue, (INT)sizeof(szValue));
    if (ArgGiven(pArgInf, nArgCount, 3)) ArgReadAt(pArgInf, 3, NULL, NULL, szDomain, (INT)sizeof(szDomain));
    if (ArgGiven(pArgInf, nArgCount, 4)) ArgReadAt(pArgInf, 4, NULL, NULL, szPath, (INT)sizeof(szPath));
    if (ArgGiven(pArgInf, nArgCount, 5)) ArgReadAt(pArgInf, 5, &nDays, NULL, NULL, 0);
    if (ArgGiven(pArgInf, nArgCount, 6)) ArgReadAt(pArgInf, 6, &nSecure, NULL, NULL, 0);
    if (ArgGiven(pArgInf, nArgCount, 7)) ArgReadAt(pArgInf, 7, &nHttpOnly, NULL, NULL, 0);
    if (ArgGiven(pArgInf, nArgCount, 8)) ArgReadAt(pArgInf, 8, &nSameSite, NULL, NULL, 0);
    if (szName[0] == '\0') { pCM->Release(); return; }

    WCHAR wzName[WV_TEXT_MAX], wzValue[WV_TEXT_MAX], wzDomain[WV_TEXT_MAX], wzPath[WV_TEXT_MAX];
    TextToWideBuf(szName, wzName, (INT)sizeof(wzName));
    TextToWideBuf(szValue, wzValue, (INT)sizeof(wzValue));
    TextToWideBuf(szDomain, wzDomain, (INT)sizeof(wzDomain));
    TextToWideBuf(szPath, wzPath, (INT)sizeof(wzPath));

    ICoreWebView2Cookie* pCookie = NULL;
    HRESULT hr = pCM->CreateCookie(wzName, wzValue,
                                   szDomain[0] ? wzDomain : NULL, szPath[0] ? wzPath : NULL, &pCookie);
    if (SUCCEEDED(hr) && pCookie != NULL)
    {
        if (nDays > 0)
        {
            SYSTEMTIME stNow, stThen;
            GetLocalTime(&stNow);
            FILETIME ftNow, ftThen;
            SystemTimeToFileTime(&stNow, &ftNow);
            LARGE_INTEGER li;
            li.LowPart = ftNow.dwLowDateTime; li.HighPart = ftNow.dwHighDateTime;
            li.QuadPart += (LONGLONG)nDays * 24 * 60 * 60 * 10000000;
            ftThen.dwLowDateTime = li.LowPart; ftThen.dwHighDateTime = li.HighPart;
            DATE dt = 0;
            if (SUCCEEDED(SystemTimeToVariantTime(&stThen, &dt))) pCookie->put_Expires(dt);
        }
        pCookie->put_IsSecure(nSecure ? TRUE : FALSE);
        pCookie->put_IsHttpOnly(nHttpOnly ? TRUE : FALSE);
        pCookie->put_SameSite((COREWEBVIEW2_COOKIE_SAME_SITE_KIND)nSameSite);
        hr = pCM->AddOrUpdateCookie(pCookie);
        pCookie->Release();
    }
    pCM->Release();
    SetRetBool(pRet, (SUCCEEDED(hr)) ? TRUE : FALSE);
}

void WvDelCookie(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    IUnknown* pMgr = NULL;
    if (!ReflResolveTarget(pH, RF_T_COOKIEMGR, &pMgr) || pMgr == NULL) return;
    CHAR szUrl[WV_TEXT_MAX], szName[WV_TEXT_MAX];
    szUrl[0] = szName[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szUrl, (INT)sizeof(szUrl));
    ArgReadAt(pArgInf, 2, NULL, NULL, szName, (INT)sizeof(szName));
    WCHAR wzUrl[WV_TEXT_MAX], wzName[WV_TEXT_MAX];
    TextToWideBuf(szUrl, wzUrl, (INT)sizeof(wzUrl));
    TextToWideBuf(szName, wzName, (INT)sizeof(wzName));
    const HRESULT hr = ((ICoreWebView2CookieManager*)pMgr)->DeleteCookies(wzName, wzUrl);
    pMgr->Release();
    SetRetBool(pRet, SUCCEEDED(hr) ? TRUE : FALSE);
}

void WvDelCookieAll(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    IUnknown* pMgr = NULL;
    if (!ReflResolveTarget(pH, RF_T_COOKIEMGR, &pMgr) || pMgr == NULL) return;
    CHAR szUrl[WV_TEXT_MAX], szName[WV_TEXT_MAX], szPath[WV_TEXT_MAX];
    szUrl[0] = szName[0] = szPath[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szUrl, (INT)sizeof(szUrl));
    ArgReadAt(pArgInf, 2, NULL, NULL, szName, (INT)sizeof(szName));
    if (ArgGiven(pArgInf, nArgCount, 3)) ArgReadAt(pArgInf, 3, NULL, NULL, szPath, (INT)sizeof(szPath));
    WCHAR wzUrl[WV_TEXT_MAX], wzName[WV_TEXT_MAX], wzPath[WV_TEXT_MAX];
    TextToWideBuf(szUrl, wzUrl, (INT)sizeof(wzUrl));
    TextToWideBuf(szName, wzName, (INT)sizeof(wzName));
    TextToWideBuf(szPath, wzPath, (INT)sizeof(wzPath));
    ICoreWebView2CookieManager* pCM = (ICoreWebView2CookieManager*)pMgr;
    const HRESULT hr = (szPath[0] != '\0')
        ? pCM->DeleteCookiesWithDomainAndPath(wzName, wzUrl, wzPath)
        : pCM->DeleteCookies(wzName, wzUrl);
    pCM->Release();
    SetRetBool(pRet, SUCCEEDED(hr) ? TRUE : FALSE);
}

void WvDelAllCookies(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    IUnknown* pMgr = NULL;
    if (!ReflResolveTarget(pH, RF_T_COOKIEMGR, &pMgr) || pMgr == NULL) return;
    const HRESULT hr = ((ICoreWebView2CookieManager*)pMgr)->DeleteAllCookies();
    pMgr->Release();
    SetRetBool(pRet, SUCCEEDED(hr) ? TRUE : FALSE);
}

void WvProcInfos(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetText(pRet, "");
    IUnknown* pEnv = NULL;
    if (!ReflResolveTarget(pH, RF_T_ENV, &pEnv) || pEnv == NULL) return;
    ICoreWebView2Environment8* p8 = NULL;
    CHAR szJson[WV_EVT_JSON_MAX];
    szJson[0] = '\0';
    if (SUCCEEDED(pEnv->QueryInterface(__uuidof(ICoreWebView2Environment8), (void**)&p8)) && p8)
    {
        ICoreWebView2ProcessInfoCollection* pList = NULL;
        if (SUCCEEDED(p8->GetProcessInfos(&pList)) && pList)
        {
            WvDescribeObject("ICoreWebView2ProcessInfoCollection", (void*)pList, szJson, (INT)sizeof(szJson));
            pList->Release();
        }
        p8->Release();
    }
    pEnv->Release();
    SetRetText(pRet, szJson);
}

void WvClearDataAll(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    IUnknown* pProf = NULL;
    if (!ReflResolveTarget(pH, RF_T_PROFILE, &pProf) || pProf == NULL) return;
    ICoreWebView2Profile2* p2 = NULL;
    INT nMs = 10000;
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, &nMs, NULL, NULL, 0);
    if (SUCCEEDED(pProf->QueryInterface(__uuidof(ICoreWebView2Profile2), (void**)&p2)) && p2)
    {
        pH->BeginOp();
        WvVoidHandler2 h(pH, pH->m_pState);
        const HRESULT hr = p2->ClearBrowsingDataAll(&h);
        p2->Release();
        if (SUCCEEDED(hr) && pH->WaitOp(nMs)) SetRetBool(pRet, SUCCEEDED(pH->m_hrOp) ? TRUE : FALSE);
    }
    pProf->Release();
}

// 按位掩码清除指定时刻之前的浏览数据（掩码同内核 COREWEBVIEW2_BROWSING_DATA_KINDS）。
void WvClearDataIn(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    IUnknown* pProf = NULL;
    if (!ReflResolveTarget(pH, RF_T_PROFILE, &pProf) || pProf == NULL) return;
    INT nKinds = 0, nMs = 10000;
    ArgReadAt(pArgInf, 1, &nKinds, NULL, NULL, 0);
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, &nMs, NULL, NULL, 0);
    if (nKinds == 0) { pProf->Release(); return; }

    ICoreWebView2Profile2* p2 = NULL;
    if (SUCCEEDED(pProf->QueryInterface(__uuidof(ICoreWebView2Profile2), (void**)&p2)) && p2)
    {
        pH->BeginOp();
        WvVoidHandler2 h(pH, pH->m_pState);
        const HRESULT hr = p2->ClearBrowsingData((COREWEBVIEW2_BROWSING_DATA_KINDS)nKinds, &h);
        p2->Release();
        if (SUCCEEDED(hr) && pH->WaitOp(nMs)) SetRetBool(pRet, SUCCEEDED(pH->m_hrOp) ? TRUE : FALSE);
    }
    pProf->Release();
}

void WvExtensions(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetText(pRet, "");
    IUnknown* pProf = NULL;
    if (!ReflResolveTarget(pH, RF_T_PROFILE, &pProf) || pProf == NULL) return;
    ICoreWebView2Profile7* p7 = NULL;
    INT nMs = 8000;
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, &nMs, NULL, NULL, 0);
    if (SUCCEEDED(pProf->QueryInterface(__uuidof(ICoreWebView2Profile7), (void**)&p7)) && p7)
    {
        pH->BeginOp();
        WvExtListHandler h(pH, pH->m_pState);
        const HRESULT hr = p7->GetBrowserExtensions(&h);
        p7->Release();
        if (SUCCEEDED(hr) && pH->WaitOp(nMs)) SetRetText(pRet, pH->m_szOpJson);
    }
    pProf->Release();
}

void WvAddExtension(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    IUnknown* pProf = NULL;
    if (!ReflResolveTarget(pH, RF_T_PROFILE, &pProf) || pProf == NULL) return;
    CHAR szFolder[WV_TEXT_MAX];
    szFolder[0] = '\0';
    INT nAllowRun = 0, nMs = 10000;
    ArgReadAt(pArgInf, 1, NULL, NULL, szFolder, (INT)sizeof(szFolder));
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, &nAllowRun, NULL, NULL, 0);
    if (ArgGiven(pArgInf, nArgCount, 3)) ArgReadAt(pArgInf, 3, &nMs, NULL, NULL, 0);

    ICoreWebView2Profile7* p7 = NULL;
    if (szFolder[0] != '\0' && SUCCEEDED(pProf->QueryInterface(__uuidof(ICoreWebView2Profile7), (void**)&p7)) && p7)
    {
        WCHAR wzFolder[WV_TEXT_MAX];
        TextToWideBuf(szFolder, wzFolder, (INT)sizeof(wzFolder));
        pH->BeginOp();
        WvExtItemHandler h(pH, pH->m_pState);
        const HRESULT hr = p7->AddBrowserExtension(wzFolder, &h);
        p7->Release();
        if (FAILED(hr) || !pH->WaitOp(nMs)) return;
        if (SUCCEEDED(pH->m_hrOp)) SetRetBool(pRet, TRUE);
        UNREFERENCED_PARAMETER(nAllowRun);   // 本内核版本装上即生效，启用/停用走 启用当前扩展
    }
    pProf->Release();
}

// 卸载 / 启用作用于「最近一次扩展事件或安装命令带出来的扩展对象」。
void WvRemoveExtension(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    if (pH == NULL || pH->m_pLastEventObj == NULL) return;
    ICoreWebView2BrowserExtension* pExt = NULL;
    INT nMs = 10000;
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, &nMs, NULL, NULL, 0);
    if (FAILED(pH->m_pLastEventObj->QueryInterface(__uuidof(ICoreWebView2BrowserExtension), (void**)&pExt)) || pExt == NULL) return;
    pH->BeginOp();
    WvVoidHandler3 h(pH, pH->m_pState);
    const HRESULT hr = pExt->Remove(&h);
    pExt->Release();
    if (SUCCEEDED(hr) && pH->WaitOp(nMs)) SetRetBool(pRet, SUCCEEDED(pH->m_hrOp) ? TRUE : FALSE);
}

void WvEnableExtension(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    if (pH == NULL || pH->m_pLastEventObj == NULL) return;
    INT nEnable = 1, nMs = 10000;
    ArgReadAt(pArgInf, 1, &nEnable, NULL, NULL, 0);
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, &nMs, NULL, NULL, 0);
    ICoreWebView2BrowserExtension* pExt = NULL;
    if (FAILED(pH->m_pLastEventObj->QueryInterface(__uuidof(ICoreWebView2BrowserExtension), (void**)&pExt)) || pExt == NULL) return;
    pH->BeginOp();
    WvVoidHandler4 h(pH, pH->m_pState);
    const HRESULT hr = pExt->Enable(nEnable ? TRUE : FALSE, &h);
    pExt->Release();
    if (SUCCEEDED(hr) && pH->WaitOp(nMs)) SetRetBool(pRet, SUCCEEDED(pH->m_hrOp) ? TRUE : FALSE);
}

// ---------------- 全局命令 ----------------

void WvCompareVersions(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    SetRetInt(pRet, 0);
    if (g_pfnCompareVersions == NULL || pArgInf == NULL) return;
    CHAR szA[WV_TEXT_MAX], szB[WV_TEXT_MAX];
    szA[0] = szB[0] = '\0';
    ArgReadAt(pArgInf, 0, NULL, NULL, szA, (INT)sizeof(szA));
    ArgReadAt(pArgInf, 1, NULL, NULL, szB, (INT)sizeof(szB));
    if (szA[0] == '\0' || szB[0] == '\0') return;
    WCHAR wzA[WV_TEXT_MAX], wzB[WV_TEXT_MAX];
    TextToWideBuf(szA, wzA, (INT)sizeof(wzA));
    TextToWideBuf(szB, wzB, (INT)sizeof(wzB));
    INT nResult = 0;
    if (SUCCEEDED(g_pfnCompareVersions(wzA, wzB, &nResult))) SetRetInt(pRet, nResult);
}

// ---------------- 事件对象快照 ----------------

void WvEvtObjJson(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetText(pRet, "");
    if (pH == NULL || pH->m_pLastEventObj == NULL) return;
    CHAR szJson[WV_EVT_JSON_MAX];
    szJson[0] = '\0';
    // 事件对象具体是哪一族由事件名决定，逐个候选接口试。
    static const char* const szCandidates[] = {
        "ICoreWebView2DownloadOperation", "ICoreWebView2Frame", "ICoreWebView2FrameInfo",
        "ICoreWebView2WebResourceRequest", "ICoreWebView2WebResourceResponse",
        "ICoreWebView2ProcessInfo", "ICoreWebView2BrowserExtension", "ICoreWebView2Cookie",
        "ICoreWebView2Profile", "ICoreWebView2ClientCertificate",
    };
    for (INT i = 0; i < (INT)(sizeof(szCandidates) / sizeof(szCandidates[0])); i++)
    {
        if (WvDescribeObject(szCandidates[i], (void*)pH->m_pLastEventObj, szJson, (INT)sizeof(szJson))
            && szJson[0] != '\0')
            break;
    }
    SetRetText(pRet, szJson);
}


// ============================================================ ④-2b 事件参数对象命令
// 对象号参数一律「可空」：留空就用最近一次事件的那个对象。

static WV_OBJ_SLOT* ObjSelf(PMDATA_INF pArgInf, INT nArgCount, INT nIdIdx)
{
    INT nId = 0;
    if (pArgInf != NULL && nArgCount > nIdIdx && pArgInf[nIdIdx].m_dtDataType == SDT_INT)
        nId = pArgInf[nIdIdx].m_int;
    if (nId == 0)
    {
        CWebViewHost* pH = Self(pArgInf);
        if (pH == NULL) return NULL;
        nId = pH->m_nLastEventObjId;
    }
    return WvObjSlot(nId);
}

void WvObjCurId(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetInt(pRet, (pH != NULL) ? pH->m_nLastEventObjId : 0);
}

void WvObjIface(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    WV_OBJ_SLOT* p = ObjSelf(pArgInf, nArgCount, 1);
    SetRetText(pRet, (p != NULL) ? p->szIface : "");
}

void WvObjNames(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    WV_OBJ_SLOT* p = ObjSelf(pArgInf, nArgCount, 1);
    SetRetText(pRet, "");
    if (p == NULL) return;
    static CHAR szBuf[8 * 1024];
    WvObjPropNames(p->szIface, szBuf, (INT)sizeof(szBuf));
    SetRetText(pRet, szBuf);
}

// 通用取值：mode 0 文本 1 整数 2 逻辑 3 小数
static void ObjProp(PMDATA_INF pRet, PMDATA_INF pArgInf, INT nArgCount, INT nOut)
{
    pRet->m_dtDataType = _SDT_NULL;
    CHAR szName[WV_TEXT_MAX];
    szName[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szName, (INT)sizeof(szName));
    WV_OBJ_SLOT* p = ObjSelf(pArgInf, nArgCount, 2);
    if (p == NULL || szName[0] == '\0') { if (nOut == 1) SetRetInt(pRet, 0); else if (nOut == 2) SetRetBool(pRet, FALSE); return; }

    INT nInt = 0; DOUBLE dbl = 0;
    CHAR szText[WV_TEXT_MAX * 4];
    szText[0] = '\0';
    const INT rc = WvObjPropGet(p->szIface, (void*)p->pUnk, szName, &nInt, &dbl, szText, (INT)sizeof(szText));
    switch (nOut)
    {
    case 0: SetRetText(pRet, (rc == 2) ? szText : ""); break;
    case 1: SetRetInt(pRet, (rc >= 1) ? nInt : 0); break;
    case 2: SetRetBool(pRet, (rc >= 1 && nInt) ? TRUE : FALSE); break;
    default: pRet->m_dtDataType = SDT_DOUBLE; pRet->m_double = (rc >= 1) ? dbl : 0; break;
    }
}

void WvObjText(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf) { ObjProp(pRet, pArgInf, nArgCount, 0); }
void WvObjInt(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf) { ObjProp(pRet, pArgInf, nArgCount, 1); }
void WvObjBool(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf) { ObjProp(pRet, pArgInf, nArgCount, 2); }
void WvObjDbl(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf) { ObjProp(pRet, pArgInf, nArgCount, 3); }

void WvObjChild(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CHAR szName[WV_TEXT_MAX];
    szName[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szName, (INT)sizeof(szName));
    WV_OBJ_SLOT* p = ObjSelf(pArgInf, nArgCount, 2);
    SetRetInt(pRet, 0);
    if (p == NULL || szName[0] == '\0') return;
    void* pChild = NULL; LPCSTR szChildIface = NULL;
    if (!WvObjPropChild(p->szIface, (void*)p->pUnk, szName, &pChild, &szChildIface) || pChild == NULL) return;
    const INT nId = WvObjAdd((IUnknown*)pChild, szChildIface);
    ((IUnknown*)pChild)->Release();     // 表里已持有一份
    SetRetInt(pRet, nId);
}

void WvObjListNum(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CHAR szName[WV_TEXT_MAX];
    szName[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szName, (INT)sizeof(szName));
    WV_OBJ_SLOT* p = ObjSelf(pArgInf, nArgCount, 2);
    SetRetInt(pRet, 0);
    if (p == NULL) return;
    INT nCnt = 0;
    if (WvObjListCount(p->szIface, (void*)p->pUnk, szName, &nCnt)) SetRetInt(pRet, nCnt);
}

void WvObjListAt(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CHAR szName[WV_TEXT_MAX];
    szName[0] = '\0';
    INT nIndex = 0;
    ArgReadAt(pArgInf, 1, NULL, NULL, szName, (INT)sizeof(szName));
    ArgReadAt(pArgInf, 2, &nIndex, NULL, NULL, 0);
    WV_OBJ_SLOT* p = ObjSelf(pArgInf, nArgCount, 3);
    SetRetInt(pRet, 0);
    if (p == NULL || szName[0] == '\0' || nIndex < 0) return;
    void* pChild = NULL; LPCSTR szChildIface = NULL;
    if (!WvObjPropChildAt(p->szIface, (void*)p->pUnk, szName, nIndex, &pChild, &szChildIface) || pChild == NULL) return;
    const INT nId = WvObjAdd((IUnknown*)pChild, szChildIface);
    ((IUnknown*)pChild)->Release();
    SetRetInt(pRet, nId);
}

void WvObjJson(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    WV_OBJ_SLOT* p = ObjSelf(pArgInf, nArgCount, 1);
    SetRetText(pRet, "");
    if (p == NULL) return;
    static CHAR szJson[WV_EVT_JSON_MAX];
    szJson[0] = '\0';
    WvDescribeObject(p->szIface, (void*)p->pUnk, szJson, (INT)sizeof(szJson));
    SetRetText(pRet, szJson);
}

void WvObjSet(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CHAR szName[WV_TEXT_MAX];
    szName[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szName, (INT)sizeof(szName));
    WV_OBJ_SLOT* p = ObjSelf(pArgInf, nArgCount, 3);
    SetRetBool(pRet, FALSE);
    if (p == NULL || szName[0] == '\0') return;

    INT nInt = 0; DOUBLE dbl = 0;
    CHAR szText[WV_TEXT_MAX];
    szText[0] = '\0';
    if (pArgInf != NULL) ArgReadAt(pArgInf, 2, &nInt, &dbl, szText, (INT)sizeof(szText));
    if (WvObjPropSet(p->szIface, (void*)p->pUnk, szName, nInt, dbl, szText))
    {
        SetRetBool(pRet, TRUE);
    }
}

void WvObjDefer(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    WV_OBJ_SLOT* p = ObjSelf(pArgInf, nArgCount, 1);
    SetRetBool(pRet, FALSE);
    if (p == NULL || p->pDeferral != NULL) { if (p != NULL) SetRetBool(pRet, TRUE); return; }
    void* pOut = NULL;
    if (!WvObjDeferral(p->szIface, (void*)p->pUnk, &pOut) || pOut == NULL) return;
    p->pDeferral = (ICoreWebView2Deferral*)pOut;     // 带回来的引用由表持有
    SetRetBool(pRet, TRUE);
}

void WvObjDeferDone(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    WV_OBJ_SLOT* p = ObjSelf(pArgInf, nArgCount, 1);
    SetRetBool(pRet, FALSE);
    if (p == NULL || p->pDeferral == NULL) return;
    const HRESULT hr = p->pDeferral->Complete();
    p->pDeferral->Release();
    p->pDeferral = NULL;
    SetRetBool(pRet, SUCCEEDED(hr) ? TRUE : FALSE);
}

void WvObjFreeCmd(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    INT nId = 0;
    if (pArgInf != NULL) ArgReadAt(pArgInf, 1, &nId, NULL, NULL, 0);
    SetRetBool(pRet, WvObjFree(nId) ? TRUE : FALSE);
}

// ============================================================ ④-3 WebView2环境 类
// 进程级的内核环境：一个用户数据目录一个环境，多个组件可以共用它，
// 也能在组件之外单独拿版本、进程信息、CDP 之外的环境能力。

class CWebViewEnv
{
public:
    ICoreWebView2Environment* m_pEnv;
    WCHAR               m_wzFolder[MAX_PATH * 4];
    BOOL                m_blDone;
    HRESULT             m_hr;
    ICoreWebView2Environment* m_pResult;

    CWebViewEnv()
    {
        m_pEnv = NULL;
        m_blDone = FALSE;
        m_hr = S_OK;
        m_pResult = NULL;
        m_wzFolder[0] = L'\0';
    }
    ~CWebViewEnv()
    {
        if (m_pEnv != NULL) m_pEnv->Release();
        if (m_pResult != NULL) m_pResult->Release();
    }
};

// 环境创建回调：结果塞进对象自己的槽，命令里泵消息等它。
class WvEnvCreateHandler : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
{
public:
    WvEnvCreateHandler(CWebViewEnv* p) : m_ref(1), m_pObj(p) {}
    virtual ~WvEnvCreateHandler() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler))
            *ppv = (ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; }
        AddRef();
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release()
    {
        const ULONG n = (ULONG)InterlockedDecrement(&m_ref);
        if (n == 0) delete this;
        return n;
    }
    STDMETHODIMP Invoke(HRESULT errorCode, ICoreWebView2Environment* result)
    {
        m_pObj->m_hr = errorCode;
        if (m_pObj->m_pResult != NULL) { m_pObj->m_pResult->Release(); m_pObj->m_pResult = NULL; }
        if (result != NULL) { result->AddRef(); m_pObj->m_pResult = result; }
        m_pObj->m_blDone = TRUE;
        return S_OK;
    }
private:
    LONG m_ref;
    CWebViewEnv* m_pObj;
};

static CWebViewEnv* EnvObj(PMDATA_INF pArgInf)
{
    if (pArgInf == NULL || pArgInf[0].m_pCompoundData == NULL) return NULL;
    return (CWebViewEnv*)(INT_PTR) * (PINT)pArgInf[0].m_pCompoundData;
}

static void PumpUntilEnv(CWebViewEnv* p, INT nMs)
{
    const DWORD dwEnd = GetTickCount() + (DWORD)nMs;
    while (!p->m_blDone && GetTickCount() < dwEnd)
    {
        MSG msg;
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        else Sleep(1);
    }
}

void WvEnvConstruct(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRet);
    UNREFERENCED_PARAMETER(nArgCount);
    if (pArgInf == NULL || pArgInf[0].m_pCompoundData == NULL) return;
    *(PINT)pArgInf[0].m_pCompoundData = (INT)(INT_PTR)new CWebViewEnv;
}

void WvEnvCopy(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRet);
    UNREFERENCED_PARAMETER(nArgCount);
    if (pArgInf == NULL || pArgInf[0].m_pCompoundData == NULL) return;
    CWebViewEnv* pDst = new CWebViewEnv;
    CWebViewEnv* pSrc = EnvObj(&pArgInf[1]);
    if (pSrc != NULL && pSrc->m_pEnv != NULL)
    {
        pDst->m_pEnv = pSrc->m_pEnv;
        pDst->m_pEnv->AddRef();
        lstrcpynW(pDst->m_wzFolder, pSrc->m_wzFolder, (INT)(sizeof(pDst->m_wzFolder) / sizeof(WCHAR)));
    }
    *(PINT)pArgInf[0].m_pCompoundData = (INT)(INT_PTR)pDst;
}

void WvEnvDestruct(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRet);
    UNREFERENCED_PARAMETER(nArgCount);
    if (pArgInf == NULL || pArgInf[0].m_pCompoundData == NULL) return;
    CWebViewEnv* p = EnvObj(pArgInf);
    if (p != NULL) delete p;
    *(PINT)pArgInf[0].m_pCompoundData = 0;
}

void WvEnvCreate(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewEnv* p = EnvObj(pArgInf);
    SetRetBool(pRet, FALSE);
    if (p == NULL) return;

    CHAR szBrowser[WV_TEXT_MAX], szFolder[WV_TEXT_MAX];
    szBrowser[0] = szFolder[0] = '\0';
    if (ArgGiven(pArgInf, nArgCount, 1)) ArgReadAt(pArgInf, 1, NULL, NULL, szBrowser, (INT)sizeof(szBrowser));
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, NULL, NULL, szFolder, (INT)sizeof(szFolder));
    INT nMs = 15000;
    if (ArgGiven(pArgInf, nArgCount, 3)) ArgReadAt(pArgInf, 3, &nMs, NULL, NULL, 0);

    if (!ProbeRuntime() || g_pfnCreateEnv == NULL) return;

    WCHAR wzBrowser[WV_TEXT_MAX], wzFolder[WV_TEXT_MAX];
    wzBrowser[0] = wzFolder[0] = L'\0';
    if (szBrowser[0] != '\0') Utf8ToWide(szBrowser, wzBrowser, (INT)(sizeof(wzBrowser) / sizeof(WCHAR)));
    if (szFolder[0] != '\0')
    {
        Utf8ToWide(szFolder, wzFolder, (INT)(sizeof(wzFolder) / sizeof(WCHAR)));
    }
    else
    {
        CHAR szExe[MAX_PATH * 2];
        GetExeDirectory(szExe, (INT)sizeof(szExe));
        Utf8ToWide(szExe, wzFolder, (INT)(sizeof(wzFolder) / sizeof(WCHAR)));
        INT n = (INT)lstrlenW(wzFolder);
        if (n > 0 && wzFolder[n - 1] != L'\\') { wzFolder[n] = L'\\'; wzFolder[n + 1] = L'\0'; }
        WCHAR wzDef[24];
        Utf8ToWide(WV_DEFAULT_CACHE_DIR, wzDef, (INT)(sizeof(wzDef) / sizeof(WCHAR)));
        lstrcatW(wzFolder, wzDef);
    }

    p->m_blDone = FALSE;
    p->m_hr = S_OK;
    if (p->m_pResult != NULL) { p->m_pResult->Release(); p->m_pResult = NULL; }

    WvEnvCreateHandler h(p);
    if (FAILED(g_pfnCreateEnv(wzBrowser[0] ? wzBrowser : NULL, wzFolder, NULL, &h))) return;
    PumpUntilEnv(p, nMs);
    if (!p->m_blDone || FAILED(p->m_hr) || p->m_pResult == NULL) return;

    if (p->m_pEnv != NULL) { p->m_pEnv->Release(); p->m_pEnv = NULL; }
    p->m_pEnv = p->m_pResult;
    p->m_pEnv->AddRef();
    lstrcpynW(p->m_wzFolder, wzFolder, (INT)(sizeof(p->m_wzFolder) / sizeof(WCHAR)));
    SetRetBool(pRet, TRUE);
}

void WvEnvIsValid(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewEnv* p = EnvObj(pArgInf);
    SetRetBool(pRet, (p != NULL && p->m_pEnv != NULL) ? TRUE : FALSE);
}

void WvEnvVersion(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    SetRetText(pRet, "");
    CWebViewEnv* p = EnvObj(pArgInf);
    if (p == NULL || p->m_pEnv == NULL) return;
    LPWSTR pwsz = NULL;
    if (p->m_pEnv->get_BrowserVersionString(&pwsz) == S_OK && pwsz != NULL)
    {
        CHAR szVer[128];
        szVer[0] = '\0';
        WideToUtf8(pwsz, szVer, (INT)sizeof(szVer));
        CoTaskMemFree(pwsz);
        SetRetText(pRet, szVer);
    }
}

void WvEnvCacheFolder(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    SetRetText(pRet, "");
    CWebViewEnv* p = EnvObj(pArgInf);
    if (p == NULL || p->m_wzFolder[0] == L'\0') return;
    CHAR szPath[WV_TEXT_MAX];
    szPath[0] = '\0';
    WideToUtf8(p->m_wzFolder, szPath, (INT)sizeof(szPath));
    SetRetText(pRet, szPath);
}

void WvEnvProcInfos(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    SetRetText(pRet, "");
    CWebViewEnv* p = EnvObj(pArgInf);
    if (p == NULL || p->m_pEnv == NULL) return;
    ICoreWebView2Environment8* p8 = NULL;
    CHAR szJson[WV_EVT_JSON_MAX];
    szJson[0] = '\0';
    if (SUCCEEDED(p->m_pEnv->QueryInterface(__uuidof(ICoreWebView2Environment8), (void**)&p8)) && p8)
    {
        ICoreWebView2ProcessInfoCollection* pList = NULL;
        if (SUCCEEDED(p8->GetProcessInfos(&pList)) && pList)
        {
            WvDescribeObject("ICoreWebView2ProcessInfoCollection", (void*)pList, szJson, (INT)sizeof(szJson));
            pList->Release();
        }
        p8->Release();
    }
    SetRetText(pRet, szJson);
}

// 环境对象同样能按名读写属性：组名固定「环境」，成员名照内核写法。
static void EnvBridge(PMDATA_INF pRet, CWebViewEnv* p, INT mode, LPCSTR member,
                      INT nInt, DOUBLE dbl, LPCSTR szIn, INT nOut)
{
    SetRetBool(pRet, FALSE);
    if (p == NULL || p->m_pEnv == NULL || member == NULL) return;
    INT nOutInt = 0; DOUBLE dOut = 0;
    CHAR szOut[WV_TEXT_MAX];
    szOut[0] = '\0';
    IUnknown* pUnk = (IUnknown*)p->m_pEnv;
    const int rc = ReflDispatch(RF_T_ENV, pUnk, mode, member, nInt, dbl, szIn, &nOutInt, &dOut, szOut, (INT)sizeof(szOut));
    if (rc != RF_OK && rc != RF_OK_TEXT) return;
    if (mode != 0) { SetRetBool(pRet, TRUE); return; }
    switch (nOut)
    {
    case 1: SetRetText(pRet, szOut); break;
    case 2: SetRetInt(pRet, nOutInt); break;
    case 3: pRet->m_dtDataType = SDT_DOUBLE; pRet->m_double = dOut; break;
    default: SetRetBool(pRet, nOutInt ? TRUE : FALSE); break;
    }
}

void WvEnvGet(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewEnv* p = EnvObj(pArgInf);
    CHAR szName[WV_TEXT_MAX];
    szName[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szName, (INT)sizeof(szName));
    LPWSTR pwsz = NULL;
    if (p != NULL && p->m_pEnv != NULL && lstrcmpiA(szName, "BrowserVersionString") == 0
        && p->m_pEnv->get_BrowserVersionString(&pwsz) == S_OK && pwsz != NULL)
    {
        CHAR szVer[128];
        szVer[0] = '\0';
        WideToUtf8(pwsz, szVer, (INT)sizeof(szVer));
        CoTaskMemFree(pwsz);
        SetRetText(pRet, szVer);
        return;
    }
    EnvBridge(pRet, p, 0, szName, 0, 0, NULL, 1);
}

void WvEnvCall(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    CWebViewEnv* p = EnvObj(pArgInf);
    CHAR szName[WV_TEXT_MAX], szArg[WV_TEXT_MAX];
    szName[0] = szArg[0] = '\0';
    ArgReadAt(pArgInf, 1, NULL, NULL, szName, (INT)sizeof(szName));
    if (ArgGiven(pArgInf, nArgCount, 2)) ArgReadAt(pArgInf, 2, NULL, NULL, szArg, (INT)sizeof(szArg));
    EnvBridge(pRet, p, 2, szName, 0, 0, szArg, 0);
}

void WvEnvNames(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    SetRetText(pRet, "");
    CWebViewEnv* p = EnvObj(pArgInf);
    if (p == NULL) return;
    static CHAR szBuf[8 * 1024];
    ReflNames_ENV(szBuf, (INT)sizeof(szBuf));
    SetRetText(pRet, szBuf);
}

// 组件绑定到进程级环境：必须在页面第一次打开前调用。
void WvUseEnv(PMDATA_INF pRet, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CWebViewHost* pH = Self(pArgInf);
    SetRetBool(pRet, FALSE);
    if (pH == NULL || pArgInf == NULL || pH->IsWebviewStarted()) return;
    CWebViewEnv* pEnv = EnvObj(&pArgInf[1]);
    if (pEnv == NULL || pEnv->m_pEnv == NULL) return;

    if (pH->m_pBoundEnv != NULL) pH->m_pBoundEnv->Release();
    pH->m_pBoundEnv = pEnv->m_pEnv;
    pH->m_pBoundEnv->AddRef();
    lstrcpynW(pH->m_pBoundEnvFolder, pEnv->m_wzFolder, (INT)(sizeof(pH->m_pBoundEnvFolder) / sizeof(WCHAR)));
    SetRetBool(pRet, TRUE);
}

// ============================================================ ⑤ 声明表
// 静态编译只需要命令实现，表与库信息不进 .lib。
#ifndef __E_STATIC_LIB

#define WV_EVENT(name, explain, state, argc, args) \
    { _T(name), _T(explain), (state), (argc), (args) }

EVENT_ARG_INFO g_eventArgLoad[] = {
    { _T("载入成功"), _T("真表示页面载入成功，假表示被中止或出错。"), EAS_IS_BOOL_ARG },
};
EVENT_ARG_INFO g_eventArgObjId[] = {
    { _T("对象号"), _T("本事件参数对象的对象号；用「取事件属性文本 / 整数 / 逻辑 / 小数」「取事件子对象」「置事件属性」按名读写，"
                     "用「释放事件对象」回收。0 表示本事件没有参数对象。"), 0 },
};
EVENT_ARG_INFO g_eventArgError[] = {
    { _T("错误码"), _T("Edge 内核给出的网页错误码，0 表示浏览器进程异常退出。"), 0 },
};

EVENT_INFO g_events[] = {
    WV_EVENT("页面开始载入", "网页开始跳转到新地址时产生，可通过“取页面地址”读出即将载入的地址。",
             _EVENT_OS(__OS_WIN), 0, NULL),
    WV_EVENT("页面载入完毕", "页面载入结束时产生。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgLoad),
    WV_EVENT("标题被改变", "页面标题变化时产生，可用“取页面标题”读取。",
             _EVENT_OS(__OS_WIN), 0, NULL),
    WV_EVENT("地址被改变", "当前地址变化时产生，可用“取页面地址”读取。",
             _EVENT_OS(__OS_WIN), 0, NULL),
    WV_EVENT("新窗口请求", "页面用 window.open 或 target=_blank 请求打开新窗口时产生，可用“取页面地址”读到目标地址。",
             _EVENT_OS(__OS_WIN), 0, NULL),
    WV_EVENT("页面载入出错", "网页载入失败时产生，参数为内核错误码。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgError),
    WV_EVENT("内核事件", "任何按名订阅的 WebView2 事件都从这里出来；用「取事件名称」认事件、用「取事件数据」读参数 JSON。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgObjId),
    WV_EVENT("网页消息", "页面调 window.chrome.webview.postMessage 时产生，正文在「取事件数据」里。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgObjId),
    WV_EVENT("下载开始", "页面发起下载时产生，可用「应答下载」决定取消或改落地路径。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgObjId),
    WV_EVENT("权限请求", "页面申请定位 / 摄像头 / 麦克风等权限时产生，用「应答权限」回话。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgObjId),
    WV_EVENT("页面对话框请求", "页面弹 alert / confirm / prompt 时产生；需先自行订阅 ScriptDialogOpening，用「应答对话框」替页面回话。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgObjId),
    WV_EVENT("身份验证请求", "站点要 HTTP 基本认证账号时产生，用「应答身份验证」代填或拒绝。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgObjId),
    WV_EVENT("客户端证书请求", "站点要客户端证书时产生，用「应答客户端证书」挑一张交出去。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgObjId),
    WV_EVENT("服务器证书错误", "HTTPS 证书校验失败时产生，用「应答证书错误」决定忽略还是阻断。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgObjId),
    WV_EVENT("右键菜单请求", "页面右键菜单即将弹出时产生，用「应答右键菜单」接管。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgObjId),
    WV_EVENT("文档结构就绪", "页面 DOM 构建完成（DOMContentLoaded）时产生，早于「页面载入完毕」。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgObjId),
    WV_EVENT("请求关闭窗口", "页面调 window.close 时产生，由程序决定是否关窗口。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgObjId),
    WV_EVENT("键盘按键", "内核处理按键前产生，可用「应答按键」阻止默认行为。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgObjId),
    WV_EVENT("外部协议请求", "页面要打开 mailto / 自定义协议链接时产生，用「应答外部协议」放行或拦下。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgObjId),
    WV_EVENT("另存为界面", "页面触发另存为时产生，用「应答另存为」指定路径或抑制默认对话框。",
             _EVENT_OS(__OS_WIN), 1, g_eventArgObjId),
};

#define WV_PROPERTY(name, eg, explain, type, state, pick) \
    { _T(name), _T(eg), _T(explain), (type), (state), (pick) }

UNIT_PROPERTY g_properties[] = {
    FIXED_WIN_UNIT_PROPERTY,                        // 前 8 项固定属性，不可增删或调序

    WV_PROPERTY("网址", "url",
        "组件启动后要打开的页面地址，支持 http / https / file / data。",
        UD_TEXT, _PROP_OS(__OS_WIN), NULL),
    WV_PROPERTY("缓存目录", "CacheFolder",
        "Edge 的用户数据目录（cookie、缓存、本地存储都放这里）。留空为程序运行目录下的 "
        WV_DEFAULT_CACHE_DIR_W _T("；相对路径按程序运行目录解释；页面第一次打开前设置。"),
        UD_TEXT, _PROP_OS(__OS_WIN), NULL),
    WV_PROPERTY("允许打开新窗口", "AllowNewWindow",
        "假（默认）时页面请求的新窗口会被阻止；真时在当前浏览框内打开。",
        UD_BOOL, _PROP_OS(__OS_WIN), NULL),
    WV_PROPERTY("允许开发者工具", "AllowDevTools",
        "是否允许 F12 / 右键检查打开开发者工具。",
        UD_BOOL, _PROP_OS(__OS_WIN), NULL),
    WV_PROPERTY("缩放比例", "Zoom",
        "页面显示缩放比例。",
        UD_PICK_INT, _PROP_OS(__OS_WIN),
        // 注意：'\0' 后面紧跟数字会被 C 当八进制转义吃掉，必须拆成相邻字符串字面量。
        _T("50%\0" "75%\0" "100%\0" "125%\0" "150%\0" "200%\0" "\0")),
    WV_PROPERTY("载入提示", "ShowLoading",
        "真（默认）时初始化和跳转期间自动用载入状态遮住页面，显示滚动进度与居中提示。",
        UD_BOOL, _PROP_OS(__OS_WIN), NULL),
};

// 组件数据类型：LDT_WIN_UNIT 表示可直接拖进窗口的可视化组件。
// 类对象只存一个内部指针成员，必须给出元素表，否则系统不知道对象多大。
LIB_DATA_TYPE_ELEMENT g_wvEnvElement[] = {
    { SDT_INT, NULL, _T("内部数据"), NULL, _T("内核环境的内部句柄，请勿读写。"), LES_HIDED, 0 },
};

LIB_DATA_TYPE_INFO g_dataTypes[] = {
    {
        _T("网页浏览框"), _T("WebViewBox"),
        _T("基于 Edge WebView2 的网页显示组件，拖进窗口后设置“网址”即可显示页面。"),
        0, NULL,                                    // 成员命令数与下标表在下面回填
        LDT_WIN_UNIT,                               // 与官方 HtmlView 范例一致，不加 _DT_OS 高位
        WV_UNIT_BMP_ID,                             // 工具箱图标（edgewv.rc 里的 1000）
        sizeof(g_events) / sizeof(g_events[0]),
        (PEVENT_INFO2)g_events,
        sizeof(g_properties) / sizeof(g_properties[0]),
        g_properties,
        pvi_GetInterface_WebView,
        0, NULL,
    },
    {
        _T("网页环境"), _T("WebViewEnvironment"),
        _T("进程级的 Edge 内核环境：统一用户数据目录、取内核版本与进程信息，多个浏览框可共用同一个环境。"),
        0, NULL,                                    // 成员命令数与下标表在下面回填
        0, 0, 0, NULL, 0, NULL,
        NULL,
        sizeof(g_wvEnvElement) / sizeof(g_wvEnvElement[0]), g_wvEnvElement,
    },
};

#define WV_CMD(cn, eg, explain, cat, ret, argc, args) \
    { _T(cn), _T(eg), _T(explain), (cat), _CMD_OS(__OS_WIN), (ret), 0, LVL_SIMPLE, 0, 0, (argc), (args) }
#define WV_OBJ(cn, eg, explain, state, ret, argc, args) \
    { _T(cn), _T(eg), _T(explain), -1, (state), (ret), 0, LVL_SIMPLE, 0, 0, (argc), (args) }

#define WV_ARG(name, explain, type, defaultVal, state) \
    { _T(name), _T(explain), 0, 0, (type), (INT)(DWORD_PTR)(defaultVal), (state) }

ARG_INFO g_argEvtSub[] = {
    WV_ARG("事件名", "要订阅的 WebView2 事件名，不区分大小写，如 DownloadStarting。", SDT_TEXT, "", 0),
    WV_ARG("组名", "事件挂在哪个对象上：内核 / 控制器 / 环境 / 配置文件 / 框架 / 下载 / 协议事件接收器。留空按内核找。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argEvtName[] = {
    WV_ARG("事件名", "要退订的 WebView2 事件名。", SDT_TEXT, "", 0),
};
ARG_INFO g_argOptGroup[] = {
    WV_ARG("组名", "留空则一次列出所有组的可订阅事件。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argState[] = {
    WV_ARG("状态", "0 默认（交回内核弹授权条）1 允许 2 拒绝。", SDT_INT, 0, 0),
};
ARG_INFO g_argDialogAns[] = {
    WV_ARG("接受", "真按确定应答，假按取消关闭。", SDT_BOOL, 0, 0),
    WV_ARG("输入文本", "prompt 对话框的返回值，留空用页面默认值。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argDownloadAns[] = {
    WV_ARG("取消", "真时中止这次下载。", SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("另存路径", "改写落地文件全路径，留空用内核建议值。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("已处理", "真表示由本程序接管下载界面；留空按真处理。", SDT_BOOL, 1, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argAuthAns[] = {
    WV_ARG("用户名", "留空表示不代填，交回内核弹默认认证框。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("密码", "同上。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("取消", "真时直接拒绝该次认证。", SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argCertAns[] = {
    WV_ARG("证书序号", "要提交的客户端证书在列表里的下标，留空表示不选证书。", SDT_INT, -1, AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("取消", "真时取消本次请求。", SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argAction[] = {
    WV_ARG("动作", "0 默认 1 忽略错误继续 2 阻断连接。", SDT_INT, 0, 0),
};
ARG_INFO g_argMenuAns[] = {
    WV_ARG("命令序号", "用户点了自定义菜单里的哪一项（-1 表示没点）。", SDT_INT, -1, AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("已处理", "真表示本程序已接管菜单，内核不再弹默认菜单。", SDT_BOOL, 1, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argKeyBlock[] = {
    WV_ARG("阻止默认", "真时内核不再处理这个按键。", SDT_BOOL, 0, 0),
};
ARG_INFO g_argExtCancel[] = {
    WV_ARG("取消", "真时阻止系统打开该外部协议链接。", SDT_BOOL, 1, 0),
};
ARG_INFO g_argSaveAsAns[] = {
    WV_ARG("另存路径", "指定落地文件全路径，留空由用户选。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("取消", "真时中止另存。", SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("允许替换", "目标已存在时是否允许覆盖。", SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("抑制默认框", "真时不再弹内核的另存对话框。", SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argHandled[] = {
    WV_ARG("已处理", "真表示本程序接管了这个新窗口请求。", SDT_BOOL, 1, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argTimeout[] = {
    WV_ARG("超时毫秒", "等待内核回话的上限，留空 5000。", SDT_INT, 5000, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argBool1[] = {
    WV_ARG("值", "真 / 假。", SDT_BOOL, 0, 0),
};
ARG_INFO g_argMuted[] = {
    WV_ARG("静音", "真时页面不再出声。", SDT_BOOL, 0, 0),
};
ARG_INFO g_argText1[] = {
    WV_ARG("文本", "要交给页面的 UTF-8 文本。", SDT_TEXT, "", 0),
};
ARG_INFO g_argJson1[] = {
    WV_ARG("JSON文本", "UTF-8 的 JSON 串。", SDT_TEXT, "", 0),
};
ARG_INFO g_argStatusText[] = {
    WV_ARG("文本", "要显示在状态栏的 UTF-8 文本，留空清空。", SDT_TEXT, "", 0),
};
ARG_INFO g_argZoomDbl[] = {
    WV_ARG("缩放比例", "1.0 为原始大小，0.2 到 10。", SDT_DOUBLE, 1, 0),
};
ARG_INFO g_argMemLevel[] = {
    WV_ARG("级别", "0 默认 1 低内存占用。", SDT_INT, 0, 0),
};
ARG_INFO g_argShot[] = {
    WV_ARG("图片格式", "0 为 PNG（默认），1 为 JPEG。", SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("超时毫秒", "留空 8000。", SDT_INT, 8000, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argPdf[] = {
    WV_ARG("保存路径", "PDF 落地全路径。", SDT_TEXT, "", 0),
    WV_ARG("超时毫秒", "留空 15000。", SDT_INT, 15000, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argPrintKind[] = {
    WV_ARG("界面种类", "0 打印预览 1 打印设置 2 打印机选择。", SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argInitScript[] = {
    WV_ARG("脚本", "页面文档一创建就执行的 JavaScript 源码。", SDT_TEXT, "", 0),
    WV_ARG("超时毫秒", "留空 5000。", SDT_INT, 5000, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argScriptId[] = {
    WV_ARG("注册标识", "注入页面脚本 返回的标识串。", SDT_TEXT, "", 0),
};
ARG_INFO g_argDevCall[] = {
    WV_ARG("方法名", "CDP 方法名，如 Page.captureScreenshot。", SDT_TEXT, "", 0),
    WV_ARG("参数JSON", "该方法的参数，UTF-8 JSON 串；留空按空对象处理。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("超时毫秒", "留空 8000。", SDT_INT, 8000, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argDevEvt[] = {
    WV_ARG("事件名", "CDP 事件名，如 Network.requestWillBeSent。", SDT_TEXT, "", 0),
    WV_ARG("参数JSON", "事件订阅参数，UTF-8 JSON 串（例如给 Network 事件设节流周期）；留空按空对象处理。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argHostMap[] = {
    WV_ARG("虚拟域名", "页面里使用的假域名，如 myapp.local。", SDT_TEXT, "", 0),
    WV_ARG("本地目录", "映射到的本地目录全路径。", SDT_TEXT, "", 0),
    WV_ARG("访问级别", "0 仅同源 1 可读父目录 2 可执行。", SDT_INT, 1, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argHostName[] = {
    WV_ARG("虚拟域名", "之前登记的假域名。", SDT_TEXT, "", 0),
};
ARG_INFO g_argReqFilter[] = {
    WV_ARG("通配URI", "如 https://* 或 https://api.example.com/*。", SDT_TEXT, "", 0),
    WV_ARG("资源类型", "0 全部资源，其它值按内核的 WebResourceContext 枚举。", SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argReqAns[] = {
    WV_ARG("状态码", "要回给页面的 HTTP 状态码。", SDT_INT, 200, 0),
    WV_ARG("状态文本", "留空用 OK。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("响应头", "每行一条「名称: 值」。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("正文", "UTF-8 文本，留空回空正文。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argCookieGet[] = {
    WV_ARG("网址", "取该网址可见的 cookie，留空按当前页地址。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("超时毫秒", "留空 5000。", SDT_INT, 5000, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argCookieSet[] = {
    WV_ARG("名称", "cookie 名。", SDT_TEXT, "", 0),
    WV_ARG("值", "cookie 值（UTF-8）。", SDT_TEXT, "", 0),
    WV_ARG("域名", "留空按当前页地址推断。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("路径", "留空为 /。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("有效天数", "正数表示 N 天后过期，留空或 0 为会话级。", SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("安全", "真加 Secure 标记。", SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("仅Http", "真加 HttpOnly 标记。", SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("同站策略", "0 严格 1 宽松 2 不限制。", SDT_INT, 2, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argCookieDelAll[] = {
    WV_ARG("网址", "cookie 所属网址或域名。", SDT_TEXT, "", 0),
    WV_ARG("名称", "要删的 cookie 名。", SDT_TEXT, "", 0),
    WV_ARG("路径", "只删这个路径下的同名 cookie。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argCookieDel[] = {
    WV_ARG("网址", "cookie 所属网址。", SDT_TEXT, "", 0),
    WV_ARG("名称", "要删的 cookie 名。", SDT_TEXT, "", 0),
};
ARG_INFO g_argClearIn[] = {
    WV_ARG("数据种类掩码", "内核 BrowsingDataKinds 位或：2=下载记录 8=Cookie 16=缓存文件 32=历史记录 64=密码 4096=站点数据。", SDT_INT, 0, 0),
    WV_ARG("超时毫秒", "留空 10000。", SDT_INT, 10000, AS_DEFAULT_VALUE_IS_EMPTY),
};


ARG_INFO g_argExtAdd[] = {
    WV_ARG("扩展目录", "解包后的扩展所在目录全路径。", SDT_TEXT, "", 0),
    WV_ARG("允许运行", "真表示装上即允许运行。", SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("超时毫秒", "留空 10000。", SDT_INT, 10000, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argExtEnable[] = {
    WV_ARG("启用", "真启用、假停用。", SDT_BOOL, 1, 0),
    WV_ARG("超时毫秒", "留空 10000。", SDT_INT, 10000, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argVersions[] = {
    WV_ARG("版本A", "如 139.0.3405.86。", SDT_TEXT, "", 0),
    WV_ARG("版本B", "比较对象。", SDT_TEXT, "", 0),
};
ARG_INFO g_argSaveAs[] = {
    WV_ARG("超时毫秒", "等用户选完的最长时间，留空 60000。", SDT_INT, 60000, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argUrlOpt[] = {
    WV_ARG("网址", "按该网址作用域删除，如 https://example.com。", SDT_TEXT, "", 0),
};


ARG_INFO g_argUrl[] = {
    WV_ARG("网址", "要打开的页面地址，支持 http / https / file / data；留空打开空白页。",
           SDT_TEXT, "", 0),
};
ARG_INFO g_argHtml[] = {
    WV_ARG("网页文本", "要显示的 HTML 文本（UTF-8）。", SDT_TEXT, "", 0),
};
ARG_INFO g_argSkipCache[] = {
    WV_ARG("跳过缓存", "真时忽略本地缓存重新取页。", SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argScript[] = {
    WV_ARG("脚本", "要执行的 JavaScript 源码。", SDT_TEXT, "", 0),
};
ARG_INFO g_argEval[] = {
    WV_ARG("脚本", "要执行并取返回值的 JavaScript 源码，返回值需可转成文本。", SDT_TEXT, "", 0),
    WV_ARG("超时毫秒", "等待返回值的时间上限，留空 5000。", SDT_INT, 5000, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argZoom[] = {
    WV_ARG("百分比", "缩放百分比，25 到 500。", SDT_INT, 100, 0),
};
ARG_INFO g_argReflGet[] = {
    WV_ARG("组名", "内核 / 设置 / 控制器 / 环境 / 环境选项 / Cookie管理器 / 配置文件。", SDT_TEXT, "", 0),
    WV_ARG("成员名", "WebView2 属性名，不区分大小写，如 UserAgent、IsScriptEnabled。", SDT_TEXT, "", 0),
};
ARG_INFO g_argReflSet[] = {
    WV_ARG("组名", "同上。", SDT_TEXT, "", 0),
    WV_ARG("成员名", "要写的 WebView2 属性名。", SDT_TEXT, "", 0),
    WV_ARG("值", "整数 / 小数 / 逻辑 / 文本均可，按属性原形传。", _SDT_ALL, "", AS_RECEIVE_VAR_OR_OTHER),
};
ARG_INFO g_argReflCall[] = {
    WV_ARG("组名", "同上。", SDT_TEXT, "", 0),
    WV_ARG("成员名", "要调用的 WebView2 方法名（无参或单个简单参数的方法）。", SDT_TEXT, "", 0),
    WV_ARG("参数", "方法只有一个简单参数时传，留空表示无参调用。", _SDT_ALL, "", AS_DEFAULT_VALUE_IS_EMPTY | AS_RECEIVE_VAR_OR_OTHER),
};
ARG_INFO g_argReflNames[] = {
    WV_ARG("组名", "同上。", SDT_TEXT, "", 0),
};

ARG_INFO g_argCopySelf[] = {
    WV_ARG("源组件", "另一个网页浏览框组件。", WV_DT_PAGEVIEW, 0, AS_RECEIVE_VAR),
};

ARG_INFO g_argEnvCopySelf[] = {
    WV_ARG("源环境", "另一个网页环境对象。", WV_DT_ENV, 0, AS_RECEIVE_VAR),
};
ARG_INFO g_argEnvCreate[] = {
    WV_ARG("内核目录", "自带 Edge 内核（固定版本）的目录，留空用系统安装的 WebView2 Runtime。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("数据目录", "用户数据目录（cookie / 缓存），留空为程序运行目录下的 edge_cache。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
    WV_ARG("超时毫秒", "等待内核起动的上限，留空 15000。", SDT_INT, 15000, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argEnvName[] = {
    WV_ARG("成员名", "WebView2 环境属性名，不区分大小写。", SDT_TEXT, "", 0),
};
ARG_INFO g_argEnvCall[] = {
    WV_ARG("成员名", "环境上的方法名。", SDT_TEXT, "", 0),
    WV_ARG("参数", "方法只有一个文本参数时传，留空表示无参调用。", SDT_TEXT, "", AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argUseEnv[] = {
    WV_ARG("网页环境", "已经「创建网页环境」成功的环境对象。", WV_DT_ENV, 0, 0),
};

ARG_INFO g_argObjId[] = {
    WV_ARG("对象号", "要操作的事件参数对象号，留空用最近一次事件的对象。", SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argFreeId[] = {
    WV_ARG("对象号", "要释放的事件参数对象号。", SDT_INT, 0, 0),
};
ARG_INFO g_argProp[] = {
    WV_ARG("属性名", "内核参数对象上的属性名，不区分大小写；用 取事件属性列表 可列出。", SDT_TEXT, "", 0),
    WV_ARG("对象号", "留空用最近一次事件的对象。", SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argPropIdx[] = {
    WV_ARG("属性名", "列表属性名，内核参数对象上一般叫 Items。", SDT_TEXT, "", 0),
    WV_ARG("序号", "列表里的第几项，从 0 开始。", SDT_INT, 0, 0),
    WV_ARG("对象号", "留空用最近一次事件的对象。", SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY),
};
ARG_INFO g_argSetProp[] = {
    WV_ARG("属性名", "要写的属性名，如 Cancel / Handled / State / ResultFilePath。", SDT_TEXT, "", 0),
    WV_ARG("值", "整数 / 逻辑 / 文本均可，按属性原形传。", _SDT_ALL, "", AS_RECEIVE_VAR_OR_OTHER),
    WV_ARG("对象号", "留空用最近一次事件的对象。", SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY),
};

CMD_INFO g_commands[] = {
    // 0-2 全局命令
    WV_CMD("Edge网页浏览_测试命令", "EdgewvTest", "返回库与内核装载情况，用于确认支持库已正确装载。", 1, SDT_TEXT, 0, NULL),
    WV_CMD("检测网页内核", "DetectWebViewRuntime", "系统是否具备 Edge WebView2 运行环境。", 1, SDT_BOOL, 0, NULL),
    WV_CMD("取网页内核版本", "GetWebViewRuntimeVersion", "返回 Edge WebView2 Runtime 版本号，未安装返回空文本。", 1, SDT_TEXT, 0, NULL),

    // 3-21 网页浏览框成员命令
    WV_OBJ("网页浏览框_构造", "WebViewBoxC", "由系统在窗口载入时调用，用户不需要手工调用。",
           CT_IS_HIDED | CT_IS_OBJ_CONSTURCT_CMD, _SDT_NULL, 0, NULL),
    WV_OBJ("网页浏览框_析构", "WebViewBoxD", "由系统在组件销毁时调用，用户不需要手工调用。",
           CT_IS_HIDED | CT_IS_OBJ_FREE_CMD, _SDT_NULL, 0, NULL),
    WV_OBJ("打开网址", "Navigate", "让浏览框跳转到指定网址。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_argUrl),
    WV_OBJ("载入网页文本", "LoadHtml", "直接显示一段 HTML 文本。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_argHtml),
    WV_OBJ("刷新页面", "Refresh", "重新载入当前页面。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_argSkipCache),
    WV_OBJ("后退", "GoBack", "回到上一页。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("前进", "GoForward", "前进到下一页。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("停止载入", "Stop", "中止当前页面的载入。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("取页面地址", "GetPageUrl", "取当前页面地址。",
           _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    WV_OBJ("取页面标题", "GetPageTitle", "取当前页面标题。",
           _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    WV_OBJ("取页面源码", "GetPageSource", "取当前页面的 HTML 源码（同步等待，可能耗时）。",
           _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    WV_OBJ("是否载入中", "IsLoading", "页面是否正在载入。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("能否后退", "CanGoBack", "历史上是否有上一页。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("能否前进", "CanGoForward", "历史上是否有下一页。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("执行脚本", "ExecuteScript", "在页面里执行 JavaScript，不取返回值。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_argScript),
    WV_OBJ("计算脚本", "EvaluateScript", "在页面里执行 JavaScript 并等待返回文本结果。",
           _CMD_OS(__OS_WIN), SDT_TEXT, 2, g_argEval),
    WV_OBJ("取缓存目录", "GetCacheFolder", "取实际生效的缓存目录绝对路径。",
           _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    WV_OBJ("设置缩放", "SetZoom", "按百分比设置页面缩放。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_argZoom),
    WV_OBJ("打开开发者工具", "OpenDevTools", "弹出该页面的 DevTools 窗口。",
           _CMD_OS(__OS_WIN), _SDT_NULL, 0, NULL),
    WV_OBJ("清除缓存", "ClearCache", "清空 HTTP 缓存（不动 cookie 与本地存储）。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("取窗口句柄", "GetHandle", "取浏览框宿主窗口的 HWND。",
           _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    WV_OBJ("是否有效", "IsValid", "组件对象是否仍指向窗口里存在的浏览框。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("销毁组件", "Destroy", "立即销毁该浏览框及其窗口。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("取内核属性", "GetKernelProperty", "按名字读 WebView2 属性，覆盖内核全部 262 个属性；名字可用中文组名限定，取不到返回空白数据。",
           _CMD_OS(__OS_WIN), _SDT_ALL, 2, g_argReflGet),
    WV_OBJ("置内核属性", "SetKernelProperty", "按名字写 WebView2 属性；写成功返回真。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_argReflSet),
    WV_OBJ("调用内核方法", "CallKernelMethod", "按名字调用 WebView2 的无参或单参方法，如 NavigateToString、Stop。",
           _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_argReflCall),
    WV_OBJ("取内核成员列表", "GetKernelMemberList", "列出某个组里可以按名访问的全部成员，每行一个。",
           _CMD_OS(__OS_WIN), SDT_TEXT, 1, g_argReflNames),
    WV_OBJ("网页浏览框_复制", "WebViewBoxB", "由系统在组件对象赋值时调用，用户不需要手工调用。",
           CT_IS_HIDED | CT_IS_OBJ_COPY_CMD, _SDT_NULL, 1, g_argCopySelf),

    // ---- 全量对接层：事件 / 应答 / 异步能力，一律往末尾追加，不动上面的序号 ----
    WV_OBJ("订阅内核事件", "SubscribeKernelEvent", "按名订阅 WebView2 的任意事件；事件产生时触发「内核事件」，用 取事件数据 读参数快照。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argEvtSub) / sizeof(g_argEvtSub[0]), g_argEvtSub),
    WV_OBJ("退订内核事件", "UnsubscribeKernelEvent", "取消一次按名订阅。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argEvtName) / sizeof(g_argEvtName[0]), g_argEvtName),
    WV_OBJ("取可订阅事件列表", "GetKernelEventList", "列出某组（或全部组）可以按名订阅的事件名，每行一个。", _CMD_OS(__OS_WIN), SDT_TEXT, sizeof(g_argOptGroup) / sizeof(g_argOptGroup[0]), g_argOptGroup),
    WV_OBJ("取事件名称", "GetEventName", "最近一次内核事件的英文名。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    WV_OBJ("取事件数据", "GetEventData", "最近一次内核事件的参数快照（UTF-8 JSON 文本）。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    WV_OBJ("取事件对象快照", "GetEventObjectSnapshot", "把最近一次事件带出来的对象（下载 / 帧 / 请求 / 扩展…）整个转成 JSON 文本。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    WV_OBJ("应答权限", "AnswerPermission", "在「权限请求」里回话：0 默认 1 允许 2 拒绝。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argState) / sizeof(g_argState[0]), g_argState),
    WV_OBJ("应答对话框", "AnswerDialog", "在「页面对话框请求」里替页面回 alert / confirm / prompt。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argDialogAns) / sizeof(g_argDialogAns[0]), g_argDialogAns),
    WV_OBJ("应答下载", "AnswerDownload", "在「下载开始」里决定取消、改写落地路径、是否接管下载界面。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argDownloadAns) / sizeof(g_argDownloadAns[0]), g_argDownloadAns),
    WV_OBJ("应答身份验证", "AnswerBasicAuth", "在「身份验证请求」里代填账号密码或直接拒绝。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argAuthAns) / sizeof(g_argAuthAns[0]), g_argAuthAns),
    WV_OBJ("应答客户端证书", "AnswerClientCertificate", "在「客户端证书请求」里挑一张证书交出去。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argCertAns) / sizeof(g_argCertAns[0]), g_argCertAns),
    WV_OBJ("应答证书错误", "AnswerServerCertificate", "在「服务器证书错误」里决定忽略还是阻断。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argAction) / sizeof(g_argAction[0]), g_argAction),
    WV_OBJ("应答右键菜单", "AnswerContextMenu", "在「右键菜单请求」里回话用户选了哪一项。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argMenuAns) / sizeof(g_argMenuAns[0]), g_argMenuAns),
    WV_OBJ("应答按键", "AnswerAcceleratorKey", "在「键盘按键」里阻止内核继续处理这个键。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argKeyBlock) / sizeof(g_argKeyBlock[0]), g_argKeyBlock),
    WV_OBJ("应答外部协议", "AnswerExternalUri", "在「外部协议请求」里决定放行还是拦下。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argExtCancel) / sizeof(g_argExtCancel[0]), g_argExtCancel),
    WV_OBJ("应答另存为", "AnswerSaveAsUI", "在「另存为界面」里指定落地路径、是否覆盖、是否抑制默认对话框。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argSaveAsAns) / sizeof(g_argSaveAsAns[0]), g_argSaveAsAns),
    WV_OBJ("应答新窗口", "AnswerNewWindow", "接管 window.open 这类新窗口请求（配合属性「允许打开新窗口」使用）。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argHandled) / sizeof(g_argHandled[0]), g_argHandled),
    WV_OBJ("取下载详情", "GetDownloadSnapshot", "把「下载开始」事件里的下载对象转成 JSON 文本。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    WV_OBJ("尝试挂起页面", "TrySuspendPage", "让页面进入挂起以省资源，返回内核是否同意挂起。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argTimeout) / sizeof(g_argTimeout[0]), g_argTimeout),
    WV_OBJ("恢复页面", "ResumePage", "解除挂起。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("静音页面", "SetMuted", "整页静音，不改变页面自身的音量。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argMuted) / sizeof(g_argMuted[0]), g_argMuted),
    WV_OBJ("是否播放音频", "IsPlayingAudio", "页面当前是否在出声。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("取浏览器进程ID", "GetBrowserProcessId", "渲染本页面的 Edge 浏览器进程 PID。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    WV_OBJ("取网站图标地址", "GetFaviconUri", "当前页面图标的地址，没有则返回空文本。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    WV_OBJ("取状态栏文本", "GetStatusBarText", "页面想显示在状态栏的文字。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    WV_OBJ("设置状态栏文本", "SetStatusBarText", "改写状态栏文字。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argStatusText) / sizeof(g_argStatusText[0]), g_argStatusText),
    WV_OBJ("取缩放比例", "GetZoomFactor", "当前缩放比例，1.0 为原始大小。", _CMD_OS(__OS_WIN), SDT_DOUBLE, 0, NULL),
    WV_OBJ("设置缩放比例", "SetZoomFactor", "按小数精确设置缩放（0.2 到 10）。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argZoomDbl) / sizeof(g_argZoomDbl[0]), g_argZoomDbl),
    WV_OBJ("是否包含全屏元素", "ContainsFullScreenElement", "页面里是否有元素处于全屏状态。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("设置内存占用级别", "SetMemoryUsageTargetLevel", "告诉内核本窗口是低内存目标，空闲时会回收更多资源。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argMemLevel) / sizeof(g_argMemLevel[0]), g_argMemLevel),
    WV_OBJ("取内存占用级别", "GetMemoryUsageTargetLevel", "当前的内存占用级别。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    WV_OBJ("打开内核任务管理器", "OpenTaskManagerWindow", "弹出 Edge 的任务管理器看进程内存。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("显示下载对话框", "OpenDefaultDownloadDialog", "打开内核自带的下载列表气泡。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("关闭下载对话框", "CloseDefaultDownloadDialog", "收起下载列表气泡。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("发送网页消息", "PostWebMessage", "向页面发一条字符串消息，页面在 message 事件里收到。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argText1) / sizeof(g_argText1[0]), g_argText1),
    WV_OBJ("发送网页消息JSON", "PostWebMessageJson", "向页面发一条 JSON 消息。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argJson1) / sizeof(g_argJson1[0]), g_argJson1),
    WV_OBJ("注入页面脚本", "AddInitScript", "登记一段每次文档创建都会最先执行的脚本，返回注册标识。", _CMD_OS(__OS_WIN), SDT_TEXT, sizeof(g_argInitScript) / sizeof(g_argInitScript[0]), g_argInitScript),
    WV_OBJ("移除注入脚本", "RemoveInitScript", "按注册标识取消注入脚本。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argScriptId) / sizeof(g_argScriptId[0]), g_argScriptId),
    WV_OBJ("调用调试协议", "CallDevToolsProtocol", "直接调 Chrome DevTools Protocol 方法，返回其 JSON 结果。", _CMD_OS(__OS_WIN), SDT_TEXT, sizeof(g_argDevCall) / sizeof(g_argDevCall[0]), g_argDevCall),
    WV_OBJ("订阅调试协议事件", "SubscribeDevToolsEvent", "订阅一个 CDP 事件，之后从「内核事件」里读它的 JSON。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argDevEvt) / sizeof(g_argDevEvt[0]), g_argDevEvt),
    WV_OBJ("映射虚拟主机", "SetVirtualHostNameMapping", "把本地目录挂成一个假域名，页面可用 http://该域名 读本地文件。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argHostMap) / sizeof(g_argHostMap[0]), g_argHostMap),
    WV_OBJ("取消虚拟主机映射", "ClearVirtualHostNameMapping", "撤掉一个虚拟域名。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argHostName) / sizeof(g_argHostName[0]), g_argHostName),
    WV_OBJ("开启请求拦截", "AddWebResourceRequestFilter", "按通配 URI 拦截网络请求，之后在「内核事件」的 WebResourceRequested 里应答。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argReqFilter) / sizeof(g_argReqFilter[0]), g_argReqFilter),
    WV_OBJ("关闭请求拦截", "RemoveWebResourceRequestedFilter", "撤掉一个拦截过滤器。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argReqFilter) / sizeof(g_argReqFilter[0]), g_argReqFilter),
    WV_OBJ("应答网络请求", "AnswerWebResourceRequest", "对被拦截的请求直接给出响应（状态码 / 响应头 / 正文）。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argReqAns) / sizeof(g_argReqAns[0]), g_argReqAns),
    WV_OBJ("取页面截图", "CapturePreview", "把当前页面截成图片，返回 PNG 或 JPEG 的字节集。", _CMD_OS(__OS_WIN), SDT_BIN, sizeof(g_argShot) / sizeof(g_argShot[0]), g_argShot),
    WV_OBJ("打印为PDF", "PrintToPdf", "把当前页面输出成 PDF 文件。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argPdf) / sizeof(g_argPdf[0]), g_argPdf),
    WV_OBJ("显示打印界面", "ShowPrintUI", "弹出内核的打印预览 / 设置 / 打印机选择界面。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argPrintKind) / sizeof(g_argPrintKind[0]), g_argPrintKind),
    WV_OBJ("打印页面", "PrintPage", "静默打印当前页面（走系统默认打印机）。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argTimeout) / sizeof(g_argTimeout[0]), g_argTimeout),
    WV_OBJ("显示另存为界面", "ShowSaveAsUI", "弹内核的另存为对话框，返回结果码。", _CMD_OS(__OS_WIN), SDT_INT, sizeof(g_argSaveAs) / sizeof(g_argSaveAs[0]), g_argSaveAs),
    WV_OBJ("取Cookie列表", "GetCookieList", "取指定网址可见的 cookie，返回 JSON 数组文本。", _CMD_OS(__OS_WIN), SDT_TEXT, sizeof(g_argCookieGet) / sizeof(g_argCookieGet[0]), g_argCookieGet),
    WV_OBJ("写Cookie", "SetCookieValue", "新建或覆盖一条 cookie。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argCookieSet) / sizeof(g_argCookieSet[0]), g_argCookieSet),
    WV_OBJ("删Cookie", "DeleteCookie", "删除指定网址下的同名 cookie。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argCookieDel) / sizeof(g_argCookieDel[0]), g_argCookieDel),
    WV_OBJ("删同名Cookie", "DeleteCookieAll", "按域名 / 路径批量删除同名 cookie。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argCookieDelAll) / sizeof(g_argCookieDelAll[0]), g_argCookieDelAll),
    WV_OBJ("删全部Cookie", "DeleteAllCookies", "清空本缓存目录里的全部 cookie。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    WV_OBJ("取进程信息列表", "GetProcessInfos", "列出内核各进程的类型与 PID（JSON 数组文本）。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    WV_OBJ("清除全部浏览数据", "ClearBrowsingDataAll", "清掉本配置文件的全部浏览数据。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argTimeout) / sizeof(g_argTimeout[0]), g_argTimeout),
    WV_OBJ("清除时间段浏览数据", "ClearBrowsingDataInTime", "按时间段和范围清除浏览数据，范围用内核的 JSON 键。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argClearIn) / sizeof(g_argClearIn[0]), g_argClearIn),
    WV_OBJ("取扩展列表", "GetBrowserExtensions", "列出已安装浏览器扩展（JSON 数组文本）。", _CMD_OS(__OS_WIN), SDT_TEXT, sizeof(g_argTimeout) / sizeof(g_argTimeout[0]), g_argTimeout),
    WV_OBJ("安装扩展", "AddBrowserExtension", "装一个解包好的扩展目录；装完的对象可用 启用当前扩展 / 卸载当前扩展 操作。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argExtAdd) / sizeof(g_argExtAdd[0]), g_argExtAdd),
    WV_OBJ("卸载当前扩展", "RemoveCurrentExtension", "卸载最近一次扩展事件或安装命令带出来的那个扩展。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argTimeout) / sizeof(g_argTimeout[0]), g_argTimeout),
    WV_OBJ("启用当前扩展", "EnableCurrentExtension", "启用 / 停用最近一次带出来的扩展。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argExtEnable) / sizeof(g_argExtEnable[0]), g_argExtEnable),
    WV_OBJ("取内核版本字符串", "GetBrowserVersionString", "当前环境实际使用的内核完整版本号。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    WV_OBJ("取配置文件名", "GetProfileName", "当前 Edge 配置文件名。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    WV_CMD("比较内核版本", "CompareWebViewVersions", "比较两个内核版本号：A 大于 B 回 1，相等回 0，小于回 -1。", 1, SDT_INT, sizeof(g_argVersions) / sizeof(g_argVersions[0]), g_argVersions),

    // ---- WebView2环境 类（成员命令 96..106），107 是组件上的「使用网页环境」 ----
WV_OBJ("网页环境_构造", "WebViewEnvA", "由系统在声明对象时调用，用户不需要手工调用。", CT_IS_HIDED | CT_IS_OBJ_CONSTURCT_CMD, _SDT_NULL, 0, NULL),
WV_OBJ("网页环境_复制", "WebViewEnvB", "由系统在对象赋值时调用，用户不需要手工调用。", CT_IS_HIDED | CT_IS_OBJ_COPY_CMD, _SDT_NULL, 1, g_argEnvCopySelf),
WV_OBJ("网页环境_析构", "WebViewEnvC", "由系统在对象离开作用域时调用，用户不需要手工调用。", CT_IS_HIDED | CT_IS_OBJ_FREE_CMD, _SDT_NULL, 0, NULL),
WV_OBJ("创建网页环境", "CreateEnvironment", "建立进程级的 Edge 内核环境：可指定内核目录与用户数据目录，返回是否建立成功。", _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_argEnvCreate),
WV_OBJ("环境是否有效", "IsEnvironmentValid", "环境对象是否已经建立成功。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
WV_OBJ("取环境内核版本", "GetEnvironmentVersion", "该环境实际使用的内核完整版本号。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
WV_OBJ("取环境缓存目录", "GetEnvironmentCacheFolder", "该环境使用的用户数据目录绝对路径。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
WV_OBJ("取环境进程信息列表", "GetEnvironmentProcessInfos", "该环境下所有内核进程的 JSON 列表。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
WV_OBJ("取环境属性", "GetEnvironmentProperty", "按内核属性名读环境属性，如 BrowserVersionString。", _CMD_OS(__OS_WIN), SDT_TEXT, 1, g_argEnvName),
WV_OBJ("调用环境方法", "CallEnvironmentMethod", "按内核方法名调用环境上无参或单文本参的方法。", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_argEnvCall),
WV_OBJ("取环境成员列表", "GetEnvironmentMembers", "列出环境对象可按名读写的属性名，每行一个。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
WV_OBJ("使用网页环境", "UseWebViewEnvironment", "让本组件跑在指定的网页环境上（共用同一用户数据目录与进程）；必须在页面第一次打开前调用。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_argUseEnv),
    WV_OBJ("取事件对象号", "GetEventArgsId", "最近一次事件的参数对象号；0 表示该事件没有参数对象。", _CMD_OS(__OS_WIN), SDT_INT, NULL, NULL),
    WV_OBJ("取事件对象类型", "GetEventArgsType", "对象对应的内核接口名，如 ICoreWebView2DownloadStartingEventArgs。", _CMD_OS(__OS_WIN), SDT_TEXT, sizeof(g_argObjId) / sizeof(g_argObjId[0]), g_argObjId),
    WV_OBJ("取事件属性列表", "GetEventArgsNames", "列出该事件参数对象上可按名读写的属性名，每行一个。", _CMD_OS(__OS_WIN), SDT_TEXT, sizeof(g_argObjId) / sizeof(g_argObjId[0]), g_argObjId),
    WV_OBJ("取事件属性文本", "GetEventArgsText", "按属性名读文本型参数（UTF-8）。", _CMD_OS(__OS_WIN), SDT_TEXT, sizeof(g_argProp) / sizeof(g_argProp[0]), g_argProp),
    WV_OBJ("取事件属性整数", "GetEventArgsInt", "按属性名读整数 / 枚举型参数。", _CMD_OS(__OS_WIN), SDT_INT, sizeof(g_argProp) / sizeof(g_argProp[0]), g_argProp),
    WV_OBJ("取事件属性逻辑", "GetEventArgsBool", "按属性名读逻辑型参数，如 Cancel、IsUserInitiated。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argProp) / sizeof(g_argProp[0]), g_argProp),
    WV_OBJ("取事件属性小数", "GetEventArgsDouble", "按属性名读小数型参数。", _CMD_OS(__OS_WIN), SDT_DOUBLE, sizeof(g_argProp) / sizeof(g_argProp[0]), g_argProp),
    WV_OBJ("置事件属性", "SetEventArgs", "按属性名写回参数对象，用于应答事件（如 Cancel / Handled / State）。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argSetProp) / sizeof(g_argSetProp[0]), g_argSetProp),
    WV_OBJ("取事件子对象", "GetEventArgsChild", "取属性里的对象（如下载事件里的 DownloadOperation），返回新的对象号。", _CMD_OS(__OS_WIN), SDT_INT, sizeof(g_argProp) / sizeof(g_argProp[0]), g_argProp),
    WV_OBJ("取事件列表数量", "GetEventArgsListCount", "取参数对象里列表属性的条目数。", _CMD_OS(__OS_WIN), SDT_INT, sizeof(g_argProp) / sizeof(g_argProp[0]), g_argProp),
    WV_OBJ("取事件列表成员", "GetEventArgsListItem", "取列表属性里的第 N 项，返回新的对象号。", _CMD_OS(__OS_WIN), SDT_INT, sizeof(g_argPropIdx) / sizeof(g_argPropIdx[0]), g_argPropIdx),
    WV_OBJ("取事件对象JSON", "GetEventArgsJson", "把任意事件参数对象整个转成 UTF-8 JSON 文本。", _CMD_OS(__OS_WIN), SDT_TEXT, sizeof(g_argObjId) / sizeof(g_argObjId[0]), g_argObjId),
    WV_OBJ("取得事件延后", "GetEventArgsDeferral", "挂起本事件的应答：先返回去做别的事，之后用 完成事件延后 收尾。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argObjId) / sizeof(g_argObjId[0]), g_argObjId),
    WV_OBJ("完成事件延后", "CompleteEventArgsDeferral", "结束延后，告诉内核本事件已处理完。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argObjId) / sizeof(g_argObjId[0]), g_argObjId),
    WV_OBJ("释放事件对象", "ReleaseEventArgs", "释放一个事件参数对象；留到组件销毁或库卸载时会自动回收。", _CMD_OS(__OS_WIN), SDT_BOOL, sizeof(g_argFreeId) / sizeof(g_argFreeId[0]), g_argFreeId),
};

// 成员命令在命令表中的下标，顺序必须与上面一致。
// 环境类的成员命令：构造 / 复制 / 析构 + 8 条能力命令。
INT g_envCmdIndex[] = { 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106 };
static_assert(sizeof(g_envCmdIndex) / sizeof(INT) == 11, "环境类成员命令条数变了");

INT g_pageViewCmdIndex[] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122 };

PFN_EXECUTE_CMD g_commandFuncs[] = {
    WvTest, WvDetectRuntime, WvGetRuntimeVersion,
    WvConstruct, WvDestruct, WvNavigate, WvLoadHtml, WvRefresh, WvGoBack, WvGoForward,
    WvStop, WvGetPageAddress, WvGetPageTitle, WvGetPageSource, WvIsLoading,
    WvCanGoBack, WvCanGoForward, WvExecuteScript, WvEvaluateScript, WvGetCacheFolder,
    WvSetZoom, WvOpenDevTools, WvClearCache, WvGetHandle, WvIsValid, WvDestroyUnit,
    WvReflGet, WvReflSet, WvReflCall, WvReflNames, WvCopy,
    WvSubEvt,
    WvUnsubEvt,
    WvEvtList,
    WvEvtName,
    WvEvtData,
    WvEvtObjJson,
    WvAnsPermission,
    WvAnsDialog,
    WvAnsDownload,
    WvAnsBaseAuth,
    WvAnsClientCert,
    WvAnsSvcCert,
    WvAnsMenu,
    WvAnsKey,
    WvAnsExtUri,
    WvAnsSaveAsUI,
    WvAnsNewWindow,
    WvDownloadInfo,
    WvTrySuspend,
    WvResume,
    WvSetMuted,
    WvIsPlayingAudio,
    WvBrowserPid,
    WvFavicon,
    WvStatusText,
    WvSetStatusText,
    WvGetZoom,
    WvSetZoomD,
    WvFullScreen,
    WvSetMemLevel,
    WvGetMemLevel,
    WvOpenTaskMgr,
    WvShowDownloadDlg,
    WvCloseDownloadDlg,
    WvPostMsg,
    WvPostMsgJson,
    WvAddInitScript,
    WvDelInitScript,
    WvDevToolsCall,
    WvDevToolsSub,
    WvMapHostName,
    WvUnmapHostName,
    WvAddReqFilter,
    WvDelReqFilter,
    WvAnsRequest,
    WvScreenshot,
    WvPrintPdf,
    WvPrintUI,
    WvPrintPage,
    WvShowSaveAs,
    WvGetCookies,
    WvSetCookie,
    WvDelCookie,
    WvDelCookieAll,
    WvDelAllCookies,
    WvProcInfos,
    WvClearDataAll,
    WvClearDataIn,
    WvExtensions,
    WvAddExtension,
    WvRemoveExtension,
    WvEnableExtension,
    WvBrowserVersion,
    WvProfileName,
    WvCompareVersions,
    WvEnvConstruct,
    WvEnvCopy,
    WvEnvDestruct,
    WvEnvCreate,
    WvEnvIsValid,
    WvEnvVersion,
    WvEnvCacheFolder,
    WvEnvProcInfos,
    WvEnvGet,
    WvEnvCall,
    WvEnvNames,
    WvUseEnv,
    WvObjCurId,
    WvObjIface,
    WvObjNames,
    WvObjText,
    WvObjInt,
    WvObjBool,
    WvObjDbl,
    WvObjSet,
    WvObjChild,
    WvObjListNum,
    WvObjListAt,
    WvObjJson,
    WvObjDefer,
    WvObjDeferDone,
    WvObjFreeCmd,
};

// 静态编译按名字找实现：顺序与数目必须和 g_commands / g_commandFuncs 完全一致。
const char* g_commandFuncNames[] = {
    "WvTest", "WvDetectRuntime", "WvGetRuntimeVersion",
    "WvConstruct", "WvDestruct", "WvNavigate", "WvLoadHtml", "WvRefresh", "WvGoBack", "WvGoForward",
    "WvStop", "WvGetPageAddress", "WvGetPageTitle", "WvGetPageSource", "WvIsLoading",
    "WvCanGoBack", "WvCanGoForward", "WvExecuteScript", "WvEvaluateScript", "WvGetCacheFolder",
    "WvSetZoom", "WvOpenDevTools", "WvClearCache", "WvGetHandle", "WvIsValid", "WvDestroyUnit",
    "WvReflGet", "WvReflSet", "WvReflCall", "WvReflNames", "WvCopy",
    "WvSubEvt",
    "WvUnsubEvt",
    "WvEvtList",
    "WvEvtName",
    "WvEvtData",
    "WvEvtObjJson",
    "WvAnsPermission",
    "WvAnsDialog",
    "WvAnsDownload",
    "WvAnsBaseAuth",
    "WvAnsClientCert",
    "WvAnsSvcCert",
    "WvAnsMenu",
    "WvAnsKey",
    "WvAnsExtUri",
    "WvAnsSaveAsUI",
    "WvAnsNewWindow",
    "WvDownloadInfo",
    "WvTrySuspend",
    "WvResume",
    "WvSetMuted",
    "WvIsPlayingAudio",
    "WvBrowserPid",
    "WvFavicon",
    "WvStatusText",
    "WvSetStatusText",
    "WvGetZoom",
    "WvSetZoomD",
    "WvFullScreen",
    "WvSetMemLevel",
    "WvGetMemLevel",
    "WvOpenTaskMgr",
    "WvShowDownloadDlg",
    "WvCloseDownloadDlg",
    "WvPostMsg",
    "WvPostMsgJson",
    "WvAddInitScript",
    "WvDelInitScript",
    "WvDevToolsCall",
    "WvDevToolsSub",
    "WvMapHostName",
    "WvUnmapHostName",
    "WvAddReqFilter",
    "WvDelReqFilter",
    "WvAnsRequest",
    "WvScreenshot",
    "WvPrintPdf",
    "WvPrintUI",
    "WvPrintPage",
    "WvShowSaveAs",
    "WvGetCookies",
    "WvSetCookie",
    "WvDelCookie",
    "WvDelCookieAll",
    "WvDelAllCookies",
    "WvProcInfos",
    "WvClearDataAll",
    "WvClearDataIn",
    "WvExtensions",
    "WvAddExtension",
    "WvRemoveExtension",
    "WvEnableExtension",
    "WvBrowserVersion",
    "WvProfileName",
    "WvCompareVersions",
    "WvEnvConstruct",
    "WvEnvCopy",
    "WvEnvDestruct",
    "WvEnvCreate",
    "WvEnvIsValid",
    "WvEnvVersion",
    "WvEnvCacheFolder",
    "WvEnvProcInfos",
    "WvEnvGet",
    "WvEnvCall",
    "WvEnvNames",
    "WvUseEnv",
    "WvObjCurId",
    "WvObjIface",
    "WvObjNames",
    "WvObjText",
    "WvObjInt",
    "WvObjBool",
    "WvObjDbl",
    "WvObjSet",
    "WvObjChild",
    "WvObjListNum",
    "WvObjListAt",
    "WvObjJson",
    "WvObjDefer",
    "WvObjDeferDone",
    "WvObjFreeCmd",
};

const TCHAR g_categories[] = _T("0000Edge网页\0\0");

// ---- 表自洽性检查：错位一格 = 命令调到别人的实现，而且常常不报错 ----
static_assert(sizeof(g_commands) / sizeof(g_commands[0]) == 123, "命令表数目变了，同步更新此处");
static_assert(sizeof(g_commandFuncs) / sizeof(g_commandFuncs[0]) == sizeof(g_commands) / sizeof(g_commands[0]),
              "命令表与实现表数目不一致");
static_assert(sizeof(g_commandFuncNames) / sizeof(g_commandFuncNames[0]) == sizeof(g_commands) / sizeof(g_commands[0]),
              "命令表与函数名表数目不一致");
static_assert(sizeof(g_pageViewCmdIndex) / sizeof(g_pageViewCmdIndex[0]) == 108, "成员命令数目变了");

static_assert(sizeof(g_properties) / sizeof(g_properties[0]) == WVPROP_COUNT, "属性表与属性序号宏不一致");
static_assert(sizeof(g_events) / sizeof(g_events[0]) == WVEVENT_COUNT, "事件表与事件序号宏不一致");
static_assert(WVPROP_COUNT == FIXED_WIN_UNIT_PROPERTY_COUNT + 6, "固定属性 + 自定义属性数目不符");

// 内核枚举与数值常量（权限状态 / 错误码 / 资源类型 / 截图格式…），由 gen_consts.mjs 生成。
#include "wv_consts.h"

// 成员命令下标表回填到数据类型表（编译期不能取用后面数组的地址）。
struct WvFixup
{
    WvFixup()
    {
        g_dataTypes[WV_TYPE_PAGEVIEW - 1].m_nCmdCount =
            (INT)(sizeof(g_pageViewCmdIndex) / sizeof(g_pageViewCmdIndex[0]));
        g_dataTypes[WV_TYPE_PAGEVIEW - 1].m_pnCmdsIndex = g_pageViewCmdIndex;
        g_dataTypes[WV_TYPE_ENV - 1].m_nCmdCount =
            (INT)(sizeof(g_envCmdIndex) / sizeof(g_envCmdIndex[0]));
        g_dataTypes[WV_TYPE_ENV - 1].m_pnCmdsIndex = g_envCmdIndex;
    }
};
WvFixup g_wvFixup;

#endif  // !__E_STATIC_LIB

// ============================================================ ⑥ 库通知

INT WINAPI edgewv_ProcessNotifyLib(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    UNREFERENCED_PARAMETER(dwParam2);
    switch (nMsg)
    {
    case NL_SYS_NOTIFY_FUNCTION:
        // 装载支持库前通知，可能来多次，后值覆盖前值。
        g_pfnNotifySys = (PFN_NOTIFY_SYS)dwParam1;
        return NR_OK;

#ifndef __E_STATIC_LIB
    case NL_GET_CMD_FUNC_NAMES:             // 静态编译必需
        return (INT)g_commandFuncNames;
    case NL_GET_NOTIFY_LIB_FUNC_NAME:       // 静态编译必需
        return (INT)WV_NOTIFY_FUNC_NAME;
    case NL_GET_DEPENDENT_LIBS:             // 静态编译必需：不链外部静态库
        return (INT)NULL;
#endif

    case NL_FREE_LIB_DATA:
    case NL_UNLOAD_FROM_IDE:
        EnvCacheReleaseAll();
        g_pfnNotifySys = NULL;
        return NR_OK;

    default:
        return NR_ERR;
    }
}

// ============================================================ ⑦ 库信息
// 字段顺序固定，见 lib2.h 的 LIB_INFO。

#ifndef __E_STATIC_LIB

LIB_INFO g_libraryInfo = {
    LIB_FORMAT_VER,
    WV_GUID,
    2, 1, 0,                       // 主/次/构建版本号
    3, 0,                          // 要求的易语言系统版本：只可 ≤ 目标机实际版本
    3, 0,                          // 要求的系统核心支持库版本（同上，填高了 IDE 会拒载）
    WV_LIB_NAME,
    __GBK_LANG_VER,
    _T("Edge 网页浏览组件：把网页当控件拖进窗口，支持自定义缓存目录、脚本交互、开发者工具。"),
    _LIB_OS(__OS_WIN),
    _T("创想软件"),
    _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""),
    sizeof(g_dataTypes) / sizeof(g_dataTypes[0]),
    g_dataTypes,
    1, const_cast<TCHAR*>(g_categories),
    sizeof(g_commands) / sizeof(g_commands[0]),
    g_commands,
    g_commandFuncs,
    NULL, NULL,                    // AddIn
    edgewv_ProcessNotifyLib,
    NULL, NULL,                    // 超级模板
    WV_CONST_COUNT, g_wvConsts,    // 库常量：内核枚举与数值常量
    const_cast<TCHAR*>(_T(WV_LOADER_DLL "\0")),   // 随库分发的依赖文件
};

#endif  // !__E_STATIC_LIB

}  // namespace wv

// ==== 以下由 gen_static_trampolines.mjs 生成，勿手改 ====
// 静态编译时易语言按名字到 edgewv_static.lib 里找命令实现，
// 而实现在 namespace wv 内，故每个命令生成一份 extern "C" 全局转发。
#define WV_FWD(fn) extern "C" void fn (PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf) { wv::fn (pRetData, nArgCount, pArgInf); }

WV_FWD(WvTest)
WV_FWD(WvDetectRuntime)
WV_FWD(WvGetRuntimeVersion)
WV_FWD(WvConstruct)
WV_FWD(WvDestruct)
WV_FWD(WvNavigate)
WV_FWD(WvLoadHtml)
WV_FWD(WvRefresh)
WV_FWD(WvGoBack)
WV_FWD(WvGoForward)
WV_FWD(WvStop)
WV_FWD(WvGetPageAddress)
WV_FWD(WvGetPageTitle)
WV_FWD(WvGetPageSource)
WV_FWD(WvIsLoading)
WV_FWD(WvCanGoBack)
WV_FWD(WvCanGoForward)
WV_FWD(WvExecuteScript)
WV_FWD(WvEvaluateScript)
WV_FWD(WvGetCacheFolder)
WV_FWD(WvSetZoom)
WV_FWD(WvOpenDevTools)
WV_FWD(WvClearCache)
WV_FWD(WvGetHandle)
WV_FWD(WvIsValid)
WV_FWD(WvDestroyUnit)
WV_FWD(WvReflGet)
WV_FWD(WvReflSet)
WV_FWD(WvReflCall)
WV_FWD(WvReflNames)
WV_FWD(WvCopy)
WV_FWD(WvSubEvt)
WV_FWD(WvUnsubEvt)
WV_FWD(WvEvtList)
WV_FWD(WvEvtName)
WV_FWD(WvEvtData)
WV_FWD(WvEvtObjJson)
WV_FWD(WvAnsPermission)
WV_FWD(WvAnsDialog)
WV_FWD(WvAnsDownload)
WV_FWD(WvAnsBaseAuth)
WV_FWD(WvAnsClientCert)
WV_FWD(WvAnsSvcCert)
WV_FWD(WvAnsMenu)
WV_FWD(WvAnsKey)
WV_FWD(WvAnsExtUri)
WV_FWD(WvAnsSaveAsUI)
WV_FWD(WvAnsNewWindow)
WV_FWD(WvDownloadInfo)
WV_FWD(WvTrySuspend)
WV_FWD(WvResume)
WV_FWD(WvSetMuted)
WV_FWD(WvIsPlayingAudio)
WV_FWD(WvBrowserPid)
WV_FWD(WvFavicon)
WV_FWD(WvStatusText)
WV_FWD(WvSetStatusText)
WV_FWD(WvGetZoom)
WV_FWD(WvSetZoomD)
WV_FWD(WvFullScreen)
WV_FWD(WvSetMemLevel)
WV_FWD(WvGetMemLevel)
WV_FWD(WvOpenTaskMgr)
WV_FWD(WvShowDownloadDlg)
WV_FWD(WvCloseDownloadDlg)
WV_FWD(WvPostMsg)
WV_FWD(WvPostMsgJson)
WV_FWD(WvAddInitScript)
WV_FWD(WvDelInitScript)
WV_FWD(WvDevToolsCall)
WV_FWD(WvDevToolsSub)
WV_FWD(WvMapHostName)
WV_FWD(WvUnmapHostName)
WV_FWD(WvAddReqFilter)
WV_FWD(WvDelReqFilter)
WV_FWD(WvAnsRequest)
WV_FWD(WvScreenshot)
WV_FWD(WvPrintPdf)
WV_FWD(WvPrintUI)
WV_FWD(WvPrintPage)
WV_FWD(WvShowSaveAs)
WV_FWD(WvGetCookies)
WV_FWD(WvSetCookie)
WV_FWD(WvDelCookie)
WV_FWD(WvDelCookieAll)
WV_FWD(WvDelAllCookies)
WV_FWD(WvProcInfos)
WV_FWD(WvClearDataAll)
WV_FWD(WvClearDataIn)
WV_FWD(WvExtensions)
WV_FWD(WvAddExtension)
WV_FWD(WvRemoveExtension)
WV_FWD(WvEnableExtension)
WV_FWD(WvBrowserVersion)
WV_FWD(WvProfileName)
WV_FWD(WvCompareVersions)
WV_FWD(WvEnvConstruct)
WV_FWD(WvEnvCopy)
WV_FWD(WvEnvDestruct)
WV_FWD(WvEnvCreate)
WV_FWD(WvEnvIsValid)
WV_FWD(WvEnvVersion)
WV_FWD(WvEnvCacheFolder)
WV_FWD(WvEnvProcInfos)
WV_FWD(WvEnvGet)
WV_FWD(WvEnvCall)
WV_FWD(WvEnvNames)
WV_FWD(WvUseEnv)
WV_FWD(WvObjCurId)
WV_FWD(WvObjIface)
WV_FWD(WvObjNames)
WV_FWD(WvObjText)
WV_FWD(WvObjInt)
WV_FWD(WvObjBool)
WV_FWD(WvObjDbl)
WV_FWD(WvObjSet)
WV_FWD(WvObjChild)
WV_FWD(WvObjListNum)
WV_FWD(WvObjListAt)
WV_FWD(WvObjJson)
WV_FWD(WvObjDefer)
WV_FWD(WvObjDeferDone)
WV_FWD(WvObjFreeCmd)

#undef WV_FWD

extern "C" INT WINAPI edgewv_ProcessNotifyLib (INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    return wv::edgewv_ProcessNotifyLib (nMsg, dwParam1, dwParam2);
}

// 组件接口：静态编译同样按名字挑这些符号，所以要有全局名 + dllexport。
extern "C" __declspec(dllexport) HUNIT WINAPI wv_CreateUnit (LPBYTE pAllData, INT nAllDataSize, DWORD dwStyle, HWND hParentWnd, UINT uID, HMENU hMenu, INT x, INT y, INT cx, INT cy, DWORD dwWinFormID, DWORD dwUnitID, HWND hDesignWnd, BOOL blInDesignMode) { return wv::pvi_CreateUnit(pAllData, nAllDataSize, dwStyle, hParentWnd, uID, hMenu, x, y, cx, cy, dwWinFormID, dwUnitID, hDesignWnd, blInDesignMode); }
extern "C" __declspec(dllexport) BOOL WINAPI wv_PropertyUpdateUI (HUNIT hUnit, INT nPropertyIndex) { return wv::pvi_PropertyUpdateUI(hUnit, nPropertyIndex); }
extern "C" __declspec(dllexport) BOOL WINAPI wv_NotifyPropertyChanged (HUNIT hUnit, INT nPropertyIndex, PUNIT_PROPERTY_VALUE pPropertyValue, LPTSTR* ppszTipText) { return wv::pvi_NotifyPropertyChanged(hUnit, nPropertyIndex, pPropertyValue, ppszTipText); }
extern "C" __declspec(dllexport) BOOL WINAPI wv_GetPropertyData (HUNIT hUnit, INT nPropertyIndex, PUNIT_PROPERTY_VALUE pPropertyValue) { return wv::pvi_GetPropertyData(hUnit, nPropertyIndex, pPropertyValue); }
extern "C" __declspec(dllexport) HGLOBAL WINAPI wv_GetAllPropertyData (HUNIT hUnit) { return wv::pvi_GetAllPropertyData(hUnit); }
// 运行时按「短名_GetInterface_<组件数据类型英文名>」找符号，两个名字都要有。
extern "C" __declspec(dllexport) PFN_INTERFACE WINAPI edgewv_GetInterface_WebView (INT nInterfaceNO)
{
    return wv::pvi_GetInterface_WebView (nInterfaceNO);
}
extern "C" __declspec(dllexport) PFN_INTERFACE WINAPI edgewv_GetInterface_WebViewBox (INT nInterfaceNO)
{
    return wv::pvi_GetInterface_WebView (nInterfaceNO);
}

// 导出名必须固定为 GetNewInf 且不带修饰，所以靠 .def 导出而不是 __declspec。
#ifndef __E_STATIC_LIB
extern "C" PLIB_INFO WINAPI GetNewInf()
{
    return &wv::g_libraryInfo;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        wv::g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        wv::EnvCacheReleaseAll();
    }
    return TRUE;
}
#endif  // !__E_STATIC_LIB
