// 创想AH会员框架 · 壳子演示程序
// 不依赖易语言 IDE：按支持库的正规装载顺序（GetNewInf → NL_SYS_NOTIFY_FUNCTION → 命令表）接入 cxah.fne，
// 初始化后真开会员窗口，关窗后打印拿到的授权信息。用来肉眼看界面效果与验证库可被宿主装载。
//   cx_demo.exe [api地址] [协议密钥] [产品类型] [代理KEY]
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <tchar.h>

typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;

#include <lib2.h>
#include <lang.h>

static PFN_NOTIFY_SYS g_notify = NULL;

INT WINAPI HostNotify(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    UNREFERENCED_PARAMETER(dwParam2);
    switch (nMsg) {
    case NRS_MALLOC: return (INT)(INT_PTR)malloc(dwParam1 ? dwParam1 : 1);
    case NRS_MFREE: if (dwParam1) free((void*)(INT_PTR)dwParam1); return 0;
    case NRS_RUNTIME_ERR: printf("[runerr] %s\n", (const char*)(INT_PTR)dwParam1); return 0;
    default: return 0;
    }
}

static PLIB_INFO g_info = NULL;

static int FindCmd(const char* name)
{
    for (int i = 0; i < g_info->m_nCmdCount; ++i) {
        if (_tcscmp(g_info->m_pBeginCmdInfo[i].m_szName, (LPCTSTR)name) == 0) return i;
    }
    return -1;
}

struct Ret { std::string text; int num; bool flag; };

static Ret Do(const char* name, const std::vector<const char*>& texts, const std::vector<int>& ints)
{
    Ret bad; bad.num = -999; bad.flag = false;
    const int idx = FindCmd(name);
    if (idx < 0) { printf("  [!] 找不到命令：%s\n", name); return bad; }

    MDATA_INF args[12];
    int n = 0;
    for (size_t k = 0; k < texts.size() && n < 12; ++k) {
        memset(&args[n], 0, sizeof(MDATA_INF));
        args[n].m_dtDataType = SDT_TEXT;
        args[n].m_pText = (char*)(texts[k] ? texts[k] : "");
        n++;
    }
    for (size_t k = 0; k < ints.size() && n < 12; ++k) {
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
    if (ret.m_dtDataType == SDT_TEXT && ret.m_pText) { out.text = ret.m_pText; free(ret.m_pText); }
    else if (ret.m_dtDataType == SDT_INT) { out.num = ret.m_int; char b[16]; sprintf(b, "%d", ret.m_int); out.text = b; }
    else if (ret.m_dtDataType == SDT_BOOL) { out.flag = (ret.m_bool != BL_FALSE); out.num = out.flag ? 1 : 0; }
    return out;
}

static std::wstring U8(const std::string& utf8)
{
    if (utf8.empty()) return std::wstring();
    const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), NULL, 0);
    std::wstring w((size_t)(n > 0 ? n : 0), wchar_t(0));
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &w[0], n);
    return w;
}

static std::string LocalOf(const std::string& utf8)   // UTF-8 -> 控制台 GBK
{
    const std::wstring w = U8(utf8);
    if (w.empty()) return std::string();
    const int n = WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string out((size_t)(n > 0 ? n : 0), char(0));
    if (n > 0) WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(), &out[0], n, NULL, NULL);
    return out;
}

