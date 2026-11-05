// 创想AH会员框架 · 接口信封与业务动作
#include "cx_core.h"

#include <stdio.h>
#include <string.h>

namespace cx {

// ==================== 时间 ====================

static long long LocalMs()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    // 1601->1970 的 100ns 间隔数
    return (long long)(u.QuadPart - 116444736000000000ULL) / 10000LL;
}

static std::string MsToText(long long ms)
{
    FILETIME ft;
    ULARGE_INTEGER u;
    u.QuadPart = (ULONGLONG)ms * 10000ULL + 116444736000000000ULL;
    ft.dwLowDateTime = u.LowPart;
    ft.dwHighDateTime = u.HighPart;
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    // UTC -> 北京时间（服务器按 +8 出时间）
    st.wMinute += 0;
    FILETIME localFt;
    SystemTimeToFileTime(&st, &localFt);
    FILETIME tzFt = localFt;
    // 直接加 8 小时，避免依赖本机时区设置
    ULARGE_INTEGER t;
    t.LowPart = tzFt.dwLowDateTime;
    t.HighPart = tzFt.dwHighDateTime;
    t.QuadPart += 8ULL * 3600ULL * 10000000ULL;
    tzFt.dwLowDateTime = t.LowPart;
    tzFt.dwHighDateTime = t.HighPart;
    FileTimeToSystemTime(&tzFt, &st);

    char buf[32];
    wsprintfA(buf, "%04d-%02d-%02d %02d:%02d:%02d", (int)st.wYear, (int)st.wMonth, (int)st.wDay,
        (int)st.wHour, (int)st.wMinute, (int)st.wSecond);
    return std::string(buf);
}

long long NowMs()
{
    return LocalMs() + S().timeOffsetMs;
}

std::string NowBeijingText()
{
    return MsToText(NowMs());
}

static std::string NowMsText()
{
    return I64ToText(LocalMs() + S().timeOffsetMs);
}

// ==================== 信封 ====================

static void DeriveKey(const char* purpose, const std::string& route, const std::string& ts, unsigned char out[32])
{
    const std::string src = S().secret + "|" + purpose + "|" + route + "|" + ts;
    if (!Sha256Raw((const unsigned char*)src.data(), src.size(), out)) {
        memset(out, 0, 32);
    }
}

std::string BuildEnvelope(const std::string& route, const std::string& businessJson, std::string* tsOut)
{
    const std::string ts = NowMsText();
    if (tsOut != NULL) *tsOut = ts;

    unsigned char encKey[32], sigKey[32];
    DeriveKey("ENC-REQ", route, ts, encKey);
    DeriveKey("SIG-REQ", route, ts, sigKey);

    std::string cipher = Aes256CbcEncrypt(encKey, businessJson, NULL);
    if (cipher.empty()) return std::string();

    // 后端按整串 base64(iv + 密文) 计算签名，这里必须保持同一形态
    const std::string dWhole = Base64Encode((const unsigned char*)cipher.data(), cipher.size());

    const std::string sign = HmacSha256Hex(std::string((char*)sigKey, 32), route + "|" + ts + "|" + dWhole);
    if (sign.empty()) return std::string();

    return "{\"r\":\"" + JsonEscape(route) + "\",\"v\":1,\"t\":" + ts +
           ",\"d\":\"" + dWhole + "\",\"s\":\"" + sign + "\"}";
}

