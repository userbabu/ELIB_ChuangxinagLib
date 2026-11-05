// 离线宿主：不启动易语言 IDE，直接装载 out\cxbrow.fne，
// 1) 校验三张表与自定义类型自洽；2) 用真 winsock 客户端扮演浏览器插件，把 /cxa/ 五个端点全打一遍；
// 3) 用一份 stdcall 假子程序模拟易语言接收端，逐字段核对拿到的数据块与回给插件的应答。
// 编译与运行见 build_host.bat。
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <tchar.h>

typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;

#include <lib2.h>
#include <lang.h>

// 库出口是 UTF-8，宿主源码里的中文常量是 GBK，比对中文值要用下面这串 UTF-8 字节
#define U8_MEI_GUO_ZHAN "\xE7\xBE\x8E\xE5\x9B\xBD\xE7\xAB\x99"      // 美国站
#define U8_FA_GUO_ZHAN  "\xE6\xB3\x95\xE5\x9B\xBD\xE7\xAB\x99"      // 法国站
#define U8_DE_GUO_ZHAN  "\xE5\xBE\xB7\xE5\x9B\xBD\xE7\xAB\x99"      // 德国站

// ---------------- 断言 ----------------
static int g_failed = 0;
static int g_passed = 0;
static void Check(bool cond, const char* name, const char* detail)
{
    if (cond) { g_passed++; printf("  [ OK ] %s\n", name); }
    else { g_failed++; printf("  [FAIL] %s  %s\n", name, detail ? detail : ""); fflush(stdout); }
}

// ---------------- 假 NotifySys ----------------
static volatile LONG g_poolOutstanding = 0;
INT WINAPI FakeNotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    UNREFERENCED_PARAMETER(dwParam2);
    switch (nMsg)
    {
    case NRS_MALLOC:
        {
            void* p = malloc(dwParam1 ? dwParam1 : 1);
            if (p != NULL) InterlockedIncrement(&g_poolOutstanding);
            return (INT)(INT_PTR)p;
        }
    case NRS_MFREE:
        if (dwParam1) { InterlockedDecrement(&g_poolOutstanding); free((void*)(INT_PTR)dwParam1); }
        return 0;
    case NRS_RUNTIME_ERR: printf("    [运行时错误] %s\n", (const char*)(INT_PTR)dwParam1); return 0;
    default: return 0;
    }
}

// ---------------- 库装载与命令调用 ----------------
static PLIB_INFO g_info = NULL;

static int FindCmd(const char* name)
{
    for (int i = 0; i < g_info->m_nCmdCount; ++i)
    {
        if (strcmp((const char*)g_info->m_pBeginCmdInfo[i].m_szName, name) == 0) return i;
    }
    printf("[FAIL] 命令表里找不到「%s」\n", name);
    g_failed++;
    return -1;
}

static __declspec(thread) EXCEPTION_POINTERS* g_exc = NULL;
static LONG WINAPI ExcFilter(EXCEPTION_POINTERS* ep) { g_exc = ep; return EXCEPTION_EXECUTE_HANDLER; }

static __declspec(thread) char* g_retText = NULL;

// 易语言内核按 1 字节对齐传参：参数数组里每个 MDATA_INF 占 12 个字节（数据 8 + 数据类型码 4），
// 而本机 MSVC 不认 lib2.h 里 Borland 写法的 pack 指示，sizeof(MDATA_INF) 编出来是 16。
// 宿主必须按 12 摆参数，否则和库一起错位，这个 bug 就验不出来（真机取证：raw=[1 0 80000301 813fd2c 0 20001 ...]）。
#define HOST_ARG_DWORDS 3        // 每个参数 3 个字
static void PutRaw(DWORD* p, DWORD dataLo, DWORD dt)
{
    p[0] = dataLo; p[1] = 0; p[2] = dt;
}

struct Args
{
    DWORD d[8 * HOST_ARG_DWORDS];
    int n;
    Args() : n(0) { memset(d, 0, sizeof(d)); }
    void Add(DWORD data, DWORD dt) { PutRaw(d + n * HOST_ARG_DWORDS, data, dt); ++n; }
    void AddText(const char* s) { Add((DWORD)(INT_PTR)s, SDT_TEXT); }
    void AddNull() { Add(0, _SDT_NULL); }
    void AddInt(int v) { Add((DWORD)v, SDT_INT); }
    void AddBool(int v) { Add(v ? (DWORD)BL_TRUE : (DWORD)BL_FALSE, SDT_BOOL); }
    void AddSub(void* p) { Add((DWORD)(INT_PTR)p, SDT_SUB_PTR); }
    // 参考的库定义数据类型：m_pCompoundData 放「变量的地址」，类型码 = MAKELONG(序号, 模块序号)
    void AddCompound(void* pVarSlot) { Add((DWORD)(INT_PTR)pVarSlot, MAKELONG(1, 2)); }
    PMDATA_INF Ptr() { return (PMDATA_INF)(void*)d; }
};

// 按命令表声明的 m_nArgCount 补齐参数：实现是按 12 字节步长读参数表的，少传就是越界读。
static MDATA_INF CallCmd(const char* name, Args* pArgs)
{
    MDATA_INF ret;
    memset(&ret, 0, sizeof(ret));
    const int idx = FindCmd(name);
    if (idx < 0) { ret.m_dtDataType = _SDT_NULL; return ret; }
    const int n = g_info->m_pBeginCmdInfo[idx].m_nArgCount;
    DWORD argsRaw[8 * HOST_ARG_DWORDS];
    memset(argsRaw, 0, sizeof(argsRaw));
    for (int i = 0; i < n && i < 8; ++i)
    {
        if (pArgs != NULL && i < pArgs->n)
            memcpy(argsRaw + i * HOST_ARG_DWORDS, pArgs->d + i * HOST_ARG_DWORDS, HOST_ARG_DWORDS * sizeof(DWORD));
        else argsRaw[i * HOST_ARG_DWORDS + 2] = _SDT_NULL;
    }
    g_exc = NULL;
    PFN_EXECUTE_CMD pfn = g_info->m_pCmdsFunc[idx];
    __try { if (pfn != NULL) pfn(&ret, n, n ? (PMDATA_INF)(void*)argsRaw : NULL); }
    __except (ExcFilter(GetExceptionInformation())) {}
    if (g_exc != NULL)
    {
        printf("  [CRASH] %s 异常码=%08X\n", name, (unsigned)g_exc->ExceptionRecord->ExceptionCode);
        g_failed++;
    }
    if (ret.m_dtDataType == SDT_TEXT)
    {
        if (g_retText) free(g_retText);
        g_retText = _strdup(ret.m_pText == NULL ? "" : ret.m_pText);
        FakeNotifySys(NRS_MFREE, (DWORD)(INT_PTR)ret.m_pText, 0);
        ret.m_pText = g_retText;
    }
    return ret;
}

static bool CmdBool(const char* name, Args* p = NULL)
{
    MDATA_INF r = CallCmd(name, p);
    if (r.m_dtDataType != SDT_BOOL) { printf("  [FAIL] %s 没返回逻辑型\n", name); g_failed++; return false; }
    return r.m_bool != BL_FALSE;
}
static int CmdInt(const char* name, Args* p = NULL)
{
    MDATA_INF r = CallCmd(name, p);
    if (r.m_dtDataType != SDT_INT) { printf("  [FAIL] %s 没返回整数型\n", name); g_failed++; return -99999; }
    return r.m_int;
}
// 无返回值的命令：只看有没有崩，返回类型必须是 _SDT_NULL
static bool CmdVoid(const char* name, Args* p = NULL)
{
    MDATA_INF r = CallCmd(name, p);
    return r.m_dtDataType == _SDT_NULL && g_exc == NULL;
}
static const char* CmdText(const char* name, Args* p = NULL)
{
    CallCmd(name, p);
    if (g_retText == NULL) g_retText = _strdup("");
    return g_retText;
}

// ---------------- 极简 HTTP 客户端（扮演插件） ----------------
static int g_port = 0;
static char g_key[64] = "";
// 扮演的插件身份：一台浏览器一组值。测多浏览器时按台切换，
// 故意让 Chrome 和 Edge 用同一个扩展 ID（打包 CRX 在两个浏览器里 ID 相同），验证库里是靠「浏览器名」分台的。
static const char* EXT_VER = "0.6.28";
static const char* EXT_BROWSER = "Chrome 139";
static const char* EXT_ID = "abcdefghijklmnopabcdefghijklmnop";

static void AsBrowser(const char* pszVer, const char* pszBrowser, const char* pszId)
{
    EXT_VER = pszVer;
    EXT_BROWSER = pszBrowser;
    EXT_ID = pszId;
}

struct Resp { int status; char body[24576]; };

static bool HttpRaw(const char* method, const char* path, const char* body, const char* keyOverride, Resp* pOut)
{
    if (pOut) { pOut->status = 0; pOut->body[0] = '\0'; }
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return false;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)g_port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) != 0) { closesocket(s); return false; }
    DWORD to = 30000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&to, sizeof(to));

    const char* pszKey = (keyOverride != NULL) ? keyOverride : g_key;
    char req[9216];
    int n;
    if (body != NULL && body[0] != '\0')
    {
        n = _snprintf(req, sizeof(req) - 1,
            "%s /cxa/%s HTTP/1.1\r\nHost: 127.0.0.1:%d\r\nOrigin: chrome-extension://%s\r\n"
            "Accept: application/json\r\nx-cxa-ext: %s\r\nx-cxa-ext-browser: %s\r\nx-cxa-ext-id: %s\r\n"
            "x-cxa-key: %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s",
            method, path, g_port, EXT_ID, EXT_VER, EXT_BROWSER, EXT_ID,
            pszKey, (int)strlen(body), body);
    }
    else
    {
        n = _snprintf(req, sizeof(req) - 1,
            "%s /cxa/%s HTTP/1.1\r\nHost: 127.0.0.1:%d\r\nOrigin: chrome-extension://%s\r\n"
            "Accept: application/json\r\nx-cxa-ext: %s\r\nx-cxa-ext-browser: %s\r\nx-cxa-ext-id: %s\r\n"
            "x-cxa-key: %s\r\n\r\n",
            method, path, g_port, EXT_ID, EXT_VER, EXT_BROWSER, EXT_ID, pszKey);
    }
    if (n <= 0 || n >= (int)sizeof(req)) { closesocket(s); return false; }
    int sent = 0;
    while (sent < n)
    {
        const int w = send(s, req + sent, n - sent, 0);
        if (w <= 0) { closesocket(s); return false; }
        sent += w;
    }
    static char buf[32768];
    int got = 0;
    // 服务端每条应答都带 Connection: close，读到连接关闭为止才算收完
    for (int loop = 0; loop < 64; ++loop)
    {
        const int r = recv(s, buf + got, (int)sizeof(buf) - got - 1, 0);
        if (r <= 0) break;
        got += r;
        buf[got] = '\0';
    }
    const int nErr = WSAGetLastError();
    closesocket(s);
    if (got <= 0)
    {
        if (getenv("CXBROW_DEBUG") != NULL)
            printf("    [net] %s %s recv 0 bytes, WSAErr=%d\n", method, path, nErr);
        return false;
    }
    if (sscanf(buf, "HTTP/1.1 %d", &pOut->status) != 1)
    {
        if (getenv("CXBROW_DEBUG") != NULL) printf("    [net] bad status line: %.60s\n", buf);
        return false;
    }
    char* pBody = strstr(buf, "\r\n\r\n");
    if (pBody == NULL) return false;
    pBody += 4;
    strncpy(pOut->body, pBody, sizeof(pOut->body) - 1);
    pOut->body[sizeof(pOut->body) - 1] = '\0';
    return true;
}

static bool JsonHas(const char* json, const char* frag) { return json != NULL && strstr(json, frag) != NULL; }

