// 创想AH会员框架 · 内部公共声明
// 约定：跨越支持库边界的文本一律是 UTF-8 原始字节，库内不做 GBK 转换；
//       界面显示前才转成宽字符。机器码由易语言宿主生成后传入，库内不生成。
#ifndef CXAH_CORE_H
#define CXAH_CORE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <map>

namespace cx {

// ---------------- 文本工具 ----------------
std::string  Utf8FromWide(const std::wstring& w);
std::wstring WideFromUtf8(const std::string& s);
std::string  WideToAnsi(const std::wstring& w);
std::string  TrimStr(const std::string& s);
std::string  LowerStr(const std::string& s);
std::string  ToStr(int v);
// 静态编译由易语言的 VC6 链接器完成：sprintf 会变成 ___stdio_common_vsprintf、
// (long long)double 会变成 __dtol3、strtoll 更是根本没有，所以数值文本一律自给。
std::string  I64ToText(long long v);
long long    ParseI64(const char* p);
long long    DoubleToI64(double d);
std::string  NumToText(double d);
std::string  Base64Encode(const unsigned char* p, size_t n);
std::string  Base64EncodeStr(const std::string& s);
std::vector<unsigned char> Base64Decode(const std::string& text);
bool StartsWith(const std::string& s, const char* head);

// ---------------- JSON ----------------
// 只做本项目需要的最小实现：解析对象/数组/字符串/数字/布尔/null，键按点路径取值。
struct JVal {
    enum Type { TNull, TStr, TNum, TBool, TObj, TArr } type;
    std::string str;
    double num;
    std::string numText;                          // 数字令牌原文，回写 JSON 时逐字保留
    bool bval;
    std::vector<std::pair<std::string, JVal> > obj;   // 保序，便于调试输出
    std::vector<JVal> arr;
    JVal() : type(TNull), num(0), bval(false) {}

    const JVal* find(const std::string& key) const;
    std::string asStr(const char* def = "") const;
    int         asInt(int def = 0) const;
    long long   asInt64(long long def = 0) const;
    bool        asBool(bool def = false) const;
    bool        isNull() const { return type == TNull; }
    std::string dump() const;   // 重新序列化为 JSON 文本
};

bool JsonParse(const std::string& text, JVal& out);
std::string JsonEscape(const std::string& raw);

// 请求体构造器
class JBuild {
public:
    void str(const std::string& k, const std::string& v);
    void num(const std::string& k, const std::string& v);   // 数字也以字符串传入，内部原样写出
    void raw(const std::string& k, const std::string& v);
    void boolean(const std::string& k, bool v);
    std::string done() const;      // {a:1,...}
    bool empty() const { return m_items.empty(); }
private:
    std::vector<std::string> m_items;
};

// ---------------- 密码学 ----------------
bool Sha256Raw(const unsigned char* p, size_t n, unsigned char out[32]);
std::string Sha256Hex(const std::string& text);
std::string HmacSha256Hex(const std::string& keyRaw, const std::string& data);
std::string Aes256CbcEncrypt(const unsigned char* key32, const std::string& plain, std::string* ivOut);
bool Aes256CbcDecrypt(const unsigned char* key32, const unsigned char* iv, const unsigned char* cipher, size_t cipherLen, std::string* plainOut);
std::string Md5Hex(const std::string& text);

// ---------------- HTTP ----------------
struct HttpResult {
    bool ok;
    int status;
    std::string body;
    std::string error;
    HttpResult() : ok(false), status(0) {}
};

HttpResult HttpPostJson(const std::string& url, const std::string& body, int timeoutMs);

// ---------------- 授权数据 ----------------
// 后台「功能开通」表里的选配功能，用户中心要按行显示
struct Feature {
    std::string code, name, dueDate;
    bool usable;
    Feature() : usable(false) {}
};

struct Grant {
    std::string uid, email, qq, bindMac, macKey, state, stateReason;
    std::string dueDate, regDate, lastLogin, function, functionName;
    std::string softLimit, orderNumber, pucType, channelId, homeUsername, remarks;
    std::string serverTime, authStatus, rawJson;
    std::string attrJson, ip, ipLocation;   // 旧模块的 附加信息 / IP地址
    std::vector<Feature> features;
    int loginNum, limitChange, changeTime, changeLeft;
    bool isTrial, isExpired, authorized;
    Grant() : loginNum(0), limitChange(0), changeTime(0), changeLeft(0),
              isTrial(false), isExpired(false), authorized(false) {}
    void load(const JVal& v);
    std::string field(const std::string& name) const;
    std::string featuresText() const;      // 每行「名称|到期时间|1」，1 表示当前可用
    std::string featureNames() const;      // 可用功能名顿号连接，给用户中心一行显示
    std::string attrField(const std::string& key) const;   // 附加信息里的一个键
};

struct OemInfo {
    std::string channelId, key, brandName, softName, pucType, themeColor;
    std::string qq, qq2, wechat, phone, home, shopHome, helpHome, downloadHome, changeHome;
    std::string buyText, adUrl, adUrl2, serverUrl, serverUrl2, settingRaw;
    bool official;
    int ableReg, protection;
    OemInfo() : official(true), ableReg(0), protection(0) {}
    void load(const JVal& v);
    std::string field(const std::string& name) const;
};

// ---------------- 运行期状态 ----------------
struct State {
    // 宿主初始化传入
    std::string server;      // 形如 https://host/xxx/public/api.php
    std::string secret;      // 与后端 config app_secret 一致
    std::string pucType;     // 产品类型
    std::string channelKey;  // 代理KEY
    std::string macKey;      // 机器码（宿主生成）
    std::string version;     // 程序版本
    std::string appMd5;      // 程序文件 MD5
    std::string systemVer;   // Windows 版本代号
    std::string email;       // 当前账号
    std::string regPath;     // 本地账号存哪个注册表项
    int  timeoutMs;
    int  timeOffsetMs;       // 服务器时间 - 本机时间
    bool debug;
    bool allowLogout;        // 界面里的「退出本机账号」按钮，默认不显示
    bool autoAlert;          // 宿主没接管事件时，由库直接提醒（换机顶号、快到期）
    int  expireWarnDays;     // 到期前几天提醒
    std::string expireWarnDay;   // 已经提醒过的日期，同一天不重复弹

