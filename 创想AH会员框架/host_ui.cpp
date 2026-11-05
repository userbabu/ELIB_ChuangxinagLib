// UI 离线宿主：不靠人手点，直接在宿主里驱动会员窗口——
// 填控件、按按钮、断言面板切换与授权结果，覆盖 注册→自动试用→复制机器码→登出→换机引导→续费 全流程。
// 编译与运行见 build_host.bat（与命令宿主同一套参数）。
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <tchar.h>
#include <vector>

typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;

#include <lib2.h>
#include <lang.h>
#include "cx_ui_ids.h"

static int g_passed = 0, g_failed = 0;

INT WINAPI FakeNotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    UNREFERENCED_PARAMETER(dwParam2);
    switch (nMsg) {
    case NRS_MALLOC: return (INT)(INT_PTR)malloc(dwParam1 ? dwParam1 : 1);
    case NRS_MFREE: if (dwParam1) free((void*)(INT_PTR)dwParam1); return 0;
    case NRS_RUNTIME_ERR: printf("    [runerr] %s\n", (const char*)(INT_PTR)dwParam1); return 0;
    default: return 0;
    }
}

static void Check(bool cond, const char* name, const char* detail = NULL)
{
    if (cond) { g_passed++; printf("  [ OK ] %s\n", name); }
    else { g_failed++; printf("  [FAIL] %s  %s\n", name, detail ? detail : ""); }
}

// ---------------- 编码转换 ----------------
static std::string Utf8Of(const std::wstring& w)
{
    if (w.empty()) return std::string();
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string out((size_t)(n > 0 ? n : 0), char(0));
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &out[0], n, NULL, NULL);
    return out;
}

static std::wstring WideOfLocal(const char* gbk)
{
    const int n = MultiByteToWideChar(CP_ACP, 0, gbk, (int)strlen(gbk), NULL, 0);
    std::wstring out((size_t)(n > 0 ? n : 0), wchar_t(0));
    if (n > 0) MultiByteToWideChar(CP_ACP, 0, gbk, (int)strlen(gbk), &out[0], n);
    return out;
}

static std::wstring WideOf(const std::string& utf8)
{
    if (utf8.empty()) return std::wstring();
    const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), NULL, 0);
    std::wstring out((size_t)(n > 0 ? n : 0), wchar_t(0));
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &out[0], n);
    return out;
}

// ---------------- 库装载与调用 ----------------
static PLIB_INFO g_info = NULL;

static int FindCmd(const char* name)
{
    for (int i = 0; i < g_info->m_nCmdCount; ++i) {
        if (_tcscmp(g_info->m_pBeginCmdInfo[i].m_szName, (LPCTSTR)name) == 0) return i;
    }
    Check(false, name, "命令表里没有这条命令");
    return -1;
}

struct Ret { std::string text; int num; bool flag; };

// 参数按顺序压：前 textCount 个是文本，后 intCount 个是整数（末位 bool 参数也用整数槽传）
static Ret Call(int idx, const char* const* texts, int textCount, const int* ints, int intCount)
{
    MDATA_INF args[12];
    int n = 0;
    for (int k = 0; k < textCount && n < 12; ++k) {
        memset(&args[n], 0, sizeof(MDATA_INF));
        args[n].m_dtDataType = SDT_TEXT;
        args[n].m_pText = (char*)(texts[k] ? texts[k] : "");
        n++;
    }
    for (int k = 0; k < intCount && n < 12; ++k) {
        memset(&args[n], 0, sizeof(MDATA_INF));
        args[n].m_dtDataType = SDT_INT;
        args[n].m_int = ints[k];
        n++;
    }

    MDATA_INF ret;
    memset(&ret, 0, sizeof(ret));
    g_info->m_pCmdsFunc[idx](&ret, n, n ? args : NULL);

    Ret out;
    out.num = 0;
    out.flag = false;
    if (ret.m_dtDataType == SDT_TEXT && ret.m_pText) {
        out.text = ret.m_pText;
        free(ret.m_pText);
    } else if (ret.m_dtDataType == SDT_INT) {
        out.num = ret.m_int;
        char buf[16];
        sprintf(buf, "%d", ret.m_int);
        out.text = buf;
    } else if (ret.m_dtDataType == SDT_BOOL) {
        out.flag = (ret.m_bool != BL_FALSE);
        out.num = out.flag ? 1 : 0;
    }
    return out;
}