// 结构自检：真实扩展是 JSON.parse 整个回包的，这里不用解析器就自己扫一遍括号与逗号，
// 少一个逗号、多一个逗号这类毛病只有走真解析才会暴露。
static bool JsonValid(const char* s)
{
    if (s == NULL || s[0] == '\0') return false;
    int depth = 0;
    bool inStr = false;
    for (int i = 0; s[i] != '\0'; ++i)
    {
        const char c = s[i];
        if (inStr)
        {
            if (c == '\\') { ++i; continue; }
            if (c == '"') inStr = false;
            continue;
        }
        if (c == '"') { inStr = true; continue; }
        if (c == '{' || c == '[')
        {
            ++depth;
            int j = i + 1;
            while (s[j] == ' ' || s[j] == '\t' || s[j] == '\r' || s[j] == '\n') ++j;
            if (s[j] == ',' || s[j] == '}' || s[j] == ']') return false;
            continue;
        }
        if (c == '}' || c == ']')
        {
            --depth;
            int j = i - 1;
            while (j >= 0 && (s[j] == ' ' || s[j] == '\t' || s[j] == '\r' || s[j] == '\n')) --j;
            if (j >= 0 && s[j] == ',') return false;
            if (depth < 0) return false;
            continue;
        }
        if (c == ',')
        {
            int j = i + 1;
            while (s[j] == ' ' || s[j] == '\t' || s[j] == '\r' || s[j] == '\n') ++j;
            if (s[j] == ',' || s[j] == '}' || s[j] == ']') return false;
            int k = i - 1;
            while (k >= 0 && (s[k] == ' ' || s[k] == '\t' || s[k] == '\r' || s[k] == '\n')) --k;
            if (k < 0 || s[k] == '{' || s[k] == '[' || s[k] == ',') return false;
            continue;
        }
    }
    return depth == 0 && !inStr;
}

// 回包必须是合法 UTF-8：插件那边 JSON.parse 之后按 UTF-8 显示，
// 程序名/站点名/提示消息只要是 GBK 字节就会在浏览器里变成一串问号
static bool Utf8Valid(const char* s)
{
    return s != NULL && s[0] != 0 && MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0) > 0;
}

static bool JsonVal(const char* json, const char* key, char* out, int cch)
{
    char pat[64];
    _snprintf(pat, sizeof(pat) - 1, "\"%s\":", key);
    const char* p = strstr(json, pat);
    if (p == NULL) { out[0] = '\0'; return false; }
    p += strlen(pat);
    int i = 0;
    if (*p == '"')
    {
        ++p;
        while (*p != '"' && *p != '\0' && i < cch - 1) out[i++] = *p++;
    }
    else
    {
        while (*p != ',' && *p != '}' && *p != '\0' && i < cch - 1) out[i++] = *p++;
    }
    out[i] = '\0';
    return true;
}

// ---------------- 假易语言接收子程序 ----------------
// 事件块成员序号，必须与 lib.cpp 的 g_eventElements 同序（1.1.0 起只剩入参）
enum { BM_TYPE = 0, BM_SEQ, BM_ARRIVE, BM_URL, BM_TITLE, BM_PAGETYPE, BM_COLMODE,
    BM_PLATFORM, BM_SITE, BM_COUNTRY, BM_CURRENCY, BM_LANGUAGE, BM_ASIN, BM_START,
    BM_EXT_VER, BM_BROWSER, BM_EXT_ID, BM_RAW, BM_TASKNAME, BM_TASKID, BM_BROWSER_COUNT, BM_COUNT };

static volatile int g_cbCount = 0;
static volatile int g_pumpOn = 1;
static int g_cbLastType = -1;
static char g_cbUrl[2048], g_cbAsin[32], g_cbPlatform[64], g_cbSite[64], g_cbExtVer[64];
static char g_cbBrowser[64], g_cbExtId[64], g_cbRaw[2048], g_cbTitle[256], g_cbArrive[64];
static char g_cbTaskName[256];
static volatile int g_cbSawDisconnect = 0;   // 收到过「插件已断开」事件（跨线程晚到时 lastType 会被覆盖）
static volatile int g_cbSawAllGone = 0;      // 收到过「所有浏览器已断开」事件（类型 5）
static int g_servePrint = 0;                 // serve 模式：每个事件打一行关键字段
static char g_cbRecallId[128];               // 撤回事件带回来的任务编号
static int g_cbRecallAnswered = 0;           // 撤回事件在回调里应答成功
static int g_cbRecallAccept = 1;             // 1 = 同意撤回，0 = 拒收（测拒绝路径）
static int g_cbPageType = -1, g_cbColMode = -1, g_cbStart = -1, g_cbSeq = -1;
static volatile int g_cbMode = 0;          // 0 正常接受 1 拒绝
static char g_cbTaskId[32] = "T-77";
static int g_cbRestoreOk = -1;             // 还原事件信息 的返回
static int g_cbAnswerOk = -1;              // 应答采集事件 的返回
static int g_cbStaleOk = -1;               // 过期句柄应当返回假
static int g_cbShapeC = -1;                // 槽是 4 字节且为空指针时，库要回填一块
static char g_cbShapeCAsin[32] = "";
static int g_cbShapeB = -1;                // 地址本身就是易语言刚分配出来的数据块
static char g_cbShapeBAsin[32] = "";
static int g_cbShapeN = -1;                // 变量还没分配：传进来是空白数据，必须报回能读的提示
static char g_cbShapeNMsg[512] = "";
static int g_cbBrowserCount = -1;            // 事件里的「已连接浏览器数量」
// 事件流水：多条浏览器同连时断言要看「顺序 + 每台是谁 + 当时剩几台」，只看最后一个类型不够
struct EvRec { int nType; int nCount; char szId[64]; char szBrowser[64]; };
#define EVLOG_CAP 64
static EvRec g_evLog[EVLOG_CAP];
static volatile int g_evLogNext = 0;         // 绝对条数（环形覆盖，只保证最近 64 条可查）

static void EvLogPush(int nType, int nCount, const char* pszId, const char* pszBrowser)
{
    const int i = (int)InterlockedIncrement((volatile long*)&g_evLogNext) - 1;
    EvRec* p = &g_evLog[i % EVLOG_CAP];
    p->nType = nType;
    p->nCount = nCount;
    strncpy(p->szId, pszId, sizeof(p->szId) - 1);
    p->szId[sizeof(p->szId) - 1] = '\0';
    strncpy(p->szBrowser, pszBrowser, sizeof(p->szBrowser) - 1);
    p->szBrowser[sizeof(p->szBrowser) - 1] = '\0';
}

static const EvRec* EvLogAt(int nAbs)
{
    if (nAbs < 0 || nAbs >= g_evLogNext || (g_evLogNext - nAbs) > EVLOG_CAP) return NULL;
    return &g_evLog[nAbs % EVLOG_CAP];
}

// 从 nAbs 起逐条比对「类型 + 台数」。少一条、多一条、顺序反了都算不过；
// bExact 为真还要求这批之后没有新事件（用来断言「全部断开」只派发一次）。
static bool EvLogIsSeq(int nAbs, const int* pTypes, const int* pCounts, int n, bool bExact)
{
    for (int i = 0; i < n; ++i)
    {
        const EvRec* p = EvLogAt(nAbs + i);
        if (p == NULL) return false;
        if (pTypes[i] >= 0 && p->nType != pTypes[i]) return false;
        if (pCounts != NULL && pCounts[i] >= 0 && p->nCount != pCounts[i]) return false;
    }
    if (bExact && EvLogAt(nAbs + n) != NULL) return false;
    return true;
}

static void* g_pTailSlot = NULL;           // 一页的最后 4 字节：后面不足 84 字节，只能是变量槽

// 造一个「后面没有多余内存」的 4 字节槽：模拟易语言把局部变量排在数据块末尾的情形
static bool TailSlotEnsure()
{
    if (g_pTailSlot != NULL) return true;
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    BYTE* pRes = (BYTE*)VirtualAlloc(NULL, si.dwPageSize * 2, MEM_RESERVE, PAGE_READWRITE);
    if (pRes == NULL) return false;
    if (VirtualAlloc(pRes, si.dwPageSize, MEM_COMMIT, PAGE_READWRITE) == NULL) return false;
    g_pTailSlot = (void*)(pRes + si.dwPageSize - sizeof(void*));
    return true;
}

// 模拟易语言的「局部变量 事件, 浏览器事件信息」：变量槽里存的是数据块指针
static int g_userBlock[BM_COUNT];
static void* g_pUserBlock = g_userBlock;
static int g_ownershipProbe = 0;
static int g_cbIndependentText = 0;
static int g_cbRepeatedRestore = 0;
static int* g_retainedBlock = NULL;

// 模拟易语言销毁复合变量：成员文本和结构块分别归变量所有。
static void FreeEventMembers(int* m)
{
    const int owned[] = { BM_ARRIVE, BM_URL, BM_TITLE, BM_PLATFORM, BM_SITE, BM_COUNTRY,
        BM_CURRENCY, BM_LANGUAGE, BM_ASIN, BM_EXT_VER, BM_BROWSER, BM_EXT_ID,
        BM_RAW, BM_TASKNAME, BM_TASKID };
    for (int i = 0; i < (int)(sizeof(owned) / sizeof(owned[0])); ++i)
    {
        FakeNotifySys(NRS_MFREE, (DWORD)m[owned[i]], 0);
        m[owned[i]] = 0;
    }
}

// 回调形参是一个整数型句柄：易语言不允许把带自定义类型形参的子程序取地址（错误 10063）
static volatile int g_cbDone = 0;            // 回调已返回的条数：断言要等这个，等「已开始」会读到半成品
// 原子性探针：同时在子程序里跑着的线程数与观测到的峰值
static volatile int g_cbInFlight = 0;
static volatile int g_cbMaxInFlight = 0;
static int g_cbHoldMs = 0;                   // 回调里占住这么久，制造并发窗口

static void WINAPI FakeEplSubWrap(int nHandle);
static void WINAPI FakeEplSub(int nHandle)
{
    const int nNow = (int)InterlockedIncrement((volatile long*)&g_cbInFlight);
    for (;;)                                        // 峰值用 CAS 更新，别让竞态把重入漏成 1
    {
        const int nOld = g_cbMaxInFlight;
        if (nOld >= nNow || InterlockedCompareExchange((volatile long*)&g_cbMaxInFlight, nNow, nOld) == nOld) break;
    }
    if (g_cbHoldMs > 0) Sleep(g_cbHoldMs);
    FakeEplSubWrap(nHandle);
    InterlockedIncrement((LPLONG)&g_cbDone);
    InterlockedDecrement((volatile long*)&g_cbInFlight);
}