    Grant   grant;
    OemInfo oem;

    std::string lastCode;    // 最近一次接口错误码
    std::string lastMsg;     // 最近一次服务器用户文案
    std::string captchaId;
    std::string captchaB64;
    std::string requestId;

    HWND hParent;
    State() : timeoutMs(20000), timeOffsetMs(0), debug(false), allowLogout(false),
              autoAlert(true), expireWarnDays(3), hParent(NULL) {}
};

State& S();

// ---------------- 接口层 ----------------
struct ApiResponse {
    bool transportOk;   // 网络与信封解析成功
    bool ok;            // 业务成功
    std::string code;
    std::string msg;
    JVal data;
    std::string raw;
    ApiResponse() : transportOk(false), ok(false) {}
};

ApiResponse ApiCall(const std::string& route, const std::string& businessJson);
std::string CommonJsonBody();          // 带上产品类型/渠道/版本等公共字段
std::string BuildEnvelope(const std::string& route, const std::string& businessJson, std::string* tsOut);
bool ParseEnvelope(const std::string& route, const std::string& ts, const std::string& body, ApiResponse& out);

void SaveAccount(const std::string& email);
std::string LoadAccount();
void ApplyGrant(const JVal& data);
std::string NowBeijingText();
long long NowMs();   // 校准过的 UTC 毫秒时间戳
void SetApiError(const std::string& code, const std::string& msg);

// 业务动作（界面与命令共用）
std::string DoLogin(const std::string& email);
std::string DoRefresh(const std::string& email);
std::string DoRegister(const std::string& email, const std::string& qq, const std::string& referee,
                       const std::string& phone, const std::string& learn, const std::string& remarks,
                       const std::string& captchaId, const std::string& captchaCode);
std::string DoTrial(const std::string& email);
std::string DoActivate(const std::string& email, const std::string& keyNum, const std::string& captchaId, const std::string& captchaCode);
std::string DoChangeProbe(const std::string& email);
std::string DoChangeConfirm(const std::string& email, const std::string& keyNum, const std::string& captchaId,
                            const std::string& captchaCode, const std::string& requestId);
std::string DoLoadOem();
std::string DoSyncTime();
void DoSendQuitEvent();

// 供界面取验证码
bool FetchCaptcha(std::string* idOut, std::string* pngOut);

// ---------------- 错误类别 ----------------
// 错误码只说明「哪一步没过」，宿主分支要的是「该做什么」，所以按处置口径归类。
// 编号只往末尾追加，宿主的判断分支才不会随版本漂移。
enum CxAhErrCat {
    CXCAT_NONE = 0,             // 成功
    CXCAT_NETWORK = 1,          // 连不上、超时、被防火墙拦
    CXCAT_ACCOUNT_INVALID = 2,  // 账号不存在 / 未注册 / 重复注册
    CXCAT_ACCOUNT_FROZEN = 3,   // 账号被停止使用（冻结）
    CXCAT_EXPIRED = 4,          // 授权已到期
    CXCAT_NO_GRANT = 5,         // 还没开通授权
    CXCAT_NEED_CHANGE = 6,      // 机器码不符，要走换机
    CXCAT_CHANGE_LIMIT = 7,     // 换机次数已用完
    CXCAT_CHANGE_VERIFY = 8,    // 换机验证不通过或次数过多被锁
    CXCAT_BAD_KEY = 9,          // 序列号问题
    CXCAT_CAPTCHA = 10,         // 验证码问题
    CXCAT_TRIAL_USED = 11,      // 免费试用已用过
    CXCAT_PARAM = 12,           // 参数不全 / 软件或机器码校验异常 / 时间偏差
    CXCAT_CHANNEL = 13,         // 渠道或版本已停止服务
    CXCAT_SERVER = 14,          // 服务器繁忙或维护
    CXCAT_UNKNOWN = 15
};

int ErrorCategory(const std::string& code);

// ---------------- 事件与自动刷新 ----------------
// 事件码：宿主用 会员_设置事件窗口 收 WM_USER 消息，wParam 即事件码；
// 或 会员_设置事件回调 传子程序指针（只在登记它的线程上触发）；
// 或随时 会员_取事件 轮询，返回一条 JSON：{"e":码,"n":"名字","t":"时间","d":{...}}。
enum CxAhEvent {
    EV_NONE = 0,
    EV_LOGIN_OK = 10, EV_LOGIN_FAIL = 11,
    EV_REGISTER_OK = 20, EV_TRIAL_OK = 21,
    EV_ACTIVATE_OK = 30,
    EV_CHANGE_OK = 40, EV_MAC_CHANGED = 41,
    EV_AUTH_REFRESHED = 50, EV_AUTH_EXPIRED = 51, EV_AUTH_FROZEN = 52, EV_AUTH_LOST = 53,
    EV_AUTH_SOON_EXPIRE = 54,
    EV_LOGOUT = 60,
    EV_AUTO_STARTED = 70, EV_AUTO_STOPPED = 71,
    EV_SERVER_UNREACHABLE = 90,
    EV_WINDOW_CLOSE = 80
};

// 回调约定 __stdcall，参数是事件码，返回值 1 表示宿主已接管（库不再做默认提醒），
// 返回 0 或不登记则由库给出默认提示；要拿详情就在回调里调 会员_取事件。
typedef INT (__stdcall *PFN_CX_EVENT)(INT eventCode);

void AlertMacChanged();
void AlertSoonExpire(int days);
void RevokeLocalGrant();

void FireEvent(int code, const std::string& fields);   // fields 形如 {"a":"1"}，可空
// 需要「宿主没接管就替它提醒」的事件走这个：先同步回调，再给窗口宿主一段宽限期。
bool FireEventWait(int code, const std::string& fields, int graceMs);
void MarkEventHandled(int code);
bool IsEventHandled(int code);
int  DaysToExpire();                       // 按校准后的服务器时间算，负数=已过期，无授权=0
void SetAlertPolicy(bool autoAlert, int warnDays);
bool IsAutoAlertOn();
int  GetExpireWarnDays();
bool IsAuthUsable(const std::string& status);
void FireAuthChange(const std::string& before, const std::string& after);
bool EventSetWindow(HWND h, UINT msgId);
bool EventSetCallback(PFN_CX_EVENT cb);
bool EventPoll(std::string* jsonOut);
int  EventCount();
void EventClear();
const char* EventName(int code);                        // UTF-8 中文说明
bool AutoStart(int seconds, HWND h, UINT msgId);
void AutoStop();
int  AutoInterval();
void EventShutdown();

void LogLine(const std::string& text);
std::string GbkToUtf8(const std::string& gbk);      // 库内中文常量 -> UTF-8 出口
std::string LocalCodeText(const std::string& code);   // 错误码 -> 中文说明

}  // namespace cx

#endif
