// 创想AH会员框架 · 事件与自动刷新
// 三种取事件方式：轮询队列（最稳）、窗口消息（跨线程安全）、子程序回调（只允许登记线程自己收）。
// 自动刷新在库内起一个工作线程定时调 会员_刷新授权，状态变化会发事件，宿主不用自己写时钟。
#include "cx_core.h"

#include <stdio.h>
#include <string.h>

namespace cx {

// ==================== 事件登记 ====================

namespace {

struct EvItem {
    int code;
    std::string json;
};

CRITICAL_SECTION g_cs;
volatile LONG g_csState = 0;      // 0 未初始化 1 初始化中 2 就绪
std::vector<EvItem> g_queue;
HWND g_hwnd = NULL;
UINT g_msg = 0;
PFN_CX_EVENT g_cb = NULL;
DWORD g_cbThread = 0;
volatile LONG g_handledCode = 0;    // 宿主说「这条我自己处理」的事件码

HANDLE g_autoThread = NULL;
HANDLE g_autoStop = NULL;
volatile LONG g_autoOn = 0;
volatile int g_autoSeconds = 0;
volatile LONG g_autoTick = 0;     // 手动唤醒：启动后立即先刷一次

const int CX_EV_MAX = 200;        // 队列上限，超了丢最老的

void EnsureLock()
{
    if (g_csState == 2) return;
    if (InterlockedCompareExchange(&g_csState, 1, 0) == 0) {
        InitializeCriticalSection(&g_cs);
        MemoryBarrier();
        g_csState = 2;
        return;
    }
    while (g_csState != 2) Sleep(1);
}

struct Named { int code; const char* en; const char* cn; };
const Named kEvents[] = {
    { EV_LOGIN_OK,        "login_ok",        "登录成功，已拿到授权" },
    { EV_LOGIN_FAIL,      "login_fail",      "登录失败，d.code 是接口错误码" },
    { EV_REGISTER_OK,     "register_ok",     "注册成功" },
    { EV_TRIAL_OK,        "trial_ok",        "免费试用已开通" },
    { EV_ACTIVATE_OK,     "activate_ok",     "序列号激活成功，授权已充值" },
    { EV_CHANGE_OK,       "change_ok",       "换机成功，本机已绑定" },
    { EV_MAC_CHANGED,     "mac_changed",     "账号已改绑到别的电脑，本机授权作废" },
    { EV_AUTH_REFRESHED,  "auth_refreshed",  "授权信息已刷新" },
    { EV_AUTH_EXPIRED,    "auth_expired",    "授权已到期" },
    { EV_AUTH_FROZEN,     "auth_frozen",     "账号已被停止使用" },
    { EV_AUTH_LOST,       "auth_lost",       "授权已失效（到期/冻结/取消），功能要立刻收回去" },
    { EV_AUTH_SOON_EXPIRE,"soon_expire",     "授权快要到期，d.days 是剩余天数" },
    { EV_SERVER_UNREACHABLE,"unreachable",   "连不上授权服务器，网络或地址有问题" },
    { EV_LOGOUT,          "logout",          "已退出本机账号" },
    { EV_AUTO_STARTED,    "auto_started",    "自动刷新已启动" },
    { EV_AUTO_STOPPED,    "auto_stopped",    "自动刷新已停止" },
    { EV_WINDOW_CLOSE,    "window_close",    "会员窗口被关闭" }
};

const char* EventEn(int code)
{
    for (size_t i = 0; i < sizeof(kEvents) / sizeof(kEvents[0]); ++i) {
        if (kEvents[i].code == code) return kEvents[i].en;
    }
    return "unknown";
}

}  // namespace

const char* EventName(int code)
{
    for (size_t i = 0; i < sizeof(kEvents) / sizeof(kEvents[0]); ++i) {
        if (kEvents[i].code == code) return kEvents[i].cn;
    }
    return "";
}

void FireEvent(int code, const std::string& fields)
{
    if (code == EV_NONE) return;
    EnsureLock();
    EnterCriticalSection(&g_cs);

    EvItem item;
    item.code = code;
    item.json = "{\"e\":" + ToStr(code) + ",\"n\":\"" + EventEn(code) +
                "\",\"t\":\"" + NowBeijingText() + "\",\"d\":" + (fields.empty() ? "{}" : fields) + "}";
    g_queue.push_back(item);
    if ((int)g_queue.size() > CX_EV_MAX) {
        g_queue.erase(g_queue.begin(), g_queue.begin() + ((int)g_queue.size() - CX_EV_MAX));
    }

    const HWND hwnd = g_hwnd;
    const UINT msg = g_msg;
    const PFN_CX_EVENT cb = g_cb;
    const DWORD cbThread = g_cbThread;
    LeaveCriticalSection(&g_cs);

    // 窗口消息：任何线程都能发，宿主在界面线程里收，最推荐
    if (hwnd != NULL && msg != 0) PostMessageW(hwnd, msg, (WPARAM)code, 0);
    // 回调：只在登记它的那个线程上触发，避免从库的工作线程直接扎进宿主界面代码
    if (cb != NULL && cbThread == GetCurrentThreadId() && cb(code) == 1) {
        InterlockedExchange(&g_handledCode, code);
    }
}

void MarkEventHandled(int code)
{
    InterlockedExchange(&g_handledCode, code);
}

bool IsEventHandled(int code)
{
    return InterlockedCompareExchange(&g_handledCode, 0, 0) == code;
}

// 需要「宿主不接管就由库提醒」的事件走这里。回调是同步的，答过就不用等；
// 只登记了窗口消息的宿主要回一句 会员_事件已接管，所以在宽限期内轮询那个标记。
bool FireEventWait(int code, const std::string& fields, int graceMs)
{
    InterlockedExchange(&g_handledCode, 0);
    // 宿主既没登记回调也没登记窗口，等多久都不会有人接管，直接走默认提醒。
    const bool mayTakeOver = (g_cb != NULL) || (g_hwnd != NULL && g_msg != 0);
    FireEvent(code, fields);
    if (IsEventHandled(code)) return true;
    if (!mayTakeOver) return false;   // 没登记接收方，没人会来接管，别白等
    const DWORD deadline = GetTickCount() + (DWORD)(graceMs < 0 ? 0 : graceMs);
    while (!IsEventHandled(code)) {
        if ((LONG)(deadline - GetTickCount()) <= 0) break;
        Sleep(20);
    }
    return IsEventHandled(code);
}

bool EventSetWindow(HWND h, UINT msgId)
{
    EnsureLock();
    EnterCriticalSection(&g_cs);
    g_hwnd = h;
    g_msg = msgId;
    LeaveCriticalSection(&g_cs);
    return true;
}

bool EventSetCallback(PFN_CX_EVENT cb)
{
    EnsureLock();
    EnterCriticalSection(&g_cs);
    g_cb = cb;
    g_cbThread = GetCurrentThreadId();
    LeaveCriticalSection(&g_cs);
    return true;
}

bool EventPoll(std::string* jsonOut)
{
    if (jsonOut == NULL) return false;
    EnsureLock();
    EnterCriticalSection(&g_cs);
    bool got = !g_queue.empty();
    if (got) {
        *jsonOut = g_queue.front().json;
        g_queue.erase(g_queue.begin());
    }
    LeaveCriticalSection(&g_cs);
    return got;
}

int EventCount()
{
    EnsureLock();
    EnterCriticalSection(&g_cs);
    const int n = (int)g_queue.size();
    LeaveCriticalSection(&g_cs);
    return n;
}

void EventClear()
{
    EnsureLock();
    EnterCriticalSection(&g_cs);
    g_queue.clear();
    LeaveCriticalSection(&g_cs);
}

// ==================== 授权状态变化 ====================

bool IsAuthUsable(const std::string& status)
{
    return status == "ok" || status == "trial";
}

namespace {

volatile LONG g_alertUp = 0;      // 同一时刻只允许一张提醒框

// 提醒走一次性线程：调接口的线程（可能就是库自己的自动刷新线程）不能被模态框卡住。
DWORD WINAPI AlertProc(LPVOID p)
{
    std::wstring* text = (std::wstring*)p;
    MessageBoxW(S().hParent, text->c_str(),
        WideFromUtf8(GbkToUtf8("创想软件授权提醒")).c_str(),
        MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
    delete text;
    InterlockedExchange(&g_alertUp, 0);
    return 0;
}

void QueueAlert(const std::string& utf8Text)
{
    if (InterlockedCompareExchange(&g_alertUp, 0, 0) != 0) return;   // 上一张还没点掉，不再叠
    InterlockedExchange(&g_alertUp, 1);
    std::wstring* w = new std::wstring(WideFromUtf8(utf8Text));
    DWORD tid = 0;
    HANDLE h = CreateThread(NULL, 0, AlertProc, w, 0, &tid);
    if (h == NULL) { delete w; InterlockedExchange(&g_alertUp, 0); return; }
    CloseHandle(h);
}

}  // namespace

// 默认提醒只在宿主没接管时才出现，文案里不写接口名和错误码，只写用户要做的事。
void AlertMacChanged()
{
    QueueAlert(GbkToUtf8("您的账户已更换到其他电脑，即将退出。\r\n")
        + GbkToUtf8("如需继续在本机使用，请重新登录并按提示完成换机验证。"));
}

void AlertSoonExpire(int days)
{
    const std::string text = days <= 0
        ? GbkToUtf8("您的授权今天到期，请及时续费，到期后软件将无法继续使用。")
        : GbkToUtf8("您的授权还有 ") + ToStr(days) + GbkToUtf8(" 天到期（到期时间 ") + S().grant.dueDate
          + GbkToUtf8("），请及时续费。");
    QueueAlert(text);
}

// 被别的电脑顶掉时只收回「本机可用」这一件事：账号和换机次数还得留着，
// 用户就是从这台电脑上换机换回来的，清了本地账号等于把退路也删了。
void RevokeLocalGrant()
{
    State& st = S();
    const int left = st.grant.changeLeft;
    const int limit = st.grant.limitChange;
    const int used = st.grant.changeTime;
    st.grant = Grant();
    st.grant.authStatus = "revoked";
    st.grant.changeLeft = left;
    st.grant.limitChange = limit;
    st.grant.changeTime = used;
}

void FireAuthChange(const std::string& before, const std::string& after)
{
    JBuild b;
    b.str("status", after);
    b.str("due_date", S().grant.dueDate);
    const std::string fields = b.done();
    FireEvent(EV_AUTH_REFRESHED, fields);
    if (after == "expired") FireEvent(EV_AUTH_EXPIRED, fields);
    else if (after == "frozen") FireEvent(EV_AUTH_FROZEN, fields);
    else if (IsAuthUsable(before) && !IsAuthUsable(after)) FireEvent(EV_AUTH_LOST, fields);

    if (!IsAuthUsable(after) || S().expireWarnDays <= 0) return;
    const int days = DaysToExpire();
    const std::string today = NowBeijingText().substr(0, 10);
    if (days < 0 || days > S().expireWarnDays || S().expireWarnDay == today) return;
    S().expireWarnDay = today;       // 一天只提一次，自动刷新每几分钟就会走到这里
    JBuild e;
    e.str("status", after);
    e.str("due_date", S().grant.dueDate);
    e.str("days", ToStr(days));
    if (!FireEventWait(EV_AUTH_SOON_EXPIRE, e.done(), 800) && S().autoAlert) AlertSoonExpire(days);
}

// ==================== 到期提醒策略 ====================

void SetAlertPolicy(bool autoAlert, int warnDays)
{
    S().autoAlert = autoAlert;
    if (warnDays >= 0) S().expireWarnDays = warnDays > 3650 ? 3650 : warnDays;
}
bool IsAutoAlertOn()     { return S().autoAlert; }
int  GetExpireWarnDays() { return S().expireWarnDays; }

namespace {

// "YYYY-MM-DD HH:MM:SS"（北京时间）-> UTC 毫秒时间戳；解析失败返回 -1。
// 不用 mktime：那会跟着本机时区走，用户改时区就把到期算错。
long long TextToMs(const std::string& text)
{
    int v[6] = { 0, 0, 0, 0, 0, 0 };
    const char* p = text.c_str();
    for (int n = 0; n < 6; ++n) {
        int got = 0;
        while (*p >= '0' && *p <= '9') { v[n] = v[n] * 10 + (*p - '0'); ++p; ++got; }
        if (got == 0) return -1;
        if (n < 5) {
            if (*p == 0) return -1;
            ++p;
        }
    }
    if (v[0] < 1970 || v[1] < 1 || v[1] > 12 || v[2] < 1 || v[2] > 31) return -1;
    SYSTEMTIME st;
    st.wYear = (WORD)v[0]; st.wMonth = (WORD)v[1]; st.wDay = (WORD)v[2];
    st.wHour = (WORD)v[3]; st.wMinute = (WORD)v[4]; st.wSecond = (WORD)v[5];
    st.wMilliseconds = 0;
    FILETIME ft;
    if (!SystemTimeToFileTime(&st, &ft)) return -1;
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime; u.HighPart = ft.dwHighDateTime;
    u.QuadPart -= 8ULL * 3600ULL * 10000000ULL;         // 北京时间换回 UTC
    u.QuadPart -= 116444736000000000ULL;                // 1601 历元换回 1970
    return (long long)(u.QuadPart / 10000ULL);
}

}  // namespace

int DaysToExpire()
{
    if (S().grant.dueDate.empty()) return 0;
    const long long due = TextToMs(S().grant.dueDate);
    if (due < 0) return 0;
    const long long diff = due - NowMs();
    return (int)(diff / 86400000LL);
}

// ==================== 自动刷新线程 ====================

namespace {

DWORD WINAPI AutoProc(LPVOID)
{
    // 启动后先刷一次，之后按间隔循环；停止事件被置位就立刻退出
    for (;;) {
        const int waitMs = (InterlockedExchangeAdd(&g_autoTick, 0) != 0)
                               ? 0
                               : (g_autoSeconds < 5 ? 5 : g_autoSeconds) * 1000;
        InterlockedExchange(&g_autoTick, 0);
        const DWORD r = WaitForSingleObject(g_autoStop, (DWORD)waitMs);
        if (r != WAIT_TIMEOUT || InterlockedCompareExchange(&g_autoOn, 0, 0) == 0) break;

        const std::string email = S().email.empty() ? LoadAccount() : S().email;
        if (!email.empty() && !S().server.empty()) DoRefresh(email);   // 事件由接口层统一发
    }
    InterlockedExchange(&g_autoOn, 0);
    return 0;
}

}  // namespace

bool AutoStart(int seconds, HWND h, UINT msgId)
{
    if (seconds < 5) seconds = 5;
    if (h != NULL && msgId != 0) EventSetWindow(h, msgId);
    if (InterlockedCompareExchange(&g_autoOn, 1, 0) != 0) {
        g_autoSeconds = seconds;      // 已在跑：只改间隔，下一次等待就生效
        InterlockedExchange(&g_autoTick, 1);
        return true;
    }
    EnsureLock();
    g_autoSeconds = seconds;
    InterlockedExchange(&g_autoTick, 1);
    if (g_autoStop == NULL) g_autoStop = CreateEventW(NULL, TRUE, FALSE, NULL);
    ResetEvent(g_autoStop);
    DWORD tid = 0;
    g_autoThread = CreateThread(NULL, 0, AutoProc, NULL, 0, &tid);
    if (g_autoThread == NULL) {
        InterlockedExchange(&g_autoOn, 0);
        return false;
    }
    FireEvent(EV_AUTO_STARTED, "{\"seconds\":" + ToStr(seconds) + "}");
    return true;
}

void AutoStop()
{
    if (InterlockedCompareExchange(&g_autoOn, 0, 1) != 1) return;
    if (g_autoStop != NULL) SetEvent(g_autoStop);
    if (g_autoThread != NULL) {
        WaitForSingleObject(g_autoThread, 8000);
        CloseHandle(g_autoThread);
        g_autoThread = NULL;
    }
    FireEvent(EV_AUTO_STOPPED, "{}");
}

int AutoInterval()
{
    return InterlockedCompareExchange(&g_autoOn, 0, 0) == 0 ? 0 : g_autoSeconds;
}

void EventShutdown()
{
    AutoStop();
    EnsureLock();
    EnterCriticalSection(&g_cs);
    g_queue.clear();
    g_hwnd = NULL;
    g_msg = 0;
    g_cb = NULL;
    LeaveCriticalSection(&g_cs);
}

}  // namespace cx