static std::string MakeMac(const char* puc, int seed)
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
    char s7[12]; sprintf(s7, "%08X", (unsigned)(0x2468135u + seed));
    char plain[190];
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
    SetConsoleOutputCP(936);
    const char* server = (argc > 1) ? argv[1] : "http://127.0.0.1:8317/api.php";
    const char* secret = (argc > 2) ? argv[2] : "CXAH-DEV-SECRET-CHANGE-ME";
    const char* puc    = (argc > 3) ? argv[3] : "3";
    const char* oemKey = (argc > 4) ? argv[4] : "cxangsoft";
    const int startPage = (argc > 5) ? atoi(argv[5]) : 0;

    printf("== 创想AH会员框架 壳子演示 ==\n");
    printf("接口：%s   产品：%s   渠道KEY：%s\n\n", server, puc, oemKey);

    HMODULE mod = LoadLibraryA("cxah.fne");
    if (mod == NULL) mod = LoadLibraryA("out\\cxah.fne");
    if (mod == NULL) { printf("[x] 装载 cxah.fne 失败，把它和本程序放同一目录\n"); return 1; }
    typedef PLIB_INFO(WINAPI* FnGet)();
    FnGet getNewInf = (FnGet)GetProcAddress(mod, "GetNewInf");
    if (getNewInf == NULL) { printf("[x] 无 GetNewInf\n"); return 1; }
    g_info = getNewInf();
    printf("[1] 支持库已装载：%s  命令 %d 条\n", (const char*)g_info->m_szName, g_info->m_nCmdCount);
    g_info->m_pfnNotify(NL_SYS_NOTIFY_FUNCTION, (DWORD)HostNotify, 0);

    const std::string mac = MakeMac(puc, (int)(GetTickCount() & 0x7FFF) + 1000);
    printf("[2] 本机机器码（演示用，真实程序由主程序生成）：\n    %.60s...\n", mac.c_str());

    std::vector<const char*> t;
    t.push_back(server); t.push_back(puc); t.push_back(oemKey); t.push_back(mac.c_str());
    t.push_back("20260001"); t.push_back("11223344556677889900112233445566"); t.push_back("12");
    t.push_back(secret); t.push_back("");
    std::vector<int> iv;
    iv.push_back(20000);
    iv.push_back(1);
    const Ret init = Do("会员_初始化", t, iv);
    if (init.num != 0) {
        printf("[x] 初始化失败，返回码 %d：%s\n", init.num, LocalOf(Do("会员_取服务器文案", std::vector<const char*>(), std::vector<int>()).text).c_str());
        return 2;
    }
    printf("[3] 初始化成功，服务器时间：%s\n", LocalOf(Do("会员_取当前北京时间", std::vector<const char*>(), std::vector<int>()).text).c_str());

    const std::string soft = Do("会员_取OEM信息", std::vector<const char*>{ "soft_name" }, std::vector<int>()).text;
    const std::string qq = Do("会员_取OEM信息", std::vector<const char*>{ "qq" }, std::vector<int>()).text;
    printf("[4] 渠道信息：%ls   客服QQ：%s\n", U8(soft).c_str(), qq.c_str());
    printf("[5] 本地保存的账号：%s\n\n", LocalOf(Do("会员_取本地账号", std::vector<const char*>(), std::vector<int>()).text).c_str());
    printf("    即将打开会员窗口，请直接在窗口里注册/登录/续费/换机，关掉窗口后这里会打印结果。\n");
    fflush(stdout);

    std::vector<int> page;
    const bool onlyRenew = (startPage == 99);
    const bool onlyCenter = (startPage == 98);
    page.push_back(onlyRenew || onlyCenter ? 0 : startPage);
    Ret shown;
    if (startPage == 97) {
        const std::vector<const char*> none;
        printf("    （演示：直接弹换机验证小窗）\n");
        const Ret r = Do("会员_显示换机窗口", none, std::vector<int>());
        printf("\n[6] 换机窗口返回：%s\n", r.text.c_str());
        return 0;
    }
    if (onlyCenter) shown = Do("会员_显示用户中心", std::vector<const char*>(), std::vector<int>());
    else if (onlyRenew) shown = Do("会员_显示续费窗口", std::vector<const char*>(), std::vector<int>());
    else shown = Do("会员_显示注册窗口", std::vector<const char*>(), page);

    const std::vector<const char*> noArg;
    const std::vector<int> noInt;
    printf("\n[6] 窗口已关闭。\n");
    printf("    窗口返回值：%s（真=已拿到授权）\n", shown.flag ? "true" : "false");
    struct Field { const char* name; const char* key; };
    const Field fields[] = {
        { "账号", "email" }, { "到期时间", "due_date" }, { "版本", "function_name" },
        { "增配权限", "soft_limit" }, { "绑定机器码", "mac_key" }, { "最后序列号", "order_number" },
    };
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
        std::vector<const char*> one;
        one.push_back(fields[i].key);
        printf("    %s：%s\n", fields[i].name, LocalOf(Do("会员_取授权信息", one, noInt).text).c_str());
    }
    std::vector<const char*> left1;
    left1.push_back("change_left");
    printf("    剩余换机次数：%s\n", Do("会员_取授权信息", left1, noInt).text.c_str());
    printf("    授权状态：%s\n", Do("会员_取授权状态", noArg, noInt).text.c_str());
    printf("    是否已授权：%s\n", Do("会员_是否已授权", noArg, noInt).flag ? "是" : "否");
    printf("    最近一次服务器文案：%s\n", LocalOf(Do("会员_取服务器文案", noArg, noInt).text).c_str());

    Do("会员_发送退出事件", noArg, noInt);
    printf("\n按任意键结束...\n");
    getchar();
    return 0;
}