bool ParseEnvelope(const std::string& route, const std::string& ts, const std::string& body, ApiResponse& out)
{
    JVal env;
    if (!JsonParse(body, env) || env.type != JVal::TObj) return false;

    const JVal* pOk = env.find("ok");
    const JVal* pCode = env.find("code");
    const JVal* pMsg = env.find("msg");
    out.code = pCode ? pCode->asStr("9999") : "9999";
    out.msg = pMsg ? pMsg->asStr() : "";
    out.ok = pOk ? pOk->asBool(false) : false;

    const JVal* d = env.find("d");
    if (d == NULL) {
        // 明文调试通道：直接给 data
        const JVal* data = env.find("data");
        if (data != NULL) out.data = *data;
        out.transportOk = true;
        return true;
    }

    std::vector<unsigned char> raw = Base64Decode(d->asStr());
    if (raw.size() <= 16) return false;

    unsigned char respKey[32], respSig[32];
    DeriveKey("ENC-RESP", route, ts, respKey);
    DeriveKey("SIG-RESP", route, ts, respSig);

    const std::string encoded = d->asStr();
    const std::string expect = HmacSha256Hex(std::string((char*)respSig, 32), route + "|" + ts + "|" + encoded);
    const JVal* s = env.find("s");
    if (!expect.empty() && (s == NULL || LowerStr(expect) != LowerStr(s->asStr()))) {
        out.code = "1002";
        out.msg = GbkToUtf8("服务器响应校验失败，请确认软件为官方版本。");
        out.ok = false;
        out.transportOk = true;
        return true;
    }

    std::string plain;
    if (!Aes256CbcDecrypt(respKey, &raw[0], &raw[16], raw.size() - 16, &plain)) {
        out.code = "1004";
        out.ok = false;
        out.transportOk = true;
        return true;
    }
    JVal inner;
    if (!JsonParse(plain, inner) || inner.type != JVal::TObj) return false;

    const JVal* data = inner.find("data");
    if (data != NULL) out.data = *data;
    const JVal* iCode = inner.find("code");
    if (iCode != NULL) out.code = iCode->asStr(out.code.c_str());
    const JVal* iMsg = inner.find("msg");
    if (iMsg != NULL && !iMsg->asStr().empty()) out.msg = iMsg->asStr();
    out.transportOk = true;
    return true;
}

ApiResponse ApiCall(const std::string& route, const std::string& businessJson)
{
    ApiResponse res;
    if (S().server.empty() || S().secret.empty()) {
        res.code = "1001";
        res.msg = GbkToUtf8("授权模块尚未初始化。");
        return res;
    }

    std::string ts;
    const std::string envelope = BuildEnvelope(route, businessJson, &ts);
    if (envelope.empty()) {
        res.code = "1004";
        res.msg = GbkToUtf8("请求数据打包失败，请检查本机时间与系统版本。");
        return res;
    }

    HttpResult http = HttpPostJson(S().server, envelope, S().timeoutMs);
    if (!http.ok) {
        res.code = "9998";
        res.msg = GbkToUtf8("无法连接授权服务器，请检查网络是否可用，或本软件是否被防火墙拦截。");
        res.raw = http.error;
        LogLine("http fail " + route + " " + http.error + " status=" + ToStr(http.status));
        return res;
    }

    res.raw = http.body;
    if (!ParseEnvelope(route, ts, http.body, res)) {
        res.code = "1004";
        res.msg = GbkToUtf8("服务器返回数据无法解析，请稍后重试。");
        LogLine("envelope parse fail " + route + " body=" + http.body.substr(0, 200));
        return res;
    }
    if (!res.code.empty() && res.code != "0") {
        LogLine("api code " + route + " " + res.code + " " + res.msg);
    }
    return res;
}

static void AddCommon(JBuild& b)
{
    b.str("puc_type", S().pucType);
    b.str("channel_id", S().oem.channelId.empty() ? "0" : S().oem.channelId);
    b.str("version", S().version);
    b.str("app_md5", S().appMd5);
    b.str("system", S().systemVer);
}

void SetApiError(const std::string& code, const std::string& msg)
{
    S().lastCode = code;
    S().lastMsg = msg;
}

void ApplyGrant(const JVal& data)
{
    const JVal* g = data.find("grant");
    if (g == NULL || g->type != JVal::TObj) return;
    S().grant.load(*g);
    const JVal* st = data.find("auth_status");
    if (st != NULL) S().grant.authStatus = st->asStr();
    if (!S().grant.email.empty()) S().email = S().grant.email;
}

// ==================== 本地账号（注册表，运行期解析 advapi32） ====================

