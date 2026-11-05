// 创想AH会员框架 —— 原生 C++ 支持库（.fne），32 位、静态 CRT、GBK 产物。
// 对外文本一律 UTF-8 原始字节，库内不做 GBK 转换；机器码由宿主传入，库内不生成。
// 协议与 PHP 后端配套，见 php/README.md 第 3 节。

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

// lib2.h 用到 DATE / DOUBLE / FLOAT / INT64，SDK 的 mtypes.h 与 windows.h 冲突，这里只补类型定义。
typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;

#include <lib2.h>
#include <lang.h>

#include "cx_core.h"
#include "cx_ui.h"

#define LIB_NOTIFY_FUNC_NAME "cxah_ProcessNotifyLib"

namespace cx {

PFN_NOTIFY_SYS g_pfnNotifySys = NULL;

INT NotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    if (g_pfnNotifySys == NULL) return 0;
    return g_pfnNotifySys(nMsg, dwParam1, dwParam2);
}

char* CloneTextData(LPCTSTR pszText)
{
    if (pszText == NULL || g_pfnNotifySys == NULL) return NULL;
    const INT nLen = (INT)lstrlen(pszText);
    if (nLen <= 0) return NULL;
    char* pd = (char*)NotifySys(NRS_MALLOC, (DWORD)(nLen + 1), 0);
    if (pd == NULL) return NULL;
    memcpy(pd, pszText, nLen);
    pd[nLen] = '\0';
    return pd;
}

// ---------------- 取参数（全部按 UTF-8 字节串处理） ----------------

static std::string ArgStr(PMDATA_INF pArgInf, INT index, INT count)
{
    if (index >= count) return std::string();
    const MDATA_INF& a = pArgInf[index];
    if (a.m_dtDataType == SDT_TEXT && a.m_pText != NULL) return std::string(a.m_pText);
    if (a.m_dtDataType == SDT_INT) return ToStr(a.m_int);
    return std::string();
}

static INT ArgInt(PMDATA_INF pArgInf, INT index, INT count, INT def)
{
    if (index >= count) return def;
    const MDATA_INF& a = pArgInf[index];
    if (a.m_dtDataType == SDT_INT) return a.m_int;
    if (a.m_dtDataType == SDT_TEXT && a.m_pText != NULL) return atoi(a.m_pText);
    return def;
}

static bool ArgBool(PMDATA_INF pArgInf, INT index, INT count, bool def)
{
    if (index >= count) return def;
    const MDATA_INF& a = pArgInf[index];
    if (a.m_dtDataType == SDT_BOOL) return a.m_bool != BL_FALSE;
    if (a.m_dtDataType == SDT_INT) return a.m_int != 0;
    return def;
}

static void RetText(PMDATA_INF pRetData, const std::string& utf8)
{
    pRetData->m_dtDataType = SDT_TEXT;
    pRetData->m_pText = CloneTextData(utf8.c_str());
    if (pRetData->m_pText == NULL) pRetData->m_pText = CloneTextData("");
}

static void RetInt(PMDATA_INF pRetData, INT v)
{
    pRetData->m_dtDataType = SDT_INT;
    pRetData->m_int = v;
}

static void RetBool(PMDATA_INF pRetData, bool v)
{
    pRetData->m_dtDataType = SDT_BOOL;
    pRetData->m_bool = v ? BL_TRUE : BL_FALSE;
}

// 会员管理类与全局命令共用的小工具，定义在文件后面的「会员管理 类」一节里。
static std::string EmailArg(PMDATA_INF p, INT n);
static bool HasFeatureCode(const std::string& list, const std::string& want);
static std::string SoftSettingOf(const std::string& key, const std::string& def);
static std::string RawResponseJson(const ApiResponse& res);

// ---------------- 错误码本地文案（服务器没给文案时兜底） ----------------

struct CodeText { const char* code; const char* text; };
static const CodeText kCodes[] = {
    {"0", "成功"},
    {"1001", "提交的信息不完整，请检查后重试。"},
    {"1002", "软件校验失败，请重新下载官方安装包后重试。"},
    {"1003", "本机时间与标准时间相差过大，请检查电脑日期设置后重试。"},
    {"1004", "软件数据异常，请重新下载官方安装包后重试。"},
    {"1005", "当前软件版本与本服务器不匹配，请更新软件后重试。"},
    {"1006", "服务器正在维护，请稍后再试。"},
    {"2001", "账号不存在，请先注册后再登录。"},
    {"2002", "该邮箱已注册过本软件，请直接登录或使用其他邮箱注册。"},
    {"2003", "账号已被停止使用。"},
    {"2004", "授权已到期，续费后即可继续使用。"},
    {"2005", "该账号还没有开通授权，可以先免费试用，或输入购买的序列号激活。"},
    {"3001", "本机机器码获取异常，请重启软件后重试。"},
    {"3002", "这台电脑已被其他账号绑定，请换回原账号或联系客服处理。"},
    {"4001", "本账号绑定的不是这台电脑。"},
    {"4002", "该账号的换机次数已用完，请联系客服处理。"},
    {"4003", "序列号与该账号不匹配，请填写本账号激活过的序列号。"},
    {"4004", "验证失败次数过多，请稍后再试或联系客服。"},
    {"5001", "序列号不正确，请核对后重新输入。"},
    {"5002", "该序列号已经使用过，请核对是否输入错误或联系卖家确认。"},
    {"5003", "该序列号已作废，请联系卖家或客服处理。"},
    {"5004", "该序列号不适用于本软件，请核对后重新输入。"},
    {"5005", "验证码错误，请重新输入。"},
    {"5006", "验证码已过期，请刷新后重新输入。"},
    {"5007", "请先获取验证码后再提交。"},
    {"6001", "您已经试用过本软件，购买正式版即可继续使用。"},
    {"7001", "该版本渠道已停止服务，请联系客服。"},
    {"7002", "当前版本不支持自助注册，请联系客服开通。"},
    {"9001", "服务器繁忙，请稍后再试。"},
    {"9998", "无法连接授权服务器，请检查网络是否可用，或本软件是否被防火墙拦截。"},
    {"9999", "服务暂时不可用，请稍后再试或联系客服。"},
};

std::string LocalCodeText(const std::string& code)
{
    for (size_t i = 0; i < sizeof(kCodes) / sizeof(kCodes[0]); ++i) {
        if (code == kCodes[i].code) return GbkToUtf8(kCodes[i].text);
    }
    return GbkToUtf8("未知错误（") + code + GbkToUtf8("）");
}

// ---------------- 机器码校验（与后端 app/Core/MachineCode.php 同规则） ----------------

static bool B64DecodeInto(const std::string& text, std::string* out)
{
    std::vector<unsigned char> raw = Base64Decode(text);
    if (raw.empty()) return false;
    out->assign((const char*)&raw[0], raw.size());
    return true;
}

static void SplitDash(const std::string& s, std::vector<std::string>* parts)
{
    size_t start = 0;
    for (;;) {
        size_t pos = s.find('-', start);
        if (pos == std::string::npos) { parts->push_back(s.substr(start)); break; }
        parts->push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
}

static bool IsDigits(const std::string& s)
{
    if (s.empty()) return false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (!isdigit((unsigned char)s[i])) return false;
    }
    return true;
}