static void WINAPI FakeEplSubWrap(int nHandle)
{
    InterlockedIncrement((LPLONG)&g_cbCount);
    g_cbLastType = -1;
    g_cbRestoreOk = -1;
    int userBlock[BM_COUNT] = { 0 };
    void* pUserBlock = userBlock;
    int shapeBlock[BM_COUNT] = { 0 };

    Args ar;
    ar.AddInt(nHandle);
    ar.AddCompound(&pUserBlock);
    MDATA_INF r = CallCmd("还原事件信息", &ar);
    g_cbRestoreOk = (r.m_dtDataType == SDT_BOOL && r.m_bool != BL_FALSE) ? 1 : 0;
    if (!g_cbRestoreOk) return;

    int* m = userBlock;
    g_cbLastType = m[BM_TYPE];
    if (m[BM_TYPE] == 3) InterlockedExchange((LPLONG)&g_cbSawDisconnect, 1);   // 断开事件专用：别的线程可能晚到，不能只看 lastType
    if (m[BM_TYPE] == 5) InterlockedExchange((LPLONG)&g_cbSawAllGone, 1);
    g_cbSeq = m[BM_SEQ];
    strcpy(g_cbArrive, m[BM_ARRIVE] ? (char*)m[BM_ARRIVE] : "");
    strcpy(g_cbUrl, m[BM_URL] ? (char*)m[BM_URL] : "");
    strcpy(g_cbTitle, m[BM_TITLE] ? (char*)m[BM_TITLE] : "");
    g_cbPageType = m[BM_PAGETYPE];
    g_cbColMode = m[BM_COLMODE];
    strcpy(g_cbPlatform, m[BM_PLATFORM] ? (char*)m[BM_PLATFORM] : "");
    strcpy(g_cbSite, m[BM_SITE] ? (char*)m[BM_SITE] : "");
    strcpy(g_cbAsin, m[BM_ASIN] ? (char*)m[BM_ASIN] : "");
    g_cbStart = m[BM_START];
    strcpy(g_cbExtVer, m[BM_EXT_VER] ? (char*)m[BM_EXT_VER] : "");
    strcpy(g_cbBrowser, m[BM_BROWSER] ? (char*)m[BM_BROWSER] : "");
    strcpy(g_cbExtId, m[BM_EXT_ID] ? (char*)m[BM_EXT_ID] : "");
    strcpy(g_cbRaw, m[BM_RAW] ? (char*)m[BM_RAW] : "");
    strcpy(g_cbTaskName, m[BM_TASKNAME] ? (char*)m[BM_TASKNAME] : "");
    g_cbBrowserCount = m[BM_BROWSER_COUNT];
    EvLogPush(m[BM_TYPE], m[BM_BROWSER_COUNT],
        m[BM_EXT_ID] ? (char*)m[BM_EXT_ID] : "",
        m[BM_BROWSER] ? (char*)m[BM_BROWSER] : "");

    if (m[BM_TYPE] == 1)
    {
        // 当场拿一个没用过的句柄号试一次：必须返回假且不能崩
        Args ast;
        ast.AddInt(nHandle + 800000);
        ast.AddCompound(&pUserBlock);
        MDATA_INF rs = CallCmd("还原事件信息", &ast);
        g_cbStaleOk = (rs.m_dtDataType == SDT_BOOL && rs.m_bool != BL_FALSE) ? 1 : 0;

        // C：变量槽后面不足 84 字节（真·4 字节槽）且槽里是空指针：库要回填一块并还原成功
        if (TailSlotEnsure())
        {
            void* pEmptyBlock = NULL;
            void* pEmptySlot = g_cbHoldMs > 0 ? (void*)&pEmptyBlock : g_pTailSlot;
            *(void**)pEmptySlot = NULL;
            Args aC;
            aC.AddInt(nHandle);
            aC.AddCompound(pEmptySlot);
            MDATA_INF rC = CallCmd("还原事件信息", &aC);
            void* pBack = *(void**)pEmptySlot;
            g_cbShapeC = (rC.m_dtDataType == SDT_BOOL && rC.m_bool != BL_FALSE && pBack != NULL) ? 1 : 0;
            if (g_cbShapeC)
            {
                int* pC = (int*)pBack;
                if (pC[BM_ASIN]) strcpy(g_cbShapeCAsin, (char*)pC[BM_ASIN]);
                if (g_ownershipProbe)
                {
                    // 两个变量的文本必须各自分配；改写一个变量不能改变事件源数据。
                    g_cbIndependentText = pC[BM_URL] != m[BM_URL] && pC[BM_ASIN] != m[BM_ASIN];
                    if (m[BM_URL]) ((char*)(INT_PTR)m[BM_URL])[0] = 'X';
                    MDATA_INF again = CallCmd("还原事件信息", &ar);
                    g_cbRepeatedRestore = again.m_dtDataType == SDT_BOOL && again.m_bool != BL_FALSE &&
                        m[BM_URL] != 0 && strcmp((char*)(INT_PTR)m[BM_URL], (char*)(INT_PTR)pC[BM_URL]) == 0;
                    g_retainedBlock = pC;
                }
                else
                {
                    FreeEventMembers(pC);
                    FakeNotifySys(NRS_MFREE, (DWORD)(INT_PTR)pC, 0);
                }
                *(void**)pEmptySlot = NULL;
            }
        }

        // B：传进来的地址本身就是数据块（一次解栈）—— 只有首成员正好是事件类型取值时才认
        memset(shapeBlock, 0, sizeof(shapeBlock));
        shapeBlock[BM_TYPE] = 1;
        {
            Args aB;
            aB.AddInt(nHandle);
            aB.AddCompound(shapeBlock);
            MDATA_INF rB = CallCmd("还原事件信息", &aB);
            g_cbShapeB = (rB.m_dtDataType == SDT_BOOL && rB.m_bool != BL_FALSE) ? 1 : 0;
            if (g_cbShapeB && shapeBlock[BM_ASIN]) strcpy(g_cbShapeBAsin, (char*)shapeBlock[BM_ASIN]);
            FreeEventMembers(shapeBlock);
        }

        // N：变量还没被易语言分配过 → 传进来的就是空白数据（地址 0）。必须返回假，且报错要告诉他
        // 先给某个成员赋一次值；整句还得是一种编码（以前半句 GBK 半句 UTF-8，客户看到的是乱码，
        // 而且自写的格式化器不认 %p，打出来是死字，等于没有线索）
        {
            Args aN;
            aN.AddInt(nHandle);
            aN.AddCompound(NULL);
            MDATA_INF rN = CallCmd("还原事件信息", &aN);
            strncpy(g_cbShapeNMsg, CmdText("取对接错误"), sizeof(g_cbShapeNMsg) - 1);
            g_cbShapeNMsg[sizeof(g_cbShapeNMsg) - 1] = '\0';
            g_cbShapeN = (rN.m_dtDataType == SDT_BOOL && rN.m_bool == BL_FALSE &&
                          g_cbShapeNMsg[0] != '\0' && strstr(g_cbShapeNMsg, "%") == NULL &&
                          strstr(g_cbShapeNMsg, "99") != NULL &&
                          strstr(g_cbShapeNMsg, "nArg=2") != NULL &&
                          strstr(g_cbShapeNMsg, "szof=") != NULL &&
                          strstr(g_cbShapeNMsg, "raw=[") != NULL &&
                          strstr(g_cbShapeNMsg, "blk=[") != NULL &&
                          Utf8Valid(g_cbShapeNMsg)) ? 1 : 0;
        }

        Args aa;
        aa.AddInt(nHandle);
        aa.AddBool(g_cbMode == 1 ? 0 : 1);
        aa.AddText(g_cbTaskId);
        aa.AddText(g_cbMode == 1 ? "REFUSED_BY_APP" : "DUP_TASK");
        aa.AddBool(1);
        aa.AddInt(5);
        aa.AddBool(g_cbMode == 1 ? 0 : 1);
        MDATA_INF an = CallCmd("应答采集事件", &aa);
        g_cbAnswerOk = (an.m_dtDataType == SDT_BOOL && an.m_bool != BL_FALSE) ? 1 : 0;
    }
    if (m[BM_TYPE] == 4)
    {
        // 撤回事件：抄下任务号并应答「是否撤回成功」，回包要按这个来
        strcpy(g_cbRecallId, m[BM_TASKID] ? (char*)m[BM_TASKID] : "");
        g_cbLastType = 4;
        Args ar4;
        ar4.AddInt(nHandle);
        ar4.AddBool(g_cbRecallAccept ? 1 : 0);
        ar4.AddText(g_cbRecallId);
        ar4.AddText(g_cbRecallAccept ? "RECALLED 已撤回" : "这条已经采完了，撤不回");
        ar4.AddBool(0);
        ar4.AddInt(3);            // 队列剩余 3 个 —— 要原样出现在回执里
        ar4.AddBool(1);           // 撤回时这条已经在采
        MDATA_INF r4 = CallCmd("应答采集事件", &ar4);
        g_cbRecallAnswered = (r4.m_dtDataType == SDT_BOOL && r4.m_bool != BL_FALSE) ? 1 : 0;
    }
    if (g_servePrint)   // 两端联调（serve）时把收到的关键字段打出来，看插件到底传了什么
    {
        printf("[event] type=%d cnt=%d browser=%s asin=%s id=%s task=%s url=%.70s\n",
            g_cbLastType, g_cbBrowserCount, g_cbBrowser, g_cbAsin, g_cbRecallId, g_cbTaskName, g_cbUrl);
        fflush(stdout);
    }
    FreeEventMembers(userBlock);
}

// 模拟界面时钟：另起线程周期调用 处理插件消息
static DWORD WINAPI PumpThread(LPVOID)
{
    for (;;)
    {
        Sleep(80);
        if (g_pumpOn)
        {
            Args a;
            a.AddInt(0);
            CallCmd("处理插件消息", &a);
        }
    }
    return 0;
}

static bool WaitCallback(int expectCount, int ms)
{
    for (int i = 0; i < ms / 30; ++i)
    {
        if (g_cbDone >= expectCount) return true;
        Sleep(30);
    }
    return g_cbDone >= expectCount;
}

// 等事件彻底派发干净（手动调泵区用）。nBase 必须在「没有在飞的回调 + 队列已空」的时刻取：
// g_cbCount 是回调进入计数、g_cbDone 是返回计数，抢在中间取就会把上一条事件的尾巴算成下一条，
// 读到的字段是旧事件的。多浏览器一段比原来多出好几条上线/断开事件，不排干净就会误判。
static void Settle(int ms)
{
    for (int i = 0; i < ms / 50; ++i)
    {
        if (g_cbDone == g_cbCount && CmdInt("取待处理消息数") == 0) return;
        Sleep(50);
    }
}

static SOCKET g_occupy[8];
static int g_nOccupy = 0;

// 原子性验证：六个线程各投一条事件（自动派发模式下就在自己线程里跑回调）
static int g_nPostOk = 0, g_nPostZero = 0;
static DWORD WINAPI PostOneEvent(LPVOID p)
{
    UNREFERENCED_PARAMETER(p);
    Args a;
    a.AddText("https://www.amazon.com/dp/B0ATOMIC01");
    a.AddBool(1);
    const int nSeq = CmdInt("模拟插件事件", &a);
    InterlockedIncrement((LPLONG)&g_nPostOk);
    if (nSeq == 0) InterlockedIncrement((LPLONG)&g_nPostZero);
    return 0;
}
static void PostSixEvents()
{
    HANDLE hTh[6];
    g_cbMaxInFlight = 0;
    g_cbInFlight = 0;
    g_nPostOk = g_nPostZero = 0;
    for (int i = 0; i < 6; i++) hTh[i] = CreateThread(NULL, 0, PostOneEvent, NULL, 0, NULL);
    WaitForMultipleObjects(6, hTh, TRUE, 20000);
    for (int i = 0; i < 6; i++) CloseHandle(hTh[i]);
    printf("       [probe] 投递返回非零 %d / 返回 0 %d / 回调累计 %d / 峰值并发 %d / 待处理 %d / 回调模式 %d / 服务 %d\n",
        g_nPostOk, g_nPostZero, g_cbDone, g_cbMaxInFlight, CmdInt("取待处理消息数"),
        CmdInt("取回调模式"), CmdBool("服务是否运行") ? 1 : 0);
}

// 端口独占验证：Windows 上带 SO_REUSEADDR 的 bind 能抢走已监听端口的连接，
// 库不设这个选项之后必须抢不动，否则两个采集程序会互相偷请求（点按钮没反应就是这么来的）
static bool TryBindReuse(int port)
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return false;
    const int on = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port = htons((u_short)port);
    a.sin_addr.s_addr = inet_addr("127.0.0.1");
    const bool bOk = (bind(s, (struct sockaddr*)&a, sizeof(a)) == 0);
    closesocket(s);
    return bOk;
}
static void OccupyPort(int port)
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port = htons((u_short)port);
    a.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(s, (struct sockaddr*)&a, sizeof(a)) == 0 && listen(s, 1) == 0) g_occupy[g_nOccupy++] = s;
    else closesocket(s);
}
static void ReleasePorts()
{
    for (int i = 0; i < g_nOccupy; ++i) closesocket(g_occupy[i]);
    g_nOccupy = 0;
}