namespace {

typedef LSTATUS(APIENTRY *PFN_OpenKey)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
typedef LSTATUS(APIENTRY *PFN_CreateKey)(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);
typedef LSTATUS(APIENTRY *PFN_SetValue)(HKEY, LPCWSTR, DWORD, DWORD, BYTE*, DWORD);
typedef LSTATUS(APIENTRY *PFN_QueryValue)(HKEY, LPCWSTR, LPDWORD, LPDWORD, BYTE*, LPDWORD);
typedef LSTATUS(APIENTRY *PFN_CloseKey)(HKEY);

struct RegApi {
    PFN_OpenKey OpenKey;
    PFN_CreateKey CreateKey;
    PFN_SetValue SetValue;
    PFN_QueryValue QueryValue;
    PFN_CloseKey CloseKey;
    bool ready;
    RegApi() : OpenKey(NULL), CreateKey(NULL), SetValue(NULL), QueryValue(NULL), CloseKey(NULL), ready(false)
    {
        HMODULE mod = LoadLibraryA("advapi32.dll");
        if (mod == NULL) return;
        OpenKey = (PFN_OpenKey)GetProcAddress(mod, "RegOpenKeyExW");
        CreateKey = (PFN_CreateKey)GetProcAddress(mod, "RegCreateKeyExW");
        SetValue = (PFN_SetValue)GetProcAddress(mod, "RegSetValueExW");
        QueryValue = (PFN_QueryValue)GetProcAddress(mod, "RegQueryValueExW");
        CloseKey = (PFN_CloseKey)GetProcAddress(mod, "RegCloseKey");
        ready = OpenKey && CreateKey && SetValue && QueryValue && CloseKey;
    }
};

std::wstring RegPath()
{
    std::wstring base = L"Software\\CxSoft\\CxAhMember\\";
    base += WideFromUtf8(S().pucType.empty() ? "0" : S().pucType);
    return base;
}

std::string RegRead(const std::wstring& name, const std::string& def)
{
    static RegApi reg;
    if (!reg.ready) return def;
    HKEY hKey = NULL;
    if (reg.OpenKey(HKEY_CURRENT_USER, RegPath().c_str(), 0, KEY_READ, &hKey) != 0) return def;
    wchar_t buf[512];
    DWORD type = 0, size = sizeof(buf);
    std::string out = def;
    if (reg.QueryValue(hKey, name.c_str(), &type, NULL, (BYTE*)buf, &size) == 0 && type == REG_SZ) {
        buf[size / sizeof(wchar_t)] = 0;
        out = Utf8FromWide(std::wstring(buf));
    }
    reg.CloseKey(hKey);
    return out;
}

bool RegWrite(const std::wstring& name, const std::string& utf8Value)
{
    static RegApi reg;
    if (!reg.ready) return false;
    HKEY hKey = NULL;
    DWORD disp = 0;
    if (reg.CreateKey(HKEY_CURRENT_USER, RegPath().c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &disp) != 0) {
        return false;
    }
    const std::wstring w = WideFromUtf8(utf8Value);
    const LSTATUS st = reg.SetValue(hKey, name.c_str(), 0, REG_SZ, (BYTE*)w.c_str(), (DWORD)((w.size() + 1) * sizeof(wchar_t)));
    reg.CloseKey(hKey);
    return st == 0;
}

}  // namespace

void SaveAccount(const std::string& email)
{
    S().email = email;
    RegWrite(L"Account", email);
}

std::string LoadAccount()
{
    if (!S().email.empty()) return S().email;
    std::string saved = RegRead(L"Account", "");
    saved = TrimStr(saved);
    if (!saved.empty()) S().email = LowerStr(saved);
    return S().email;
}

// ==================== 业务动作 ====================

std::string DoLoadOem()
{
    JBuild b;
    b.str("key", S().channelKey);
    b.str("puc_type", S().pucType);
    const ApiResponse res = ApiCall("channel.oem", b.done());
    SetApiError(res.code, res.msg);
    if (!res.ok) return res.code;
    S().oem.load(res.data);
    return "0";
}

std::string DoSyncTime()
{
    const ApiResponse res = ApiCall("common.time", "{}");
    SetApiError(res.code, res.msg);
    if (!res.ok) return res.code;
    const JVal* ts = res.data.find("timestamp");
    if (ts == NULL) return "1004";
    const long long server = ts->asInt64(0);   // 毫秒时间戳是 13 位，必须按 64 位取
    S().timeOffsetMs = (int)(server - LocalMs());
    return "0";
}

