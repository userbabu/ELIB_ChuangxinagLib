// 线程锁族：Windows 内核对象（Mutex / Semaphore）+ 条件变量的运行期入口。
// 一律 LoadLibrary + GetProcAddress（stdcall），不新增链接依赖；拿不到入口就报运行时错误而不是崩。
// 由 lib.cpp 在 namespace cx 内、cxhttp_util.h 之后 #include。
#ifndef CXLOCK_API_INC
#define CXLOCK_API_INC

typedef void* HXHANDLE;
typedef struct _LK_SECURITY *LPXSECURITY;
typedef HXHANDLE (WINAPI *fn_create_mutex_t)(LPXSECURITY, BOOL, LPCSTR);
typedef HXHANDLE (WINAPI *fn_open_mutex_t)(DWORD, BOOL, LPCSTR);
typedef long (WINAPI *fn_release_mutex_t)(HXHANDLE);
typedef HXHANDLE (WINAPI *fn_create_sem_t)(LPXSECURITY, LONG, LONG, LPCSTR);
typedef long (WINAPI *fn_release_sem_t)(HXHANDLE, LONG, LPVOID);
typedef long (WINAPI *fn_wait_t)(HXHANDLE, DWORD);
typedef long (WINAPI *fn_close_t)(HXHANDLE);
struct LkApi
{
    HMODULE hMod;
    fn_create_mutex_t CreateMutexA;
    fn_open_mutex_t OpenMutexA;
    fn_release_mutex_t ReleaseMutex;
    fn_create_sem_t CreateSemaphoreA;
    fn_release_sem_t ReleaseSemaphore;
    fn_wait_t WaitForSingleObject;
    fn_close_t CloseHandle;
    char LoadError[128];
};
static LkApi g_lk = { 0 };

static const LkApi& LkGet()
{
    if (g_lk.hMod != NULL) return g_lk;
    const HMODULE h = LoadLibraryA("kernel32.dll");
    if (h == NULL) { lstrcpynA(g_lk.LoadError, "无法加载 kernel32.dll", 128); return g_lk; }
    g_lk.CreateMutexA = (fn_create_mutex_t)GetProcAddress(h, "CreateMutexA");
    g_lk.OpenMutexA = (fn_open_mutex_t)GetProcAddress(h, "OpenMutexA");
    g_lk.ReleaseMutex = (fn_release_mutex_t)GetProcAddress(h, "ReleaseMutex");
    g_lk.CreateSemaphoreA = (fn_create_sem_t)GetProcAddress(h, "CreateSemaphoreA");
    g_lk.ReleaseSemaphore = (fn_release_sem_t)GetProcAddress(h, "ReleaseSemaphore");
    g_lk.WaitForSingleObject = (fn_wait_t)GetProcAddress(h, "WaitForSingleObject");
    g_lk.CloseHandle = (fn_close_t)GetProcAddress(h, "CloseHandle");
    if (g_lk.CreateMutexA == NULL || g_lk.WaitForSingleObject == NULL
        || g_lk.ReleaseMutex == NULL || g_lk.CreateSemaphoreA == NULL
        || g_lk.ReleaseSemaphore == NULL || g_lk.CloseHandle == NULL)
        lstrcpynA(g_lk.LoadError, "kernel32 线程同步入口不全", 128);
    g_lk.hMod = h;
    return g_lk;
}
#define WAIT_INFINITE 0xFFFFFFFFu
#define LK_WAIT_FAILED 0xFFFFFFFFu
static const LONG LK_MAX_COUNT = 0x7FFFFFFF;

#endif  // CXLOCK_API_INC
