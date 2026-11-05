// 创想AH会员框架 · 注册登录界面
// 版面：顶部品牌横幅（素材取自旧版窗口）+ 白色卡片式向导页 + 底部文字链接。
// 全部控件自绘或自绘配色，不依赖宿主是否带 comctl32 v6 清单，外观在各宿主一致。
#include "cx_ui.h"
#include "cx_core.h"
#include "cx_ui_ids.h"
#include "cx_assets.h"

#include <windowsx.h>
#include <stdio.h>
#include <string.h>
#include <vector>

namespace cx {

std::string GetTextOf(HWND h);      // 供命令层复用（同文件内实现）

namespace {

// 序列号激活 / 换机小窗口，向导与命令层都会用到
std::string ShowKeyDialog(const std::string& email, HWND owner, bool forChange);
LRESULT CALLBACK UcProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool EnsureUcClass();

// ==================== 主题 ====================
const int CX_W = CXAH_W, CX_H = CXAH_H;
const int CX_BANNER_H = CXAH_BANNER_H;
const int CX_PX = CXAH_PX, CX_PY = CXAH_PY;
const int CX_PW = CXAH_PW;
const int CX_CARD_X = 16, CX_CARD_Y = 116, CX_CARD_R = 640, CX_CARD_B = 502;

const COLORREF CX_BG = RGB(244, 246, 248);
const COLORREF CX_SURFACE = RGB(255, 255, 255);
const COLORREF CX_MUTED = RGB(248, 250, 252);
const COLORREF CX_LINE = RGB(225, 229, 234);
const COLORREF CX_LINE_INPUT = RGB(211, 216, 224);
const COLORREF CX_TEXT = RGB(31, 41, 51);
const COLORREF CX_SUB = RGB(107, 118, 131);
const COLORREF CX_ACCENT = RGB(22, 119, 255);
const COLORREF CX_ACCENT_DOWN = RGB(14, 95, 216);
const COLORREF CX_DANGER = RGB(217, 48, 38);
const COLORREF CX_BTN_LINE = RGB(213, 218, 225);
const COLORREF CX_BTN_FILL = RGB(253, 253, 253);
const COLORREF CX_BTN_DOWN = RGB(236, 241, 248);
const COLORREF CX_DISABLED_TEXT = RGB(184, 191, 199);

const int CX_BTN_H = 38;        // 按钮高度
const int CX_ROW_Y = 322;       // 每页底部按钮行

enum Ink { INK_TEXT, INK_SUB, INK_ACCENT, INK_DANGER };
enum Kind { K_PLAIN, K_SECONDARY, K_PRIMARY, K_LINK };
enum Paper { PAP_SURFACE, PAP_MUTED };

struct Style {
    int id;
    Ink ink;
    Paper paper;
    Kind kind;
};

struct Frame {           // 输入框的圆角外框，由父窗口统一绘制
    int id;
    int page;
    RECT rc;
};

// ==================== GDI+ ====================
typedef void* GpToken;
typedef void* GpBitmap;
typedef void* GpImage;
typedef void* GpGraphics;

struct GpPlusApi {
    HMODULE mod;
    HMODULE modOle;
    int (__stdcall *Startup)(GpToken*, const void*, void*);
    int (__stdcall *CreateBitmapFromStream)(void*, GpBitmap**);
    int (__stdcall *CreateGraphicsFromHDC)(HDC, GpGraphics**);
    int (__stdcall *DrawImage)(GpGraphics*, GpImage*, INT, INT);
    int (__stdcall *DrawImageRect)(GpGraphics*, GpImage*, INT, INT, INT, INT);
    int (__stdcall *DisposeImage)(GpImage*);
    int (__stdcall *DeleteGraphics)(GpGraphics*);
    long (__stdcall *CreateStreamOnHGlobal)(HGLOBAL, BOOL, void**);
    bool ready;
    GpPlusApi() : mod(NULL), modOle(NULL), Startup(NULL), CreateBitmapFromStream(NULL),
                  CreateGraphicsFromHDC(NULL), DrawImage(NULL), DrawImageRect(NULL), DisposeImage(NULL),
                  DeleteGraphics(NULL), CreateStreamOnHGlobal(NULL), ready(false) {}
    void load()
    {
        if (mod != NULL) return;
        mod = LoadLibraryA("gdiplus.dll");
        modOle = LoadLibraryA("ole32.dll");
        if (mod == NULL || modOle == NULL) return;
        *(void**)&Startup = GetProcAddress(mod, "GdiplusStartup");
        *(void**)&CreateBitmapFromStream = GetProcAddress(mod, "GdipCreateBitmapFromStream");
        *(void**)&CreateGraphicsFromHDC = GetProcAddress(mod, "GdipCreateFromHDC");
        *(void**)&DrawImage = GetProcAddress(mod, "GdipDrawImageI");
        *(void**)&DrawImageRect = GetProcAddress(mod, "GdipDrawImageRectI");
        *(void**)&DisposeImage = GetProcAddress(mod, "GdipDisposeImage");
        *(void**)&DeleteGraphics = GetProcAddress(mod, "GdipDeleteGraphics");
        *(void**)&CreateStreamOnHGlobal = GetProcAddress(modOle, "CreateStreamOnHGlobal");
        ready = Startup && CreateBitmapFromStream && CreateGraphicsFromHDC && DrawImage && CreateStreamOnHGlobal;
        if (ready) {
            struct Input { unsigned long version; unsigned long count; void* event; void* external; } input;
            memset(&input, 0, sizeof(input));
            input.version = 1;
            static GpToken token = NULL;
            Startup(&token, &input, NULL);
        }
    }
};

GpPlusApi g_gp;
GpBitmap* g_banner = NULL;
GpBitmap* g_okIcon = NULL;
GpBitmap* g_spin = NULL;

const wchar_t* CX_CURSOR_WAIT = (const wchar_t*)(UINT_PTR)32514;
const wchar_t* CX_CURSOR_ARROW = (const wchar_t*)(UINT_PTR)32512;

void ReleaseComObject(void* p)
{
    if (p == NULL) return;
    typedef long (__stdcall *PFN_Release)(void*);
    void** vtable = *(void***)p;
    PFN_Release release = (PFN_Release)vtable[2];   // QueryInterface / AddRef / Release
    if (release != NULL) release(p);
}

GpBitmap* BitmapFromBase64(const std::string& base64Png)
{
    g_gp.load();
    if (!g_gp.ready || base64Png.empty()) return NULL;
    std::vector<unsigned char> raw = Base64Decode(base64Png);
    if (raw.empty()) return NULL;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, raw.size());
    if (hMem == NULL) return NULL;
    memcpy(GlobalLock(hMem), &raw[0], raw.size());
    GlobalUnlock(hMem);
    void* stream = NULL;
    if (g_gp.CreateStreamOnHGlobal(hMem, TRUE, &stream) != 0 || stream == NULL) {
        GlobalFree(hMem);
        return NULL;
    }
    GpBitmap* bmp = NULL;
    if (g_gp.CreateBitmapFromStream(stream, &bmp) != 0) bmp = NULL;
    ReleaseComObject(stream);
    return bmp;
}

void DrawBitmap(GpBitmap* bmp, HDC dc, int x, int y, int w, int h)
{
    if (bmp == NULL || !g_gp.ready) return;
    GpGraphics* graphics = NULL;
    if (g_gp.CreateGraphicsFromHDC(dc, &graphics) != 0) return;
    if (w > 0 && h > 0) g_gp.DrawImageRect(graphics, (GpImage*)bmp, x, y, w, h);
    else g_gp.DrawImage(graphics, (GpImage*)bmp, x, y);
    if (g_gp.DeleteGraphics) g_gp.DeleteGraphics(graphics);
}

void DrawPngScaled(HDC dc, int x, int y, int w, int h, const std::string& base64Png)
{
    GpBitmap* bmp = BitmapFromBase64(base64Png);
    DrawBitmap(bmp, dc, x, y, w, h);
    if (bmp != NULL && g_gp.DisposeImage) g_gp.DisposeImage((GpImage*)bmp);
}

// ==================== 窗口状态 ====================

struct Win {
    HWND hMain;
    HFONT font, fontSmall, fontTitle, fontBtn;
    HBRUSH brBg, brSurface, brMuted;
    std::vector<Style> styles;
    std::vector<Frame> frames;
    std::vector<int> pageIds[PG_COUNT];
    int page;
    int ox, oy;             // 内容原点：向导卡片内与小窗口不同
    bool authorized;
    bool withNav;         // 底部链接行是否显示
    bool busy;
    bool doneView;      // 换机小窗已切到成功页
    std::string pics[3];  // 0 注册 1 序列号小窗
    std::wstring title;
    std::string dlgEmail;   // 小窗口：要激活/换机的账号
    bool dlgChange;         // 小窗口：真是换机验证而不是序列号激活
    int keyDY;              // 小窗口：说明块撑高后，下面所有控件要往下挪的量
};

Win* g_win = NULL;
DWORD g_clipErr = 0;

std::wstring U(const std::string& s) { return WideFromUtf8(s); }

std::string GetTextOf(HWND h)
{
    if (h == NULL) return std::string();
    const int n = GetWindowTextLengthW(h);
    if (n <= 0) return std::string();
    std::wstring w((size_t)n + 1, L'\0');
    GetWindowTextW(h, &w[0], n + 1);
    w.resize((size_t)n);
    return Utf8FromWide(w);
}

std::string SelText(HWND combo)
{
    if (combo == NULL) return std::string();
    const int sel = (int)SendMessageW(combo, CB_GETCURSEL, 0, 0);
    if (sel < 0) return std::string();
    const int n = (int)SendMessageW(combo, CB_GETLBTEXTLEN, sel, 0);
    if (n <= 0) return std::string();
    std::wstring w((size_t)n + 1, L'\0');
    SendMessageW(combo, CB_GETLBTEXT, sel, (LPARAM)&w[0]);
    w.resize((size_t)n);
    return Utf8FromWide(w);
}

std::wstring SoftName()
{
    const std::string& n = S().oem.softName;
    return U(n.empty() ? "本软件" : n);
}

std::wstring Fill(std::wstring text)
{
    size_t p = 0;
    const std::wstring name = SoftName();
    while ((p = text.find(L"{ProgramerName}", p)) != std::wstring::npos) {
        text.replace(p, 15, name);
        p += name.size();
    }
    const std::wstring site = U(S().oem.home);
    p = 0;
    while ((p = text.find(L"{site}", p)) != std::wstring::npos) {
        text.replace(p, 6, site);
        p += site.size();
    }
    return text;
}

const Style* FindStyle(Win* win, int id)
{
    for (size_t i = 0; i < win->styles.size(); ++i) {
        if (win->styles[i].id == id) return &win->styles[i];
    }
    return NULL;
}

HBRUSH BrushFor(Win* win, Paper p)
{
    return p == PAP_MUTED ? win->brMuted : win->brSurface;
}

COLORREF InkOf(Ink ink)
{
    if (ink == INK_SUB) return CX_SUB;
    if (ink == INK_ACCENT) return CX_ACCENT;
    if (ink == INK_DANGER) return CX_DANGER;
    return CX_TEXT;
}

void Register(Win* win, int id, int page, Ink ink, Paper paper, Kind kind)
{
    Style st;
    st.id = id;
    st.ink = ink;
    st.paper = paper;
    st.kind = kind;
    win->styles.push_back(st);
    if (page >= 0 && page < PG_COUNT) win->pageIds[page].push_back(id);
}

// 建控件：坐标一律是卡片内容区坐标，页号决定随哪一页显隐
HWND Make(Win* win, int page, HFONT font, const wchar_t* cls, const wchar_t* text, DWORD style,
          int x, int y, int w, int h, int id, Ink ink, Paper paper, Kind kind)
{
    HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, win->ox + x, win->oy + y, w, h,
                             win->hMain, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    if (c == NULL) return NULL;
    if (font != NULL) SendMessageW(c, WM_SETFONT, (WPARAM)font, TRUE);
    Register(win, id, page, ink, paper, kind);
    return c;
}

HWND Label(Win* win, int page, const wchar_t* text, int x, int y, int w, int h, int id,
           Ink ink = INK_TEXT, HFONT font = NULL)
{
    return Make(win, page, font == NULL ? win->font : font, L"STATIC", text, SS_LEFT, x, y, w, h, id,
                ink, PAP_SURFACE, K_PLAIN);
}

// 圆角外框 + 无边框编辑框：外框由父窗口画，视觉上是现代输入框
HWND Input(Win* win, int page, const wchar_t* text, int x, int y, int w, int h, int id,
           DWORD extra = ES_AUTOHSCROLL, Paper paper = PAP_SURFACE, Ink ink = INK_TEXT)
{
    Frame fr;
    fr.id = id;
    fr.page = page;
    fr.rc.left = win->ox + x;
    fr.rc.top = win->oy + y;
    fr.rc.right = fr.rc.left + w;
    fr.rc.bottom = fr.rc.top + h;
    win->frames.push_back(fr);
    return Make(win, page, win->font, L"EDIT", text, extra, x + 10, y + (h - 18) / 2, w - 20, 18, id,
                ink, paper, K_PLAIN);
}

HWND Btn(Win* win, int page, const wchar_t* text, int x, int y, int w, int h, int id, Kind kind,
         HFONT font = NULL)
{
    return Make(win, page, font == NULL ? win->fontBtn : font, L"BUTTON", text,
                BS_OWNERDRAW | (kind == K_LINK ? 0 : WS_TABSTOP), x, y, w, h, id,
                kind == K_LINK ? INK_ACCENT : INK_TEXT, PAP_SURFACE, kind);
}

HWND Check(Win* win, int page, const wchar_t* text, int x, int y, int w, int id)
{
    return Make(win, page, win->font, L"BUTTON", text, BS_AUTOCHECKBOX | WS_TABSTOP, x, y, w, 22, id,
                INK_TEXT, PAP_SURFACE, K_PLAIN);
}

void SetTip(Win* win, int id, const std::wstring& text)
{
    HWND h = GetDlgItem(win->hMain, id);
    if (h != NULL) SetWindowTextW(h, text.c_str());
}

void SetTipU(Win* win, int id, const std::string& utf8) { SetTip(win, id, U(utf8)); }

// 接口返回的文案优先，空则用本地错误码说明
std::wstring LastText(const std::string& code)
{
    if (!S().lastMsg.empty()) return U(S().lastMsg);
    return U(LocalCodeText(code));
}

void ShowGrant(Win* win)
{
    State& st = S();
    // 这里显示的是本机用户自己刚注册的邮箱，要让他看清并记住，不打码
    std::wstring email = st.grant.email.empty() ? U(LoadAccount()) : U(st.grant.email);
    const std::string typed = GetTextOf(GetDlgItem(win->hMain, IDC_LOGIN_EMAIL));
    if (email.empty() && !typed.empty()) email = U(typed);
    SetTip(win, IDC_SHOW_EMAIL, email);
    SetTip(win, IDC_SHOW_MAC, U(st.macKey));
}

void FillDue(Win* win)
{
    State& st = S();
    std::wstring due = st.grant.dueDate.empty() ? L"未开通" : U(st.grant.dueDate);
    if (st.grant.isTrial) due += L"（试用）";
    SetTip(win, IDC_DUE, L"授权有效期至 " + due);
}

void RefreshPic(Win* win, int slot, int picId, int tipId)
{
    std::string id, png;
    if (!FetchCaptcha(&id, &png)) {
        SetTipU(win, tipId, S().lastMsg.empty() ? GbkToUtf8("验证码获取失败，请点击图片重试。") : S().lastMsg);
        return;
    }
    win->pics[slot] = png;
    HWND h = GetDlgItem(win->hMain, picId);
    if (h != NULL) InvalidateRect(h, NULL, TRUE);
}

void ShowPage(Win* win, int page)
{
    win->page = page;
    for (int g = 0; g < PG_COUNT; ++g) {
        const BOOL show = (g == page) ? TRUE : FALSE;
        for (size_t i = 0; i < win->pageIds[g].size(); ++i) {
            ShowWindow(GetDlgItem(win->hMain, win->pageIds[g][i]), show);
        }
    }
    const BOOL nav = win->withNav ? TRUE : FALSE;
    ShowWindow(GetDlgItem(win->hMain, IDC_BTN_TUTORIAL), nav);
    ShowWindow(GetDlgItem(win->hMain, IDC_BTN_HELP), nav);
    // 退出账号默认不给，初始化里显式开了才出现
    ShowWindow(GetDlgItem(win->hMain, IDC_BTN_LOGOUT),
               (nav && S().allowLogout) ? TRUE : FALSE);
    ShowWindow(GetDlgItem(win->hMain, IDC_BTN_SERVICE), nav);
    const BOOL busy = (win->busy && page == PG_REGISTER) ? TRUE : FALSE;
    ShowWindow(GetDlgItem(win->hMain, IDC_BUSY), busy);
    ShowWindow(GetDlgItem(win->hMain, IDC_SPIN), busy);
    ShowWindow(GetDlgItem(win->hMain, IDC_BUSY_TEXT), busy);
    if (page == PG_GRANT) ShowGrant(win);
    if (page == PG_DONE) FillDue(win);
    if (page == PG_REGISTER && win->pics[0].empty()) RefreshPic(win, 0, IDC_REG_PIC, IDC_REG_TIP);
    InvalidateRect(win->hMain, NULL, TRUE);
}

void OnAuthorized(Win* win)
{
    win->authorized = true;
    ShowPage(win, PG_DONE);
}

// 别的进程正占着剪贴板时 OpenClipboard 会失败，换 NULL 属主重试一次
bool CopyToClipboard(HWND hOwner, const std::wstring& text)
{
    g_clipErr = 0;
    if (!OpenClipboard(hOwner) && !OpenClipboard(NULL)) {
        g_clipErr = GetLastError();
        return false;
    }
    EmptyClipboard();
    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hMem != NULL) {
        memcpy(GlobalLock(hMem), text.c_str(), bytes);
        GlobalUnlock(hMem);
        if (SetClipboardData(CF_UNICODETEXT, hMem) == NULL) g_clipErr = GetLastError();
    }
    CloseClipboard();
    return g_clipErr == 0;
}