static Ret Cmd(const char* name, const std::vector<const char*>& texts, const std::vector<int>& ints)
{
    const int idx = FindCmd(name);
    if (idx < 0) { Ret e; e.num = -999; e.flag = false; return e; }
    return Call(idx, texts.empty() ? NULL : &texts[0], (int)texts.size(), ints.empty() ? NULL : &ints[0], (int)ints.size());
}

static std::string CmdText(const char* name, const char* a = NULL)
{
    std::vector<const char*> t;
    if (a) t.push_back(a);
    return Cmd(name, t, std::vector<int>()).text;
}

static int CmdInt(const char* name) { return Cmd(name, std::vector<const char*>(), std::vector<int>()).num; }
static bool CmdBool(const char* name) { return Cmd(name, std::vector<const char*>(), std::vector<int>()).flag; }

static const char* g_secret = "CXAH-DEV-SECRET-CHANGE-ME";
static int g_allowLogout = 1;   // 界面宿主默认开着，最后一轮再验证关掉的情形

static int CallInit(const char* server, const char* mac, const char* account)
{
    std::vector<const char*> t;
    t.push_back(server); t.push_back("3"); t.push_back("cxangsoft"); t.push_back(mac);
    t.push_back("20260001"); t.push_back("cccccccccccccccccccccccccccccccc"); t.push_back("12");
    t.push_back(g_secret); t.push_back(account ? account : "");
    std::vector<int> iv;
    iv.push_back(20000);
    iv.push_back(1);            // 调试模式真，便于把过程写进调试器
    if (g_allowLogout) iv.push_back(1);   // 允许退出账号：默认不给，用例里按需打开
    const Ret r = Cmd("会员_初始化", t, iv);
    return r.num;
}

// ---------------- 造合法 V2 机器码 ----------------
static std::string MakeMac(int seed)
{
    char cpu[32];
    sprintf(cpu, "000506EA%08X", (unsigned)(seed * 2654435761u));
    long long diskVal = (long long)seed * 1024 - (long long)strlen(cpu);
    if (diskVal < 0) diskVal = -diskVal;
    char seg2[32];
    sprintf(seg2, "%lld", diskVal);
    const int len = (int)(strlen(seg2) + strlen(cpu));
    char s1[8]; sprintf(s1, "%d", len); s1[2] = 0;
    char s4[8]; sprintf(s4, "%d", len);
    long long right = (long long)len * atoll(s1);
    if (right < 0) right = -right;
    char rb[24]; sprintf(rb, "%lld", right);
    const size_t rl = strlen(rb);
    char s6[16]; sprintf(s6, "%s", rl > 2 ? rb + rl - 2 : rb);
    char s7[12]; sprintf(s7, "%08X", (unsigned)(0x7654321u + seed));
    char plain[160];
    sprintf(plain, "%s-%s-%s-%s-3-%s-%s-MACVER2", s1, seg2, cpu, s4, s6, s7);

    static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string b64;
    const size_t n = strlen(plain);
    for (size_t i = 0; i < n; i += 3) {
        unsigned v = (unsigned)((unsigned char)plain[i]) << 16;
        if (i + 1 < n) v |= (unsigned)((unsigned char)plain[i + 1]) << 8;
        if (i + 2 < n) v |= (unsigned char)plain[i + 2];
        b64 += tbl[(v >> 18) & 63];
        b64 += tbl[(v >> 12) & 63];
        b64 += (i + 1 < n) ? tbl[(v >> 6) & 63] : '=';
        b64 += (i + 2 < n) ? tbl[v & 63] : '=';
    }
    return b64;
}

// ---------------- 窗口驱动 ----------------
static HWND g_main = NULL;
static HWND g_target = NULL;      // 当前要驱动的窗口：向导或小窗

static HWND Tgt() { return g_target != NULL ? g_target : g_main; }

// 只认本进程的会员窗口：机器上可能同时开着演示程序，FindWindowW 会抓到别人的窗口
static const wchar_t* g_wantCls = NULL;

static BOOL CALLBACK FindOwnProcBy(HWND h, LPARAM lp)
{
    uint32_t pid = 0;
    GetWindowThreadProcessId(h, (LPDWORD)&pid);
    if (pid != GetCurrentProcessId()) return TRUE;
    wchar_t cls[64] = {0};
    GetClassNameW(h, cls, 62);
    if (lstrcmpW(cls, g_wantCls) != 0) return TRUE;
    *(HWND*)lp = h;
    return FALSE;
}

static BOOL CALLBACK FindOwnProc(HWND h, LPARAM lp)
{
    g_wantCls = CxAhClassName;
    return FindOwnProcBy(h, lp);
}