static unsigned int Crc32Str(const std::string& s)
{
    static unsigned int table[256];
    static bool built = false;
    if (!built) {
        for (unsigned int i = 0; i < 256; ++i) {
            unsigned int c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        built = true;
    }
    unsigned int c = 0xFFFFFFFFu;
    for (size_t i = 0; i < s.size(); ++i) c = table[(c ^ (unsigned char)s[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

// 返回错误码文本："0" 通过，其余为说明
static std::string CheckMachineCode(const std::string& mac, const std::string& expectPuc)
{
    std::string raw;
    if (!B64DecodeInto(TrimStr(mac), &raw)) return GbkToUtf8("机器码不是合法的 base64 串");
    std::vector<std::string> p;
    SplitDash(raw, &p);
    if (p.size() < 5) return GbkToUtf8("机器码段数不足");
    if (!IsDigits(p[0]) || !IsDigits(p[1]) || p[2].empty()) return GbkToUtf8("机器码数字段异常");

    const int len = (int)(p[1].size() + p[2].size());
    char left[8];
    wsprintfA(left, "%d", len);
    left[2] = 0;                       // 取前两位（不足两位时就是整个长度值）
    if (p[0] != left) return GbkToUtf8("机器码左校验位不符");

    long rightVal = (long)(len * atol(p[0].c_str()));
    if (rightVal < 0) rightVal = -rightVal;
    char rightBuf[16];
    wsprintfA(rightBuf, "%ld", rightVal);
    const std::string rightAll(rightBuf);
    const std::string right = rightAll.size() > 2 ? rightAll.substr(rightAll.size() - 2) : rightAll;

    const bool v2 = (p[p.size() - 1] == "MACVER2");
    if (v2) {
        char lenTxt[16];
        wsprintfA(lenTxt, "%d", len);
        if (p[3] != lenTxt) return GbkToUtf8("机器码长度段不符");
        if (p[5] != right) return GbkToUtf8("机器码右校验位不符");
    } else {
        char crc[16];
        wsprintfA(crc, "%08X", (unsigned int)Crc32Str(p[1] + p[2]));
        if (p[3] != crc) return GbkToUtf8("机器码校验和不符");
        if (p[5] != right) return GbkToUtf8("机器码右校验位不符");
    }
    if (!expectPuc.empty() && p[4] != expectPuc) return GbkToUtf8("机器码产品类型与本软件不一致");
    return "0";
}

static std::string ParseMachineField(const std::string& mac, const std::string& field)
{
    std::string raw;
    if (!B64DecodeInto(TrimStr(mac), &raw)) return std::string();
    std::vector<std::string> p;
    SplitDash(raw, &p);
    if (p.size() < 5) return std::string();
    if (field == "raw") return raw;
    if (field == "puc_type") return p[4];
    if (field == "cpuid") return p.size() > 2 ? p[2] : std::string();
    if (field == "disk") return p.size() > 1 ? p[1] : std::string();
    if (field == "version") return (p.back() == "MACVER2") ? "2" : "1";
    if (field == "host_crc") {
        if (p.size() >= 7 && p.back() == "MACVER2") return p[6];
        return std::string();
    }
    return std::string();
}

// ==================== 命令实现 ====================

// 会员_初始化(服务器接口地址, 产品类型, 代理KEY, 机器码, 程序版本, 程序MD5, 系统版本, 协议密钥, 本地账号, 超时毫秒, 调试模式)
void cxah_Init(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    State& st = S();
    st.server    = TrimStr(ArgStr(pArgInf, 0, nArgCount));
    st.pucType   = TrimStr(ArgStr(pArgInf, 1, nArgCount));
    st.channelKey= TrimStr(ArgStr(pArgInf, 2, nArgCount));
    st.macKey    = TrimStr(ArgStr(pArgInf, 3, nArgCount));
    st.version   = TrimStr(ArgStr(pArgInf, 4, nArgCount));
    st.appMd5    = TrimStr(ArgStr(pArgInf, 5, nArgCount));
    st.systemVer = TrimStr(ArgStr(pArgInf, 6, nArgCount));
    st.secret    = ArgStr(pArgInf, 7, nArgCount);
    const std::string account = TrimStr(ArgStr(pArgInf, 8, nArgCount));
    st.timeoutMs = ArgInt(pArgInf, 9, nArgCount, 20000);
    st.debug     = ArgBool(pArgInf, 10, nArgCount, false);
    st.allowLogout = ArgBool(pArgInf, 11, nArgCount, false);

    if (st.server.empty() || st.secret.empty() || st.pucType.empty()) {
        SetApiError("1001", GbkToUtf8("初始化缺少服务器地址、协议密钥或产品类型。"));
        RetInt(pRetData, 1);
        return;
    }
    if (st.macKey.empty()) {
        SetApiError("3001", GbkToUtf8("初始化未传入机器码。"));
        RetInt(pRetData, 3);
        return;
    }
    st.channelKey = st.channelKey.empty() ? "cxangsoft" : st.channelKey;

    // 先校时，再载入渠道；失败不阻断，宿主可按需重试
    DoSyncTime();
    DoLoadOem();
    if (!account.empty()) st.email = LowerStr(account);
    else st.email = LowerStr(LoadAccount());

    LogLine("init ok server=" + st.server + " puc=" + st.pucType);
    RetInt(pRetData, 0);
}

void cxah_CodeText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    const std::string code = TrimStr(ArgStr(pArgInf, 0, nArgCount));
    const std::string fromServer = (!S().lastMsg.empty() && S().lastCode == code) ? S().lastMsg : std::string();
    RetText(pRetData, fromServer.empty() ? LocalCodeText(code) : fromServer);
}

void cxah_LastCode(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetText(pRetData, S().lastCode.empty() ? "0" : S().lastCode);
}

void cxah_LastMsg(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetText(pRetData, S().lastMsg.empty() ? LocalCodeText(S().lastCode) : S().lastMsg);
}

void cxah_SetAccount(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    const std::string email = LowerStr(TrimStr(ArgStr(pArgInf, 0, nArgCount)));
    SaveAccount(email);
    RetBool(pRetData, true);
}

void cxah_LoadAccount(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetText(pRetData, LoadAccount());
}

void cxah_OemField(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RetText(pRetData, S().oem.field(TrimStr(ArgStr(pArgInf, 0, nArgCount))));
}

void cxah_SoftSetting(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RetText(pRetData, SoftSettingOf(TrimStr(ArgStr(pArgInf, 0, nArgCount)), ArgStr(pArgInf, 1, nArgCount)));
}

void cxah_LoadOem(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetText(pRetData, DoLoadOem());
}

void cxah_SyncTime(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetText(pRetData, DoSyncTime());
}

void cxah_BeijingTime(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetText(pRetData, NowBeijingText());
}

void cxah_Register(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RetText(pRetData, DoRegister(ArgStr(pArgInf, 0, nArgCount), ArgStr(pArgInf, 1, nArgCount),
                                 ArgStr(pArgInf, 2, nArgCount), ArgStr(pArgInf, 3, nArgCount),
                                 ArgStr(pArgInf, 4, nArgCount), ArgStr(pArgInf, 5, nArgCount),
                                 ArgStr(pArgInf, 6, nArgCount), ArgStr(pArgInf, 7, nArgCount)));
}

void cxah_Trial(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    const std::string email = ArgStr(pArgInf, 0, nArgCount);
    RetText(pRetData, DoTrial(email.empty() ? LoadAccount() : email));
}

void cxah_Login(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    const std::string email = ArgStr(pArgInf, 0, nArgCount);
    RetText(pRetData, DoLogin(email.empty() ? LoadAccount() : email));
}

void cxah_Refresh(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    const std::string email = ArgStr(pArgInf, 0, nArgCount);
    RetText(pRetData, DoRefresh(email.empty() ? LoadAccount() : email));
}

void cxah_Activate(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    std::string email = ArgStr(pArgInf, 0, nArgCount);
    if (email.empty()) email = LoadAccount();
    RetText(pRetData, DoActivate(email, ArgStr(pArgInf, 1, nArgCount),
                                 ArgStr(pArgInf, 2, nArgCount), ArgStr(pArgInf, 3, nArgCount)));
}

void cxah_ChangeProbe(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    std::string email = ArgStr(pArgInf, 0, nArgCount);
    if (email.empty()) email = LoadAccount();
    RetText(pRetData, DoChangeProbe(email));
}

void cxah_ChangeConfirm(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    std::string email = ArgStr(pArgInf, 0, nArgCount);
    if (email.empty()) email = LoadAccount();
    std::string req = ArgStr(pArgInf, 4, nArgCount);
    if (req.empty()) req = S().requestId;
    RetText(pRetData, DoChangeConfirm(email, ArgStr(pArgInf, 1, nArgCount),
                                      ArgStr(pArgInf, 2, nArgCount), ArgStr(pArgInf, 3, nArgCount), req));
}

void cxah_QuitEvent(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    DoSendQuitEvent();
    RetBool(pRetData, true);
}

void cxah_Logout(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    FireEvent(EV_LOGOUT, "{}");
    State& st = S();
    st.grant = Grant();
    st.email.clear();
    st.lastCode.clear();
    st.lastMsg.clear();
    SaveAccount("");
    RetBool(pRetData, true);
}

void cxah_IsAuthorized(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    const std::string status = S().grant.authStatus;
    RetBool(pRetData, status == "ok" || status == "trial");
}

void cxah_GrantField(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RetText(pRetData, S().grant.field(TrimStr(ArgStr(pArgInf, 0, nArgCount))));
}

void cxah_DueDate(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetText(pRetData, S().grant.dueDate);
}

void cxah_AuthStatus(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetText(pRetData, S().grant.authStatus);
}

void cxah_ChangeLeft(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetInt(pRetData, S().grant.changeLeft);
}

void cxah_HasFeature(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RetBool(pRetData, HasFeatureCode(S().grant.softLimit, TrimStr(ArgStr(pArgInf, 0, nArgCount))));
}

void cxah_Attr(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    const std::string key = TrimStr(ArgStr(pArgInf, 0, nArgCount));
    RetText(pRetData, key.empty() ? S().grant.attrJson : S().grant.attrField(key));
}

void cxah_ShowUserCenter(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    const std::string email = ArgStr(pArgInf, 0, nArgCount);
    RetBool(pRetData, Ui_ShowUserCenter(email, S().hParent));
}

void cxah_Features(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetText(pRetData, S().grant.featuresText());
}

void cxah_SetEventWindow(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    const HWND h = (HWND)(INT_PTR)ArgInt(pArgInf, 0, nArgCount, 0);
    const UINT msg = (UINT)ArgInt(pArgInf, 1, nArgCount, 0);
    RetBool(pRetData, EventSetWindow(h, msg));
}

void cxah_SetEventCallback(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    const INT ptr = ArgInt(pArgInf, 0, nArgCount, 0);
    RetBool(pRetData, EventSetCallback(ptr == 0 ? NULL : (PFN_CX_EVENT)(INT_PTR)ptr));
}

void cxah_PollEvent(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    std::string json;
    RetText(pRetData, EventPoll(&json) ? json : std::string());
}

void cxah_EventCount(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetInt(pRetData, EventCount());
}

void cxah_ClearEvents(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    EventClear();
    RetBool(pRetData, true);
}

void cxah_EventName(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RetText(pRetData, GbkToUtf8(EventName(ArgInt(pArgInf, 0, nArgCount, 0))));
}

void cxah_StartAutoRefresh(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    const int sec = ArgInt(pArgInf, 0, nArgCount, 300);
    const HWND h = (HWND)(INT_PTR)ArgInt(pArgInf, 1, nArgCount, 0);
    const UINT msg = (UINT)ArgInt(pArgInf, 2, nArgCount, 0);
    RetBool(pRetData, AutoStart(sec, h, msg));
}

void cxah_StopAutoRefresh(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    AutoStop();
    RetBool(pRetData, true);
}

void cxah_AutoState(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetInt(pRetData, AutoInterval());
}

void cxah_ShowRegister(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    const int page = ArgInt(pArgInf, 0, nArgCount, 0);
    RetBool(pRetData, Ui_ShowMemberWindow(page));
}

void cxah_ShowRenew(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetBool(pRetData, Ui_ShowRenewWindow());
}

void cxah_ShowChange(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    std::string email = ArgStr(pArgInf, 0, nArgCount);
    if (email.empty()) email = LoadAccount();
    RetText(pRetData, Ui_ShowChangeWindow(email));
}

void cxah_CaptchaBase64(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    std::string id, png;
    if (!FetchCaptcha(&id, &png)) { RetText(pRetData, ""); return; }
    RetText(pRetData, png);
}

void cxah_CaptchaId(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetText(pRetData, S().captchaId);
}

void cxah_CheckMachine(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RetText(pRetData, CheckMachineCode(ArgStr(pArgInf, 0, nArgCount), ArgStr(pArgInf, 1, nArgCount)));
}

void cxah_ParseMachine(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    RetText(pRetData, ParseMachineField(ArgStr(pArgInf, 0, nArgCount), LowerStr(TrimStr(ArgStr(pArgInf, 1, nArgCount)))));
}

void cxah_RawRequest(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    const std::string route = TrimStr(ArgStr(pArgInf, 0, nArgCount));
    std::string body = ArgStr(pArgInf, 1, nArgCount);
    if (body.empty()) body = "{}";
    const ApiResponse res = ApiCall(route, body);
    RetText(pRetData, RawResponseJson(res));
}

void cxah_LibVersion(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetText(pRetData, "1.1.0");
}

// ---------------- 错误类别与提醒策略 ----------------

void cxah_ErrCategory(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetInt(pRetData, ErrorCategory(S().lastCode));
}

void cxah_MarkHandled(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    MarkEventHandled(ArgInt(pArgInf, 0, nArgCount, 0));
    RetBool(pRetData, true);
}

void cxah_SetAlertPolicy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    SetAlertPolicy(ArgBool(pArgInf, 0, nArgCount, true), ArgInt(pArgInf, 1, nArgCount, GetExpireWarnDays()));
    RetBool(pRetData, true);
}

void cxah_DaysLeft(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetInt(pRetData, DaysToExpire());
}

void cxah_IsAutoAlert(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetBool(pRetData, IsAutoAlertOn());
}

void cxah_WarnDays(PMDATA_INF pRetData, INT, PMDATA_INF)
{
    RetInt(pRetData, GetExpireWarnDays());
}

// ==================== 会员管理 类（库定义数据类型） ====================
// 授权数据（账号、渠道、事件队列、自动刷新线程）是进程内单例，接口层与界面共用它；
// 对象本身只带两件事：事件往哪儿发、自动刷新是不是我起的。
// 因此多个对象看到的是同一份授权 —— 这与「一个进程一套授权」的实际形态一致。
struct CxMemberObj
{
    HWND hEventWnd;
    UINT msgId;
    PFN_CX_EVENT cb;
    bool autoOwned;
    CxMemberObj() : hEventWnd(NULL), msgId(0), cb(NULL), autoOwned(false) {}
};

static CxMemberObj* MObj(PMDATA_INF pArgInf)
{
    if (pArgInf[0].m_pCompoundData == NULL) return NULL;
    return (CxMemberObj*)(INT_PTR) * (PINT)pArgInf[0].m_pCompoundData;
}

// 对象槽位可能已被宿主清零（复制半途失败等），取不到就补一个，别让方法直接崩。
static CxMemberObj* NeedObj(PMDATA_INF pArgInf)
{
    CxMemberObj* o = MObj(pArgInf);
    if (o == NULL && pArgInf[0].m_pCompoundData != NULL) {
        o = new CxMemberObj();
        *(PINT)pArgInf[0].m_pCompoundData = (INT)(INT_PTR)o;
    }
    return o;
}

// 成员命令的 pArgInf[0] 是对象自身，用户参数从 [1] 起。
static std::string EmailArg(PMDATA_INF p, INT n)
{
    const std::string email = ArgStr(p, 1, n);
    return email.empty() ? LoadAccount() : email;
}

static bool HasFeatureCode(const std::string& list, const std::string& want)
{
    if (want.empty()) return false;
    size_t start = 0;
    while (start <= list.size()) {
        size_t pos = list.find(',', start);
        if (pos == std::string::npos) pos = list.size();
        if (TrimStr(list.substr(start, pos - start)) == want) return true;
        start = pos + 1;
    }
    return false;
}

static std::string SoftSettingOf(const std::string& key, const std::string& def)
{
    if (S().oem.settingRaw.empty()) return def;
    JVal setting;
    if (!JsonParse(S().oem.settingRaw, setting) || setting.type != JVal::TObj) return def;
    const JVal* v = setting.find(TrimStr(key));
    return v == NULL ? def : v->asStr(def.c_str());
}

static std::string RawResponseJson(const ApiResponse& res)
{
    if (!res.raw.empty()) return res.raw;
    return std::string("{\"ok\":false,\"code\":\"") + res.code + "\",\"msg\":\"" + JsonEscape(res.msg) + "\"}";
}


void cxahM_Construct(PMDATA_INF, INT, PMDATA_INF p)
{
    // 构造函数执行时对象内容是全零，槽位要自己填
    if (p[0].m_pCompoundData != NULL) {
        *(PINT)p[0].m_pCompoundData = (INT)(INT_PTR) new CxMemberObj();
    }
}

void cxahM_Copy(PMDATA_INF, INT n, PMDATA_INF p)
{
    // 默认浅拷会让两个对象析构时释放同一个指针，必须自己 new 一份
    CxMemberObj* src = (n > 1 && p[1].m_pCompoundData != NULL)
                           ? (CxMemberObj*)(INT_PTR) * (PINT)p[1].m_pCompoundData : NULL;
    CxMemberObj* dst = new CxMemberObj();
    if (src != NULL) *dst = *src;
    if (p[0].m_pCompoundData != NULL) *(PINT)p[0].m_pCompoundData = (INT)(INT_PTR)dst;
}

void cxahM_Destruct(PMDATA_INF, INT, PMDATA_INF p)
{
    CxMemberObj* o = MObj(p);
    if (o == NULL) return;
    if (o->autoOwned) AutoStop();          // 谁起的自动刷新谁收，宿主不用记得停
    delete o;
    *(PINT)p[0].m_pCompoundData = 0;
}

void cxahM_Init(PMDATA_INF r, INT n, PMDATA_INF p)
{
    State& st = S();
    st.server      = TrimStr(ArgStr(p, 1, n));
    st.pucType     = TrimStr(ArgStr(p, 2, n));
    st.channelKey  = TrimStr(ArgStr(p, 3, n));
    st.macKey      = TrimStr(ArgStr(p, 4, n));
    st.version     = TrimStr(ArgStr(p, 5, n));
    st.appMd5      = TrimStr(ArgStr(p, 6, n));
    st.systemVer   = TrimStr(ArgStr(p, 7, n));
    st.secret      = ArgStr(p, 8, n);
    const std::string account = TrimStr(ArgStr(p, 9, n));
    st.timeoutMs   = ArgInt(p, 10, n, 20000);
    st.debug       = ArgBool(p, 11, n, false);
    st.allowLogout = ArgBool(p, 12, n, false);
    if (st.server.empty() || st.secret.empty() || st.pucType.empty()) {
        SetApiError("1001", GbkToUtf8("初始化缺少服务器地址、协议密钥或产品类型。"));
        RetInt(r, 1);
        return;
    }
    if (st.macKey.empty()) {
        SetApiError("3001", GbkToUtf8("初始化未传入机器码。"));
        RetInt(r, 3);
        return;
    }
    st.channelKey = st.channelKey.empty() ? "cxangsoft" : st.channelKey;
    DoSyncTime();
    DoLoadOem();
    st.email = account.empty() ? LowerStr(LoadAccount()) : LowerStr(account);
    RetInt(r, 0);
}

void cxahM_Login(PMDATA_INF r, INT n, PMDATA_INF p)     { RetText(r, DoLogin(EmailArg(p, n))); }
void cxahM_Trial(PMDATA_INF r, INT n, PMDATA_INF p)     { RetText(r, DoTrial(EmailArg(p, n))); }
void cxahM_Refresh(PMDATA_INF r, INT n, PMDATA_INF p)   { RetText(r, DoRefresh(EmailArg(p, n))); }
void cxahM_Register(PMDATA_INF r, INT n, PMDATA_INF p)
{
    RetText(r, DoRegister(ArgStr(p, 1, n), ArgStr(p, 2, n), ArgStr(p, 3, n), ArgStr(p, 4, n),
                          ArgStr(p, 5, n), ArgStr(p, 6, n), ArgStr(p, 7, n), ArgStr(p, 8, n)));
}
void cxahM_Activate(PMDATA_INF r, INT n, PMDATA_INF p)
{
    RetText(r, DoActivate(EmailArg(p, n), ArgStr(p, 2, n), ArgStr(p, 3, n), ArgStr(p, 4, n)));
}
void cxahM_ChangeProbe(PMDATA_INF r, INT n, PMDATA_INF p)   { RetText(r, DoChangeProbe(EmailArg(p, n))); }
void cxahM_ChangeConfirm(PMDATA_INF r, INT n, PMDATA_INF p)
{
    std::string req = ArgStr(p, 5, n);
    if (req.empty()) req = S().requestId;
    RetText(r, DoChangeConfirm(EmailArg(p, n), ArgStr(p, 2, n), ArgStr(p, 3, n), ArgStr(p, 4, n), req));
}
void cxahM_QuitEvent(PMDATA_INF r, INT, PMDATA_INF)          { DoSendQuitEvent(); RetBool(r, true); }
void cxahM_Logout(PMDATA_INF r, INT, PMDATA_INF)
{
    FireEvent(EV_LOGOUT, "{}");
    State& st = S();
    st.grant = Grant();
    st.email.clear();
    st.lastCode.clear();
    st.lastMsg.clear();
    SaveAccount("");
    RetBool(r, true);
}

void cxahM_OpenMemberWindow(PMDATA_INF r, INT n, PMDATA_INF p) { RetBool(r, Ui_ShowMemberWindow(ArgInt(p, 1, n, 0))); }
void cxahM_OpenKeyWindow(PMDATA_INF r, INT, PMDATA_INF)        { RetBool(r, Ui_ShowRenewWindow()); }
void cxahM_OpenUserCenter(PMDATA_INF r, INT n, PMDATA_INF p)   { RetBool(r, Ui_ShowUserCenter(EmailArg(p, n), S().hParent)); }
void cxahM_OpenChangeWindow(PMDATA_INF r, INT n, PMDATA_INF p) { RetText(r, Ui_ShowChangeWindow(EmailArg(p, n))); }

void cxahM_BindEventWindow(PMDATA_INF r, INT n, PMDATA_INF p)
{
    CxMemberObj* o = NeedObj(p);
    const HWND h = (HWND)(INT_PTR)ArgInt(p, 1, n, 0);
    const UINT msg = (UINT)ArgInt(p, 2, n, 0);
    if (o != NULL) { o->hEventWnd = h; o->msgId = msg; }
    RetBool(r, EventSetWindow(h, msg));
}
void cxahM_BindEventCallback(PMDATA_INF r, INT n, PMDATA_INF p)
{
    CxMemberObj* o = NeedObj(p);
    const INT ptr = ArgInt(p, 1, n, 0);
    PFN_CX_EVENT cb = ptr == 0 ? NULL : (PFN_CX_EVENT)(INT_PTR)ptr;
    if (o != NULL) o->cb = cb;
    RetBool(r, EventSetCallback(cb));
}
void cxahM_PollEvent(PMDATA_INF r, INT, PMDATA_INF)
{
    std::string json;
    RetText(r, EventPoll(&json) ? json : std::string());
}
void cxahM_EventCount(PMDATA_INF r, INT, PMDATA_INF)          { RetInt(r, EventCount()); }
void cxahM_ClearEvents(PMDATA_INF r, INT, PMDATA_INF)         { EventClear(); RetBool(r, true); }
void cxahM_EventName(PMDATA_INF r, INT n, PMDATA_INF p)       { RetText(r, GbkToUtf8(EventName(ArgInt(p, 1, n, 0)))); }
void cxahM_MarkHandled(PMDATA_INF r, INT n, PMDATA_INF p)
{
    MarkEventHandled(ArgInt(p, 1, n, 0));
    RetBool(r, true);
}

void cxahM_StartAutoRefresh(PMDATA_INF r, INT n, PMDATA_INF p)
{
    CxMemberObj* o = NeedObj(p);
    const HWND h = (HWND)(INT_PTR)ArgInt(p, 2, n, o != NULL ? (INT)(INT_PTR)o->hEventWnd : 0);
    const UINT msg = (UINT)ArgInt(p, 3, n, o != NULL ? (int)o->msgId : 0);
    const bool on = AutoStart(ArgInt(p, 1, n, 300), h, msg);
    if (o != NULL) o->autoOwned = on;
    RetBool(r, on);
}
void cxahM_StopAutoRefresh(PMDATA_INF r, INT, PMDATA_INF)     { AutoStop(); RetBool(r, true); }
void cxahM_AutoInterval(PMDATA_INF r, INT, PMDATA_INF)        { RetInt(r, AutoInterval()); }

void cxahM_SetAlertPolicy(PMDATA_INF r, INT n, PMDATA_INF p)
{
    SetAlertPolicy(ArgBool(p, 1, n, true), ArgInt(p, 2, n, GetExpireWarnDays()));
    RetBool(r, true);
}
void cxahM_WarnDays(PMDATA_INF r, INT, PMDATA_INF)            { RetInt(r, GetExpireWarnDays()); }
void cxahM_IsAutoAlert(PMDATA_INF r, INT, PMDATA_INF)         { RetBool(r, IsAutoAlertOn()); }

void cxahM_IsAuthorized(PMDATA_INF r, INT, PMDATA_INF)        { RetBool(r, IsAuthUsable(S().grant.authStatus)); }
void cxahM_AuthStatus(PMDATA_INF r, INT, PMDATA_INF)          { RetText(r, S().grant.authStatus); }
void cxahM_DueDate(PMDATA_INF r, INT, PMDATA_INF)             { RetText(r, S().grant.dueDate); }
void cxahM_DaysLeft(PMDATA_INF r, INT, PMDATA_INF)            { RetInt(r, DaysToExpire()); }
void cxahM_Email(PMDATA_INF r, INT, PMDATA_INF)               { RetText(r, S().grant.email); }
void cxahM_Mac(PMDATA_INF r, INT, PMDATA_INF)                 { RetText(r, S().macKey); }
void cxahM_ChangeLeft(PMDATA_INF r, INT, PMDATA_INF)          { RetInt(r, S().grant.changeLeft); }
void cxahM_LastCode(PMDATA_INF r, INT, PMDATA_INF)            { RetText(r, S().lastCode); }
void cxahM_ErrCategory(PMDATA_INF r, INT, PMDATA_INF)         { RetInt(r, ErrorCategory(S().lastCode)); }
void cxahM_CodeText(PMDATA_INF r, INT n, PMDATA_INF p)
{
    const std::string code = TrimStr(ArgStr(p, 1, n));
    const std::string fromServer = (!S().lastMsg.empty() && S().lastCode == code) ? S().lastMsg : std::string();
    RetText(r, fromServer.empty() ? LocalCodeText(code) : fromServer);
}
void cxahM_HasFeature(PMDATA_INF r, INT n, PMDATA_INF p)
{
    RetBool(r, HasFeatureCode(S().grant.softLimit, TrimStr(ArgStr(p, 1, n))));
}
void cxahM_Features(PMDATA_INF r, INT, PMDATA_INF)            { RetText(r, S().grant.featuresText()); }
void cxahM_Attr(PMDATA_INF r, INT n, PMDATA_INF p)
{
    const std::string key = TrimStr(ArgStr(p, 1, n));
    RetText(r, key.empty() ? S().grant.attrJson : S().grant.attrField(key));
}
void cxahM_GrantField(PMDATA_INF r, INT n, PMDATA_INF p)      { RetText(r, S().grant.field(TrimStr(ArgStr(p, 1, n)))); }
void cxahM_OemField(PMDATA_INF r, INT n, PMDATA_INF p)        { RetText(r, S().oem.field(TrimStr(ArgStr(p, 1, n)))); }
void cxahM_SoftSetting(PMDATA_INF r, INT n, PMDATA_INF p)     { RetText(r, SoftSettingOf(ArgStr(p, 1, n), ArgStr(p, 2, n))); }
void cxahM_ServerTime(PMDATA_INF r, INT, PMDATA_INF)          { RetText(r, NowBeijingText()); }
void cxahM_ContextJson(PMDATA_INF r, INT, PMDATA_INF)
{
    // 「全局属性」在这里的落点：一次把授权上下文整体拿走，宿主不用逐条问
    std::string json = S().grant.field("json");
    if (json.empty()) json = S().grant.rawJson;
    RetText(r, json);
}
void cxahM_CaptchaImage(PMDATA_INF r, INT, PMDATA_INF)
{
    std::string id, png;
    RetText(r, FetchCaptcha(&id, &png) ? png : std::string());
}
void cxahM_CaptchaId(PMDATA_INF r, INT, PMDATA_INF)           { RetText(r, S().captchaId); }
void cxahM_CheckMachine(PMDATA_INF r, INT n, PMDATA_INF p)
{
    RetText(r, CheckMachineCode(ArgStr(p, 1, n), ArgStr(p, 2, n)));
}
void cxahM_ParseMachine(PMDATA_INF r, INT n, PMDATA_INF p)
{
    RetText(r, ParseMachineField(ArgStr(p, 1, n), LowerStr(TrimStr(ArgStr(p, 2, n)))));
}
void cxahM_RawRequest(PMDATA_INF r, INT n, PMDATA_INF p)
{
    std::string body = ArgStr(p, 2, n);
    if (body.empty()) body = "{}";
    RetText(r, RawResponseJson(ApiCall(TrimStr(ArgStr(p, 1, n)), body)));
}
void cxahM_LibVersion(PMDATA_INF r, INT, PMDATA_INF)          { RetText(r, "1.1.0"); }

// 静态编译只要命令实现：表与库信息用 __E_STATIC_LIB 排除。
#ifndef __E_STATIC_LIB
// ==================== 参数表 ====================

#define ARG(NAME, EXPLAIN, TYPE, STATE) \
    { _T(NAME), _T(EXPLAIN), 0, 0, TYPE, 0, STATE }

// 类成员命令：m_shtCategory 必须是 -1，且下标要进该类型的 m_pnCmdsIndex。
#define OBJ(NAME, EG, EXPLAIN, RET, ARG_COUNT, ARGS, FUNC) \
    { _T(NAME), _T(EG), _T(EXPLAIN), -1, _CMD_OS(__OS_WIN), RET, 0, LVL_SIMPLE, 0, 0, ARG_COUNT, (PARG_INFO)ARGS }

#define LIFE(NAME, EG, FLAGS, ARG_COUNT, ARGS) \
    { _T(NAME), _T(EG), _T("内部命令，由易语言自动调用，请勿手工调用。"), -1, _CMD_OS(__OS_WIN) | (FLAGS), _SDT_NULL, 0, LVL_SIMPLE, 0, 0, ARG_COUNT, (PARG_INFO)ARGS }

static ARG_INFO g_argInit[] = {
    ARG("服务器接口地址", "授权接口 api.php 的完整地址，末尾不带斜杠，如 https://host/api.php。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("产品类型", "本软件的产品代号，与后台渠道表 puc_type 一致，如 3。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("代理KEY", "渠道唯一KEY，官方版本传 cxangsoft，留空同样按官方处理。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("机器码", "由宿主生成的本机机器码，库内不生成机器码。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("程序版本", "本软件版本号，会写入登录记录。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("程序MD5", "主程序文件 MD5，用于后台核对版本。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("系统版本", "Windows 版本代号，整数文本。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("协议密钥", "与后端 config app_secret 完全一致的共享密钥。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("本地账号", "上次登录的邮箱，留空则读库内保存的账号。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("超时毫秒", "单次接口超时，默认 20000。", SDT_INT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("调试模式", "真则把接口过程写到调试器输出。", SDT_BOOL, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("允许退出账号", "真才在会员窗口底部显示「退出本机账号」按钮；默认假不显示，命令 会员_退出本机账号 不受影响。", SDT_BOOL, AS_DEFAULT_VALUE_IS_EMPTY),
};

static ARG_INFO g_argCodeText[] = { ARG("错误码", "接口返回的错误码。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY) };
static ARG_INFO g_argAccount[]    = { ARG("邮箱", "要保存的账号邮箱。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY) };
static ARG_INFO g_argField[]      = { ARG("字段名", "字段名称，见命令说明。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY) };
static ARG_INFO g_argSetting[]    = {
    ARG("键名", "渠道程序配置里的键名。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("默认值", "取不到时返回的文本。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
};
static ARG_INFO g_argEmail[]      = { ARG("邮箱", "留空则使用本地保存的账号。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY) };
static ARG_INFO g_argRegister[]   = {
    ARG("邮箱", "注册邮箱，服务端会转小写。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("QQ", "联系QQ。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("关联账号", "同一用户名下其它账号的邮箱。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("官网手机号", "官网注册手机号。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("了解渠道", "从哪里知道本软件的。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("备注", "备注信息。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("验证码ID", "会员_取验证码ID 的返回值，界面注册时留空由界面提交。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("验证码", "图形验证码内容。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
};
static ARG_INFO g_argActivate[]   = {
    ARG("邮箱", "留空使用本地账号。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("序列号", "购买的激活序列号。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("验证码ID", "会员_取验证码ID 的返回值。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("验证码", "图形验证码内容。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
};
static ARG_INFO g_argChangeConfirm[] = {
    ARG("邮箱", "留空使用本地账号。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("序列号", "本账号以前激活过的序列号。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("验证码ID", "会员_取验证码ID 的返回值。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("验证码", "图形验证码内容。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("换机申请号", "会员_申请换机 返回的申请号，留空自动取上次的。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
};
static ARG_INFO g_argPage[]       = { ARG("初始页", "0=登录/注册首页 1=登录账号 2=填写注册信息 3=开通授权 4=授权成功，其它值按 0 处理。序列号激活与换机验证是独立小窗口，不占页号。", SDT_INT, AS_DEFAULT_VALUE_IS_EMPTY) };
static ARG_INFO g_argEventWin[]   = {
    ARG("窗口句柄", "接收事件的窗口句柄，取易语言 窗口.取窗口句柄()。", SDT_INT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("消息号", "自定义消息号，填 1024 即 WM_USER。wParam 是事件码。", SDT_INT, AS_DEFAULT_VALUE_IS_EMPTY),
};
static ARG_INFO g_argEventCb[]    = { ARG("回调子程序", "形如 子程序(整数型 事件码) 的子程序指针，传 0 取消。回调只在登记它的线程上触发。", SDT_INT, AS_DEFAULT_VALUE_IS_EMPTY) };
static ARG_INFO g_argEventCode[]  = { ARG("事件码", "事件 JSON 里的 e 字段。", SDT_INT, AS_DEFAULT_VALUE_IS_EMPTY) };
// 复制函数的参数是本类型自己；序号 1 = g_dataTypes 里的第一个类。
static ARG_INFO g_argCopyMember[] = {
    { _T("来源会员对象"), _T("要复制的会员管理对象"), 0, 0, MAKELONG(1, 0), 0, AS_RECEIVE_VAR },
};
static ARG_INFO g_argAlert[] = {
    ARG("自动提醒", "真则宿主没接管事件时由库直接弹窗；传假全部交给宿主自己处理。", SDT_BOOL, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("到期前天数", "还剩几天时提醒，0 表示关掉到期提醒。", SDT_INT, AS_DEFAULT_VALUE_IS_EMPTY),
};
static ARG_INFO g_argAuto[]       = {
    ARG("间隔秒", "多久刷新一次授权，最小 5 秒，默认 300 秒。", SDT_INT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("窗口句柄", "事件发往的窗口句柄，留空沿用之前设置的。", SDT_INT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("消息号", "事件消息号，留空沿用之前设置的。", SDT_INT, AS_DEFAULT_VALUE_IS_EMPTY),
};
static ARG_INFO g_argCheckMac[]   = {
    ARG("机器码", "待校验的机器码。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("产品类型", "要求机器码里声明的产品代号，留空不比对。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
};
static ARG_INFO g_argParseMac[]   = {
    ARG("机器码", "机器码原文。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("字段名", "raw 原文 / puc_type 产品类型 / cpuid CPU序列号 / disk 硬盘段 / host_crc 计算机名校验 / version 版本号。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
};
static ARG_INFO g_argFeature[]    = { ARG("功能码", "增配权限码。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY) };
static ARG_INFO g_argRaw[]        = {
    ARG("路由", "接口路由名，如 auth.login。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
    ARG("业务JSON", "UTF-8 的业务参数 JSON，留空按 {} 处理。", SDT_TEXT, AS_DEFAULT_VALUE_IS_EMPTY),
};

// ==================== 命令表 ====================
// 只能往末尾追加：改名、调序、删项都会让老程序按序号调用时错乱。

#define CMD(NAME, EG, EXPLAIN, CATEGORY, RET, ARG_COUNT, ARGS, FUNC) \
    { _T(NAME), _T(EG), _T(EXPLAIN), CATEGORY, _CMD_OS(__OS_WIN), RET, 0, LVL_SIMPLE, 0, 0, ARG_COUNT, (PARG_INFO)ARGS }

static CMD_INFO g_commands[] = {
    CMD("会员_初始化", "CxahInit",
        "载入授权模块。必须最先调用。机器码、程序MD5、版本、系统版本等库内取不到的信息由宿主传入。返回 0 成功，1 参数缺失，3 机器码为空。",
        1, SDT_INT, 12, g_argInit, cxah_Init),
    CMD("会员_取错误码文本", "CxahCodeText", "取错误码对应的中文说明；若上一次接口给了服务器文案则优先用服务器文案。", 1, SDT_TEXT, 1, g_argCodeText, cxah_CodeText),
    CMD("会员_取服务器错误码", "CxahLastCode", "取最近一次接口返回的错误码，0 表示成功。", 1, SDT_TEXT, 0, NULL, cxah_LastCode),
    CMD("会员_取服务器文案", "CxahLastMsg", "取最近一次服务器返回的用户提示文本，界面直接显示。", 1, SDT_TEXT, 0, NULL, cxah_LastMsg),
    CMD("会员_设置当前账号", "CxahSetAccount", "保存本机登录邮箱（写注册表 HKCU\\Software\\CxSoft\\CxAhMember\\产品类型）。", 1, SDT_BOOL, 1, g_argAccount, cxah_SetAccount),
    CMD("会员_取本地账号", "CxahLoadAccount", "取本机保存的登录邮箱，没有则返回空文本。", 1, SDT_TEXT, 0, NULL, cxah_LoadAccount),
    CMD("会员_载入渠道信息", "CxahLoadOem", "从服务器载入渠道（品牌、客服、网址等），返回错误码文本，0 为成功。", 1, SDT_TEXT, 0, NULL, cxah_LoadOem),
    CMD("会员_取OEM信息", "CxahOemField", "取渠道字段：channel_id key brand_name soft_name puc_type theme_color qq qq2 wechat phone home shop_home help_home download_home change_home buy_text ad_url ad_url2 server_url server_url2 official able_reg protection setting。", 1, SDT_TEXT, 1, g_argField, cxah_OemField),
    CMD("会员_取程序配置", "CxahSoftSetting", "取渠道 setting 里的程序配置项，取不到返回默认值。", 1, SDT_TEXT, 2, g_argSetting, cxah_SoftSetting),
    CMD("会员_同步服务器时间", "CxahSyncTime", "取服务器时间并校准本机偏移，之后所有到期判断都用校准时间，用户改系统时间无效。返回错误码文本。", 1, SDT_TEXT, 0, NULL, cxah_SyncTime),
    CMD("会员_取当前北京时间", "CxahBeijingTime", "返回校准后的当前时间，形如 2026-10-18 21:30:00。", 1, SDT_TEXT, 0, NULL, cxah_BeijingTime),

    CMD("会员_注册账号", "CxahRegister", "注册新账号并绑定当前机器码，需要图形验证码。返回错误码文本，0 成功，2002 已注册，5005 验证码错误。", 2, SDT_TEXT, 8, g_argRegister, cxah_Register),
    CMD("会员_开通试用", "CxahTrial", "为账号开通一次免费试用。返回错误码文本，0 成功，6001 已试用过。", 2, SDT_TEXT, 1, g_argEmail, cxah_Trial),
    CMD("会员_登录", "CxahLogin", "邮箱加本机机器码登录，成功即取得授权。返回错误码文本：0 通过；4001 机器码不符但可换机（配合会员_显示换机窗口）；4002 换机次数不足；2001 账号不存在；2004 已到期；2005 未开通授权。", 2, SDT_TEXT, 1, g_argEmail, cxah_Login),
    CMD("会员_刷新授权", "CxahRefresh", "只刷新授权信息，不写登录记录，适合运行中定时刷新。返回错误码文本。", 2, SDT_TEXT, 1, g_argEmail, cxah_Refresh),
    CMD("会员_激活序列号", "CxahActivate", "输入序列号开通或续费授权，需要图形验证码。返回错误码文本，0 成功，5001 序列号不存在，5002 已被使用，5004 不适用本软件。", 2, SDT_TEXT, 4, g_argActivate, cxah_Activate),
    CMD("会员_申请换机", "CxahChangeProbe", "本机机器码与账号绑定不一致时先调用，返回 0 表示可以换机（会记录申请号与剩余次数），4002 表示次数已用完。", 2, SDT_TEXT, 1, g_argEmail, cxah_ChangeProbe),
    CMD("会员_换机验证", "CxahChangeConfirm", "提交本账号激活过的序列号与验证码，通过即换机并登录。返回错误码文本，0 成功，4003 序列号与账号不符，4004 失败过多被锁定。", 2, SDT_TEXT, 5, g_argChangeConfirm, cxah_ChangeConfirm),
    CMD("会员_发送退出事件", "CxahQuitEvent", "软件关闭前调用，上报下线时间。", 2, SDT_BOOL, 0, NULL, cxah_QuitEvent),
    CMD("会员_退出本机账号", "CxahLogout", "清除本机保存的账号与授权信息，不影响服务器数据。", 2, SDT_BOOL, 0, NULL, cxah_Logout),

    CMD("会员_是否已授权", "CxahIsAuthorized", "当前是否可用（正式或试用且未过期）。", 3, SDT_BOOL, 0, NULL, cxah_IsAuthorized),
    CMD("会员_取授权信息", "CxahGrantField", "取授权字段：uid email qq bind_mac mac_key state state_reason due_date reg_date last_login login_num function function_name soft_limit functions function_names order_number puc_type channel_id home_username remarks server_time limit_change change_time change_left is_trial is_expired authorized attr ip ip_location json。", 3, SDT_TEXT, 1, g_argField, cxah_GrantField),
    CMD("会员_取到期时间", "CxahDueDate", "授权到期时间，未开通为空。", 3, SDT_TEXT, 0, NULL, cxah_DueDate),
    CMD("会员_取授权状态", "CxahAuthStatus", "ok 正式 / trial 试用 / expired 到期 / frozen 冻结 / none 未开通。", 3, SDT_TEXT, 0, NULL, cxah_AuthStatus),
    CMD("会员_取剩余换机次数", "CxahChangeLeft", "账号还能换机的次数，界面可提前提示。", 3, SDT_INT, 0, NULL, cxah_ChangeLeft),
    CMD("会员_是否有功能权限", "CxahHasFeature", "判断增配权限串里是否包含指定功能码。", 3, SDT_BOOL, 1, g_argFeature, cxah_HasFeature),

    CMD("会员_显示注册窗口", "CxahShowRegister", "弹出会员向导窗口，含登录、注册、开通授权；登录遇到机器码不符会自动弹换机小窗口引导输入序列号。返回真表示已拿到授权。", 4, SDT_BOOL, 1, g_argPage, cxah_ShowRegister),
    CMD("会员_显示续费窗口", "CxahShowRenew", "弹独立的序列号激活小窗口（输序列号 + 图形验证码），用于到期续费。返回真表示已拿到授权。", 4, SDT_BOOL, 0, NULL, cxah_ShowRenew),
    CMD("会员_显示换机窗口", "CxahShowChange", "先申请换机，再弹独立的换机验证小窗口：输入本账号激活过的序列号与验证码，通过后自动换机并登录。返回错误码文本。", 4, SDT_TEXT, 1, g_argEmail, cxah_ShowChange),
    CMD("会员_取验证码图片", "CxahCaptchaBase64", "取一张新的图形验证码，返回 PNG 的 Base64 文本（同时更新验证码ID）。自行绘制界面时用本命令。", 4, SDT_TEXT, 0, NULL, cxah_CaptchaBase64),
    CMD("会员_取验证码ID", "CxahCaptchaId", "取最近一次验证码的ID，提交注册或激活时一起传。", 4, SDT_TEXT, 0, NULL, cxah_CaptchaId),

    CMD("会员_校验机器码", "CxahCheckMachine", "按服务端同规则校验机器码结构，返回 0 表示通过，其它为说明文本。", 5, SDT_TEXT, 2, g_argCheckMac, cxah_CheckMachine),
    CMD("会员_解析机器码", "CxahParseMachine", "解出机器码里的字段，便于后台排障与界面显示。", 5, SDT_TEXT, 2, g_argParseMac, cxah_ParseMachine),

    CMD("会员_请求接口", "CxahRawRequest", "通用逃生口：按路由名直连接口，返回服务器原始响应 JSON，方便试用新功能而不改支持库。", 6, SDT_TEXT, 2, g_argRaw, cxah_RawRequest),
    CMD("会员_库版本", "CxahLibVersion", "返回本支持库版本号。", 6, SDT_TEXT, 0, NULL, cxah_LibVersion),

    CMD("会员_设置事件窗口", "CxahSetEventWindow", "把会员事件用 PostMessage 发到指定窗口，wParam 是事件码。跨线程安全，推荐用法。", 7, SDT_BOOL, 2, g_argEventWin, cxah_SetEventWindow),
    CMD("会员_设置事件回调", "CxahSetEventCallback", "注册子程序指针作为事件回调，参数是事件码，返回 1 表示宿主已接管（库不再弹默认提醒），返回 0 交给库；传 0 取消登记。回调只在登记它的线程上触发，自动刷新线程产生的事件只走窗口消息与队列。", 7, SDT_BOOL, 1, g_argEventCb, cxah_SetEventCallback),
    CMD("会员_取事件", "CxahPollEvent", "从事件队列取最早的一条并弹出，返回 JSON 文本，字段为 e 事件码、n 名字、t 时间、d 详情；没有事件返回空文本。", 7, SDT_TEXT, 0, NULL, cxah_PollEvent),
    CMD("会员_取事件数", "CxahEventCount", "队列里还有多少条未取的事件。", 7, SDT_INT, 0, NULL, cxah_EventCount),
    CMD("会员_清空事件", "CxahClearEvents", "丢弃队列里全部未取的事件。", 7, SDT_BOOL, 0, NULL, cxah_ClearEvents),
    CMD("会员_取事件名", "CxahEventName", "事件码对应的中文说明，可直接显示。", 7, SDT_TEXT, 1, g_argEventCode, cxah_EventName),
    CMD("会员_启动自动刷新", "CxahStartAutoRefresh", "库内起线程按间隔刷新授权，授权到期/冻结/失效会发事件；重复调用只改间隔。返回真表示已在运行。", 7, SDT_BOOL, 3, g_argAuto, cxah_StartAutoRefresh),
    CMD("会员_停止自动刷新", "CxahStopAutoRefresh", "停掉自动刷新线程并等它退出。", 7, SDT_BOOL, 0, NULL, cxah_StopAutoRefresh),
    CMD("会员_取自动刷新间隔", "CxahAutoState", "返回当前自动刷新间隔秒数，0 表示未启动。", 7, SDT_INT, 0, NULL, cxah_AutoState),

    CMD("会员_显示用户中心", "CxahShowUserCenter", "弹用户中心窗口：看账号、机器码、授权版本、到期时间、剩余换机次数、已开通的选配功能，并可打开渠道购买页、刷新授权、激活序列号。返回真表示关闭时账号可用。", 4, SDT_BOOL, 1, g_argEmail, cxah_ShowUserCenter),
    CMD("会员_取选配功能", "CxahFeatures", "已开通的选配功能，每行「名称|到期时间|1」，末位 1 表示当前可用、0 表示已停用；没有则返回空文本。", 3, SDT_TEXT, 0, NULL, cxah_Features),

    CMD("会员_取附加信息", "CxahAttr", "旧模块 授权信息.附加信息 的口径：键名可填 cor_last_actdate（企业版序列号最后激活时间）、ac_function_catalog_duedate（类目采集到期）、ac_function_cc_duedate（僵尸采集到期）、ac_sp1、ac_sp2、ac_sp_max、ac_sp_v3_1、ac_sp_v3_2、ac_sp_v3_max（采集限速参数）；键名留空返回整段 JSON。", 3, SDT_TEXT, 1, g_argField, cxah_Attr),
    CMD("会员_取错误类别", "CxahErrCategory", "把最近一次错误码归成处置类别：0 无错误 1 网络不通 2 账号无效 3 账号被停止使用 4 已到期 5 未开通授权 6 需要换机 7 换机次数用完 8 换机验证不通过 9 序列号问题 10 验证码问题 11 试用已用过 12 提交或校验异常 13 渠道停止服务 14 服务器忙 15 未知。宿主要区分「网络错误」和「账号问题」判这个，别去比错误码。", 1, SDT_INT, 0, NULL, cxah_ErrCategory),
    CMD("会员_事件已接管", "CxahMarkHandled", "告诉库这条事件宿主已经自己处理，库就不再给默认提醒。用窗口消息收事件的宿主在收到消息后调用；用回调的宿主直接在回调里返回 1，不用调本命令。", 7, SDT_BOOL, 1, g_argEventCode, cxah_MarkHandled),
    CMD("会员_设置提醒策略", "CxahSetAlertPolicy", "自动提醒开关与到期提前天数。默认自动提醒为真、到期前 3 天提醒；账号被换到别的电脑而宿主没接管事件时，库会提示「您的账户已更换到其他电脑，即将退出」并收回本机授权。", 7, SDT_BOOL, 2, g_argAlert, cxah_SetAlertPolicy),
    CMD("会员_取到期剩余天数", "CxahDaysLeft", "按校准后的服务器时间算还剩几天到期，负数表示已过期天数，未开通为 0。", 3, SDT_INT, 0, NULL, cxah_DaysLeft),
    CMD("会员_是否自动提醒", "CxahIsAutoAlert", "当前事件没被接管时会不会由库弹窗。", 7, SDT_BOOL, 0, NULL, cxah_IsAutoAlert),
    CMD("会员_取到期提醒天数", "CxahWarnDays", "到期前几天提醒，0 表示不提醒。", 7, SDT_INT, 0, NULL, cxah_WarnDays),
    LIFE("会员对象_构造函数", "CxahMConstruct", CT_IS_HIDED | CT_IS_OBJ_CONSTURCT_CMD, 0, NULL),
    LIFE("会员对象_复制函数", "CxahMCopy", CT_IS_HIDED | CT_IS_OBJ_COPY_CMD, 1, g_argCopyMember),
    LIFE("会员对象_析构函数", "CxahMDestruct", CT_IS_HIDED | CT_IS_OBJ_FREE_CMD, 0, NULL),
    OBJ("初始化", "CxahM_Init",
        "装载授权模块，必须最先调用。机器码、程序MD5、版本、系统版本这些库取不到的信息由对象带走；返回 0 成功，1 参数缺失，3 机器码为空。",
        SDT_INT, 12, g_argInit, cxahM_Init),
    OBJ("登录", "CxahM_Login",
        "邮箱加本机机器码登录，成功即取得授权。返回错误码文本：0 通过，4001 机器码不符（要换机），4002 换机次数不足，2001 账号不存在，2004 已到期，2005 未开通授权。",
        SDT_TEXT, 1, g_argEmail, cxahM_Login),
    OBJ("注册", "CxahM_Register",
        "注册新账号并绑定当前机器码，需要图形验证码。返回错误码文本，0 成功，2002 已注册，5005 验证码错误。",
        SDT_TEXT, 8, g_argRegister, cxahM_Register),
    OBJ("开通试用", "CxahM_Trial",
        "为账号开通一次免费试用。返回错误码文本，0 成功，6001 已试用过。",
        SDT_TEXT, 1, g_argEmail, cxahM_Trial),
    OBJ("激活序列号", "CxahM_Activate",
        "输入序列号开通或续费授权，需要图形验证码。返回错误码文本，0 成功，5001 序列号不存在，5002 已被使用，5004 不适用本软件。",
        SDT_TEXT, 4, g_argActivate, cxahM_Activate),
    OBJ("刷新授权", "CxahM_Refresh",
        "只刷新授权信息，不写登录记录，适合运行中定时调用；也会触发快到期提醒。返回错误码文本。",
        SDT_TEXT, 1, g_argEmail, cxahM_Refresh),
    OBJ("申请换机", "CxahM_ChangeProbe",
        "本机机器码与账号绑定不一致时先问一次：返回 0 表示可以换机（记下申请号与剩余次数），4002 表示次数已用完。",
        SDT_TEXT, 1, g_argEmail, cxahM_ChangeProbe),
    OBJ("换机验证", "CxahM_ChangeConfirm",
        "提交本账号激活过的序列号与验证码，通过即换机并登录。返回错误码文本，0 成功，4003 序列号与账号不符，4004 失败过多被锁定。",
        SDT_TEXT, 5, g_argChangeConfirm, cxahM_ChangeConfirm),
    OBJ("发送退出事件", "CxahM_QuitEvent",
        "软件关闭前调用，上报下线时间。",
        SDT_BOOL, 0, NULL, cxahM_QuitEvent),
    OBJ("退出账号", "CxahM_Logout",
        "清除本机保存的账号与授权信息，不影响服务器数据。",
        SDT_BOOL, 0, NULL, cxahM_Logout),
    OBJ("打开会员窗口", "CxahM_OpenMemberWindow",
        "弹出会员向导窗口（登录、注册、开通授权）；登录遇到机器码不符会自动引导换机。返回真表示关闭时已拿到授权。",
        SDT_BOOL, 1, g_argPage, cxahM_OpenMemberWindow),
    OBJ("打开序列号窗口", "CxahM_OpenKeyWindow",
        "弹独立的序列号激活小窗口（输序列号 + 图形验证码），用于续费。返回真表示已拿到授权。",
        SDT_BOOL, 0, NULL, cxahM_OpenKeyWindow),
    OBJ("打开用户中心", "CxahM_OpenUserCenter",
        "弹用户中心：账号、机器码、授权版本、到期时间、剩余换机次数、已开通的选配功能，可打开渠道购买页、刷新授权、激活序列号。返回真表示关闭时账号可用。",
        SDT_BOOL, 1, g_argEmail, cxahM_OpenUserCenter),
    OBJ("打开换机窗口", "CxahM_OpenChangeWindow",
        "先申请换机，再弹换机验证小窗口，输入本账号激活过的序列号与验证码，通过后自动换机并登录。返回错误码文本。",
        SDT_TEXT, 1, g_argEmail, cxahM_OpenChangeWindow),
    OBJ("绑定事件窗口", "CxahM_BindEventWindow",
        "把会员事件用 PostMessage 发到指定窗口，wParam 是事件码。跨线程安全，推荐用法。",
        SDT_BOOL, 2, g_argEventWin, cxahM_BindEventWindow),
    OBJ("绑定事件回调", "CxahM_BindEventCallback",
        "注册子程序指针作为事件回调：参数是事件码，返回 1 表示宿主已接管（库不再弹默认提醒），返回 0 交给库。传 0 取消登记。回调只在登记它的线程上触发。",
        SDT_BOOL, 1, g_argEventCb, cxahM_BindEventCallback),
    OBJ("取事件", "CxahM_PollEvent",
        "从事件队列取最早的一条并弹出，返回 JSON：e 事件码、n 名字、t 时间、d 详情（含 code 与 category）；没有事件返回空文本。",
        SDT_TEXT, 0, NULL, cxahM_PollEvent),
    OBJ("取事件数", "CxahM_EventCount",
        "队列里还有多少条未取的事件。",
        SDT_INT, 0, NULL, cxahM_EventCount),
    OBJ("清空事件", "CxahM_ClearEvents",
        "丢弃队列里全部未取的事件。",
        SDT_BOOL, 0, NULL, cxahM_ClearEvents),
    OBJ("取事件名", "CxahM_EventName",
        "事件码对应的中文说明，可直接显示给用户。",
        SDT_TEXT, 1, g_argEventCode, cxahM_EventName),
    OBJ("事件已接管", "CxahM_MarkHandled",
        "告诉库这条事件宿主已自己处理，库不再给默认提醒。用窗口消息收事件时配套使用；用回调的话直接在回调里返回 1。",
        SDT_BOOL, 1, g_argEventCode, cxahM_MarkHandled),
    OBJ("启动自动刷新", "CxahM_StartAutoRefresh",
        "库内起线程按间隔从服务器载入最新授权；到期、冻结、被别的电脑顶号都会发事件。事件发往本对象先前绑定的窗口，也可以在这里直接指定。返回真表示已在运行。",
        SDT_BOOL, 3, g_argAuto, cxahM_StartAutoRefresh),
    OBJ("停止自动刷新", "CxahM_StopAutoRefresh",
        "停掉自动刷新线程并等它退出。",
        SDT_BOOL, 0, NULL, cxahM_StopAutoRefresh),
    OBJ("取自动刷新间隔", "CxahM_AutoInterval",
        "当前自动刷新间隔秒数，0 表示未启动。",
        SDT_INT, 0, NULL, cxahM_AutoInterval),
    OBJ("设置提醒策略", "CxahM_SetAlertPolicy",
        "自动提醒开关与到期提前天数。默认开、提前 3 天。宿主没接管事件时，由库提示「您的账户已更换到其他电脑，即将退出」并收回本机授权。",
        SDT_BOOL, 2, g_argAlert, cxahM_SetAlertPolicy),
    OBJ("取到期提醒天数", "CxahM_WarnDays",
        "到期前几天提醒，0 表示不提醒。",
        SDT_INT, 0, NULL, cxahM_WarnDays),
    OBJ("是否自动提醒", "CxahM_IsAutoAlert",
        "事件没被接管时会不会由库直接弹窗。",
        SDT_BOOL, 0, NULL, cxahM_IsAutoAlert),
    OBJ("是否已授权", "CxahM_IsAuthorized",
        "当前是否可用（正式或试用且未过期）。",
        SDT_BOOL, 0, NULL, cxahM_IsAuthorized),
    OBJ("取授权状态", "CxahM_AuthStatus",
        "ok 正式 / trial 试用 / expired 到期 / frozen 冻结 / none 未开通 / revoked 已被别的电脑顶掉。",
        SDT_TEXT, 0, NULL, cxahM_AuthStatus),
    OBJ("取到期时间", "CxahM_DueDate",
        "授权到期时间，未开通为空。",
        SDT_TEXT, 0, NULL, cxahM_DueDate),
    OBJ("取到期剩余天数", "CxahM_DaysLeft",
        "按校准后的服务器时间算还剩几天，负数表示已过期天数，未开通为 0。",
        SDT_INT, 0, NULL, cxahM_DaysLeft),
    OBJ("取账号", "CxahM_Email",
        "当前登录账号邮箱。",
        SDT_TEXT, 0, NULL, cxahM_Email),
    OBJ("取机器码", "CxahM_Mac",
        "初始化时由宿主传进来的本机机器码。",
        SDT_TEXT, 0, NULL, cxahM_Mac),
    OBJ("取剩余换机次数", "CxahM_ChangeLeft",
        "账号还能换机的次数，界面可提前提示。",
        SDT_INT, 0, NULL, cxahM_ChangeLeft),
    OBJ("取错误码", "CxahM_LastCode",
        "最近一次接口返回的错误码，0 表示成功。",
        SDT_TEXT, 0, NULL, cxahM_LastCode),
    OBJ("取错误类别", "CxahM_ErrCategory",
        "把最近一次错误码归成处置类别：0 无错误 1 网络不通 2 账号无效 3 账号被停止使用 4 已到期 5 未开通授权 6 需要换机 7 换机次数用完 8 换机验证不通过 9 序列号问题 10 验证码问题 11 试用已用过 12 提交或校验异常 13 渠道停止服务 14 服务器忙 15 未知。区分网络故障和账号问题判这个。",
        SDT_INT, 0, NULL, cxahM_ErrCategory),
    OBJ("取错误文本", "CxahM_CodeText",
        "错误码对应的中文说明；上一次接口给了服务器文案时优先用服务器文案。",
        SDT_TEXT, 1, g_argCodeText, cxahM_CodeText),
    OBJ("是否有功能权限", "CxahM_HasFeature",
        "判断增配权限串里是否包含指定功能码。",
        SDT_BOOL, 1, g_argFeature, cxahM_HasFeature),
    OBJ("取选配功能", "CxahM_Features",
        "已开通的选配功能，每行「名称|到期时间|1」，末位 1 表示当前可用。",
        SDT_TEXT, 0, NULL, cxahM_Features),
    OBJ("取附加信息", "CxahM_Attr",
        "旧模块 授权信息.附加信息 的口径；键名留空返回整段 JSON。",
        SDT_TEXT, 1, g_argField, cxahM_Attr),
    OBJ("取授权字段", "CxahM_GrantField",
        "按字段名取授权信息：uid email qq bind_mac mac_key state due_date reg_date function function_name soft_limit order_number puc_type channel_id remarks server_time limit_change change_time change_left attr ip ip_location json。",
        SDT_TEXT, 1, g_argField, cxahM_GrantField),
    OBJ("取渠道信息", "CxahM_OemField",
        "取渠道字段：brand_name soft_name puc_type theme_color qq wechat phone home shop_home help_home download_home change_home buy_text ad_url server_url official able_reg protection 等。",
        SDT_TEXT, 1, g_argField, cxahM_OemField),
    OBJ("取程序配置", "CxahM_SoftSetting",
        "取渠道 setting 里的程序配置项，取不到返回默认值。",
        SDT_TEXT, 2, g_argSetting, cxahM_SoftSetting),
    OBJ("取服务器时间", "CxahM_ServerTime",
        "校准后的当前时间，形如 2026-10-19 21:30:00；用户改本机时间不影响到期判断。",
        SDT_TEXT, 0, NULL, cxahM_ServerTime),
    OBJ("取上下文文本", "CxahM_ContextJson",
        "一次拿到当前授权上下文的全部 JSON（含账号、到期时间、状态、选配功能、附加信息、IP），做界面或写日志用它。",
        SDT_TEXT, 0, NULL, cxahM_ContextJson),
    OBJ("取验证码图片", "CxahM_CaptchaImage",
        "取一张新的图形验证码，返回 PNG 的 Base64 文本（同时更新验证码ID）。",
        SDT_TEXT, 0, NULL, cxahM_CaptchaImage),
    OBJ("取验证码ID", "CxahM_CaptchaId",
        "取最近一次验证码的ID，提交注册或激活时一起传。",
        SDT_TEXT, 0, NULL, cxahM_CaptchaId),
    OBJ("校验机器码", "CxahM_CheckMachine",
        "按服务端同规则校验机器码结构，返回 0 表示通过，其它为说明文本。",
        SDT_TEXT, 2, g_argCheckMac, cxahM_CheckMachine),
    OBJ("解析机器码", "CxahM_ParseMachine",
        "解出机器码里的字段，便于后台排障与界面显示。",
        SDT_TEXT, 2, g_argParseMac, cxahM_ParseMachine),
    OBJ("请求接口", "CxahM_RawRequest",
        "通用逃生口：按路由名直连接口，返回服务器原始响应 JSON，试用新功能时不用改支持库。",
        SDT_TEXT, 2, g_argRaw, cxahM_RawRequest),
    OBJ("取库版本", "CxahM_LibVersion",
        "返回本支持库版本号。",
        SDT_TEXT, 0, NULL, cxahM_LibVersion),

};

static PFN_EXECUTE_CMD g_commandFuncs[] = {
    cxah_Init, cxah_CodeText, cxah_LastCode, cxah_LastMsg, cxah_SetAccount, cxah_LoadAccount,
    cxah_LoadOem, cxah_OemField, cxah_SoftSetting, cxah_SyncTime, cxah_BeijingTime,
    cxah_Register, cxah_Trial, cxah_Login, cxah_Refresh, cxah_Activate, cxah_ChangeProbe,
    cxah_ChangeConfirm, cxah_QuitEvent, cxah_Logout,
    cxah_IsAuthorized, cxah_GrantField, cxah_DueDate, cxah_AuthStatus, cxah_ChangeLeft, cxah_HasFeature,
    cxah_ShowRegister, cxah_ShowRenew, cxah_ShowChange, cxah_CaptchaBase64, cxah_CaptchaId,
    cxah_CheckMachine, cxah_ParseMachine, cxah_RawRequest, cxah_LibVersion,
    cxah_SetEventWindow, cxah_SetEventCallback, cxah_PollEvent, cxah_EventCount, cxah_ClearEvents,
    cxah_EventName, cxah_StartAutoRefresh, cxah_StopAutoRefresh, cxah_AutoState,
    cxah_ShowUserCenter, cxah_Features, cxah_Attr,
    cxah_ErrCategory, cxah_MarkHandled, cxah_SetAlertPolicy, cxah_DaysLeft, cxah_IsAutoAlert, cxah_WarnDays,
    cxahM_Construct, cxahM_Copy, cxahM_Destruct, cxahM_Init, cxahM_Login, cxahM_Register, cxahM_Trial, cxahM_Activate, cxahM_Refresh, cxahM_ChangeProbe, cxahM_ChangeConfirm, cxahM_QuitEvent, cxahM_Logout, cxahM_OpenMemberWindow, cxahM_OpenKeyWindow, cxahM_OpenUserCenter, cxahM_OpenChangeWindow, cxahM_BindEventWindow,
    cxahM_BindEventCallback, cxahM_PollEvent, cxahM_EventCount, cxahM_ClearEvents, cxahM_EventName, cxahM_MarkHandled, cxahM_StartAutoRefresh, cxahM_StopAutoRefresh, cxahM_AutoInterval, cxahM_SetAlertPolicy, cxahM_WarnDays, cxahM_IsAutoAlert, cxahM_IsAuthorized, cxahM_AuthStatus, cxahM_DueDate, cxahM_DaysLeft, cxahM_Email, cxahM_Mac, cxahM_ChangeLeft, cxahM_LastCode, cxahM_ErrCategory, cxahM_CodeText, cxahM_HasFeature, cxahM_Features, cxahM_Attr, cxahM_GrantField, cxahM_OemField, cxahM_SoftSetting, cxahM_ServerTime, cxahM_ContextJson, cxahM_CaptchaImage, cxahM_CaptchaId, cxahM_CheckMachine, cxahM_ParseMachine, cxahM_RawRequest, cxahM_LibVersion,
};

const char* g_commandFuncNames[] = {
    "cxah_Init", "cxah_CodeText", "cxah_LastCode", "cxah_LastMsg", "cxah_SetAccount", "cxah_LoadAccount",
    "cxah_LoadOem", "cxah_OemField", "cxah_SoftSetting", "cxah_SyncTime", "cxah_BeijingTime",
    "cxah_Register", "cxah_Trial", "cxah_Login", "cxah_Refresh", "cxah_Activate", "cxah_ChangeProbe",
    "cxah_ChangeConfirm", "cxah_QuitEvent", "cxah_Logout",
    "cxah_IsAuthorized", "cxah_GrantField", "cxah_DueDate", "cxah_AuthStatus", "cxah_ChangeLeft", "cxah_HasFeature",
    "cxah_ShowRegister", "cxah_ShowRenew", "cxah_ShowChange", "cxah_CaptchaBase64", "cxah_CaptchaId",
    "cxah_CheckMachine", "cxah_ParseMachine", "cxah_RawRequest", "cxah_LibVersion",
    "cxah_SetEventWindow", "cxah_SetEventCallback", "cxah_PollEvent", "cxah_EventCount", "cxah_ClearEvents",
    "cxah_EventName", "cxah_StartAutoRefresh", "cxah_StopAutoRefresh", "cxah_AutoState",
    "cxah_ShowUserCenter", "cxah_Features", "cxah_Attr",
    "cxah_ErrCategory", "cxah_MarkHandled", "cxah_SetAlertPolicy", "cxah_DaysLeft", "cxah_IsAutoAlert", "cxah_WarnDays",
    "cxahM_Construct", "cxahM_Copy", "cxahM_Destruct", "cxahM_Init", "cxahM_Login", "cxahM_Register", "cxahM_Trial", "cxahM_Activate", "cxahM_Refresh", "cxahM_ChangeProbe", "cxahM_ChangeConfirm", "cxahM_QuitEvent", "cxahM_Logout", "cxahM_OpenMemberWindow", "cxahM_OpenKeyWindow", "cxahM_OpenUserCenter", "cxahM_OpenChangeWindow", "cxahM_BindEventWindow",
    "cxahM_BindEventCallback", "cxahM_PollEvent", "cxahM_EventCount", "cxahM_ClearEvents", "cxahM_EventName", "cxahM_MarkHandled", "cxahM_StartAutoRefresh", "cxahM_StopAutoRefresh", "cxahM_AutoInterval", "cxahM_SetAlertPolicy", "cxahM_WarnDays", "cxahM_IsAutoAlert", "cxahM_IsAuthorized", "cxahM_AuthStatus", "cxahM_DueDate", "cxahM_DaysLeft", "cxahM_Email", "cxahM_Mac", "cxahM_ChangeLeft", "cxahM_LastCode", "cxahM_ErrCategory", "cxahM_CodeText", "cxahM_HasFeature", "cxahM_Features", "cxahM_Attr", "cxahM_GrantField", "cxahM_OemField", "cxahM_SoftSetting", "cxahM_ServerTime", "cxahM_ContextJson", "cxahM_CaptchaImage", "cxahM_CaptchaId", "cxahM_CheckMachine", "cxahM_ParseMachine", "cxahM_RawRequest", "cxahM_LibVersion",
};

// 会员管理 = 授权功能的对象化入口：初始化、界面、事件、自动刷新与全部取值方法。
// 只有一个隐藏子成员存内部指针，复制函数必须深拷，析构函数负责停掉自己起的自动刷新。
LIB_DATA_TYPE_ELEMENT g_onePtrElement[] = {
    { SDT_INT, NULL, _T("impl"), NULL, _T("内部数据，请勿读写。"), LES_HIDED, 0 },
};

#define CX_TYPE(NAME, EG, EXPLAIN, CMDINDEX) \
    { _T(NAME), _T(EG), _T(EXPLAIN), \
      sizeof(CMDINDEX) / sizeof(CMDINDEX[0]), CMDINDEX, \
      0, 0, 0, NULL, 0, NULL, NULL, \
      sizeof(g_onePtrElement) / sizeof(g_onePtrElement[0]), g_onePtrElement }

INT g_memberCmdIndex[] = { 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106 };
static_assert(sizeof(g_memberCmdIndex) / sizeof(INT) == 54, "会员管理成员命令条数变了");

static LIB_DATA_TYPE_INFO g_dataTypes[] = {
    CX_TYPE("会员管理", "MemberManager", "创想AH会员授权对象：初始化后就能登录、注册、试用、激活序列号、自助换机、刷新授权，打开会员窗口/序列号窗口/用户中心，绑定事件、启动后台自动刷新，并读取当前授权上下文。", g_memberCmdIndex),
};

// 每个类别单独一条字面量再串起来：连写成 "…\00002注册与授权" 会被 C 当成八进制转义 \000
// 吞掉两个 0，第二三类往后错位，IDE 里类别名显示成乱码。
static const TCHAR g_categories[] =
    _T("0001初始化与配置\0")
    _T("0002注册与授权\0")
    _T("0003授权信息\0")
    _T("0004注册登录界面\0")
    _T("0005机器码\0")
    _T("0006通用\0")
    _T("0007事件与自动刷新\0")
    _T("\0");

#endif  // !__E_STATIC_LIB

// ==================== 库通知 ====================

INT WINAPI cxah_ProcessNotifyLib(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    UNREFERENCED_PARAMETER(dwParam2);

    switch (nMsg)
    {
    case NL_SYS_NOTIFY_FUNCTION:
        g_pfnNotifySys = (PFN_NOTIFY_SYS)dwParam1;
        return NR_OK;

#ifndef __E_STATIC_LIB
    case NL_GET_CMD_FUNC_NAMES:
        return (INT)g_commandFuncNames;
    case NL_GET_NOTIFY_LIB_FUNC_NAME:
        return (INT)LIB_NOTIFY_FUNC_NAME;
    case NL_GET_DEPENDENT_LIBS:
        return (INT)NULL;
#endif

    case NL_FREE_LIB_DATA:
    case NL_UNLOAD_FROM_IDE:
        EventShutdown();
        g_pfnNotifySys = NULL;
        return NR_OK;

    default:
        return NR_ERR;
    }
}

// ==================== 库信息 ====================
#ifndef __E_STATIC_LIB

LIB_INFO g_libraryInfo = {
    LIB_FORMAT_VER,
    _T("E0817B3362096FA711BF8F50631378D7"),
    1, 1, 0,   // 1.1.0：新增错误类别、事件接管与自动提醒、会员管理 类
    3, 0,
    3, 0,
    _T("创想AH会员框架"),
    __GBK_LANG_VER,
    _T("创想AH软件授权客户端：注册、登录、试用、序列号激活、自助换机与授权信息读取，配套全新 JSON 后端。机器码由宿主传入，对外文本为 UTF-8。"),
    _LIB_OS(__OS_WIN),
    _T("创想软件"),
    _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""),
    sizeof(g_dataTypes) / sizeof(g_dataTypes[0]), g_dataTypes,
    7, const_cast<TCHAR*>(g_categories),
    sizeof(g_commands) / sizeof(g_commands[0]),
    g_commands,
    g_commandFuncs,
    NULL, NULL,
    cxah_ProcessNotifyLib,
    NULL, NULL,
    0, NULL,
    NULL,
};

#endif  // !__E_STATIC_LIB

}  // namespace cx

// ==== 以下由 gen_static_trampolines.mjs 生成，勿手改 ====
// 静态编译时易语言按名字到 cxsoft_static.lib 里找命令实现，
// 而实现在 namespace cx 内，故每个命令生成一份 extern "C" 全局转发。
#define CX_FWD(fn) extern "C" void fn (PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf) { cx::fn (pRetData, nArgCount, pArgInf); }

CX_FWD(cxah_Init)
CX_FWD(cxah_CodeText)
CX_FWD(cxah_LastCode)
CX_FWD(cxah_LastMsg)
CX_FWD(cxah_SetAccount)
CX_FWD(cxah_LoadAccount)
CX_FWD(cxah_LoadOem)
CX_FWD(cxah_OemField)
CX_FWD(cxah_SoftSetting)
CX_FWD(cxah_SyncTime)
CX_FWD(cxah_BeijingTime)
CX_FWD(cxah_Register)
CX_FWD(cxah_Trial)
CX_FWD(cxah_Login)
CX_FWD(cxah_Refresh)
CX_FWD(cxah_Activate)
CX_FWD(cxah_ChangeProbe)
CX_FWD(cxah_ChangeConfirm)
CX_FWD(cxah_QuitEvent)
CX_FWD(cxah_Logout)
CX_FWD(cxah_IsAuthorized)
CX_FWD(cxah_GrantField)
CX_FWD(cxah_DueDate)
CX_FWD(cxah_AuthStatus)
CX_FWD(cxah_ChangeLeft)
CX_FWD(cxah_HasFeature)
CX_FWD(cxah_ShowRegister)
CX_FWD(cxah_ShowRenew)
CX_FWD(cxah_ShowChange)
CX_FWD(cxah_CaptchaBase64)
CX_FWD(cxah_CaptchaId)
CX_FWD(cxah_CheckMachine)
CX_FWD(cxah_ParseMachine)
CX_FWD(cxah_RawRequest)
CX_FWD(cxah_LibVersion)
CX_FWD(cxah_SetEventWindow)
CX_FWD(cxah_SetEventCallback)
CX_FWD(cxah_PollEvent)
CX_FWD(cxah_EventCount)
CX_FWD(cxah_ClearEvents)
CX_FWD(cxah_EventName)
CX_FWD(cxah_StartAutoRefresh)
CX_FWD(cxah_StopAutoRefresh)
CX_FWD(cxah_AutoState)
CX_FWD(cxah_ShowUserCenter)
CX_FWD(cxah_Features)
CX_FWD(cxah_Attr)
CX_FWD(cxah_ErrCategory)
CX_FWD(cxah_MarkHandled)
CX_FWD(cxah_SetAlertPolicy)
CX_FWD(cxah_DaysLeft)
CX_FWD(cxah_IsAutoAlert)
CX_FWD(cxah_WarnDays)
CX_FWD(cxahM_Construct)
CX_FWD(cxahM_Copy)
CX_FWD(cxahM_Destruct)
CX_FWD(cxahM_Init)
CX_FWD(cxahM_Login)
CX_FWD(cxahM_Register)
CX_FWD(cxahM_Trial)
CX_FWD(cxahM_Activate)
CX_FWD(cxahM_Refresh)
CX_FWD(cxahM_ChangeProbe)
CX_FWD(cxahM_ChangeConfirm)
CX_FWD(cxahM_QuitEvent)
CX_FWD(cxahM_Logout)
CX_FWD(cxahM_OpenMemberWindow)
CX_FWD(cxahM_OpenKeyWindow)
CX_FWD(cxahM_OpenUserCenter)
CX_FWD(cxahM_OpenChangeWindow)
CX_FWD(cxahM_BindEventWindow)
CX_FWD(cxahM_BindEventCallback)
CX_FWD(cxahM_PollEvent)
CX_FWD(cxahM_EventCount)
CX_FWD(cxahM_ClearEvents)
CX_FWD(cxahM_EventName)
CX_FWD(cxahM_MarkHandled)
CX_FWD(cxahM_StartAutoRefresh)
CX_FWD(cxahM_StopAutoRefresh)
CX_FWD(cxahM_AutoInterval)
CX_FWD(cxahM_SetAlertPolicy)
CX_FWD(cxahM_WarnDays)
CX_FWD(cxahM_IsAutoAlert)
CX_FWD(cxahM_IsAuthorized)
CX_FWD(cxahM_AuthStatus)
CX_FWD(cxahM_DueDate)
CX_FWD(cxahM_DaysLeft)
CX_FWD(cxahM_Email)
CX_FWD(cxahM_Mac)
CX_FWD(cxahM_ChangeLeft)
CX_FWD(cxahM_LastCode)
CX_FWD(cxahM_ErrCategory)
CX_FWD(cxahM_CodeText)
CX_FWD(cxahM_HasFeature)
CX_FWD(cxahM_Features)
CX_FWD(cxahM_Attr)
CX_FWD(cxahM_GrantField)
CX_FWD(cxahM_OemField)
CX_FWD(cxahM_SoftSetting)
CX_FWD(cxahM_ServerTime)
CX_FWD(cxahM_ContextJson)
CX_FWD(cxahM_CaptchaImage)
CX_FWD(cxahM_CaptchaId)
CX_FWD(cxahM_CheckMachine)
CX_FWD(cxahM_ParseMachine)
CX_FWD(cxahM_RawRequest)
CX_FWD(cxahM_LibVersion)

#undef CX_FWD

extern "C" INT WINAPI cxah_ProcessNotifyLib (INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    return cx::cxah_ProcessNotifyLib (nMsg, dwParam1, dwParam2);
}

#ifndef __E_STATIC_LIB

// 导出名必须固定为 GetNewInf 且不带修饰，所以靠 .def 导出而不是 __declspec。
extern "C" PLIB_INFO WINAPI GetNewInf()
{
    return &cx::g_libraryInfo;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID)
{
    if (dwReason == DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(hModule);
    return TRUE;
}

#endif  // !__E_STATIC_LIB