void OpenUrl(const std::string& url)
{
    if (url.empty()) return;
    // shell32 运行期解析：静态编译时不必往依赖表里加库，也少一个装载失败点
    typedef HINSTANCE (WINAPI *PFN_ShellExecute)(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);
    static PFN_ShellExecute shellExecute = NULL;
    if (shellExecute == NULL) {
        HMODULE mod = LoadLibraryA("shell32.dll");
        if (mod == NULL) return;
        shellExecute = (PFN_ShellExecute)GetProcAddress(mod, "ShellExecuteW");
        if (shellExecute == NULL) return;
    }
    const std::wstring w = U(url);
    shellExecute(NULL, L"open", w.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

// 当前要操作的账号：界面输入优先，其次已载入的授权与本地保存值
std::string CurrentEmail(Win* win)
{
    std::string e = TrimStr(GetTextOf(GetDlgItem(win->hMain, IDC_LOGIN_EMAIL)));
    if (e.empty()) e = TrimStr(GetTextOf(GetDlgItem(win->hMain, IDC_SHOW_EMAIL)));
    if (e.empty()) e = S().email;
    if (e.empty()) e = LoadAccount();
    return e;
}

std::string ShopUrl()
{
    if (!S().oem.shopHome.empty()) return S().oem.shopHome;
    if (!S().oem.buyText.empty()) return S().oem.buyText;
    return S().oem.home;
}

std::string AgreementUrl()
{
    if (!S().oem.changeHome.empty() && S().oem.home.empty()) return S().oem.changeHome;
    return S().oem.home.empty() ? std::string() : S().oem.home + "/user_agreement.php";
}

// ==================== 业务动作 ====================

void RunLogin(Win* win)
{
    if (SendMessageW(GetDlgItem(win->hMain, IDC_LOGIN_AGREE), BM_GETCHECK, 0, 0) != BST_CHECKED) {
        SetTip(win, IDC_LOGIN_TIP, L"请先勾选同意用户许可协议和本软件使用许可。");
        return;
    }
    const std::string email = TrimStr(GetTextOf(GetDlgItem(win->hMain, IDC_LOGIN_EMAIL)));
    if (email.empty()) { SetTip(win, IDC_LOGIN_TIP, L"请输入注册软件时的邮箱账号"); return; }
    SetTip(win, IDC_LOGIN_TIP, L"正在登录…");
    SetCursor(LoadCursorW(NULL, CX_CURSOR_WAIT));
    const std::string code = DoLogin(email);
    SetCursor(LoadCursorW(NULL, CX_CURSOR_ARROW));

    if (code == "0") { OnAuthorized(win); return; }
    if (code == "4001") {
        // 机器码不符就是「登录不上」，直接弹换机验证窗；关掉后停在登录页
        const std::string done = ShowKeyDialog(email, win->hMain, true);
        if (done == "0") OnAuthorized(win);
        else if (done == "4001") SetTip(win, IDC_LOGIN_TIP, LastText("4001") + L"　点「登录」可重新打开换机验证。");
        else SetTip(win, IDC_LOGIN_TIP, LastText(done));
        return;
    }
    SetTip(win, IDC_LOGIN_TIP, LastText(code));
    if (code == "2001") {
        SetTip(win, IDC_LOGIN_TIP, LastText(code) + L"　没有账号可点「免费注册账号」。");
    } else if (code == "4002") {
        // 换机次数用完，只能人工处理，提示里带上客服号
        std::wstring box = LastText(code);
        if (!S().oem.qq.empty()) box += L"　客服QQ：" + U(S().oem.qq);
        if (!S().oem.qq2.empty()) box += L"（备用 " + U(S().oem.qq2) + L"）";
        MessageBoxW(win->hMain, box.c_str(), win->title.c_str(), MB_OK | MB_ICONINFORMATION);
    }
}

void RunRegister(Win* win)
{
    if (SendMessageW(GetDlgItem(win->hMain, IDC_REG_AGREE), BM_GETCHECK, 0, 0) != BST_CHECKED) {
        SetTip(win, IDC_REG_TIP, L"请先勾选同意用户协议和软件使用许可。");
        return;
    }
    const std::string email = GetTextOf(GetDlgItem(win->hMain, IDC_REG_EMAIL));
    win->busy = true;
    ShowPage(win, PG_REGISTER);
    SetTip(win, IDC_REG_TIP, L"");
    UpdateWindow(win->hMain);
    SetCursor(LoadCursorW(NULL, CX_CURSOR_WAIT));
    const std::string code = DoRegister(email,
                                        GetTextOf(GetDlgItem(win->hMain, IDC_REG_QQ)),
                                        GetTextOf(GetDlgItem(win->hMain, IDC_REG_REF)),
                                        GetTextOf(GetDlgItem(win->hMain, IDC_REG_PHONE)),
                                        SelText(GetDlgItem(win->hMain, IDC_REG_LEARN)),
                                        "", S().captchaId,
                                        GetTextOf(GetDlgItem(win->hMain, IDC_REG_CODE)));
    SetCursor(LoadCursorW(NULL, CX_CURSOR_ARROW));
    win->busy = false;
    if (code != "0") {
        ShowPage(win, PG_REGISTER);
        SetTip(win, IDC_REG_TIP, LastText(code));
        RefreshPic(win, 0, IDC_REG_PIC, IDC_REG_TIP);
        return;
    }
    const std::string mail = LowerStr(TrimStr(email));
    SetWindowTextW(GetDlgItem(win->hMain, IDC_LOGIN_EMAIL), U(mail).c_str());
    ShowPage(win, PG_GRANT);
    SetTip(win, IDC_NOTE_GRANT, L"账号注册成功，选择下面任意一种方式开通授权。");
}

void RunTrial(Win* win)
{
    const std::string email = CurrentEmail(win);
    SetCursor(LoadCursorW(NULL, CX_CURSOR_WAIT));
    const std::string code = DoTrial(email);
    SetCursor(LoadCursorW(NULL, CX_CURSOR_ARROW));
    if (code == "0") { OnAuthorized(win); return; }
    SetTip(win, IDC_NOTE_GRANT, LastText(code));
}

// ==================== 自绘 ====================

void RoundFill(HDC dc, const RECT& rc, int radius, COLORREF fill, COLORREF line)
{
    HPEN pen = CreatePen(PS_SOLID, 1, line);
    HBRUSH br = CreateSolidBrush(fill);
    HGDIOBJ oldP = SelectObject(dc, pen);
    HGDIOBJ oldB = SelectObject(dc, br);
    RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, radius * 2, radius * 2);
    SelectObject(dc, oldP);
    SelectObject(dc, oldB);
    DeleteObject(pen);
    DeleteObject(br);
}

void DrawCtlText(HDC dc, HWND ctl, const RECT& rc, UINT flags)
{
    const int n = GetWindowTextLengthW(ctl);
    if (n <= 0) return;
    std::wstring text((size_t)n + 1, L'\0');
    GetWindowTextW(ctl, &text[0], n + 1);
    text.resize((size_t)n);
    RECT tr = rc;
    DrawTextW(dc, text.c_str(), -1, &tr, flags | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void DrawButton(Win* win, DRAWITEMSTRUCT* dis)
{
    const Style* st = FindStyle(win, (int)dis->CtlID);
    const Kind kind = st ? st->kind : K_SECONDARY;
    const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
    const bool disabled = (dis->itemState & ODS_DISABLED) != 0;
    HDC dc = dis->hDC;
    const RECT rc = dis->rcItem;

    SetBkMode(dc, TRANSPARENT);
    HGDIOBJ oldFont = SelectObject(dc, win->fontBtn);

    if (kind == K_LINK) {
        SetTextColor(dc, disabled ? CX_DISABLED_TEXT : CX_ACCENT);
        DrawCtlText(dc, dis->hwndItem, rc, DT_LEFT | DT_VCENTER);
    } else if (kind == K_PRIMARY) {
        const COLORREF fill = disabled ? RGB(170, 200, 240) : (pressed ? CX_ACCENT_DOWN : CX_ACCENT);
        RoundFill(dc, rc, 6, fill, fill);
        SetTextColor(dc, RGB(255, 255, 255));
        RECT tr = rc;
        InflateRect(&tr, -12, -4);
        DrawCtlText(dc, dis->hwndItem, tr, DT_CENTER | DT_VCENTER);
    } else {
        const COLORREF fill = disabled ? CX_MUTED : (pressed ? CX_BTN_DOWN : CX_BTN_FILL);
        const COLORREF line = disabled ? RGB(226, 230, 235) : CX_BTN_LINE;
        RoundFill(dc, rc, 6, fill, line);
        SetTextColor(dc, disabled ? CX_DISABLED_TEXT : CX_TEXT);
        RECT tr = rc;
        InflateRect(&tr, -12, -4);
        DrawCtlText(dc, dis->hwndItem, tr, DT_CENTER | DT_VCENTER);
    }
    SelectObject(dc, oldFont);
}

void DrawBusy(DRAWITEMSTRUCT* dis)
{
    RECT rc = dis->rcItem;
    InflateRect(&rc, 1, 1);
    RoundFill(dis->hDC, rc, 6, CX_SURFACE, CX_SURFACE);
}

void DrawSpin(DRAWITEMSTRUCT* dis)
{
    const RECT rc = dis->rcItem;
    RoundFill(dis->hDC, rc, 4, CX_SURFACE, CX_SURFACE);
    if (g_spin != NULL) DrawBitmap(g_spin, dis->hDC, rc.left + 2, rc.top + 2, 31, 31);
}

void DrawShop(DRAWITEMSTRUCT* dis)
{
    HDC dc = dis->hDC;
    const RECT rc = dis->rcItem;
    RoundFill(dc, rc, 8, CX_MUTED, RGB(232, 236, 241));
    SetBkMode(dc, TRANSPARENT);
    HGDIOBJ oldFont = SelectObject(dc, g_win->font);
    SetTextColor(dc, CX_TEXT);
    TextOutW(dc, rc.left + 18, rc.top + 14, L"购买 / 续费序列号", 9);
    SetTextColor(dc, CX_SUB);
    TextOutW(dc, rc.left + 18, rc.top + 38, L"在浏览器里选购，付款后把序列号填回「激活序列号」。", 26);
    const std::wstring url = U(ShopUrl());
    if (!url.empty()) TextOutW(dc, rc.left + 18, rc.top + 60, url.c_str(), (int)url.size());
    SelectObject(dc, oldFont);
}

void DrawOkIcon(DRAWITEMSTRUCT* dis)
{
    HDC dc = dis->hDC;
    const RECT rc = dis->rcItem;
    RoundFill(dc, rc, 12, RGB(240, 250, 245), RGB(240, 250, 245));
    if (g_okIcon != NULL) {
        const int side = 56;
        DrawBitmap(g_okIcon, dc, rc.left + (rc.right - rc.left - side) / 2,
                   rc.top + (rc.bottom - rc.top - side) / 2, side, side);
    }
}

void DrawPic(Win* win, DRAWITEMSTRUCT* dis, int slot)
{
    HDC dc = dis->hDC;
    const RECT rc = dis->rcItem;
    RoundFill(dc, rc, 5, CX_SURFACE, CX_LINE_INPUT);
    if (!win->pics[slot].empty()) {
        DrawPngScaled(dc, rc.left + 1, rc.top + 1, rc.right - rc.left - 2, rc.bottom - rc.top - 2,
                      win->pics[slot]);
    } else {
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, CX_SUB);
        TextOutW(dc, rc.left + 12, rc.top + 9, L"点击刷新", 4);
    }
}

void DrawFrames(Win* win, HDC dc)
{
    const HWND focus = GetFocus();
    for (size_t i = 0; i < win->frames.size(); ++i) {
        const Frame& fr = win->frames[i];
        if (fr.page != win->page) continue;
        HWND h = GetDlgItem(win->hMain, fr.id);
        if (h == NULL || IsWindowVisible(h) == FALSE) continue;
        RoundFill(dc, fr.rc, 6, CX_SURFACE, h == focus ? CX_ACCENT : CX_LINE_INPUT);
    }
}

// 客户区底：窗口底色 + 白色卡片 + 输入框描边 + 品牌横幅
void PaintSurface(Win* win, HDC dc)
{
    RECT all;
    GetClientRect(win->hMain, &all);
    FillRect(dc, &all, win->brBg);
    RECT card;
    card.left = CX_CARD_X;
    card.top = CX_CARD_Y;
    card.right = CX_CARD_R;
    card.bottom = CX_CARD_B;
    RoundFill(dc, card, 10, CX_SURFACE, CX_LINE);
    DrawFrames(win, dc);
    if (g_banner != NULL) {
        DrawBitmap(g_banner, dc, 0, 0, CX_W, CX_BANNER_H);
    } else {
        RECT head = { 0, 0, CX_W, CX_BANNER_H };
        HBRUSH br = CreateSolidBrush(RGB(0, 90, 158));
        FillRect(dc, &head, br);
        DeleteObject(br);
    }
}

void OnPaint(Win* win)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(win->hMain, &ps);
    PaintSurface(win, dc);
    EndPaint(win->hMain, &ps);
}

// ==================== 消息处理 ====================

// 两个窗口（向导 / 序列号小窗）共用同一套配色与自绘分发
LRESULT OnColor(Win* win, HDC dc, HWND ctl)
{
    const int id = (int)(INT_PTR)GetWindowLongPtrW(ctl, GWLP_ID);
    const Style* st = FindStyle(win, id);
    const Paper paper = st ? st->paper : PAP_SURFACE;
    SetBkColor(dc, paper == PAP_MUTED ? CX_MUTED : CX_SURFACE);
    SetTextColor(dc, st ? InkOf(st->ink) : CX_TEXT);
    return (LRESULT)BrushFor(win, paper);
}

LRESULT OnDraw(Win* win, DRAWITEMSTRUCT* dis)
{
    switch (dis->CtlID) {
    case IDC_BUSY: DrawBusy(dis); return TRUE;
    case IDC_SPIN: DrawSpin(dis); return TRUE;
    case IDC_SHOP: DrawShop(dis); return TRUE;
    case IDC_OKICON: DrawOkIcon(dis); return TRUE;
    case IDC_REG_PIC: DrawPic(win, dis, 0); return TRUE;
    case IDC_RENEW_PIC: DrawPic(win, dis, 1); return TRUE;
    default:
        DrawButton(win, dis);
        return TRUE;
    }
}

LRESULT CALLBACK MemberProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Win* win = (Win*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    switch (msg) {
    case WM_CREATE:
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCTW*)lParam)->lpCreateParams);
        return 0;
    // PrintWindow / 打印抓取不走 WM_PAINT，得单独应答
    case WM_PRINTCLIENT:
        if (win != NULL) PaintSurface(win, (HDC)wParam);
        return 0;
    case WM_ERASEBKGND:
        return 1;                       // 底色由 PaintSurface 统一画，避免先刷后画闪一下
    case WM_PAINT:
        if (win != NULL) OnPaint(win);
        else DefWindowProcW(hWnd, msg, wParam, lParam);
        return 0;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN:
        if (win != NULL) return OnColor(win, (HDC)wParam, (HWND)lParam);
        break;
    case WM_DRAWITEM:
        if (win != NULL) return OnDraw(win, (DRAWITEMSTRUCT*)lParam);
        break;
    case WM_COMMAND:
        if (win == NULL) return 0;
        // 输入框获得/失去焦点要重画描边
        if (HIWORD(wParam) == EN_SETFOCUS || HIWORD(wParam) == EN_KILLFOCUS) {
            InvalidateRect(hWnd, NULL, TRUE);
            return 0;
        }
        switch (LOWORD(wParam)) {
        case IDC_BTN_W_LOGIN: ShowPage(win, PG_LOGIN); return 0;
        case IDC_BTN_W_REG:   ShowPage(win, PG_REGISTER); return 0;
        case IDC_BTN_TUTORIAL:
            OpenUrl(S().oem.helpHome.empty() ? S().oem.home : S().oem.helpHome);
            return 0;
        case IDC_BTN_HELP:
            OpenUrl(S().oem.home);
            return 0;
        case IDC_BTN_LOGIN:   RunLogin(win); return 0;
        case IDC_BTN_FREE:    ShowPage(win, PG_REGISTER); return 0;
        case IDC_BTN_AGREE: case IDC_BTN_AGREE2:
            OpenUrl(AgreementUrl());
            return 0;
        case IDC_BTN_PREV:    ShowPage(win, PG_WELCOME); return 0;
        case IDC_BTN_REG:     RunRegister(win); return 0;
        case IDC_BTN_TRIAL:   RunTrial(win); return 0;
        case IDC_BTN_RENEW: {
            const std::string code = ShowKeyDialog(CurrentEmail(win), hWnd, false);
            if (code == "0") OnAuthorized(win);
            else SetTip(win, IDC_NOTE_GRANT, LastText(code));
            return 0;
        }
        case IDC_BTN_REFRESH: {
            const std::string code = DoRefresh(GetTextOf(GetDlgItem(hWnd, IDC_LOGIN_EMAIL)));
            SetTip(win, IDC_NOTE_GRANT, code == "0" ? L"授权信息已刷新。" : LastText(code));
            if (code == "0") FillDue(win);
            return 0;
        }
        case IDC_BTN_COPYMAC: case IDC_BTN_COPY: {
            const bool ok = CopyToClipboard(hWnd, U(S().macKey));
            wchar_t text[80];
            wsprintfW(text, ok ? L"机器码已复制，可直接发给客服。" : L"复制失败（%u），请选中机器码手动复制。",
                      g_clipErr);
            SetTip(win, win->page == PG_GRANT ? IDC_NOTE_GRANT : IDC_LOGIN_TIP, text);
            return 0;
        }
        case IDC_BTN_LOGOUT:
            SaveAccount("");
            S().grant = Grant();
            S().grant.authStatus = "none";
            FireEvent(EV_LOGOUT, "{}");
            ShowPage(win, PG_LOGIN);
            SetTip(win, IDC_LOGIN_TIP, L"已退出本机账号，下次需要重新登录。");
            return 0;
        case IDC_BTN_SERVICE: {
            std::wstring text = L"客服QQ：";
            text += S().oem.qq.empty() ? L"未设置" : U(S().oem.qq);
            if (!S().oem.qq2.empty()) text += L"　备用：" + U(S().oem.qq2);
            if (!S().oem.wechat.empty()) text += L"　微信：" + U(S().oem.wechat);
            const std::string shop = ShopUrl();
            if (!shop.empty()) text += L"\r\n购买地址：" + U(shop);
            MessageBoxW(hWnd, text.c_str(), L"联系方式", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        case IDC_SHOP: case IDC_BTN_SHOP:
            OpenUrl(ShopUrl());
            return 0;
        case IDC_BTN_OK:
            DestroyWindow(hWnd);
            return 0;
        default:
            return 0;
        }
    case WM_CLOSE:
        if (win != NULL && !win->authorized) {
            if (MessageBoxW(hWnd, L"还未完成登录，确定退出吗？", win->title.c_str(), MB_YESNO | MB_ICONQUESTION) != IDYES) {
                return 0;
            }
        }
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ==================== 建窗 ====================

void BuildPages(Win* win)
{
    const int c1 = 0, c2 = 296, cw = 280;   // 两列栅格

    // ---- 0 登录或注册 ----
    Label(win, PG_WELCOME, L"登录或注册", 0, 0, 400, 26, IDC_TITLE_WELCOME, INK_TEXT, win->fontTitle);
    Label(win, PG_WELCOME,
          Fill(L"感谢您使用 {ProgramerName}。首次使用需免费注册，注册成功后即可免费试用。").c_str(),
          0, 34, 576, 20, IDC_INTRO, INK_SUB);
    Btn(win, PG_WELCOME, L"登录", 0, 84, 260, CX_BTN_H, IDC_BTN_W_LOGIN, K_PRIMARY);
    Label(win, PG_WELCOME, L"已注册账号可直接登录；官网账号与本软件不互通。", 276, 94, 300, 20,
          IDC_HINT_W_LOGIN, INK_SUB, win->fontSmall);
    Btn(win, PG_WELCOME, L"注册账号", 0, 134, 260, CX_BTN_H, IDC_BTN_W_REG, K_SECONDARY);
    Label(win, PG_WELCOME, L"新用户先注册，注册后可开通授权或直接免费试用。", 276, 144, 300, 20,
          IDC_HINT_W_REG, INK_SUB, win->fontSmall);

    // ---- 1 登录账号 ----
    Label(win, PG_LOGIN, L"登录账号", 0, 0, 400, 26, IDC_TITLE_LOGIN, INK_TEXT, win->fontTitle);
    Label(win, PG_LOGIN, L"请输入注册软件时的邮箱账号，无需密码。", 0, 34, 576, 20, IDC_NOTE_LOGIN,
          INK_SUB, win->fontSmall);
    Label(win, PG_LOGIN, L"邮箱账号", 0, 68, 200, 18, IDC_LBL_LOGIN_EMAIL, INK_SUB, win->fontSmall);
    Input(win, PG_LOGIN, L"", 0, 88, 576, 34, IDC_LOGIN_EMAIL);
    Label(win, PG_LOGIN, L"此电脑机器码", 0, 134, 200, 18, IDC_LBL_MAC, INK_SUB, win->fontSmall);
    Input(win, PG_LOGIN, U(S().macKey).c_str(), 0, 154, 456, 34, IDC_MAC, ES_AUTOHSCROLL | ES_READONLY,
          PAP_MUTED, INK_SUB);
    Btn(win, PG_LOGIN, L"复制", 472, 154, 104, 34, IDC_BTN_COPYMAC, K_SECONDARY);
    Check(win, PG_LOGIN, L"我同意软件使用许可、用户协议", 0, 200, 220, IDC_LOGIN_AGREE);
    Btn(win, PG_LOGIN, L"查看许可协议", 228, 202, 120, 20, IDC_BTN_AGREE, K_LINK, win->fontSmall);
    Btn(win, PG_LOGIN, L"免费注册账号", 456, 202, 120, 20, IDC_BTN_FREE, K_LINK, win->fontSmall);
    Btn(win, PG_LOGIN, L"登录", 0, 236, 576, CX_BTN_H, IDC_BTN_LOGIN, K_PRIMARY);
    Label(win, PG_LOGIN, L"", 0, 288, 576, 20, IDC_LOGIN_TIP, INK_DANGER, win->fontSmall);
    Label(win, PG_LOGIN, L"说明：本软件账号与官网账号不互通；换机直接登录，按提示验证即可继续使用。",
          0, 316, 576, 18, IDC_TERMS, INK_SUB, win->fontSmall);

    // ---- 2 填写注册信息 ----
    Label(win, PG_REGISTER, L"填写注册信息", 0, 0, 400, 26, IDC_TITLE_REGISTER, INK_TEXT, win->fontTitle);
    Label(win, PG_REGISTER, Fill(L"注：账号仅限登录 {ProgramerName}，无需密码，注册后即绑定本机。").c_str(),
          0, 30, 576, 18, IDC_NOTE_REG, INK_SUB, win->fontSmall);
    Label(win, PG_REGISTER, L"邮箱（必填）", c1, 58, 200, 18, IDC_LBL_EMAIL, INK_SUB, win->fontSmall);
    Input(win, PG_REGISTER, L"", c1, 78, cw, 32, IDC_REG_EMAIL);
    Label(win, PG_REGISTER, L"手机号码", c2, 58, 200, 18, IDC_LBL_PHONE, INK_SUB, win->fontSmall);
    Input(win, PG_REGISTER, L"", c2, 78, cw, 32, IDC_REG_PHONE);
    Label(win, PG_REGISTER, L"关联账号", c1, 122, 200, 18, IDC_LBL_REF, INK_SUB, win->fontSmall);
    Input(win, PG_REGISTER, L"", c1, 142, cw, 32, IDC_REG_REF);
    Label(win, PG_REGISTER, L"QQ", c2, 122, 200, 18, IDC_LBL_QQ, INK_SUB, win->fontSmall);
    Input(win, PG_REGISTER, L"", c2, 142, cw, 32, IDC_REG_QQ);
    Label(win, PG_REGISTER, L"获取渠道", c1, 186, 200, 18, IDC_LBL_LEARN, INK_SUB, win->fontSmall);
    Make(win, PG_REGISTER, win->font, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
         c1, 206, cw, 200, IDC_REG_LEARN, INK_TEXT, PAP_SURFACE, K_PLAIN);
    Label(win, PG_REGISTER, L"验证码", c2, 186, 200, 18, IDC_LBL_CODE, INK_SUB, win->fontSmall);
    Input(win, PG_REGISTER, L"", c2, 206, 104, 32, IDC_REG_CODE, ES_AUTOHSCROLL | ES_NUMBER);
    Btn(win, PG_REGISTER, L"", c2 + 116, 206, 88, 32, IDC_REG_PIC, K_SECONDARY);
    Label(win, PG_REGISTER, L"手机号 / 关联账号 / QQ 选填，方便售后；邮箱务必有效，换机与找回都靠它。",
          0, 250, 576, 18, IDC_GUIDE_REG, INK_SUB, win->fontSmall);
    Check(win, PG_REGISTER, L"我已阅读并同意用户协议和软件使用许可", 0, 274, 280, IDC_REG_AGREE);
    Btn(win, PG_REGISTER, L"查看许可协议", 288, 276, 120, 20, IDC_BTN_AGREE2, K_LINK, win->fontSmall);
    Label(win, PG_REGISTER, L"", 0, 300, 576, 18, IDC_REG_TIP, INK_DANGER, win->fontSmall);
    Btn(win, PG_REGISTER, L"上一步", c1, CX_ROW_Y, 120, CX_BTN_H, IDC_BTN_PREV, K_SECONDARY);
    Btn(win, PG_REGISTER, L"免费注册", c2 + 100, CX_ROW_Y, 180, CX_BTN_H, IDC_BTN_REG, K_PRIMARY);
    // 提交中遮罩：盖住整片表单
    Make(win, PG_REGISTER, NULL, L"STATIC", L"", SS_OWNERDRAW, 0, 52, 576, 256, IDC_BUSY,
         INK_TEXT, PAP_SURFACE, K_PLAIN);
    Make(win, PG_REGISTER, NULL, L"STATIC", L"", SS_OWNERDRAW, 190, 150, 35, 35, IDC_SPIN,
         INK_TEXT, PAP_SURFACE, K_PLAIN);
    Label(win, PG_REGISTER, L"请稍候，正在提交注册信息…", 236, 158, 280, 20, IDC_BUSY_TEXT, INK_SUB);

    // ---- 3 开通授权 ----
    Label(win, PG_GRANT, L"开通授权", 0, 0, 400, 26, IDC_TITLE_GRANT, INK_TEXT, win->fontTitle);
    Label(win, PG_GRANT, L"账号注册成功，选择下面任意一种方式开通授权。", 0, 30, 576, 18, IDC_NOTE_GRANT,
          INK_SUB, win->fontSmall);
    Label(win, PG_GRANT, L"注册邮箱", 0, 60, 80, 18, IDC_LBL_SHOW_EMAIL, INK_SUB, win->fontSmall);
    Label(win, PG_GRANT, L"", 88, 60, 360, 18, IDC_SHOW_EMAIL, INK_TEXT, win->fontSmall);
    Label(win, PG_GRANT, L"机器码", 0, 86, 80, 18, IDC_LBL_SHOW_MAC, INK_SUB, win->fontSmall);
    Label(win, PG_GRANT, L"", 88, 86, 360, 18, IDC_SHOW_MAC, INK_SUB, win->fontSmall);
    Btn(win, PG_GRANT, L"复制机器码", 456, 56, 120, 22, IDC_BTN_COPY, K_LINK, win->fontSmall);
    Make(win, PG_GRANT, win->font, L"STATIC", L"", SS_OWNERDRAW, 0, 118, 576, 128, IDC_SHOP,
         INK_TEXT, PAP_SURFACE, K_PLAIN);
    Btn(win, PG_GRANT, L"打开购买页面", 396, 194, 160, 34, IDC_BTN_SHOP, K_SECONDARY);
    Btn(win, PG_GRANT, L"免费试用", c1, CX_ROW_Y, 180, CX_BTN_H, IDC_BTN_TRIAL, K_PRIMARY);
    Btn(win, PG_GRANT, L"激活序列号", 198, CX_ROW_Y, 180, CX_BTN_H, IDC_BTN_RENEW, K_SECONDARY);
    Btn(win, PG_GRANT, L"刷新授权", 396, CX_ROW_Y, 180, CX_BTN_H, IDC_BTN_REFRESH, K_SECONDARY);

    // ---- 4 授权成功 ----
    Make(win, PG_DONE, NULL, L"STATIC", L"", SS_OWNERDRAW, 0, 20, 88, 88, IDC_OKICON,
         INK_TEXT, PAP_SURFACE, K_PLAIN);
    Label(win, PG_DONE, L"授权成功，现在可以开始使用了", 108, 26, 460, 26, IDC_TITLE_DONE, INK_TEXT, win->fontTitle);
    Label(win, PG_DONE, L"授权有效期至 未开通", 108, 62, 460, 20, IDC_DUE, INK_ACCENT, win->fontSmall);
    Label(win, PG_DONE,
          Fill(L"本账号仅限登录 {ProgramerName}，无法用于登录官网。\r\n"
               L"授权期内免费功能升级与问题修复；只能在这台电脑使用，重装系统无需重新注册。\r\n"
               L"请妥善保管激活的序列号，换机时用它验证身份。\r\n"
               L"您还可以访问官方网站 {site}").c_str(),
          0, 132, 576, 120, IDC_TERMS2, INK_SUB, win->fontSmall);
    Btn(win, PG_DONE, L"开始使用软件", 0, CX_ROW_Y, 576, CX_BTN_H, IDC_BTN_OK, K_PRIMARY);

    // ---- 底部文字链接 ----
    struct Perm { const wchar_t* text; int x; int w; int id; };
    const Perm perms[] = {
        { L"注册教程/说明", CX_PX, 110, IDC_BTN_TUTORIAL },
        { L"官方网站", CX_PX + 118, 80, IDC_BTN_HELP },
        { L"联系方式", 448, 80, IDC_BTN_SERVICE },
        { L"退出本机账号", 536, 104, IDC_BTN_LOGOUT }
    };
    for (int i = 0; i < 4; ++i) {
        HWND c = CreateWindowExW(0, L"BUTTON", perms[i].text, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                 perms[i].x, 514, perms[i].w, 22, win->hMain, (HMENU)(INT_PTR)perms[i].id,
                                 GetModuleHandleW(NULL), NULL);
        if (c != NULL) SendMessageW(c, WM_SETFONT, (WPARAM)win->fontSmall, TRUE);
        Register(win, perms[i].id, -1, INK_SUB, PAP_SURFACE, K_LINK);
    }
}

Win* NewWin()
{
    Win* win = new Win();
    win->hMain = NULL;
    win->font = NULL;
    win->fontSmall = NULL;
    win->fontTitle = NULL;
    win->fontBtn = NULL;
    win->brBg = NULL;
    win->brSurface = NULL;
    win->brMuted = NULL;
    win->page = PG_WELCOME;
    win->ox = CX_PX;
    win->oy = CX_PY;
    win->authorized = false;
    win->withNav = true;
    win->busy = false;
    win->doneView = false;
    win->dlgChange = false;
    const std::wstring name = S().oem.softName.empty() ? L"软件会员" : U(S().oem.softName);
    win->title = L"注册 / 登录 " + name;
    g_win = win;
    return win;
}

// 静态控件按控件宽度自动换行，高度给小了最后一行就被截掉
int MeasureTextH(HFONT f, const std::wstring& text, int w)
{
    HDC dc = GetDC(NULL);
    HFONT old = (HFONT)SelectObject(dc, f);
    RECT rc = { 0, 0, w, 0 };
    DrawTextW(dc, text.c_str(), -1, &rc, DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL | DT_LEFT);
    SelectObject(dc, old);
    ReleaseDC(NULL, dc);
    return rc.bottom;
}

HFONT MakeFont(int h, int weight)
{
    return CreateFontW(-h, 0, 0, 0, weight, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
}

bool EnsureClass()
{
    static bool registered = false;
    if (registered) return true;
    static HBRUSH brBg = NULL;
    if (brBg == NULL) brBg = CreateSolidBrush(CX_BG);
    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = MemberProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hCursor = LoadCursorW(NULL, CX_CURSOR_ARROW);
    wc.hbrBackground = brBg;
    wc.lpszClassName = CxAhClassName;
    if (RegisterClassW(&wc) == 0 && GetLastError() != 1410) return false;   // 1410 = 类已存在
    registered = true;
    return true;
}

bool CreateAndRun(Win* win, int page, bool withNav)
{
    if (!EnsureClass()) return false;
    win->withNav = withNav;

    RECT rc = { 0, 0, CX_W, CX_H };
    AdjustWindowRect(&rc, WS_POPUP | WS_CAPTION | WS_SYSMENU, FALSE);
    win->hMain = CreateWindowExW(WS_EX_DLGMODALFRAME, CxAhClassName, win->title.c_str(),
                                 WS_POPUP | WS_CAPTION | WS_SYSMENU,
                                 CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
                                 S().hParent, NULL, GetModuleHandleW(NULL), win);
    if (win->hMain == NULL) return false;

    win->font = MakeFont(12, FW_NORMAL);
    win->fontSmall = MakeFont(12, FW_NORMAL);
    win->fontBtn = MakeFont(13, FW_NORMAL);
    win->fontTitle = MakeFont(18, FW_SEMIBOLD);
    win->brBg = CreateSolidBrush(CX_BG);
    win->brSurface = CreateSolidBrush(CX_SURFACE);
    win->brMuted = CreateSolidBrush(CX_MUTED);
    if (g_banner == NULL) g_banner = BitmapFromBase64(g_bannerPng);
    if (g_okIcon == NULL) g_okIcon = BitmapFromBase64(g_okPng);
    if (g_spin == NULL) g_spin = BitmapFromBase64(g_spinGif);

    BuildPages(win);

    HWND combo = GetDlgItem(win->hMain, IDC_REG_LEARN);
    if (combo != NULL) {
        const wchar_t* channels[] = { L"网络搜索", L"淘宝订购", L"朋友介绍", L"公司集采", L"广告", L"其他" };
        for (int i = 0; i < 6; ++i) SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)channels[i]);
        SendMessageW(combo, CB_SETCURSEL, 0, 0);
    }
    const std::string saved = LoadAccount();
    if (!saved.empty()) SetWindowTextW(GetDlgItem(win->hMain, IDC_LOGIN_EMAIL), U(saved).c_str());
    ShowPage(win, page >= 0 && page < PG_COUNT ? page : PG_WELCOME);

    // 居中并禁用父窗
    RECT rcParent = { 0, 0, CX_W, CX_H };
    if (S().hParent != NULL) GetWindowRect(S().hParent, &rcParent);
    const int pw = rcParent.right - rcParent.left, ph = rcParent.bottom - rcParent.top;
    SetWindowPos(win->hMain, HWND_TOP, rcParent.left + (pw - CX_W) / 2, rcParent.top + (ph - CX_H) / 2,
                 0, 0, SWP_NOSIZE);
    if (S().hParent != NULL) EnableWindow(S().hParent, FALSE);
    ShowWindow(win->hMain, SW_SHOW);
    UpdateWindow(win->hMain);
    SetForegroundWindow(win->hMain);

    MSG m;
    while (GetMessageW(&m, NULL, 0, 0)) {
        if (!IsDialogMessageW(win->hMain, &m)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
    }

    if (S().hParent != NULL) EnableWindow(S().hParent, TRUE);
    FireEvent(EV_WINDOW_CLOSE, "{\"authorized\":\"" + std::string(win->authorized ? "1" : "0") + "\"}");
    const bool result = win->authorized;
    if (win->font) DeleteObject(win->font);
    if (win->fontSmall) DeleteObject(win->fontSmall);
    if (win->fontTitle) DeleteObject(win->fontTitle);
    if (win->fontBtn) DeleteObject(win->fontBtn);
    if (win->brBg) DeleteObject(win->brBg);
    if (win->brSurface) DeleteObject(win->brSurface);
    if (win->brMuted) DeleteObject(win->brMuted);
    delete win;
    g_win = NULL;
    return result;
}

// ==================== 序列号激活 / 换机小窗口 ====================

LRESULT CALLBACK KeyProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
std::string ShowKeyDialog(const std::string& email, HWND owner, bool forChange);

bool EnsureKeyClass()
{
    static bool registered = false;
    if (registered) return true;
    static HBRUSH br = NULL;
    if (br == NULL) br = CreateSolidBrush(CX_SURFACE);
    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = KeyProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hCursor = LoadCursorW(NULL, CX_CURSOR_ARROW);
    wc.hbrBackground = br;
    wc.lpszClassName = CxAhKeyClassName;
    if (RegisterClassW(&wc) == 0 && GetLastError() != 1410) return false;
    registered = true;
    return true;
}

void SetInk(Win* win, int id, Ink ink);

// 换机成功后不直接关窗：留在成功页，把剩余次数说清楚
void ShowChangeDone(Win* win)
{
    win->doneView = true;
    const int hides[] = { IDC_LBL_KEY, IDC_RENEW_KEY, IDC_LBL_RENEW_CODE, IDC_RENEW_CODE,
                          IDC_RENEW_PIC, IDC_BTN_PIC_REFRESH, IDC_BTN_RENEW_BACK, IDC_BTN_HOME };
    for (size_t i = 0; i < sizeof(hides) / sizeof(hides[0]); ++i) {
        ShowWindow(GetDlgItem(win->hMain, hides[i]), SW_HIDE);
    }
    SetTip(win, IDC_TITLE_RENEW, L"换机成功");
    SetTip(win, IDC_RENEW_NOTE,
           L"本账号已绑定到这台电脑，可以正常使用了。\r\n"
           L"原机器上的软件会自动退出，需要的话请回原机器保存采集数据。");
    SetTip(win, IDC_RENEW_TIP, L"本账号剩余换机次数：" + std::to_wstring(S().grant.changeLeft) + L" 次");
    SetInk(win, IDC_RENEW_TIP, INK_ACCENT);
    ShowWindow(GetDlgItem(win->hMain, IDC_OKICON), SW_SHOW);
    HWND act = GetDlgItem(win->hMain, IDC_BTN_ACT);
    SetWindowTextW(act, L"完成");
    SetWindowPos(act, NULL, win->ox + 146, win->oy + 258 + win->keyDY, 180, CX_BTN_H, SWP_NOZORDER);
    InvalidateRect(win->hMain, NULL, TRUE);
    SetFocus(act);
}

void KeySubmit(Win* win)
{
    const std::string key = TrimStr(GetTextOf(GetDlgItem(win->hMain, IDC_RENEW_KEY)));
    if (key.empty()) { SetTip(win, IDC_RENEW_TIP, L"请输入序列号。"); return; }
    SetCursor(LoadCursorW(NULL, CX_CURSOR_WAIT));
    const std::string code = win->dlgChange
        ? DoChangeConfirm(win->dlgEmail, key, S().captchaId, GetTextOf(GetDlgItem(win->hMain, IDC_RENEW_CODE)),
                          S().requestId)
        : DoActivate(win->dlgEmail, key, S().captchaId, GetTextOf(GetDlgItem(win->hMain, IDC_RENEW_CODE)));
    SetCursor(LoadCursorW(NULL, CX_CURSOR_ARROW));
    if (code == "0") {
        win->authorized = true;
        if (win->dlgChange) { ShowChangeDone(win); return; }
        DestroyWindow(win->hMain);
        return;
    }
    std::wstring tip = LastText(code);
    // 剩余次数只在验证之后才给，第一次进窗口不显示
    if (win->dlgChange && S().grant.changeLeft > 0) {
        tip += L"　还剩 " + std::to_wstring(S().grant.changeLeft) + L" 次换机机会。";
    }
    SetTip(win, IDC_RENEW_TIP, tip);
    RefreshPic(win, 1, IDC_RENEW_PIC, IDC_RENEW_TIP);
}

LRESULT CALLBACK KeyProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Win* win = (Win*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    switch (msg) {
    case WM_CREATE:
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCTW*)lParam)->lpCreateParams);
        return 0;
    case WM_PRINTCLIENT: {
        if (win == NULL) break;
        RECT all;
        GetClientRect(hWnd, &all);
        FillRect((HDC)wParam, &all, win->brSurface);
        DrawFrames(win, (HDC)wParam);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        if (win == NULL) break;
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hWnd, &ps);
        RECT all;
        GetClientRect(hWnd, &all);
        FillRect(dc, &all, win->brSurface);
        DrawFrames(win, dc);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN:
        if (win != NULL) return OnColor(win, (HDC)wParam, (HWND)lParam);
        break;
    case WM_DRAWITEM:
        if (win != NULL) return OnDraw(win, (DRAWITEMSTRUCT*)lParam);
        break;
    case WM_COMMAND:
        if (win == NULL) return 0;
        if (HIWORD(wParam) == EN_SETFOCUS || HIWORD(wParam) == EN_KILLFOCUS) {
            InvalidateRect(hWnd, NULL, TRUE);
            return 0;
        }
        switch (LOWORD(wParam)) {
        case IDC_BTN_ACT:
            if (win->doneView) DestroyWindow(hWnd);
            else KeySubmit(win);
            return 0;
        case IDC_RENEW_PIC: case IDC_BTN_PIC_REFRESH:
            RefreshPic(win, 1, IDC_RENEW_PIC, IDC_RENEW_TIP);
            return 0;
        case IDC_BTN_HOME:
            OpenUrl(ShopUrl());
            return 0;
        case IDC_BTN_RENEW_BACK:
            DestroyWindow(hWnd);
            return 0;
        default:
            return 0;
        }
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

std::string ShowKeyDialog(const std::string& email, HWND owner, bool forChange)
{
    if (S().server.empty()) return "1001";
    if (!EnsureKeyClass()) return "1001";

    Win* outer = g_win;
    Win* win = NewWin();
    win->dlgChange = forChange;
    win->dlgEmail = TrimStr(email.empty() ? LoadAccount() : email);
    win->ox = 24;
    win->oy = 16;
    win->withNav = false;
    win->title = forChange ? L"换机验证" : L"激活序列号";

    // 换机说明里带着账号，行数随邮箱长度变；先量准再排窗口，不然最后一行会被截掉。
    // 量文本用与 win->fontSmall 完全同规格的临时字体，窗口建好后不用再建一遍。
    const std::wstring note = forChange
        ? (L"您的 " + U(win->dlgEmail) + L" 账号已绑定到另一台机器，如果您需要登录，"
           L"请输入以前激活过的序列号完成验证即可登录。\r\n"
           L"注意：更换后原机器的软件将自动退出，如有采集数据等请保存后再更换。")
        : std::wstring(L"输入购买的序列号开通或续费授权，激活后授权充值到当前账号。");
    const int cw = 472;
    HFONT fNote = MakeFont(12, FW_NORMAL);
    const int noteH = MeasureTextH(fNote, note, cw) + 2;
    DeleteObject(fNote);
    win->keyDY = noteH > 44 ? noteH - 44 : 0;

    RECT rc = { 0, 0, CXKEY_W, CXKEY_H + win->keyDY };
    AdjustWindowRect(&rc, WS_POPUP | WS_CAPTION | WS_SYSMENU, FALSE);
    win->hMain = CreateWindowExW(WS_EX_DLGMODALFRAME, CxAhKeyClassName, win->title.c_str(),
                                 WS_POPUP | WS_CAPTION | WS_SYSMENU,
                                 CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
                                 owner, NULL, GetModuleHandleW(NULL), win);
    if (win->hMain == NULL) {
        delete win;
        g_win = outer;
        return "1001";
    }

    win->font = MakeFont(12, FW_NORMAL);
    win->fontSmall = MakeFont(12, FW_NORMAL);
    win->fontBtn = MakeFont(13, FW_NORMAL);
    win->fontTitle = MakeFont(18, FW_SEMIBOLD);
    win->brBg = CreateSolidBrush(CX_BG);
    win->brSurface = CreateSolidBrush(CX_SURFACE);
    win->brMuted = CreateSolidBrush(CX_MUTED);
    const int dy = win->keyDY;

    Label(win, PG_WELCOME, forChange ? L"换机验证" : L"激活序列号", 0, 0, 300, 26, IDC_TITLE_RENEW,
          INK_TEXT, win->fontTitle);
    // 剩余换机次数要等验证过才给，这里只讲「账号绑在别的机器」和换机后果
    Label(win, PG_WELCOME, note.c_str(), 0, 28, cw, noteH, IDC_RENEW_NOTE, INK_SUB, win->fontSmall);
    Label(win, PG_WELCOME, L"序列号", 0, 80 + dy, 200, 18, IDC_LBL_KEY, INK_SUB, win->fontSmall);
    Input(win, PG_WELCOME, L"", 0, 100 + dy, cw, 34, IDC_RENEW_KEY);
    Label(win, PG_WELCOME, L"验证码", 0, 146 + dy, 200, 18, IDC_LBL_RENEW_CODE, INK_SUB, win->fontSmall);
    Input(win, PG_WELCOME, L"", 0, 166 + dy, 120, 34, IDC_RENEW_CODE);
    Btn(win, PG_WELCOME, L"", 132, 166 + dy, 100, 34, IDC_RENEW_PIC, K_SECONDARY);
    Btn(win, PG_WELCOME, L"看不清？换一张", 244, 172 + dy, 160, 20, IDC_BTN_PIC_REFRESH, K_LINK, win->fontSmall);
    Label(win, PG_WELCOME, L"", 0, 210 + dy, cw, 18, IDC_RENEW_TIP, INK_DANGER, win->fontSmall);
    if (!forChange) {
        Btn(win, PG_WELCOME, L"还没有序列号？去官网购买", 0, 232 + dy, 260, 18, IDC_BTN_HOME, K_LINK, win->fontSmall);
    }
    Btn(win, PG_WELCOME, L"取消", 0, 258 + dy, 120, CX_BTN_H, IDC_BTN_RENEW_BACK, K_SECONDARY);
    Btn(win, PG_WELCOME, forChange ? L"验证并换机" : L"开通授权", 292, 258 + dy, 180, CX_BTN_H, IDC_BTN_ACT,
        K_PRIMARY);
    if (forChange) {
        // 换机成功页用的对勾，先建好藏起来
        Make(win, PG_WELCOME, NULL, L"STATIC", L"", SS_OWNERDRAW, 0, 80 + dy, 88, 88, IDC_OKICON,
             INK_TEXT, PAP_SURFACE, K_PLAIN);
        ShowWindow(GetDlgItem(win->hMain, IDC_OKICON), SW_HIDE);
    }

    RefreshPic(win, 1, IDC_RENEW_PIC, IDC_RENEW_TIP);

    RECT rcOwner = { 0, 0, CXKEY_W, CXKEY_H };
    if (owner != NULL) GetWindowRect(owner, &rcOwner);
    const int ow = rcOwner.right - rcOwner.left, oh = rcOwner.bottom - rcOwner.top;
    SetWindowPos(win->hMain, HWND_TOP, rcOwner.left + (ow - (rc.right - rc.left)) / 2,
                 rcOwner.top + (oh - (rc.bottom - rc.top)) / 2, 0, 0, SWP_NOSIZE);
    if (owner != NULL) EnableWindow(owner, FALSE);
    ShowWindow(win->hMain, SW_SHOW);
    UpdateWindow(win->hMain);
    SetForegroundWindow(win->hMain);
    SetFocus(GetDlgItem(win->hMain, IDC_RENEW_KEY));

    MSG m;
    while (GetMessageW(&m, NULL, 0, 0)) {
        if (!IsDialogMessageW(win->hMain, &m)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
    }

    if (owner != NULL) EnableWindow(owner, TRUE);
    std::string result = win->authorized ? "0" : S().lastCode;
    if (result.empty()) result = "9999";
    if (win->font) DeleteObject(win->font);
    if (win->fontSmall) DeleteObject(win->fontSmall);
    if (win->fontTitle) DeleteObject(win->fontTitle);
    if (win->fontBtn) DeleteObject(win->fontBtn);
    if (win->brBg) DeleteObject(win->brBg);
    if (win->brSurface) DeleteObject(win->brSurface);
    if (win->brMuted) DeleteObject(win->brMuted);
    delete win;
    g_win = outer;
    return result;
}

bool EnsureUcClass()
{
    static bool registered = false;
    if (registered) return true;
    static HBRUSH br = NULL;
    if (br == NULL) br = CreateSolidBrush(CX_SURFACE);
    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = UcProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hCursor = LoadCursorW(NULL, CX_CURSOR_ARROW);
    wc.hbrBackground = br;
    wc.lpszClassName = CxAhUcClassName;
    if (RegisterClassW(&wc) == 0 && GetLastError() != 1410) return false;
    registered = true;
    return true;
}

// ==================== 用户中心 ====================

// 多行只读框：外框同样由父窗口画圆角
HWND Box(Win* win, int page, const wchar_t* text, int x, int y, int w, int h, int id,
         DWORD extra, Paper paper, Ink ink)
{
    Frame fr;
    fr.id = id;
    fr.page = page;
    fr.rc.left = win->ox + x;
    fr.rc.top = win->oy + y;
    fr.rc.right = fr.rc.left + w;
    fr.rc.bottom = fr.rc.top + h;
    win->frames.push_back(fr);
    return Make(win, page, win->fontSmall, L"EDIT", text, extra, x + 10, y + 6, w - 20, h - 12, id,
                ink, paper, K_PLAIN);
}

void SetInk(Win* win, int id, Ink ink)
{
    for (size_t i = 0; i < win->styles.size(); ++i) {
        if (win->styles[i].id == id) { win->styles[i].ink = ink; return; }
    }
}

// 把当前授权数据铺到界面上；刷新授权与激活成功后都会重跑一遍
void UcFill(Win* win)
{
    Grant& g = S().grant;
    const std::string email = !g.email.empty() ? g.email : (win->dlgEmail.empty() ? LoadAccount() : win->dlgEmail);
    SetTip(win, IDC_UC_ACCOUNT, email.empty() ? L"未登录，请先在会员窗口注册或登录" : U(email));
    SetTip(win, IDC_UC_MAC, U(S().macKey));

    std::wstring ed = U(g.functionName.empty() ? GbkToUtf8("标准版") : g.functionName);
    if (!g.function.empty()) ed += L"（版本代号 " + U(g.function) + L"）";
    SetTip(win, IDC_UC_EDITION, ed);

    std::wstring due = g.dueDate.empty() ? L"未开通" : U(g.dueDate);
    if (g.isTrial) due += L"（试用）";
    else if (g.isExpired) due += L"（已到期）";
    SetTip(win, IDC_UC_DUE, due);

    SetTip(win, IDC_UC_LEFT, email.empty() ? L"—"
             : std::to_wstring(g.changeLeft) + L" 次（上限 " + std::to_wstring(g.limitChange) + L" 次）");

    std::wstring feats;
    for (size_t i = 0; i < g.features.size(); ++i) {
        const Feature& f = g.features[i];
        feats += U(f.name) + L"　" + (f.dueDate.empty() ? L"长期有效" : U(f.dueDate) + L" 到期") +
                 (f.usable ? L"" : L"（已停用）") + L"\r\n";
    }
    if (feats.empty()) feats = L"未开通选配功能";
    SetTip(win, IDC_UC_FEATURES, feats);

    const std::string st = g.authStatus;
    std::wstring label;
    Ink ink = INK_TEXT;
    if (st == "ok") { label = L"授权正常"; ink = INK_ACCENT; }
    else if (st == "trial") { label = L"免费试用中，购买后可继续使用"; ink = INK_ACCENT; }
    else if (st == "expired") { label = L"授权已到期，续费后即可继续使用"; ink = INK_DANGER; }
    else if (st == "frozen") { label = L"账号已被停止使用，请联系客服"; ink = INK_DANGER; }
    else { label = L"尚未开通授权，可先免费试用或输入序列号激活"; ink = INK_DANGER; }
    if (!g.bindMac.empty() && g.bindMac != S().macKey) label += L"　※ 本账号绑定的不是这台电脑";
    SetTip(win, IDC_UC_STATE, label);
    SetInk(win, IDC_UC_STATE, ink);
    InvalidateRect(GetDlgItem(win->hMain, IDC_UC_STATE), NULL, TRUE);
}

void UcRefresh(Win* win)
{
    SetCursor(LoadCursorW(NULL, CX_CURSOR_WAIT));
    const std::string code = DoRefresh(win->dlgEmail.empty() ? CurrentEmail(win) : win->dlgEmail);
    SetCursor(LoadCursorW(NULL, CX_CURSOR_ARROW));
    UcFill(win);
    SetTip(win, IDC_UC_TIP, code == "0" ? L"授权信息已刷新。" : LastText(code));
}

LRESULT CALLBACK UcProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Win* win = (Win*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    switch (msg) {
    case WM_CREATE:
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCTW*)lParam)->lpCreateParams);
        return 0;
    case WM_PRINTCLIENT: {
        if (win == NULL) break;
        RECT all;
        GetClientRect(hWnd, &all);
        FillRect((HDC)wParam, &all, win->brSurface);
        DrawFrames(win, (HDC)wParam);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        if (win == NULL) break;
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hWnd, &ps);
        RECT all;
        GetClientRect(hWnd, &all);
        FillRect(dc, &all, win->brSurface);
        DrawFrames(win, dc);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN:
        if (win != NULL) return OnColor(win, (HDC)wParam, (HWND)lParam);
        break;
    case WM_DRAWITEM:
        if (win != NULL) return OnDraw(win, (DRAWITEMSTRUCT*)lParam);
        break;
    case WM_COMMAND:
        if (win == NULL) return 0;
        if (HIWORD(wParam) == EN_SETFOCUS || HIWORD(wParam) == EN_KILLFOCUS) {
            InvalidateRect(hWnd, NULL, TRUE);
            return 0;
        }
        switch (LOWORD(wParam)) {
        case IDC_BTN_UC_BUY:
            OpenUrl(ShopUrl());
            return 0;
        case IDC_BTN_UC_REFRESH:
            UcRefresh(win);
            return 0;
        case IDC_BTN_UC_ACT: {
            const std::string code = ShowKeyDialog(win->dlgEmail, hWnd, false);
            if (code == "0") {
                UcRefresh(win);
                SetTip(win, IDC_UC_TIP, L"序列号已激活，授权已充值。");
            }
            return 0;
        }
        case IDC_BTN_UC_COPY:
            CopyToClipboard(hWnd, U(S().macKey));
            SetTip(win, IDC_UC_TIP, L"机器码已复制，可直接发给客服。");
            return 0;
        case IDC_BTN_UC_CLOSE:
            DestroyWindow(hWnd);
            return 0;
        default:
            return 0;
        }
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool RunUserCenter(const std::string& email, HWND owner)
{
    if (S().server.empty()) return false;
    if (!EnsureUcClass()) return false;

    Win* outer = g_win;
    Win* win = NewWin();
    win->dlgEmail = TrimStr(email.empty() ? LoadAccount() : email);
    win->ox = 24;
    win->oy = 20;
    win->withNav = false;
    win->title = L"用户中心 - " + SoftName();

    RECT rc = { 0, 0, CXUC_W, CXUC_H };
    AdjustWindowRect(&rc, WS_POPUP | WS_CAPTION | WS_SYSMENU, FALSE);
    win->hMain = CreateWindowExW(WS_EX_DLGMODALFRAME, CxAhUcClassName, win->title.c_str(),
                                 WS_POPUP | WS_CAPTION | WS_SYSMENU,
                                 CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
                                 owner, NULL, GetModuleHandleW(NULL), win);
    if (win->hMain == NULL) {
        delete win;
        g_win = outer;
        return false;
    }
    win->font = MakeFont(12, FW_NORMAL);
    win->fontSmall = MakeFont(12, FW_NORMAL);
    win->fontBtn = MakeFont(13, FW_NORMAL);
    win->fontTitle = MakeFont(18, FW_SEMIBOLD);
    win->brBg = CreateSolidBrush(CX_BG);
    win->brSurface = CreateSolidBrush(CX_SURFACE);
    win->brMuted = CreateSolidBrush(CX_MUTED);

    const int half = 278, gap = 16, cw = 572;
    Label(win, PG_WELCOME, L"用户中心", 0, 0, 300, 26, IDC_UC_TITLE, INK_TEXT, win->fontTitle);
    Label(win, PG_WELCOME, L"", 0, 30, cw, 18, IDC_UC_STATE, INK_SUB, win->fontSmall);

    Label(win, PG_WELCOME, L"登录账号", 0, 62, half, 18, IDC_LBL_ACCOUNT, INK_SUB, win->fontSmall);
    Input(win, PG_WELCOME, L"", 0, 82, half, 34, IDC_UC_ACCOUNT, ES_AUTOHSCROLL | ES_READONLY, PAP_MUTED, INK_TEXT);
    Label(win, PG_WELCOME, L"授权版本", half + gap, 62, half, 18, IDC_LBL_EDITION, INK_SUB, win->fontSmall);
    Input(win, PG_WELCOME, L"", half + gap, 82, half, 34, IDC_UC_EDITION, ES_AUTOHSCROLL | ES_READONLY,
          PAP_MUTED, INK_TEXT);

    Label(win, PG_WELCOME, L"本机机器码", 0, 128, 460, 18, IDC_LBL_UC_MAC, INK_SUB, win->fontSmall);
    Input(win, PG_WELCOME, L"", 0, 148, 470, 34, IDC_UC_MAC, ES_AUTOHSCROLL | ES_READONLY, PAP_MUTED, INK_SUB);
    Btn(win, PG_WELCOME, L"复制", 482, 148, 90, 34, IDC_BTN_UC_COPY, K_SECONDARY);

    Label(win, PG_WELCOME, L"到期时间", 0, 194, half, 18, IDC_LBL_DUE, INK_SUB, win->fontSmall);
    Input(win, PG_WELCOME, L"", 0, 214, half, 34, IDC_UC_DUE, ES_AUTOHSCROLL | ES_READONLY, PAP_MUTED, INK_TEXT);
    Label(win, PG_WELCOME, L"剩余换机次数", half + gap, 194, half, 18, IDC_LBL_LEFT, INK_SUB, win->fontSmall);
    Input(win, PG_WELCOME, L"", half + gap, 214, half, 34, IDC_UC_LEFT, ES_AUTOHSCROLL | ES_READONLY,
          PAP_MUTED, INK_TEXT);

    Label(win, PG_WELCOME, L"已开通的选配功能", 0, 260, cw, 18, IDC_LBL_FEATURES, INK_SUB, win->fontSmall);
    Box(win, PG_WELCOME, L"", 0, 280, cw, 84, IDC_UC_FEATURES,
        ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL, PAP_MUTED, INK_TEXT);
    Label(win, PG_WELCOME, L"", 0, 370, cw, 18, IDC_UC_TIP, INK_DANGER, win->fontSmall);

    Btn(win, PG_WELCOME, L"打开购买页面", 0, 400, 150, CX_BTN_H, IDC_BTN_UC_BUY, K_SECONDARY);
    Btn(win, PG_WELCOME, L"激活序列号", 160, 400, 130, CX_BTN_H, IDC_BTN_UC_ACT, K_SECONDARY);
    Btn(win, PG_WELCOME, L"刷新授权", 300, 400, 120, CX_BTN_H, IDC_BTN_UC_REFRESH, K_SECONDARY);
    Btn(win, PG_WELCOME, L"关闭", 442, 400, 130, CX_BTN_H, IDC_BTN_UC_CLOSE, K_PRIMARY);

    // 打开先拉一次授权，界面才有最新的到期时间与选配功能
    if (!win->dlgEmail.empty()) DoRefresh(win->dlgEmail);
    UcFill(win);

    RECT rcOwner = { 0, 0, CXUC_W, CXUC_H };
    if (owner != NULL) GetWindowRect(owner, &rcOwner);
    const int ow = rcOwner.right - rcOwner.left, oh = rcOwner.bottom - rcOwner.top;
    SetWindowPos(win->hMain, HWND_TOP, rcOwner.left + (ow - (rc.right - rc.left)) / 2,
                 rcOwner.top + (oh - (rc.bottom - rc.top)) / 2, 0, 0, SWP_NOSIZE);
    if (owner != NULL) EnableWindow(owner, FALSE);
    ShowWindow(win->hMain, SW_SHOW);
    UpdateWindow(win->hMain);
    SetForegroundWindow(win->hMain);

    MSG m;
    while (GetMessageW(&m, NULL, 0, 0)) {
        if (!IsDialogMessageW(win->hMain, &m)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
    }

    if (owner != NULL) EnableWindow(owner, TRUE);
    const bool ok = S().grant.authorized;
    if (win->font) DeleteObject(win->font);
    if (win->fontSmall) DeleteObject(win->fontSmall);
    if (win->fontTitle) DeleteObject(win->fontTitle);
    if (win->fontBtn) DeleteObject(win->fontBtn);
    if (win->brBg) DeleteObject(win->brBg);
    if (win->brSurface) DeleteObject(win->brSurface);
    if (win->brMuted) DeleteObject(win->brMuted);
    delete win;
    g_win = outer;
    return ok;
}

}  // namespace

// ==================== 对外入口 ====================

bool Ui_ShowMemberWindow(int page)
{
    if (S().server.empty()) return false;
    Win* win = NewWin();
    return CreateAndRun(win, page, true);
}

bool Ui_ShowUserCenter(const std::string& email, HWND owner)
{
    return RunUserCenter(email, owner);
}

bool Ui_ShowRenewWindow()
{
    if (S().server.empty()) return false;
    return ShowKeyDialog(LoadAccount(), S().hParent, false) == "0";
}

std::string Ui_ShowChangeWindow(const std::string& email)
{
    if (S().server.empty()) return "1001";
    const std::string probe = DoChangeProbe(email);
    if (probe != "0") return probe;
    return ShowKeyDialog(email, S().hParent, true);
}

void Ui_ClearLocalAccount()
{
    SaveAccount("");
}

}  // namespace cx