static HWND OwnWindowOf(const wchar_t* cls)
{
    g_wantCls = cls;
    HWND found = NULL;
    EnumWindows(FindOwnProcBy, (LPARAM)&found);
    return found;
}

static HWND OwnMemberWindow()
{
    HWND found = NULL;
    EnumWindows(FindOwnProc, (LPARAM)&found);
    return found;
}

struct UiThread {
    HANDLE handle;
    unsigned id;
    volatile bool finished;
    volatile bool result;
    int page;
    bool center;      // 真=弹用户中心，假=弹向导
};

static unsigned __stdcall UiThreadProc(void* p)
{
    UiThread* t = (UiThread*)p;
    Ret r;
    if (t->center) {
        std::vector<const char*> tx;      // 邮箱留空 = 用本机保存的账号
        r = Cmd("会员_显示用户中心", tx, std::vector<int>());
    } else {
        std::vector<int> iv;
        iv.push_back(t->page);
        r = Cmd("会员_显示注册窗口", std::vector<const char*>(), iv);
    }
    t->result = r.flag;
    t->finished = true;
    return 0;
}

static bool OpenWindow(int page, UiThread* t, bool center = false)
{
    t->finished = false;
    t->result = false;
    t->page = page;
    t->center = center;
    if (center) {
        t->handle = (HANDLE)_beginthreadex(NULL, 0, UiThreadProc, t, 0, &t->id);
        for (int i = 0; i < 120; ++i) {
            g_main = OwnWindowOf(CxAhUcClassName);
            if (g_main != NULL) {
                for (int k = 0; k < 100 && GetDlgItem(g_main, IDC_BTN_UC_CLOSE) == NULL; ++k) Sleep(50);
                return true;
            }
            Sleep(50);
        }
        return false;
    }
    t->handle = (HANDLE)_beginthreadex(NULL, 0, UiThreadProc, t, 0, &t->id);
    for (int i = 0; i < 120; ++i) {
        g_main = OwnMemberWindow();
        if (g_main != NULL) {
            // 窗口句柄先于子控件存在，等控件建完再返回，避免宿主读到半初始化界面
            for (int k = 0; k < 100; ++k) {
                if (GetDlgItem(g_main, IDC_BTN_LOGIN) != NULL && GetDlgItem(g_main, IDC_BTN_REG) != NULL
                    && GetDlgItem(g_main, IDC_BTN_OK) != NULL && GetDlgItem(g_main, IDC_BTN_LOGOUT) != NULL) {
                    return true;
                }
                Sleep(50);
            }
            return true;
        }
        Sleep(50);
    }
    return false;
}

// 文字被截掉是肉眼可见的缺陷，但宿主不能靠看图判定：量一遍这段文字在这个宽度下要占多高，
// 再和控件实际高度比。说明块、提示行都用它。
static bool TextFits(HWND h)
{
    if (h == NULL) return false;
    wchar_t buf[512] = { 0 };
    if (GetWindowTextW(h, buf, 500) <= 0) return true;
    RECT rc;
    GetWindowRect(h, &rc);
    const int w = rc.right - rc.left, hgt = rc.bottom - rc.top;
    HDC dc = GetDC(h);
    HFONT f = (HFONT)SendMessageW(h, WM_GETFONT, 0, 0);
    HFONT old = f ? (HFONT)SelectObject(dc, f) : NULL;
    RECT calc = { 0, 0, w, 0 };
    DrawTextW(dc, buf, -1, &calc, DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL | DT_LEFT);
    if (old) SelectObject(dc, old);
    ReleaseDC(h, dc);
    return calc.bottom <= hgt + 1;
}

static void KillDialogs(int rounds)
{
    for (int i = 0; i < rounds; ++i) {
        HWND dlg = FindWindowExW(NULL, NULL, L"#32770", NULL);
        if (dlg == NULL) { if (i > 0) break; }
        else PostMessageW(dlg, WM_COMMAND, IDOK, 0);
        Sleep(40);
    }
}

static void CloseWindow(UiThread* t)
{
    if (g_main != NULL && IsWindow(g_main)) PostMessageW(g_main, WM_CLOSE, 0, 0);
    for (int i = 0; i < 40; ++i) {
        HWND dlg = FindWindowExW(NULL, NULL, L"#32770", NULL);
        if (dlg != NULL) PostMessageW(dlg, WM_COMMAND, IDYES, 0);
        if ((g_main == NULL || !IsWindow(g_main)) && dlg == NULL) break;
        Sleep(50);
    }
    g_main = NULL;
    if (t != NULL && t->handle != NULL) {
        WaitForSingleObject(t->handle, 3000);
        CloseHandle(t->handle);
        t->handle = NULL;
    }
}

