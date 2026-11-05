// 会员管理类（库定义数据类型）的离线宿主：不启易语言 IDE，直接把 out\cxah.fne 装载起来，
// 1) 校验类型登记（构造/复制/析构各一个、成员命令下标全部指向 m_shtCategory == -1 的命令）；
// 2) 按命令表声明的参数类型自动造实参，把每一条成员命令真跑一遍；
// 3) 断言覆盖：成员命令要么被跑到，要么在跳过表里写明理由。
// 这样以后往类里加方法，忘了跑它就直接红，不会悄悄漏测。
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <deque>

typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;
#include <lib2.h>
#include <lang.h>

static int g_failed = 0;
static int g_passed = 0;

INT WINAPI FakeNotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    UNREFERENCED_PARAMETER(dwParam2);
    switch (nMsg) {
    case NRS_MALLOC: return (INT)(INT_PTR)malloc(dwParam1 ? dwParam1 : 1);
    case NRS_MFREE: if (dwParam1) free((void*)(INT_PTR)dwParam1); return 0;
    case NRS_RUNTIME_ERR: printf("    [运行时错误] %s\n", (const char*)(INT_PTR)dwParam1); return 0;
    default: return 0;
    }
}

static void Check(bool cond, const char* name, const char* detail)
{
    if (cond) { g_passed++; printf("  [ OK ] %s\n", name); }
    else { g_failed++; printf("  [FAIL] %s  %s\n", name, detail ? detail : ""); }
}

static std::string TrimTail(const std::string& s, size_t cap)
{
    return s.size() <= cap ? s : s.substr(0, cap);
}

// ---------------- 库装载 ----------------
static PLIB_INFO g_info = NULL;
static int g_memberCount = 0;
static int g_ran = 0;
static int g_skipped = 0;

static int FindCmd(const char* name)
{
    for (int i = 0; i < g_info->m_nCmdCount; ++i) {
        if (_tcscmp(g_info->m_pBeginCmdInfo[i].m_szName, (LPCTSTR)name) == 0) return i;
    }
    return -1;
}

static EXCEPTION_POINTERS* g_exc = NULL;
static LONG WINAPI HostExcFilter(EXCEPTION_POINTERS* ep) { g_exc = ep; return EXCEPTION_EXECUTE_HANDLER; }

// 对象内部指针的槽位：易语言给类的 4 字节内存，库里放 CxMemberObj*
static INT g_slot = 0;
static INT g_slotCopy = 0;

static std::string g_server, g_secret, g_email, g_keyOwn, g_keyNew, g_mac;

// 造实参时给出的文本必须有地方存着，否则 m_pText 指着一个已经析构的临时串。
// 用 deque 不用 vector：vector 扩容会把先前元素的地址搬走，前面几个参数就成悬垂指针了。
static std::deque<std::string> g_keep;

// 按参数名给一个有意义的实参；表里换了名字也不用改调用处
static std::string ArgTextByName(const char* argName)
{
    if (strstr(argName, "服务器") || strstr(argName, "接口地址")) return g_server;
    if (strstr(argName, "密钥")) return g_secret;
    if (strstr(argName, "产品类型")) return "3";
    if (strstr(argName, "代理") || strstr(argName, "渠道KEY")) return "cxangsoft";
    if (strstr(argName, "机器码")) return g_mac;
    if (strstr(argName, "程序版本")) return "20260001";
    if (strstr(argName, "MD5")) return "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    if (strstr(argName, "系统版本")) return "12";
    if (strstr(argName, "邮箱") || strstr(argName, "账号")) return g_email;
    if (strstr(argName, "序列号")) return g_keyOwn;
    if (strstr(argName, "验证码") || strstr(argName, "申请号")) return "";
    if (strstr(argName, "QQ")) return "10001";
    if (strstr(argName, "手机")) return "13800000000";
    if (strstr(argName, "了解")) return "host";
    if (strstr(argName, "备注")) return "host-class";
    if (strstr(argName, "关联")) return "";
    if (strstr(argName, "字段名")) return "email";
    if (strstr(argName, "键名")) return "email";
    if (strstr(argName, "默认值")) return "d";
    if (strstr(argName, "路由")) return "common.time";
    if (strstr(argName, "请求体")) return "{}";
    return "";
}