int main(int argc, char** argv)
{
    const char* fne = (argc > 1) ? argv[1] : "out\\cxbrow.fne";
    // 常驻服务模式：host_bridge.exe out\cxbrow.fne serve [秒]
    // 给插件侧的 ac-client.js 当对端用，两端联调时不用起易语言。
    if (argc > 2 && strcmp(argv[2], "serve") == 0)
    {
        HMODULE hs = LoadLibraryA(fne);
        if (hs == NULL) { printf("[FAIL] 装载失败\n"); return 1; }
        typedef PLIB_INFO(WINAPI* PFN_GET2)();
        g_info = ((PFN_GET2)GetProcAddress(hs, "GetNewInf"))();
        g_info->m_pfnNotify(NL_SYS_NOTIFY_FUNCTION, (DWORD)(INT_PTR)FakeNotifySys, 0);
        char dir[MAX_PATH];
        GetCurrentDirectoryA(sizeof(dir), dir);
        Args ai2;
        ai2.AddText("serve");
        ai2.AddText(dir);
        ai2.AddText("创想亚马逊ASIN采集软件");
        CallCmd("初始化对接", &ai2);
        Args ab2;
        ab2.AddSub((void*)FakeEplSub);
        CallCmd("绑定事件子程序", &ab2);
        CallCmd("启动对接服务", NULL);
        g_port = CmdInt("取监听端口");
        g_servePrint = 1;
        printf("serve on %d\n", g_port);
        fflush(stdout);
        const int nSec = (argc > 3) ? atoi(argv[3]) : 60;
        for (int i = 0; i < nSec * 10; ++i)
        {
            Sleep(100);
            Args a3;
            a3.AddInt(0);
            CallCmd("处理插件消息", &a3);
            if (g_cbCount > 0 && i > 40)
            {
                // 收到过采集请求就再多留一会儿，便于看回包
                Sleep(200);
            }
        }
        CallCmd("停止对接服务", NULL);
        printf("events=%d tasks=%d\n", g_cbCount, CmdInt("取累计采集请求数"));
        return 0;
    }
    printf("== 装载 %s ==\n", fne);
    HMODULE h = LoadLibraryA(fne);
    if (h == NULL) { printf("[FAIL] 装载失败 %u\n", GetLastError()); return 1; }
    typedef PLIB_INFO(WINAPI* PFN_GET)();
    PFN_GET pGet = (PFN_GET)GetProcAddress(h, "GetNewInf");
    if (pGet == NULL) { printf("[FAIL] 找不到 GetNewInf\n"); return 1; }
    g_info = pGet();
    if (g_info == NULL) { printf("[FAIL] GetNewInf 返回空\n"); return 1; }
    g_info->m_pfnNotify(NL_SYS_NOTIFY_FUNCTION, (DWORD)(INT_PTR)FakeNotifySys, 0);

    // ---------------- 1 表自洽 ----------------
    printf("-- 库表 --\n");
    Check(g_info->m_nCmdCount == 32, "命令条数 32", "");
    Check(g_info->m_pBeginCmdInfo != NULL && g_info->m_pCmdsFunc != NULL, "命令表与实现表都已登记", "");
    bool bAllFunc = true, bAllName = true;
    for (int i = 0; i < g_info->m_nCmdCount; ++i)
    {
        if (g_info->m_pCmdsFunc[i] == NULL) bAllFunc = false;
        if (g_info->m_pBeginCmdInfo[i].m_szName == NULL) bAllName = false;
        for (int j = i + 1; j < g_info->m_nCmdCount; ++j)
        {
            if (strcmp((const char*)g_info->m_pBeginCmdInfo[i].m_szName,
                (const char*)g_info->m_pBeginCmdInfo[j].m_szName) == 0)
            {
                printf("[FAIL] 命令重名：%s\n", (const char*)g_info->m_pBeginCmdInfo[i].m_szName);
                g_failed++;
            }
        }
        const int nArg = g_info->m_pBeginCmdInfo[i].m_nArgCount;
        if (nArg > 0 && g_info->m_pBeginCmdInfo[i].m_pBeginArgInfo == NULL)
        {
            printf("[FAIL] %s 声明了 %d 个参数但没有参数表\n",
                (const char*)g_info->m_pBeginCmdInfo[i].m_szName, nArg);
            g_failed++;
        }
        if (g_info->m_pBeginCmdInfo[i].m_szExplain == NULL ||
            g_info->m_pBeginCmdInfo[i].m_szExplain[0] == '\0')
        {
            printf("[FAIL] %s 没有命令说明\n", (const char*)g_info->m_pBeginCmdInfo[i].m_szName);
            g_failed++;
        }
        for (int k = 0; k < nArg; ++k)
        {
            const ARG_INFO* pA = &g_info->m_pBeginCmdInfo[i].m_pBeginArgInfo[k];
            if (pA->m_szName == NULL || pA->m_szName[0] == '\0' ||
                pA->m_szExplain == NULL || pA->m_szExplain[0] == '\0')
            {
                printf("[FAIL] %s 第 %d 个参数缺名字或说明\n",
                    (const char*)g_info->m_pBeginCmdInfo[i].m_szName, k + 1);
                g_failed++;
            }
        }
    }
    Check(bAllFunc, "实现表每条都是有效指针", "");
    Check(bAllName, "命令名全部非空", "");
    Check(g_info->m_nDataTypeCount == 1, "登记了 1 个自定义类型", "");
    if (g_info->m_nDataTypeCount >= 1)
    {
        const PLIB_DATA_TYPE_INFO pT = &g_info->m_pDataType[0];
        Check(pT->m_nElementCount == BM_COUNT && BM_COUNT == 21, "浏览器事件信息 21 个入参成员", "");
        Check(pT->m_nCmdCount == 0, "它是纯数据类型，没有成员命令", "");
        bool bExpl = true;
        for (int i = 0; i < pT->m_nElementCount; ++i)
        {
            const LIB_DATA_TYPE_ELEMENT* pE = &pT->m_pElementBegin[i];
            if (pE->m_szName == NULL || pE->m_szExplain == NULL || pE->m_szExplain[0] == '\0') bExpl = false;
            if (pE->m_pArySpec != NULL) bExpl = false;
        }
        Check(bExpl, "每个成员都有中文名与说明，且没有数组成员", "");
    }
    Check(g_info->m_nLibConstCount == 15, "常量 15 条", "");
    {
        // 类别表：每条必须是「4 位序号 + 名称」再接 \0。写成 "…\0002名称" 会被 C 当成八进制
        // 转义吞掉 00，IDE 里第二三类就变乱码，所以逐条核一遍前四位是不是数字。
        const char* pCat = (const char*)g_info->m_szzCategory;
        int nCat = 0;
        bool bLayoutOk = true;
        while (pCat != NULL && *pCat != '\0' && nCat < 16)
        {
            const int nLen = (int)strlen(pCat);
            ++nCat;
            if (nLen < 6) { bLayoutOk = false; break; }
            for (int i = 0; i < 4; ++i)
            {
                if (pCat[i] < '0' || pCat[i] > '9') bLayoutOk = false;
            }
            if (pCat[3] != (char)('0' + nCat)) bLayoutOk = false;
            pCat += nLen + 1;
        }
        Check(g_info->m_nCategoryCount == 3 && nCat == 3, "登记了 3 个类别", "");
        Check(bLayoutOk, "类别名都是「4 位序号 + 名称」且序号连续", "");
    }

    // ---------------- 2 启动 ----------------
    printf("-- 启停 --\n");
    Check(CmdBool("服务是否运行") == false, "没启动时服务不在运行", "");
    Check(CmdInt("取连接状态") == 0, "没启动时连接状态是 0", "");
    Check(CmdInt("取监听端口") == 0, "没启动时端口是 0", "");
    Check(CmdInt("取待处理消息数") == 0, "没启动时队列是空", "");

    char runDir[MAX_PATH];
    GetCurrentDirectoryA(sizeof(runDir), runDir);
    Args ai;
    ai.AddText("20260022");
    ai.AddText(runDir);
    ai.AddText("AmazonAsinCollector");
    ai.AddText("cxac");
    // 测试专用端口段：真实插件只扫 47630-47633，采集软件也可能正在那里监听，
    // 混用会让「无请求超时判断开」这类断言被外来请求打断
    ai.AddInt(47690);
    ai.AddInt(4);
    Check(CmdBool("初始化对接", &ai), "初始化对接成功", "");
    const char* pszLog = CmdText("取对接日志路径");
    Check(strstr(pszLog, runDir) != NULL && strstr(pszLog, "log") != NULL, "日志路径落在运行目录", pszLog);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // 先绑好接收子程序，后面的连接事件才看得到
    Args ab0;
    ab0.AddSub((void*)FakeEplSub);
    Check(CmdBool("绑定事件子程序", &ab0), "绑定事件子程序成功", "");
    CreateThread(NULL, 0, PumpThread, NULL, 0, NULL);

    // 后面大量断言依赖「程序自己调泵」的语义，先显式切到模式 1
    Args amode;
    amode.AddInt(1);
    Check(CmdBool("置回调线程", &amode), "切到手动调泵模式", "");
    Check(CmdInt("取回调模式") == 1, "取回调模式回 1", "");
    Args amode0;
    amode0.AddInt(7);
    Check(CmdBool("置回调线程", &amode0) == false, "模式值非法直接拒收", "");

    Check(CmdBool("启动对接服务"), "启动对接服务成功", "");
    g_port = CmdInt("取监听端口");
    Check(g_port >= 47690 && g_port <= 47693, "端口在候选段内", "");
    Check(CmdBool("服务是否运行"), "服务已在监听", "");
    Check(TryBindReuse(g_port) == false, "已监听的端口别人抢不走", "");
    Check(TryBindReuse(g_port + 4) == true, "（对照）空闲端口能绑上", "");

    // ---------------- 3 handshake ----------------
    printf("-- 握手 --\n");
    Resp r;
    Check(HttpRaw("GET", "handshake", NULL, "", &r), "handshake 有回包", "");
    Check(r.status == 200 && JsonHas(r.body, "\"ok\":true"), "handshake 返回 ok", r.body);
    Check(JsonValid(r.body), "handshake 回包是合法 JSON", r.body);
    Check(Utf8Valid(r.body), "handshake 回包是合法 UTF-8（程序名不乱码）", r.body);
    if (getenv("CXBROW_DEBUG") != NULL) { printf("    [body] %s\n", r.body); fflush(stdout); }
    char val[512];
    JsonVal(r.body, "service", val, sizeof(val));
    Check(strcmp(val, "cxac") == 0, "服务标识是 cxac", val);
    JsonVal(r.body, "bridge", val, sizeof(val));
    Check(strcmp(val, "1") == 0, "桥协议版本 1", val);
    JsonVal(r.body, "version", val, sizeof(val));
    Check(strcmp(val, "20260022") == 0, "握手带回程序版本", val);
    JsonVal(r.body, "app", val, sizeof(val));
    Check(strcmp(val, "AmazonAsinCollector") == 0, "握手带回程序名称", val);
    JsonVal(r.body, "key", val, sizeof(val));
    Check(strlen(val) == 32, "会话标识 32 位", val);
    strcpy(g_key, val);
    JsonVal(r.body, "port", val, sizeof(val));
    Check(atoi(val) == g_port, "握手回的是实际端口", val);
    Check(CmdInt("取连接状态") == 2, "握手后连接状态是 2", "");
    Check(WaitCallback(1, 3000), "已派发一条连接事件", "");
    Check(g_cbLastType == 2, "事件类型是 2 插件已连接", "");
    Check(strcmp(g_cbExtVer, EXT_VER) == 0, "连接事件带回插件版本", g_cbExtVer);
    Check(strcmp(g_cbBrowser, EXT_BROWSER) == 0, "连接事件带回浏览器", g_cbBrowser);
    Check(strcmp(g_cbExtId, EXT_ID) == 0, "连接事件带回插件编号", g_cbExtId);
    Check(strcmp(CmdText("取插件版本"), EXT_VER) == 0, "取插件版本命令口径一致", "");
    Check(strcmp(CmdText("取插件浏览器"), EXT_BROWSER) == 0, "取插件浏览器命令口径一致", "");
    Check(CmdInt("取累计连接次数") >= 1, "累计连接次数在涨", "");

    // ---------------- 4 会话标识校验 ----------------
    printf("-- 会话标识 --\n");
    Resp r2;
    Check(HttpRaw("GET", "sites", NULL, "wrong-key", &r2) && r2.status == 403, "错会话标识被拒 403", r2.body);
    Check(JsonHas(r2.body, "key_required"), "拒因是 key_required", r2.body);
    Check(JsonValid(r2.body), "错误回包也是合法 JSON", r2.body);
    Check(HttpRaw("GET", "sites", NULL, "", &r2) && r2.status == 403, "不带会话标识同样被拒", r2.body);
    HttpRaw("GET", "handshake", NULL, "", &r);
    JsonVal(r.body, "key", val, sizeof(val));
    Check(strcmp(val, g_key) == 0, "同一次监听里会话标识不变", val);

    // ---------------- 5 站点名单 ----------------
    printf("-- 站点 --\n");
    Check(HttpRaw("GET", "sites", NULL, NULL, &r) && r.status == 200, "sites 返回 200", "");
    Check(JsonValid(r.body), "sites 回包是合法 JSON", r.body);
    Check(Utf8Valid(r.body), "sites 回包是合法 UTF-8（站点名不乱码）", "");
    JsonVal(r.body, "siteCount", val, sizeof(val));
    Check(atoi(val) == 21, "内置 21 个亚马逊站点", val);
    Check(JsonHas(r.body, "AmazonUs") && JsonHas(r.body, "amazon.com") && JsonHas(r.body, "AmazonBe"),
        "站点含美国站与比利时站", "");
    Check(JsonHas(r.body, "\"pageTypes\":[{\"type\":1") && JsonHas(r.body, "\"type\":3"),
        "每个站点都带列表/详情页面类型", "");
    Check(JsonHas(r.body, "\"homeUrl\":\"https://www.amazon.de/\""), "首页地址拼法正确", "");
    Check(JsonHas(r.body, U8_MEI_GUO_ZHAN), "站点中文名以 UTF-8 输出", "");

    // ---------------- 6 网址识别 ----------------
    printf("-- 识别 --\n");
    Check(HttpRaw("GET", "match?url=https%3A%2F%2Fwww.amazon.com%2Fdp%2FB0ABCDEF12", NULL, NULL, &r),
        "match 有回包", "");
    Check(JsonValid(r.body), "match 回包是合法 JSON", r.body);
    Check(JsonHas(r.body, "\"identifiable\":true"), "详情页可识别", r.body);
    JsonVal(r.body, "asin", val, sizeof(val));
    Check(strcmp(val, "B0ABCDEF12") == 0, "详情页取到 ASIN", val);
    JsonVal(r.body, "pageType", val, sizeof(val));
    Check(strcmp(val, "3") == 0, "详情页页面类型是 3", val);
    Check(JsonVal(r.body, "colModType", val, sizeof(val)) && strcmp(val, "Detail") == 0,
        "详情页采集模式是 Detail", val);
    Check(JsonVal(r.body, "displayName", val, sizeof(val)) && strcmp(val, U8_MEI_GUO_ZHAN) == 0,
        "识别结果带回中文站名", val);
    Check(JsonHas(r.body, "\"universal\":false") && JsonHas(r.body, "\"auth\":true"), "带齐插件用的其它标记", "");

    HttpRaw("GET", "match?url=https%3A%2F%2Fwww.amazon.co.jp%2Fs%3Fk%3Dphone", NULL, NULL, &r);
    Check(JsonHas(r.body, "\"identifiable\":true") && JsonHas(r.body, "\"platformName\":\"AmazonJp\""),
        "日本站搜索页可识别", r.body);
    JsonVal(r.body, "pageType", val, sizeof(val));
    Check(strcmp(val, "1") == 0, "搜索页页面类型是 1", val);

    HttpRaw("GET", "match?url=https%3A%2F%2Fwww.amazon.com%2Fgp%2Faw%2Fd%2FB0GP product", NULL, NULL, &r);
    HttpRaw("GET", "match?url=https%3A%2F%2Fwww.amazon.com%2Fstores%2F123", NULL, NULL, &r);
    Check(JsonHas(r.body, "\"pageType\":2") && JsonHas(r.body, "\"colModType\":\"List\""),
        "店铺页判成 2 且按列表采", r.body);

    HttpRaw("GET", "match?url=https%3A%2F%2Fwww.ebay.com%2Fitm%2F123", NULL, NULL, &r);
    Check(JsonHas(r.body, "\"identifiable\":false"), "非名单站点判成不可识别", r.body);
    HttpRaw("GET", "match?url=https%3A%2F%2Famazon.com.evil.net%2Fdp%2FB0ABCDEF12", NULL, NULL, &r);
    Check(JsonHas(r.body, "\"identifiable\":false"), "伪装域名不会被误认", r.body);
    HttpRaw("GET", "match?url=https%3A%2F%2Fwww.amazon.com%2F", NULL, NULL, &r);
    Check(JsonHas(r.body, "\"identifiable\":false"), "站点首页判成不可识别", r.body);
    HttpRaw("GET", "match", NULL, NULL, &r);
    Check(r.status == 400 && JsonHas(r.body, "url_invalid"), "缺网址时回 400", r.body);

    Args am;
    am.AddText("https://www.amazon.de/gp/product/A123456789");
    const char* pszM = CmdText("取网址识别", &am);
    Check(strstr(pszM, "AmazonDe") != NULL && strstr(pszM, U8_DE_GUO_ZHAN) != NULL,
        "取网址识别命令口径一致", pszM);
    Args am2;
    am2.AddText("https://www.amazon.com/dp/B0ABCDEF12");
    Check(strstr(CmdText("取网址识别", &am2), "\"identifiable\":true") != NULL,
        "识别命令的布尔口径是 true", "");

    // ---------------- 7 没绑定子程序时的采集请求 ----------------
    printf("-- 未绑定 --\n");
    Args a0;
    a0.AddSub(NULL);
    Check(CmdBool("绑定事件子程序", &a0) == false, "传 0 解除绑定", "");
    Check(HttpRaw("POST", "task", "{\"url\":\"https://www.amazon.com/dp/B0ABCDEF12\",\"start\":true}", NULL, &r)
        && r.status == 503, "没绑定子程序时回 503", r.body);
    Check(JsonHas(r.body, "no_binding"), "拒因是 no_binding", r.body);
    Args ab;
    ab.AddSub((void*)FakeEplSub);
    Check(CmdBool("绑定事件子程序", &ab), "重新绑定成功", "");

    // ---------------- 8 采集请求 ----------------
    printf("-- 采集请求 --\n");
    int nBase = g_cbCount;
    Check(HttpRaw("POST", "task",
        "{\"url\":\"https://www.amazon.com/dp/B0ABCDEF12?ref=abc\",\"start\":true,\"title\":\"Widget Case\",\"taskName\":\"Oct-pick\"}",
        NULL, &r) && r.status == 200, "task 返回 200", r.body);
    Check(JsonValid(r.body), "task 应答是合法 JSON", r.body);
    Check(Utf8Valid(r.body), "task 应答是合法 UTF-8（提示消息不乱码）", r.body);
    Check(WaitCallback(nBase + 1, 5000), "已派发采集事件", "");
    Check(g_cbLastType == 1, "事件类型是 1 收到采集请求", "");
    Check(strcmp(g_cbUrl, "https://www.amazon.com/dp/B0ABCDEF12?ref=abc") == 0, "事件带回完整网址", g_cbUrl);
    Check(strcmp(g_cbAsin, "B0ABCDEF12") == 0, "事件带回 ASIN", g_cbAsin);
    Check(strcmp(g_cbTitle, "Widget Case") == 0, "事件带回页面标题", g_cbTitle);
    Check(strcmp(g_cbTaskName, "Oct-pick") == 0, "事件带回插件提交任务时设的任务名", g_cbTaskName);
    Check(strcmp(g_cbPlatform, "AmazonUs") == 0, "事件带回平台名", g_cbPlatform);
    Check(strcmp(g_cbSite, U8_MEI_GUO_ZHAN) == 0, "事件带回站点显示名", g_cbSite);
    Check(strcmp(g_cbExtVer, EXT_VER) == 0, "事件带回插件版本", g_cbExtVer);
    Check(g_cbPageType == 3 && g_cbColMode == 2, "事件带列表/详情口径", "");
    Check(g_cbStart == BL_TRUE, "事件带「开始采集」为真", "");
    Check(strstr(g_cbRaw, "ref=abc") != NULL, "事件带原始请求内容", g_cbRaw);
    Check(strlen(g_cbArrive) >= 19, "事件带到达时间", g_cbArrive);
    Check(g_cbSeq > 0, "事件带流水号", "");
    Check(JsonHas(r.body, "\"ok\":true"), "应答是成功", r.body);
    JsonVal(r.body, "taskId", val, sizeof(val));
    Check(strcmp(val, "T-77") == 0, "任务编号用的是采集程序填的值", val);
    Check(JsonHas(r.body, "\"started\":true"), "已开始采集回给插件", r.body);
    Check(JsonHas(r.body, "\"state\":\"queued\""), "状态是 queued", r.body);
    Check(JsonHas(r.body, "\"alreadyQueued\":true"), "已在队列标记带回", r.body);
    Check(JsonHas(r.body, "\"queueCount\":5"), "队列数量带回", r.body);
    Check(strstr(r.body, "DUP_TASK") != NULL, "提示消息原样带回", r.body);
    Check(strstr(r.body, "Widget Case") != NULL, "商品名回给插件显示", r.body);
    Check(g_cbRestoreOk == 1, "还原事件信息 在回调里取到了数据", "");
    Check(g_cbAnswerOk == 1, "应答采集事件 在回调里写成功", "");
    Check(g_cbStaleOk == 0, "回调里用别的句柄号还原被拒", "");
    {
        // 回调已经返回，句柄必须立刻作废且不崩
        Args astale;
        astale.AddInt(999999);
        astale.AddCompound(&g_pUserBlock);
        Check(CmdBool("还原事件信息", &astale) == false, "回调结束后句柄立刻作废", "");
        Args aans;
        aans.AddInt(999999);
        aans.AddBool(1);
        Check(CmdBool("应答采集事件", &aans) == false, "过期句柄写应答被拒", "");
    }
    Check(CmdInt("取累计采集请求数") >= 2, "累计采集请求数在涨", "");
    Check(CmdInt("取累计请求数") >= 12, "累计对接请求数在涨", "");
    Check(strcmp(CmdText("取插件编号"), EXT_ID) == 0, "取插件编号命令口径一致", "");

    // 模拟插件事件：不靠浏览器也能验「入队 → 调泵 → 回调 → 应答」这条链
    nBase = g_cbCount;
    Args asim;
    asim.AddText("https://www.amazon.com/dp/B0SIMULATE");
    asim.AddBool(1);
    g_ownershipProbe = 1;
    Check(CmdInt("模拟插件事件", &asim) > 0, "模拟插件事件 已入队", "");
    Check(WaitCallback(nBase + 1, 5000), "模拟事件也走到了接收子程序", "");
    g_ownershipProbe = 0;
    Check(g_cbLastType == 1 && strcmp(g_cbAsin, "B0SIMULATE") == 0, "模拟事件字段与真实请求同形", g_cbAsin);
    Check(g_cbIndependentText == 1, "多个还原变量各自持有独立文本", "");
    Check(g_cbRepeatedRestore == 1, "变量修改后重复还原仍读取原事件并释放旧成员", "");
    Check(g_retainedBlock != NULL && g_retainedBlock[BM_ASIN] != 0 &&
        strcmp((char*)(INT_PTR)g_retainedBlock[BM_ASIN], "B0SIMULATE") == 0,
        "回调返回和事件销毁后还原变量仍持有有效文本", "");
    if (g_retainedBlock != NULL)
    {
        FreeEventMembers(g_retainedBlock);
        FakeNotifySys(NRS_MFREE, (DWORD)(INT_PTR)g_retainedBlock, 0);
        g_retainedBlock = NULL;
    }
    Check(g_cbRestoreOk == 1 && g_cbAnswerOk == 1, "模拟事件同样能还原与应答", "");
    Check(g_cbShapeC == 1 && strcmp(g_cbShapeCAsin, "B0SIMULATE") == 0,
        "槽只有 4 字节且为空时库回填并还原成功", g_cbShapeCAsin);
    Check(g_cbShapeB == 1 && strcmp(g_cbShapeBAsin, "B0SIMULATE") == 0,
        "地址本身就是刚分配出来的数据块时也能还原成功", g_cbShapeBAsin);
    Check(g_cbShapeN == 1, "变量没分配时报假并告诉他先给成员赋值（且是一句能读的 UTF-8）", g_cbShapeNMsg);
    Args asim2;
    asim2.AddText("https://www.ebay.com/itm/1");
    asim2.AddBool(1);
    Check(CmdInt("模拟插件事件", &asim2) == 0, "认不出的网址模拟返回 0", "");
    {
        // 模拟口也支持指定任务名（第三个参数），不装浏览器就能验这条字段
        nBase = g_cbCount;
        Args asim3;
        asim3.AddText("https://www.amazon.com/dp/B0NAMETEST");
        asim3.AddBool(1);
        asim3.AddText("named-sim-task");     // 用 ASCII：宿主里的中文字面量不是 UTF-8，断言会假通过
        Check(CmdInt("模拟插件事件", &asim3) > 0, "模拟插件事件 带任务名 已入队", "");
        Check(WaitCallback(nBase + 1, 5000), "带任务名的模拟事件也走到了接收子程序", "");
        Check(strcmp(g_cbTaskName, "named-sim-task") == 0, "模拟事件的任务名原样交给接收子程序", g_cbTaskName);
    }

    // ===== 事件原子性：默认串行派发，接收子程序不得被两个线程同时进入 =====
    // 自动派发模式下「投递线程 = 派发线程」，多线程投递才会真的撞进同一个子程序；
    // 前面几项测试把模式切成了手动调泵，这里必须自己切回来并事后还原。
    const int nModeSaved = CmdInt("取回调模式");
    {
        Args am0;
        am0.AddInt(0);
        CmdBool("置回调线程", &am0);
    }
    nBase = g_cbDone;
    g_cbHoldMs = 80;
    PostSixEvents();
    g_cbHoldMs = 0;
    Check(g_cbDone >= nBase + 6, "并发投递的 6 条事件全部派发到齐", "");
    Check(g_cbMaxInFlight == 1, "默认串行：同时在接收子程序里的线程始终只有 1 个（事件原子性）", "");
    {
        // 自动派发的连续提交逐次完成并销毁变量，池用量不能随任务数增长。
        const LONG nPoolBeforeBurst = g_poolOutstanding;
        const int nDoneBeforeBurst = g_cbDone;
        bool allPosted = true;
        Args burst;
        burst.AddText("https://www.amazon.com/dp/B0BURST001");
        burst.AddBool(0);
        for (int i = 0; i < 200; ++i)
            if (CmdInt("模拟插件事件", &burst) <= 0) allPosted = false;
        Check(allPosted && g_cbDone == nDoneBeforeBurst + 200,
            "未启动采集的连续 200 次提交全部处理完成", "");
        Check(g_poolOutstanding == nPoolBeforeBurst,
            "连续提交并销毁变量后易语言内存池无积累", "");
    }
    {
        Args acon;
        acon.AddBool(1);
        Check(CmdBool("置事件并发", &acon) == true, "置事件并发 (真) 返回成功", "");
        g_cbHoldMs = 120;
        PostSixEvents();
        g_cbHoldMs = 0;
        Check(g_cbMaxInFlight >= 2, "放开并发后确实允许多个线程同时进子程序（开关是真生效的）", "");
        Args aoff;
        aoff.AddBool(0);
        CmdBool("置事件并发", &aoff);
        g_cbHoldMs = 80;
        PostSixEvents();
        g_cbHoldMs = 0;
        Check(g_cbMaxInFlight == 1, "收回并发后重新串行（默认态可恢复）", "");
    }
    {
        Args aback;
        aback.AddInt(nModeSaved);
        CmdBool("置回调线程", &aback);
    }

    // 等待插件消息：先灌一条断开事件，再用等待口取走，验证子线程取消息这条路
    g_pumpOn = 0;
    HttpRaw("POST", "disconnect", "{}", NULL, &r);
    Args awt;
    awt.AddInt(3000);
    Check(CmdBool("等待插件消息", &awt), "等待插件消息取到一条", "");
    g_pumpOn = 1;
    // 这次试探把连接状态断开过，立刻连回来，后面「断开与重连」一节才有起点
    nBase = g_cbCount;
    HttpRaw("GET", "handshake", NULL, "", &r);
    WaitCallback(nBase + 1, 3000);

    // 不填任务编号时按流水号回
    nBase = g_cbCount;
    char szSave[32];
    strcpy(szSave, g_cbTaskId);
    g_cbTaskId[0] = '\0';
    HttpRaw("POST", "task", "{\"url\":\"https://www.amazon.fr/dp/B0ZZZZZZZZ\",\"start\":true}", NULL, &r);
    WaitCallback(nBase + 1, 5000);
    JsonVal(r.body, "taskId", val, sizeof(val));
    Check(val[0] >= '1' && val[0] <= '9', "任务编号留空时回流水号", val);
    Check(strcmp(g_cbSite, U8_FA_GUO_ZHAN) == 0, "换站点后事件跟着变", g_cbSite);
    Check(strstr(g_cbTaskName, "B0ZZZZZZZZ") != NULL && strstr(g_cbTaskName, U8_FA_GUO_ZHAN) == g_cbTaskName,
        "插件没带任务名时按「站点名 + ASIN」自动补", g_cbTaskName);
    strcpy(g_cbTaskId, szSave);

    // 只添加不启动
    nBase = g_cbCount;
    HttpRaw("POST", "task", "{\"url\":\"https://www.amazon.com/s?k=test\",\"start\":false}", NULL, &r);
    WaitCallback(nBase + 1, 5000);
    Check(JsonHas(r.body, "\"state\":\"added\""), "start=false 回 added", r.body);
    Check(g_cbStart == BL_FALSE, "事件带「只添加」口径", "");
    Check(g_cbPageType == 1, "列表页事件页面类型是 1", "");
    Check(strstr(g_cbTaskName, U8_MEI_GUO_ZHAN) == g_cbTaskName
        && strlen(g_cbTaskName) > strlen(U8_MEI_GUO_ZHAN) + 1,
        "列表页没有 ASIN，任务名用「站点名 + 页面类型」顶上（不为空）", g_cbTaskName);

    // 采集程序拒绝
    nBase = g_cbCount;
    g_cbMode = 1;
    HttpRaw("POST", "task", "{\"url\":\"https://www.amazon.com/dp/B0REFUSE01\",\"start\":true}", NULL, &r);
    WaitCallback(nBase + 1, 5000);
    Check(r.status == 500 && JsonHas(r.body, "rejected"), "拒绝时回 500 rejected", r.body);
    Check(strstr(r.body, "REFUSED_BY_APP") != NULL, "拒绝理由带回提示文本", r.body);
    g_cbMode = 0;

    // 坏报文与非 POST
    Check(HttpRaw("GET", "task", NULL, NULL, &r) && r.status == 405, "task 用 GET 时回 405", r.body);
    Check(HttpRaw("POST", "task", "not-json", NULL, &r) && r.status == 400, "坏报文回 400", r.body);
    Check(HttpRaw("POST", "task", "{\"url\":\"https://www.ebay.com/itm/1\",\"start\":true}", NULL, &r)
        && r.status == 400, "不可采网址回 400", r.body);

    // 批量接口必须沿用真实接收事件；失败立即停止，不能把部分成功报成整批成功。
    {
        const char* batch = "{\"start\":false,\"tasks\":[{\"url\":\"https://www.amazon.com/dp/B0BATCH001\",\"taskName\":\"batch-first\"},{\"url\":\"https://www.amazon.co.jp/dp/B0BATCH002\",\"taskName\":\"batch-second\"}]}";
        nBase = g_cbCount;
        Check(HttpRaw("POST", "tasks", batch, NULL, &r) && r.status == 200,
            "批量添加通过真实 HTTP 返回200", r.body);
        Check(JsonHas(r.body, "\"ok\":true") && JsonHas(r.body, "\"successCount\":2")
            && JsonHas(r.body, "\"remainingCount\":0"), "批量成功统计准确", r.body);
        Check(g_cbCount == nBase + 2, "批量请求逐条触发两个正式采集事件", "");
        Check(strcmp(g_cbTaskName, "batch-second") == 0 && strcmp(g_cbAsin, "B0BATCH002") == 0,
            "批量按输入顺序派发并保留各项任务名与ASIN", g_cbTaskName);
        Check(g_cbStart == BL_FALSE && JsonHas(r.body, "\"state\":\"added\""),
            "公共start=false作用于未指定start的任务", r.body);
        Check(JsonHas(r.body, "\"index\":0") && JsonHas(r.body, "\"index\":1")
            && JsonHas(r.body, "\"taskId\":\"T-77\""), "批量保留索引与字符串任务编号", r.body);
        HttpRaw("POST", "tasks", "{\"start\":false,\"tasks\":[{\"url\":\"https://www.amazon.com/dp/B0BATCH003\",\"start\":true}]}", NULL, &r);
        Check(r.status == 200 && g_cbStart == BL_TRUE && JsonHas(r.body, "\"state\":\"queued\""),
            "单项start覆盖公共start", r.body);

        nBase = g_cbCount;
        HttpRaw("POST", "tasks", "{\"tasks\":[{\"url\":\"https://www.amazon.com/dp/B0BATCH004\"},{\"url\":\"https://www.ebay.com/itm/1\"},{\"url\":\"https://www.amazon.com/dp/B0BATCH005\"}]}", NULL, &r);
        Check(r.status == 200 && JsonHas(r.body, "\"ok\":false") && JsonHas(r.body, "\"successCount\":1")
            && JsonHas(r.body, "\"failedCount\":1") && JsonHas(r.body, "\"remainingCount\":1"),
            "中间任务识别失败回报部分成功和未处理数", r.body);
        Check(g_cbCount == nBase + 1 && strcmp(g_cbAsin, "B0BATCH004") == 0,
            "识别失败后不派发剩余任务", g_cbAsin);
        Check(JsonHas(r.body, "can_not_identify_url") && !JsonHas(r.body, "B0BATCH005"),
            "失败回执保留真实原因且不伪造剩余项结果", r.body);

        g_cbMode = 1;
        nBase = g_cbCount;
        HttpRaw("POST", "tasks", batch, NULL, &r);
        Check(r.status == 200 && JsonHas(r.body, "\"successCount\":0") && JsonHas(r.body, "rejected")
            && JsonHas(r.body, "REFUSED_BY_APP"), "接收程序拒绝首项时保留拒绝原因", r.body);
        Check(g_cbCount == nBase + 1 && JsonHas(r.body, "\"remainingCount\":1"),
            "接收程序拒绝后立即停止整批", r.body);
        g_cbMode = 0;

        nBase = g_cbCount;
        Check(HttpRaw("GET", "tasks", NULL, NULL, &r) && r.status == 405,
            "批量接口拒绝GET", r.body);
        Check(HttpRaw("POST", "tasks", batch, "wrong-key", &r) && r.status == 403,
            "批量接口拒绝错误会话", r.body);
        Check(HttpRaw("POST", "tasks", batch, "", &r) && r.status == 403,
            "批量接口拒绝缺少会话", r.body);
        const char* badBatches[] = {
            "not-json", "{}", "{\"tasks\":[]}", "{\"tasks\":null}", "{\"tasks\":[null]}",
            "{\"tasks\":[{}]}", "{\"tasks\":[{\"url\":123}]}",
            "{\"tasks\":[{\"url\":\"   \"}]}",
            "{\"tasks\":[{\"url\":\"https://www.amazon.com/dp/B0BATCH006\"},{}]}",
            "{\"tasks\":[{\"url\":\"https://www.amazon.com/dp/B0BATCH006\"}],\"start\":\"false\"}",
            "{\"tasks\":[{\"url\":\"https://www.amazon.com/dp/B0BATCH006\",\"start\":1}]}",
            "{\"tasks\":[{\"url\":\"https://www.amazon.com/dp/B0BATCH006\"},]}",
            "{\"tasks\":[{\"url\":\"https://www.amazon.com/dp/B0BATCH006\"}]",
            "{\"tasks\":[{\"url\":\"https://www.amazon.com/dp/B0BATCH006\",\"name\":\"bad\\q\"}]}",
            "{\"tasks\":[{\"url\":\"https://www.amazon.com/dp/B0BATCH006\"}]} trailing"
        };
        for (int i = 0; i < (int)(sizeof(badBatches) / sizeof(badBatches[0])); ++i)
        {
            HttpRaw("POST", "tasks", badBatches[i], NULL, &r);
            Check(r.status == 400 && JsonHas(r.body, "bad_json"), "批量接口拒绝非法JSON或任务结构", r.body);
        }
        char tooMany[8192] = "{\"tasks\":[";
        for (int i = 0; i < 101; ++i)
        {
            if (i) strcat(tooMany, ",");
            strcat(tooMany, "{\"url\":\"https://www.amazon.com/dp/B0BATCH006\"}");
        }
        strcat(tooMany, "]}");
        HttpRaw("POST", "tasks", tooMany, NULL, &r);
        Check(r.status == 400 && JsonHas(r.body, "bad_json"), "批量接口拒绝超过100项", r.body);
        Check(g_cbCount == nBase, "整批参数及鉴权失败均不产生任何采集事件", "");
    }

    // ---------------- 8.5 撤回任务 ----------------
    printf("-- 撤回任务 --\n");
    Check(HttpRaw("GET", "recall", NULL, NULL, &r) && r.status == 405, "recall 用 GET 时回 405", r.body);
    Check(HttpRaw("POST", "recall", "{}", NULL, &r) && r.status == 400, "不带任务号回 400", r.body);
    Check(JsonHas(r.body, "task_id_required"), "缺任务号的理由是 task_id_required", r.body);
    g_cbRecallId[0] = '\0';
    g_cbRecallAnswered = 0;
    g_cbRecallAccept = 1;
    Check(HttpRaw("POST", "recall", "{\"taskId\":\"T-321\"}", NULL, &r) && r.status == 200,
        "程序同意时撤回回 200", r.body);
    Check(JsonHas(r.body, "\"ok\":true"), "撤回应答 ok:true", r.body);
    JsonVal(r.body, "taskId", val, sizeof(val));
    Check(strcmp(val, "T-321") == 0, "回包原样带回任务编号", val);
    Check(g_cbRecallAnswered == 1, "撤回事件走到了接收子程序并能应答（事件类型 4）", "");
    Check(strcmp(g_cbRecallId, "T-321") == 0, "撤回事件带回了要撤的任务编号", g_cbRecallId);
    Check(JsonHas(r.body, "\"removed\":true"), "回执带 removed:true", r.body);
    Check(JsonHas(r.body, "\"queueCount\":3"), "回执带采集程序填的队列剩余数", r.body);
    Check(JsonHas(r.body, "\"started\":true"), "回执带撤回时是否已在采", r.body);
    Check(JsonHas(r.body, "\"seq\""), "回执带本条撤回的流水号（对账用）", r.body);
    JsonVal(r.body, "recalledAt", val, sizeof(val));
    Check(strlen(val) >= 19 && val[4] == '-' && val[7] == '-', "回执带库盖的撤回时刻", val);
    JsonVal(r.body, "message", val, sizeof(val));
    Check(strstr(val, "RECALLED") != NULL, "回执正文用的是采集程序自己填的那句（含中文，已转 UTF-8）", val);
    g_cbRecallAccept = 0;
    Check(HttpRaw("POST", "recall", "{\"taskId\":\"T-999\"}", NULL, &r) && r.status == 500,
        "程序拒收时回 500", r.body);
    Check(JsonHas(r.body, "recall_rejected"), "拒收理由是 recall_rejected", r.body);
    Check(JsonHas(r.body, "\"removed\":false"), "拒收回执 removed:false", r.body);
    Check(JsonHas(r.body, "\"queueCount\":3"), "拒收时也把队列剩余数带回执", r.body);
    g_cbRecallAccept = 1;

    // 撤回事件自带页面身份：先提交一条（登记 号→页面），再按同一个号撤回
    nBase = g_cbCount;
    HttpRaw("POST", "task", "{\"url\":\"https://www.amazon.co.jp/dp/B0JPRECALL\",\"start\":true}", NULL, &r);
    WaitCallback(nBase + 1, 5000);
    g_cbRecallId[0] = '\0';
    g_cbRecallAnswered = 0;
    Check(HttpRaw("POST", "recall", "{\"taskId\":\"T-77\"}", NULL, &r) && r.status == 200,
        "撤回刚提交的那条任务成功", r.body);
    Check(g_cbRecallAnswered == 1 && strcmp(g_cbRecallId, "T-77") == 0, "撤回事件仍是类型 4 并带号", g_cbRecallId);
    Check(strstr(g_cbUrl, "B0JPRECALL") != NULL, "撤回事件带回了当初那一页的网址", g_cbUrl);
    Check(strcmp(g_cbAsin, "B0JPRECALL") == 0, "撤回事件带回了 ASIN", g_cbAsin);
    Check(g_cbPageType == 3, "撤回事件带回了页面类型（详情页）", "");
    Check(strstr(g_cbTaskName, "B0JPRECALL") != NULL, "撤回事件带回了当初的任务名", g_cbTaskName);

    // 无人来取消息时的超时：队列引用也必须释放，不能随每次失败积累事件文本。
    Settle(6000);
    Sleep(150);
    const LONG nPoolBeforeTimeout = g_poolOutstanding;
    g_pumpOn = 0;
    Args aw;
    aw.AddInt(700);
    Check(CmdBool("置应答等待毫秒", &aw), "应答等待改成 700 毫秒", "");
    DWORD t0 = GetTickCount();
    HttpRaw("POST", "task", "{\"url\":\"https://www.amazon.com/dp/B0TIMEOUT1\",\"start\":true}", NULL, &r);
    const DWORD dwCost = GetTickCount() - t0;
    Check(r.status == 500 && JsonHas(r.body, "reply_timeout"), "没人接消息时回 reply_timeout", r.body);
    Check(dwCost >= 600 && dwCost < 4000, "超时按设定毫秒收尾", "");
    g_pumpOn = 1;
    Sleep(400);
    Check(CmdInt("取待处理消息数") == 0, "超时的事件已从队列摘走", "");
    Check(g_poolOutstanding == nPoolBeforeTimeout, "超时移除事件后易语言池分配数量恢复", "");
    {
        // 请求超时只释放调用者引用；已经进入子程序的事件要保留到调泵真正完成。
        Args manual;
        manual.AddInt(1);
        CmdBool("置回调线程", &manual);
        g_pumpOn = 1;
        Args shortWait;
        shortWait.AddInt(200);
        Check(CmdBool("置应答等待毫秒", &shortWait), "在飞超时测试等待设为 200 毫秒", "");
        const char* paths[] = { "task", "recall" };
        const char* bodies[] = {
            "{\"url\":\"https://www.amazon.com/dp/B0INFLIGHT\",\"start\":false}",
            "{\"taskId\":\"T-77\"}"
        };
        for (int i = 0; i < 2; ++i)
        {
            Settle(6000);
            const LONG poolBefore = g_poolOutstanding;
            const int doneBefore = g_cbDone;
            g_cbHoldMs = 1000;
            const bool sent = HttpRaw("POST", paths[i], bodies[i], NULL, &r);
            Check(sent && r.status == 500 && JsonHas(r.body, "reply_timeout"),
                i == 0 ? "采集回调在飞时请求按期超时" : "撤回回调在飞时请求按期超时", r.body);
            Check(g_cbInFlight > 0 && g_cbDone == doneBefore,
                i == 0 ? "采集请求已返回而回调仍持有事件" : "撤回请求已返回而回调仍持有事件", "");
            const bool callbackDone = WaitCallback(doneBefore + 1, 4000);
            g_cbHoldMs = 0;
            Check(callbackDone,
                i == 0 ? "超时后的采集回调安全执行到结束" : "超时后的撤回回调安全执行到结束", "");
            for (int wait = 0; wait < 50 && g_poolOutstanding != poolBefore; ++wait) Sleep(10);
            Check(g_poolOutstanding == poolBefore,
                i == 0 ? "在飞采集超时完成后事件内存全部释放" : "在飞撤回超时完成后事件内存全部释放", "");
        }
        Args normalWait;
        normalWait.AddInt(15000);
        CmdBool("置应答等待毫秒", &normalWait);
        Check(HttpRaw("POST", "task",
            "{\"url\":\"https://www.amazon.com/dp/B0AFTERWAIT\",\"start\":false}", NULL, &r) &&
            r.status == 200 && JsonHas(r.body, "\"ok\":true"),
            "在飞超时结束后服务仍可处理后续采集提交", r.body);
    }
    Args aw2;
    aw2.AddInt(15000);
    CmdBool("置应答等待毫秒", &aw2);

    // ---------------- 9 断开与重连 ----------------
    printf("-- 断开与重连 --\n");
    Settle(6000);
    nBase = g_cbCount;
    const int nEv0 = g_evLogNext;
    g_cbSawDisconnect = 0;
    g_cbSawAllGone = 0;
    HttpRaw("POST", "disconnect", "{}", NULL, &r);
    Check(r.status == 200 && JsonHas(r.body, "\"ok\":true"), "disconnect 返回 ok", r.body);
    Check(WaitCallback(nBase + 2, 5000), "只有一台在线时断开派发两条（单台断开 + 全部断开）", "");
    Settle(6000);
    Check(g_cbSawDisconnect != 0, "断开派发的是「插件已断开」事件", "");
    Check(g_cbSawAllGone != 0, "最后一台断开后派发「所有浏览器已断开」事件", "");
    {
        const int pTypes[] = { 3, 5 };
        const int pCounts[] = { 0, 0 };
        Check(EvLogIsSeq(nEv0, pTypes, pCounts, 2, true), "两条按「3 再 5」的顺序派发，台数都是 0", "");
    }
    Check(CmdInt("取已连接浏览器数") == 0, "全部断开后在线台数是 0", "");
    Check(CmdText("取已连接浏览器清单")[0] == '\0', "全部断开后清单是空串", "");
    Check(CmdInt("取连接状态") == 1, "断开后回到等待插件", "");
    nBase = g_cbCount;
    const int nEv1 = g_evLogNext;
    HttpRaw("GET", "handshake", NULL, "", &r);
    Check(CmdInt("取连接状态") == 2, "重连后状态回到已连接", "");
    Check(WaitCallback(nBase + 1, 5000), "重连再派发一条连接事件", "");
    Settle(6000);
    {
        const int pTypes[] = { 2 };
        const int pCounts[] = { 1 };
        Check(EvLogIsSeq(nEv1, pTypes, pCounts, 1, true), "重连的连接事件类型 2、台数 1", "");
    }

    Args ao;
    ao.AddInt(5);
    Check(CmdBool("置连接超时秒", &ao), "连接超时改成 5 秒", "");
    g_cbSawAllGone = 0;
    Sleep(7000);
    Settle(6000);
    Check(CmdInt("取连接状态") == 1, "长时间没请求自动判断开", "");
    Check(CmdInt("取已连接浏览器数") == 0, "超时判开后在线台数归 0", "");
    Check(g_cbSawAllGone != 0, "超时判开也补发了「所有浏览器已断开」", "");
    Args ao2;
    ao2.AddInt(120);
    Check(CmdBool("置连接超时秒", &ao2), "连接超时改回 120 秒", "");
    Args ao3;
    ao3.AddInt(1);
    Check(CmdBool("置连接超时秒", &ao3) == false, "超时秒数太小直接拒收", "");

    // ---------------- 9b 多台浏览器同时连 ----------------
    // 两台浏览器故意用同一个扩展 ID：打包 CRX 在 Chrome 和 Edge 里 ID 相同，
    // 分台只能靠「扩展 ID + 浏览器名」，这一节就是钉住这条口径。
    // 断言一律读事件流水（每条事件记自己的类型/台数/是谁），不读全局的最后一次赋值。
    printf("-- 多台浏览器同时连 --\n");
    {
        Settle(6000);
        nBase = g_cbCount;
        const int nEvChrome = g_evLogNext;
        AsBrowser("0.6.28", "Chrome 139", "abcdefghijklmnopabcdefghijklmnop");
        HttpRaw("GET", "handshake", NULL, "", &r);
        Check(WaitCallback(nBase + 1, 5000), "第一台上线派发一条连接事件", "");
        Settle(6000);
        {
            const EvRec* p = EvLogAt(nEvChrome);
            Check(p != NULL && p->nType == 2 && p->nCount == 1 && strcmp(p->szBrowser, "Chrome 139") == 0,
                "第一台：类型 2、已连接 1 台、点名 Chrome 139", p == NULL ? "(没记到)" : p->szBrowser);
        }
        Check(CmdInt("取已连接浏览器数") == 1, "取已连接浏览器数 是 1", "");

        nBase = g_cbCount;
        const int nEvEdge = g_evLogNext;
        AsBrowser("0.6.28", "Edge 154", "abcdefghijklmnopabcdefghijklmnop");
        HttpRaw("GET", "handshake", NULL, "", &r);
        Check(WaitCallback(nBase + 1, 5000), "第二台上线再派发一条连接事件（不是顶掉第一台）", "");
        Settle(6000);
        {
            const EvRec* p = EvLogAt(nEvEdge);
            Check(p != NULL && p->nType == 2 && p->nCount == 2 && strcmp(p->szBrowser, "Edge 154") == 0,
                "第二台：类型 2、已连接 2 台、点名 Edge 154", p == NULL ? "(没记到)" : p->szBrowser);
        }
        {
            const int pTypes[] = { 2, 2 };
            const int pCounts[] = { 1, 2 };
            Check(EvLogIsSeq(nEvChrome, pTypes, pCounts, 2, true), "两台上线各一条、台数递增 1→2", "");
        }
        Check(CmdInt("取已连接浏览器数") == 2, "取已连接浏览器数 是 2", "");
        Check(CmdInt("取连接状态") == 2, "两台在线时取连接状态仍是 2", "");
        {
            const char* pszList = CmdText("取已连接浏览器清单");
            Check(strstr(pszList, "Chrome 139") != NULL && strstr(pszList, "Edge 154") != NULL,
                "清单把两台都列出来", pszList);
            Check(strchr(pszList, '\n') != NULL, "清单一行一台", "");
        }

        // 采集事件认的是发起那台，不是「最后一次上报的那台」
        nBase = g_cbCount;
        const int nEvTask = g_evLogNext;
        HttpRaw("POST", "task", "{\"url\":\"https://www.amazon.com/dp/B0MULBROWSER\",\"start\":true}", NULL, &r);
        Check(WaitCallback(nBase + 1, 5000), "Edge 提交的采集事件到达", "");
        Settle(6000);
        {
            const EvRec* p = EvLogAt(nEvTask);
            Check(p != NULL && p->nType == 1 && p->nCount == 2 && strcmp(p->szBrowser, "Edge 154") == 0,
                "采集事件带发起浏览器名字与台数（不是最后上报的那台）", p == NULL ? "(没记到)" : p->szBrowser);
        }

        // 一台主动断开：只撤自己，另一台照旧在线，且不发「全部断开」
        nBase = g_cbCount;
        const int nEvEdgeOff = g_evLogNext;
        g_cbSawAllGone = 0;
        HttpRaw("POST", "disconnect", "{}", NULL, &r);
        Check(WaitCallback(nBase + 1, 5000), "Edge 断开派发一条事件", "");
        Settle(6000);
        {
            const int pTypes[] = { 3 };
            const int pCounts[] = { 1 };
            Check(EvLogIsSeq(nEvEdgeOff, pTypes, pCounts, 1, true), "Edge 断开只发一条「单台断开」、剩余 1 台（没发全部断开）", "");
        }
        Check(g_cbSawAllGone == 0, "还剩 Chrome 时不派发「所有浏览器已断开」", "");
        Check(CmdInt("取已连接浏览器数") == 1, "Edge 断开后还剩 Chrome 一台", "");
        Check(CmdInt("取连接状态") == 2, "还剩一台时连接状态保持已连接（老程序不受影响）", "");
        {
            const char* pszList = CmdText("取已连接浏览器清单");
            Check(strstr(pszList, "Chrome 139") != NULL && strstr(pszList, "Edge 154") == NULL,
                "清单里只剩 Chrome", pszList);
        }

        // 陌生浏览器发断开：不能把还连着的那台误判成下线，也不能把自己登记成上线
        nBase = g_cbCount;
        const int nEvStray = g_evLogNext;
        AsBrowser("0.6.28", "Firefox 130", "abcdefghijklmnopabcdefghijklmnop");
        HttpRaw("POST", "disconnect", "{}", NULL, &r);
        Check(r.status == 200, "陌生浏览器的断开请求照样正常回包", r.body);
        Settle(6000);
        Check(g_cbCount == nBase, "陌生浏览器断开没有派发任何事件", "");
        Check(g_evLogNext == nEvStray, "陌生浏览器断开没在事件流水里留下记录", "");
        Check(CmdInt("取已连接浏览器数") == 1, "陌生浏览器断开后 Chrome 照旧在线", "");

        AsBrowser("0.6.28", "Chrome 139", "abcdefghijklmnopabcdefghijklmnop");
        nBase = g_cbCount;
        HttpRaw("POST", "disconnect", "{}", NULL, &r);
        Check(WaitCallback(nBase + 2, 5000), "Chrome 断开派发两条（最后一台 + 全部断开）", "");
        Settle(6000);
        {
            const int pTypes[] = { 3, 5 };
            const int pCounts[] = { 0, 0 };
            Check(EvLogIsSeq(nEvStray, pTypes, pCounts, 2, true),
                "陌生浏览器没留事件，Chrome 随后断开才是「3 剩 0 台 + 5」", "");
        }
        Check(CmdInt("取已连接浏览器数") == 0, "两台都断开后在线台数归 0", "");
        Check(CmdInt("取连接状态") == 1, "两台都断开后回到等待插件", "");
    }

    // ---------------- 10 站点名单覆盖 ----------------
    printf("-- 站点名单覆盖 --\n");
    Args as2;
    as2.AddText("# 注释行忽略\nAmazonDe|de-site|amazon.de|DE|EUR|de\n");
    Check(CmdInt("置站点名单", &as2) == 1, "覆盖成 1 个站点", "");
    Check(HttpRaw("GET", "sites", NULL, NULL, &r) && strstr(r.body, "AmazonDe") != NULL
        && strstr(r.body, "AmazonUs") == NULL, "sites 只列自定义站点", r.body);
    HttpRaw("GET", "match?url=https%3A%2F%2Fwww.amazon.com%2Fdp%2FB0ABCDEF12", NULL, NULL, &r);
    Check(JsonHas(r.body, "\"identifiable\":false"), "被覆盖后美国站不再可采", r.body);
    Check(strstr(CmdText("取站点名单"), "amazon.de") != NULL, "取站点名单回当前生效名单", "");
    Args as3;
    as3.AddText("");
    Check(CmdInt("置站点名单", &as3) == 22, "留空恢复内置 22 站点", "");
    HttpRaw("GET", "match?url=https%3A%2F%2Fwww.amazon.com%2Fdp%2FB0ABCDEF12", NULL, NULL, &r);
    Check(JsonHas(r.body, "\"identifiable\":true"), "恢复后美国站又可采", r.body);

    // 模式 0（默认）：不调泵也要能收到事件 —— 这正是用户踩的坑
    {
        Settle(6000);                       // 先把上一节攒下的上线/断开事件派干净，再关掉调泵线程
        Args a0;
        a0.AddInt(0);
        Check(CmdBool("置回调线程", &a0), "切回库内自动派发", "");
        g_pumpOn = 0;                       // 彻底不调泵
        int nBase0 = g_cbCount;
        Check(HttpRaw("POST", "task",
            "{\"url\":\"https://www.amazon.com/dp/B0AUTO0001\",\"start\":true}", NULL, &r)
            && r.status == 200, "不调泵时 task 也直接回 200", r.body);
        Check(g_cbCount == nBase0 + 1 && g_cbLastType == 1, "不调泵也回调了接收子程序", "");
        Check(strcmp(g_cbAsin, "B0AUTO0001") == 0, "自动派发同样能还原字段", g_cbAsin);
        Check(JsonHas(r.body, "\"started\":true") && JsonHas(r.body, "\"taskId\":\"T-77\""),
            "自动派发的应答同样回给插件", r.body);
        Check(CmdInt("取待处理消息数") == 0, "自动派发不留队列", "");
        // 连接与断开事件同样自动派发
        nBase0 = g_cbCount;
        HttpRaw("GET", "handshake", NULL, "", &r);
        Check(g_cbCount == nBase0, "同一台浏览器重复握手不再刷连接事件", "");
        nBase0 = g_cbCount;
        AsBrowser("0.6.28", "Edge 154", "abcdefghijklmnopabcdefghijklmnop");
        HttpRaw("GET", "handshake", NULL, "", &r);
        Check(g_cbCount == nBase0 + 1 && g_cbLastType == 2, "新浏览器上线同样自动派发", "");
        Check(g_cbBrowserCount == 2, "自动派发时「已连接浏览器数量」也填好", "");
        AsBrowser("0.6.28", "Chrome 139", "abcdefghijklmnopabcdefghijklmnop");
        Args a1;
        a1.AddInt(1);
        CmdBool("置回调线程", &a1);
        g_pumpOn = 1;
    }

    // ---------------- 11 端口顺延与自动重试 ----------------
    printf("-- 端口 --\n");
    Check(CmdVoid("停止对接服务"), "停止服务（无返回值）", "");
    Sleep(300);
    OccupyPort(47690);
    OccupyPort(47691);
    Check(CmdBool("启动对接服务"), "前两个端口被占也能起来", "");
    const int np = CmdInt("取监听端口");
    Check(np >= 47692 && np <= 47693, "自动顺延到空闲端口", "");
    g_port = np;
    Check(HttpRaw("GET", "handshake", NULL, "", &r), "顺延后的端口能握手", "");
    CmdVoid("停止对接服务");
    ReleasePorts();

    OccupyPort(47690);
    OccupyPort(47691);
    OccupyPort(47692);
    OccupyPort(47693);
    Check(CmdBool("启动对接服务") == false, "端口全被占时启动返回假", "");
    Check(CmdBool("服务是否运行") == false, "此时确实没在监听", "");
    Check(strlen(CmdText("取对接错误")) > 0, "取对接错误给了原因", "");
    if (g_nOccupy > 0) { closesocket(g_occupy[g_nOccupy - 1]); g_nOccupy--; }
    bool bBack = false;
    for (int i = 0; i < 14; ++i)
    {
        Sleep(1000);
        if (CmdBool("服务是否运行")) { bBack = true; break; }
    }
    Check(bBack, "腾出端口后后台自动重新监听", "");
    Check(CmdInt("取监听端口") == 47693, "自动重试用的是腾出的端口", "");
    g_port = 47693;
    ReleasePorts();

    // ---------------- 12 停止与复用 ----------------
    printf("-- 停止与复用 --\n");
    Check(CmdVoid("停止对接服务"), "停止服务", "");
    Check(CmdBool("服务是否运行") == false, "停止后不在监听", "");
    Check(HttpRaw("GET", "handshake", NULL, "", &r) == false, "停止后握手打不通", "");
    Check(CmdInt("取待处理消息数") == 0, "停止后队列清空", "");
    Check(CmdBool("启动对接服务"), "停止后可以再次启动", "");
    g_port = CmdInt("取监听端口");
    HttpRaw("GET", "handshake", NULL, "", &r);
    Check(JsonHas(r.body, "\"ok\":true"), "再次启动后又能握手", r.body);
    Args al;
    al.AddText("custom log line from host");
    Check(CmdBool("写对接日志", &al), "写对接日志", "");
    const char* pszLogPath = CmdText("取对接日志路径");
    Check(_access(pszLogPath, 0) == 0, "日志文件真的落盘（中文目录走 ANSI 兜底）", pszLogPath);
    CmdVoid("停止对接服务");
    {
        // 日志写不进去必须说出来，否则整份对接过程无声丢失（用户那边就是这么丢的）
        Args abad;
        abad.AddText("1.2.0");
        abad.AddText("D:\\cxbrow-不存在-的-目录");
        CallCmd("初始化对接", &abad);
        Args al2;
        al2.AddText("probe");
        CallCmd("写对接日志", &al2);
        const char* pszE = CmdText("取对接错误");
        Check(pszE[0] != '\0' && (strstr(pszE, "日志") != NULL || strstr(pszE, "log") != NULL),
            "日志写不进去会在「取对接错误」里报出来", pszE);
    }

    printf("\n通过 %d 项，失败 %d 项\n", g_passed, g_failed);
    ReleasePorts();
    return g_failed == 0 ? 0 : 1;
}
