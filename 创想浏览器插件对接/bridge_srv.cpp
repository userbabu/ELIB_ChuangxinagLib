// 本机 HTTP 对接服务：只监听 127.0.0.1，协议与 EC 侧的 /cxa/ 保持一致，扩展那边可以照同一套客户端连。
// 不用 HttpListener 那类共享实现：Windows 的 http.sys 需要管理员或 urlacl 预留，采集软件是绿色运行，只能自己收字节流。
// winsock 入口全部运行期解析（易语言静态编译用的 VC6 链接目录里没有 ws2_32.lib）。
#include "bridge.h"
#include <stdio.h>

namespace cx {

// ===================== winsock 运行期绑定 =====================

#define BR_INVALID_SOCKET ((BR_SOCKET)(~(BR_SOCKET)0))
#define BR_SOCKET_ERROR   (-1)
typedef UINT_PTR BR_SOCKET;

struct BR_SOCKADDR_IN {
    short          sin_family;
    unsigned short sin_port;
    unsigned long  sin_addr;
    char           sin_zero[8];
};

typedef INT (WINAPI *PFN_WSAStartup)(unsigned short, void*);
typedef INT (WINAPI *PFN_WSACleanup)();
typedef INT (WINAPI *PFN_WSAGetLastError)(void);
typedef BR_SOCKET (WINAPI *PFN_socket)(int, int, int);
typedef INT (WINAPI *PFN_bind)(BR_SOCKET, const void*, int);
typedef INT (WINAPI *PFN_listen)(BR_SOCKET, int);
typedef BR_SOCKET (WINAPI *PFN_accept)(BR_SOCKET, void*, int*);
typedef INT (WINAPI *PFN_closesocket)(BR_SOCKET);
typedef INT (WINAPI *PFN_shutdown)(BR_SOCKET, int);
typedef INT (WINAPI *PFN_send)(BR_SOCKET, const char*, int, int);
typedef INT (WINAPI *PFN_recv)(BR_SOCKET, char*, int, int);
typedef INT (WINAPI *PFN_setsockopt)(BR_SOCKET, int, int, const char*, int);
typedef INT (WINAPI *PFN_ioctlsocket)(BR_SOCKET, long, unsigned long*);
typedef INT (WINAPI *PFN_SystemFunction036)(void*, unsigned long);

#define BR_AF_INET        2
#define BR_SOCK_STREAM    1
#define BR_IPPROTO_TCP    6
#define BR_SOL_SOCKET     0xffff
#define BR_SO_REUSEADDR   0x0004
#define BR_SO_RCVTIMEO    0x1006
#define BR_SO_SNDTIMEO    0x1007
#define BR_FIONBIO        0x8004601eUL
#define BR_WSAEWOULDBLOCK 10035
#define BR_INADDR_LOOPBACK 0x7f000001UL

struct WsApi {
    HMODULE h;
    PFN_WSAStartup Startup;
    PFN_WSACleanup Cleanup;
    PFN_WSAGetLastError LastError;
    PFN_socket socket;
    PFN_bind bind;
    PFN_listen listen;
    PFN_accept accept;
    PFN_closesocket closesocket;
    PFN_shutdown shutdown;
    PFN_send send;
    PFN_recv recv;
    PFN_setsockopt setsockopt;
    PFN_ioctlsocket ioctlsocket;
    PFN_SystemFunction036 Rand;
    INT  nTried;
};

static WsApi g_ws;

static unsigned short BrHtons(unsigned short v) { return (unsigned short)((v << 8) | (v >> 8)); }
static unsigned long BrHtonl(unsigned long v)
{
    return ((v & 0x000000ffUL) << 24) | ((v & 0x0000ff00UL) << 8) |
        ((v & 0x00ff0000UL) >> 8) | ((v & 0xff000000UL) >> 24);
}

static BOOL WsLoad()
{
    if (g_ws.nTried) return g_ws.Startup != NULL;
    g_ws.nTried = 1;
    g_ws.h = LoadLibraryA("ws2_32.dll");
    if (g_ws.h == NULL)
    {
        BrSetError("%s", Zh("本机缺少 ws2_32.dll，无法启动浏览器对接服务。"));
        return FALSE;
    }
    g_ws.Startup = (PFN_WSAStartup)GetProcAddress(g_ws.h, "WSAStartup");
    g_ws.Cleanup = (PFN_WSACleanup)GetProcAddress(g_ws.h, "WSACleanup");
    g_ws.LastError = (PFN_WSAGetLastError)GetProcAddress(g_ws.h, "WSAGetLastError");
    g_ws.socket = (PFN_socket)GetProcAddress(g_ws.h, "socket");
    g_ws.bind = (PFN_bind)GetProcAddress(g_ws.h, "bind");
    g_ws.listen = (PFN_listen)GetProcAddress(g_ws.h, "listen");
    g_ws.accept = (PFN_accept)GetProcAddress(g_ws.h, "accept");
    g_ws.closesocket = (PFN_closesocket)GetProcAddress(g_ws.h, "closesocket");
    g_ws.shutdown = (PFN_shutdown)GetProcAddress(g_ws.h, "shutdown");
    g_ws.send = (PFN_send)GetProcAddress(g_ws.h, "send");
    g_ws.recv = (PFN_recv)GetProcAddress(g_ws.h, "recv");
    g_ws.setsockopt = (PFN_setsockopt)GetProcAddress(g_ws.h, "setsockopt");
    g_ws.ioctlsocket = (PFN_ioctlsocket)GetProcAddress(g_ws.h, "ioctlsocket");
    HMODULE hAdv = LoadLibraryA("advapi32.dll");
    if (hAdv != NULL) g_ws.Rand = (PFN_SystemFunction036)GetProcAddress(hAdv, "SystemFunction036");
    if (g_ws.Startup == NULL || g_ws.socket == NULL || g_ws.bind == NULL || g_ws.listen == NULL ||
        g_ws.accept == NULL || g_ws.recv == NULL || g_ws.send == NULL || g_ws.closesocket == NULL)
    {
        BrSetError("%s", Zh("本机 winsock 接口不完整，无法启动浏览器对接服务。"));
        return FALSE;
    }
    return TRUE;
}

// ===================== 服务状态 =====================

#define BR_MAX_HEAD   (32 * 1024)
#define BR_MAX_BODY   (256 * 1024)
#define BR_IO_TIMEOUT 8000
#define BR_RETRY_MS   5000

static BR_SOCKET g_listen = BR_INVALID_SOCKET;
static HANDLE    g_hAccept = NULL;
static HANDLE    g_hStop = NULL;
static HANDLE    g_hTick = NULL;
static volatile INT g_run = 0;
static volatile INT g_nWorker = 0;
static INT        g_wsaReady = 0;

static void KeyMake(char* dst)
{
    unsigned char rnd[16];
    BOOL bRand = FALSE;
    if (g_ws.Rand != NULL) bRand = (g_ws.Rand(rnd, sizeof(rnd)) != 0);
    if (!bRand)
    {
        LARGE_INTEGER li;
        QueryPerformanceCounter(&li);
        DWORD seed = (DWORD)(li.LowPart ^ (li.HighPart << 1) ^ GetTickCount() ^
            ((DWORD)GetCurrentProcessId() << 7) ^ ((DWORD)GetCurrentThreadId() << 3));
        for (INT i = 0; i < 16; ++i)
        {
            seed = seed * 1103515245UL + 12345UL;
            rnd[i] = (unsigned char)(seed >> 16);
        }
    }
    // 小写十六进制 32 位，和 EC 侧 GUID("N") 的形态一致
    static const char hex[] = "0123456789abcdef";
    for (INT i = 0; i < 16; ++i)
    {
        dst[i * 2] = hex[rnd[i] >> 4];
        dst[i * 2 + 1] = hex[rnd[i] & 0x0f];
    }
    dst[32] = '\0';
}

// ===================== 请求解析 =====================

struct BrReq {
    char  szMethod[8];
    char  szPath[1024];       // 不含查询
    char  szQuery[1024];
    char  szOrigin[200];
    char  szKey[64];
    char  szExtVer[64];
    char  szExtBrowser[96];
    char  szExtId[160];
    char  szUrl[2048];        // 查询里的 url，已解百分号
    char  szBody[BR_MAX_BODY];
    INT   nBodyLen;
    Buf*  pCapturedReply;     // 批量受理时收集单条回执，由外层统一输出 HTTP 响应
    INT*  pCapturedStatus;
};

static void HeaderValue(const char* pszHead, const char* pszName, char* dst, INT cchDst)
{
    dst[0] = '\0';
    const INT nName = StrLen(pszName);
    const char* p = pszHead;
    while (*p != '\0')
    {
        // 逐行找 "名字:"，大小写无关
        if (StrNCmpI(p, pszName, nName) && p[nName] == ':')
        {
            const char* v = p + nName + 1;
            while (*v == ' ' || *v == '\t') ++v;
            INT i = 0;
            for (; v[i] != '\0' && v[i] != '\r' && v[i] != '\n' && i < cchDst - 1; ++i) dst[i] = v[i];
            dst[i] = '\0';
            StrTrim(dst);
            return;
        }
        // 跳到下一行
        while (*p != '\0' && *p != '\n') ++p;
        if (*p == '\n') ++p;
    }
}

static void UrlDecode(const char* src, char* dst, INT cchDst)
{
    INT j = 0;
    for (INT i = 0; src[i] != '\0' && j < cchDst - 1; ++i)
    {
        char c = src[i];
        if (c == '%' && src[i + 1] != '\0' && src[i + 2] != '\0')
        {
            INT h1 = -1, h2 = -1;
            char a = src[i + 1], b = src[i + 2];
            if (a >= '0' && a <= '9') h1 = a - '0'; else if (a >= 'a' && a <= 'f') h1 = a - 'a' + 10; else if (a >= 'A' && a <= 'F') h1 = a - 'A' + 10;
            if (b >= '0' && b <= '9') h2 = b - '0'; else if (b >= 'a' && b <= 'f') h2 = b - 'a' + 10; else if (b >= 'A' && b <= 'F') h2 = b - 'A' + 10;
            if (h1 >= 0 && h2 >= 0)
            {
                dst[j++] = (char)((h1 << 4) | h2);
                i += 2;
                continue;
            }
        }
        if (c == '+') c = ' ';
        dst[j++] = c;
    }
    dst[j] = '\0';
}

static BOOL QueryValue(const char* pszQuery, const char* pszKey, char* dst, INT cchDst)
{
    const INT nKey = StrLen(pszKey);
    const char* p = pszQuery;
    while (*p != '\0')
    {
        if (StrNCmpI(p, pszKey, nKey) && p[nKey] == '=')
        {
            const char* v = p + nKey + 1;
            INT i = 0;
            char raw[2048];
            for (; v[i] != '\0' && v[i] != '&' && i < (INT)sizeof(raw) - 1; ++i) raw[i] = v[i];
            raw[i] = '\0';
            UrlDecode(raw, dst, cchDst);
            return TRUE;
        }
        while (*p != '\0' && *p != '&') ++p;
        if (*p == '&') ++p;
    }
    dst[0] = '\0';
    return FALSE;
}

static BOOL ParseRequest(const char* pszAll, BrReq* pReq)
{
    ZeroMemory(pReq, sizeof(*pReq));
    const char* pEnd = StrFindI(pszAll, "\r\n\r\n");
    char szHead[BR_MAX_HEAD];
    if (pEnd == NULL) StrCpyN(szHead, sizeof(szHead), pszAll);
    else
    {
        INT n = (INT)(pEnd - pszAll);
        if (n > (INT)sizeof(szHead) - 1) n = (INT)sizeof(szHead) - 1;
        memcpy(szHead, pszAll, n);
        szHead[n] = '\0';
    }
    // 请求行
    INT i = 0;
    while (szHead[i] != '\0' && szHead[i] != ' ' && i < (INT)sizeof(pReq->szMethod) - 1)
    {
        pReq->szMethod[i] = szHead[i];
        ++i;
    }
    pReq->szMethod[i] = '\0';
    while (szHead[i] == ' ') ++i;
    INT j = 0;
    while (szHead[i] != '\0' && szHead[i] != ' ' && szHead[i] != '\r' && szHead[i] != '\n')
    {
        if (j < (INT)sizeof(pReq->szPath) - 1) pReq->szPath[j++] = szHead[i];
        ++i;
    }
    pReq->szPath[j] = '\0';
    const char* pQ = StrFindI(pReq->szPath, "?");
    if (pQ != NULL)
    {
        INT nPath = (INT)(pQ - pReq->szPath);
        StrCpyN(pReq->szQuery, sizeof(pReq->szQuery), pQ + 1);
        pReq->szPath[nPath] = '\0';
    }
    HeaderValue(szHead, "Origin", pReq->szOrigin, sizeof(pReq->szOrigin));
    HeaderValue(szHead, "x-cxa-key", pReq->szKey, sizeof(pReq->szKey));
    HeaderValue(szHead, "x-cxa-ext", pReq->szExtVer, sizeof(pReq->szExtVer));
    HeaderValue(szHead, "x-cxa-ext-browser", pReq->szExtBrowser, sizeof(pReq->szExtBrowser));
    HeaderValue(szHead, "x-cxa-ext-id", pReq->szExtId, sizeof(pReq->szExtId));
    if (pReq->szExtId[0] == '\0' && StrFindI(pReq->szOrigin, "://") != NULL)
    {
        StrCpyN(pReq->szExtId, sizeof(pReq->szExtId), StrFindI(pReq->szOrigin, "://") + 3);
        INT n = StrLen(pReq->szExtId);
        while (n > 0 && (pReq->szExtId[n - 1] == '/' || pReq->szExtId[n - 1] == ' ')) pReq->szExtId[--n] = '\0';
    }
    QueryValue(pReq->szQuery, "url", pReq->szUrl, sizeof(pReq->szUrl));

    if (pEnd != NULL)
    {
        const char* pBody = pEnd + 4;
        INT n = (INT)strlen(pBody);
        if (n > (INT)sizeof(pReq->szBody) - 1) n = (INT)sizeof(pReq->szBody) - 1;
        memcpy(pReq->szBody, pBody, n);
        pReq->szBody[n] = '\0';
        pReq->nBodyLen = n;
    }
    return TRUE;
}

// ===================== 应答 =====================

static BOOL IsExtensionOrigin(const char* pszOrigin)
{
    return StrHasI(pszOrigin, "chrome-extension://") || StrHasI(pszOrigin, "moz-extension://") ||
        StrHasI(pszOrigin, "edge-extension://") || StrHasI(pszOrigin, "chrome-search://");
}

static void SendAll(BR_SOCKET s, const char* psz, INT n)
{
    INT sent = 0;
    while (sent < n)
    {
        const INT r = g_ws.send(s, psz + sent, n - sent, 0);
        if (r <= 0) return;
        sent += r;
    }
}

static void Reply(BR_SOCKET s, INT nStatus, const char* pszStatus, const BrReq* pReq, const char* pszJson)
{
    if (pReq->pCapturedReply != NULL)
    {
        BufAddSz(pReq->pCapturedReply, pszJson);
        *pReq->pCapturedStatus = nStatus;
        return;
    }
    Buf b;
    BufInit(&b);
    BufAddF(&b, "HTTP/1.1 %d %s\r\n", nStatus, pszStatus);
    BufAddSz(&b, "Content-Type: application/json; charset=utf-8\r\n");
    BufAddSz(&b, "Content-Length: ");
    BufAddF(&b, "%d\r\n", StrLen(pszJson));
    BufAddSz(&b, "Cache-Control: no-store\r\nConnection: close\r\n");
    if (IsExtensionOrigin(pReq->szOrigin))
    {
        // 浏览器要求回显来源并明确放行私有网络，否则扩展在 https 页面上打不开本机端口
        BufAddSz(&b, "Access-Control-Allow-Origin: ");
        BufAdd(&b, pReq->szOrigin, StrLen(pReq->szOrigin));
        BufAddSz(&b, "\r\n");
        BufAddSz(&b, "Access-Control-Allow-Headers: content-type, x-cxa-key, x-cxa-ext, x-cxa-ext-browser, x-cxa-ext-id\r\n");
        BufAddSz(&b, "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n");
        BufAddSz(&b, "Access-Control-Allow-Private-Network: true\r\n");
        BufAddSz(&b, "Access-Control-Max-Age: 600\r\nVary: Origin\r\n");
    }
    BufAddSz(&b, "\r\n");
    BufAdd(&b, pszJson, StrLen(pszJson));
    if (b.p != NULL) SendAll(s, b.p, b.nLen);
    BufFree(&b);
}

static void ReplyErr(BR_SOCKET s, INT nStatus, const BrReq* pReq, const char* pszCode, const char* pszMsgUtf8)
{
    Buf b;
    BufInit(&b);
    BufAddSz(&b, "{\"ok\":false,\"code\":");
    BufAddJsonStr(&b, pszCode);
    BufAddSz(&b, ",\"message\":");
    BufAddJsonStr(&b, pszMsgUtf8);
    BufAddCh(&b, '}');
    const char* psz = (b.p != NULL) ? b.p : "{}";
    const INT nStatus2 = nStatus;
    const char* pszText = (nStatus2 == 400) ? "Bad Request" : (nStatus2 == 403) ? "Forbidden" :
        (nStatus2 == 404) ? "Not Found" : (nStatus2 == 405) ? "Method Not Allowed" :
        (nStatus2 == 503) ? "Service Unavailable" : "Internal Server Error";
    Reply(s, nStatus2, pszText, pReq, psz);
    BufFree(&b);
}

// ===================== 浏览器会话表 =====================
// 一台浏览器一条记录。以前状态只有一份：第二个浏览器握手会把第一个的身份顶掉，任何一方断开就全体判离线。
// 现在按 (扩展ID, 浏览器名) 各记各的心跳与超时；g_stat 留作「汇总 + 最后上报的那台」，老程序不受影响。

static BrSess g_sess[BR_MAX_SESSIONS];

static INT SessIndexOf(INT nCount, const char* pszExtId, const char* pszBrowser)
{
    for (INT i = 0; i < nCount; ++i)
    {
        // 空槽的两个字段也是空串：不跳过就会让「没带身份的客户端」永远匹配到第一个空槽，
        // 结果就是新浏览器永远算不上「新上线」，身份也永远写不进去。
        if (g_sess[i].dwLastReq == 0) continue;
        if (StrEqI(g_sess[i].szExtId, pszExtId) && StrEqI(g_sess[i].szBrowser, pszBrowser)) return i;
    }
    return -1;
}

static INT SessCountLocked()
{
    INT n = 0;
    for (INT i = 0; i < BR_MAX_SESSIONS; ++i)
    {
        if (g_sess[i].dwLastReq != 0) ++n;
    }
    return n;
}

// 表满时挑一个「最久没露面」的位置改写给新浏览器，不给被顶掉的那台补发断开事件：
// 一台机器同时挂 16 台以上浏览器不是真实场景，宁可少记一台也不在这里套出递归派发。
static INT SessEvictLocked()
{
    INT nWorst = 0;
    for (INT i = 1; i < BR_MAX_SESSIONS; ++i)
    {
        if ((LONG)(g_sess[i].dwLastReq - g_sess[nWorst].dwLastReq) < 0) nWorst = i;
    }
    BrLogOnce("sess-full", Zh("%s（最多同时记录 %d 台）"), Zh("浏览器会话表已满，最久没露面的一台已让位给新来的"), BR_MAX_SESSIONS);
    return nWorst;
}

BOOL SessTouch(const char* pszExtId, const char* pszBrowser, const char* pszExtVer, BOOL bCreate, BrSess* pOut, INT* pnCount)
{
    char szId[160], szBr[96];
    StrCpyN(szId, sizeof(szId), pszExtId == NULL ? "" : pszExtId);
    StrCpyN(szBr, sizeof(szBr), pszBrowser == NULL ? "" : pszBrowser);

    EnterCriticalSection(&g_cs);
    const DWORD dwNow = GetTickCount();
    INT nSlot = SessIndexOf(BR_MAX_SESSIONS, szId, szBr);
    if (nSlot < 0) nSlot = SessIndexOf(BR_MAX_SESSIONS, "", "");   // 没带身份的客户端共用这一条
    if (nSlot < 0 && !bCreate)
    {
        // 不在在线清单里的浏览器只是路过（典型是它那条断开请求）：不登记、不改心跳、不派发任何事件
        if (pOut != NULL) ZeroMemory(pOut, sizeof(*pOut));
        if (pnCount != NULL) *pnCount = SessCountLocked();
        LeaveCriticalSection(&g_cs);
        return FALSE;
    }
    BOOL bNew = FALSE;
    if (nSlot < 0)
    {
        for (INT i = 0; i < BR_MAX_SESSIONS; ++i)
        {
            if (g_sess[i].dwLastReq == 0) { nSlot = i; break; }
        }
        if (nSlot < 0) nSlot = SessEvictLocked();
        ZeroMemory(&g_sess[nSlot], sizeof(g_sess[nSlot]));
        StrCpyN(g_sess[nSlot].szExtId, sizeof(g_sess[nSlot].szExtId), szId);
        StrCpyN(g_sess[nSlot].szBrowser, sizeof(g_sess[nSlot].szBrowser), szBr);
        g_sess[nSlot].dwFirstAt = dwNow;
        ++g_stat.nConn;
        bNew = TRUE;
    }
    g_sess[nSlot].dwLastReq = dwNow;
    ++g_sess[nSlot].nReq;
    if (pszExtVer != NULL && pszExtVer[0] != '\0')
        StrCpyN(g_sess[nSlot].szExtVer, sizeof(g_sess[nSlot].szExtVer), pszExtVer);

    // 汇总位：至少一台在线就算「已连接」
    const INT nCount = SessCountLocked();
    if (!g_stat.connected)
    {
        g_stat.connected = 1;
        g_stat.dwConnectedAt = dwNow;
    }
    g_stat.dwLastReq = dwNow;
    ++g_stat.nReq;
    if (pszExtVer != NULL && pszExtVer[0] != '\0') StrCpyN(g_stat.szExtVer, sizeof(g_stat.szExtVer), pszExtVer);
    if (szBr[0] != '\0') StrCpyN(g_stat.szExtBrowser, sizeof(g_stat.szExtBrowser), szBr);
    if (szId[0] != '\0') StrCpyN(g_stat.szExtId, sizeof(g_stat.szExtId), szId);
    if (pOut != NULL) *pOut = g_sess[nSlot];
    if (pnCount != NULL) *pnCount = nCount;
    LeaveCriticalSection(&g_cs);
    return bNew;
}

INT SessOnline()
{
    BrInitOnce();        // 程序可能一条命令都没调就先问台数，锁要先建起来
    EnterCriticalSection(&g_cs);
    const INT n = SessCountLocked();
    LeaveCriticalSection(&g_cs);
    return n;
}

// 摘掉一条记录并把汇总位跟着改（调用方已持有 g_cs）
static void SessDropLocked(INT nSlot, BrSess* pOut, INT* pnCount)
{
    if (pOut != NULL) *pOut = g_sess[nSlot];
    ZeroMemory(&g_sess[nSlot], sizeof(g_sess[nSlot]));
    const INT nLeft = SessCountLocked();
    if (nLeft == 0) g_stat.connected = 0;
    if (pnCount != NULL) *pnCount = nLeft;
}

BOOL SessDrop(const char* pszExtId, const char* pszBrowser, BrSess* pOut, INT* pnCount)
{
    char szId[160], szBr[96];
    StrCpyN(szId, sizeof(szId), pszExtId == NULL ? "" : pszExtId);
    StrCpyN(szBr, sizeof(szBr), pszBrowser == NULL ? "" : pszBrowser);
    BOOL bHit = FALSE;
    EnterCriticalSection(&g_cs);
    const INT nSlot = SessIndexOf(BR_MAX_SESSIONS, szId, szBr);
    if (nSlot >= 0)
    {
        SessDropLocked(nSlot, pOut, pnCount);
        bHit = TRUE;
    }
    else if (pnCount != NULL) *pnCount = SessCountLocked();
    LeaveCriticalSection(&g_cs);
    return bHit;
}

BOOL SessTakeExpired(DWORD dwTimeoutMs, BrSess* pOut, INT* pnCount)
{
    BOOL bHit = FALSE;
    EnterCriticalSection(&g_cs);
    const DWORD dwNow = GetTickCount();
    for (INT i = 0; i < BR_MAX_SESSIONS; ++i)
    {
        if (g_sess[i].dwLastReq == 0) continue;
        if ((DWORD)(dwNow - g_sess[i].dwLastReq) <= dwTimeoutMs) continue;
        SessDropLocked(i, pOut, pnCount);
        bHit = TRUE;
        break;
    }
    if (!bHit && pnCount != NULL) *pnCount = SessCountLocked();
    LeaveCriticalSection(&g_cs);
    return bHit;
}

void SessClearAll()
{
    EnterCriticalSection(&g_cs);
    ZeroMemory(g_sess, sizeof(g_sess));
    g_stat.connected = 0;
    LeaveCriticalSection(&g_cs);
}

// 每行一台：浏览器名|插件版本|扩展ID|已在线秒数
void SessListText(char* dst, INT cchDst)
{
    if (dst == NULL || cchDst <= 0) return;
    dst[0] = '\0';
    BrInitOnce();
    Buf b;
    BufInit(&b);
    EnterCriticalSection(&g_cs);
    const DWORD dwNow = GetTickCount();
    for (INT i = 0; i < BR_MAX_SESSIONS; ++i)
    {
        if (g_sess[i].dwLastReq == 0) continue;
        if (b.nLen > 0) BufAddCh(&b, '\n');
        INT nSec = (INT)((dwNow - g_sess[i].dwFirstAt) / 1000UL);
        if (nSec < 0) nSec = 0;
        BufAddF(&b, "%s|%s|%s|%d",
            g_sess[i].szBrowser[0] ? g_sess[i].szBrowser : Zh("未知浏览器"),
            g_sess[i].szExtVer[0] ? g_sess[i].szExtVer : Zh("未带版本"),
            g_sess[i].szExtId[0] ? g_sess[i].szExtId : Zh("未带编号"),
            nSec);
    }
    LeaveCriticalSection(&g_cs);
    StrCpyN(dst, cchDst, b.p == NULL ? "" : b.p);
    BufFree(&b);
}

// ===================== 端点 =====================

static BOOL KeyOk(const BrReq* pReq)
{
    return g_stat.szKey[0] != '\0' && StrEqI(pReq->szKey, g_stat.szKey);
}

// 每个请求都过这里：记下这台浏览器的心跳；第一次露面就派发一条「已连接」事件。
// bCreate 只有断开接口传假 —— 见 SessTouch 的注释。
static void Touch(const BrReq* pReq, BOOL bCreate = TRUE)
{
    BrSess s;
    INT nCount = 0;
    const BOOL bNew = SessTouch(pReq == NULL ? "" : pReq->szExtId,
        pReq == NULL ? "" : pReq->szExtBrowser,
        pReq == NULL ? "" : pReq->szExtVer, bCreate, &s, &nCount);
    if (!bNew) return;

    BrLog(Zh("%s：第 %d 台 %s（插件 %s / %s）"), Zh("浏览器上线"), nCount,
        s.szBrowser[0] ? s.szBrowser : Zh("未知浏览器"),
        s.szExtVer[0] ? s.szExtVer : Zh("未带版本"),
        s.szExtId[0] ? s.szExtId : Zh("未带编号"));

    BrEv* pEv = EvNew(EV_CONNECT);
    char szTime[32];
    NowText(szTime, sizeof(szTime));
    EvSetText(pEv, BM_URL, "");
    EvSetText(pEv, BM_EXT_VER, s.szExtVer);
    EvSetText(pEv, BM_BROWSER, s.szBrowser);
    EvSetText(pEv, BM_EXT_ID, s.szExtId);
    EvSetText(pEv, BM_ARRIVE, szTime);
    EvPost(pEv, 0);
}

static void HandleHandshake(BR_SOCKET s, const BrReq* pReq)
{
    // 心跳与「上线」事件都在 Touch 里按浏览器逐台处理，这里只回握手数据
    Touch(pReq);

    Buf b;
    BufInit(&b);
    BufAddCh(&b, '{');
    JsonBool(&b, "ok", 1, FALSE);
    JsonStr(&b, "service", g_cfg.szService, TRUE);
    JsonInt(&b, "bridge", 1, TRUE);
    JsonStr(&b, "version", ToUtf8(g_cfg.szVersion), TRUE);
    JsonStr(&b, "app", ToUtf8(g_cfg.szApp), TRUE);
    JsonBool(&b, "ready", 1, TRUE);
    JsonInt(&b, "port", g_stat.port, TRUE);
    JsonStr(&b, "key", g_stat.szKey, TRUE);
    JsonInt(&b, "browsers", SessOnline(), TRUE);
    BufAddCh(&b, '}');
    Reply(s, 200, "OK", pReq, b.p == NULL ? "{}" : b.p);
    BufFree(&b);
}

static void HandleSites(BR_SOCKET s, const BrReq* pReq)
{
    Touch(pReq);
    Buf b;
    BufInit(&b);
    BufAddSz(&b, "{\"ok\":true,\"ready\":true");
    JsonInt(&b, "siteCount", SiteCount(), TRUE);
    JsonKey(&b, "sites", TRUE);
    BufAddCh(&b, '[');
    SiteJson(&b);
    BufAddCh(&b, ']');
    BufAddCh(&b, '}');
    Reply(s, 200, "OK", pReq, b.p == NULL ? "{}" : b.p);
    BufFree(&b);
}

static void MatchJson(Buf* b, const BrMatch* pM, const char* pszUrl)
{
    BufAddCh(b, '{');
    JsonBool(b, "ok", 1, FALSE);
    JsonBool(b, "identifiable", pM->bIdentifiable, TRUE);
    JsonBool(b, "universal", 0, TRUE);
    JsonStr(b, "platformName", pM->platform, TRUE);
    JsonStr(b, "displayName", pM->display, TRUE);
    JsonInt(b, "pageType", pM->nPageType, TRUE);
    JsonStr(b, "colModType", pM->nColMode == CM_DETAIL ? "Detail" : (pM->nColMode == CM_LIST ? "List" : "Unknown"), TRUE);
    JsonBool(b, "canDetail", pM->nColMode == CM_DETAIL, TRUE);
    JsonBool(b, "auth", 1, TRUE);
    char szName[2048];
    if (pM->asin[0] != '\0') StrCpyN(szName, sizeof(szName), pM->asin);
    else
    {
        const char* p = StrFindI(pszUrl, "://");
        StrCpyN(szName, sizeof(szName), (p != NULL) ? p + 3 : pszUrl);
    }
    JsonStr(b, "name", szName, TRUE);
    if (pM->country[0] != '\0')
    {
        JsonStr(b, "country", pM->country, TRUE);
        JsonStr(b, "currency", pM->currency, TRUE);
        JsonStr(b, "language", pM->language, TRUE);
    }
    if (pM->asin[0] != '\0') JsonStr(b, "asin", pM->asin, TRUE);
    BufAddCh(b, '}');
}

static void HandleMatch(BR_SOCKET s, const BrReq* pReq)
{
    Touch(pReq);
    if (pReq->szUrl[0] == '\0')
    {
        ReplyErr(s, 400, pReq, "url_invalid", Zh("缺少要识别的网址。"));
        return;
    }
    BrMatch m;
    MatchUrl(pReq->szUrl, &m);
    if (!m.bOk)
    {
        // 域名根本不在名单里：回 identifiable:false，让页面自己把按钮撤掉，不算错误
        m.bIdentifiable = FALSE;
    }
    Buf b;
    BufInit(&b);
    MatchJson(&b, &m, pReq->szUrl);
    Reply(s, 200, "OK", pReq, b.p == NULL ? "{}" : b.p);
    BufFree(&b);
}

// 一台浏览器掉线：先派发「插件已断开」（带它是谁、还剩几台）；剩 0 台再补一条「所有浏览器已断开」。
// 单独一条 5 是给程序做「整体收工」用的（停按钮提示、清界面状态），不用自己数台数。
static void SessFireGone(const BrSess* pS, INT nLeft, const char* pszReason)
{
    char szId[160], szBr[96], szVer[64];
    if (pS != NULL)
    {
        StrCpyN(szId, sizeof(szId), pS->szExtId);
        StrCpyN(szBr, sizeof(szBr), pS->szBrowser);
        StrCpyN(szVer, sizeof(szVer), pS->szExtVer);
    }
    else
    {
        StrCpyN(szId, sizeof(szId), g_stat.szExtId);
        StrCpyN(szBr, sizeof(szBr), g_stat.szExtBrowser);
        StrCpyN(szVer, sizeof(szVer), g_stat.szExtVer);
    }
    BrLog(Zh("%s：%s（插件 %s / %s），还剩 %d 台"), Zh("浏览器断开"),
        szBr[0] ? szBr : Zh("未知浏览器"),
        szVer[0] ? szVer : Zh("未带版本"),
        szId[0] ? szId : Zh("未带编号"), nLeft);

    char szTime[32];
    NowText(szTime, sizeof(szTime));

    BrEv* pEv = EvNew(EV_DISCONNECT);
    EvSetText(pEv, BM_ARRIVE, szTime);
    // 断开原因放进「请求内容」：出参文本成员归易语言所有，库不能往里塞自己的指针
    EvSetText(pEv, BM_RAW, pszReason);
    EvSetText(pEv, BM_EXT_VER, szVer);
    EvSetText(pEv, BM_BROWSER, szBr);
    EvSetText(pEv, BM_EXT_ID, szId);
    EvPost(pEv, 0);

    if (nLeft > 0) return;
    BrLog("%s", Zh("已没有任何浏览器在线。"));
    BrEv* pAll = EvNew(EV_ALL_DISCONNECT);
    EvSetText(pAll, BM_ARRIVE, szTime);
    EvSetText(pAll, BM_RAW, Zh("所有浏览器都已断开对接。"));
    EvSetText(pAll, BM_EXT_VER, szVer);
    EvSetText(pAll, BM_BROWSER, szBr);
    EvSetText(pAll, BM_EXT_ID, szId);
    EvPost(pAll, 0);
}

// 主动断开：只撤自己这一台，别的浏览器照常在线。
static void DisconnectNotify(BR_SOCKET s, const BrReq* pReq, const char* pszReason)
{
    BrSess gone;
    INT nLeft = 0;
    const BOOL bHit = SessDrop(pReq == NULL ? "" : pReq->szExtId,
        pReq == NULL ? "" : pReq->szExtBrowser, &gone, &nLeft);
    if (bHit) SessFireGone(&gone, nLeft, pszReason);
    else BrLog("%s", Zh("收到断开请求，但发起方不在在线清单里，已忽略。"));
    if (s != BR_INVALID_SOCKET) Reply(s, 200, "OK", pReq, "{\"ok\":true}");
}

static void HandleTask(BR_SOCKET s, const BrReq* pReq)
{
    Touch(pReq);
    if (!StrEqI(pReq->szMethod, "POST"))
    {
        ReplyErr(s, 405, pReq, "method_not_allowed", Zh("插入任务只能使用 POST。"));
        return;
    }
    char szUrl[2048];
    szUrl[0] = '\0';
    if (pReq->nBodyLen > 0) JsonGetString(pReq->szBody, "url", szUrl, sizeof(szUrl));
    if (szUrl[0] == '\0') StrCpyN(szUrl, sizeof(szUrl), pReq->szUrl);
    if (szUrl[0] == '\0')
    {
        ReplyErr(s, 400, pReq, "bad_json", Zh("请求内容解析失败，请确认是 JSON 文本。"));
        return;
    }
    INT nStart = 1;
    JsonGetBool(pReq->szBody, "start", &nStart);
    char szTitle[1024];
    szTitle[0] = '\0';
    JsonGetString(pReq->szBody, "title", szTitle, sizeof(szTitle));
    char szAsinExt[16];
    szAsinExt[0] = '\0';
    JsonGetString(pReq->szBody, "asin", szAsinExt, sizeof(szAsinExt));
    // 任务名：插件提交任务时可带 taskName（也认 name —— ac-client 的 addTask 第三参 info 里哪个都行）
    char szTaskName[256];
    szTaskName[0] = '\0';
    if (!JsonGetString(pReq->szBody, "taskName", szTaskName, sizeof(szTaskName)))
        JsonGetString(pReq->szBody, "name", szTaskName, sizeof(szTaskName));

    BrMatch m;
    MatchUrl(szUrl, &m);
    if (!m.bIdentifiable)
    {
        BrLogOnce("task", "%s %s", Zh("收到无法识别的网址："), szUrl);
        ReplyErr(s, 400, pReq, "can_not_identify_url", Zh("这个网址不是可采集的页面，请在商品页或列表页点「发送到采集」。"));
        return;
    }
    if (!m.country[0])
    {
        ReplyErr(s, 400, pReq, "country_code_not_supported", Zh("该站点国家代码不在 AC 支持列表中，任务未提交。"));
        return;
    }

    // 与「模拟插件事件」共用同一份组包：自检走的就是真链路
    BrEv* pEv = EvMakeTask(szUrl, nStart, szTitle, szAsinExt, pReq->szBody, szTaskName);
    if (pEv == NULL)
    {
        ReplyErr(s, 400, pReq, "identify_failed", Zh("网址识别失败。"));
        return;
    }

    // 这一条事件是谁发的以本次请求的头为准：g_stat 只记最后一次上报的那台，多台浏览器在线时会认错人
    EvSetText(pEv, BM_EXT_VER, pReq->szExtVer);
    EvSetText(pEv, BM_BROWSER, pReq->szExtBrowser);
    EvSetText(pEv, BM_EXT_ID, pReq->szExtId);

    EnterCriticalSection(&g_cs);
    ++g_stat.nTask;
    LeaveCriticalSection(&g_cs);

    const char* pszTypeName = (m.nPageType == PT_DETAIL) ? Zh("详情页") :
        (m.nPageType == PT_SHOP) ? Zh("店铺页") : Zh("列表页");
    BrLog(Zh("收到采集请求：%s %s 开始=%d 插件版本=%s"),
        pszTypeName, szUrl, nStart, g_stat.szExtVer[0] ? g_stat.szExtVer : Zh("未带"));

    if (!EvBound())
    {
        BrLogOnce("nobind", "%s", Zh("采集程序没有绑定接收子程序，请先执行「绑定事件子程序」。"));
        ReplyErr(s, 503, pReq, "no_binding", Zh("采集程序当前没有接收任务，请稍后重试。"));
        EvRelease(pEv);
        return;
    }

    const BOOL bAnswered = EvPost(pEv, g_cfg.nReplyWaitMs > 0 ? g_cfg.nReplyWaitMs : 15000);
    if (!bAnswered)
    {
        EvRelease(pEv);
        BrLogOnce("reply", "%s", Zh("采集程序没有在限定时间内应答，请确认界面定时器在调用「处理插件消息」。"));
        Buf b;
        BufInit(&b);
        BufAddSz(&b, "{\"ok\":false,\"code\":");
        BufAddJsonStr(&b, "reply_timeout");
        BufAddSz(&b, ",\"message\":");
        BufAddJsonStr(&b, Zh("采集程序没有及时应答：请在程序里调「置回调线程 (0)」让库自动派发，或定时调用「处理插件消息」。"));
        BufAddCh(&b, '}');
        Reply(s, 500, "Internal Server Error", pReq, b.p == NULL ? "{}" : b.p);
        BufFree(&b);
        return;
    }
    // 应答先全部抄出来，再放手这个事件（放手后派发者随时可能回收）
    char szTaskId[128], szMsg[512];
    StrCpyN(szTaskId, sizeof(szTaskId), pEv->pszTaskId == NULL ? "" : pEv->pszTaskId);
    StrCpyN(szMsg, sizeof(szMsg), pEv->pszMessage == NULL ? "" : pEv->pszMessage);
    const INT nAccept = pEv->bAccept;
    const INT nStarted = pEv->bStarted;
    const INT nAlready = pEv->bAlready;
    const INT nQueue = pEv->nQueue;
    const INT nSeq = pEv->nSeq;
    // 先把这一页的身份抄进局部结构（EvRelease 之后 pEv 随时可能被回收，不能再读）
    BrTaskRef ref;
    memset(&ref, 0, sizeof(ref));
    StrCpyN(ref.szUrl, sizeof(ref.szUrl), pEv->m[BM_URL] ? (const char*)(INT_PTR)pEv->m[BM_URL] : szUrl);
    StrCpyN(ref.szAsin, sizeof(ref.szAsin), pEv->m[BM_ASIN] ? (const char*)(INT_PTR)pEv->m[BM_ASIN] : "");
    StrCpyN(ref.szSite, sizeof(ref.szSite), pEv->m[BM_SITE] ? (const char*)(INT_PTR)pEv->m[BM_SITE] : "");
    StrCpyN(ref.szPlatform, sizeof(ref.szPlatform), pEv->m[BM_PLATFORM] ? (const char*)(INT_PTR)pEv->m[BM_PLATFORM] : "");
    StrCpyN(ref.szCountry, sizeof(ref.szCountry), pEv->m[BM_COUNTRY] ? (const char*)(INT_PTR)pEv->m[BM_COUNTRY] : "");
    StrCpyN(ref.szCurrency, sizeof(ref.szCurrency), pEv->m[BM_CURRENCY] ? (const char*)(INT_PTR)pEv->m[BM_CURRENCY] : "");
    StrCpyN(ref.szLanguage, sizeof(ref.szLanguage), pEv->m[BM_LANGUAGE] ? (const char*)(INT_PTR)pEv->m[BM_LANGUAGE] : "");
    StrCpyN(ref.szName, sizeof(ref.szName), pEv->m[BM_TASKNAME] ? (const char*)(INT_PTR)pEv->m[BM_TASKNAME] : "");
    ref.nPageType = pEv->m[BM_PAGETYPE];
    ref.nColMode = pEv->m[BM_COLMODE];
    ref.nSeq = nSeq;
    EvRelease(pEv);

    if (!nAccept)
    {
        Buf b;
        BufInit(&b);
        BufAddSz(&b, "{\"ok\":false,\"code\":");
        BufAddJsonStr(&b, "rejected");
        BufAddSz(&b, ",\"message\":");
        BufAddJsonStr(&b, ToUtf8(szMsg[0] ? szMsg : Zh("采集程序不接受这个任务。")));
        BufAddCh(&b, '}');
        Reply(s, 500, "Internal Server Error", pReq, b.p == NULL ? "{}" : b.p);
        BufFree(&b);
        return;
    }

    char szAuto[128];
    if (szTaskId[0] == '\0')
    {
        CxSnprintf(szAuto, sizeof(szAuto) - 1, "%d", nSeq);
        szAuto[sizeof(szAuto) - 1] = '\0';
    }
    else StrCpyN(szAuto, sizeof(szAuto), szTaskId);

    StrCpyN(ref.szId, sizeof(ref.szId), szAuto);
    TaskRefPut(&ref);

    Buf b;
    BufInit(&b);
    BufAddCh(&b, '{');
    JsonBool(&b, "ok", 1, FALSE);
    JsonStr(&b, "taskId", ToUtf8(szAuto), TRUE);
    char szName[2048];
    if (szTitle[0] != '\0') StrCpyN(szName, sizeof(szName), szTitle);
    else if (m.asin[0] != '\0') StrCpyN(szName, sizeof(szName), m.asin);
    else StrCpyN(szName, sizeof(szName), szUrl);
    JsonStr(&b, "name", ToUtf8(szName), TRUE);
    JsonStr(&b, "state", nStart ? "queued" : "added", TRUE);
    JsonBool(&b, "started", nStarted, TRUE);
    JsonBool(&b, "paused", 0, TRUE);
    JsonBool(&b, "stopped", 0, TRUE);
    JsonInt(&b, "queueCount", nQueue, TRUE);
    JsonBool(&b, "alreadyQueued", nAlready, TRUE);
    if (szMsg[0] != '\0') JsonStr(&b, "message", ToUtf8(szMsg), TRUE);
    else BufAddSz(&b, ",\"message\":null");
    BufAddCh(&b, '}');
    Reply(s, 200, "OK", pReq, b.p == NULL ? "{}" : b.p);
    BufFree(&b);
}

// 严格验证批量 JSON 的结构与分隔符；限制嵌套深度，拒绝截断正文和非法转义。
static void BatchSpace(const char*& p)
{
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
}

static BOOL BatchValue(const char*& p, INT depth)
{
    BatchSpace(p);
    if (depth > 32 || !*p) return FALSE;
    if (*p == '"')
    {
        ++p;
        while (*p && *p != '"')
        {
            if ((unsigned char)*p < 32) return FALSE;
            if (*p++ == '\\')
            {
                if (*p == 'u')
                {
                    ++p;
                    for (INT i = 0; i < 4; ++i, ++p)
                        if (!((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))) return FALSE;
                }
                else
                {
                    if (!*p || !strchr("\"\\/bfnrt", *p)) return FALSE;
                    ++p;
                }
            }
        }
        if (*p != '"') return FALSE;
        ++p;
        return TRUE;
    }
    if (*p == '{' || *p == '[')
    {
        const BOOL object = *p == '{';
        const char end = object ? '}' : ']';
        ++p; BatchSpace(p);
        if (*p == end) { ++p; return TRUE; }
        for (;;)
        {
            if (object)
            {
                if (*p != '"' || !BatchValue(p, depth + 1)) return FALSE;
                BatchSpace(p);
                if (*p != ':') return FALSE;
                ++p;
            }
            if (!BatchValue(p, depth + 1)) return FALSE;
            BatchSpace(p);
            if (*p == end) { ++p; return TRUE; }
            if (*p != ',') return FALSE;
            ++p;
            BatchSpace(p);
        }
    }
    const char* literals[] = { "true", "false", "null" };
    for (INT i = 0; i < 3; ++i)
    {
        const INT n = StrLen(literals[i]);
        if (strncmp(p, literals[i], n) == 0) { p += n; return TRUE; }
    }
    if (*p == '-') ++p;
    if (*p == '0') ++p;
    else
    {
        if (*p < '1' || *p > '9') return FALSE;
        while (*p >= '0' && *p <= '9') ++p;
    }
    if (*p == '.') { ++p; if (*p < '0' || *p > '9') return FALSE; while (*p >= '0' && *p <= '9') ++p; }
    if (*p == 'e' || *p == 'E')
    {
        ++p; if (*p == '+' || *p == '-') ++p;
        if (*p < '0' || *p > '9') return FALSE;
        while (*p >= '0' && *p <= '9') ++p;
    }
    return TRUE;
}

static void HandleTasks(BR_SOCKET s, const BrReq* pReq)
{
    if (!StrEqI(pReq->szMethod, "POST"))
    {
        ReplyErr(s, 405, pReq, "method_not_allowed", Zh("插入任务只能使用 POST。"));
        return;
    }
    // 先验证整批结构再派发事件，非法参数不会造成前半批已经入库。
    const char* p = pReq->szBody;
    BatchSpace(p);
    BOOL valid = *p == '{' && BatchValue(p, 0);
    BatchSpace(p);
    Buf raw; BufInit(&raw); BufReserve(&raw, BR_MAX_BODY);
    if (!raw.p) { ReplyErr(s, 503, pReq, "out_of_memory", "Out of memory"); return; }
    raw.p[0] = 0;
    valid = valid && !*p && JsonGetRaw(pReq->szBody, "tasks", raw.p, BR_MAX_BODY);
    p = raw.p;
    BatchSpace(p);
    valid = valid && *p == '[';
    const char* starts[100];
    INT lengths[100], total = 0;
    if (valid)
    {
        ++p; BatchSpace(p);
        while (valid && *p != ']')
        {
            if (total == 100 || *p != '{') { valid = FALSE; break; }
            starts[total] = p;
            valid = BatchValue(p, 0);
            lengths[total] = (INT)(p - starts[total]);
            if (lengths[total] + 16 >= BR_MAX_BODY) valid = FALSE;
            ++total; BatchSpace(p);
            if (*p == ']') break;
            if (*p++ != ',') { valid = FALSE; break; }
            BatchSpace(p);
        }
        valid = valid && total > 0 && *p == ']';
    }
    BrReq* item = (BrReq*)malloc(sizeof(BrReq));
    if (!item) { BufFree(&raw); ReplyErr(s, 503, pReq, "out_of_memory", "Out of memory"); return; }
    *item = *pReq;
    char flag[16];
    INT commonStart = 1;
    if (JsonGetRaw(pReq->szBody, "start", flag, sizeof(flag)))
    {
        StrTrim(flag);
        valid = valid && (strcmp(flag, "true") == 0 || strcmp(flag, "false") == 0);
    }
    JsonGetBool(pReq->szBody, "start", &commonStart);
    BOOL unsupportedCountry = FALSE;
    BOOL unidentifiedPage = FALSE;
    for (INT i = 0; valid && i < total; ++i)
    {
        memcpy(item->szBody, starts[i], lengths[i]); item->szBody[lengths[i]] = 0;
        char url[2048]; url[0] = 0;
        char value[4096];
        valid = JsonGetRaw(item->szBody, "url", value, sizeof(value)) && value[0] == '"' &&
            JsonGetString(item->szBody, "url", url, sizeof(url)) && url[0];
        const char* urlStart = url;
        BatchSpace(urlStart);
        valid = valid && *urlStart;
        if (valid)
        {
            BrMatch match;
            MatchUrl(url, &match);
            if (!match.bIdentifiable) unidentifiedPage = TRUE;
            if (match.bIdentifiable && !match.country[0]) unsupportedCountry = TRUE;
        }
        const char* textKeys[] = { "name", "taskName", "title", "asin" };
        for (INT k = 0; valid && k < 4; ++k)
            if (JsonGetRaw(item->szBody, textKeys[k], value, sizeof(value)))
                valid = value[0] == '"' && value[StrLen(value) - 1] == '"';
        if (JsonGetRaw(item->szBody, "start", flag, sizeof(flag)))
        {
            StrTrim(flag);
            valid = valid && (strcmp(flag, "true") == 0 || strcmp(flag, "false") == 0);
        }
    }
    if (!valid)
    {
        free(item); BufFree(&raw);
        ReplyErr(s, 400, pReq, "bad_json", Zh("批量请求必须包含 1 至 100 个任务，每项必须提供网址，start 必须是逻辑值。"));
        return;
    }
    if (unidentifiedPage)
    {
        free(item); BufFree(&raw);
        ReplyErr(s, 400, pReq, "can_not_identify_url", Zh("批量任务中包含无法识别的采集页面，整批未提交。"));
        return;
    }
    if (unsupportedCountry)
    {
        free(item); BufFree(&raw);
        ReplyErr(s, 400, pReq, "country_code_not_supported", Zh("批量任务中包含 AC 不支持的国家代码，整批未提交。"));
        return;
    }
    Buf results; BufInit(&results); BufAddCh(&results, '[');
    INT processed = 0, success = 0;
    for (INT i = 0; i < total; ++i)
    {
        memcpy(item->szBody, starts[i], lengths[i]); item->szBody[lengths[i]] = 0;
        if (!JsonGetRaw(item->szBody, "start", flag, sizeof(flag)))
        {
            item->szBody[lengths[i] - 1] = 0;
            strcat(item->szBody, commonStart ? ",\"start\":true}" : ",\"start\":false}");
        }
        item->nBodyLen = StrLen(item->szBody);
        Buf one; BufInit(&one);
        INT status = 500;
        item->pCapturedReply = &one; item->pCapturedStatus = &status;
        HandleTask(s, item);
        if (i) BufAddCh(&results, ',');
        BufAddF(&results, "{\"index\":%d,\"url\":", i);
        char url[2048]; JsonGetString(item->szBody, "url", url, sizeof(url));
        BufAddJsonStr(&results, url);
        // 合并单条回执，任务编号、状态及拒绝原因均保留原样。
        if (one.p && one.nLen > 2) { BufAddCh(&results, ','); BufAdd(&results, one.p + 1, one.nLen - 2); }
        BufAddCh(&results, '}');
        ++processed;
        BufFree(&one);
        if (status != 200) break;
        ++success;
    }
    BufAddCh(&results, ']');
    Buf response; BufInit(&response);
    BufAddF(&response, "{\"ok\":%s,\"total\":%d,\"processed\":%d,\"successCount\":%d,\"failedCount\":%d,\"remainingCount\":%d,\"results\":",
        success == total ? "true" : "false", total, processed, success, processed - success, total - processed);
    BufAddSz(&response, results.p); BufAddCh(&response, '}');
    Reply(s, 200, "OK", pReq, response.p);
    BufFree(&response); BufFree(&results); BufFree(&raw); free(item);
}

// ============ 任务号 → 页面身份：撤回事件靠它自证撤的是哪一页 ============
#define TASK_REF_SLOTS 256
static BrTaskRef g_taskRef[TASK_REF_SLOTS];
static INT g_nTaskRefNext = 0;

void TaskRefPut(const BrTaskRef* pRef)
{
    if (pRef == NULL || pRef->szId[0] == '\0') return;
    EnterCriticalSection(&g_cs);
    INT nSlot = -1;
    for (INT i = 0; i < TASK_REF_SLOTS; ++i)
    {
        if (g_taskRef[i].szId[0] == '\0') { nSlot = i; break; }
        if (StrEqI(g_taskRef[i].szId, pRef->szId)) { nSlot = i; break; }
    }
    if (nSlot < 0)
    {
        nSlot = g_nTaskRefNext;
        g_nTaskRefNext = (g_nTaskRefNext + 1) % TASK_REF_SLOTS;
    }
    g_taskRef[nSlot] = *pRef;
    g_taskRef[nSlot].dwAt = GetTickCount();
    LeaveCriticalSection(&g_cs);
}

BOOL TaskRefGet(const char* pszId, BrTaskRef* pOut)
{
    BOOL bFound = FALSE;
    if (pszId == NULL || pszId[0] == '\0' || pOut == NULL) return FALSE;
    EnterCriticalSection(&g_cs);
    for (INT i = 0; i < TASK_REF_SLOTS; ++i)
    {
        if (g_taskRef[i].szId[0] != '\0' && StrEqI(g_taskRef[i].szId, pszId))
        {
            *pOut = g_taskRef[i];
            bFound = TRUE;
            break;
        }
    }
    LeaveCriticalSection(&g_cs);
    return bFound;
}

// 撤回任务：插件点「撤回」时打过来，POST /cxa/recall，body {"taskId":"…"}。
// 转成一条 事件类型=4 的事件交给采集程序决定；程序用 应答采集事件 回「是否接受」＝是否撤回成功。
static void HandleRecall(BR_SOCKET s, const BrReq* pReq)
{
    Touch(pReq);
    if (!StrEqI(pReq->szMethod, "POST"))
    {
        ReplyErr(s, 405, pReq, "method_not_allowed", Zh("撤回任务只能使用 POST。"));
        return;
    }
    char szRecallId[128];
    szRecallId[0] = '\0';
    JsonGetString(pReq->szBody, "taskId", szRecallId, sizeof(szRecallId));
    if (szRecallId[0] == '\0')
    {
        ReplyErr(s, 400, pReq, "task_id_required", Zh("没带要撤回的任务编号（提交时回给你的那个 taskId）。"));
        return;
    }

    BrEv* pEv = EvNew(EV_RECALL);
    if (pEv == NULL)
    {
        ReplyErr(s, 500, pReq, "out_of_memory", Zh("内存不足，没建成撤回事件。"));
        return;
    }
    char szTime[32];
    NowText(szTime, sizeof(szTime));
    EvSetText(pEv, BM_ARRIVE, szTime);
    EvSetText(pEv, BM_TASKID, szRecallId);
    // 回填提交那一页的身份：查得到就带网址/站点/页面类型/采集模式/ASIN/任务名；
    // 查不到（程序重启过、号是外来的）就只带号，采集程序照旧按号查自己的表
    BrTaskRef ref;
    if (TaskRefGet(szRecallId, &ref))
    {
        EvSetText(pEv, BM_URL, ref.szUrl);
        EvSetText(pEv, BM_ASIN, ref.szAsin);
        EvSetText(pEv, BM_SITE, ref.szSite);
        EvSetText(pEv, BM_PLATFORM, ref.szPlatform);
        EvSetText(pEv, BM_COUNTRY, ref.szCountry);
        EvSetText(pEv, BM_CURRENCY, ref.szCurrency);
        EvSetText(pEv, BM_LANGUAGE, ref.szLanguage);
        EvSetText(pEv, BM_TASKNAME, ref.szName);
        pEv->m[BM_PAGETYPE] = ref.nPageType;
        pEv->m[BM_COLMODE] = ref.nColMode;
    }
    EvSetText(pEv, BM_RAW, pReq->szBody);
    EvSetText(pEv, BM_EXT_VER, pReq->szExtVer);
    EvSetText(pEv, BM_BROWSER, pReq->szExtBrowser);
    EvSetText(pEv, BM_EXT_ID, pReq->szExtId);

    if (!EvBound())
    {
        BrLogOnce("recall-nobind", "%s", Zh("收到撤回，但采集程序没绑定接收子程序。"));
        ReplyErr(s, 503, pReq, "no_binding", Zh("采集程序当前不接收消息，请稍后重试。"));
        EvRelease(pEv);
        return;
    }

    const BOOL bAnswered = EvPost(pEv, g_cfg.nReplyWaitMs > 0 ? g_cfg.nReplyWaitMs : 15000);
    if (!bAnswered)
    {
        EvRelease(pEv);
        Buf b;
        BufInit(&b);
        BufAddSz(&b, "{\"ok\":false,\"removed\":false,\"code\":");
        BufAddJsonStr(&b, "reply_timeout");
        BufAddSz(&b, ",\"message\":");
        BufAddJsonStr(&b, Zh("采集程序没有及时应答撤回请求：请在程序里调「置回调线程 (0)」让库自动派发，或定时调用「处理插件消息」。"));
        BufAddCh(&b, '}');
        Reply(s, 500, "Internal Server Error", pReq, b.p == NULL ? "{}" : b.p);
        BufFree(&b);
        return;
    }
    // 回执字段先全部抄出来再放手（放手后派发者随时回收）：采集程序在 应答采集事件 里填的
    // 提示消息 / 队列数量 / 是否已开始 就是回执正文，撤回时刻与流水号由库盖上。
    char szMsg[512];
    StrCpyN(szMsg, sizeof(szMsg), pEv->pszMessage == NULL ? "" : pEv->pszMessage);
    const INT nAccept = pEv->bAccept;
    const INT nQueue = pEv->nQueue;
    const INT nStarted = pEv->bStarted;
    const INT nSeq = pEv->nSeq;
    char szAt[32];
    szAt[0] = '\0';
    NowText(szAt, sizeof(szAt));
    EvRelease(pEv);

    Buf b;
    BufInit(&b);
    if (!nAccept)
    {
        BufAddCh(&b, '{');
        JsonBool(&b, "ok", 0, FALSE);
        JsonBool(&b, "removed", 0, TRUE);
        BufAddSz(&b, ",\"code\":");
        BufAddJsonStr(&b, "recall_rejected");
        BufAddSz(&b, ",\"message\":");
        BufAddJsonStr(&b, ToUtf8(szMsg[0] ? szMsg : Zh("采集程序没有撤回这个任务（可能已经采完或找不到了）。")));
        JsonStr(&b, "taskId", szRecallId, TRUE);
        JsonInt(&b, "queueCount", nQueue, TRUE);
        JsonStr(&b, "recalledAt", ToUtf8(szAt), TRUE);
        BufAddCh(&b, '}');
        BrLog("%s %s", Zh("撤回被拒："), szRecallId);
        Reply(s, 500, "Internal Server Error", pReq, b.p == NULL ? "{}" : b.p);
        BufFree(&b);
        return;
    }
    BufAddCh(&b, '{');
    JsonBool(&b, "ok", 1, FALSE);
    JsonBool(&b, "removed", 1, TRUE);
    JsonStr(&b, "taskId", szRecallId, TRUE);
    JsonStr(&b, "message", ToUtf8(szMsg[0] ? szMsg : Zh("采集程序确认已撤回。")), TRUE);
    JsonInt(&b, "queueCount", nQueue, TRUE);          // 采集程序填的剩余队列数
    JsonBool(&b, "started", nStarted, TRUE);          // 撤回时这条任务是不是已经在采
    JsonStr(&b, "recalledAt", ToUtf8(szAt), TRUE);    // 库盖的回执时刻
    JsonInt(&b, "seq", nSeq, TRUE);                   // 本条撤回事件的流水号，对账用
    BufAddCh(&b, '}');
    BrLog("%s %s（回执 %s）", Zh("已撤回任务："), szRecallId, szAt);
    Reply(s, 200, "OK", pReq, b.p == NULL ? "{}" : b.p);
    BufFree(&b);
}

static void HandleDisconnect(BR_SOCKET s, const BrReq* pReq)
{
    Touch(pReq, FALSE);
    DisconnectNotify(s, pReq, Zh("浏览器插件主动断开对接。"));
}

// ===================== 连接处理 =====================

struct BrReq;
static void RouteRequest(BR_SOCKET cs, BrReq* pReq);

// 应答发完先半关闭（对方能读到 FIN 再断），直接 closesocket 有概率把刚发出去的数据带成 RST
static void CloseClient(BR_SOCKET cs)
{
    if (cs == BR_INVALID_SOCKET) return;
    if (g_ws.shutdown != NULL) g_ws.shutdown(cs, 1);
    g_ws.closesocket(cs);
}

static void ServeConnection(BR_SOCKET cs)
{
    if (cs == BR_INVALID_SOCKET) return;
    DWORD dwTo = BR_IO_TIMEOUT;
    g_ws.setsockopt(cs, BR_SOL_SOCKET, BR_SO_RCVTIMEO, (const char*)&dwTo, sizeof(dwTo));
    g_ws.setsockopt(cs, BR_SOL_SOCKET, BR_SO_SNDTIMEO, (const char*)&dwTo, sizeof(dwTo));

    char* pAll = (char*)malloc(BR_MAX_HEAD + BR_MAX_BODY + 16);
    if (pAll == NULL) { CloseClient(cs); return; }
    INT nGot = 0;
    INT nNeed = -1;
    BOOL bHeadDone = FALSE;
    for (INT loop = 0; loop < 512; ++loop)
    {
        const INT n = g_ws.recv(cs, pAll + nGot, BR_MAX_HEAD + BR_MAX_BODY - nGot, 0);
        if (n <= 0) break;
        nGot += n;
        pAll[nGot] = '\0';
        if (!bHeadDone)
        {
            const char* pEnd = StrFindI(pAll, "\r\n\r\n");
            if (pEnd != NULL)
            {
                bHeadDone = TRUE;
                char szLen[32];
                HeaderValue(pAll, "Content-Length", szLen, sizeof(szLen));
                INT nLen = 0;
                for (INT i = 0; szLen[i] != '\0'; ++i)
                {
                    // 手工转整数，避免 atoi 系列在静态库里的差异
                    if (szLen[i] < '0' || szLen[i] > '9') { nLen = 0; break; }
                    nLen = nLen * 10 + (szLen[i] - '0');
                }
                nNeed = (INT)(pEnd + 4 - pAll) + nLen;
                if (nNeed > BR_MAX_HEAD + BR_MAX_BODY) nNeed = -2;
            }
        }
        if (bHeadDone && (nNeed < 0 || nGot >= nNeed)) break;
        if (nNeed == -2) break;
    }
    if (nGot <= 0) { free(pAll); CloseClient(cs); return; }

    // BrReq 里带了正文缓冲，放到堆上：线程栈默认 1M，压两个大数组迟早爆
    BrReq* pReq = (BrReq*)malloc(sizeof(BrReq));
    if (pReq == NULL) { free(pAll); CloseClient(cs); return; }
    ParseRequest(pAll, pReq);
    free(pAll);
    RouteRequest(cs, pReq);
    free(pReq);
    CloseClient(cs);
}

static void RouteRequest(BR_SOCKET cs, BrReq* pReq)
{
    BrReq& req = *pReq;
    const char* pszApi = StrFindI(req.szPath, "/cxa/");
    if (pszApi == NULL)
    {
        ReplyErr(cs, 404, &req, "not_found", Zh("接口不存在。"));
        return;
    }
    const char* pszName = pszApi + 5;
    if (StrEqI(req.szMethod, "OPTIONS"))
    {
        Touch(&req);
        Reply(cs, 200, "OK", &req, "{\"ok\":true}");
        return;
    }
    if (StrEqI(pszName, "handshake"))
    {
        // 换了一个会话串就说明插件重连了一次（比如采集程序重启过）
        HandleHandshake(cs, &req);
    }
    else
    {
        if (!KeyOk(&req))
        {
            ReplyErr(cs, 403, &req, "key_required", Zh("缺少会话标识，请先调用 handshake。"));
            return;
        }
        if (StrEqI(pszName, "sites")) HandleSites(cs, &req);
        else if (StrEqI(pszName, "match")) HandleMatch(cs, &req);
        else if (StrEqI(pszName, "task")) HandleTask(cs, &req);
        else if (StrEqI(pszName, "tasks")) HandleTasks(cs, &req);
        else if (StrEqI(pszName, "recall")) HandleRecall(cs, &req);
        else if (StrEqI(pszName, "disconnect")) HandleDisconnect(cs, &req);
        else ReplyErr(cs, 404, &req, "not_found", Zh("接口不存在。"));
    }
}

static DWORD WINAPI WorkerThread(LPVOID lp)
{
    const BR_SOCKET cs = (BR_SOCKET)(INT_PTR)lp;
    ATOM_INC(&g_nWorker);
    __try
    {
        ServeConnection(cs);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        BrLogOnce("req", "%s", Zh("处理浏览器请求时出错，已断开这一条连接。"));
    }
    ATOM_DEC(&g_nWorker);
    return 0;
}

static DWORD WINAPI AcceptThread(LPVOID)
{
    DWORD dwSleep = 150;
    while (g_run)
    {
        if (g_listen == BR_INVALID_SOCKET)
        {
            if (WaitForSingleObject(g_hStop, dwSleep) != WAIT_TIMEOUT) break;
            continue;
        }
        BR_SOCKET cs = g_ws.accept(g_listen, NULL, NULL);
        if (cs == BR_INVALID_SOCKET)
        {
            const INT nErr = g_ws.LastError ? g_ws.LastError() : 0;
            if (nErr != BR_WSAEWOULDBLOCK)
            {
                // 监听句柄失效（端口被抢或进程收尾），交给定时器重新绑定
                EnterCriticalSection(&g_cs);
                g_stat.listening = 0;
                g_stat.port = 0;
                LeaveCriticalSection(&g_cs);
                g_ws.closesocket(g_listen);
                g_listen = BR_INVALID_SOCKET;
                BrLogOnce("acc", "%s (%d)", Zh("监听中断，稍后自动重试"), nErr);
            }
            else if (WaitForSingleObject(g_hStop, dwSleep) != WAIT_TIMEOUT) break;
            continue;
        }
        if (!g_run) { CloseClient(cs); break; }
        HANDLE h = CreateThread(NULL, 0, WorkerThread, (LPVOID)(INT_PTR)cs, 0, NULL);
        if (h == NULL) ServeConnection(cs);
        else CloseHandle(h);
    }
    return 0;
}

static DWORD WINAPI TickThread(LPVOID)
{
    while (g_run)
    {
        if (WaitForSingleObject(g_hStop, 1000) != WAIT_TIMEOUT) break;
        __try
        {
            SrvTick();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
    return 0;
}

// ===================== 启停 =====================

BOOL SrvTryBind()
{
    if (!WsLoad()) return FALSE;
    if (!g_wsaReady)
    {
        char szData[512];
        if (g_ws.Startup(0x0202, szData) != 0)
        {
            BrSetError("%s", Zh("本机网络组件初始化失败，无法监听浏览器插件。"));
            return FALSE;
        }
        g_wsaReady = 1;
    }
    INT nFirst = g_cfg.nPortFirst > 0 ? g_cfg.nPortFirst : 47630;
    INT nCount = g_cfg.nPortCount > 0 ? g_cfg.nPortCount : 4;
    for (INT i = 0; i < nCount; ++i)
    {
        const INT nPort = nFirst + i;
        const BR_SOCKET s = g_ws.socket(BR_AF_INET, BR_SOCK_STREAM, BR_IPPROTO_TCP);
        if (s == BR_INVALID_SOCKET) continue;
        // 绝对不要 SO_REUSEADDR：Windows 上它不是 Linux 那种"只放 TIME_WAIT"，
        // 而是允许后绑的进程直接抢走同一端口的连接。两个采集程序（或一个残留的测试进程）
        // 会同时"绑定成功"，插件的请求被抢走的那个收到，另一个怎么点都没反应。
        BR_SOCKADDR_IN addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = BR_AF_INET;
        addr.sin_port = BrHtons((unsigned short)nPort);
        addr.sin_addr = BrHtonl(BR_INADDR_LOOPBACK);   // 网络字节序：只写 0x7f000001 会绑到 1.0.0.127
        if (g_ws.bind(s, (const void*)&addr, sizeof(addr)) != BR_SOCKET_ERROR &&
            g_ws.listen(s, 8) != BR_SOCKET_ERROR)
        {
            unsigned long nBlock = 1;
            g_ws.ioctlsocket(s, (long)BR_FIONBIO, &nBlock);
            if (g_listen != BR_INVALID_SOCKET) g_ws.closesocket(g_listen);
            g_listen = s;
            EnterCriticalSection(&g_cs);
            g_stat.listening = 1;
            g_stat.port = nPort;
            KeyMake(g_stat.szKey);
            g_stat.dwLastReq = GetTickCount();
            LeaveCriticalSection(&g_cs);
            // 会话串换了 = 这是一份新的服务身份，之前那些浏览器都算不上了：清空在线表，
            // 等它们各自重新握手回来（不清就会留着几条永远超不了时的"幽灵台数"）
            SessClearAll();
            BrSetError("");
            BrLog(Zh("%s：%d（会话标识已更换）"), Zh("开始监听本机端口"), nPort);
            return TRUE;
        }
        g_ws.closesocket(s);
    }
    BrSetError("%s", Zh("本机端口都被占用，浏览器对接暂时连不上，稍后会自动重试。"));
    return FALSE;
}

BOOL SrvStart()
{
    BrInitOnce();
    const BOOL bBound = SrvTryBind();
    if (g_run) return bBound;
    g_hStop = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (g_hStop == NULL) return FALSE;
    g_run = 1;
    g_hAccept = CreateThread(NULL, 0, AcceptThread, NULL, 0, NULL);
    g_hTick = CreateThread(NULL, 0, TickThread, NULL, 0, NULL);
    if (g_hAccept == NULL || g_hTick == NULL)
    {
        BrLogOnce("start", "%s", Zh("对接服务的后台线程启动失败。"));
    }
    return bBound;
}

void SrvStop()
{
    if (!g_run)
    {
        SessClearAll();
        return;
    }
    g_run = 0;
    if (g_hStop != NULL) SetEvent(g_hStop);
    if (g_hAccept != NULL)
    {
        WaitForSingleObject(g_hAccept, 3000);
        CloseHandle(g_hAccept);
        g_hAccept = NULL;
    }
    if (g_hTick != NULL)
    {
        WaitForSingleObject(g_hTick, 3000);
        CloseHandle(g_hTick);
        g_hTick = NULL;
    }
    if (g_listen != BR_INVALID_SOCKET)
    {
        g_ws.closesocket(g_listen);
        g_listen = BR_INVALID_SOCKET;
    }
    if (g_hStop != NULL)
    {
        CloseHandle(g_hStop);
        g_hStop = NULL;
    }
    EnterCriticalSection(&g_cs);
    g_stat.listening = 0;
    g_stat.port = 0;
    g_stat.szKey[0] = '\0';
    LeaveCriticalSection(&g_cs);
    SessClearAll();      // 停服务不派发断开事件（老口径一致），但在线台数要归零
    EvClearAll();
    BrLog("%s", Zh("浏览器对接服务已停止。"));
}

void SrvTick()
{
    BrInitOnce();
    if (!g_stat.listening)
    {
        // 端口没绑上就一直定时重试，采集程序不用管
        if (GetTickCount() >= g_stat.dwBindRetryAt)
        {
            g_stat.dwBindRetryAt = GetTickCount() + BR_RETRY_MS;
            if (SrvTryBind()) BrLog("%s", Zh("已自动恢复监听。"));
        }
        return;
    }
    EnterCriticalSection(&g_cs);
    const DWORD dwTimeout = (DWORD)(g_cfg.nOfflineSec * 1000UL);
    LeaveCriticalSection(&g_cs);
    // 每台浏览器各自按自己的心跳判超时：掉一台发一条断开，最后一台掉了才发「所有浏览器已断开」
    for (INT i = 0; i < BR_MAX_SESSIONS; ++i)
    {
        BrSess gone;
        INT nLeft = 0;
        if (!SessTakeExpired(dwTimeout, &gone, &nLeft)) break;
        SessFireGone(&gone, nLeft, Zh("浏览器插件超过设定时间没有联系，已按断开处理。"));
    }
}

}  // namespace cx