static int ArgIntByName(const char* argName)
{
    if (strstr(argName, "超时")) return 20000;
    if (strstr(argName, "间隔")) return 60;
    if (strstr(argName, "天数")) return 3;
    if (strstr(argName, "初始页")) return 0;
    if (strstr(argName, "句柄")) return 0;
    if (strstr(argName, "消息号")) return 0;
    if (strstr(argName, "回调")) return 0;
    if (strstr(argName, "事件码")) return 50;
    return 0;
}

struct CallRes {
    bool crashed;
    int kind;                 // 0 文本 1 整数 2 布尔 3 其它
    std::string text;
    int num;
    bool flag;
    CallRes() : crashed(false), kind(3), num(0), flag(false) {}
};

// 按命令表声明的参数类型造实参；成员命令的 pArgInf[0] 固定是对象自身。
static CallRes Run(int idx, INT* objSlot, bool = false)
{
    CallRes r;
    const CMD_INFO& ci = g_info->m_pBeginCmdInfo[idx];
    std::vector<MDATA_INF> args;
    args.resize(ci.m_nArgCount + 1);
    for (size_t i = 0; i < args.size(); ++i) memset(&args[i], 0, sizeof(MDATA_INF));

    args[0].m_dtDataType = MAKELONG(1, 0);
    args[0].m_pCompoundData = objSlot;
    for (int k = 1; k <= ci.m_nArgCount; ++k) {
        const ARG_INFO& ai = ci.m_pBeginArgInfo[k - 1];
        const std::string name = (const char*)ai.m_szName;
        const DATA_TYPE t = ai.m_dtType;
        MDATA_INF& a = args[k];
        if (t != SDT_INT && t != SDT_BOOL && t != SDT_BIN && t != SDT_TEXT && t != SDT_DATE_TIME) {
            // 不是系统简单类型，就是又一个会员管理对象（复制函数的参数）
            a.m_dtDataType = (INT)t;
            a.m_pCompoundData = &g_slot;
        } else if (t == SDT_INT) {
            a.m_dtDataType = SDT_INT;
            a.m_int = ArgIntByName(name.c_str());
        } else if (t == SDT_BOOL) {
            a.m_dtDataType = SDT_BOOL;
            a.m_bool = BL_TRUE;
        } else if (t == SDT_BIN) {
            a.m_dtDataType = SDT_BIN;
        } else {
            a.m_dtDataType = SDT_TEXT;
            g_keep.push_back(ArgTextByName(name.c_str()));
            a.m_pText = (char*)g_keep.back().c_str();
        }
    }
    if (ci.m_nArgCount == 0) {
        // 只有对象自身
        args.resize(1);
    }

    MDATA_INF ret;
    memset(&ret, 0, sizeof(ret));
    g_exc = NULL;
    __try {
        g_info->m_pCmdsFunc[idx](&ret, (int)args.size(), &args[0]);
    } __except (HostExcFilter(GetExceptionInformation())) {
        r.crashed = true;
    }
    if (g_exc != NULL) {
        r.crashed = true;
        printf("    [CRASH] cmd#%d %s code=%08X addr=%p\n", idx, (const char*)ci.m_szName,
               (unsigned)g_exc->ExceptionRecord->ExceptionCode, (void*)g_exc->ExceptionRecord->ExceptionAddress);
        fflush(stdout);
        return r;
    }
    if (ret.m_dtDataType == SDT_TEXT) {
        r.kind = 0;
        if (ret.m_pText) { r.text = ret.m_pText; free(ret.m_pText); }
    } else if (ret.m_dtDataType == SDT_INT) {
        r.kind = 1;
        r.num = ret.m_int;
    } else if (ret.m_dtDataType == SDT_BOOL) {
        r.kind = 2;
        r.flag = ret.m_bool != BL_FALSE;
    }
    return r;
}