static void Type(int id, const std::string& utf8)
{
    SetWindowTextW(GetDlgItem(Tgt(), id), WideOf(utf8).c_str());
}

// 必须用 PostMessage：点「登录」「激活序列号」会在界面线程里开出嵌套模态小窗，
// SendMessage 要等小窗关掉才返回，宿主线程就和小窗互相等死了。
static void Click(int id)
{
    PostMessageW(GetDlgItem(Tgt(), id), BM_CLICK, 0, 0);
    Sleep(120);
}

static bool HasCtl(int id)
{
    return Tgt() != NULL && GetDlgItem(Tgt(), id) != NULL;
}

// 窗口句柄在控件创建之前就能被 FindWindow 找到，先等控件就位再断言
static bool Shown(int id)
{
    const HWND h = GetDlgItem(Tgt(), id);
    return h != NULL && IsWindowVisible(h) != FALSE;
}

static bool Enabled(int id)
{
    const HWND h = GetDlgItem(Tgt(), id);
    return h != NULL && IsWindowEnabled(h) != FALSE;
}

static std::string GetUtf8(int id)
{
    const HWND h = GetDlgItem(Tgt(), id);
    if (h == NULL) return std::string();
    const int n = GetWindowTextLengthW(h);
    std::wstring w((size_t)(n > 0 ? n : 1), wchar_t(0));
    if (n > 0) GetWindowTextW(h, &w[0], n + 1);
    w.resize((size_t)n);
    return Utf8Of(w);
}

typedef bool (*Pred)();

static bool WaitFor(Pred pred, int ms)
{
    for (int waited = 0; waited < ms; waited += 100) {
        if (pred()) return true;
        Sleep(100);
    }
    return false;
}

static bool PredAuthorized() { return CmdBool("会员_是否已授权"); }
static bool PredNotAuthorized() { return !CmdBool("会员_是否已授权"); }
static bool PredWelcome() { return Shown(IDC_BTN_W_LOGIN) && Shown(IDC_BTN_W_REG); }
static bool PredLoginPage() { return Shown(IDC_BTN_LOGIN) && !Shown(IDC_BTN_REG); }
static bool PredRegisterPage() { return Shown(IDC_BTN_REG) && !Shown(IDC_BTN_LOGIN); }
static bool PredBottomRow() { return Shown(IDC_BTN_LOGOUT) && Shown(IDC_BTN_SERVICE); }
static bool PredRenewPage() { return OwnWindowOf(CxAhKeyClassName) != NULL; }
static bool PredChangeDone() {
    const HWND h = GetDlgItem(Tgt(), IDC_OKICON);
    return h != NULL && IsWindowVisible(h) != FALSE;
}

static bool PredGrantPage() { return Shown(IDC_BTN_TRIAL) && Shown(IDC_SHOP); }
static bool PredDonePage() { return Shown(IDC_BTN_OK) && Shown(IDC_DUE); }
static bool PredNeedChange() { return OwnWindowOf(CxAhKeyClassName) != NULL; }
static bool PredWrongKey() { return CmdText("会员_取服务器错误码") == "4003"; }
static bool PredRenewOk() { return CmdText("会员_取授权状态") == "ok"; }

static std::string g_clipDiag;