// 错误码 -> 处置类别。宿主要的是「该做什么」而不是「哪一步没过」，
// 所以这里按前缀段归类；新增码只往表尾加，类别号不能改。
int ErrorCategory(const std::string& code)
{
    if (code == "0" || code.empty()) return CXCAT_NONE;
    if (code == "9998") return CXCAT_NETWORK;
    if (code == "9999" || code == "9001" || code == "1006") return CXCAT_SERVER;
    if (code == "2001" || code == "2002") return CXCAT_ACCOUNT_INVALID;
    if (code == "2003") return CXCAT_ACCOUNT_FROZEN;
    if (code == "2004") return CXCAT_EXPIRED;
    if (code == "2005") return CXCAT_NO_GRANT;
    if (code == "4001") return CXCAT_NEED_CHANGE;
    if (code == "4002") return CXCAT_CHANGE_LIMIT;
    if (code == "4003" || code == "4004") return CXCAT_CHANGE_VERIFY;
    if (code == "3001" || code == "3002" || code == "1001") return CXCAT_PARAM;
    if (code == "5005" || code == "5006" || code == "5007") return CXCAT_CAPTCHA;
    if (code == "6001") return CXCAT_TRIAL_USED;
    if (code == "7001" || code == "7002") return CXCAT_CHANNEL;
    if (code.size() == 4 && code[0] == '1') return CXCAT_PARAM;   // 1xxx 都是提交/校验/环境
    if (code.size() == 4 && code[0] == '5') return CXCAT_BAD_KEY; // 5xxx 除了验证码那一组
    return CXCAT_UNKNOWN;
}

// 按路由把接口结果翻译成事件，界面与命令两条调用路径都会经过这里
static void FireForRoute(const std::string& route, const std::string& code, const std::string& before)
{
    JBuild b;
    b.str("code", code);
    b.str("msg", S().lastMsg);
    b.str("email", S().email);
    b.str("status", S().grant.authStatus);
    b.str("due_date", S().grant.dueDate);
    b.str("category", ToStr(ErrorCategory(code)));
    const std::string f = b.done();
    // 已经在本机授权过的账号被别的电脑顶掉：宿主不接管就由库直接说明并收回授权。
    // 没授权过的 4001 不算顶号，那是新用户换电脑登录，界面会走换机验证引导。
    if ((route == "auth.login" || route == "auth.refresh") && code == "4001" && IsAuthUsable(before)) {
        RevokeLocalGrant();
        if (!FireEventWait(EV_MAC_CHANGED, f, 1500) && S().autoAlert) AlertMacChanged();
    }
    if (route == "auth.login") FireEvent(code == "0" ? EV_LOGIN_OK : EV_LOGIN_FAIL, f);
    else if (route == "account.register" && code == "0") FireEvent(EV_REGISTER_OK, f);
    else if (route == "account.trial" && code == "0") FireEvent(EV_TRIAL_OK, f);
    else if (route == "key.activate" && code == "0") FireEvent(EV_ACTIVATE_OK, f);
    else if (route == "change.confirm" && code == "0") FireEvent(EV_CHANGE_OK, f);
    else if (route == "auth.refresh") FireAuthChange(before, S().grant.authStatus);
}

static std::string HandleAuthResult(const std::string& route, const ApiResponse& res)
{
    const std::string before = S().grant.authStatus;
    SetApiError(res.code, res.msg);
    if (res.ok) {
        ApplyGrant(res.data);
        FireForRoute(route, "0", before);
        return "0";
    }
    // 换机相关错误把参数带回来，界面据此决定弹窗内容
    const JVal* need = res.data.find("need_change");
    if (need != NULL && need->asBool(false)) {
        const JVal* req = res.data.find("request_id");
        if (req != NULL) S().requestId = req->asStr();
    }
    const JVal* left = res.data.find("change_left");   // 4003 这类失败也会带剩余次数
    if (left != NULL) S().grant.changeLeft = left->asInt(0);
    LogLine(route + " -> " + res.code);
    FireForRoute(route, res.code, before);
    return res.code;
}

// 传输层失败（连不上、超时、信封解不开）单独发一个事件，宿主可以只提示不打扰用户。
static void FireTransportFailure(const std::string& route, const ApiResponse& res)
{
    if (res.transportOk || res.code != "9998") return;
    JBuild b;
    b.str("code", res.code);
    b.str("msg", res.msg);
    b.str("route", route);
    b.str("category", ToStr(CXCAT_NETWORK));
    FireEvent(EV_SERVER_UNREACHABLE, b.done());
}

std::string DoLogin(const std::string& email)
{
    JBuild b;
    AddCommon(b);
    b.str("email", LowerStr(TrimStr(email)));
    b.str("mac_key", S().macKey);
    const ApiResponse res = ApiCall("auth.login", b.done());
    FireTransportFailure("auth.login", res);
    const std::string code = HandleAuthResult("auth.login", res);
    if (code == "0") SaveAccount(LowerStr(TrimStr(email)));
    return code;
}