// ---------------- 造机器码（与 host_test 同规则） ----------------
static std::string MakeMac(const char* puc, int seed)
{
    char cpu[32];
    sprintf(cpu, "000506EA%08X", (unsigned)(seed * 2654435761u));
    char seg2[32];
    long long diskVal = (long long)seed * 1024 - (long long)strlen(cpu);
    if (diskVal < 0) diskVal = -diskVal;
    sprintf(seg2, "%lld", diskVal);
    const int len = (int)(strlen(seg2) + strlen(cpu));
    char s1[8], s4[8], s6[16], s7[12], rightBuf[24], plain[160];
    sprintf(s1, "%d", len); s1[2] = 0;
    sprintf(s4, "%d", len);
    long long right = (long long)len * atoll(s1);
    if (right < 0) right = -right;
    sprintf(rightBuf, "%lld", right);
    const size_t rl = strlen(rightBuf);
    sprintf(s6, "%s", rl > 2 ? rightBuf + rl - 2 : rightBuf);
    sprintf(s7, "%08X", (unsigned)(0x1234567u + seed));
    sprintf(plain, "%s-%s-%s-%s-%s-%s-%s-MACVER2", s1, seg2, cpu, s4, puc, s6, s7);

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

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    const char* fne = (argc > 1) ? argv[1] : "out\\cxah.fne";
    g_server = (argc > 2) ? argv[2] : "http://127.0.0.1:8317/api.php";
    g_secret = (argc > 3) ? argv[3] : "CXAH-DEV-SECRET-CHANGE-ME";
    g_email = (argc > 4) ? argv[4] : "cxah-host@test.local";
    g_keyOwn = (argc > 5) ? argv[5] : "TESTKEY-HOST-OWN";
    g_keyNew = (argc > 6) ? argv[6] : "TESTKEY-HOST-NEW";
    const char* keyFeat = (argc > 7) ? argv[7] : "HOSTKEY-FEAT";

    printf("== 类宿主 装载 %s ==\n", fne);
    HMODULE mod = LoadLibraryA(fne);
    if (mod == NULL) { printf("[FAIL] 装载失败\n"); return 1; }
    typedef PLIB_INFO(WINAPI* FnGet)();
    FnGet getNewInf = (FnGet)GetProcAddress(mod, "GetNewInf");
    if (getNewInf == NULL) { printf("[FAIL] 无 GetNewInf\n"); return 1; }
    g_info = getNewInf();
    if (g_info == NULL) { printf("[FAIL] 库信息为空\n"); return 1; }
    PFN_NOTIFY_LIB notify = g_info->m_pfnNotify;
    if (notify == NULL) { printf("[FAIL] 无库通知函数\n"); return 1; }
    notify(NL_SYS_NOTIFY_FUNCTION, (DWORD)FakeNotifySys, 0);

    // ---------------- 类型登记 ----------------
    printf("-- 类型登记 --\n");
    Check(g_info->m_nDataTypeCount == 1, "登记了 1 个类", NULL);
    const LIB_DATA_TYPE_INFO* dt = &g_info->m_pDataType[0];
    Check(_tcscmp(dt->m_szName, (LPCTSTR)"会员管理") == 0, "类名是 会员管理", (const char*)dt->m_szName);
    // 类型序号 = 它在 m_pDataType 里的 1 起位置，成员参数里写的 MAKELONG(1,0) 要能对上
    Check(dt->m_pnCmdsIndex != NULL && dt->m_nCmdCount > 0, "成员命令下标表已填", NULL);
    Check(dt->m_pElementBegin != NULL && dt->m_nElementCount == 1, "只有一个隐藏子成员", NULL);
    Check((dt->m_pElementBegin[0].m_dwState & LES_HIDED) != 0, "子成员是隐藏的", NULL);
    Check(dt->m_nEventCount == 0 && dt->m_nPropertyCount == 0, "非组件类不填事件与属性", NULL);

    int memberTotal = 0;
    for (int i = 0; i < g_info->m_nCmdCount; ++i) {
        if (g_info->m_pBeginCmdInfo[i].m_shtCategory == -1) memberTotal++;
    }
    g_memberCount = memberTotal;
    {
        char d[64];
        sprintf(d, "表里 %d 条，下标表 %d 条", memberTotal, dt->m_nCmdCount);
        Check(memberTotal == dt->m_nCmdCount, "成员命令条数与下标表一致", d);
    }
    int outOfRange = 0, notMember = 0;
    for (int k = 0; k < dt->m_nCmdCount; ++k) {
        const int i = dt->m_pnCmdsIndex[k];
        if (i < 0 || i >= g_info->m_nCmdCount) { outOfRange++; continue; }
        if (g_info->m_pBeginCmdInfo[i].m_shtCategory != -1) notMember++;
    }
    Check(outOfRange == 0, "成员命令下标都在命令表范围内", NULL);
    Check(notMember == 0, "下标表指向的全是成员命令", NULL);

    int ctor = 0, copy = 0, dtor = 0;
    for (int k = 0; k < dt->m_nCmdCount; ++k) {
        const DWORD st = g_info->m_pBeginCmdInfo[dt->m_pnCmdsIndex[k]].m_wState;
        if (st & CT_IS_OBJ_CONSTURCT_CMD) ctor++;
        if (st & CT_IS_OBJ_COPY_CMD) copy++;
        if (st & CT_IS_OBJ_FREE_CMD) dtor++;
    }
    {
        char d[64];
        sprintf(d, "构造 %d 复制 %d 析构 %d", ctor, copy, dtor);
        Check(ctor == 1 && copy == 1 && dtor == 1, "构造/复制/析构 各且仅一个", d);
    }

    // ---------------- 生命周期 ----------------
    printf("-- 生命周期 --\n");
    const int iCtor = FindCmd("会员对象_构造函数");
    const int iCopy = FindCmd("会员对象_复制函数");
    const int iDtor = FindCmd("会员对象_析构函数");
    Check(iCtor >= 0 && iCopy >= 0 && iDtor >= 0, "三条生命周期命令都在表里", NULL);
    if (iCtor < 0) return 1;

    CallRes cr = Run(iCtor, &g_slot, false);
    Check(!cr.crashed && g_slot != 0, "构造后对象槽非 0", NULL);

    const int iInit = FindCmd("初始化");
    if (iInit < 0) { printf("[FAIL] 类里没有 初始化\n"); return 1; }
    g_mac = MakeMac("3", 777001);
    CallRes ri = Run(iInit, &g_slot, false);
    Check(!ri.crashed && ri.kind == 1 && ri.num == 0, "对象初始化返回 0", NULL);
    g_ran += 2;   // 构造 + 初始化

    // 新种子里这个账号还没注册，先注册再试用，才谈得上登录拿授权
    const int iReg = FindCmd("注册");
    if (iReg >= 0) {
        CallRes x = Run(iReg, &g_slot, false);
        Check(!x.crashed && x.kind == 0 && x.text == "0", "对象注册账号", x.text.c_str());
        g_ran++;
    }
    const int iTrial = FindCmd("开通试用");
    if (iTrial >= 0) {
        CallRes x = Run(iTrial, &g_slot, false);
        Check(!x.crashed && x.kind == 0 && x.text == "0", "对象开通试用", x.text.c_str());
        g_ran++;
    }
    const int iLogin = FindCmd("登录");
    CallRes rl = (iLogin >= 0) ? Run(iLogin, &g_slot, false) : CallRes();
    Check(!rl.crashed && rl.kind == 0 && rl.text == "0", "对象登录拿到授权", rl.text.c_str());
    if (iLogin >= 0) g_ran++;

    const int iRefresh = FindCmd("刷新授权");
    if (iRefresh >= 0) {
        CallRes x = Run(iRefresh, &g_slot, false);
        Check(!x.crashed && x.text == "0", "对象刷新授权", x.text.c_str());
        g_ran++;
    }

    // 复制函数：必须给出新的指针，否则两个对象析构时释放同一块内存
    printf("-- 复制与析构 --\n");
    g_slotCopy = 0;
    {
        MDATA_INF args[2];
        memset(args, 0, sizeof(args));
        args[0].m_dtDataType = MAKELONG(1, 0);
        args[0].m_pCompoundData = &g_slotCopy;      // 目标对象（未初始化）
        args[1].m_dtDataType = MAKELONG(1, 0);
        args[1].m_pCompoundData = &g_slot;          // 来源对象
        g_exc = NULL;
        __try { g_info->m_pCmdsFunc[iCopy](&args[0], 2, args); }
        __except (HostExcFilter(GetExceptionInformation())) {}
        char d[96];
        sprintf(d, "src=%p dst=%p", (void*)(INT_PTR)g_slot, (void*)(INT_PTR)g_slotCopy);
        Check(g_exc == NULL && g_slotCopy != 0 && g_slotCopy != g_slot, "复制函数给的是新对象", d);
    }
    g_ran++;

    // 取上下文文本 = 一次拿全授权上下文
    {
        const int i = FindCmd("取上下文文本");
        CallRes x = Run(i, &g_slot, false);
        Check(!x.crashed && x.kind == 0 && x.text.find('{') != std::string::npos, "取上下文文本返回 JSON",
              TrimTail(x.text, 120).c_str());
        g_ran++;
    }

    // ---------------- 其余成员命令逐条真跑 ----------------
    printf("-- 成员命令全覆盖 --\n");
    const int skipIds[] = { iCtor, iCopy, iDtor, iInit, iLogin, iReg, iTrial, iRefresh };
    struct Skip { const char* name; const char* reason; };
    const Skip skips[] = {
        { "打开会员窗口", "交互式模态窗口，需真人操作；同一段实现在界面宿主的 会员_显示注册窗口 那一路真跑并断言" },
        { "打开序列号窗口", "同上，界面宿主驱动 会员_显示续费窗口" },
        { "打开换机窗口", "同上，界面宿主驱动 会员_显示换机窗口" },
        { "打开用户中心", "同上，界面宿主第四轮真跑并断言字段" },
    };
    int ranHere = 0, skipHere = 0;
    std::string lastJson;
    for (int k = 0; k < dt->m_nCmdCount; ++k) {
        const int idx = dt->m_pnCmdsIndex[k];
        bool done = false;
        for (unsigned q = 0; q < sizeof(skipIds) / sizeof(skipIds[0]); ++q) {
            if (skipIds[q] == idx) { done = true; break; }
        }
        if (done) { ranHere++; continue; }
        const char* name = (const char*)g_info->m_pBeginCmdInfo[idx].m_szName;
        bool isSkip = false;
        for (unsigned q = 0; q < sizeof(skips) / sizeof(skips[0]); ++q) {
            if (strcmp(name, skips[q].name) == 0) {
                isSkip = true;
                printf("  [跳过] %s：%s\n", name, skips[q].reason);
                break;
            }
        }
        if (isSkip) { skipHere++; continue; }
        CallRes x = Run(idx, &g_slot, false);
        if (x.crashed) { g_failed++; printf("  [FAIL] 成员命令崩了：%s\n", name); continue; }
        if (strcmp(name, "取事件") == 0) lastJson = x.text;
        if (strcmp(name, "停止自动刷新") == 0 || strcmp(name, "清空事件") == 0) { /* 顺序在后面单独跑 */ }
        ranHere++;
    }
    {
        char d[96];
        sprintf(d, "跑到 %d 条，有理由跳过 %d 条，类里共 %d 条", ranHere, skipHere, dt->m_nCmdCount);
        Check(ranHere + skipHere == dt->m_nCmdCount, "每条成员命令都被跑到或写明跳过理由", d);
    }
    g_ran = ranHere;

    // ---------------- 事件、自动刷新、提醒策略 ----------------
    printf("-- 事件与自动刷新 --\n");
    {
        const int i = FindCmd("启动自动刷新");
        CallRes x = Run(i, &g_slot, false);
        Check(!x.crashed && x.kind == 2 && x.flag, "对象启动自动刷新", NULL);
        const int j = FindCmd("取自动刷新间隔");
        CallRes y = Run(j, &g_slot, false);
        Check(y.kind == 1 && y.num >= 5, "自动刷新间隔生效", NULL);
        Sleep(1200);
        const int c = FindCmd("取事件数");
        CallRes z = Run(c, &g_slot, false);
        Check(z.kind == 1 && z.num >= 1, "后台刷新产生了事件", NULL);
        const int s = FindCmd("停止自动刷新");
        CallRes w = Run(s, &g_slot, false);
        Check(w.kind == 2 && w.flag, "对象停止自动刷新", NULL);
        Check(Run(FindCmd("取自动刷新间隔"), &g_slot, false).num == 0, "已停", NULL);
    }
    {
        const int i = FindCmd("设置提醒策略");
        CallRes x = Run(i, &g_slot, false);
        Check(!x.crashed, "对象设置提醒策略", NULL);
        const int j = FindCmd("取到期提醒天数");
        Check(Run(j, &g_slot, false).kind == 1, "对象取到期提醒天数", NULL);
        const int k = FindCmd("是否自动提醒");
        CallRes y = Run(k, &g_slot, false);
        Check(y.kind == 2, "对象是否自动提醒", NULL);
    }
    {
        const int i = FindCmd("事件已接管");
        CallRes x = Run(i, &g_slot, false);
        Check(!x.crashed && x.flag, "对象事件已接管", NULL);
        const int j = FindCmd("取事件名");
        Check(!Run(j, &g_slot, false).text.empty(), "对象取事件名有说明", NULL);
        const int k = FindCmd("清空事件");
        Check(!Run(k, &g_slot, false).crashed, "对象清空事件", NULL);
        Check(Run(FindCmd("取事件数"), &g_slot, false).num == 0, "队列已清空", NULL);
    }

    printf("-- 授权读取与工具方法 --\n");
    {
        // 上面的覆盖循环里跑到了「退出账号」，本机授权被清掉；先装回来再验读取类方法
        CallRes back = Run(iInit, &g_slot, false);
        Check(!back.crashed && back.num == 0, "重新初始化以便验证读取方法", NULL);
        Run(iLogin, &g_slot, false);

        const int i = FindCmd("是否已授权");
        Check(!Run(i, &g_slot, false).crashed, "对象是否已授权", NULL);
        const int j = FindCmd("取授权状态");
        Check(!Run(j, &g_slot, false).text.empty(), "对象取授权状态非空", NULL);
        const int k = FindCmd("取错误类别");
        Check(Run(k, &g_slot, false).kind == 1, "对象取错误类别返回整数", NULL);
        const int m = FindCmd("校验机器码");
        Check(!Run(m, &g_slot, false).crashed, "对象校验机器码", NULL);
        const int p = FindCmd("请求接口");
        CallRes x = Run(p, &g_slot, false);
        Check(x.text.find("ok") != std::string::npos, "对象请求接口返回信封", TrimTail(x.text, 80).c_str());
        const int q = FindCmd("取库版本");
        Check(Run(q, &g_slot, false).text.find('.') != std::string::npos, "对象取库版本", NULL);
    }

    // ---------------- 析构 ----------------
    printf("-- 析构 --\n");
    {
        CallRes x = Run(iDtor, &g_slotCopy, false);
        Check(!x.crashed && g_slotCopy == 0, "析构后复制出来的对象槽归零", NULL);
        x = Run(iDtor, &g_slot, false);
        Check(!x.crashed && g_slot == 0, "析构后对象槽归零", NULL);
    }

    printf("\n类成员命令 %d 条，本宿主跑到 %d 条，有理由跳过 %d 条\n", dt->m_nCmdCount, ranHere, skipHere);
    printf("通过 %d 项，失败 %d 项\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