static std::string ReadClipboardUtf8()
{
    std::string out;
    char d[120];
    // Windows 的剪贴板历史服务会在写入后短暂占住剪贴板，读不到就等一下再试
    int tries = 0;
    while (!OpenClipboard(NULL)) {
        if (++tries > 40) {
            sprintf(d, "open=0 gle=%lu tries=%d", GetLastError(), tries);
            g_clipDiag = d;
            return out;
        }
        Sleep(50);
    }
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    sprintf(d, "open=1 h=%p", h);
    if (h != NULL) {
        const wchar_t* w = (const wchar_t*)GlobalLock(h);
        sprintf(d + strlen(d), " lock=%p len=%d", w, w ? (int)wcslen(w) : -1);
        if (w != NULL) {
            out = Utf8Of(std::wstring(w));
            GlobalUnlock(h);
        }
    } else {
        sprintf(d + strlen(d), " gle=%lu", GetLastError());
    }
    CloseClipboard();
    g_clipDiag = d;
    return out;
}

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* fne = (argc > 1) ? argv[1] : "out\\cxah.fne";
    const char* server = (argc > 2) ? argv[2] : "http://127.0.0.1:8317/api.php";
    g_secret = (argc > 3) ? argv[3] : "CXAH-DEV-SECRET-CHANGE-ME";
    const char* email = (argc > 4) ? argv[4] : "cxah-host@test.local";
    const char* keyOwn = (argc > 5) ? argv[5] : "TESTKEY-HOST-OWN";
    const char* keyNew = (argc > 6) ? argv[6] : "TESTKEY-HOST-NEW";
    const char* keyFeat = (argc > 7) ? argv[7] : "HOSTKEY-FEAT";

    printf("== UI 宿主 %s ==\n", fne);
    HMODULE mod = LoadLibraryA(fne);
    if (mod == NULL) { printf("[FAIL] 装载失败\n"); return 1; }
    typedef PLIB_INFO(WINAPI* FnGet)();
    FnGet getNewInf = (FnGet)GetProcAddress(mod, "GetNewInf");
    if (getNewInf == NULL) { printf("[FAIL] 无 GetNewInf\n"); return 1; }
    g_info = getNewInf();
    g_info->m_pfnNotify(NL_SYS_NOTIFY_FUNCTION, (DWORD)FakeNotifySys, 0);

    const std::string mac1 = MakeMac(246810);
    const std::string mac2 = MakeMac(135792);
    UiThread ui;
    memset(&ui, 0, sizeof(ui));

    printf("-- 第一轮：界面注册 + 自动试用 + 复制机器码 + 登出 --\n");
    Check(CallInit(server, mac1.c_str(), "") == 0, "初始化", CmdText("会员_取服务器文案").c_str());
    Check(!CmdText("会员_取OEM信息", "soft_name").empty(), "渠道信息已载入", NULL);

    Check(OpenWindow(0, &ui), "会员_显示注册窗口 真开出了窗口", "找不到窗口类");
    if (g_main == NULL) return 1;
    Check(WaitFor(PredWelcome, 5000), "初始页落在「登录或注册」", NULL);
    Click(IDC_BTN_W_REG);
    Check(WaitFor(PredRegisterPage, 5000), "点「注册账号」进注册页", NULL);
    Check(WaitFor(PredBottomRow, 5000), "底部退出/联系链接可见", NULL);
    Type(IDC_REG_EMAIL, email);
    Type(IDC_REG_QQ, "709005505");
    SendMessageW(GetDlgItem(g_main, IDC_REG_AGREE), BM_SETCHECK, BST_CHECKED, 0);
    Click(IDC_BTN_REG);
    KillDialogs(6);
    Check(WaitFor(PredGrantPage, 20000), "注册成功后停在「开通授权」页", CmdText("会员_取服务器错误码").c_str());
    Check(CmdText("会员_取授权状态") == "none", "注册本身不给授权", CmdText("会员_取授权状态").c_str());
    Check(CmdText("会员_取本地账号") == email, "本机账号已保存", NULL);
    Check(GetUtf8(IDC_SHOW_EMAIL) == email, "开通授权页回显注册邮箱", GetUtf8(IDC_SHOW_EMAIL).c_str());
    Click(IDC_BTN_TRIAL);
    KillDialogs(6);
    Check(WaitFor(PredAuthorized, 30000), "点「免费试用」后拿到试用授权", CmdText("会员_取服务器错误码").c_str());
    Check(WaitFor(PredDonePage, 8000), "试用成功后停在「授权完成」页", NULL);
    Check(CmdText("会员_取授权状态") == "trial", "授权状态 = trial", CmdText("会员_取授权状态").c_str());

    CloseWindow(&ui);
    Check(OpenWindow(1, &ui), "重开窗口落在登录页", NULL);
    Check(WaitFor(PredLoginPage, 5000), "登录页控件齐了", NULL);
    Check(Shown(IDC_BTN_FREE) && Shown(IDC_BTN_AGREE), "免费注册/许可协议入口可见", NULL);
    Check(Shown(IDC_TERMS) && Shown(IDC_MAC), "说明块与机器码框可见", NULL);
    Click(IDC_BTN_COPYMAC);
    const std::string clip = ReadClipboardUtf8();
    { char d[220]; sprintf(d, "clip=%d want=%d %s", (int)clip.size(), (int)mac1.size(), g_clipDiag.c_str()); Check(clip == mac1, "复制机器码写进剪贴板", d); }

    Click(IDC_BTN_LOGOUT);
    Check(WaitFor(PredNotAuthorized, 8000), "界面退出本机账号后失权", NULL);
    CloseWindow(&ui);

    printf("-- 第二轮：机器码不符 → 自动切换机页 → 序列号验证换机 --\n");
    Check(CallInit(server, mac2.c_str(), email) == 0, "换到新电脑重新初始化", NULL);
    Check(OpenWindow(1, &ui), "再次打开窗口", NULL);
    Type(IDC_LOGIN_EMAIL, email);
    SendMessageW(GetDlgItem(g_main, IDC_LOGIN_AGREE), BM_SETCHECK, BST_CHECKED, 0);
    Click(IDC_BTN_LOGIN);
    KillDialogs(6);
    Check(WaitFor(PredNeedChange, 30000), "机器码不符自动弹换机小窗", CmdText("会员_取服务器错误码").c_str());
    g_target = OwnWindowOf(CxAhKeyClassName);
    Check(g_target != NULL, "换机小窗句柄已拿到", NULL);
    const std::string note = GetUtf8(IDC_RENEW_NOTE);
    Check(note.find(Utf8Of(WideOfLocal("另一台机器"))) != std::string::npos, "说明写明账号绑在另一台机器", note.c_str());
    Check(note.find(email) != std::string::npos, "说明里带上是哪个账号", note.c_str());
    Check(note.find(Utf8Of(WideOfLocal("保存后再更换"))) != std::string::npos, "提醒先保存采集数据", note.c_str());
    // 换机次数只在验证之后出现；不能拿「note 里没有 5」当判据——测试邮箱本身就带数字。
    Check(note.find(Utf8Of(WideOfLocal("次"))) == std::string::npos && GetUtf8(IDC_RENEW_TIP).empty(),
        "初次打开不显示可换次数", note.c_str());

    Check(TextFits(GetDlgItem(g_target, IDC_RENEW_NOTE)), "换机说明文字完整可见（没被截行）", NULL);

    Type(IDC_RENEW_KEY, "SMOKE-NOPE-NOPE-NOPE");
    Check(GetUtf8(IDC_RENEW_KEY) == "SMOKE-NOPE-NOPE-NOPE", "换机输入框可写入", GetUtf8(IDC_RENEW_KEY).c_str());
    Click(IDC_BTN_ACT);
    KillDialogs(6);
    WaitFor(PredWrongKey, 15000);
    Check(!PredAuthorized(), "错序列号后仍未拿到授权", CmdText("会员_取服务器错误码").c_str());
    Check(CmdInt("会员_取剩余换机次数") == 5, "错序列号不扣换机次数", NULL);
    Check(Shown(IDC_RENEW_TIP) && !GetUtf8(IDC_RENEW_TIP).empty(), "小窗里给出错误提示且窗口不关", GetUtf8(IDC_RENEW_TIP).c_str());
    {
        const std::string t = GetUtf8(IDC_RENEW_TIP);
        Check(TextFits(GetDlgItem(g_target, IDC_RENEW_TIP)), "错误提示行也没被截", NULL);
        Check(t.find(Utf8Of(WideOfLocal("还剩"))) != std::string::npos && t.find("5") != std::string::npos,
              "验证失败后才显示剩余次数", t.c_str());
    }
    Check(CmdText("会员_取授权信息", "mac_key") != mac2 || !PredAuthorized(), "错序列号没有把机器码换过去", NULL);

    Type(IDC_RENEW_KEY, keyOwn);
    Click(IDC_BTN_ACT);
    KillDialogs(6);
    Check(WaitFor(PredChangeDone, 30000), "换机成功后留在成功页", CmdText("会员_取服务器错误码").c_str());
    Check(GetUtf8(IDC_TITLE_RENEW) == Utf8Of(WideOfLocal("换机成功")), "成功页标题", GetUtf8(IDC_TITLE_RENEW).c_str());
    {
        const std::string t = GetUtf8(IDC_RENEW_TIP);
        Check(t.find(Utf8Of(WideOfLocal("剩余换机次数"))) != std::string::npos && t.find("4") != std::string::npos,
              "成功页写明剩余换机次数", t.c_str());
    }
    Check(!Shown(IDC_RENEW_KEY) && Shown(IDC_OKICON) && Shown(IDC_BTN_ACT), "成功页收起表单只留完成", NULL);
    Click(IDC_BTN_ACT);
    for (int i = 0; i < 60 && OwnWindowOf(CxAhKeyClassName) != NULL; ++i) Sleep(50);
    Check(OwnWindowOf(CxAhKeyClassName) == NULL, "点完成后小窗关闭", NULL);
    g_target = NULL;
    Check(WaitFor(PredAuthorized, 30000), "本账号序列号换机并登录", (CmdText("会员_取服务器错误码") + " / " + CmdText("会员_取服务器文案")).c_str());
    Check(CmdInt("会员_取剩余换机次数") == 4, "换机次数扣到 4", NULL);
    Check(CmdText("会员_取授权信息", "mac_key") == mac2, "机器码已落到新电脑", NULL);

    printf("-- 第三轮：激活页续费序列号 --\n");
    const std::string dueBefore = CmdText("会员_取到期时间");
    CloseWindow(&ui);
    Check(CallInit(server, mac2.c_str(), email) == 0, "重新初始化", NULL);
    Check(OpenWindow(3, &ui), "打开「开通授权」页", NULL);
    Check(WaitFor(PredGrantPage, 5000), "授权页可见", NULL);
    Click(IDC_BTN_RENEW);
    Check(WaitFor(PredRenewPage, 8000), "点「激活序列号」弹出独立小窗", NULL);
    g_target = OwnWindowOf(CxAhKeyClassName);
    Type(IDC_RENEW_KEY, keyNew);
    Click(IDC_BTN_ACT);
    KillDialogs(6);
    for (int i = 0; i < 60 && OwnWindowOf(CxAhKeyClassName) != NULL; ++i) Sleep(50);
    g_target = NULL;
    Check(WaitFor(PredRenewOk, 30000), "激活后转为正式授权", CmdText("会员_取服务器文案").c_str());
    Check(WaitFor(PredDonePage, 8000), "小窗关掉后向导停在授权成功页", NULL);
    Check(CmdText("会员_取到期时间") != dueBefore, "到期时间已顺延", NULL);
    Check(CmdText("会员_取授权信息", "order_number") == keyNew, "最后序列号已记录", NULL);

    Click(IDC_BTN_OK);
    for (int i = 0; i < 60 && !ui.finished; ++i) Sleep(50);
    Check(ui.finished, "点「进入软件」后窗口线程返回", NULL);
    Check(ui.result, "窗口返回已授权", NULL);

    printf("-- 第四轮：用户中心 --\n");
    {
        const char* act[] = { email, keyFeat, "", "" };
        const std::string r = Cmd("会员_激活序列号", std::vector<const char*>(act, act + 4), std::vector<int>()).text;
        Check(r == "0", "先激活选配功能序列号", r.c_str());
    }
    UiThread uc;
    memset(&uc, 0, sizeof(uc));
    Check(OpenWindow(0, &uc, true), "会员_显示用户中心 真开出了窗口", "找不到用户中心窗口类");
    if (g_main == NULL) return 1;
    Check(GetUtf8(IDC_UC_ACCOUNT) == email, "用户中心显示登录账号", GetUtf8(IDC_UC_ACCOUNT).c_str());
    Check(GetUtf8(IDC_UC_MAC) == mac2, "用户中心显示本机机器码", GetUtf8(IDC_UC_MAC).c_str());
    Check(!GetUtf8(IDC_UC_DUE).empty() && GetUtf8(IDC_UC_DUE) != "未开通", "显示到期时间", GetUtf8(IDC_UC_DUE).c_str());
    Check(GetUtf8(IDC_UC_FEATURES).find(Utf8Of(WideOfLocal("跟卖监控"))) != std::string::npos, "列出已开通选配功能",
          GetUtf8(IDC_UC_FEATURES).c_str());
    Check(GetUtf8(IDC_UC_STATE).find(Utf8Of(WideOfLocal("正常"))) != std::string::npos, "状态行显示授权正常",
          GetUtf8(IDC_UC_STATE).c_str());
    Check(Shown(IDC_BTN_UC_BUY) && Shown(IDC_BTN_UC_ACT) && Shown(IDC_BTN_UC_REFRESH), "三个动作按钮都在", NULL);

    Click(IDC_BTN_UC_REFRESH);
    for (int i = 0; i < 60 && GetUtf8(IDC_UC_TIP).empty(); ++i) Sleep(50);
    Check(!GetUtf8(IDC_UC_TIP).empty(), "刷新授权给出结果提示", GetUtf8(IDC_UC_TIP).c_str());

    Click(IDC_BTN_UC_ACT);
    Check(WaitFor(PredRenewPage, 8000), "用户中心里点激活会弹小窗", NULL);
    g_target = OwnWindowOf(CxAhKeyClassName);
    Click(IDC_BTN_RENEW_BACK);
    for (int i = 0; i < 60 && OwnWindowOf(CxAhKeyClassName) != NULL; ++i) Sleep(50);
    Check(OwnWindowOf(CxAhKeyClassName) == NULL, "小窗取消后关掉，用户中心还在", NULL);
    g_target = NULL;
    Check(OwnWindowOf(CxAhUcClassName) != NULL, "用户中心窗口没被误关", NULL);

    Click(IDC_BTN_UC_COPY);
    const std::string clip2 = ReadClipboardUtf8();
    Check(clip2 == mac2, "用户中心复制机器码", clip2.c_str());

    Click(IDC_BTN_UC_CLOSE);
    for (int i = 0; i < 60 && !uc.finished; ++i) Sleep(50);
    Check(uc.finished, "点「关闭」后用户中心线程返回", NULL);
    Check(uc.result, "关闭时账号仍可用", NULL);

    printf("-- 第五轮：退出本机账号按钮默认隐藏 --\n");
    g_allowLogout = 0;
    Check(CallInit(server, mac2.c_str(), email) == 0, "关掉开关重新初始化", NULL);
    UiThread lo;
    memset(&lo, 0, sizeof(lo));
    Check(OpenWindow(1, &lo), "重新打开登录页", NULL);
    if (g_main == NULL) return 1;
    Check(WaitFor(PredLoginPage, 5000), "登录页可见", NULL);
    Check(!Shown(IDC_BTN_LOGOUT), "没开开关就不显示退出本机账号", NULL);
    Check(Shown(IDC_BTN_SERVICE), "联系方式仍然显示", NULL);
    CloseWindow(&lo);

    g_allowLogout = 1;
    Check(CallInit(server, mac2.c_str(), email) == 0, "打开开关重新初始化", NULL);
    memset(&lo, 0, sizeof(lo));
    Check(OpenWindow(1, &lo), "再次打开登录页", NULL);
    Check(WaitFor(PredLoginPage, 5000), "登录页可见", NULL);
    Check(Shown(IDC_BTN_LOGOUT), "开了开关才显示退出本机账号", NULL);
    CloseWindow(&lo);

    printf("-- 第六轮：账号被别的电脑顶掉且宿主没接管 → 库自动提醒 --\n");
    {
        // 先在新电脑上正常登录，拿到可用授权
        Check(CallInit(server, mac2.c_str(), email) == 0, "顶号前重新初始化", NULL);
        const std::string lg = CmdText("会员_登录", email);
        Check(lg == "0", "顶号前先正常登录", lg.c_str());
        Check(PredAuthorized(), "顶号前本机可用", NULL);

        // 换回旧机器码登录：服务端说账号绑在另一台，宿主没登记回调，库要自己提醒
        Check(CallInit(server, mac1.c_str(), email) == 0, "换回旧机器码初始化", NULL);
        const std::string hit = CmdText("会员_登录", email);
        Check(hit == "4001", "旧机器码登录返回 4001", hit.c_str());
        Check(CmdText("会员_取授权状态") == "revoked", "本机授权被收回", CmdText("会员_取授权状态").c_str());
        Check(CmdInt("会员_取错误类别") == 6, "类别是可换机", NULL);

        HWND hAlert = NULL;
        for (int k = 0; k < 60 && hAlert == NULL; ++k) {
            hAlert = FindWindowW(NULL, WideOfLocal("创想软件授权提醒").c_str());
            if (hAlert == NULL) Sleep(50);
        }
        Check(hAlert != NULL, "没接管时库弹出更换到其他电脑的提醒", NULL);
        // 消息框正文的子类名不一定还是 Static，遍历所有子窗口取最长的那段文本
        std::wstring alertText;
        for (HWND c = FindWindowExW(hAlert, NULL, NULL, NULL); c != NULL; c = FindWindowExW(hAlert, c, NULL, NULL)) {
            wchar_t buf[512] = { 0 };
            const LRESULT n = SendMessageW(c, WM_GETTEXT, 500, (LPARAM)buf);
            if (n > 0 && (size_t)n > alertText.size()) alertText.assign(buf, (size_t)n);
        }
        Check(alertText.find(L"其他电脑") != std::wstring::npos, "提醒文案说清是账号换了电脑", Utf8Of(alertText).c_str());
        Check(alertText.find(L"重新登录") != std::wstring::npos, "提醒里给了下一步怎么做", NULL);

        // 直接关掉这张框，别用 KillDialogs：桌面上同时还有别的 #32770，会错杀
        PostMessageW(hAlert, WM_CLOSE, 0, 0);
        bool closed = false;
        for (int k = 0; k < 60; ++k) {
            if (!IsWindow(hAlert)) { closed = true; break; }
            Sleep(50);
        }
        Check(closed, "提醒框已关闭", NULL);
        Check(!PredAuthorized(), "提醒过后本机依旧不可用", NULL);
    }

    printf("\n通过 %d 项，失败 %d 项\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