std::string DoRefresh(const std::string& email)
{
    JBuild b;
    AddCommon(b);
    b.str("email", LowerStr(TrimStr(email)));
    b.str("mac_key", S().macKey);
    const ApiResponse res = ApiCall("auth.refresh", b.done());
    FireTransportFailure("auth.refresh", res);
    return HandleAuthResult("auth.refresh", res);
}

std::string DoRegister(const std::string& email, const std::string& qq, const std::string& referee,
                       const std::string& phone, const std::string& learn, const std::string& remarks,
                       const std::string& captchaId, const std::string& captchaCode)
{
    JBuild b;
    AddCommon(b);
    b.str("email", LowerStr(TrimStr(email)));
    b.str("mac_key", S().macKey);
    b.str("qq", qq);
    b.str("referee", LowerStr(TrimStr(referee)));
    b.str("home_username", phone);
    b.str("learn", learn);
    b.str("remarks", remarks);
    b.str("captcha_id", captchaId);
    b.str("captcha_code", captchaCode);
    const ApiResponse res = ApiCall("account.register", b.done());
    const std::string code = HandleAuthResult("account.register", res);
    if (code == "0") SaveAccount(LowerStr(TrimStr(email)));
    return code;
}

std::string DoTrial(const std::string& email)
{
    JBuild b;
    AddCommon(b);
    b.str("email", LowerStr(TrimStr(email)));
    b.str("mac_key", S().macKey);
    const ApiResponse res = ApiCall("account.trial", b.done());
    return HandleAuthResult("account.trial", res);
}

std::string DoActivate(const std::string& email, const std::string& keyNum, const std::string& captchaId, const std::string& captchaCode)
{
    JBuild b;
    AddCommon(b);
    b.str("email", LowerStr(TrimStr(email)));
    b.str("mac_key", S().macKey);
    b.str("key_num", TrimStr(keyNum));
    b.str("captcha_id", captchaId);
    b.str("captcha_code", captchaCode);
    const ApiResponse res = ApiCall("key.activate", b.done());
    return HandleAuthResult("key.activate", res);
}

std::string DoChangeProbe(const std::string& email)
{
    JBuild b;
    AddCommon(b);
    b.str("email", LowerStr(TrimStr(email)));
    b.str("mac_key", S().macKey);
    const ApiResponse res = ApiCall("change.request", b.done());
    SetApiError(res.code, res.msg);
    if (res.ok) {
        const JVal* req = res.data.find("request_id");
        if (req != NULL) S().requestId = req->asStr();
        const JVal* left = res.data.find("change_left");
        if (left != NULL) S().grant.changeLeft = left->asInt(0);
        return "0";
    }
    return res.code;
}

std::string DoChangeConfirm(const std::string& email, const std::string& keyNum, const std::string& captchaId,
                            const std::string& captchaCode, const std::string& requestId)
{
    JBuild b;
    AddCommon(b);
    b.str("email", LowerStr(TrimStr(email)));
    b.str("mac_key", S().macKey);
    b.str("key_num", TrimStr(keyNum));
    b.str("captcha_id", captchaId);
    b.str("captcha_code", captchaCode);
    if (!requestId.empty()) b.raw("request_id", requestId);
    const ApiResponse res = ApiCall("change.confirm", b.done());
    const std::string code = HandleAuthResult("change.confirm", res);
    if (code == "0") SaveAccount(LowerStr(TrimStr(email)));
    return code;
}

void DoSendQuitEvent()
{
    if (S().email.empty() || S().server.empty()) return;
    JBuild b;
    AddCommon(b);
    b.str("email", S().email);
    b.str("mac_key", S().macKey);
    b.str("event", "quit");
    ApiCall("auth.event", b.done());
}

bool FetchCaptcha(std::string* idOut, std::string* pngOut)
{
    const ApiResponse res = ApiCall("common.captcha", "{}");
    SetApiError(res.code, res.msg);
    if (!res.ok) return false;
    const JVal* id = res.data.find("id");
    const JVal* img = res.data.find("image_base64");
    if (id == NULL || img == NULL) return false;
    S().captchaId = id->asStr();
    S().captchaB64 = img->asStr();
    if (idOut != NULL) *idOut = S().captchaId;
    if (pngOut != NULL) *pngOut = S().captchaB64;
    return true;
}

}  // namespace cx
