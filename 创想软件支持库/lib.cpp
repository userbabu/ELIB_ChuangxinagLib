// 创想软件支持库 —— 原生 C++ 支持库（.fne），32 位、静态 CRT、GBK 输出。
// 源码 UTF-8 保存，由 /source-charset:utf-8 /execution-charset:gbk 转出 GBK 产物。
// 编译与自动落库见 build.bat；维护流程见项目 skill epl-support-lib。
//
// 网络部分封装 libcurl-impersonate（可伪装浏览器 TLS/HTTP2 指纹），运行时 LoadLibrary 加载，
// .fne 不产生导入依赖。对象模型：浏览器会话（配置+连接）→ 浏览器请求（单次调用参数）
// → 浏览器响应（结果）。文本一律按 UTF-8 原样进出，不做 GBK 转换。

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

// lib2.h 用到 DATE / DOUBLE / FLOAT / INT64；SDK 的 mtypes.h 与 windows.h 冲突，这里补齐。
typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;

#include <lib2.h>
#include <lang.h>
// ==== JSON2 头文件（gen_json2_table.mjs 生成，勿手改）====
#include <cx_json.h>
// ==== JSON2 头文件 结束 ====
#include <cx_uni.h>

#define CX_LIB_NAME _T("创想软件支持库")
#define CX_NOTIFY_LIB_FUNC_NAME "cxsoft_ProcessNotifyLib"
#define CX_CURL_DLL_NAME "libcurl-impersonate.dll"

#define CX_VER_MAJOR 3
#define CX_VER_MINOR 21  // 只读入参一律取消强制变量
#define CX_VER_BUILD 0

#define CX_TYPE_SESSION 1          // m_pDataType 里的序号，从 1 开始
#define CX_TYPE_REQUEST 2
#define CX_TYPE_RESPONSE 3
#define CX_TYPE_CERT_INFO 10       // CURL_证书信息（纯数据类型，无成员命令）
#define CX_TYPE_CURL_EASY 11       // curl_easy
#define CX_TYPE_CURL_HTTP 12       // curl_http
#define CX_TYPE_CURL_SMTP 13       // curl_smtp
#define CX_TYPE_CURL_MULTI 14      // curl_multi
#define CX_TYPE_URL_PARAM 15       // URL参数类
#define CX_TYPE_CURL_MSG 20        // CURL_消息（纯数据类型，无成员命令；排在访问类之后）
#define CX_TYPE_FORM_DATA 19         // formData（纯数据类型）
#define CX_TYPE_LOCK 21          // 线程锁
#define CX_TYPE_SEM 22           // 信号量
#define CX_TYPE_COND 23          // 条件变量
#define CX_TYPE_CANCEL 25        // 共享取消与暂停令牌
#define CX_TYPE_JOIN 24          // 到齐后仅最后一个线程继续
#define CX_TYPE_ACC_CURL 16        // HTTP访问_Curl访问类
#define CX_TYPE_ACC_WINHTTP 17     // HTTP访问_WinHTTP访问类
#define CX_TYPE_ACC_WININET 18     // HTTP访问_WinInet访问类

// 命令实现放在带名命名空间里：静态编译时文件末尾会为每个命令生成 extern "C" 转发，
// 让 static_lib 里能看到与 g_commandFuncNames 同名的符号（匿名命名空间是内部链接，进不了 .lib）。
namespace cx {

// ============================================================ 易语言系统接口

// ASCII 大小写无关比较：不用 _stricmp，它在新 CRT 里引出 __stricmp，VC6 静态链接取不到。
static int CxAsciiCmpI(LPCSTR a, LPCSTR b)
{
    while (*a != '\0' && *b != '\0')
    {
        char x = (*a >= 'A' && *a <= 'Z') ? (char)(*a + 32) : *a;
        char y = (*b >= 'A' && *b <= 'Z') ? (char)(*b + 32) : *b;
        if (x != y) return (int)(unsigned char)x - (int)(unsigned char)y;
        a++; b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

PFN_NOTIFY_SYS g_pfnNotifySys = NULL;

INT NotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    if (g_pfnNotifySys == NULL) return 0;
    return g_pfnNotifySys(nMsg, dwParam1, dwParam2);
}

void ReportError(LPCSTR szErrText)
{
    NotifySys(NRS_RUNTIME_ERR, (DWORD)szErrText, 0);
}

// 文本/字节集都必须落在易语言内存池上，返回后由系统释放。
char* CloneTextData(LPCSTR pszText, INT nLen)
{
    if (pszText == NULL || nLen < 0 || g_pfnNotifySys == NULL) return NULL;
    char* pd = (char*)NotifySys(NRS_MALLOC, (DWORD)(nLen + 1), 0);
    if (pd == NULL) return NULL;
    if (nLen > 0) memcpy(pd, pszText, nLen);
    pd[nLen] = '\0';
    return pd;
}

char* CloneTextData(LPCSTR pszText)
{
    if (pszText == NULL) return NULL;
    return CloneTextData(pszText, (INT)lstrlenA(pszText));
}

LPBYTE CloneBinData(LPCSTR pData, INT nDataSize)
{
    if (pData == NULL || nDataSize <= 0 || g_pfnNotifySys == NULL) return NULL;
    LPBYTE pd = (LPBYTE)NotifySys(NRS_MALLOC, (DWORD)(sizeof(INT) * 2 + nDataSize), 0);
    if (pd == NULL) return NULL;
    *(LPINT)pd = 1;
    *(LPINT)(pd + sizeof(INT)) = nDataSize;
    memcpy(pd + sizeof(INT) * 2, pData, nDataSize);
    return pd;
}

void SetRetText(PMDATA_INF pRet, LPCSTR szUtf8OrNull)
{
    pRet->m_dtDataType = SDT_TEXT;
    pRet->m_pText = CloneTextData(szUtf8OrNull == NULL ? "" : szUtf8OrNull);
}

// 只在本库内部使用的动态字节缓冲。
struct CxBuf
{
    char* pData;
    int nLen;
    int nCap;
};

void BufReset(CxBuf* b) { if (b && b->pData) { b->pData[0] = '\0'; b->nLen = 0; } }

void BufInit(CxBuf* b, int nCap)
{
    b->nCap = nCap > 64 ? nCap : 64;
    b->nLen = 0;
    b->pData = (char*)LocalAlloc(LPTR, b->nCap);
    if (b->pData != NULL) b->pData[0] = '\0';
}

void BufFree(CxBuf* b) { if (b && b->pData) { LocalFree(b->pData); b->pData = NULL; b->nLen = b->nCap = 0; } }

void BufAppend(CxBuf* b, const char* pData, int nAdd)
{
    if (b == NULL || b->pData == NULL || pData == NULL || nAdd <= 0) return;
    const int need = b->nLen + nAdd + 1;
    if (need > b->nCap)
    {
        int nCap = b->nCap;
        while (nCap < need) nCap *= 2;
        char* pNew = (char*)LocalAlloc(LPTR, nCap);
        if (pNew == NULL) return;
        memcpy(pNew, b->pData, b->nLen);
        LocalFree(b->pData);
        b->pData = pNew;
        b->nCap = nCap;
    }
    memcpy(b->pData + b->nLen, pData, nAdd);
    b->nLen += nAdd;
    b->pData[b->nLen] = '\0';
}

void BufCopy(CxBuf* pDst, const CxBuf* pSrc)
{
    BufReset(pDst);
    if (pSrc != NULL && pSrc->nLen > 0) BufAppend(pDst, pSrc->pData, pSrc->nLen);
}

// ============================================================ curl 绑定

typedef void* CX_CURL;
typedef void* CX_MIME;
typedef void* CX_MIMEPART;
struct CxSlist { char* pData; CxSlist* pNext; };

typedef CX_CURL (*fn_easy_init_t)(void);
typedef void (*fn_easy_cleanup_t)(CX_CURL);
typedef void (*fn_easy_reset_t)(CX_CURL);
typedef int (*fn_easy_setopt_t)(CX_CURL, int, ...);
typedef int (*fn_easy_perform_t)(CX_CURL);
typedef int (*fn_easy_getinfo_t)(CX_CURL, int, ...);
typedef const char* (*fn_easy_strerror_t)(int);
typedef int (*fn_impersonate_t)(CX_CURL, const char*, char);
typedef const char* (*fn_version_t)(void);
typedef CxSlist* (*fn_slist_append_t)(CxSlist*, const char*);
typedef void (*fn_slist_free_all_t)(CxSlist*);
typedef CX_MIME (*fn_mime_init_t)(CX_CURL);
typedef CX_MIMEPART (*fn_mime_addpart_t)(CX_MIME);
typedef int (*fn_mime_name_t)(CX_MIMEPART, const char*);
typedef int (*fn_mime_filename_t)(CX_MIMEPART, const char*);
typedef int (*fn_mime_type_t)(CX_MIMEPART, const char*);
typedef int (*fn_mime_filedata_t)(CX_MIMEPART, const char*);
typedef int (*fn_mime_data_t)(CX_MIMEPART, const char*, size_t);
typedef void (*fn_mime_free_t)(CX_MIME);

// 数值全部核对自 third_party\include\curl\curl.h：
// OBJECTPOINT=10000 FUNCTIONPOINT=20000 LONG=0；CURLINFO_STRING=0x100000 LONG=0x200000
// DOUBLE=0x300000 SLIST=0x400000。抄错不会报错，只会静默设到别的选项上。
enum {
    OPT_TIMEOUT = 13,
    OPT_CONNECTTIMEOUT = 78,
    OPT_LOW_SPEED_LIMIT = 19,
    OPT_LOW_SPEED_TIME = 20,
    OPT_FOLLOWLOCATION = 52,
    OPT_MAXREDIRS = 68,
    OPT_NOBODY = 44,
    OPT_RESUME_FROM = 21,
    OPT_UPLOAD = 46,
    OPT_HTTPAUTH = 107,
    OPT_SSL_VERIFYPEER = 64,
    OPT_SSL_VERIFYHOST = 81,
    OPT_VERBOSE = 41,
    OPT_URL = 10002,
    OPT_PROXY = 10004,
    OPT_PROXYUSERPWD = 10006,
    OPT_RANGE = 10007,
    OPT_WRITEFUNCTION = 20011,
    OPT_READFUNCTION = 20012,
    OPT_WRITEDATA = 10001,
    OPT_POSTFIELDS = 10015,
    OPT_REFERER = 10016,
    OPT_USERAGENT = 10018,
    OPT_COOKIE = 10022,
    OPT_HTTPHEADER = 10023,
    OPT_COOKIEFILE = 10031,
    OPT_CUSTOMREQUEST = 10036,
    OPT_COOKIEJAR = 10082,
    OPT_HEADERFUNCTION = 20079,
    OPT_HEADERDATA = 10029,
    OPT_ACCEPT_ENCODING = 10102,
    OPT_CAINFO = 10065,
    OPT_CAPATH = 10097,
    OPT_USERNAME = 10173,
    OPT_PASSWORD = 10174,
    OPT_DEBUGFUNCTION = 20094,
    OPT_DEBUGDATA = 10095,
    OPT_POSTFIELDSIZE = 60,
    OPT_MIMEPOST = 10272,
    INFO_EFFECTIVE_URL = 0x100001,
    INFO_RESPONSE_CODE = 0x200002,
    INFO_CONTENT_TYPE = 0x100012,
    INFO_REDIRECT_COUNT = 0x200014,
    INFO_COOKIELIST = 0x40001C,
    INFO_SIZE_DOWNLOAD = 0x300004,
    INFO_TOTAL_TIME = 0x300002,
    INFO_OS_ERRNO = 0x200019,
};

struct CurlApi
{
    HMODULE hModule;
    bool Ready;
    fn_easy_init_t EasyInit;
    fn_easy_cleanup_t EasyCleanup;
    fn_easy_reset_t EasyReset;
    fn_easy_setopt_t EasySetopt;
    fn_easy_perform_t EasyPerform;
    fn_easy_getinfo_t EasyGetinfo;
    fn_easy_strerror_t EasyStrerror;
    fn_impersonate_t Impersonate;
    fn_version_t Version;
    fn_slist_append_t SlistAppend;
    fn_slist_free_all_t SlistFreeAll;
    fn_mime_init_t MimeInit;
    fn_mime_addpart_t MimeAddPart;
    fn_mime_name_t MimeName;
    fn_mime_filename_t MimeFilename;
    fn_mime_type_t MimeType;
    fn_mime_filedata_t MimeFileData;
    fn_mime_data_t MimeData;
    fn_mime_free_t MimeFree;
    char LoadError[192];
};

CurlApi g_curl = { 0 };
CRITICAL_SECTION g_curlLoadCs;
volatile LONG g_csReady = 0;

bool FileExistsA(LPCSTR p) { return p != NULL && GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES; }

bool LoadCurlApi()
{
    if (InterlockedCompareExchange(&g_csReady, 1, 0) == 0)
    {
        InitializeCriticalSection(&g_curlLoadCs);
        InterlockedExchange(&g_csReady, 2);
    }
    else while (InterlockedCompareExchange(&g_csReady, 0, 0) != 2) Sleep(0);
    EnterCriticalSection(&g_curlLoadCs);
    if (g_curl.Ready) { LeaveCriticalSection(&g_curlLoadCs); return true; }
    if (g_curl.hModule != NULL) { LeaveCriticalSection(&g_curlLoadCs); return g_curl.Ready; }

    char path[MAX_PATH] = { 0 };
    char dir[MAX_PATH];
    HMODULE hSelf = GetModuleHandleA("cxsoft.fne");
    if (hSelf != NULL && GetModuleFileNameA(hSelf, dir, MAX_PATH) > 0)
    {
        char* p = strrchr(dir, '\\');
        if (p != NULL) { *p = '\0'; wsprintfA(path, "%s\\%s", dir, CX_CURL_DLL_NAME); }
    }
    if (!FileExistsA(path) && GetModuleFileNameA(NULL, dir, MAX_PATH) > 0)
    {
        char* p = strrchr(dir, '\\');
        if (p != NULL) { *p = '\0'; wsprintfA(path, "%s\\%s", dir, CX_CURL_DLL_NAME); }
    }

    HMODULE h = FileExistsA(path) ? LoadLibraryA(path) : NULL;
    if (h == NULL) h = LoadLibraryA(CX_CURL_DLL_NAME);
    if (h == NULL)
    {
        wsprintfA(g_curl.LoadError, "加载 %s 失败（错误码 %lu）：必须是 32 位版本，并与本支持库同目录",
            CX_CURL_DLL_NAME, GetLastError());
        LeaveCriticalSection(&g_curlLoadCs);
        return false;
    }

    g_curl.hModule = h;
    g_curl.EasyInit = (fn_easy_init_t)GetProcAddress(h, "curl_easy_init");
    g_curl.EasyCleanup = (fn_easy_cleanup_t)GetProcAddress(h, "curl_easy_cleanup");
    g_curl.EasyReset = (fn_easy_reset_t)GetProcAddress(h, "curl_easy_reset");
    g_curl.EasySetopt = (fn_easy_setopt_t)GetProcAddress(h, "curl_easy_setopt");
    g_curl.EasyPerform = (fn_easy_perform_t)GetProcAddress(h, "curl_easy_perform");
    g_curl.EasyGetinfo = (fn_easy_getinfo_t)GetProcAddress(h, "curl_easy_getinfo");
    g_curl.EasyStrerror = (fn_easy_strerror_t)GetProcAddress(h, "curl_easy_strerror");
    g_curl.Impersonate = (fn_impersonate_t)GetProcAddress(h, "curl_easy_impersonate");
    g_curl.Version = (fn_version_t)GetProcAddress(h, "curl_version");
    g_curl.SlistAppend = (fn_slist_append_t)GetProcAddress(h, "curl_slist_append");
    g_curl.SlistFreeAll = (fn_slist_free_all_t)GetProcAddress(h, "curl_slist_free_all");
    g_curl.MimeInit = (fn_mime_init_t)GetProcAddress(h, "curl_mime_init");
    g_curl.MimeAddPart = (fn_mime_addpart_t)GetProcAddress(h, "curl_mime_addpart");
    g_curl.MimeName = (fn_mime_name_t)GetProcAddress(h, "curl_mime_name");
    g_curl.MimeFilename = (fn_mime_filename_t)GetProcAddress(h, "curl_mime_filename");
    g_curl.MimeType = (fn_mime_type_t)GetProcAddress(h, "curl_mime_type");
    g_curl.MimeFileData = (fn_mime_filedata_t)GetProcAddress(h, "curl_mime_filedata");
    g_curl.MimeData = (fn_mime_data_t)GetProcAddress(h, "curl_mime_data");
    g_curl.MimeFree = (fn_mime_free_t)GetProcAddress(h, "curl_mime_free");

    if (g_curl.EasyInit == NULL || g_curl.EasySetopt == NULL || g_curl.EasyPerform == NULL ||
        g_curl.Impersonate == NULL || g_curl.EasyGetinfo == NULL || g_curl.EasyCleanup == NULL)
    {
        lstrcpyA(g_curl.LoadError, CX_CURL_DLL_NAME " 缺少必需的 curl 导出函数");
        LeaveCriticalSection(&g_curlLoadCs);
        return false;
    }
    g_curl.Ready = true;
    LeaveCriticalSection(&g_curlLoadCs);
    return true;
}

// ============================================================ 对象结构

#define CX_MAX_HEADERS 64
#define CX_MAX_FILES 8

struct CxHeader
{
    char szName[64];
    char szValue[2048];
    bool bUsed;
};

struct CxFilePart
{
    char szField[64];
    char szFile[MAX_PATH];
    char szType[64];
    bool bUsed;
};

struct CxSession
{
    CRITICAL_SECTION cs;
    CX_CURL hEasy;               // 常驻句柄：同会话内连接复用
    char szProfile[64];
    char szAppliedProfile[64];   // 句柄当前已应用的型号
    char szProxy[256];
    char szProxyUserPwd[256];
    char szUserName[128];
    char szPassword[128];
    char szUserAgent[256];
    char szCookieFile[260];
    char szCookieText[4096];
    char szCAInfo[260];
    char szCAPath[260];
    CxHeader headers[CX_MAX_HEADERS];
    int nTimeout;
    int nConnectTimeout;
    int nLowSpeedLimit;
    int nLowSpeedTime;
    int nMaxRedirects;
    int nVerifyPeer;             // -1 = 不主动设置，沿用型号自带
    int nVerifyHost;
    int nAuth;
    bool bFollowRedirect;
    bool bDebug;
    CxBuf debugLog;
    int nErrCode;
    char szErrText[256];
};

struct CxRequest
{
    char szUrl[2048];
    char szMethod[16];
    CxHeader headers[CX_MAX_HEADERS];
    CxBuf body;
    CxFilePart files[CX_MAX_FILES];
    char szOutFile[260];
    int nResumeFrom;
    bool bPost;                  // 有正文/文件即视为提交
};

struct CxResponse
{
    CxBuf body;
    CxBuf headRaw;
    CxHeader headers[CX_MAX_HEADERS];
    char* pFile;                 // 下载落盘用的路径副本
    FILE* pOutFp;
    long nStatus;
    INT64 nReceived;             // 自计数字节，落盘模式也准确
    int nErrCode;
    char szErrText[256];
    char szFinalUrl[2048];
    char szContentType[128];
    int nRedirectCount;
    int nElapsedMs;
};

void HeadersSet(CxHeader* p, LPCSTR szName, LPCSTR szValue)
{
    int nFree = -1;
    for (int i = 0; i < CX_MAX_HEADERS; i++)
    {
        if (p[i].bUsed && CxAsciiCmpI(p[i].szName, szName) == 0)
        {
            lstrcpynA(p[i].szValue, szValue, sizeof(p[i].szValue));
            return;
        }
        if (!p[i].bUsed && nFree < 0) nFree = i;
    }
    if (nFree < 0) { ReportError("请求头数量已达上限 64"); return; }
    p[nFree].bUsed = true;
    lstrcpynA(p[nFree].szName, szName, sizeof(p[nFree].szName));
    lstrcpynA(p[nFree].szValue, szValue, sizeof(p[nFree].szValue));
}

void HeadersDel(CxHeader* p, LPCSTR szName)
{
    for (int i = 0; i < CX_MAX_HEADERS; i++)
        if (p[i].bUsed && CxAsciiCmpI(p[i].szName, szName) == 0) { p[i].bUsed = false; return; }
}

void HeadersClear(CxHeader* p) { memset(p, 0, sizeof(CxHeader) * CX_MAX_HEADERS); }

void HeadersCopy(CxHeader* pDst, const CxHeader* pSrc)
{
    HeadersClear(pDst);
    for (int i = 0; i < CX_MAX_HEADERS; i++)
        if (pSrc[i].bUsed) HeadersSet(pDst, pSrc[i].szName, pSrc[i].szValue);
}

// ============================================================ 会话

CxSession* NewSession()
{
    CxSession* p = (CxSession*)LocalAlloc(LPTR, sizeof(CxSession));
    if (p == NULL) return NULL;
    InitializeCriticalSection(&p->cs);
    lstrcpynA(p->szProfile, "chrome133a", sizeof(p->szProfile));
    p->nTimeout = 30;
    p->nConnectTimeout = 10;
    p->nMaxRedirects = 10;
    p->nVerifyPeer = 1;
    p->nVerifyHost = 2;
    p->nAuth = 1 | 2 | 4 | 8 | 16;   // basic/digest/ntlm/negotiate/any，由 curl 协商
    p->bFollowRedirect = true;
    BufInit(&p->debugLog, 4096);
    return p;
}

void FreeSession(CxSession* p)
{
    if (p == NULL) return;
    if (p->hEasy != NULL && g_curl.EasyCleanup != NULL) g_curl.EasyCleanup(p->hEasy);
    BufFree(&p->debugLog);
    DeleteCriticalSection(&p->cs);
    LocalFree(p);
}

void CopySession(CxSession* pDst, const CxSession* pSrc)
{
    if (pDst == NULL || pSrc == NULL) return;
    pDst->hEasy = NULL;                       // 连接不复用，交给对方自己新建
    pDst->szAppliedProfile[0] = '\0';
    lstrcpynA(pDst->szProfile, pSrc->szProfile, sizeof(pDst->szProfile));
    lstrcpynA(pDst->szProxy, pSrc->szProxy, sizeof(pDst->szProxy));
    lstrcpynA(pDst->szProxyUserPwd, pSrc->szProxyUserPwd, sizeof(pDst->szProxyUserPwd));
    lstrcpynA(pDst->szUserName, pSrc->szUserName, sizeof(pDst->szUserName));
    lstrcpynA(pDst->szPassword, pSrc->szPassword, sizeof(pDst->szPassword));
    lstrcpynA(pDst->szUserAgent, pSrc->szUserAgent, sizeof(pDst->szUserAgent));
    lstrcpynA(pDst->szCookieFile, pSrc->szCookieFile, sizeof(pDst->szCookieFile));
    lstrcpynA(pDst->szCookieText, pSrc->szCookieText, sizeof(pDst->szCookieText));
    lstrcpynA(pDst->szCAInfo, pSrc->szCAInfo, sizeof(pDst->szCAInfo));
    lstrcpynA(pDst->szCAPath, pSrc->szCAPath, sizeof(pDst->szCAPath));
    HeadersCopy(pDst->headers, pSrc->headers);
    pDst->nTimeout = pSrc->nTimeout;
    pDst->nConnectTimeout = pSrc->nConnectTimeout;
    pDst->nLowSpeedLimit = pSrc->nLowSpeedLimit;
    pDst->nLowSpeedTime = pSrc->nLowSpeedTime;
    pDst->nMaxRedirects = pSrc->nMaxRedirects;
    pDst->nVerifyPeer = pSrc->nVerifyPeer;
    pDst->nVerifyHost = pSrc->nVerifyHost;
    pDst->nAuth = pSrc->nAuth;
    pDst->bFollowRedirect = pSrc->bFollowRedirect;
    pDst->bDebug = pSrc->bDebug;
}

CxRequest* NewRequest()
{
    CxRequest* p = (CxRequest*)LocalAlloc(LPTR, sizeof(CxRequest));
    if (p == NULL) return NULL;
    BufInit(&p->body, 4096);
    return p;
}

void FreeRequest(CxRequest* p)
{
    if (p == NULL) return;
    BufFree(&p->body);
    LocalFree(p);
}

void CopyRequest(CxRequest* pDst, const CxRequest* pSrc)
{
    if (pDst == NULL || pSrc == NULL) return;
    lstrcpynA(pDst->szUrl, pSrc->szUrl, sizeof(pDst->szUrl));
    lstrcpynA(pDst->szMethod, pSrc->szMethod, sizeof(pDst->szMethod));
    lstrcpynA(pDst->szOutFile, pSrc->szOutFile, sizeof(pDst->szOutFile));
    HeadersCopy(pDst->headers, pSrc->headers);
    BufCopy(&pDst->body, &pSrc->body);
    pDst->nResumeFrom = pSrc->nResumeFrom;
    pDst->bPost = pSrc->bPost;
    for (int i = 0; i < CX_MAX_FILES; i++) pDst->files[i] = pSrc->files[i];
}

CxResponse* NewResponse()
{
    CxResponse* p = (CxResponse*)LocalAlloc(LPTR, sizeof(CxResponse));
    if (p == NULL) return NULL;
    BufInit(&p->body, 1 << 15);
    BufInit(&p->headRaw, 4096);
    return p;
}

void FreeResponse(CxResponse* p)
{
    if (p == NULL) return;
    if (p->pOutFp != NULL) fclose(p->pOutFp);
    BufFree(&p->body);
    BufFree(&p->headRaw);
    LocalFree(p);
}

void CopyResponse(CxResponse* pDst, const CxResponse* pSrc)
{
    if (pDst == NULL || pSrc == NULL) return;
    BufCopy(&pDst->body, &pSrc->body);
    BufCopy(&pDst->headRaw, &pSrc->headRaw);
    HeadersCopy(pDst->headers, pSrc->headers);
    pDst->nStatus = pSrc->nStatus;
    pDst->nErrCode = pSrc->nErrCode;
    lstrcpynA(pDst->szErrText, pSrc->szErrText, sizeof(pDst->szErrText));
    lstrcpynA(pDst->szFinalUrl, pSrc->szFinalUrl, sizeof(pDst->szFinalUrl));
    lstrcpynA(pDst->szContentType, pSrc->szContentType, sizeof(pDst->szContentType));
    pDst->nRedirectCount = pSrc->nRedirectCount;
    pDst->nElapsedMs = pSrc->nElapsedMs;
    pDst->nReceived = pSrc->nReceived;
}

void ResponseReset(CxResponse* p)
{
    BufReset(&p->body);
    BufReset(&p->headRaw);
    HeadersClear(p->headers);
    p->nStatus = 0;
    p->nErrCode = 0;
    p->szErrText[0] = '\0';
    p->szFinalUrl[0] = '\0';
    p->szContentType[0] = '\0';
    p->nRedirectCount = 0;
    p->nElapsedMs = 0;
    p->nReceived = 0;
    if (p->pOutFp != NULL) { fclose(p->pOutFp); p->pOutFp = NULL; }
}

// ============================================================ 回调

size_t __cdecl WriteCb(char* ptr, size_t size, size_t n, void* user)
{
    CxResponse* p = (CxResponse*)user;
    const size_t bytes = size * n;
    if (p == NULL) return 0;
    if (p->pOutFp != NULL) return fwrite(ptr, size, n, p->pOutFp);
    BufAppend(&p->body, ptr, (int)bytes);
    return bytes;
}

size_t __cdecl HeaderCb(char* ptr, size_t size, size_t n, void* user)
{
    CxResponse* p = (CxResponse*)user;
    const size_t bytes = size * n;
    if (p == NULL) return 0;
    // 走 http 代理时 CONNECT 的应答不属于目标站，丢掉。
    if (bytes >= 23 && strncmp(ptr, "HTTP/1.1 200 Connection", 23) == 0) return bytes;
    // 新的一个响应块（跳转/100 继续）：清掉上一轮的解析结果，只留最后一次。
    if (bytes >= 5 && strncmp(ptr, "HTTP/", 5) == 0)
    {
        HeadersClear(p->headers);
        BufReset(&p->headRaw);
    }
    BufAppend(&p->headRaw, ptr, (int)bytes);

    // 解析 name: value
    for (size_t i = 0; i + 1 < bytes; i++)
    {
        if (ptr[i] == ':' && i < 63)
        {
            char name[64] = { 0 };
            memcpy(name, ptr, i);
            name[i] = '\0';
            size_t vStart = i + 1;
            while (vStart < bytes && (ptr[vStart] == ' ' || ptr[vStart] == '\t')) vStart++;
            size_t vEnd = bytes;
            while (vEnd > vStart && (ptr[vEnd - 1] == '\r' || ptr[vEnd - 1] == '\n' || ptr[vEnd - 1] == ' ')) vEnd--;
            char value[2048] = { 0 };
            if (vEnd - vStart < (int)sizeof(value)) memcpy(value, ptr + vStart, vEnd - vStart);
            HeadersSet(p->headers, name, value);
            break;
        }
    }
    return bytes;
}

int __cdecl DebugCb(CX_CURL, int /*curl_infotype*/, char* data, size_t size, void* user)
{
    CxSession* p = (CxSession*)user;
    if (p == NULL || p->debugLog.pData == NULL) return 0;
    if (p->debugLog.nLen > 256 * 1024) return 0;      // 防止无界增长
    BufAppend(&p->debugLog, data, (int)size);
    return 0;
}

// ============================================================ 执行

// 从参数里取对象指针（对象数据 = 一个 INT，存我们的结构指针）。
template <class T>
T* ObjOf(const MDATA_INF& arg)
{
    if (arg.m_pCompoundData == NULL) return NULL;
    return (T*)(INT_PTR)*(PINT)arg.m_pCompoundData;
}

CxSlist* BuildHeaderList(const CxHeader* pSess, const CxHeader* pReq)
{
    CxSlist* list = NULL;
    char line[2200];
    for (int pass = 0; pass < 2; pass++)
    {
        const CxHeader* p = (pass == 0) ? pSess : pReq;
        for (int i = 0; i < CX_MAX_HEADERS; i++)
        {
            if (!p[i].bUsed) continue;
            // 请求级同名头覆盖会话级
            if (pass == 0 && pReq != NULL)
            {
                bool bOverride = false;
                for (int j = 0; j < CX_MAX_HEADERS; j++)
                    if (pReq[j].bUsed && CxAsciiCmpI(pReq[j].szName, p[i].szName) == 0) { bOverride = true; break; }
                if (bOverride) continue;
            }
            wsprintfA(line, "%s: %s", p[i].szName, p[i].szValue);
            list = g_curl.SlistAppend(list, line);
        }
    }
    return list;
}

bool DoExecute(CxSession* s, CxRequest* r, CxResponse* resp)
{
    ResponseReset(resp);
    s->szErrText[0] = '\0';
    s->nErrCode = 0;

    if (!LoadCurlApi())
    {
        lstrcpynA(s->szErrText, g_curl.LoadError, sizeof(s->szErrText));
        lstrcpynA(resp->szErrText, s->szErrText, sizeof(resp->szErrText));
        return false;
    }
    if (r->szUrl[0] == '\0')
    {
        lstrcpyA(s->szErrText, "请求对象里没设置网址");
        lstrcpynA(resp->szErrText, s->szErrText, sizeof(resp->szErrText));
        return false;
    }

    if (s->hEasy == NULL) s->hEasy = g_curl.EasyInit();
    if (s->hEasy == NULL)
    {
        lstrcpyA(s->szErrText, "创建 curl 会话失败");
        lstrcpynA(resp->szErrText, s->szErrText, sizeof(resp->szErrText));
        return false;
    }

    // 型号变了才重置句柄并重新应用指纹；其余情况保留句柄以复用连接。
    if (lstrcmpA(s->szProfile, s->szAppliedProfile) != 0)
    {
        g_curl.EasyReset(s->hEasy);
        s->szAppliedProfile[0] = '\0';
        BufReset(&s->debugLog);
        if (g_curl.Impersonate(s->hEasy, s->szProfile, 1) != 0)
        {
            wsprintfA(s->szErrText, "不支持的指纹型号：%s（可用 取指纹型号列表 查询）", s->szProfile);
            lstrcpynA(resp->szErrText, s->szErrText, sizeof(resp->szErrText));
            return false;
        }
        lstrcpynA(s->szAppliedProfile, s->szProfile, sizeof(s->szAppliedProfile));
    }

    fn_easy_setopt_t so = g_curl.EasySetopt;
    so(s->hEasy, OPT_URL, r->szUrl);
    so(s->hEasy, OPT_TIMEOUT, (long)s->nTimeout);
    so(s->hEasy, OPT_CONNECTTIMEOUT, (long)s->nConnectTimeout);
    so(s->hEasy, OPT_FOLLOWLOCATION, s->bFollowRedirect ? 1L : 0L);
    so(s->hEasy, OPT_MAXREDIRS, (long)s->nMaxRedirects);
    so(s->hEasy, OPT_WRITEFUNCTION, (void*)&WriteCb);
    so(s->hEasy, OPT_WRITEDATA, (void*)resp);
    so(s->hEasy, OPT_HEADERFUNCTION, (void*)&HeaderCb);
    so(s->hEasy, OPT_HEADERDATA, (void*)resp);
    so(s->hEasy, OPT_ACCEPT_ENCODING, "");        // 让 curl 透明还原 br/gzip/zstd
    so(s->hEasy, OPT_SSL_VERIFYPEER, (long)s->nVerifyPeer);
    so(s->hEasy, OPT_SSL_VERIFYHOST, (long)s->nVerifyHost);
    // cookie 引擎：给了文件就持久化，否则只在内存里维持
    so(s->hEasy, OPT_COOKIEFILE, s->szCookieFile[0] ? s->szCookieFile : "");
    if (s->szCookieFile[0]) so(s->hEasy, OPT_COOKIEJAR, s->szCookieFile);
    if (s->szCookieText[0]) so(s->hEasy, OPT_COOKIE, s->szCookieText);
    if (s->szProxy[0]) so(s->hEasy, OPT_PROXY, s->szProxy);
    if (s->szProxyUserPwd[0]) so(s->hEasy, OPT_PROXYUSERPWD, s->szProxyUserPwd);
    if (s->szUserName[0]) so(s->hEasy, OPT_USERNAME, s->szUserName);
    if (s->szPassword[0]) so(s->hEasy, OPT_PASSWORD, s->szPassword);
    if (s->nAuth > 0 && (s->szUserName[0] || s->szProxyUserPwd[0])) so(s->hEasy, OPT_HTTPAUTH, (long)s->nAuth);
    if (s->szUserAgent[0]) so(s->hEasy, OPT_USERAGENT, s->szUserAgent);
    if (s->szCAInfo[0]) so(s->hEasy, OPT_CAINFO, s->szCAInfo);
    if (s->szCAPath[0]) so(s->hEasy, OPT_CAPATH, s->szCAPath);
    if (s->nLowSpeedLimit > 0) so(s->hEasy, OPT_LOW_SPEED_LIMIT, (long)s->nLowSpeedLimit);
    if (s->nLowSpeedTime > 0) so(s->hEasy, OPT_LOW_SPEED_TIME, (long)s->nLowSpeedTime);
    if (s->bDebug)
    {
        so(s->hEasy, OPT_VERBOSE, 1L);
        so(s->hEasy, OPT_DEBUGFUNCTION, (void*)&DebugCb);
        so(s->hEasy, OPT_DEBUGDATA, (void*)s);
    }
    else
    {
        so(s->hEasy, OPT_VERBOSE, 0L);
    }

    CxSlist* list = BuildHeaderList(s->headers, r->headers);
    if (list != NULL) so(s->hEasy, OPT_HTTPHEADER, list);

    // 方法：显式优先；有正文没写方法则按 POST。
    LPCSTR szMethod = r->szMethod;
    const bool bHasBody = (r->body.nLen > 0 || r->files[0].bUsed);
    if (szMethod[0] == '\0') szMethod = bHasBody ? "POST" : "GET";
    if (lstrcmpiA(szMethod, "GET") == 0)
    {
        so(s->hEasy, OPT_CUSTOMREQUEST, NULL);
        so(s->hEasy, OPT_NOBODY, 0L);
    }
    else if (lstrcmpiA(szMethod, "HEAD") == 0)
    {
        so(s->hEasy, OPT_NOBODY, 1L);
        so(s->hEasy, OPT_CUSTOMREQUEST, "HEAD");
    }
    else
    {
        so(s->hEasy, OPT_NOBODY, 0L);
        so(s->hEasy, OPT_CUSTOMREQUEST, szMethod);
    }

    CX_MIME mime = NULL;
    if (r->files[0].bUsed && g_curl.MimeInit != NULL)
    {
        mime = g_curl.MimeInit(s->hEasy);
        for (int i = 0; i < CX_MAX_FILES; i++)
        {
            if (!r->files[i].bUsed) continue;
            CX_MIMEPART part = g_curl.MimeAddPart(mime);
            if (part == NULL) continue;
            if (r->files[i].szField[0]) g_curl.MimeName(part, r->files[i].szField);
            g_curl.MimeFileData(part, r->files[i].szFile);
            if (r->files[i].szType[0]) g_curl.MimeType(part, r->files[i].szType);
        }
        if (r->body.nLen > 0 && g_curl.MimeAddPart != NULL)
        {
            CX_MIMEPART part = g_curl.MimeAddPart(mime);
            g_curl.MimeName(part, "data");
            g_curl.MimeData(part, r->body.pData, (size_t)r->body.nLen);
        }
        so(s->hEasy, OPT_MIMEPOST, mime);
    }
    else if (r->body.nLen > 0)
    {
        so(s->hEasy, OPT_POSTFIELDSIZE, (long)r->body.nLen);
        so(s->hEasy, OPT_POSTFIELDS, r->body.pData);
    }

    if (r->nResumeFrom > 0) so(s->hEasy, OPT_RESUME_FROM, (long)r->nResumeFrom);

    if (r->szOutFile[0] != '\0')
    {
        resp->pOutFp = fopen(r->szOutFile, r->nResumeFrom > 0 ? "ab" : "wb");
        if (resp->pOutFp == NULL)
        {
            wsprintfA(s->szErrText, "无法写入下载文件：%s", r->szOutFile);
            lstrcpynA(resp->szErrText, s->szErrText, sizeof(resp->szErrText));
            if (list != NULL) g_curl.SlistFreeAll(list);
            if (mime != NULL && g_curl.MimeFree != NULL) g_curl.MimeFree(mime);
            return false;
        }
    }

    const DWORD dwStart = GetTickCount();
    const int rc = g_curl.EasyPerform(s->hEasy);
    resp->nElapsedMs = (int)(GetTickCount() - dwStart);

    g_curl.EasyGetinfo(s->hEasy, INFO_RESPONSE_CODE, &resp->nStatus);
    {
        char* pUrl = NULL;
        g_curl.EasyGetinfo(s->hEasy, INFO_EFFECTIVE_URL, &pUrl);
        if (pUrl != NULL) lstrcpynA(resp->szFinalUrl, pUrl, sizeof(resp->szFinalUrl));
        char* pType = NULL;
        g_curl.EasyGetinfo(s->hEasy, INFO_CONTENT_TYPE, &pType);
        if (pType != NULL) lstrcpynA(resp->szContentType, pType, sizeof(resp->szContentType));
        long nRedir = 0;
        g_curl.EasyGetinfo(s->hEasy, INFO_REDIRECT_COUNT, &nRedir);
        resp->nRedirectCount = (int)nRedir;
    }

    if (resp->pOutFp != NULL) { fclose(resp->pOutFp); resp->pOutFp = NULL; }
    // 接收字节数事后测量：内存模式取正文长度，落盘模式取文件大小。
    // （回调里对 8 字节字段的自增在本工程实测不生效，语义等价的做法是这里统一算。）
    resp->nReceived = (INT64)resp->body.nLen;
    if (r->szOutFile[0] != '\0')
    {
        HANDLE hf = CreateFileA(r->szOutFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);
        if (hf != INVALID_HANDLE_VALUE)
        {
            DWORD hi = 0;
            const DWORD lo = GetFileSize(hf, &hi);
            CloseHandle(hf);
            if (lo != INVALID_FILE_SIZE) resp->nReceived = ((INT64)hi << 32) | lo;
        }
    }
    if (list != NULL) g_curl.SlistFreeAll(list);
    if (mime != NULL && g_curl.MimeFree != NULL) g_curl.MimeFree(mime);

    if (rc != 0)
    {
        s->nErrCode = rc;
        resp->nErrCode = rc;
        lstrcpynA(s->szErrText, g_curl.EasyStrerror != NULL ? g_curl.EasyStrerror(rc) : "请求失败",
            sizeof(s->szErrText));
        lstrcpynA(resp->szErrText, s->szErrText, sizeof(resp->szErrText));
        return false;
    }
    return true;
}

// ============================================================ 全局命令

void cxsoft_Test(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    LPCSTR pszText = "helloworld";
    if (pArgInf != NULL && pArgInf[0].m_dtDataType == SDT_TEXT) pszText = pArgInf[0].m_pText;
    pRetData->m_dtDataType = SDT_TEXT;
    pRetData->m_pText = CloneTextData(pszText);
}

void cxsoft_GetProfileList(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    static const char* kProfiles[] = {
        "chrome99", "chrome100", "chrome101", "chrome104", "chrome107", "chrome110",
        "chrome116", "chrome119", "chrome120", "chrome123", "chrome124", "chrome131",
        "chrome131_android", "chrome133a", "chrome136", "chrome142", "chrome146", "chrome150",
        "edge99", "edge101", "firefox133", "firefox135", "firefox144", "firefox147",
        "safari153", "safari155", "safari170", "safari172_ios", "safari180", "safari184",
        "safari260", "safari2601", "safari260_ios", "okhttp4", "okhttp4_android", "tor145",
    };
    char out[1024];
    out[0] = '\0';
    int nPos = 0;
    for (int i = 0; i < (int)(sizeof(kProfiles) / sizeof(kProfiles[0])); i++)
    {
        const int n = lstrlenA(kProfiles[i]);
        if (nPos + n + 3 >= (int)sizeof(out)) break;
        lstrcpyA(out + nPos, kProfiles[i]);
        nPos += n;
        out[nPos++] = '\r';
        out[nPos++] = '\n';
        out[nPos] = '\0';
    }
    SetRetText(pRetData, out);
}

void cxsoft_GetEngineInfo(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    if (!LoadCurlApi()) { SetRetText(pRetData, g_curl.LoadError); return; }
    SetRetText(pRetData, g_curl.Version != NULL ? g_curl.Version() : "libcurl");
}

// 百分号编解码自己实现：curl_easy_escape 在这个版本上对刚 init 的裸句柄返回 NULL（实测）。
const char CX_HEX[] = "0123456789ABCDEF";

int HexVal(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

void UrlEncodeText(LPCSTR szIn, CxBuf* pOut)
{
    if (szIn == NULL || pOut == NULL) return;
    for (const unsigned char* p = (const unsigned char*)szIn; *p; p++)
    {
        char one[4];
        const unsigned char c = *p;
        const bool bSafe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
            || c == '-' || c == '_' || c == '.' || c == '~';
        if (bSafe) { one[0] = (char)c; one[1] = '\0'; BufAppend(pOut, one, 1); }
        else { one[0] = '%'; one[1] = CX_HEX[c >> 4]; one[2] = CX_HEX[c & 15]; one[3] = '\0'; BufAppend(pOut, one, 3); }
    }
}

void UrlDecodeText(LPCSTR szIn, CxBuf* pOut)
{
    if (szIn == NULL || pOut == NULL) return;
    for (const char* p = szIn; *p; p++)
    {
        char one[2];
        if (*p == '+') { one[0] = ' '; one[1] = '\0'; BufAppend(pOut, one, 1); continue; }
        if (p[0] == '%' && p[1] && p[2])
        {
            const int hi = HexVal(p[1]), lo = HexVal(p[2]);
            if (hi >= 0 && lo >= 0) { one[0] = (char)(hi * 16 + lo); one[1] = '\0'; BufAppend(pOut, one, 1); p += 2; continue; }
        }
        one[0] = *p; one[1] = '\0'; BufAppend(pOut, one, 1);
    }
}

void DoUrlCnv(PMDATA_INF pRetData, PMDATA_INF pArgInf, bool bEncode)
{
    pRetData->m_dtDataType = SDT_TEXT;
    pRetData->m_pText = CloneTextData("");
    if (pArgInf == NULL || pArgInf[0].m_dtDataType != SDT_TEXT) return;

    CxBuf out;
    BufInit(&out, lstrlenA(pArgInf[0].m_pText) * 3 + 16);
    if (bEncode) UrlEncodeText(pArgInf[0].m_pText, &out);
    else UrlDecodeText(pArgInf[0].m_pText, &out);
    pRetData->m_pText = CloneTextData(out.pData, out.nLen);
    BufFree(&out);
}

void cxsoft_UrlEncode(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    DoUrlCnv(pRetData, pArgInf, true);
}

void cxsoft_UrlDecode(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    DoUrlCnv(pRetData, pArgInf, false);
}

// UTF-8 文本 -> 本机 ANSI(GBK)，便于直接显示；非法序列按窄字符丢弃。
void cxsoft_Utf8ToGbk(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = SDT_TEXT;
    pRetData->m_pText = CloneTextData("");
    if (pArgInf == NULL || pArgInf[0].m_dtDataType != SDT_TEXT) return;

    const LPCSTR szUtf8 = pArgInf[0].m_pText;
    const int nWide = MultiByteToWideChar(CP_UTF8, 0, szUtf8, -1, NULL, 0);
    if (nWide <= 0) { SetRetText(pRetData, szUtf8); return; }
    LPWSTR pW = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * nWide);
    if (pW == NULL) { SetRetText(pRetData, szUtf8); return; }
    MultiByteToWideChar(CP_UTF8, 0, szUtf8, -1, pW, nWide);
    const int nA = WideCharToMultiByte(CP_ACP, 0, pW, -1, NULL, 0, NULL, NULL);
    if (nA > 0)
    {
        LPSTR pA = (LPSTR)LocalAlloc(LPTR, nA);
        if (pA != NULL)
        {
            WideCharToMultiByte(CP_ACP, 0, pW, -1, pA, nA, NULL, NULL);
            pRetData->m_pText = CloneTextData(pA, nA - 1);
            LocalFree(pA);
        }
    }
    LocalFree(pW);
}

// 一次性请求，内部自建临时会话与响应对象。
void cxsoft_QuickGet(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = SDT_TEXT;
    pRetData->m_pText = CloneTextData("");
    if (pArgInf == NULL || pArgInf[0].m_dtDataType != SDT_TEXT) return;

    CxSession* s = NewSession();
    CxRequest* r = NewRequest();
    CxResponse* resp = NewResponse();
    if (s == NULL || r == NULL || resp == NULL) { FreeSession(s); FreeRequest(r); FreeResponse(resp); return; }

    lstrcpynA(r->szUrl, pArgInf[0].m_pText, sizeof(r->szUrl));
    if (pArgInf[1].m_dtDataType == SDT_TEXT && pArgInf[1].m_pText != NULL && pArgInf[1].m_pText[0])
        lstrcpynA(s->szProfile, pArgInf[1].m_pText, sizeof(s->szProfile));
    if (pArgInf[2].m_dtDataType == SDT_TEXT && pArgInf[2].m_pText != NULL && pArgInf[2].m_pText[0])
        lstrcpynA(s->szProxy, pArgInf[2].m_pText, sizeof(s->szProxy));

    EnterCriticalSection(&s->cs);
    const bool ok = DoExecute(s, r, resp);
    LeaveCriticalSection(&s->cs);

    pRetData->m_pText = ok ? CloneTextData(resp->body.pData, resp->body.nLen) : CloneTextData(resp->szErrText);
    FreeSession(s);
    FreeRequest(r);
    FreeResponse(resp);
}

// ============================================================ 会话成员命令

CxSession* SessSelf(PMDATA_INF pArgInf)
{
    CxSession* p = ObjOf<CxSession>(pArgInf[0]);
    if (p == NULL) ReportError("浏览器会话对象未初始化");
    return p;
}

void SessConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData);
    UNREFERENCED_PARAMETER(nArgCount);
    *(PINT)pArgInf[0].m_pCompoundData = (INT)(INT_PTR)NewSession();
}

void SessCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData);
    UNREFERENCED_PARAMETER(nArgCount);
    CxSession* pDst = NewSession();
    *(PINT)pArgInf[0].m_pCompoundData = (INT)(INT_PTR)pDst;
    CopySession(pDst, ObjOf<CxSession>(pArgInf[1]));
}

void SessDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData);
    UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p != NULL)
    {
        FreeSession(p);
        *(PINT)pArgInf[0].m_pCompoundData = 0;
    }
}

void SessSetProfile(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p != NULL && pArgInf[1].m_dtDataType == SDT_TEXT)
        lstrcpynA(p->szProfile, pArgInf[1].m_pText, sizeof(p->szProfile));
}

void SessGetProfile(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    SetRetText(pRetData, p != NULL ? p->szProfile : "");
}

void SessSetProxy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p == NULL) return;
    if (pArgInf[1].m_dtDataType == SDT_TEXT) lstrcpynA(p->szProxy, pArgInf[1].m_pText, sizeof(p->szProxy));
    else p->szProxy[0] = '\0';
}

void SessSetProxyAuth(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p == NULL) return;
    if (pArgInf[1].m_dtDataType == SDT_TEXT && pArgInf[2].m_dtDataType == SDT_TEXT)
        wsprintfA(p->szProxyUserPwd, "%s:%s", pArgInf[1].m_pText, pArgInf[2].m_pText);
    else p->szProxyUserPwd[0] = '\0';
}

void SessSetSiteAuth(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p == NULL) return;
    if (pArgInf[1].m_dtDataType == SDT_TEXT) lstrcpynA(p->szUserName, pArgInf[1].m_pText, sizeof(p->szUserName));
    else p->szUserName[0] = '\0';
    if (pArgInf[2].m_dtDataType == SDT_TEXT) lstrcpynA(p->szPassword, pArgInf[2].m_pText, sizeof(p->szPassword));
    else p->szPassword[0] = '\0';
}

void SessSetUserAgent(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p == NULL) return;
    if (pArgInf[1].m_dtDataType == SDT_TEXT) lstrcpynA(p->szUserAgent, pArgInf[1].m_pText, sizeof(p->szUserAgent));
    else p->szUserAgent[0] = '\0';
}

void SessSetTimeout(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p != NULL && pArgInf[1].m_dtDataType == SDT_INT && pArgInf[1].m_int > 0) p->nTimeout = pArgInf[1].m_int;
}

void SessSetConnTimeout(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p != NULL && pArgInf[1].m_dtDataType == SDT_INT && pArgInf[1].m_int > 0) p->nConnectTimeout = pArgInf[1].m_int;
}

void SessSetLowSpeed(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p == NULL) return;
    p->nLowSpeedLimit = (pArgInf[1].m_dtDataType == SDT_INT) ? pArgInf[1].m_int : 0;
    p->nLowSpeedTime = (pArgInf[2].m_dtDataType == SDT_INT) ? pArgInf[2].m_int : 0;
}

void SessSetVerifyCert(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p == NULL) return;
    const bool bVerify = (pArgInf[1].m_dtDataType == SDT_BOOL) ? (pArgInf[1].m_bool != FALSE) : true;
    p->nVerifyPeer = bVerify ? 1 : 0;
    p->nVerifyHost = bVerify ? 2 : 0;
}

void SessSetCA(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p == NULL) return;
    if (pArgInf[1].m_dtDataType == SDT_TEXT) lstrcpynA(p->szCAInfo, pArgInf[1].m_pText, sizeof(p->szCAInfo));
    else p->szCAInfo[0] = '\0';
    if (pArgInf[2].m_dtDataType == SDT_TEXT) lstrcpynA(p->szCAPath, pArgInf[2].m_pText, sizeof(p->szCAPath));
    else p->szCAPath[0] = '\0';
}

void SessSetFollow(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p != NULL && pArgInf[1].m_dtDataType == SDT_BOOL) p->bFollowRedirect = (pArgInf[1].m_bool != FALSE);
}

void SessSetMaxRedirect(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p != NULL && pArgInf[1].m_dtDataType == SDT_INT) p->nMaxRedirects = pArgInf[1].m_int;
}

void SessSetCookieFile(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p == NULL) return;
    if (pArgInf[1].m_dtDataType == SDT_TEXT) lstrcpynA(p->szCookieFile, pArgInf[1].m_pText, sizeof(p->szCookieFile));
    else p->szCookieFile[0] = '\0';
}

void SessSetCookieText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p == NULL) return;
    if (pArgInf[1].m_dtDataType == SDT_TEXT) lstrcpynA(p->szCookieText, pArgInf[1].m_pText, sizeof(p->szCookieText));
    else p->szCookieText[0] = '\0';
}

void SessGetCookieText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    SetRetText(pRetData, p != NULL ? p->szCookieText : "");
    if (p == NULL || !g_curl.Ready || p->hEasy == NULL) return;

    // 句柄里的 cookie 引擎状态优先
    CxSlist* cookies = NULL;
    EnterCriticalSection(&p->cs);
    if (g_curl.EasyGetinfo(p->hEasy, INFO_COOKIELIST, &cookies) == 0 && cookies != NULL)
    {
        CxBuf out;
        BufInit(&out, 4096);
        for (CxSlist* it = cookies; it != NULL; it = it->pNext)
        {
            BufAppend(&out, it->pData, lstrlenA(it->pData));
            BufAppend(&out, "\r\n", 2);
        }
        pRetData->m_dtDataType = SDT_TEXT;
        pRetData->m_pText = CloneTextData(out.pData, out.nLen);
        BufFree(&out);
    }
    LeaveCriticalSection(&p->cs);
    if (cookies != NULL && g_curl.SlistFreeAll) g_curl.SlistFreeAll(cookies);
}

void SessSetHeader(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p != NULL && pArgInf[1].m_dtDataType == SDT_TEXT && pArgInf[2].m_dtDataType == SDT_TEXT)
        HeadersSet(p->headers, pArgInf[1].m_pText, pArgInf[2].m_pText);
}

void SessDelHeader(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p != NULL && pArgInf[1].m_dtDataType == SDT_TEXT) HeadersDel(p->headers, pArgInf[1].m_pText);
}

void SessClearHeader(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p != NULL) HeadersClear(p->headers);
}

void HeadersToText(const CxHeader* p, CxBuf* pOut)
{
    char line[2200];
    for (int i = 0; i < CX_MAX_HEADERS; i++)
    {
        if (!p[i].bUsed) continue;
        wsprintfA(line, "%s: %s\r\n", p[i].szName, p[i].szValue);
        BufAppend(pOut, line, lstrlenA(line));
    }
}

void SessGetHeaderText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    CxBuf out;
    BufInit(&out, 1024);
    if (p != NULL) HeadersToText(p->headers, &out);
    pRetData->m_dtDataType = SDT_TEXT;
    pRetData->m_pText = CloneTextData(out.pData, out.nLen);
    BufFree(&out);
}

void SessSetDebug(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p == NULL || pArgInf[1].m_dtDataType != SDT_BOOL) return;
    p->bDebug = (pArgInf[1].m_bool != FALSE);
    if (!p->bDebug) BufReset(&p->debugLog);
}

void SessGetDebugLog(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    pRetData->m_dtDataType = SDT_TEXT;
    pRetData->m_pText = CloneTextData(p != NULL ? p->debugLog.pData : "", p != NULL ? p->debugLog.nLen : 0);
}

void SessExecute(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = SDT_BOOL;
    pRetData->m_bool = FALSE;

    CxSession* s = SessSelf(pArgInf);
    CxRequest* r = ObjOf<CxRequest>(pArgInf[1]);
    CxResponse* resp = ObjOf<CxResponse>(pArgInf[2]);
    if (s == NULL || r == NULL || resp == NULL)
    {
        ReportError("执行 需要 浏览器会话、浏览器请求、浏览器响应 三个对象");
        return;
    }
    if (r->szUrl[0] == '\0')
    {
        resp->nErrCode = -1;
        lstrcpyA(resp->szErrText, "请求对象里没设置网址");
        return;
    }

    EnterCriticalSection(&s->cs);            // 同一会话串行使用句柄；跨线程请各建会话
    const bool ok = DoExecute(s, r, resp);
    LeaveCriticalSection(&s->cs);
    pRetData->m_bool = ok ? TRUE : FALSE;
}

void SessGetErrCode(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    pRetData->m_int = p != NULL ? p->nErrCode : 0;
}

void SessGetErrText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    SetRetText(pRetData, p != NULL ? p->szErrText : "");
}

void SessReset(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxSession* p = SessSelf(pArgInf);
    if (p == NULL) return;
    EnterCriticalSection(&p->cs);
    if (p->hEasy != NULL && g_curl.EasyCleanup != NULL) g_curl.EasyCleanup(p->hEasy);
    p->hEasy = NULL;
    p->szAppliedProfile[0] = '\0';
    BufReset(&p->debugLog);
    LeaveCriticalSection(&p->cs);
}

// ============================================================ 请求成员命令

CxRequest* ReqSelf(PMDATA_INF pArgInf)
{
    CxRequest* p = ObjOf<CxRequest>(pArgInf[0]);
    if (p == NULL) ReportError("浏览器请求对象未初始化");
    return p;
}

void ReqConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    *(PINT)pArgInf[0].m_pCompoundData = (INT)(INT_PTR)NewRequest();
}

void ReqCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* pDst = NewRequest();
    *(PINT)pArgInf[0].m_pCompoundData = (INT)(INT_PTR)pDst;
    CopyRequest(pDst, ObjOf<CxRequest>(pArgInf[1]));
}

void ReqDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* p = ReqSelf(pArgInf);
    if (p != NULL) { FreeRequest(p); *(PINT)pArgInf[0].m_pCompoundData = 0; }
}

void ReqSetUrl(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* p = ReqSelf(pArgInf);
    if (p != NULL && pArgInf[1].m_dtDataType == SDT_TEXT)
        lstrcpynA(p->szUrl, pArgInf[1].m_pText, sizeof(p->szUrl));
}

void ReqGetUrl(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* p = ReqSelf(pArgInf);
    SetRetText(pRetData, p != NULL ? p->szUrl : "");
}

void ReqSetMethod(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* p = ReqSelf(pArgInf);
    if (p == NULL) return;
    if (pArgInf[1].m_dtDataType == SDT_TEXT) lstrcpynA(p->szMethod, pArgInf[1].m_pText, sizeof(p->szMethod));
    else p->szMethod[0] = '\0';
}

void ReqGetMethod(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* p = ReqSelf(pArgInf);
    SetRetText(pRetData, p != NULL ? p->szMethod : "");
}

void ReqSetBodyText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* p = ReqSelf(pArgInf);
    if (p == NULL) return;
    BufReset(&p->body);
    if (pArgInf[1].m_dtDataType == SDT_TEXT) BufAppend(&p->body, pArgInf[1].m_pText, lstrlenA(pArgInf[1].m_pText));
    p->bPost = (p->body.nLen > 0 || p->files[0].bUsed);
}

void ReqSetBodyBin(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* p = ReqSelf(pArgInf);
    if (p == NULL) return;
    BufReset(&p->body);
    if (pArgInf[1].m_dtDataType == SDT_BIN && pArgInf[1].m_pBin != NULL)
    {
        const INT n = *(INT*)(pArgInf[1].m_pBin + sizeof(INT));
        BufAppend(&p->body, (LPCSTR)(pArgInf[1].m_pBin + sizeof(INT) * 2), n);
    }
    p->bPost = (p->body.nLen > 0 || p->files[0].bUsed);
}

void ReqGetBodyLen(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* p = ReqSelf(pArgInf);
    pRetData->m_int = p != NULL ? p->body.nLen : 0;
}

void ReqSetHeader(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* p = ReqSelf(pArgInf);
    if (p != NULL && pArgInf[1].m_dtDataType == SDT_TEXT && pArgInf[2].m_dtDataType == SDT_TEXT)
        HeadersSet(p->headers, pArgInf[1].m_pText, pArgInf[2].m_pText);
}

void ReqClearHeader(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* p = ReqSelf(pArgInf);
    if (p != NULL) HeadersClear(p->headers);
}

void ReqSetOutFile(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* p = ReqSelf(pArgInf);
    if (p == NULL) return;
    if (pArgInf[1].m_dtDataType == SDT_TEXT) lstrcpynA(p->szOutFile, pArgInf[1].m_pText, sizeof(p->szOutFile));
    else p->szOutFile[0] = '\0';
}

void ReqSetResumeFrom(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* p = ReqSelf(pArgInf);
    if (p != NULL && pArgInf[1].m_dtDataType == SDT_INT) p->nResumeFrom = pArgInf[1].m_int;
}

void ReqAddFile(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* p = ReqSelf(pArgInf);
    if (p == NULL || pArgInf[1].m_dtDataType != SDT_TEXT || pArgInf[2].m_dtDataType != SDT_TEXT) return;
    for (int i = 0; i < CX_MAX_FILES; i++)
    {
        if (p->files[i].bUsed) continue;
        p->files[i].bUsed = true;
        lstrcpynA(p->files[i].szField, pArgInf[1].m_pText, sizeof(p->files[i].szField));
        lstrcpynA(p->files[i].szFile, pArgInf[2].m_pText, sizeof(p->files[i].szFile));
        if (pArgInf[3].m_dtDataType == SDT_TEXT) lstrcpynA(p->files[i].szType, pArgInf[3].m_pText, sizeof(p->files[i].szType));
        p->bPost = true;
        return;
    }
    ReportError("上传文件数量已达上限 8");
}

void ReqClearFiles(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxRequest* p = ReqSelf(pArgInf);
    if (p != NULL) { memset(p->files, 0, sizeof(p->files)); p->bPost = (p->body.nLen > 0); }
}

// ============================================================ 响应成员命令

CxResponse* RespSelf(PMDATA_INF pArgInf)
{
    CxResponse* p = ObjOf<CxResponse>(pArgInf[0]);
    if (p == NULL) ReportError("浏览器响应对象未初始化");
    return p;
}

void RespConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    *(PINT)pArgInf[0].m_pCompoundData = (INT)(INT_PTR)NewResponse();
}

void RespCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* pDst = NewResponse();
    *(PINT)pArgInf[0].m_pCompoundData = (INT)(INT_PTR)pDst;
    CopyResponse(pDst, ObjOf<CxResponse>(pArgInf[1]));
}

void RespDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(pRetData); UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    if (p != NULL) { FreeResponse(p); *(PINT)pArgInf[0].m_pCompoundData = 0; }
}

void RespGetStatus(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    pRetData->m_int = p != NULL ? (INT)p->nStatus : 0;
}

void RespIsOk(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    pRetData->m_bool = (p != NULL && p->nErrCode == 0 && p->nStatus >= 200 && p->nStatus < 400) ? TRUE : FALSE;
}

void RespGetErrCode(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    pRetData->m_int = p != NULL ? p->nErrCode : 0;
}

void RespGetErrText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    SetRetText(pRetData, p != NULL ? p->szErrText : "");
}

void RespGetText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    pRetData->m_dtDataType = SDT_TEXT;
    pRetData->m_pText = (p != NULL) ? CloneTextData(p->body.pData, p->body.nLen) : CloneTextData("");
}

void RespGetTextGbk(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    pRetData->m_dtDataType = SDT_TEXT;
    pRetData->m_pText = CloneTextData("");
    if (p == NULL || p->body.nLen <= 0) return;

    const int nWide = MultiByteToWideChar(CP_UTF8, 0, p->body.pData, -1, NULL, 0);
    if (nWide <= 0) { pRetData->m_pText = CloneTextData(p->body.pData, p->body.nLen); return; }
    LPWSTR pW = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * nWide);
    if (pW == NULL) { pRetData->m_pText = CloneTextData(p->body.pData, p->body.nLen); return; }
    MultiByteToWideChar(CP_UTF8, 0, p->body.pData, -1, pW, nWide);
    const int nA = WideCharToMultiByte(CP_ACP, 0, pW, -1, NULL, 0, NULL, NULL);
    if (nA > 0)
    {
        LPSTR pA = (LPSTR)LocalAlloc(LPTR, nA);
        if (pA != NULL)
        {
            WideCharToMultiByte(CP_ACP, 0, pW, -1, pA, nA, NULL, NULL);
            pRetData->m_pText = CloneTextData(pA, nA - 1);
            LocalFree(pA);
        }
    }
    LocalFree(pW);
}

void RespGetBin(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    pRetData->m_dtDataType = SDT_BIN;
    pRetData->m_pBin = (p != NULL) ? CloneBinData(p->body.pData, p->body.nLen) : NULL;
}

void RespGetHeadText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    pRetData->m_dtDataType = SDT_TEXT;
    pRetData->m_pText = (p != NULL) ? CloneTextData(p->headRaw.pData, p->headRaw.nLen) : CloneTextData("");
}

void RespGetHeadCount(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    INT n = 0;
    if (p != NULL) for (int i = 0; i < CX_MAX_HEADERS; i++) if (p->headers[i].bUsed) n++;
    pRetData->m_int = n;
}

void RespGetHeadName(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    SetRetText(pRetData, "");
    if (p == NULL || pArgInf[1].m_dtDataType != SDT_INT) return;
    INT nWant = pArgInf[1].m_int;             // 易语言数组从 1 开始
    for (int i = 0; i < CX_MAX_HEADERS; i++)
        if (p->headers[i].bUsed && --nWant == 0) { SetRetText(pRetData, p->headers[i].szName); return; }
}

void RespGetHeadValue(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    SetRetText(pRetData, "");
    if (p == NULL || pArgInf[1].m_dtDataType != SDT_INT) return;
    INT nWant = pArgInf[1].m_int;
    for (int i = 0; i < CX_MAX_HEADERS; i++)
        if (p->headers[i].bUsed && --nWant == 0) { SetRetText(pRetData, p->headers[i].szValue); return; }
}

void RespGetHeadByName(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    SetRetText(pRetData, "");
    if (p == NULL || pArgInf[1].m_dtDataType != SDT_TEXT) return;
    for (int i = 0; i < CX_MAX_HEADERS; i++)
        if (p->headers[i].bUsed && CxAsciiCmpI(p->headers[i].szName, pArgInf[1].m_pText) == 0)
        {
            SetRetText(pRetData, p->headers[i].szValue);
            return;
        }
}

void RespGetFinalUrl(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    SetRetText(pRetData, p != NULL ? p->szFinalUrl : "");
}

void RespGetContentType(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    SetRetText(pRetData, p != NULL ? p->szContentType : "");
}

void RespGetRedirectCount(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    pRetData->m_int = p != NULL ? p->nRedirectCount : 0;
}

void RespGetElapsedMs(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    pRetData->m_int = p != NULL ? p->nElapsedMs : 0;
}

void RespGetDownloadSize(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxResponse* p = RespSelf(pArgInf);
    pRetData->m_int = p != NULL ? (INT)p->nReceived : 0;
}

void RespSaveToFile(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    pRetData->m_dtDataType = SDT_BOOL;
    pRetData->m_bool = FALSE;
    CxResponse* p = RespSelf(pArgInf);
    if (p == NULL || p->body.nLen <= 0 || pArgInf[1].m_dtDataType != SDT_TEXT) return;

    FILE* fp = fopen(pArgInf[1].m_pText, "wb");
    if (fp == NULL) return;
    pRetData->m_bool = (fwrite(p->body.pData, 1, p->body.nLen, fp) == (size_t)p->body.nLen) ? TRUE : FALSE;
    fclose(fp);
}

// ============================================================ 参数表
// 静态编译只需要命令实现，表定义与库信息不进 .lib。
// HTTP访问 族的实现体必须在 __E_STATIC_LIB 守卫之外：静态编译时命令表整段不参与编译，
// 但实现要留在 cxsoft_static.lib 里（gen_static_trampolines.mjs 按名字挑符号）。
#include "http访问/cxhttp_ops.h"
#include "http访问/cxhttp_util.h"
#include "线程锁/cxsync_lifecycle.inc"
#include "线程锁/cxcancel_core.inc"
#include "http访问/cxhttp_core.inc"
#include "http访问/cxhttp_util.inc"
#include "http访问/cxhttp_extra.inc"
#include "http访问/cxhttp_access_curl.inc"
#include "http访问/cxhttp_access_win.inc"
#include "http访问/cxhttp_glob.inc"
#include "线程锁/lock_ops.h"
#include "线程锁/cxlock_api.inc"
#include "线程锁/cxlock_core.inc"
#include "线程锁/cxlock_impl.inc"
#include "线程锁/lock_impl.inc"
#include "线程锁/cxjoin_core.inc"
#include "线程锁/cxsync_state.inc"
#include "http访问/cxhttp_impl.inc"

#ifndef __E_STATIC_LIB
// 成员命令只描述用户参数；对象自身由系统作为第 0 个参数传入。

ARG_INFO g_argGreeting[] = {
    { _T("问候语"), _T("要返回的文本内容，留空返回 helloworld。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
ARG_INFO g_argText1[] = {
    { _T("文本"), _T("要处理的 UTF-8 文本。"), 0, 0, SDT_TEXT, 0, 0 },
};
ARG_INFO g_argUrl1[] = {
    { _T("网址"), _T("完整网址，支持 http / https。"), 0, 0, SDT_TEXT, 0, 0 },
};
ARG_INFO g_argQuickGet[] = {
    { _T("网址"), _T("完整网址。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("指纹型号"), _T("如 chrome133a，留空用默认型号。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("代理"), _T("如 http://127.0.0.1:10809，留空不用代理。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
ARG_INFO g_argProfile[] = {
    { _T("型号"), _T("浏览器指纹型号，如 chrome133a、firefox135、safari170。"), 0, 0, SDT_TEXT, 0, 0 },
};
ARG_INFO g_argProxy[] = {
    { _T("代理地址"), _T("如 http://127.0.0.1:10809 或 socks5://127.0.0.1:7890；留空不用代理。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
ARG_INFO g_argAuth2[] = {
    { _T("用户名"), _T("认证用户名。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("密码"), _T("认证密码。"), 0, 0, SDT_TEXT, 0, 0 },
};
ARG_INFO g_argUserAgent[] = {
    { _T("UA"), _T("用户代理；留空则沿用指纹型号自带的 UA。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
ARG_INFO g_argTimeout[] = {
    { _T("秒数"), _T("整个请求的最长等待时间，默认 30 秒。"), 0, 0, SDT_INT, 0, 0 },
};
ARG_INFO g_argConnTimeout[] = {
    { _T("秒数"), _T("建立连接的最长等待时间，默认 10 秒。"), 0, 0, SDT_INT, 0, 0 },
};
ARG_INFO g_argLowSpeed[] = {
    { _T("字节每秒"), _T("低于该速率开始计时，0 为不检查。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("持续秒数"), _T("低于速率持续多少秒后中断，0 为不检查。"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
ARG_INFO g_argVerify[] = {
    { _T("是否校验"), _T("假为跳过证书校验，仅调试用。"), 0, 0, SDT_BOOL, 0, 0 },
};
ARG_INFO g_argCA[] = {
    { _T("证书文件"), _T("CA 打包文件路径，如 cacert.pem；留空用系统或内置。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("证书目录"), _T("存放 CA 的目录。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
ARG_INFO g_argFollow[] = {
    { _T("自动跳转"), _T("是否跟随 3xx 跳转，默认真。"), 0, 0, SDT_BOOL, 0, 0 },
};
ARG_INFO g_argMaxRedirect[] = {
    { _T("次数"), _T("最多跟随多少次跳转，默认 10。"), 0, 0, SDT_INT, 0, 0 },
};
ARG_INFO g_argCookieFile[] = {
    { _T("文件路径"), _T("cookie 持久化文件；留空只在内存里维持。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
ARG_INFO g_argCookieText[] = {
    { _T("cookie文本"), _T("形如 a=1; b=2。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
ARG_INFO g_argHeaderKV[] = {
    { _T("名称"), _T("请求头名称，不区分大小写。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("值"), _T("请求头内容。"), 0, 0, SDT_TEXT, 0, 0 },
};
ARG_INFO g_argHeaderName[] = {
    { _T("名称"), _T("要删除的请求头名称。"), 0, 0, SDT_TEXT, 0, 0 },
};
ARG_INFO g_argDebug[] = {
    { _T("开启调试"), _T("真为记录完整收发过程。"), 0, 0, SDT_BOOL, 0, 0 },
};
ARG_INFO g_argExecute[] = {
    { _T("请求"), _T("已设置好网址等参数的请求对象。"), 0, 0, MAKELONG(CX_TYPE_REQUEST, 0), 0, AS_RECEIVE_VAR },
    { _T("响应"), _T("接收结果的响应对象，执行后被填充。"), 0, 0, MAKELONG(CX_TYPE_RESPONSE, 0), 0, AS_RECEIVE_VAR },
};
ARG_INFO g_argCopySession[] = {
    { _T("源会话"), _T("要复制配置的会话对象。"), 0, 0, MAKELONG(CX_TYPE_SESSION, 0), 0, AS_RECEIVE_VAR },
};
ARG_INFO g_argCopyRequest[] = {
    { _T("源请求"), _T("要复制配置的请求对象。"), 0, 0, MAKELONG(CX_TYPE_REQUEST, 0), 0, AS_RECEIVE_VAR },
};
ARG_INFO g_argCopyResponse[] = {
    { _T("源响应"), _T("要复制内容的响应对象。"), 0, 0, MAKELONG(CX_TYPE_RESPONSE, 0), 0, AS_RECEIVE_VAR },
};
ARG_INFO g_argMethod[] = {
    { _T("方法"), _T("GET / POST / PUT / DELETE / HEAD / PATCH 等；留空自动判断。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
ARG_INFO g_argBodyText[] = {
    { _T("正文"), _T("UTF-8 请求正文，原样发送。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
ARG_INFO g_argBodyBin[] = {
    { _T("数据"), _T("以字节集提交。"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
ARG_INFO g_argOutFile[] = {
    { _T("保存路径"), _T("响应正文直接写文件，不在内存里留副本。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
ARG_INFO g_argResume[] = {
    { _T("起始偏移"), _T("从该字节续传；0 为从头下载。"), 0, 0, SDT_INT, 0, 0 },
};
ARG_INFO g_argAddFile[] = {
    { _T("字段名"), _T("表单字段名。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("文件路径"), _T("本地文件路径。"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("内容类型"), _T("如 application/octet-stream，留空自动判断。"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
ARG_INFO g_argHeadIndex[] = {
    { _T("序号"), _T("从 1 开始的响应头序号。"), 0, 0, SDT_INT, 0, 0 },
};
ARG_INFO g_argHeadName[] = {
    { _T("名称"), _T("响应头名称，不区分大小写。"), 0, 0, SDT_TEXT, 0, 0 },
};
ARG_INFO g_argSaveFile[] = {
    { _T("保存路径"), _T("要把响应正文写到的文件。"), 0, 0, SDT_TEXT, 0, 0 },
};

// ============================================================ 宽文本/正则 参数表
// 迁移自 创想软件UNICODE模块。文本型参数按 UTF-8 进出，宽文本一律用 字节集（UTF-16，惯例末尾带 {0,0}）。

#define UA(nm, expl, type)  { _T(nm), _T(expl), 0, 0, (type), 0, 0 }
#define UAE(nm, expl, type) { _T(nm), _T(expl), 0, 0, (type), 0, AS_DEFAULT_VALUE_IS_EMPTY }
#define UAR(nm, expl, type) { _T(nm), _T(expl), 0, 0, (type), 0, AS_RECEIVE_VAR }
#define UAU(nm, expl, type) { _T(nm), _T(expl), 0, 0, (type), 0, AS_RECEIVE_ARRAY_DATA }
#define UAUE(nm, expl, type) { _T(nm), _T(expl), 0, 0, (type), 0, (AS_RECEIVE_ARRAY_DATA | AS_DEFAULT_VALUE_IS_EMPTY) }
#define UARU(nm, expl, type) { _T(nm), _T(expl), 0, 0, (type), 0, AS_RECEIVE_VAR_ARRAY }
#define UARUE(nm, expl, type) { _T(nm), _T(expl), 0, 0, (type), 0, (AS_RECEIVE_VAR_ARRAY | AS_DEFAULT_VALUE_IS_EMPTY) }
#define UAOB(nm, expl)  UA(nm, expl, SDT_BIN)
#define UAUT(nm, expl)  UA(nm, expl, SDT_TEXT)
#define UANI(nm, expl)  UA(nm, expl, SDT_INT)
#define UAND(nm, expl)  UA(nm, expl, SDT_BOOL)
#define UABE(nm, expl)  UAE(nm, expl, SDT_BIN)
#define UABR(nm, expl)  UAR(nm, expl, SDT_BIN)
#define UAIN(nm, expl)  UAE(nm, expl, SDT_INT)
#define UADN(nm, expl)  UAE(nm, expl, SDT_BOOL)
#define UAA(nm, expl, t) UA(nm, expl, MAKELONG(t, 0))
#define UAAV(nm, expl, t) UAR(nm, expl, MAKELONG(t, 0))
#define UABA(nm, expl) UAU(nm, expl, SDT_BIN)
#define UABAE(nm, expl) UAUE(nm, expl, SDT_BIN)
#define UABAR(nm, expl) UARU(nm, expl, SDT_BIN)
#define UABARE(nm, expl) UARUE(nm, expl, SDT_BIN)
#define UATA_R(nm, expl) UARU(nm, expl, SDT_TEXT)
#define UATA(nm, expl) UAU(nm, expl, SDT_TEXT)

// ---- 全局宽文本命令
static ARG_INFO g_auWB[] = { UAOB("参数_文本", "待处理的宽文本（UTF-16 字节集）。") };
static ARG_INFO g_auWInt[] = { UANI("参数_整数", "整数值。") };
static ARG_INFO g_auWCode[] = { UANI("参数_代码", "Unicode 字符代码。") };
static ARG_INFO g_auWRepeat[] = { UANI("参数_重复次数", "重复次数。") };
static ARG_INFO g_auWBlank[] = { UANI("零字节数目", "要生成的 0 字节个数。") };
static ARG_INFO g_auWPtr[] = { UANI("参数_指针", "以 {0,0} 结束的宽文本指针。") };
static ARG_INFO g_auWBinLen[] = { UAOB("参数_文本", "待处理的宽文本。"), UANI("参数_长度", "取用的字符数。") };
static ARG_INFO g_auWBinPosLen[] = { UAOB("参数_文本", "待处理的宽文本。"),
    UANI("参数_起始位置", "起始位置，从 1 开始。"), UANI("参数_长度", "取用的字符数。") };
static ARG_INFO g_auWBinCodePos[] = { UAOB("参数_文本", "待处理的宽文本。"), UAIN("参数_字符位置", "字符位置，从 1 开始，默认 1。") };
static ARG_INFO g_auWFind[] = { UAOB("参数_被寻找文本", "被搜索的宽文本。"), UAOB("参数_欲寻找文本", "要查找的宽文本。"),
    UAIN("参数_起始搜寻位置", "起始位置，从 1 开始。"), UADN("参数_是否不区分大小写", "真为不区分大小写，默认假。") };
static ARG_INFO g_auW2Bin[] = { UAOB("参数_文本1", "前半段宽文本。"), UAOB("参数_文本2", "后半段宽文本。") };
static ARG_INFO g_auWCmp[] = { UAOB("参数_文本1", "比较的宽文本。"), UAOB("参数_文本2", "被比较的宽文本。"),
    UADN("参数_是否不区分大小写", "真为不区分大小写，默认假。") };
static ARG_INFO g_auWTrim[] = { UAOB("参数_文本", "待处理的宽文本。"), UAOB("参数_要删除的首尾文本", "出现在首尾要被裁掉的字符集合。") };
static ARG_INFO g_auWReplace[] = { UAOB("参数_文本", "待处理的宽文本。"), UANI("参数_起始位置", "起始位置，从 1 开始。"),
    UANI("参数_替换长度", "被替换的字符数。"), UABE("参数_用作替换的文本", "替换用的宽文本。") };
static ARG_INFO g_auWSub[] = { UAOB("参数_文本", "待处理的宽文本。"), UAOB("参数_欲被替换的子文本", "被替换的子文本。"),
    UABE("参数_用作替换的子文本", "替换用的子文本，留空即删除。"), UAIN("参数_进行替换的起始位置", "起始位置，从 1 开始。"),
    UAIN("参数_替换进行的次数", "替换次数，留空或 0 为全部。"), UADN("参数_是否区分大小写", "默认真（区分）。") };
static ARG_INFO g_auWSub2[] = { UAOB("参数_文本", "待处理的宽文本。"), UAUT("参数_欲被替换的子文本", "被替换的子文本（UTF-8）。"),
    UAE("参数_用作替换的子文本", "替换用的子文本（UTF-8）。", SDT_TEXT), UAIN("参数_进行替换的起始位置", "起始位置，从 1 开始。"),
    UAIN("参数_替换进行的次数", "替换次数，留空或 0 为全部。"), UADN("参数_是否区分大小写", "默认真（区分）。") };
static ARG_INFO g_auWRepeatBin[] = { UANI("参数_重复次数", "重复次数。"), UAOB("参数_待重复文本", "要重复的宽文本。") };
static ARG_INFO g_auWSplit[] = { UAOB("参数_文本", "待分割的宽文本。"), UAOB("参数_用作分割的文本", "分隔符。"),
    UABAR("参数_返回的文本数组", "接收分割结果的字节集数组，调用后被重定义。") };
static ARG_INFO g_auWSplitAry[] = { UAOB("参数_文本", "待分割的宽文本。"), UAOB("参数_用作分割的文本", "分隔符。"),
    UADN("是否移除空白文本", "真为丢掉分隔符之间的空片段，默认假保留。") };
static ARG_INFO g_auWTextToInt[] = { UAOB("参数_文本", "待解析的宽文本。"), UADN("参数_解析16进制", "真为按 16 进制解析，默认假。") };
static ARG_INFO g_auWFileName[] = { UAOB("参数_文件名", "文件路径（宽文本）。") };
static ARG_INFO g_auWWriteFile[] = { UAOB("参数_文件名", "文件路径（宽文本）。"), UAOB("参数_数据", "要写入的字节集。") };
static ARG_INFO g_auWMid[] = { UAOB("欲取全文本", "被搜索的宽文本。"), UAOB("前面文本", "左边界。"), UAOB("后面文本", "右边界。"),
    UAIN("起始搜寻位置", "搜索左边界时的起始位置，从 1 开始。"), UADN("是否不区分大小写", "真为不区分大小写。") };
static ARG_INFO g_auWMidT[] = { UAOB("欲取全文本", "被搜索的宽文本。"), UAUT("前面文本", "左边界（UTF-8）。"),
    UAUT("后面文本", "右边界（UTF-8）。"), UAIN("起始搜寻位置", "起始位置，从 1 开始。"), UADN("是否不区分大小写", "真为不区分大小写。") };
static ARG_INFO g_auWRMid[] = { UAOB("欲取全文本", "被搜索的宽文本。"), UAOB("右边文本", "右边界。"), UAOB("左边文本", "左边界。"),
    UAIN("起始搜寻位置", "搜索右边界时的起始位置，默认从末尾开始。"), UADN("是否不区分大小写", "真为不区分大小写。") };
static ARG_INFO g_auWRMidT[] = { UAOB("欲取全文本", "被搜索的宽文本。"), UAUT("右边文本", "右边界（UTF-8）。"),
    UAUT("左边文本", "左边界（UTF-8）。"), UAIN("起始搜寻位置", "起始位置，默认从末尾开始。"), UADN("是否不区分大小写", "真为不区分大小写。") };
static ARG_INFO g_auWSearch4[] = { UAOB("被查找的文本", "被搜索的宽文本。"), UAOB("欲寻找的文本", "要查找的宽文本。"),
    UAIN("起始寻找位置", "起始位置，从 1 开始。"), UADN("是否不区分大小写", "真为不区分大小写。") };
static ARG_INFO g_auWSearch4T[] = { UAOB("被查找的文本", "被搜索的宽文本。"), UAUT("欲寻找的文本", "要查找的文本（UTF-8）。"),
    UAIN("起始寻找位置", "起始位置，从 1 开始。"), UADN("是否不区分大小写", "真为不区分大小写。") };
static ARG_INFO g_auWExists[] = { UAOB("判断的文本", "被搜索的宽文本。"), UAOB("查找的文本", "要查找的宽文本。"),
    UADN("区分大小写", "注意：原版把它当作“不区分大小写”使用，此处保持一致。") };
static ARG_INFO g_auWCount[] = { UAOB("被搜索文本", "被搜索的宽文本。"), UAOB("欲搜索文本", "要统计的宽文本。"),
    UADN("是否区分大小写", "真为区分大小写。") };
static ARG_INFO g_auWCountT[] = { UAUT("被搜索文本", "被搜索的文本（UTF-8）。"), UAUT("欲搜索文本", "要统计的文本（UTF-8）。"),
    UADN("是否区分大小写", "真为区分大小写。") };
static ARG_INFO g_auWRegMatch[] = { UAOB("源文本", "被匹配的宽文本。"), UABE("正则表达式", "正则表达式（宽文本）。"),
    UAIN("返回数量", "返回多少条匹配，默认 1，0 为全部。"), UABE("结果分隔符", "多条结果之间的分隔符。"),
    UADN("是否区分大小写", "默认真。"), UABARE("返回数组", "接收匹配文本的字节集数组，留空不返回数组。") };
static ARG_INFO g_auWRegMatchT[] = { UAOB("源文本", "被匹配的宽文本。"), UAE("正则表达式", "正则表达式（UTF-8）。", SDT_TEXT),
    UAIN("返回数量", "返回多少条匹配，默认 1，0 为全部。"), UABE("结果分隔符", "多条结果之间的分隔符。"),
    UADN("是否区分大小写", "默认真。"), UABARE("返回数组", "接收匹配文本的字节集数组，留空不返回数组。") };
static ARG_INFO g_auWRegAllSub[] = { UAOB("源文本W", "被匹配的宽文本。"), UABE("正则表达式W", "正则表达式（宽文本）。"),
    UAIN("取子匹配索引", "取第几个子匹配，从 1 开始；留空取主匹配。"), UAIN("返回数量", "默认 1，0 为全部。"),
    UABE("结果分隔符W", "多条结果之间的分隔符。"), UADN("是否区分大小写", "默认真。"),
    UABARE("返回数组W", "接收结果的字节集数组，留空不返回数组。") };
static ARG_INFO g_auWOnlyDigits[] = { UAOB("文本", "要从中取数字的宽文本。") };
static ARG_INFO g_auWRegReplace[] = { UABR("源文本W", "被替换的宽文本，替换结果写回该变量。"), UAOB("正则表达式", "正则表达式。"),
    UABE("替换的文本W", "替换成的宽文本，留空即剔除。"), UAIN("替换次数", "保留参数，与原版一样不参与循环控制。"),
    UADN("是否区分大小写", "默认真。"), UADN("全局替换", "真为把匹配到的子串全文替换。") };
static ARG_INFO g_auWRegSubReplace[] = { UABR("源文本W", "被替换的宽文本，替换结果写回该变量。"), UAOB("正则表达式", "正则表达式。"),
    UABE("替换的文本W", "替换成的宽文本，留空即剔除。"), UAIN("子匹配索引", "取第几个子匹配，默认 1。"),
    UAIN("替换次数", "保留参数，与原版一样不参与循环控制。"), UADN("是否区分大小写", "默认真。"),
    UADN("全局替换", "真为把匹配到的子串全文替换。") };
static ARG_INFO g_auWAryJoin[] = { UABAE("数组", "要拼接的字节集数组。"), UABE("分隔符", "拼接用的分隔符。") };
static ARG_INFO g_auWHtmlMid[] = { UAOB("欲取全文本", "网页源码。"), UAOB("前面文本", "起始标签。"),
    UAIN("起始搜寻位置", "起始位置，从 1 开始。"), UADN("是否不区分大小写", "真为不区分大小写。") };
static ARG_INFO g_auWShuffle[] = { UABAE("文本数组", "要打乱的字节集数组。"),
    UABAR("返回的文本数组", "接收结果的字节集数组，会被重定义。") };
static ARG_INFO g_auWAryOut[] = { UABAR("文本数组", "要输出的字节集数组。"), UAOB("分隔符", "分隔符。") };
static ARG_INFO g_auWSubAry[] = { UAOB("待替换文本", "被处理的宽文本。"), UABAE("要替换的数组文本", "逐个用于查找的宽文本数组。"),
    UAOB("替换的文本", "替换成的宽文本。"), UAND("是否区分大小写", "真为区分大小写。") };
static ARG_INFO g_auWUnique[] = { UABARE("文本数组", "要去重的字节集数组，结果写回该变量。"),
    UADN("不区分大小写", "真为不区分大小写，默认假。") };
static ARG_INFO g_auWBatch[] = { UAOB("源文本", "被搜索的宽文本。"), UAOB("前面标识文本", "左边界。"), UAOB("后面标识文本", "右边界。"),
    UABAR("存放取出文本的数组", "接收结果的字节集数组。"), UADN("是否区分大小写", "真为区分大小写。"),
    UAIN("文本长度大于此数的不要", "超过该字符数的结果被丢弃，0 为不限制。"), UABE("为取出的文本加上前缀", "附加在结果前的宽文本。"),
    UABE("为取出的文本加上后缀", "附加在结果后的宽文本。"), UADN("连前面标识文本一起取出", "真为把左边界一起取出。"),
    UADN("连后面标识文本一起取出", "真为把右边界一起取出。"), UAIN("起始搜寻位置", "起始位置，从 1 开始。"),
    UADN("结果是否删首尾空", "真为裁掉结果首尾空格，默认真。"), UADN("自动清空数组", "真为先清空数组，默认真。") };
static ARG_INFO g_auWBatchT[] = { UAOB("源文本", "被搜索的宽文本。"), UAUT("前面标识文本", "左边界（UTF-8）。"),
    UAUT("后面标识文本", "右边界（UTF-8）。"), UABAR("存放取出文本的数组", "接收结果的字节集数组。"),
    UADN("是否区分大小写", "真为区分大小写。"), UAIN("文本长度大于此数的不要", "超过该字符数的结果被丢弃。"),
    UABE("为取出的文本加上前缀", "附加在结果前的宽文本。"), UABE("为取出的文本加上后缀", "附加在结果后的宽文本。"),
    UADN("连前面标识文本一起取出", "真为把左边界一起取出。"), UADN("连后面标识文本一起取出", "真为把右边界一起取出。"),
    UAIN("起始搜寻位置", "起始位置，从 1 开始。"), UADN("结果是否删首尾空", "默认真。") };
static ARG_INFO g_auWBatch3[] = { UAOB("源文本", "被搜索的宽文本。"), UAOB("前面标识文本", "左边界。"), UAOB("后面标识文本", "右边界。"),
    UATA_R("存放取出文本的数组", "接收结果的文本型数组，成员为 UTF-8。"), UADN("是否区分大小写", "真为区分大小写。"),
    UAIN("文本长度大于此数的不要", "超过该字符数的结果被丢弃。"), UABE("为取出的文本加上前缀", "附加在结果前的宽文本。"),
    UABE("为取出的文本加上后缀", "附加在结果后的宽文本。"), UADN("连前面标识文本一起取出", "真为把左边界一起取出。"),
    UADN("连后面标识文本一起取出", "真为把右边界一起取出。"), UAIN("起始搜寻位置", "起始位置，从 1 开始。"),
    UADN("结果是否删首尾空", "默认真。") };

// ---- 扩展模块命令
static ARG_INFO g_auWFormat[] = { UAOB("文本", "待格式化的宽文本。") };
static ARG_INFO g_auWFormat2[] = { UABR("字节集", "待补齐 {0,0} 的宽文本，结果写回该变量。") };
static ARG_INFO g_auWVar[] = { { _T("参数_变量"), _T("任意类型的变量，返回其在内存中的地址。"), 0, 0, _SDT_ALL, 0, AS_RECEIVE_VAR } };
static ARG_INFO g_auWStruct[] = { { _T("参数_自定义数据类型"), _T("自定义数据类型变量，返回其数据指针。"), 0, 0, _SDT_ALL, 0, AS_RECEIVE_VAR } };
static ARG_INFO g_auWThisCall[] = { UANI("this", "对象指针。"), UANI("index", "虚表槽位序号。"),
    UAIN("参数1", "第 1 个参数。"), UAIN("参数2", "第 2 个参数。"), UAIN("参数3", "第 3 个参数。"),
    UAIN("参数4", "第 4 个参数。"), UAIN("参数5", "第 5 个参数。"), UAIN("参数6", "第 6 个参数。"),
    UAIN("参数7", "第 7 个参数。"), UAIN("参数8", "第 8 个参数。") };
static ARG_INFO g_auWWF[] = { UAOB("欲被处理的字节集", "待处理的宽文本。"), UADN("标准Unicode编码", "真为按终止符截断并补齐 {0,0}，默认假只去末尾 {0,0}。") };
static ARG_INFO g_auWIsEmpty[] = { UAOB("判断", "待判断的宽文本。") };
static ARG_INFO g_auWEqual[] = { UAOB("a", "比较的宽文本。"), UAOB("b", "被比较的宽文本。") };
static ARG_INFO g_auWToDouble[] = { UAOB("宽文本", "待解析的宽文本。") };
static ARG_INFO g_auWDouble[] = { UA("数值", "要格式化的双精度小数。", SDT_DOUBLE) };

// ---- 宽文本类成员
#define WS_OBJ(nm, expl) UAAV(nm, expl, CX_TYPE_WSTR)
static ARG_INFO g_auWsNoneArr[] = { { _T("源对象"), _T("要复制内容的宽文本对象。"), 0, 0, MAKELONG(CX_TYPE_WSTR, 0), 0, AS_RECEIVE_VAR } };
static ARG_INFO g_auWsPtr[] = { UANI("参数_对象指针", "由 取指针() 得到的对象指针。") };
static ARG_INFO g_auWsCap[] = { UAIN("参数_缓冲区大小", "初始容量（字符数），默认 11。") };
static ARG_INFO g_auWsRes[] = { UANI("参数_资源ID", "字符串资源 ID。"), UANI("参数_缓冲区长度", "读取的最大字符数。") };
static ARG_INFO g_auWsPtrW[] = { UANI("参数_文本指针", "宽文本指针。"), UAIN("参数_长度", "字符数，留空按终止符计算。") };
static ARG_INFO g_auWsPtrA[] = { UANI("参数_文本指针", "ANSI 文本指针。"), UAIN("参数_长度", "字节数，留空按终止符计算。"),
    UAIN("参数_代码页", "源文本代码页，默认 936。") };
static ARG_INFO g_auWsT[] = { UAUT("参数_文本", "要添加的文本（UTF-8）。") };
static ARG_INFO g_auWsB[] = { UAOB("参数_文本", "要添加的宽文本。") };
static ARG_INFO g_auWsSelf[] = { WS_OBJ("参数_文本", "要添加的宽文本对象。") };
static ARG_INFO g_auWsInt[] = { UANI("参数_整数", "要添加的整数。") };
static ARG_INFO g_auWsInsT[] = { UAUT("参数_文本", "要插入的文本（UTF-8）。"), UAIN("参数_位置", "插入位置，从 0 开始。") };
static ARG_INFO g_auWsInsB[] = { UAOB("参数_文本", "要插入的宽文本。"), UAIN("参数_位置", "插入位置，从 0 开始。") };
static ARG_INFO g_auWsInsSelf[] = { WS_OBJ("参数_文本", "要插入的宽文本对象。"), UAIN("参数_位置", "插入位置，从 0 开始。") };
static ARG_INFO g_auWsChar[] = { UANI("参数_字符", "Unicode 字符代码。") };
static ARG_INFO g_auWsGetChar[] = { UANI("参数_索引", "字符索引，从 0 开始。") };
static ARG_INFO g_auWsSetChar[] = { UANI("参数_索引", "字符索引，从 0 开始。"), UANI("参数_字符代码", "Unicode 字符代码。") };
static ARG_INFO g_auWsCmpT[] = { UAUT("参数_文本", "比较的文本（UTF-8）。") };
static ARG_INFO g_auWsCmpB[] = { UAOB("参数_文本", "比较的宽文本。") };
static ARG_INFO g_auWsCmpSelf[] = { WS_OBJ("参数_String", "比较的宽文本对象。") };
static ARG_INFO g_auWsFindB[] = { UAOB("参数_欲寻找文本", "要查找的宽文本。"), UAIN("参数_起始搜寻位置", "从 0 开始。"),
    UADN("参数_是否不区分大小写", "真为不区分大小写。") };
static ARG_INFO g_auWsFindT[] = { UAUT("参数_欲寻找文本", "要查找的文本（UTF-8）。"), UAIN("参数_起始搜寻位置", "从 0 开始。"),
    UADN("参数_是否不区分大小写", "真为不区分大小写。") };
static ARG_INFO g_auWsFindSelf[] = { WS_OBJ("参数_欲寻找文本", "要查找的宽文本对象。"), UAIN("参数_起始搜寻位置", "从 0 开始。"),
    UADN("参数_是否不区分大小写", "真为不区分大小写。") };
static ARG_INFO g_auWsReplaceB[] = { UANI("参数_起始位置", "起始位置，从 0 开始。"), UANI("参数_替换长度", "被替换的字符数。"),
    UABE("参数_用作替换的文本", "替换用的宽文本。") };
static ARG_INFO g_auWsReplaceT[] = { UANI("参数_起始位置", "起始位置，从 0 开始。"), UANI("参数_替换长度", "被替换的字符数。"),
    UAE("参数_用作替换的文本", "替换用的文本（UTF-8）。", SDT_TEXT) };
static ARG_INFO g_auWsReplaceSelf[] = { UANI("参数_起始位置", "起始位置，从 0 开始。"), UANI("参数_替换长度", "被替换的字符数。"),
    UAAV("参数_用作替换的文本", "替换用的宽文本对象。", CX_TYPE_WSTR) };
static ARG_INFO g_auWsSubB[] = { UAOB("参数_欲被替换的子文本", "被替换的宽文本。"), UABE("参数_用作替换的子文本", "替换用的宽文本。"),
    UAIN("参数_进行替换的起始位置", "起始位置，从 0 开始。"), UAIN("参数_替换进行的次数", "留空或 0 为全部。"),
    UADN("参数_是否区分大小写", "注意：原版把它当作“不区分大小写”使用，默认真。") };
static ARG_INFO g_auWsSubT[] = { UAUT("参数_欲被替换的子文本", "被替换的文本（UTF-8）。"),
    UAE("参数_用作替换的子文本", "替换用的文本（UTF-8）。", SDT_TEXT), UAIN("参数_进行替换的起始位置", "起始位置，从 0 开始。"),
    UAIN("参数_替换进行的次数", "留空或 0 为全部。"), UADN("参数_是否区分大小写", "同上，默认真。") };
static ARG_INFO g_auWsSubSelf[] = { WS_OBJ("参数_欲被替换的子文本", "被替换的宽文本对象。"),
    UAAV("参数_用作替换的子文本", "替换用的宽文本对象。", CX_TYPE_WSTR), UAIN("参数_进行替换的起始位置", "起始位置，从 0 开始。"),
    UAIN("参数_替换进行的次数", "留空或 0 为全部。"), UADN("参数_是否区分大小写", "同上，默认真。") };
static ARG_INFO g_auWsMid[] = { UANI("参数_开始位置", "开始位置，从 0 开始。"), UANI("参数_长度", "取用的字符数。") };
static ARG_INFO g_auWsLen[] = { UANI("参数_长度", "取用的字符数。") };
static ARG_INFO g_auWsTrimSelf[] = { WS_OBJ("参数_要删除的首尾文本", "字符集合所在的宽文本对象。") };
static ARG_INFO g_auWsToInt[] = { UADN("参数_解析16进制", "真为按 16 进制解析。") };
static ARG_INFO g_auWsSplitB[] = { UAOB("参数_用作分割的文本", "分隔符。"), UABAR("参数_返回的文本数组", "接收分割结果的字节集数组。"),
    UADN("参数_是否移除空", "真为丢弃空片段，默认真。") };
static ARG_INFO g_auWsSplitT[] = { UAUT("参数_用作分割的文本", "分隔符（UTF-8）。"),
    UATA_R("参数_返回的文本数组", "接收分割结果的文本型数组（UTF-8）。"), UADN("参数_是否移除空", "默认真。") };
static ARG_INFO g_auWsSplitSelf[] = { WS_OBJ("参数_用作分割的文本", "分隔符对象。"),
    { _T("参数_返回的文本数组"), _T("接收分割结果的宽文本数组。"), 0, 0, MAKELONG(CX_TYPE_WSTR, 0), 0, AS_RECEIVE_VAR_ARRAY },
    UADN("参数_是否移除空", "默认真。") };

// ---- 正则类成员
#define RX_OBJ(nm, expl) UAAV(nm, expl, CX_TYPE_RXW)
#define RXC_OBJ(nm, expl) UAAV(nm, expl, CX_TYPE_RXCW)
#define RXC2_OBJ(nm, expl) UAAV(nm, expl, CX_TYPE_RXC2)
#define RXS_OBJ(nm, expl) UAAV(nm, expl, CX_TYPE_RXSW)
static ARG_INFO g_auRxCopy[] = { { _T("源正则"), _T("要复制表达式与文本的正则对象。"), 0, 0, MAKELONG(CX_TYPE_RXW, 0), 0, AS_RECEIVE_VAR } };
static ARG_INFO g_auRxcCopy[] = { { _T("源正则"), _T("要复制表达式与文本的正则对象。"), 0, 0, MAKELONG(CX_TYPE_RXCW, 0), 0, AS_RECEIVE_VAR } };
static ARG_INFO g_auRxc2Copy[] = { { _T("源正则"), _T("要复制表达式与文本的正则对象。"), 0, 0, MAKELONG(CX_TYPE_RXC2, 0), 0, AS_RECEIVE_VAR } };
static ARG_INFO g_auRxsCopy[] = { { _T("源正则"), _T("要复制表达式与文本的正则对象。"), 0, 0, MAKELONG(CX_TYPE_RXSW, 0), 0, AS_RECEIVE_VAR } };
static ARG_INFO g_auRxCreateB[] = { UAOB("正则表达式文本", "正则表达式（宽文本）。"), UAOB("被搜索的文本", "待匹配的宽文本。"),
    UADN("是否区分大小写", "默认为假，即不区分大小写。"), UADN("是否匹配多行", "默认真。"), UADN("是否全局匹配", "默认真。"),
    UADN("自动替换引号", "真为把 #引号 还原成双引号，默认假。") };
static ARG_INFO g_auRxCreateB5[] = { UAOB("正则表达式文本", "正则表达式（宽文本）。"), UAOB("被搜索的文本", "待匹配的宽文本。"),
    UADN("是否区分大小写", "默认为假。"), UADN("是否匹配多行", "默认真。"), UADN("是否全局匹配", "默认真。") };
static ARG_INFO g_auRxCreateT6[] = { UAUT("正则表达式文本", "正则表达式（UTF-8）。"), UAUT("被搜索的文本", "待匹配的文本（UTF-8）。"),
    UADN("是否区分大小写", "默认为假。"), UADN("是否匹配多行", "默认真。"), UADN("是否全局匹配", "默认真。"),
    UADN("自动替换引号", "真为把 #引号 还原成双引号。") };
static ARG_INFO g_auRxCreateT4[] = { UAUT("正则表达式文本", "正则表达式（UTF-8）。"), UAUT("被搜索的文本", "待匹配的文本（UTF-8）。"),
    UADN("是否区分大小写", "默认为假。"), UADN("是否匹配多行", "默认为真（原版取反使用）。") };
static ARG_INFO g_auRxReplaceB[] = { UABE("用作替换的文本", "替换用的宽文本。") };
static ARG_INFO g_auRxReplaceT[] = { UAE("用作替换的文本", "替换用的文本（UTF-8）。", SDT_TEXT) };
static ARG_INFO g_auRxValue[] = { UANI("匹配索引", "匹配序号，从 1 开始。"),
    { _T("起始位置"), _T("可选变量地址，调用后写入该匹配在原文中的起始位置。"), 0, 0, SDT_INT, 0, (AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY) } };
static ARG_INFO g_auRxSubValue[] = { UANI("匹配索引", "匹配序号，从 1 开始。"), UANI("子表达式索引", "子表达式序号，从 1 开始。"),
    { _T("起始位置"), _T("可选变量地址，调用后写入该子匹配在原文中的起始位置。"), 0, 0, SDT_INT, 0, (AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY) } };
static ARG_INFO g_auRxValue1[] = { UANI("匹配索引", "匹配序号，从 1 开始。") };
static ARG_INFO g_auRxSubCount1[] = { UANI("匹配索引", "匹配序号，从 1 开始。") };
static ARG_INFO g_auRxSubValue2[] = { UANI("匹配索引", "匹配序号，从 1 开始。"), UANI("子表达式索引", "子表达式序号，从 1 开始。") };
static ARG_INFO g_auRxsCreate[] = { UAUT("正则表达式文本", "正则表达式（UTF-8）。"), UAND("是否区分大小写", "真为区分大小写。"),
    UAND("全局匹配", "真为全局匹配。") };
static ARG_INFO g_auRxsCreateW[] = { UAOB("正则表达式文本", "正则表达式（宽文本）。"), UAND("是否区分大小写", "真为区分大小写。"),
    UAND("全局匹配", "真为全局匹配。") };
static ARG_INFO g_auRxsText[] = { UAUT("欲匹配的文本", "待匹配的文本（UTF-8）。") };
static ARG_INFO g_auRxsTextW[] = { UAOB("欲搜索的文本", "待匹配的宽文本。") };

// ---- 编码转换命令（代码页参数可空，留空按原版默认 936）
static ARG_INFO g_ucText1[] = { UAUT("文本", "源文本（按命令名指定的编码解释）。") };
static ARG_INFO g_ucBin1[] = { UAOB("字节集", "源字节集。") };
static ARG_INFO g_ucWide1[] = { UAOB("Unicode字节集", "UTF-16 宽文本字节集。") };
static ARG_INFO g_ucWideCp[] = { UAOB("参数_Unicode文本", "UTF-16 宽文本字节集。"),
    UAIN("参数_目标代码页", "目标 ANSI 代码页，默认为 936（简体中文）。") };
static ARG_INFO g_ucTextCp[] = { UAUT("参数_文本", "源文本。"),
    UAIN("参数_代码页", "源文本代码页，默认为 936（简体中文）；按 UTF-8 解释填 65001。") };
static ARG_INFO g_ucTextTargetCp[] = { UAUT("待转", "源文本。"),
    UAIN("目标代码页", "目标代码页，默认为 936（简体中文）。") };
static ARG_INFO g_ucAnsiCp[] = { UAUT("参数_Ansi文本", "ANSI 文本。"),
    UAIN("参数_被转代码页", "源文本代码页，默认为 936（简体中文）。") };
static ARG_INFO g_ucUtf8BinCp[] = { UAOB("utf8字节集", "UTF-8 字节集。"),
    UAIN("目标代码页", "目标代码页，默认为 936（简体中文）。") };
static ARG_INFO g_ucSrc[] = { UAUT("欲转换的源码", "源文本。") };
static ARG_INFO g_ucTrimNull[] = { UAOB("欲被处理的字节集", "宽文本字节集。"),
    UADN("标准Unicode编码", "真为按终止符截断并补齐 {0,0}，默认假只处理末尾。") };
static ARG_INFO g_ucUrlDecode[] = { UAUT("URL文本", "被百分号编码的文本。"),
    UADN("是否UTF8", "真为把解出的 UTF-8 字节再转成 936 文本，默认假原样返回。") };
static ARG_INFO g_ucUrlDecodeJs[] = { UAUT("参_待解码文本", "被百分号编码的文本。"),
    UAIN("参_类型", "0 = decodeURI（保留字符的转义原样留着），1 = decodeURIComponent，默认为 0。") };
static ARG_INFO g_ucHtml[] = { UAUT("欲转换的文本", "含 HTML 实体的文本，如 &#31934;&#26131;。") };

// ==== JSON2 参数表（gen_json2_table.mjs 生成，勿手改）====
static ARG_INFO g_j2ObjCopyArgs[] = {
    { _T("源对象"), _T("要复制的 Json2 对象。"), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, AS_RECEIVE_VAR },
};
static ARG_INFO g_j2SetCodeArgs[] = {
    { _T("参数_JSON文本"), _T(""), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_Unicode解码"), _T("默认为假，如果为真，则表示要将 \\uxxxx 样式的Unicode字符解码"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_错误代码"), _T("“JSON_”开头常量"), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_错误位置"), _T(""), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetCodeUtf8Args[] = {
    { _T("参数_JSON文本"), _T(""), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_Unicode解码"), _T("默认为假，如果为真，则表示要将 \\uxxxx 样式的Unicode字符解码"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_错误代码"), _T("“JSON_”开头常量"), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_错误位置"), _T(""), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetCodeWArgs[] = {
    { _T("参数_JSON文本"), _T(""), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_Unicode解码"), _T("默认为假，如果为真，则表示要将 \\uxxxx 样式的Unicode字符解码"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_错误代码"), _T("“JSON_”开头常量"), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_错误位置"), _T(""), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetTopFlagArgs[] = {
    { _T("是否为顶层"), _T(""), 0, 0, SDT_BOOL, 0, 0 },
};
static ARG_INFO g_j2SetPtrArgs[] = {
    { _T("参数_指针"), _T(""), 0, 0, SDT_INT, 0, 0 },
};
static ARG_INFO g_j2CreateBoolArgs[] = {
    { _T("参数_逻辑值"), _T(""), 0, 0, SDT_BOOL, 0, 0 },
};
static ARG_INFO g_j2CreateTextWArgs[] = {
    { _T("参数_文本值"), _T(""), 0, 0, SDT_BIN, 0, 0 },
};
static ARG_INFO g_j2CreateTextArgs[] = {
    { _T("参数_文本值"), _T(""), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_j2CreateDoubleArgs[] = {
    { _T("参数_双精度值"), _T(""), 0, 0, SDT_DOUBLE, 0, 0 },
};
static ARG_INFO g_j2CreateLongArgs[] = {
    { _T("参数_长整数值"), _T(""), 0, 0, SDT_INT64, 0, 0 },
};
static ARG_INFO g_j2ParseWArgs[] = {
    { _T("参数_json文本"), _T(""), 0, 0, SDT_BIN, 0, 0 },
    { _T("参数_Unicode解码"), _T("默认为假，如果为真，则表示要将 \\uxxxx 样式的Unicode字符解码"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_错误代码"), _T("“JSON_”开头常量"), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_错误位置"), _T(""), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2ParseArgs[] = {
    { _T("参数_json文本"), _T(""), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_Unicode解码"), _T("默认为假，如果为真，则表示要将 \\uxxxx 样式的Unicode字符解码"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_错误代码"), _T("“JSON_”开头常量"), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_错误位置"), _T(""), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2ToTextWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_格式化"), _T("默认为：假，如果为真：则表示自动添加换行符和制表符"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_是否转义"), _T("默认为：真"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_初始缓存大小"), _T("默认：1024"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2ToTextArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_格式化"), _T("默认为：假，如果为真：则表示自动添加换行符和制表符"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_是否转义"), _T("默认为：真"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_初始缓存大小"), _T("默认：1024"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetTypArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetTypWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetParentArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetParentWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetKeyCountArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetKeyCountWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2RemoveAttrWArgs[] = {
    { _T("参数_属性名"), _T(""), 0, 0, SDT_BIN, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2RemoveAttrArgs[] = {
    { _T("参数_属性名"), _T(""), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2RemoveItemArgs[] = {
    { _T("参数_索引"), _T(""), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2RemoveItemWArgs[] = {
    { _T("参数_索引"), _T(""), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2ClearArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2ClearWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetAttrArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_属性名"), _T("注意仅仅是当前对象下的某个属性名，非路径"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_值"), _T(""), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, 0 },
};
static ARG_INFO g_j2SetAttrWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_属性名"), _T("注意仅仅是当前对象下的某个属性名，非路径"), 0, 0, SDT_BIN, 0, 0 },
    { _T("参数_值"), _T(""), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, 0 },
};
static ARG_INFO g_j2KArgs[] = {
    { _T("参数_属性名"), _T("注意仅仅是当前对象下的某个属性名，非路径"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2KUtf8Args[] = {
    { _T("参数_属性名"), _T("UTF8编码 注意仅仅是当前对象下的某个属性名，非路径"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_路径"), _T("UTF8编码 如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2KWArgs[] = {
    { _T("参数_属性名"), _T("注意仅仅是当前对象下的某个属性名，非路径"), 0, 0, SDT_BIN, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2K2Args[] = {
    { _T("参数_属性名"), _T("注意仅仅是当前对象下的某个属性名，非路径"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2K2WArgs[] = {
    { _T("参数_属性名"), _T("注意仅仅是当前对象下的某个属性名，非路径"), 0, 0, SDT_BIN, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetAttrNameWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetAttrNameArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2AppendItemArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_成员"), _T(""), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, 0 },
};
static ARG_INFO g_j2AppendItemWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_成员"), _T(""), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, 0 },
};
static ARG_INFO g_j2InsertItemArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_索引"), _T("索引从0开始"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_成员"), _T(""), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, 0 },
};
static ARG_INFO g_j2InsertItemWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_索引"), _T("索引从0开始"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_成员"), _T(""), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, 0 },
};
static ARG_INFO g_j2IArgs[] = {
    { _T("参数_索引"), _T("索引从0开始"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2IWArgs[] = {
    { _T("参数_索引"), _T("索引从0开始"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2I2Args[] = {
    { _T("参数_索引"), _T("索引从0开始"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2I2WArgs[] = {
    { _T("参数_索引"), _T("索引从0开始"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetItemArgs[] = {
    { _T("参数_索引"), _T(""), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_值"), _T(""), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, 0 },
};
static ARG_INFO g_j2SetItemWArgs[] = {
    { _T("参数_索引"), _T(""), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_值"), _T(""), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, 0 },
};
static ARG_INFO g_j2ExistsWArgs[] = {
    { _T("参数_路径"), _T("支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, 0 },
};
static ARG_INFO g_j2ExistsArgs[] = {
    { _T("参数_路径"), _T("支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_j2SelectArgs[] = {
    { _T("参数_路径"), _T("支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, 0 },
};
static ARG_INFO g_j2SelectWArgs[] = {
    { _T("参数_路径"), _T("支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, 0 },
};
static ARG_INFO g_j2SetObjectArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetObjectWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetArrayArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetArrayWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetTextArgs[] = {
    { _T("参数_文本值"), _T(""), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetTextUtf8Args[] = {
    { _T("参数_文本值"), _T("UTF8格式"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetTextWArgs[] = {
    { _T("参数_文本值"), _T(""), 0, 0, SDT_BIN, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetNullWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetNullArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetBoolWArgs[] = {
    { _T("参数_逻辑值"), _T(""), 0, 0, SDT_BOOL, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetBoolArgs[] = {
    { _T("参数_逻辑值"), _T(""), 0, 0, SDT_BOOL, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetDoubleWArgs[] = {
    { _T("参数_双精度"), _T(""), 0, 0, SDT_DOUBLE, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetDoubleArgs[] = {
    { _T("参数_双精度"), _T(""), 0, 0, SDT_DOUBLE, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetNumberWArgs[] = {
    { _T("参数_双精度"), _T(""), 0, 0, SDT_DOUBLE, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetNumberArgs[] = {
    { _T("参数_双精度"), _T(""), 0, 0, SDT_DOUBLE, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetLongWArgs[] = {
    { _T("参数_长整数"), _T(""), 0, 0, SDT_INT64, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetLongArgs[] = {
    { _T("参数_长整数"), _T(""), 0, 0, SDT_INT64, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetIntWArgs[] = {
    { _T("参数_整数"), _T(""), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SetIntArgs[] = {
    { _T("参数_整数"), _T(""), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetRawArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetRawWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetTextArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_格式化"), _T("默认为：假，如果为真：则表示自动添加换行符和制表符"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_是否转义"), _T("默认为：真"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_初始缓存大小"), _T("默认：1024"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetTextUtf8Args[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_格式化"), _T("默认为：假，如果为真：则表示自动添加换行符和制表符"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_是否转义"), _T("默认为：真"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_初始缓存大小"), _T("默认：1024"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetTextWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_格式化"), _T("默认为：假，如果为真：则表示自动添加换行符和制表符"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_是否转义"), _T("默认为：真"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_初始缓存大小"), _T("默认：1024"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetBoolWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetBoolAArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetDoubleWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetDoubleAArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetNumberWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetNumberAArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetLongWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetLongAArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetIntWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetIntAArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetAllKeysWArgs[] = {
    { _T("参数_属性名"), _T(""), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY | AS_RECEIVE_ARRAY_DATA },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetAllKeysArgs[] = {
    { _T("参数_属性名"), _T(""), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY | AS_RECEIVE_ARRAY_DATA },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2GetAllValuesWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_值"), _T(""), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, AS_RECEIVE_VAR_ARRAY },
};
static ARG_INFO g_j2GetAllValuesArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_值"), _T(""), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, AS_RECEIVE_VAR_ARRAY },
};
static ARG_INFO g_j2CopyWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2CopyArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SortObjWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_降序"), _T(""), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SortObjArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_降序"), _T(""), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SortWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_回调函数"), _T("返回值：整数型 （整数型 JsonValue1，整数型 JsonValue2，整数型 回调参数）"), 0, 0, SDT_SUB_PTR, 0, 0 },
    { _T("参数_回调参数"), _T(""), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_算法"), _T("0、快速排序，1、希尔排序"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2SortArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_回调函数"), _T("返回值：整数型 （整数型 JsonValue1，整数型 JsonValue2，整数型 回调参数）"), 0, 0, SDT_SUB_PTR, 0, 0 },
    { _T("参数_回调参数"), _T(""), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_算法"), _T("0、快速排序，1、希尔排序"), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2MergeWArgs[] = {
    { _T("参数_被合并json文本"), _T(""), 0, 0, SDT_BIN, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_USC2解码"), _T("默认为假"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_是否添加"), _T("如果 对象属性名和“参数_被合并json文本”的属性名 不相同，选择是否添加进本对象。默认为假"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2MergeArgs[] = {
    { _T("参数_被合并json文本"), _T(""), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_USC2解码"), _T("默认为假"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_是否添加"), _T("如果 对象属性名和“参数_被合并json文本”的属性名 不相同，选择是否添加进本对象。默认为假"), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2MergeObjPlusArgs[] = {
    { _T("参数_对象"), _T("本参数必须是 JV类型_对象 类型"), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2MergeObjArgs[] = {
    { _T("参数_对象"), _T("本参数必须是 JV类型_对象 类型"), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2MergeArrArgs[] = {
    { _T("参数_数组"), _T("本参数必须是 JV类型_数组 类型"), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, 0 },
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
};
static ARG_INFO g_j2InsertAttrWArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_BIN, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_索引"), _T("插入的位置，如果 位置< 0 或 位置>成员数 则本方法会失败"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_属性名"), _T("插入的属性名"), 0, 0, SDT_BIN, 0, 0 },
    { _T("参数_值"), _T(""), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, 0 },
};
static ARG_INFO g_j2InsertAttrArgs[] = {
    { _T("参数_路径"), _T("如果留空，则表示当前值。支持：a.b.c[0] 或 [0].a.b.c。如果属性名包含.或 [ ]，可使用转义符\\. \\[ 和 \\]，\\\\转义自身"), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
    { _T("参数_索引"), _T("插入的位置，如果 位置< 0 或 位置>成员数 则本方法会失败"), 0, 0, SDT_INT, 0, 0 },
    { _T("参数_属性名"), _T("插入的属性名"), 0, 0, SDT_TEXT, 0, 0 },
    { _T("参数_值"), _T(""), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, 0 },
};
static ARG_INFO g_j2SetTextAryArgs[] = {
    { _T("文本数组"), _T(""), 0, 0, SDT_TEXT, 0, AS_RECEIVE_ARRAY_DATA },
};
static ARG_INFO g_j2SetTextAryWArgs[] = {
    { _T("文本数组"), _T(""), 0, 0, SDT_BIN, 0, AS_RECEIVE_ARRAY_DATA },
};
static ARG_INFO g_j2SetTextAryUtf8Args[] = {
    { _T("文本数组"), _T(""), 0, 0, SDT_TEXT, 0, AS_RECEIVE_ARRAY_DATA },
};
static ARG_INFO g_j2SetNumberAryUtf8Args[] = {
    { _T("数组"), _T(""), 0, 0, SDT_DOUBLE, 0, AS_RECEIVE_ARRAY_DATA },
};
static ARG_INFO g_j2SetIntAryUtf8Args[] = {
    { _T("数组"), _T(""), 0, 0, SDT_INT, 0, AS_RECEIVE_ARRAY_DATA },
};
static ARG_INFO g_j2SetBoolAryUtf8Args[] = {
    { _T("数组"), _T(""), 0, 0, SDT_BOOL, 0, AS_RECEIVE_ARRAY_DATA },
};
// ==== JSON2 参数表 结束 ====
// ============================================================ HTTP访问 族（原 zyCurl 模块库化）
// 与上面的 浏览器会话/请求/响应 完全隔离：自己持有 easy 句柄与响应缓冲，
// 只复用同一份 curl-impersonate 绑定（g_curl / LoadCurlApi）。
#include "http访问/cxhttp_args.inc"
#include "线程锁/lock_args.inc"
#include "http访问/cxhttp_cancel_args.inc"
#include "线程锁/join_args.inc"
#include "线程锁/sync_state_args.inc"
#include "线程锁/cancel_args.inc"

// ============================================================ 命令表
// 只能往末尾追加：插入、删除、改名都会让已发布的程序按序号调错命令。

#define CX_CMD(cn, eg, explain, cat, ret, argc, args) \
    { _T(cn), _T(eg), _T(explain), (cat), _CMD_OS(__OS_WIN), (ret), 0, LVL_SIMPLE, 0, 0, (argc), (args) }

#define CX_OBJ(cn, eg, explain, state, ret, argc, args) \
    { _T(cn), _T(eg), _T(explain), -1, (state), (ret), 0, LVL_SIMPLE, 0, 0, (argc), (args) }

// 返回数组的命令：m_wState 带 CT_RETRUN_ARY_TYPE_DATA，m_dtRetValType 填元素类型。
#define CX_CMD_AR(cn, eg, explain, cat, ret, argc, args) \
    { _T(cn), _T(eg), _T(explain), (cat), _CMD_OS(__OS_WIN) | CT_RETRUN_ARY_TYPE_DATA, (ret), 0, LVL_SIMPLE, 0, 0, (argc), (args) }

#define CX_LIFECYCLE(prefix, cnPrefix, egPrefix, copyArg, explain)                                  \
    CX_OBJ(cnPrefix "构造函数", egPrefix "A", explain, CT_IS_HIDED | CT_IS_OBJ_CONSTURCT_CMD, _SDT_NULL, 0, NULL), \
    CX_OBJ(cnPrefix "复制函数", egPrefix "B", explain, CT_IS_HIDED | CT_IS_OBJ_COPY_CMD, _SDT_NULL, 1, copyArg),   \
    CX_OBJ(cnPrefix "析构函数", egPrefix "C", explain, CT_IS_HIDED | CT_IS_OBJ_FREE_CMD, _SDT_NULL, 0, NULL)

// 取库版本标识的实现在文件末尾，这里先给实现表一个前置声明。
void LibVersionTag(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf);

CMD_INFO g_commands[] = {
    // 0-6 全局命令
    CX_CMD("创想软件_测试命令", "CxSoftTest", "返回一段测试文本，用于确认支持库已正确装载。", 1, SDT_TEXT, 1, g_argGreeting),
    CX_CMD("取指纹型号列表", "GetProfileList", "返回可模拟的浏览器 TLS 指纹型号，每行一个。", 2, SDT_TEXT, 0, NULL),
    CX_CMD("取网络引擎信息", "GetNetEngineInfo", "返回 libcurl-impersonate 版本；加载失败时返回错误说明。", 2, SDT_TEXT, 0, NULL),
    CX_CMD("URL编码", "UrlEncode", "对 UTF-8 文本作百分号编码。", 2, SDT_TEXT, 1, g_argText1),
    CX_CMD("URL解码", "UrlDecode", "还原百分号编码。", 2, SDT_TEXT, 1, g_argText1),
    CX_CMD("UTF8转GBK", "Utf8ToGbk", "把 UTF-8 文本转成本机 ANSI(GBK)，仅用于显示。", 2, SDT_TEXT, 1, g_argText1),
    CX_CMD("快速请求", "QuickGet", "一次性请求并返回 UTF-8 正文，不建对象。", 2, SDT_TEXT, 3, g_argQuickGet),

    // 7-35 会话
    CX_LIFECYCLE("会话", "浏览器会话", "BrowserSession", g_argCopySession, "创建可复用的浏览器会话（连接、cookie 在其中保持）。"),
    CX_OBJ("设置指纹型号", "SetProfile", "设置要模拟的浏览器型号，如 chrome133a。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argProfile),
    CX_OBJ("取指纹型号", "GetProfile", "返回当前指纹型号。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    CX_OBJ("设置代理", "SetProxy", "设置 http / socks 代理。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argProxy),
    CX_OBJ("设置代理认证", "SetProxyAuth", "设置代理的用户名与密码。", _CMD_OS(__OS_WIN), _SDT_NULL, 2, g_argAuth2),
    CX_OBJ("设置站点认证", "SetSiteAuth", "设置目标站的 HTTP 基本认证账号。", _CMD_OS(__OS_WIN), _SDT_NULL, 2, g_argAuth2),
    CX_OBJ("设置用户代理", "SetUserAgent", "覆盖 UA；留空沿用型号自带的 UA。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argUserAgent),
    CX_OBJ("设置超时", "SetTimeout", "设置整个请求的超时秒数。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argTimeout),
    CX_OBJ("设置连接超时", "SetConnectTimeout", "设置建立连接的超时秒数。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argConnTimeout),
    CX_OBJ("设置低速限制", "SetLowSpeedLimit", "速率持续低于阈值就中断，用于杀掉卡死的下载。", _CMD_OS(__OS_WIN), _SDT_NULL, 2, g_argLowSpeed),
    CX_OBJ("设置证书校验", "SetVerifyCert", "假则跳过 HTTPS 证书校验，仅调试用。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argVerify),
    CX_OBJ("设置CA证书", "SetCA", "指定 CA 证书文件或目录。", _CMD_OS(__OS_WIN), _SDT_NULL, 2, g_argCA),
    CX_OBJ("设置自动跳转", "SetFollowRedirect", "是否跟随 3xx 跳转。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argFollow),
    CX_OBJ("设置最大跳转数", "SetMaxRedirect", "最多跟随多少次跳转。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argMaxRedirect),
    CX_OBJ("设置Cookie文件", "SetCookieFile", "把 cookie 持久化到文件。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argCookieFile),
    CX_OBJ("设置Cookie文本", "SetCookieText", "以 a=1; b=2 形式附加 cookie。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argCookieText),
    CX_OBJ("取Cookie文本", "GetCookieText", "取会话当前的 cookie（Netscape 格式逐行）。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    CX_OBJ("设置请求头", "SetHeader", "设置会话级请求头，同名覆盖。", _CMD_OS(__OS_WIN), _SDT_NULL, 2, g_argHeaderKV),
    CX_OBJ("删除请求头", "DelHeader", "删除一个会话级请求头。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argHeaderName),
    CX_OBJ("清除请求头", "ClearHeader", "清空会话级请求头。", _CMD_OS(__OS_WIN), _SDT_NULL, 0, NULL),
    CX_OBJ("取请求头文本", "GetHeaderText", "返回会话级请求头，每行一个。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    CX_OBJ("设置调试日志", "SetDebug", "开启后记录完整收发过程。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argDebug),
    CX_OBJ("取调试日志", "GetDebugLog", "返回调试日志文本。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    CX_OBJ("执行", "Execute", "发送请求并把结果写入响应对象；成功返回真。", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_argExecute),
    CX_OBJ("取错误码", "GetErrorCode", "返回会话最近一次失败的 curl 错误码。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("取错误文本", "GetErrorText", "返回会话最近一次失败的原因。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    CX_OBJ("重置会话", "Reset", "丢弃当前连接与指纹状态，下次执行重新建立。", _CMD_OS(__OS_WIN), _SDT_NULL, 0, NULL),

    // 36-51 请求
    CX_LIFECYCLE("请求", "浏览器请求", "BrowserRequest", g_argCopyRequest, "创建一次调用的参数集合。"),
    CX_OBJ("设置网址", "SetUrl", "设置本次请求的网址。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argUrl1),
    CX_OBJ("取网址", "GetUrl", "返回已设置的网址。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    CX_OBJ("设置方法", "SetMethod", "设置 HTTP 方法；留空按有无正文自动取 GET/POST。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argMethod),
    CX_OBJ("取方法", "GetMethod", "返回已设置的方法。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    CX_OBJ("设置提交文本", "SetBodyText", "UTF-8 正文，原样发送。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argBodyText),
    CX_OBJ("设置提交字节集", "SetBodyBin", "以字节集提交正文。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argBodyBin),
    CX_OBJ("取提交长度", "GetBodyLength", "返回当前正文长度。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("设置请求头", "SetHeader", "设置本次请求的请求头，优先于会话级同名头。", _CMD_OS(__OS_WIN), _SDT_NULL, 2, g_argHeaderKV),
    CX_OBJ("清除请求头", "ClearHeader", "清空本次请求的请求头。", _CMD_OS(__OS_WIN), _SDT_NULL, 0, NULL),
    CX_OBJ("设置下载文件", "SetOutFile", "响应正文直接写文件，不在内存留副本。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argOutFile),
    CX_OBJ("设置续传偏移", "SetResumeFrom", "从指定字节继续下载。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_argResume),
    CX_OBJ("添加文件字段", "AddFileField", "加入一个 multipart 上传文件。", _CMD_OS(__OS_WIN), _SDT_NULL, 3, g_argAddFile),
    CX_OBJ("清除文件字段", "ClearFiles", "清空上传文件列表。", _CMD_OS(__OS_WIN), _SDT_NULL, 0, NULL),

    // 52-72 响应
    CX_LIFECYCLE("响应", "浏览器响应", "BrowserResponse", g_argCopyResponse, "创建接收结果的对象。"),
    CX_OBJ("取状态码", "GetStatusCode", "返回 HTTP 状态码。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("是否成功", "IsSuccess", "无传输错误且状态码在 200-399。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    CX_OBJ("取错误码", "GetErrorCode", "返回 curl 错误码，0 为无错误。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("取错误文本", "GetErrorText", "返回失败原因。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    CX_OBJ("取响应文本", "GetBodyText", "返回 UTF-8 响应正文。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    // 全库文本一律 UTF-8，不再提供 GBK 取值口径；命令槽位保留不动，只标隐藏。
    CX_OBJ("取响应GBK文本", "GetBodyTextGbk", "已弃用：本库文本统一按 UTF-8 进出，请改用「取响应文本」。",
           CT_IS_HIDED, SDT_TEXT, 0, NULL),
    CX_OBJ("取响应字节集", "GetBodyBin", "返回响应原始字节。", _CMD_OS(__OS_WIN), SDT_BIN, 0, NULL),
    CX_OBJ("取响应头文本", "GetHeaderRaw", "返回最后一个响应块的原始头部。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    CX_OBJ("取响应头数量", "GetHeaderCount", "返回已解析的响应头条数。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("取响应头名", "GetHeaderName", "按序号（从 1 开始）取响应头名称。", _CMD_OS(__OS_WIN), SDT_TEXT, 1, g_argHeadIndex),
    CX_OBJ("取响应头值", "GetHeaderValue", "按序号（从 1 开始）取响应头内容。", _CMD_OS(__OS_WIN), SDT_TEXT, 1, g_argHeadIndex),
    CX_OBJ("按名取响应头", "GetHeaderByName", "按名称取响应头，不区分大小写。", _CMD_OS(__OS_WIN), SDT_TEXT, 1, g_argHeadName),
    CX_OBJ("取最终网址", "GetFinalUrl", "跟随跳转后实际取到内容的网址。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    CX_OBJ("取内容类型", "GetContentType", "返回响应 Content-Type。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    CX_OBJ("取跳转次数", "GetRedirectCount", "本次请求实际跟随的跳转次数。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("取耗时毫秒", "GetElapsedMs", "从发出到收到完整响应的毫秒数。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("取下载字节数", "GetDownloadSize", "已接收的响应体字节数。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("保存到文件", "SaveToFile", "把内存中的响应正文写入文件，成功返回真。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_argSaveFile),

    // ==== 73-128 宽文本全局命令（迁移自 创想软件UNICODE模块\文本处理模块）====
    CX_CMD("取文本长度W", "WideTextLen", "返回宽文本的字符数（到第一个 {0,0} 为止）。", 3, SDT_INT, 1, g_auWB),
    CX_CMD("取文本左边W", "WideTextLeft", "取宽文本左边的若干字符。", 3, SDT_BIN, 2, g_auWBinLen),
    CX_CMD("取文本右边W", "WideTextRight", "取宽文本右边的若干字符。", 3, SDT_BIN, 2, g_auWBinLen),
    CX_CMD("取文本中间W", "WideTextMid", "从指定位置取若干字符，位置从 1 开始。", 3, SDT_BIN, 3, g_auWBinPosLen),
    CX_CMD("字符W", "WideChar", "由字符代码生成宽文本字节集。", 3, SDT_BIN, 1, g_auWCode),
    CX_CMD("取代码W", "WideTextCode", "取指定位置字符的 Unicode 代码。", 3, SDT_INT, 2, g_auWBinCodePos),
    CX_CMD("寻找文本W", "WideTextFind", "查找子文本首次出现的位置（从 1 开始），失败返回 -1。", 3, SDT_INT, 4, g_auWFind),
    CX_CMD("倒找文本W", "WideTextRFind", "从后向前查找子文本位置（从 1 开始），失败返回 -1。", 3, SDT_INT, 4, g_auWFind),
    CX_CMD("到大写W", "WideTextToUpper", "把宽文本转为大写，末尾保留 {0,0}。", 3, SDT_BIN, 1, g_auWB),
    CX_CMD("到小写W", "WideTextToLower", "把宽文本转为小写，末尾不带 {0,0}（与原版一致）。", 3, SDT_BIN, 1, g_auWB),
    CX_CMD("到全角W", "WideTextToFullWidth", "按中文区域设置把宽文本转为全角。", 3, SDT_BIN, 1, g_auWB),
    CX_CMD("到半角W", "WideTextToHalfWidth", "按中文区域设置把宽文本转为半角。", 3, SDT_BIN, 1, g_auWB),
    CX_CMD("文本连接W", "WideTextCat", "连接两段宽文本。", 3, SDT_BIN, 2, g_auW2Bin),
    CX_CMD("文本比较W", "WideTextCompare", "比较两段宽文本，返回 0/正/负。", 3, SDT_INT, 3, g_auWCmp),
    CX_CMD("删首尾W", "WideTextTrim", "裁掉宽文本首尾出现在字符集合中的字符。", 3, SDT_BIN, 2, g_auWTrim),
    CX_CMD("删首尾空W", "WideTextTrimSpace", "裁掉宽文本首尾的空格。", 3, SDT_BIN, 1, g_auWB),
    CX_CMD("文本替换W", "WideTextReplace", "按位置与长度替换宽文本内容。", 3, SDT_BIN, 4, g_auWReplace),
    CX_CMD("子文本替换W", "WideTextSubReplace", "把宽文本中的子文本替换为另一段宽文本。", 3, SDT_BIN, 6, g_auWSub),
    CX_CMD("子文本替换2W", "WideTextSubReplace2", "同上，查找与替换内容使用 UTF-8 文本。", 3, SDT_BIN, 6, g_auWSub2),
    CX_CMD("取空白文本W", "WideTextBlank", "返回指定字节数的全 0 宽文本字节集。", 3, SDT_BIN, 1, g_auWRepeat),
    CX_CMD("取重复文本W", "WideTextRepeat", "把宽文本重复若干次，末尾带 {0,0}。", 3, SDT_BIN, 2, g_auWRepeatBin),
    CX_CMD("分割文本W2", "WideTextSplitFind", "用寻找文本的方式分割宽文本，返回段数。", 3, SDT_INT, 3, g_auWSplit),
    CX_CMD("分割文本W", "WideTextSplit", "分割宽文本，返回段数。", 3, SDT_INT, 3, g_auWSplit),
    CX_CMD("指针到文本W", "WidePtrToText", "把宽文本指针读成字节集。", 3, SDT_BIN, 1, g_auWPtr),
    CX_CMD("取文本指针W", "WideTextPtr", "返回宽文本字节集内数据的地址。", 3, SDT_INT, 1, g_auWB),
    CX_CMD("文本到整数W", "WideTextToInt", "解析宽文本为整数。", 3, SDT_INT, 2, g_auWTextToInt),
    CX_CMD("整数到文本W", "WideIntToText", "把整数格式化成宽文本。", 3, SDT_BIN, 1, g_auWInt),
    CX_CMD("读入文件W", "WideReadFile", "按宽文本路径读入整个文件。", 3, SDT_BIN, 1, g_auWFileName),
    CX_CMD("写到文件W", "WideWriteFile", "按宽文本路径写出文件。", 3, SDT_BOOL, 2, g_auWWriteFile),
    CX_CMD("文本_取出中间文本W", "WideMidBetween", "取两个边界之间的宽文本。", 3, SDT_BIN, 5, g_auWMid),
    CX_CMD("文本_取出中间文本2W", "WideMidBetween2", "取两个边界之间的宽文本，边界使用 UTF-8 文本。", 3, SDT_BIN, 5, g_auWMidT),
    CX_CMD("文本_倒取出中间文本W", "WideRMidBetween", "从后向前取两个边界之间的宽文本。", 3, SDT_BIN, 5, g_auWRMid),
    CX_CMD("文本_倒取出中间文本2W", "WideRMidBetween2", "从后向前取中间文本，边界使用 UTF-8 文本。", 3, SDT_BIN, 5, g_auWRMidT),
    CX_CMD("文本_取左边W", "WideTextBefore", "取指定子文本左边的内容。", 3, SDT_BIN, 4, g_auWSearch4),
    CX_CMD("文本_取左边2W", "WideTextBefore2", "取指定子文本左边的内容，子文本使用 UTF-8 文本。", 3, SDT_BIN, 4, g_auWSearch4T),
    CX_CMD("文本_取右边W", "WideTextAfter", "取最后一段指定子文本右边的内容。", 3, SDT_BIN, 4, g_auWSearch4),
    CX_CMD("文本_取右边2W", "WideTextAfter2", "取右边内容，子文本使用 UTF-8 文本。", 3, SDT_BIN, 4, g_auWSearch4T),
    CX_CMD("文本是否存在W", "WideTextExists", "判断宽文本中是否存在子文本。", 3, SDT_BOOL, 3, g_auWExists),
    CX_CMD("文本_取文本_正则W", "WideRegMatch", "按正则取出匹配文本，多条结果用分隔符连接。", 3, SDT_BIN, 6, g_auWRegMatch),
    CX_CMD("文本_取文本_正则_取全部子匹配W", "WideRegMatchSub", "按正则取出主匹配或指定子匹配。", 3, SDT_BIN, 7, g_auWRegAllSub),
    CX_CMD("文本_取文本_正则2W", "WideRegMatch2", "按正则取出匹配文本，表达式使用 UTF-8 文本。", 3, SDT_BIN, 6, g_auWRegMatchT),
    CX_CMD("文本_只取数字W", "WidePickDigits", "取出文本中第一段连续数字（不含小数点）。", 3, SDT_BIN, 1, g_auWOnlyDigits),
    CX_CMD("文本_替换_正则W", "WideRegReplace", "按正则定位并替换，返回替换次数，源文本被就地修改。", 3, SDT_INT, 6, g_auWRegReplace),
    CX_CMD("文本_替换_正则_子匹配W", "WideRegSubReplace", "按正则子匹配定位并替换，返回替换次数。", 3, SDT_INT, 7, g_auWRegSubReplace),
    CX_CMD("数组_输出为文本W", "WideAryJoin", "把字节集数组原样拼接成一段宽文本。", 3, SDT_BIN, 2, g_auWAryJoin),
    CX_CMD("文本_取中间_批量W", "WideMidBatch", "批量取出两个边界之间的所有宽文本，结果写入数组。", 3, SDT_INT, 13, g_auWBatch),
    CX_CMD("文本_取中间_批量3W", "WideMidBatchUtf8", "批量取中间，结果数组为文本型（UTF-8）。", 3, SDT_INT, 12, g_auWBatch3),
    CX_CMD("文本_取出现次数W", "WideCountOccur", "统计子文本在宽文本中出现的次数。", 3, SDT_INT, 3, g_auWCount),
    CX_CMD("文本_取中间_批量2W", "WideMidBatch2", "批量取中间，边界使用 UTF-8 文本。", 3, SDT_INT, 12, g_auWBatchT),
    CX_CMD("文本_取出现次数2W", "WideCountOccur2", "统计子文本出现次数，参数为 UTF-8 文本。", 3, SDT_INT, 3, g_auWCountT),
    CX_CMD("文本_HTML取中间W", "WideHtmlMid", "按 HTML 标签闭合自动配对，取标签之间的内容。", 3, SDT_BIN, 4, g_auWHtmlMid),
    CX_CMD("数组_打乱数组_文本W", "WideAryShuffle", "打乱字节集数组的顺序。", 3, _SDT_NULL, 2, g_auWShuffle),
    CX_CMD("数组_输出文本W", "WideAryJoinNull", "把字节集数组按分隔符输出，末尾带 {0,0}。", 3, SDT_BIN, 2, g_auWAryOut),
    CX_CMD("文本_替换子文本_数组W", "WideSubReplaceAry", "用数组里的每个子文本依次替换。", 3, SDT_BIN, 4, g_auWSubAry),
    CX_CMD("数组_去重复W", "WideAryUnique", "去掉数组中重复的宽文本成员，返回剩余成员数。", 3, SDT_INT, 2, g_auWUnique),

    // ==== 129-147 扩展模块命令 ====
    CX_CMD("格式化宽字符W", "WideFormat", "删除宽文本末尾的 {0,0}。", 3, SDT_BIN, 1, g_auWFormat),
    CX_CMD("格式化宽字符W2", "WideFormat2", "确保宽文本末尾有 {0,0} 并写回变量。", 3, SDT_BIN, 1, g_auWFormat2),
    CX_CMD("_取变量地址", "GetVarAddress", "返回变量在内存中的地址。", 3, SDT_INT, 1, g_auWVar),
    CX_CMD("ThisCall", "ThisCall", "按虚表槽位调用对象方法（this 作为第一个参数入栈）。", 3, SDT_INT, 10, g_auWThisCall),
    CX_CMD("_取指针Struct", "GetStructPtr", "返回自定义数据类型变量的数据指针。", 3, SDT_INT, 1, g_auWStruct),
    CX_CMD("宽文本_字符处理", "WideTextFix", "删除宽文本中间所有 {0,0} 并在末尾补 {0,0}。", 3, SDT_BIN, 1, g_auWFormat),
    CX_CMD("WF", "WideTextNull", "宽文本末尾 {0,0} 处理：假为删除，真为按终止符截断并补齐。", 3, SDT_BIN, 2, g_auWWF),
    CX_CMD("WC", "WideTextClean", "等同 宽文本_字符处理。", 3, SDT_BIN, 1, g_auWFormat),
    CX_CMD("宽文本_是否为空", "WideTextIsEmpty", "判断宽文本是否为空字节集或 {0,0}。", 3, SDT_BOOL, 1, g_auWIsEmpty),
    CX_CMD("WISNULL", "WideIsNull", "判断宽文本是否为空。", 3, SDT_BOOL, 1, g_auWIsEmpty),
    CX_CMD("W_ISNULL", "WideIsNull2", "判断宽文本是否为空。", 3, SDT_BOOL, 1, g_auWIsEmpty),
    CX_CMD("宽文本_空", "WideTextNullValue", "返回 {0,0}。", 3, SDT_BIN, 0, NULL),
    CX_CMD("到整数W", "WideToInteger", "宽文本转整数。", 3, SDT_INT, 1, g_auWIsEmpty),
    CX_CMD("到数值W", "WideToDouble", "宽文本转双精度小数。", 3, SDT_DOUBLE, 1, g_auWToDouble),
    CX_CMD("数值到文本W", "DoubleToWide", "把双精度小数格式化成宽文本。", 3, SDT_BIN, 1, g_auWDouble),
    CX_CMD("字节集_取空白", "BinAllocBlank", "返回指定字节数的全 0 字节集。", 3, SDT_BIN, 1, g_auWBlank),
    CX_CMD("宽文本_是否相等", "WideTextEqual", "比较两段宽文本是否相等（忽略末尾 {0,0}）。", 3, SDT_BOOL, 2, g_auWEqual),
    CX_CMD("线程_取消COM库", "ThreadCoFree", "取消本线程的 COM 初始化。", 3, _SDT_NULL, 0, NULL),
    CX_CMD("线程_初始化COM库", "ThreadCoInit", "初始化本线程的 COM，创建正则对象前可显式调用。", 3, _SDT_NULL, 0, NULL),

    // ==== 147-206 宽文本类 ====
    CX_LIFECYCLE("宽文本", "宽文本类", "WideText", g_auWsNoneArr, "创建一段可追加的 UTF-16 文本缓冲。"),
    CX_OBJ("取指针", "GetPtr", "返回内部缓冲对象指针。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("置指针", "SetPtr", "接管由 取指针() 得到的对象指针。", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_auWsPtr),
    CX_OBJ("创建", "Create", "分配缓冲区，容量默认 11 个字符。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_auWsCap),
    CX_OBJ("添加RC资源", "AddResString", "读入本库的字符串资源并追加。", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_auWsRes),
    CX_OBJ("添加文本指针W", "AddPtrW", "追加一段宽文本指针。", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_auWsPtrW),
    CX_OBJ("添加文本指针", "AddPtrA", "追加一段 ANSI 文本指针。", _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_auWsPtrA),
    CX_OBJ("添加", "Add", "追加 UTF-8 文本。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_auWsT),
    CX_OBJ("添加W", "AddW", "追加宽文本字节集。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_auWsB),
    CX_OBJ("添加类", "AddText", "追加另一个宽文本对象的内容。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_auWsSelf),
    CX_OBJ("添加一行", "AddLine", "追加 UTF-8 文本并换行。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_auWsT),
    CX_OBJ("添加一行W", "AddLineW", "追加宽文本并换行。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_auWsB),
    CX_OBJ("添加一行类", "AddLineText", "追加另一个对象的内容并换行。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_auWsSelf),
    CX_OBJ("添加整数", "AddInt", "追加整数的十进制文本。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_auWsInt),
    CX_OBJ("插入", "Insert", "在指定位置插入 UTF-8 文本。", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_auWsInsT),
    CX_OBJ("插入W", "InsertW", "在指定位置插入宽文本。", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_auWsInsB),
    CX_OBJ("插入类", "InsertText", "在指定位置插入另一个对象的内容。", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_auWsInsSelf),
    CX_OBJ("取文本", "GetStr", "返回 UTF-8 文本。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    CX_OBJ("取文本W", "GetStrW", "返回宽文本字节集，末尾带 {0,0}。", _CMD_OS(__OS_WIN), SDT_BIN, 0, NULL),
    CX_OBJ("取文本W_no", "GetStrWNoNull", "返回宽文本字节集，末尾不带 {0,0}。", _CMD_OS(__OS_WIN), SDT_BIN, 0, NULL),
    CX_OBJ("取文本指针", "GetStrPtr", "返回内部缓冲地址，不要释放。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("清除", "Clear", "清空内容但保留缓冲。", _CMD_OS(__OS_WIN), _SDT_NULL, 0, NULL),
    CX_OBJ("销毁", "Destroy", "释放内部缓冲。", _CMD_OS(__OS_WIN), _SDT_NULL, 0, NULL),
    CX_OBJ("添加代码", "AddChar", "追加一个 Unicode 字符。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_auWsChar),
    CX_OBJ("取代码", "GetChar", "取指定位置字符的 Unicode 代码。", _CMD_OS(__OS_WIN), SDT_INT, 1, g_auWsGetChar),
    CX_OBJ("置代码", "SetChar", "改写指定位置的字符代码。", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_auWsSetChar),
    CX_OBJ("比较", "Compare", "与 UTF-8 文本比较。", _CMD_OS(__OS_WIN), SDT_INT, 1, g_auWsCmpT),
    CX_OBJ("比较W", "CompareW", "与宽文本比较。", _CMD_OS(__OS_WIN), SDT_INT, 1, g_auWsCmpB),
    CX_OBJ("比较类", "CompareText", "与另一个对象比较。", _CMD_OS(__OS_WIN), SDT_INT, 1, g_auWsCmpSelf),
    CX_OBJ("取文本长度", "GetLength", "返回当前字符数。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("克隆", "Clone", "返回内容相同的新对象。", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_WSTR, 0), 0, NULL),
    CX_OBJ("到大写", "ToUpper", "返回大写后的新对象。", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_WSTR, 0), 0, NULL),
    CX_OBJ("到小写", "ToLower", "返回小写后的新对象。", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_WSTR, 0), 0, NULL),
    CX_OBJ("到全角", "ToFullWidth", "返回全角后的新对象。", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_WSTR, 0), 0, NULL),
    CX_OBJ("到半角", "ToHalfWidth", "返回半角后的新对象。", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_WSTR, 0), 0, NULL),
    CX_OBJ("寻找W", "FindW", "查找宽文本位置（从 0 开始），失败返回 -1。", _CMD_OS(__OS_WIN), SDT_INT, 3, g_auWsFindB),
    CX_OBJ("寻找", "Find", "查找 UTF-8 文本位置（从 0 开始）。", _CMD_OS(__OS_WIN), SDT_INT, 3, g_auWsFindT),
    CX_OBJ("寻找类", "FindText", "查找另一个对象内容的位置。", _CMD_OS(__OS_WIN), SDT_INT, 3, g_auWsFindSelf),
    CX_OBJ("倒找W", "RFindW", "从后向前查找宽文本位置。", _CMD_OS(__OS_WIN), SDT_INT, 3, g_auWsFindB),
    CX_OBJ("倒找", "RFind", "从后向前查找 UTF-8 文本位置。", _CMD_OS(__OS_WIN), SDT_INT, 3, g_auWsFindT),
    CX_OBJ("倒找类", "RFindText", "从后向前查找另一个对象内容的位置。", _CMD_OS(__OS_WIN), SDT_INT, 3, g_auWsFindSelf),
    CX_OBJ("替换W", "ReplaceW", "按位置与长度替换宽文本内容。", _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_auWsReplaceB),
    CX_OBJ("替换", "Replace", "按位置与长度替换内容，参数为 UTF-8 文本。", _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_auWsReplaceT),
    CX_OBJ("替换类", "ReplaceText", "按位置与长度替换内容，参数为对象。", _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_auWsReplaceSelf),
    CX_OBJ("子替换W", "SubReplaceW", "替换宽文本中的子文本。", _CMD_OS(__OS_WIN), SDT_BOOL, 5, g_auWsSubB),
    CX_OBJ("子替换", "SubReplace", "替换子文本，参数为 UTF-8 文本。", _CMD_OS(__OS_WIN), SDT_BOOL, 5, g_auWsSubT),
    CX_OBJ("子替换类", "SubReplaceText", "替换子文本，参数为对象。", _CMD_OS(__OS_WIN), SDT_BOOL, 5, g_auWsSubSelf),
    CX_OBJ("取中间", "GetMid", "返回指定区间的新对象，位置从 0 开始。", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_WSTR, 0), 2, g_auWsMid),
    CX_OBJ("取左边", "GetLeft", "返回左边若干字符的新对象。", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_WSTR, 0), 1, g_auWsLen),
    CX_OBJ("取右边", "GetRight", "返回右边若干字符的新对象。", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_WSTR, 0), 1, g_auWsLen),
    CX_OBJ("删首尾W", "TrimW", "裁掉首尾出现在字符集合中的字符。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_auWsB),
    CX_OBJ("删首尾", "Trim", "裁掉首尾字符，参数为 UTF-8 文本。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_auWsT),
    CX_OBJ("删首尾类", "TrimText", "裁掉首尾字符，参数为对象。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_auWsTrimSelf),
    CX_OBJ("删首尾空", "TrimSpace", "裁掉首尾空格。", _CMD_OS(__OS_WIN), SDT_BOOL, 0, NULL),
    CX_OBJ("到整数", "ToInt", "把内容解析成整数。", _CMD_OS(__OS_WIN), SDT_INT, 1, g_auWsToInt),
    CX_OBJ("分割文本类", "SplitText", "按对象分隔符分割，结果写入宽文本数组，返回段数。", _CMD_OS(__OS_WIN), SDT_INT, 3, g_auWsSplitSelf),
    CX_OBJ("分割文本W", "SplitW", "按宽文本分隔符分割，结果写入字节集数组。", _CMD_OS(__OS_WIN), SDT_INT, 3, g_auWsSplitB),
    CX_OBJ("分割文本", "Split", "按分隔符分割，结果写入文本型数组（UTF-8）。", _CMD_OS(__OS_WIN), SDT_INT, 3, g_auWsSplitT),

    // ==== 207-217 类_正则表达式类W ====
    CX_LIFECYCLE("正则W", "类_正则表达式类W", "RegExpW", g_auRxCopy, "用 VBScript.RegExp 匹配宽文本的封装。"),
    CX_OBJ("创建", "Create", "设置表达式并对宽文本执行匹配，成功返回真。", _CMD_OS(__OS_WIN), SDT_BOOL, 6, g_auRxCreateB),
    CX_OBJ("查看", "Inspect", "保留命令，与原版一样不做任何事。", _CMD_OS(__OS_WIN), _SDT_NULL, 0, NULL),
    CX_OBJ("取正则文本W", "GetPatternW", "返回当前正则表达式（宽文本）。", _CMD_OS(__OS_WIN), SDT_BIN, 0, NULL),
    CX_OBJ("替换", "Replace", "返回替换后的宽文本。", _CMD_OS(__OS_WIN), SDT_BIN, 1, g_auRxReplaceB),
    CX_OBJ("取匹配数量", "GetMatchCount", "返回匹配到的条目数。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("取匹配文本W", "GetMatchTextW", "返回第 N 条匹配文本，可写回其起始位置。", _CMD_OS(__OS_WIN), SDT_BIN, 2, g_auRxValue),
    CX_OBJ("取子匹配文本W", "GetSubTextW", "返回指定子表达式的匹配文本。", _CMD_OS(__OS_WIN), SDT_BIN, 3, g_auRxSubValue),
    CX_OBJ("取子匹配数量", "GetSubCount", "返回第 1 条匹配的子表达式个数。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),

    // ==== 218-231 类_正则表达式_兼容版W ====
    CX_LIFECYCLE("正则兼容", "类_正则表达式_兼容版W", "RegExpCompatW", g_auRxcCopy, "与原模块同名类行为一致的正则封装。"),
    CX_OBJ("创建", "Create", "设置表达式并匹配 UTF-8 文本。", _CMD_OS(__OS_WIN), SDT_BOOL, 6, g_auRxCreateT6),
    CX_OBJ("创建W", "CreateW", "设置表达式并匹配宽文本。", _CMD_OS(__OS_WIN), SDT_BOOL, 5, g_auRxCreateB5),
    CX_OBJ("查看", "Inspect", "保留命令。", _CMD_OS(__OS_WIN), _SDT_NULL, 0, NULL),
    CX_OBJ("取正则文本", "GetPattern", "返回当前正则表达式（UTF-8）。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    CX_OBJ("替换", "Replace", "返回替换后的 UTF-8 文本。", _CMD_OS(__OS_WIN), SDT_TEXT, 1, g_auRxReplaceT),
    CX_OBJ("取匹配数量", "GetMatchCount", "返回匹配到的条目数。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("取匹配文本", "GetMatchText", "返回第 N 条匹配文本（UTF-8）。", _CMD_OS(__OS_WIN), SDT_TEXT, 2, g_auRxValue),
    CX_OBJ("取匹配文本W", "GetMatchTextW", "返回第 N 条匹配文本（宽文本）。", _CMD_OS(__OS_WIN), SDT_BIN, 2, g_auRxValue),
    CX_OBJ("取子匹配文本", "GetSubText", "返回子表达式匹配文本（UTF-8）。", _CMD_OS(__OS_WIN), SDT_TEXT, 3, g_auRxSubValue),
    CX_OBJ("取子匹配文本W", "GetSubTextW", "返回子表达式匹配文本（宽文本）。", _CMD_OS(__OS_WIN), SDT_BIN, 3, g_auRxSubValue),
    CX_OBJ("取子匹配数量", "GetSubCount", "返回第 1 条匹配的子表达式个数。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),

    // ==== 232-244 类_正则表达式_兼容版2 ====
    CX_LIFECYCLE("正则兼容2", "类_正则表达式_兼容版2", "RegExpCompat2", g_auRxc2Copy, "不设单行模式、恒为全局匹配的正则封装。"),
    CX_OBJ("创建", "Create", "设置表达式并匹配 UTF-8 文本。", _CMD_OS(__OS_WIN), SDT_BOOL, 4, g_auRxCreateT4),
    CX_OBJ("创建W", "CreateW", "设置表达式并匹配宽文本。", _CMD_OS(__OS_WIN), SDT_BOOL, 4, g_auRxCreateB),
    CX_OBJ("取正则文本", "GetPattern", "返回当前正则表达式（UTF-8）。", _CMD_OS(__OS_WIN), SDT_TEXT, 0, NULL),
    CX_OBJ("取正则文本W", "GetPatternW", "返回当前正则表达式（宽文本）。", _CMD_OS(__OS_WIN), SDT_BIN, 0, NULL),
    CX_OBJ("替换", "Replace", "返回替换后的 UTF-8 文本。", _CMD_OS(__OS_WIN), SDT_TEXT, 1, g_auRxReplaceT),
    CX_OBJ("替换W", "ReplaceW", "返回替换后的宽文本。", _CMD_OS(__OS_WIN), SDT_BIN, 1, g_auRxReplaceB),
    CX_OBJ("取匹配数量", "GetMatchCount", "返回匹配到的条目数。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("取匹配文本", "GetMatchText", "返回第 N 条匹配文本（UTF-8）。", _CMD_OS(__OS_WIN), SDT_TEXT, 2, g_auRxValue),
    CX_OBJ("取子匹配文本", "GetSubText", "返回子表达式匹配文本（UTF-8）。", _CMD_OS(__OS_WIN), SDT_TEXT, 3, g_auRxSubValue),
    CX_OBJ("取子匹配数量", "GetSubCount", "返回第 1 条匹配的子表达式个数。", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),

    // ==== 245-257 类_正则表达式_简单版W ====
    CX_LIFECYCLE("正则简单", "类_正则表达式_简单版W", "RegExpSimpleW", g_auRxsCopy, "只设置表达式，匹配时再传入文本的简易正则。"),
    CX_OBJ("创建", "Create", "设置表达式与匹配模式。", _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_auRxsCreate),
    CX_OBJ("匹配", "Match", "判断文本是否能被匹配。", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_auRxsText),
    CX_OBJ("搜索全部", "SearchAll", "执行匹配并返回匹配条目数。", _CMD_OS(__OS_WIN), SDT_INT, 1, g_auRxsText),
    CX_OBJ("取匹配文本", "GetMatchText", "返回第 N 条匹配文本（UTF-8）。", _CMD_OS(__OS_WIN), SDT_TEXT, 1, g_auRxValue1),
    CX_OBJ("取子表达式个数", "GetSubCount", "返回第 N 条匹配的子表达式个数。", _CMD_OS(__OS_WIN), SDT_INT, 1, g_auRxSubCount1),
    CX_OBJ("取子匹配文本", "GetSubText", "返回子表达式匹配文本（UTF-8）。", _CMD_OS(__OS_WIN), SDT_TEXT, 2, g_auRxSubValue2),
    CX_OBJ("创建W", "CreateW", "设置表达式与匹配模式（宽文本表达式）。", _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_auRxsCreateW),
    CX_OBJ("搜索全部W", "SearchAllW", "执行匹配并返回匹配条目数（宽文本）。", _CMD_OS(__OS_WIN), SDT_INT, 1, g_auRxsTextW),
    CX_OBJ("取匹配文本W", "GetMatchTextW", "返回第 N 条匹配文本（宽文本）。", _CMD_OS(__OS_WIN), SDT_BIN, 1, g_auRxValue1),
    CX_OBJ("取子匹配文本W", "GetSubTextW", "返回子表达式匹配文本（宽文本）。", _CMD_OS(__OS_WIN), SDT_BIN, 2, g_auRxSubValue2),

    // ==== 259-281 编码转换（迁移自 编码转换通用模块）====
    CX_CMD("W", "ToWideText", "文本转 UTF-16 宽文本字节集，末尾带 {0,0}。", 5, SDT_BIN, 2, g_ucTextCp),
    CX_CMD("A", "ToAnsiText", "宽文本字节集按代码页转回文本（默认 936）。", 5, SDT_TEXT, 2, g_ucWideCp),
    CX_CMD("A2W", "AnsiToWide", "ANSI 文本转宽文本，末尾带 {0,0}。", 5, SDT_BIN, 2, g_ucAnsiCp),
    CX_CMD("A2W_no", "AnsiToWideNoNull", "ANSI 文本转宽文本，末尾不带 {0,0}，适用于文本连接。", 5, SDT_BIN, 2, g_ucAnsiCp),
    CX_CMD("W2A", "WideToAnsi", "宽文本按目标代码页转成文本（默认 936）。", 5, SDT_TEXT, 2, g_ucWideCp),
    CX_CMD("W2UTF8", "WideToUtf8Bin", "宽文本转 UTF-8 字节集。", 5, SDT_BIN, 1, g_ucWide1),
    CX_CMD("W2UTF8_T", "WideToUtf8Text", "宽文本转 UTF-8 文本。", 5, SDT_TEXT, 1, g_ucWide1),
    CX_CMD("UTF82W", "Utf8BinToWide", "UTF-8 字节集转宽文本，末尾带 {0,0}。", 5, SDT_BIN, 1, g_ucBin1),
    CX_CMD("UTF82W_T", "Utf8TextToWide", "UTF-8 文本转宽文本，末尾带 {0,0}。", 5, SDT_BIN, 1, g_ucText1),
    CX_CMD("UTF82A", "Utf8BinToAnsi", "UTF-8 字节集转目标代码页文本（默认 936）。", 5, SDT_TEXT, 2, g_ucUtf8BinCp),
    CX_CMD("UTF82A_T", "Utf8TextToAnsi", "UTF-8 文本转目标代码页文本（默认 936）。", 5, SDT_TEXT, 2, g_ucTextTargetCp),
    CX_CMD("A2UTF8", "AnsiToUtf8Bin", "文本按代码页转 UTF-8 字节集。", 5, SDT_BIN, 2, g_ucTextTargetCp),
    CX_CMD("A2UTF8_T", "AnsiToUtf8Text", "文本按代码页转 UTF-8 文本。", 5, SDT_TEXT, 2, g_ucTextTargetCp),
    CX_CMD("编码_utf8到gb2312", "Utf8ToGb2312", "UTF-8 文本转 GB2312/GBK 文本。", 5, SDT_TEXT, 1, g_ucSrc),
    CX_CMD("编码_gb2312到utf8", "Gb2312ToUtf8", "GB2312/GBK 文本转 UTF-8 文本。", 5, SDT_TEXT, 1, g_ucSrc),
    CX_CMD("宽文本_处理末尾空字符", "WideTrimNull", "宽文本末尾 {0,0} 处理，等同 WF。", 5, SDT_BIN, 2, g_ucTrimNull),
    CX_CMD("编码_URL编码W", "UrlEncodeW", "宽文本按 UTF-8 做百分号编码，保留字符口径同 encodeURIComponent。", 5, SDT_TEXT, 1, g_ucWide1),
    CX_CMD("编码_URL解码", "UrlDecode", "百分号解码；「是否UTF8」为真时再把解出的 UTF-8 转成 936。", 5, SDT_TEXT, 2, g_ucUrlDecode),
    CX_CMD("编码_URL解码_JS", "UrlDecodeJs", "按 JS 口径解码：0=decodeURI，1=decodeURIComponent。", 5, SDT_TEXT, 2, g_ucUrlDecodeJs),
    CX_CMD("编码_HTML实体解码A", "HtmlDecodeA", "HTML 实体（含 &#39; 与 &#31934;）转回文本。", 5, SDT_TEXT, 1, g_ucHtml),

    // ==== 278 分割文本W 的数组返回版（追加在表尾，不动已有序号）====
    CX_CMD_AR("分割文本W_数组", "WideTextSplitToArray", "分割宽文本，直接返回 UTF-8 文本型数组。", 3, SDT_TEXT, 3, g_auWSplitAry),
    CX_CMD("取库版本标识", "LibVersionTag", "返回版本号 + 链接时间 + 命令数 + 参数结构步长，调试输出它即可确认装载的是哪一份产物。", 1, SDT_TEXT, 0, NULL),
// ==== JSON2 成员命令（gen_json2_table.mjs 生成，勿手改）====
    CX_LIFECYCLE("Json2", "Json2", "Json2", g_j2ObjCopyArgs, "创建/复制/销毁一个 Json2 值。"),
    CX_OBJ("置代码", "SetCode", "使用本类前先创建", _CMD_OS(__OS_WIN), SDT_BOOL, 4, g_j2SetCodeArgs),
    CX_OBJ("置代码UTF8", "SetCodeUtf8", "", _CMD_OS(__OS_WIN), SDT_BOOL, 4, g_j2SetCodeUtf8Args),
    CX_OBJ("置代码W", "SetCodeW", "使用本类前先创建", _CMD_OS(__OS_WIN), SDT_BOOL, 4, g_j2SetCodeWArgs),
    CX_OBJ("销毁", "Release", "将本对象的内容释放并清空。如果不调用本方法，则对象在退出其作用区域时会自动被释放", _CMD_OS(__OS_WIN), _SDT_NULL, 0, NULL),
    CX_OBJ("设置是否为顶层", "SetTopFlag", "", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_j2SetTopFlagArgs),
    CX_OBJ("取指针", "GetPtr", "获取指针，方便在只能传递整数型参数的时候使用，传递之后可用 对象.置指针（）", _CMD_OS(__OS_WIN), SDT_INT, 0, NULL),
    CX_OBJ("置指针", "SetPtr", "设置由 取指针（） 方法返回的指针", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_j2SetPtrArgs),
    CX_OBJ("创建对象", "CreateObject", "object 创建一个新的jsonValue，可以看成静态方法", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 0, NULL),
    CX_OBJ("创建数组", "CreateArray", "array 创建一个新的jsonValue，可以看成静态方法", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 0, NULL),
    CX_OBJ("创建空", "CreateNull", "null 创建一个新的jsonValue，可以看成静态方法", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 0, NULL),
    CX_OBJ("创建逻辑", "CreateBool", "bool 创建一个新的jsonValue，可以看成静态方法", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 1, g_j2CreateBoolArgs),
    CX_OBJ("创建文本W", "CreateTextW", "stringW 创建一个新的jsonValue，可以看成静态方法", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 1, g_j2CreateTextWArgs),
    CX_OBJ("创建文本", "CreateText", "string 创建一个新的jsonValue，可以看成静态方法", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 1, g_j2CreateTextArgs),
    CX_OBJ("创建双精度", "CreateDouble", "double 创建一个新的jsonValue，可以看成静态方法", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 1, g_j2CreateDoubleArgs),
    CX_OBJ("创建长整数", "CreateLong", "long 创建一个新的jsonValue，可以看成静态方法", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 1, g_j2CreateLongArgs),
    CX_OBJ("解析W", "ParseW", "将json文本解析为本对象的值", _CMD_OS(__OS_WIN), SDT_BOOL, 4, g_j2ParseWArgs),
    CX_OBJ("解析", "Parse", "将json文本解析为本对象的值", _CMD_OS(__OS_WIN), SDT_BOOL, 4, g_j2ParseArgs),
    CX_OBJ("到文本W", "ToTextW", "返回json文本", _CMD_OS(__OS_WIN), SDT_BIN, 4, g_j2ToTextWArgs),
    CX_OBJ("到文本", "ToText", "返回json文本", _CMD_OS(__OS_WIN), SDT_TEXT, 4, g_j2ToTextArgs),
    CX_OBJ("取类型", "GetTyp", "返回“JV类型_”开头常量", _CMD_OS(__OS_WIN), SDT_INT, 1, g_j2GetTypArgs),
    CX_OBJ("取类型W", "GetTypW", "返回“JV类型_”开头常量", _CMD_OS(__OS_WIN), SDT_INT, 1, g_j2GetTypWArgs),
    CX_OBJ("取父", "GetParent", "返回父级", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 1, g_j2GetParentArgs),
    CX_OBJ("取父W", "GetParentW", "返回父级", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 1, g_j2GetParentWArgs),
    CX_OBJ("取键数量", "GetKeyCount", "获取对象类型或数组类型的成员数", _CMD_OS(__OS_WIN), SDT_INT, 1, g_j2GetKeyCountArgs),
    CX_OBJ("取键数量W", "GetKeyCountW", "获取对象类型或数组类型的成员数", _CMD_OS(__OS_WIN), SDT_INT, 1, g_j2GetKeyCountWArgs),
    CX_OBJ("分离", "Detach", "从树中分离出去", _CMD_OS(__OS_WIN), _SDT_NULL, 0, NULL),
    CX_OBJ("移除属性W", "RemoveAttrW", "移除 JV类型_对象 属性，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2RemoveAttrWArgs),
    CX_OBJ("移除属性", "RemoveAttr", "移除 JV类型_对象 属性，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2RemoveAttrArgs),
    CX_OBJ("移除成员", "RemoveItem", "移除 JV类型_数组 的成员，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2RemoveItemArgs),
    CX_OBJ("移除成员W", "RemoveItemW", "移除 JV类型_数组 的成员，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2RemoveItemWArgs),
    CX_OBJ("清空", "Clear", "清空值，包括所有下级属性和成员，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_j2ClearArgs),
    CX_OBJ("清空W", "ClearW", "清空值，包括所有下级属性和成员，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_j2ClearWArgs),
    CX_OBJ("置属性", "SetAttr", "设置对象属性，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_j2SetAttrArgs),
    CX_OBJ("置属性W", "SetAttrW", "设置对象属性，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_j2SetAttrWArgs),
    CX_OBJ("K", "K", "获取对象属性", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 2, g_j2KArgs),
    CX_OBJ("K_UTF8", "KUtf8", "获取对象属性 UTF8", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 2, g_j2KUtf8Args),
    CX_OBJ("KW", "KW", "获取对象属性", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 2, g_j2KWArgs),
    CX_OBJ("K2", "K2", "获取对象属性 不存在不会自动新建", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 2, g_j2K2Args),
    CX_OBJ("K2W", "K2W", "获取对象属性", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 2, g_j2K2WArgs),
    CX_OBJ("取属性名W", "GetAttrNameW", "获取自身属性名", _CMD_OS(__OS_WIN), SDT_BIN, 1, g_j2GetAttrNameWArgs),
    CX_OBJ("取属性名", "GetAttrName", "获取自身属性名", _CMD_OS(__OS_WIN), SDT_TEXT, 1, g_j2GetAttrNameArgs),
    CX_OBJ("添加成员", "AppendItem", "添加数组成员，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2AppendItemArgs),
    CX_OBJ("添加成员W", "AppendItemW", "添加数组成员，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2AppendItemWArgs),
    CX_OBJ("插入成员", "InsertItem", "插入数组成员，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_j2InsertItemArgs),
    CX_OBJ("插入成员W", "InsertItemW", "插入数组成员，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_j2InsertItemWArgs),
    CX_OBJ("I", "I", "获取数组成员", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 2, g_j2IArgs),
    CX_OBJ("IW", "IW", "获取数组成员", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 2, g_j2IWArgs),
    CX_OBJ("I2", "I2", "获取数组成员  不存在不会自动新建", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 2, g_j2I2Args),
    CX_OBJ("I2W", "I2W", "获取数组成员  不存在不会自动新建", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 2, g_j2I2WArgs),
    CX_OBJ("置成员", "SetItem", "设置数组成员，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_j2SetItemArgs),
    CX_OBJ("置成员W", "SetItemW", "设置数组成员，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 3, g_j2SetItemWArgs),
    CX_OBJ("是否存在W", "ExistsW", "根据指定的路径判断是否存在，如果不存在返回假，存在返回真", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_j2ExistsWArgs),
    CX_OBJ("是否存在", "Exists", "根据指定的路径判断是否存在，如果不存在返回假，存在返回真", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_j2ExistsArgs),
    CX_OBJ("选择", "Select", "根据指定的路径返回一个JsonValue，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 1, g_j2SelectArgs),
    CX_OBJ("选择W", "SelectW", "根据指定的路径返回一个JsonValue，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 1, g_j2SelectWArgs),
    CX_OBJ("置对象", "SetObject", "将指定路径的值设置为一个空对象{ }", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_j2SetObjectArgs),
    CX_OBJ("置对象W", "SetObjectW", "将指定路径的值设置为一个空对象{ }", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_j2SetObjectWArgs),
    CX_OBJ("置数组", "SetArray", "将指定路径的值设置为一个空数组[ ]", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_j2SetArrayArgs),
    CX_OBJ("置数组W", "SetArrayW", "将指定路径的值设置为一个空数组[ ]", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_j2SetArrayWArgs),
    CX_OBJ("置文本", "SetText", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为文本", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SetTextArgs),
    CX_OBJ("置文本UTF8", "SetTextUtf8", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为文本", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SetTextUtf8Args),
    CX_OBJ("置文本W", "SetTextW", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为文本", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SetTextWArgs),
    CX_OBJ("置空W", "SetNullW", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为空", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_j2SetNullWArgs),
    CX_OBJ("置空", "SetNull", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为空", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_j2SetNullArgs),
    CX_OBJ("置逻辑W", "SetBoolW", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为逻辑型", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SetBoolWArgs),
    CX_OBJ("置逻辑", "SetBool", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为逻辑型", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SetBoolArgs),
    CX_OBJ("置双精度W", "SetDoubleW", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为双精度型", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SetDoubleWArgs),
    CX_OBJ("置双精度", "SetDouble", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为双精度型", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SetDoubleArgs),
    CX_OBJ("置数值W", "SetNumberW", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为双精度型", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SetNumberWArgs),
    CX_OBJ("置数值", "SetNumber", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为双精度型", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SetNumberArgs),
    CX_OBJ("置长整数W", "SetLongW", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为长整数型", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SetLongWArgs),
    CX_OBJ("置长整数", "SetLong", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为长整数型", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SetLongArgs),
    CX_OBJ("置整数W", "SetIntW", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为长整数型", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SetIntWArgs),
    CX_OBJ("置整数", "SetInt", "设置指定路径的值，如果与之前的类型不匹配，则会清除之前的值，并改变类型为长整数型", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SetIntArgs),
    CX_OBJ("取源数据", "GetRaw", "返回在解析时的源文本，不支持 “JV类型_文本型”、“JV类型_对象”和“JV类型_数组” 类型", _CMD_OS(__OS_WIN), SDT_TEXT, 1, g_j2GetRawArgs),
    CX_OBJ("取源数据W", "GetRawW", "返回在解析时的源文本，不支持 “JV类型_文本型”、“JV类型_对象”和“JV类型_数组” 类型", _CMD_OS(__OS_WIN), SDT_BIN, 1, g_j2GetRawWArgs),
    CX_OBJ("取文本", "GetText", "返回文本值，除了“JV类型_数组”和“JV类型_对象” 两个类型，其他类型都可以返回为文本数据", _CMD_OS(__OS_WIN), SDT_TEXT, 4, g_j2GetTextArgs),
    CX_OBJ("取文本UTF8", "GetTextUtf8", "返回文本值UTF8编码，除了“JV类型_数组”和“JV类型_对象” 两个类型，其他类型都可以返回为文本数据", _CMD_OS(__OS_WIN), SDT_TEXT, 4, g_j2GetTextUtf8Args),
    CX_OBJ("取文本W", "GetTextW", "返回文本值，除了“JV类型_数组”和“JV类型_对象” 两个类型，其他类型都可以返回为文本数据", _CMD_OS(__OS_WIN), SDT_BIN, 4, g_j2GetTextWArgs),
    CX_OBJ("取逻辑W", "GetBoolW", "返回逻辑值", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_j2GetBoolWArgs),
    CX_OBJ("取逻辑", "GetBoolA", "返回逻辑值", _CMD_OS(__OS_WIN), SDT_BOOL, 1, g_j2GetBoolAArgs),
    CX_OBJ("取双精度W", "GetDoubleW", "返回数值", _CMD_OS(__OS_WIN), SDT_DOUBLE, 1, g_j2GetDoubleWArgs),
    CX_OBJ("取双精度", "GetDoubleA", "返回数值", _CMD_OS(__OS_WIN), SDT_DOUBLE, 1, g_j2GetDoubleAArgs),
    CX_OBJ("取数值W", "GetNumberW", "返回数值", _CMD_OS(__OS_WIN), SDT_DOUBLE, 1, g_j2GetNumberWArgs),
    CX_OBJ("取数值", "GetNumberA", "返回数值", _CMD_OS(__OS_WIN), SDT_DOUBLE, 1, g_j2GetNumberAArgs),
    CX_OBJ("取长整数W", "GetLongW", "返回长整数型", _CMD_OS(__OS_WIN), SDT_INT64, 1, g_j2GetLongWArgs),
    CX_OBJ("取长整数", "GetLongA", "返回长整数型", _CMD_OS(__OS_WIN), SDT_INT64, 1, g_j2GetLongAArgs),
    CX_OBJ("取整数W", "GetIntW", "返回整数型", _CMD_OS(__OS_WIN), SDT_INT, 1, g_j2GetIntWArgs),
    CX_OBJ("取整数", "GetIntA", "返回整数型", _CMD_OS(__OS_WIN), SDT_INT, 1, g_j2GetIntAArgs),
    CX_OBJ("取所有键W", "GetAllKeysW", "返回数量", _CMD_OS(__OS_WIN), SDT_INT, 2, g_j2GetAllKeysWArgs),
    CX_OBJ("取所有键", "GetAllKeys", "返回数量", _CMD_OS(__OS_WIN), SDT_INT, 2, g_j2GetAllKeysArgs),
    CX_OBJ("取所有值W", "GetAllValuesW", "返回数量", _CMD_OS(__OS_WIN), SDT_INT, 2, g_j2GetAllValuesWArgs),
    CX_OBJ("取所有值", "GetAllValues", "返回数量", _CMD_OS(__OS_WIN), SDT_INT, 2, g_j2GetAllValuesArgs),
    CX_OBJ("复制W", "CopyW", "注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 1, g_j2CopyWArgs),
    CX_OBJ("复制", "Copy", "注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), MAKELONG(CX_TYPE_JSON2, 0), 1, g_j2CopyArgs),
    CX_OBJ("排序对象W", "SortObjW", "排序对象类型的属性，根据属性名", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SortObjWArgs),
    CX_OBJ("排序对象", "SortObj", "排序对象类型的属性，根据属性名", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2SortObjArgs),
    CX_OBJ("排序W", "SortW", "排序", _CMD_OS(__OS_WIN), _SDT_NULL, 4, g_j2SortWArgs),
    CX_OBJ("排序", "Sort", "排序", _CMD_OS(__OS_WIN), _SDT_NULL, 4, g_j2SortArgs),
    CX_OBJ("合并W", "MergeW", "将本对象和“参数_被合并json文本”的属性名相同的值替换", _CMD_OS(__OS_WIN), SDT_BOOL, 4, g_j2MergeWArgs),
    CX_OBJ("合并", "Merge", "将本对象和“参数_被合并json文本”的属性名相同的值替换", _CMD_OS(__OS_WIN), SDT_BOOL, 4, g_j2MergeArgs),
    CX_OBJ("合并对象Plus", "MergeObjPlus", "“参数_对象”将被合并到路径指向的对象，如果键名重复将覆盖，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2MergeObjPlusArgs),
    CX_OBJ("合并对象", "MergeObj", "“参数_对象”将被合并到路径指向的对象，如果键名重复将覆盖，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2MergeObjArgs),
    CX_OBJ("合并数组", "MergeArr", "“参数_数组”将被合并到路径指向的数组，注意：路径不存在的对象会自己创建", _CMD_OS(__OS_WIN), SDT_BOOL, 2, g_j2MergeArrArgs),
    CX_OBJ("插入属性W", "InsertAttrW", "", _CMD_OS(__OS_WIN), SDT_BOOL, 4, g_j2InsertAttrWArgs),
    CX_OBJ("插入属性", "InsertAttr", "", _CMD_OS(__OS_WIN), SDT_BOOL, 4, g_j2InsertAttrArgs),
    CX_OBJ("置文本数组", "SetTextAry", "", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_j2SetTextAryArgs),
    CX_OBJ("置文本数组W", "SetTextAryW", "", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_j2SetTextAryWArgs),
    CX_OBJ("置文本数组UTF8", "SetTextAryUtf8", "", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_j2SetTextAryUtf8Args),
    CX_OBJ("置数值数组UTF8", "SetNumberAryUtf8", "", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_j2SetNumberAryUtf8Args),
    CX_OBJ("置整数数组UTF8", "SetIntAryUtf8", "", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_j2SetIntAryUtf8Args),
    CX_OBJ("置逻辑数组UTF8", "SetBoolAryUtf8", "", _CMD_OS(__OS_WIN), _SDT_NULL, 1, g_j2SetBoolAryUtf8Args),
// ==== JSON2 成员命令 结束 ====
#include "http访问/cxhttp_cmds.inc"
#include "线程锁/lock_cmds.inc"
#include "http访问/cxhttp_cancel_cmds.inc"
#include "线程锁/join_cmds.inc"
#include "线程锁/sync_state_cmds.inc"
#include "线程锁/cancel_cmds.inc"
};

PFN_EXECUTE_CMD g_commandFuncs[] = {
    cxsoft_Test, cxsoft_GetProfileList, cxsoft_GetEngineInfo, cxsoft_UrlEncode, cxsoft_UrlDecode,
    cxsoft_Utf8ToGbk, cxsoft_QuickGet,
    SessConstruct, SessCopy, SessDestruct, SessSetProfile, SessGetProfile, SessSetProxy, SessSetProxyAuth,
    SessSetSiteAuth, SessSetUserAgent, SessSetTimeout, SessSetConnTimeout, SessSetLowSpeed, SessSetVerifyCert,
    SessSetCA, SessSetFollow, SessSetMaxRedirect, SessSetCookieFile, SessSetCookieText, SessGetCookieText,
    SessSetHeader, SessDelHeader, SessClearHeader, SessGetHeaderText, SessSetDebug, SessGetDebugLog,
    SessExecute, SessGetErrCode, SessGetErrText, SessReset,
    ReqConstruct, ReqCopy, ReqDestruct, ReqSetUrl, ReqGetUrl, ReqSetMethod, ReqGetMethod, ReqSetBodyText,
    ReqSetBodyBin, ReqGetBodyLen, ReqSetHeader, ReqClearHeader, ReqSetOutFile, ReqSetResumeFrom, ReqAddFile,
    ReqClearFiles,
    RespConstruct, RespCopy, RespDestruct, RespGetStatus, RespIsOk, RespGetErrCode, RespGetErrText,
    RespGetText, RespGetTextGbk, RespGetBin, RespGetHeadText, RespGetHeadCount, RespGetHeadName,
    RespGetHeadValue, RespGetHeadByName, RespGetFinalUrl, RespGetContentType, RespGetRedirectCount,
    RespGetElapsedMs, RespGetDownloadSize, RespSaveToFile,

    // ==== 宽文本/正则（73 起，顺序与 g_commands 追加部分一致）====
    UwTextLen, UwLeft, UwRight, UwMid, UwCharW, UwGetCode, UwFind, UwRFind, UwUpper, UwLower,
    UwFullWidth, UwHalfWidth, UwCat, UwCompare, UwTrim, UwTrimSpace, UwReplace, UwSubReplace,
    UwSubReplace2, UwBlankText, UwRepeatText, UwSplit2, UwSplit, UwPtrToText, UwTextPtr, UwTextToInt,
    UwIntToText, UwReadFile, UwWriteFile, UwMidBetween, UwMidBetween2, UwRMidBetween, UwRMidBetween2,
    UwTextBefore, UwTextBefore2, UwTextAfter, UwTextAfter2, UwTextExists, UwRegMatch, UwRegMatchAllSub,
    UwRegMatch2, UwPickDigits, UwRegReplace, UwRegSubReplace, UwAryJoin, UwMidBatch, UwMidBatch3,
    UwCountOccur, UwMidBatch2, UwCountOccur2, UwHtmlMid, UwAryShuffle, UwAryJoinNull, UwSubReplaceAry,
    UwAryUnique, UexFormatW, UexFormatW2, UexVarAddr, UexThisCall, UexStructPtr, UexWTextFix, UexWF, UexWC,
    UexWTextIsEmpty, UexWIsNull, UexW_ISNULL, UexWTextEmpty, UexWToInt, UexWToDouble, UexDoubleToW,
    UexBinBlank, UexWTextEqual, UexCoFree, UexCoInit, WsConstruct, WsCopy, WsDestruct, WsGetPtr, WsSetPtr,
    WsCreate, WsAddRes, WsAddPtrW, WsAddPtrA, WsAdd, WsAddW, WsAddSelf, WsAddLine, WsAddLineW,
    WsAddLineSelf, WsAddInt, WsInsertText, WsInsertW, WsInsertSelf, WsGetText, WsGetTextW, WsGetTextWN,
    WsGetStrPtr, WsCmdClear, WsCmdDestroy, WsAddChar, WsGetChar, WsSetChar, WsCmdCompare, WsCmdCompareW,
    WsCmdCompareSelf, WsCmdLength, WsCmdClone, WsCmdUpper, WsCmdLower, WsCmdFull, WsCmdHalf, WsCmdFindW,
    WsCmdFind, WsCmdFindSelf, WsCmdRFindW, WsCmdRFind, WsCmdRFindSelf, WsCmdReplaceW, WsCmdReplace,
    WsCmdReplaceSelf, WsCmdSubW, WsCmdSub, WsCmdSubSelf, WsCmdMid, WsCmdLeft, WsCmdRight, WsCmdTrimW,
    WsCmdTrim, WsCmdTrimSelf, WsCmdTrimSpace, WsCmdToInt, WsCmdSplitSelf, WsCmdSplitW, WsCmdSplit,
    RxConstruct, RxCopy, RxDestruct, RxCmdCreate, RxCmdView, RxCmdPattern, RxCmdReplace, RxCmdCount,
    RxCmdValueW, RxCmdSubValueW, RxCmdSubCount, RxcConstruct, RxcCopy, RxcDestruct, RxcCreate, RxcCreateW,
    RxcView, RxcPattern, RxcReplace, RxcCount, RxcValue, RxcValueW, RxcSubValue, RxcSubValueW, RxcSubCount,
    Rxc2Construct, Rxc2Copy, Rxc2Destruct, Rxc2Create, Rxc2CreateW, Rxc2Pattern, Rxc2PatternW, Rxc2Replace,
    Rxc2ReplaceW, Rxc2Count, Rxc2Value, Rxc2SubValue, Rxc2SubCount, RxsConstruct, RxsCopy, RxsDestruct,
    RxsCreate, RxsMatch, RxsSearchAll, RxsValue, RxsSubCount, RxsSubValue, RxsCreateW, RxsSearchAllW,
    RxsValueW, RxsSubValueW,
    UcW, UcA, UcA2W, UcA2WNo, UcW2A, UcW2Utf8, UcW2Utf8T, UcUtf82W, UcUtf82WT,
    UcUtf82A, UcUtf82AT, UcA2Utf8, UcA2Utf8T, UcUtf8ToGb, UcGbToUtf8, UcWideTrimNull,
    UcUrlEncodeW, UcUrlDecode, UcUrlDecodeJs, UcHtmlDecodeA,\
    UwSplitAry,
    LibVersionTag,
// ==== JSON2 成员命令实现表（gen_json2_table.mjs 生成，勿手改）====
    J2ObjConstruct,
    J2ObjCopy,
    J2ObjDestruct,
    J2SetCode,
    J2SetCodeUtf8,
    J2SetCodeW,
    J2Release,
    J2SetTopFlag,
    J2GetPtr,
    J2SetPtr,
    J2CreateObject,
    J2CreateArray,
    J2CreateNull,
    J2CreateBool,
    J2CreateTextW,
    J2CreateText,
    J2CreateDouble,
    J2CreateLong,
    J2ParseW,
    J2Parse,
    J2ToTextW,
    J2ToText,
    J2GetTyp,
    J2GetTypW,
    J2GetParent,
    J2GetParentW,
    J2GetKeyCount,
    J2GetKeyCountW,
    J2Detach,
    J2RemoveAttrW,
    J2RemoveAttr,
    J2RemoveItem,
    J2RemoveItemW,
    J2Clear,
    J2ClearW,
    J2SetAttr,
    J2SetAttrW,
    J2K,
    J2KUtf8,
    J2KW,
    J2K2,
    J2K2W,
    J2GetAttrNameW,
    J2GetAttrName,
    J2AppendItem,
    J2AppendItemW,
    J2InsertItem,
    J2InsertItemW,
    J2I,
    J2IW,
    J2I2,
    J2I2W,
    J2SetItem,
    J2SetItemW,
    J2ExistsW,
    J2Exists,
    J2Select,
    J2SelectW,
    J2SetObject,
    J2SetObjectW,
    J2SetArray,
    J2SetArrayW,
    J2SetText,
    J2SetTextUtf8,
    J2SetTextW,
    J2SetNullW,
    J2SetNull,
    J2SetBoolW,
    J2SetBool,
    J2SetDoubleW,
    J2SetDouble,
    J2SetNumberW,
    J2SetNumber,
    J2SetLongW,
    J2SetLong,
    J2SetIntW,
    J2SetInt,
    J2GetRaw,
    J2GetRawW,
    J2GetText,
    J2GetTextUtf8,
    J2GetTextW,
    J2GetBoolW,
    J2GetBoolA,
    J2GetDoubleW,
    J2GetDoubleA,
    J2GetNumberW,
    J2GetNumberA,
    J2GetLongW,
    J2GetLongA,
    J2GetIntW,
    J2GetIntA,
    J2GetAllKeysW,
    J2GetAllKeys,
    J2GetAllValuesW,
    J2GetAllValues,
    J2CopyW,
    J2Copy,
    J2SortObjW,
    J2SortObj,
    J2SortW,
    J2Sort,
    J2MergeW,
    J2Merge,
    J2MergeObjPlus,
    J2MergeObj,
    J2MergeArr,
    J2InsertAttrW,
    J2InsertAttr,
    J2SetTextAry,
    J2SetTextAryW,
    J2SetTextAryUtf8,
    J2SetNumberAryUtf8,
    J2SetIntAryUtf8,
    J2SetBoolAryUtf8,
// ==== JSON2 成员命令实现表 结束 ====
#include "http访问/cxhttp_funcs.inc"
#include "线程锁/lock_funcs.inc"
#include "http访问/cxhttp_cancel_funcs.inc"
#include "线程锁/join_funcs.inc"
#include "线程锁/sync_state_funcs.inc"
#include "线程锁/cancel_funcs.inc"
};

// 静态编译按名字找实现：顺序须与 g_commands 一一对应，名字一经发布不可更改。
const char* g_commandFuncNames[] = {
    "cxsoft_Test", "cxsoft_GetProfileList", "cxsoft_GetEngineInfo", "cxsoft_UrlEncode", "cxsoft_UrlDecode",
    "cxsoft_Utf8ToGbk",
    "cxsoft_QuickGet",
    "SessConstruct", "SessCopy", "SessDestruct", "SessSetProfile", "SessGetProfile", "SessSetProxy",
    "SessSetProxyAuth", "SessSetSiteAuth", "SessSetUserAgent", "SessSetTimeout", "SessSetConnTimeout",
    "SessSetLowSpeed", "SessSetVerifyCert", "SessSetCA", "SessSetFollow", "SessSetMaxRedirect",
    "SessSetCookieFile", "SessSetCookieText", "SessGetCookieText", "SessSetHeader", "SessDelHeader",
    "SessClearHeader", "SessGetHeaderText", "SessSetDebug", "SessGetDebugLog", "SessExecute",
    "SessGetErrCode", "SessGetErrText", "SessReset",
    "ReqConstruct", "ReqCopy", "ReqDestruct", "ReqSetUrl", "ReqGetUrl", "ReqSetMethod", "ReqGetMethod",
    "ReqSetBodyText", "ReqSetBodyBin", "ReqGetBodyLen", "ReqSetHeader", "ReqClearHeader", "ReqSetOutFile",
    "ReqSetResumeFrom", "ReqAddFile", "ReqClearFiles",
    "RespConstruct", "RespCopy", "RespDestruct", "RespGetStatus", "RespIsOk", "RespGetErrCode",
    "RespGetErrText", "RespGetText", "RespGetTextGbk", "RespGetBin", "RespGetHeadText", "RespGetHeadCount",
    "RespGetHeadName", "RespGetHeadValue", "RespGetHeadByName", "RespGetFinalUrl", "RespGetContentType",
    "RespGetRedirectCount", "RespGetElapsedMs", "RespGetDownloadSize", "RespSaveToFile",

    "UwTextLen", "UwLeft", "UwRight", "UwMid", "UwCharW", "UwGetCode", "UwFind", "UwRFind",
    "UwUpper", "UwLower", "UwFullWidth", "UwHalfWidth", "UwCat", "UwCompare", "UwTrim", "UwTrimSpace",
    "UwReplace", "UwSubReplace", "UwSubReplace2", "UwBlankText", "UwRepeatText", "UwSplit2", "UwSplit",
    "UwPtrToText", "UwTextPtr", "UwTextToInt", "UwIntToText", "UwReadFile", "UwWriteFile", "UwMidBetween",
    "UwMidBetween2", "UwRMidBetween", "UwRMidBetween2", "UwTextBefore", "UwTextBefore2", "UwTextAfter",
    "UwTextAfter2", "UwTextExists", "UwRegMatch", "UwRegMatchAllSub", "UwRegMatch2", "UwPickDigits",
    "UwRegReplace", "UwRegSubReplace", "UwAryJoin", "UwMidBatch", "UwMidBatch3", "UwCountOccur",
    "UwMidBatch2", "UwCountOccur2", "UwHtmlMid", "UwAryShuffle", "UwAryJoinNull", "UwSubReplaceAry",
    "UwAryUnique", "UexFormatW", "UexFormatW2", "UexVarAddr", "UexThisCall", "UexStructPtr", "UexWTextFix",
    "UexWF", "UexWC", "UexWTextIsEmpty", "UexWIsNull", "UexW_ISNULL", "UexWTextEmpty", "UexWToInt",
    "UexWToDouble", "UexDoubleToW", "UexBinBlank", "UexWTextEqual", "UexCoFree", "UexCoInit",
    "WsConstruct", "WsCopy", "WsDestruct", "WsGetPtr", "WsSetPtr", "WsCreate", "WsAddRes", "WsAddPtrW",
    "WsAddPtrA", "WsAdd", "WsAddW", "WsAddSelf", "WsAddLine", "WsAddLineW", "WsAddLineSelf", "WsAddInt",
    "WsInsertText", "WsInsertW", "WsInsertSelf", "WsGetText", "WsGetTextW", "WsGetTextWN", "WsGetStrPtr",
    "WsCmdClear", "WsCmdDestroy", "WsAddChar", "WsGetChar", "WsSetChar", "WsCmdCompare", "WsCmdCompareW",
    "WsCmdCompareSelf", "WsCmdLength", "WsCmdClone", "WsCmdUpper", "WsCmdLower", "WsCmdFull", "WsCmdHalf",
    "WsCmdFindW", "WsCmdFind", "WsCmdFindSelf", "WsCmdRFindW", "WsCmdRFind", "WsCmdRFindSelf",
    "WsCmdReplaceW", "WsCmdReplace", "WsCmdReplaceSelf", "WsCmdSubW", "WsCmdSub", "WsCmdSubSelf",
    "WsCmdMid", "WsCmdLeft", "WsCmdRight", "WsCmdTrimW", "WsCmdTrim", "WsCmdTrimSelf", "WsCmdTrimSpace",
    "WsCmdToInt", "WsCmdSplitSelf", "WsCmdSplitW", "WsCmdSplit", "RxConstruct", "RxCopy", "RxDestruct",
    "RxCmdCreate", "RxCmdView", "RxCmdPattern", "RxCmdReplace", "RxCmdCount", "RxCmdValueW",
    "RxCmdSubValueW", "RxCmdSubCount", "RxcConstruct", "RxcCopy", "RxcDestruct", "RxcCreate", "RxcCreateW",
    "RxcView", "RxcPattern", "RxcReplace", "RxcCount", "RxcValue", "RxcValueW", "RxcSubValue",
    "RxcSubValueW", "RxcSubCount", "Rxc2Construct", "Rxc2Copy", "Rxc2Destruct", "Rxc2Create",
    "Rxc2CreateW", "Rxc2Pattern", "Rxc2PatternW", "Rxc2Replace", "Rxc2ReplaceW", "Rxc2Count", "Rxc2Value",
    "Rxc2SubValue", "Rxc2SubCount", "RxsConstruct", "RxsCopy", "RxsDestruct", "RxsCreate", "RxsMatch",
    "RxsSearchAll", "RxsValue", "RxsSubCount", "RxsSubValue", "RxsCreateW", "RxsSearchAllW", "RxsValueW",
    "RxsSubValueW",
    "UcW", "UcA", "UcA2W", "UcA2WNo", "UcW2A", "UcW2Utf8", "UcW2Utf8T", "UcUtf82W",
    "UcUtf82WT", "UcUtf82A", "UcUtf82AT", "UcA2Utf8", "UcA2Utf8T", "UcUtf8ToGb", "UcGbToUtf8",
    "UcWideTrimNull", "UcUrlEncodeW", "UcUrlDecode", "UcUrlDecodeJs", "UcHtmlDecodeA",\
    "UwSplitAry",
    "LibVersionTag",
// ==== JSON2 成员命令实现名表（gen_json2_table.mjs 生成，勿手改）====
    "J2ObjConstruct",
    "J2ObjCopy",
    "J2ObjDestruct",
    "J2SetCode",
    "J2SetCodeUtf8",
    "J2SetCodeW",
    "J2Release",
    "J2SetTopFlag",
    "J2GetPtr",
    "J2SetPtr",
    "J2CreateObject",
    "J2CreateArray",
    "J2CreateNull",
    "J2CreateBool",
    "J2CreateTextW",
    "J2CreateText",
    "J2CreateDouble",
    "J2CreateLong",
    "J2ParseW",
    "J2Parse",
    "J2ToTextW",
    "J2ToText",
    "J2GetTyp",
    "J2GetTypW",
    "J2GetParent",
    "J2GetParentW",
    "J2GetKeyCount",
    "J2GetKeyCountW",
    "J2Detach",
    "J2RemoveAttrW",
    "J2RemoveAttr",
    "J2RemoveItem",
    "J2RemoveItemW",
    "J2Clear",
    "J2ClearW",
    "J2SetAttr",
    "J2SetAttrW",
    "J2K",
    "J2KUtf8",
    "J2KW",
    "J2K2",
    "J2K2W",
    "J2GetAttrNameW",
    "J2GetAttrName",
    "J2AppendItem",
    "J2AppendItemW",
    "J2InsertItem",
    "J2InsertItemW",
    "J2I",
    "J2IW",
    "J2I2",
    "J2I2W",
    "J2SetItem",
    "J2SetItemW",
    "J2ExistsW",
    "J2Exists",
    "J2Select",
    "J2SelectW",
    "J2SetObject",
    "J2SetObjectW",
    "J2SetArray",
    "J2SetArrayW",
    "J2SetText",
    "J2SetTextUtf8",
    "J2SetTextW",
    "J2SetNullW",
    "J2SetNull",
    "J2SetBoolW",
    "J2SetBool",
    "J2SetDoubleW",
    "J2SetDouble",
    "J2SetNumberW",
    "J2SetNumber",
    "J2SetLongW",
    "J2SetLong",
    "J2SetIntW",
    "J2SetInt",
    "J2GetRaw",
    "J2GetRawW",
    "J2GetText",
    "J2GetTextUtf8",
    "J2GetTextW",
    "J2GetBoolW",
    "J2GetBoolA",
    "J2GetDoubleW",
    "J2GetDoubleA",
    "J2GetNumberW",
    "J2GetNumberA",
    "J2GetLongW",
    "J2GetLongA",
    "J2GetIntW",
    "J2GetIntA",
    "J2GetAllKeysW",
    "J2GetAllKeys",
    "J2GetAllValuesW",
    "J2GetAllValues",
    "J2CopyW",
    "J2Copy",
    "J2SortObjW",
    "J2SortObj",
    "J2SortW",
    "J2Sort",
    "J2MergeW",
    "J2Merge",
    "J2MergeObjPlus",
    "J2MergeObj",
    "J2MergeArr",
    "J2InsertAttrW",
    "J2InsertAttr",
    "J2SetTextAry",
    "J2SetTextAryW",
    "J2SetTextAryUtf8",
    "J2SetNumberAryUtf8",
    "J2SetIntAryUtf8",
    "J2SetBoolAryUtf8",
// ==== JSON2 成员命令实现名表 结束 ====
#include "http访问/cxhttp_names.inc"
#include "线程锁/lock_names.inc"
#include "http访问/cxhttp_cancel_names.inc"
#include "线程锁/join_names.inc"
#include "线程锁/sync_state_names.inc"
#include "线程锁/cancel_names.inc"
};

// 三张表必须同长，否则按序号取实现会整体错位一格。
static_assert(sizeof(g_commandFuncs) / sizeof(g_commandFuncs[0]) == sizeof(g_commands) / sizeof(g_commands[0]),
    "实现表与命令表长度不一致");
static_assert(sizeof(g_commandFuncNames) / sizeof(g_commandFuncNames[0]) == sizeof(g_commands) / sizeof(g_commands[0]),
    "函数名表与命令表长度不一致");

// ============================================================ 类定义

LIB_DATA_TYPE_ELEMENT g_onePtrElement[] = {
    { SDT_INT, NULL, _T("impl"), NULL, _T("内部数据，请勿读写。"), LES_HIDED, 0 },
};

// 会话：命令下标 7..35
INT g_sessionCmdIndex[] = { 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
                            26, 27, 28, 29, 30, 31, 32, 33, 34, 35 };
// 请求：36..51
INT g_requestCmdIndex[] = { 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };
// 响应：52..72
INT g_responseCmdIndex[] = { 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
                             70, 71, 72 };
// 宽文本类：147..206
INT g_wstrCmdIndex[] = { 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206 };
// 类_正则表达式类W：207..217
INT g_rxWCmdIndex[] = { 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217 };
// 类_正则表达式_兼容版W：218..231
INT g_rxcCmdIndex[] = { 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231 };
// 类_正则表达式_兼容版2：232..244
INT g_rxc2CmdIndex[] = { 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244 };
// 类_正则表达式_简单版W：245..257
INT g_rxsCmdIndex[] = { 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257 };

// 每个类的成员命令条数变了要同步这里，否则命令索引表会和实际条数脱节。
static_assert(sizeof(g_sessionCmdIndex) / sizeof(INT) == 29, "会话成员命令条数变了");
static_assert(sizeof(g_requestCmdIndex) / sizeof(INT) == 16, "请求成员命令条数变了");
static_assert(sizeof(g_responseCmdIndex) / sizeof(INT) == 21, "响应成员命令条数变了");
static_assert(sizeof(g_wstrCmdIndex) / sizeof(INT) == 60, "宽文本类成员命令条数变了");
static_assert(sizeof(g_rxWCmdIndex) / sizeof(INT) == 11, "类_正则表达式类W 成员命令条数变了");
static_assert(sizeof(g_rxcCmdIndex) / sizeof(INT) == 14, "类_正则表达式_兼容版W 成员命令条数变了");
static_assert(sizeof(g_rxc2CmdIndex) / sizeof(INT) == 13, "类_正则表达式_兼容版2 成员命令条数变了");
static_assert(sizeof(g_rxsCmdIndex) / sizeof(INT) == 13, "类_正则表达式_简单版W 成员命令条数变了");

// ==== JSON2 类成员下标表与常量表（gen_json2_table.mjs 生成，勿手改）====
static LIB_CONST_INFO g_constants[] = {
    { _T("JV类型_未定义"), _T("JVTYPE_UNDEFINED"), _T("JV类型_未定义"), _CMD_OS(__OS_WIN), CT_NUM, NULL, -1 },
    { _T("JV类型_空"), _T("JVTYPE_NULL"), _T("JV类型_空"), _CMD_OS(__OS_WIN), CT_NUM, NULL, 0 },
    { _T("JV类型_双精度型"), _T("JVTYPE_DOUBLE"), _T("JV类型_双精度型"), _CMD_OS(__OS_WIN), CT_NUM, NULL, 1 },
    { _T("JV类型_长整数型"), _T("JVTYPE_LONG"), _T("JV类型_长整数型"), _CMD_OS(__OS_WIN), CT_NUM, NULL, 2 },
    { _T("JV类型_逻辑型"), _T("JVTYPE_BOOL"), _T("JV类型_逻辑型"), _CMD_OS(__OS_WIN), CT_NUM, NULL, 3 },
    { _T("JV类型_文本型"), _T("JVTYPE_STRING"), _T("JV类型_文本型"), _CMD_OS(__OS_WIN), CT_NUM, NULL, 4 },
    { _T("JV类型_数组"), _T("JVTYPE_ARRAY"), _T("JV类型_数组"), _CMD_OS(__OS_WIN), CT_NUM, NULL, 5 },
    { _T("JV类型_对象"), _T("JVTYPE_OBJECT"), _T("JV类型_对象"), _CMD_OS(__OS_WIN), CT_NUM, NULL, 6 },
    { _T("JSON_OK"), _T("JSON_OK"), _T("JSON_OK"), _CMD_OS(__OS_WIN), CT_NUM, NULL, 0 },
    { _T("JSON_ERROR"), _T("JSON_ERROR"), _T("JSON_ERROR"), _CMD_OS(__OS_WIN), CT_NUM, NULL, -1 },
    { _T("JSON_INVALIDARG"), _T("JSON_INVALIDARG"), _T("JSON_INVALIDARG"), _CMD_OS(__OS_WIN), CT_NUM, NULL, -2 },
    { _T("JSON_MEMORY"), _T("JSON_MEMORY"), _T("JSON_MEMORY"), _CMD_OS(__OS_WIN), CT_NUM, NULL, -3 },
    { _T("JSON_INVALIDNAME"), _T("JSON_INVALIDNAME"), _T("JSON_INVALIDNAME"), _CMD_OS(__OS_WIN), CT_NUM, NULL, -4 },
    { _T("JSON_OBJECTPARENT"), _T("JSON_OBJECTPARENT"), _T("JSON_OBJECTPARENT"), _CMD_OS(__OS_WIN), CT_NUM, NULL, -5 },
    { _T("JSON_INVALIDVALUE"), _T("JSON_INVALIDVALUE"), _T("JSON_INVALIDVALUE"), _CMD_OS(__OS_WIN), CT_NUM, NULL, -6 },
// ==== HTTP访问 libcurl 常量（http访问/gen_http_consts.mjs 生成，勿手改）====
#include "http访问/cxhttp_consts.inc"
// ==== 原模块自有常量（HTTP状态_/HTTP配置_/INTERNET_/WINHTTP_/CP_/SECURITY_ 等，gen_mod_consts.mjs 生成）====
#include "http访问/cxmod_consts.inc"
};

// Json2：成员命令下标（构造/复制/析构 + 112 条公开方法，追加在表尾）
INT g_json2CmdIndex[] = { 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388, 389, 390, 391, 392, 393, 394 };
static_assert(sizeof(g_json2CmdIndex) / sizeof(INT) == 115, "Json2 成员命令条数变了");

// ==== JSON2 类成员下标表与常量表 结束 ====
#define CX_TYPE(name, eg, explain, cmdIndex)                                                    \
    {                                                                                           \
        _T(name), _T(eg), _T(explain),                                                          \
        sizeof(cmdIndex) / sizeof(cmdIndex[0]), cmdIndex,                                       \
        0, 0, 0, NULL, 0, NULL, NULL,                                                            \
        sizeof(g_onePtrElement) / sizeof(g_onePtrElement[0]), g_onePtrElement,                   \
    }

// ==== HTTP访问 类成员下标表（gen_http_table.mjs 生成，勿手改）====
#include "http访问/cxhttp_cmdindex.inc"
#include "线程锁/lock_cmdindex.inc"
#include "线程锁/join_cmdindex.inc"
#include "线程锁/sync_state_assert.inc"
#include "线程锁/cancel_cmdindex.inc"

// CURL_证书信息 = { 整数型 数量, 整数型 链表数组 }：原模块的自定义类型，无成员命令
static LIB_DATA_TYPE_ELEMENT g_certElements[] = {
    { SDT_INT, NULL, _T("数量"), NULL, _T("证书个数。"), 0, 0 },
    { SDT_INT, NULL, _T("链表数组"), NULL, _T("curl 证书链表指针，逐条文本用 取信息文本数组 取。"), 0, 0 },
};

// formData = { 文本型 文件名, 字节集 提交数据, 文本型 文件格式, 文本型 表单名称 }：网页_构建提交表单数据 的元素类型
static LIB_DATA_TYPE_ELEMENT g_formElements[] = {
    { SDT_TEXT, NULL, _T("文件名"), NULL, _T("有文件名就按上传文件处理。"), 0, 0 },
    { SDT_BIN, NULL, _T("提交数据"), NULL, _T("该项要提交的字节集。"), 0, 0 },
    { SDT_TEXT, NULL, _T("文件格式"), NULL, _T("Content-Type，如 image/png；留空且带文件名时按扩展名推断。"), 0, 0 },
    { SDT_TEXT, NULL, _T("表单名称"), NULL, _T("name= 的字段名。"), 0, 0 },
};

// CURL_消息 = { 整数型 msg, 整数型 easy_handle, 整数型 result }：curl_multi_info_read 的消息体
static LIB_DATA_TYPE_ELEMENT g_msgElements[] = {
    { SDT_INT, NULL, _T("msg"), NULL, _T("消息类型，取 #CURLMSG_DONE 常量。"), 0, 0 },
    { SDT_INT, NULL, _T("easy_handle"), NULL, _T("这条消息属于哪个 curl_easy 句柄。"), 0, 0 },
    { SDT_INT, NULL, _T("result"), NULL, _T("执行结果，取 #CURLE_ 常量，CURLE_OK 为成功。"), 0, 0 },
};

LIB_DATA_TYPE_INFO g_dataTypes[] = {
    CX_TYPE("浏览器会话", "BrowserSession",
        "一次浏览器化的 HTTP 客户端：保存指纹型号、代理、cookie、超时等配置，并在多次请求间复用连接。",
        g_sessionCmdIndex),
    CX_TYPE("浏览器请求", "BrowserRequest",
        "单次调用的参数集合：网址、方法、正文、请求头、下载路径、上传文件。",
        g_requestCmdIndex),
    CX_TYPE("浏览器响应", "BrowserResponse",
        "一次调用的结果：状态码、正文、响应头、错误信息、耗时与最终网址。",
        g_responseCmdIndex),
    CX_TYPE("宽文本类", "WideText",
        "UTF-16 文本缓冲：追加、插入、替换、查找、裁剪，位置从 0 开始。",
        g_wstrCmdIndex),
    CX_TYPE("类_正则表达式类W", "RegExpW",
        "VBScript.RegExp 封装：表达式与被搜索文本都用宽文本（字节集）。",
        g_rxWCmdIndex),
    CX_TYPE("类_正则表达式_兼容版W", "RegExpCompatW",
        "与原 UNICODE 模块同名类行为一致的正则封装，支持 UTF-8 与宽文本两套入口。",
        g_rxcCmdIndex),
    CX_TYPE("类_正则表达式_兼容版2", "RegExpCompat2",
        "恒为全局匹配、单行模式不开启的正则封装。",
        g_rxc2CmdIndex),
    CX_TYPE("类_正则表达式_简单版W", "RegExpSimpleW",
        "先设置表达式，匹配时再传文本的简易正则。",
        g_rxsCmdIndex),
// ==== JSON2 Json2 类登记（gen_json2_table.mjs 生成，勿手改）====
    CX_TYPE("Json2", "JsonValue",
        "zyJson 同源实现：JSON 解析与读写；属性按插入顺序输出；W 后缀用 UTF-16 字节集，文本型按 GBK。",
        g_json2CmdIndex),
// ==== JSON2 Json2 类登记 结束 ====
    { _T("CURL_证书信息"), "CurlCertInfo", _T("curl_easy_getinfo(#CURLINFO_CERTINFO) 的证书链信息。"),
      0, NULL, 0, 0, 0, NULL, 0, NULL, NULL,
      sizeof(g_certElements) / sizeof(g_certElements[0]), g_certElements },
// ==== HTTP访问 类登记（gen_http_table.mjs 生成，勿手改；curl_easy 起，数据类型序号 11..15）====
#include "http访问/cxhttp_typereg.inc"
    { _T("formData"), "CxFormData", _T("网页_构建提交表单数据 的表单元素（原模块同名自定义类型）。"),
      0, NULL, 0, 0, 0, NULL, 0, NULL, NULL,
      sizeof(g_formElements) / sizeof(g_formElements[0]), g_formElements },
    { _T("CURL_消息"), "CurlMsg", _T("curl_multi.取信息（）读出的一条完成消息。"),
      0, NULL, 0, 0, 0, NULL, 0, NULL, NULL,
      sizeof(g_msgElements) / sizeof(g_msgElements[0]), g_msgElements },
#include "线程锁/lock_typereg.inc"
#include "线程锁/join_typereg.inc"
#include "线程锁/cancel_typereg.inc"
    // 纯数据类型 formData=21 / CURL_消息=20：都排在生成器登记的 11..19 之后
    // 实际位置（数组下标 + 1）必须等于序号宏：登记顺序一改就编译不过，不允许静默错位。
    // 见 g_dataTypes 上方的 CX_TYPE_* 定义。
};

const TCHAR g_categories[] = _T("0000测试\0" "0000网络请求\0" "0000宽文本\0" "0000正则表达式\0" "0000编码转换\0" "0000HTTP访问\0\0");

#endif  // !__E_STATIC_LIB


// 取库版本标识：把"哪一次链接产物"直接打到易语言的调试输出里。
void LibVersionTag(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    char szTag[160];
    // 静态编译时命令表整段不参与编译（__E_STATIC_LIB 下只留实现），cmds 报 0。
    INT nCmdCount = 0;
#ifndef __E_STATIC_LIB
    nCmdCount = (INT)(sizeof(g_commands) / sizeof(g_commands[0]));
#endif
    wsprintfA(szTag, "cxsoft %d.%d.%d build %s %s cmds=%d argstride=%d",
              CX_VER_MAJOR, CX_VER_MINOR, CX_VER_BUILD, __DATE__, __TIME__,
              nCmdCount, (INT)sizeof(MDATA_INF));
    SetRetText(pRetData, szTag);
}

// ============================================================ 库通知
// 2026-10-14 被 gen_static_trampolines.mjs 的一次误写截掉，按原逻辑重建。
INT WINAPI cxsoft_ProcessNotifyLib(INT nMsg, DWORD dwParam1, DWORD dwParam2)
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
        return (INT)CX_NOTIFY_LIB_FUNC_NAME;
    case NL_GET_DEPENDENT_LIBS:
        return (INT)NULL;
#endif
    case NL_FREE_LIB_DATA:
    case NL_UNLOAD_FROM_IDE:
        g_pfnNotifySys = NULL;
        return NR_OK;
    default:
        return NR_ERR;
    }
}

// ============================================================ 库信息
// 2026-10-14 由 gen_static_trampolines.mjs 的一次误写截掉，按原字段顺序重建：
// 类别数 5 -> 6（新增「HTTP访问」），其余字段与 3.4.0 一致。
#ifndef __E_STATIC_LIB

LIB_INFO g_libraryInfo = {
    LIB_FORMAT_VER,
    _T("CADE64471347478381C0C610A22C47D0"),
    CX_VER_MAJOR, CX_VER_MINOR, CX_VER_BUILD,
    3, 0,
    3, 0,
    CX_LIB_NAME,
    __GBK_LANG_VER,
    _T("创想软件支持库：浏览器指纹 HTTP 请求（libcurl-impersonate 封装）+ 原生宽文本/正则命令 + HTTP访问 族。"),
    _LIB_OS(__OS_WIN),
    _T("创想软件"),
    _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""),
    sizeof(g_dataTypes) / sizeof(g_dataTypes[0]),
    g_dataTypes,
    6, const_cast<TCHAR*>(g_categories),
    sizeof(g_commands) / sizeof(g_commands[0]),
    g_commands,
    g_commandFuncs,
    NULL, NULL,
    cxsoft_ProcessNotifyLib,
    NULL, NULL,
    sizeof(g_constants) / sizeof(g_constants[0]), g_constants,
    const_cast<TCHAR*>(_T(CX_CURL_DLL_NAME " ")),
};

#endif  // !__E_STATIC_LIB

}  // namespace cx

// ==== 以下由 gen_static_trampolines.mjs 生成，勿手改 ====
// 静态编译时易语言按名字到 cxsoft_static.lib 里找命令实现，
// 而实现在 namespace cx 内，故每个命令生成一份 extern "C" 全局转发。
#define CX_FWD(fn) extern "C" void fn (PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf) { cx::fn (pRetData, nArgCount, pArgInf); }

CX_FWD(cxsoft_Test)
CX_FWD(cxsoft_GetProfileList)
CX_FWD(cxsoft_GetEngineInfo)
CX_FWD(cxsoft_UrlEncode)
CX_FWD(cxsoft_UrlDecode)
CX_FWD(cxsoft_Utf8ToGbk)
CX_FWD(cxsoft_QuickGet)
CX_FWD(SessConstruct)
CX_FWD(SessCopy)
CX_FWD(SessDestruct)
CX_FWD(SessSetProfile)
CX_FWD(SessGetProfile)
CX_FWD(SessSetProxy)
CX_FWD(SessSetProxyAuth)
CX_FWD(SessSetSiteAuth)
CX_FWD(SessSetUserAgent)
CX_FWD(SessSetTimeout)
CX_FWD(SessSetConnTimeout)
CX_FWD(SessSetLowSpeed)
CX_FWD(SessSetVerifyCert)
CX_FWD(SessSetCA)
CX_FWD(SessSetFollow)
CX_FWD(SessSetMaxRedirect)
CX_FWD(SessSetCookieFile)
CX_FWD(SessSetCookieText)
CX_FWD(SessGetCookieText)
CX_FWD(SessSetHeader)
CX_FWD(SessDelHeader)
CX_FWD(SessClearHeader)
CX_FWD(SessGetHeaderText)
CX_FWD(SessSetDebug)
CX_FWD(SessGetDebugLog)
CX_FWD(SessExecute)
CX_FWD(SessGetErrCode)
CX_FWD(SessGetErrText)
CX_FWD(SessReset)
CX_FWD(ReqConstruct)
CX_FWD(ReqCopy)
CX_FWD(ReqDestruct)
CX_FWD(ReqSetUrl)
CX_FWD(ReqGetUrl)
CX_FWD(ReqSetMethod)
CX_FWD(ReqGetMethod)
CX_FWD(ReqSetBodyText)
CX_FWD(ReqSetBodyBin)
CX_FWD(ReqGetBodyLen)
CX_FWD(ReqSetHeader)
CX_FWD(ReqClearHeader)
CX_FWD(ReqSetOutFile)
CX_FWD(ReqSetResumeFrom)
CX_FWD(ReqAddFile)
CX_FWD(ReqClearFiles)
CX_FWD(RespConstruct)
CX_FWD(RespCopy)
CX_FWD(RespDestruct)
CX_FWD(RespGetStatus)
CX_FWD(RespIsOk)
CX_FWD(RespGetErrCode)
CX_FWD(RespGetErrText)
CX_FWD(RespGetText)
CX_FWD(RespGetTextGbk)
CX_FWD(RespGetBin)
CX_FWD(RespGetHeadText)
CX_FWD(RespGetHeadCount)
CX_FWD(RespGetHeadName)
CX_FWD(RespGetHeadValue)
CX_FWD(RespGetHeadByName)
CX_FWD(RespGetFinalUrl)
CX_FWD(RespGetContentType)
CX_FWD(RespGetRedirectCount)
CX_FWD(RespGetElapsedMs)
CX_FWD(RespGetDownloadSize)
CX_FWD(RespSaveToFile)
CX_FWD(UwTextLen)
CX_FWD(UwLeft)
CX_FWD(UwRight)
CX_FWD(UwMid)
CX_FWD(UwCharW)
CX_FWD(UwGetCode)
CX_FWD(UwFind)
CX_FWD(UwRFind)
CX_FWD(UwUpper)
CX_FWD(UwLower)
CX_FWD(UwFullWidth)
CX_FWD(UwHalfWidth)
CX_FWD(UwCat)
CX_FWD(UwCompare)
CX_FWD(UwTrim)
CX_FWD(UwTrimSpace)
CX_FWD(UwReplace)
CX_FWD(UwSubReplace)
CX_FWD(UwSubReplace2)
CX_FWD(UwBlankText)
CX_FWD(UwRepeatText)
CX_FWD(UwSplit2)
CX_FWD(UwSplit)
CX_FWD(UwPtrToText)
CX_FWD(UwTextPtr)
CX_FWD(UwTextToInt)
CX_FWD(UwIntToText)
CX_FWD(UwReadFile)
CX_FWD(UwWriteFile)
CX_FWD(UwMidBetween)
CX_FWD(UwMidBetween2)
CX_FWD(UwRMidBetween)
CX_FWD(UwRMidBetween2)
CX_FWD(UwTextBefore)
CX_FWD(UwTextBefore2)
CX_FWD(UwTextAfter)
CX_FWD(UwTextAfter2)
CX_FWD(UwTextExists)
CX_FWD(UwRegMatch)
CX_FWD(UwRegMatchAllSub)
CX_FWD(UwRegMatch2)
CX_FWD(UwPickDigits)
CX_FWD(UwRegReplace)
CX_FWD(UwRegSubReplace)
CX_FWD(UwAryJoin)
CX_FWD(UwMidBatch)
CX_FWD(UwMidBatch3)
CX_FWD(UwCountOccur)
CX_FWD(UwMidBatch2)
CX_FWD(UwCountOccur2)
CX_FWD(UwHtmlMid)
CX_FWD(UwAryShuffle)
CX_FWD(UwAryJoinNull)
CX_FWD(UwSubReplaceAry)
CX_FWD(UwAryUnique)
CX_FWD(UexFormatW)
CX_FWD(UexFormatW2)
CX_FWD(UexVarAddr)
CX_FWD(UexThisCall)
CX_FWD(UexStructPtr)
CX_FWD(UexWTextFix)
CX_FWD(UexWF)
CX_FWD(UexWC)
CX_FWD(UexWTextIsEmpty)
CX_FWD(UexWIsNull)
CX_FWD(UexW_ISNULL)
CX_FWD(UexWTextEmpty)
CX_FWD(UexWToInt)
CX_FWD(UexWToDouble)
CX_FWD(UexDoubleToW)
CX_FWD(UexBinBlank)
CX_FWD(UexWTextEqual)
CX_FWD(UexCoFree)
CX_FWD(UexCoInit)
CX_FWD(WsConstruct)
CX_FWD(WsCopy)
CX_FWD(WsDestruct)
CX_FWD(WsGetPtr)
CX_FWD(WsSetPtr)
CX_FWD(WsCreate)
CX_FWD(WsAddRes)
CX_FWD(WsAddPtrW)
CX_FWD(WsAddPtrA)
CX_FWD(WsAdd)
CX_FWD(WsAddW)
CX_FWD(WsAddSelf)
CX_FWD(WsAddLine)
CX_FWD(WsAddLineW)
CX_FWD(WsAddLineSelf)
CX_FWD(WsAddInt)
CX_FWD(WsInsertText)
CX_FWD(WsInsertW)
CX_FWD(WsInsertSelf)
CX_FWD(WsGetText)
CX_FWD(WsGetTextW)
CX_FWD(WsGetTextWN)
CX_FWD(WsGetStrPtr)
CX_FWD(WsCmdClear)
CX_FWD(WsCmdDestroy)
CX_FWD(WsAddChar)
CX_FWD(WsGetChar)
CX_FWD(WsSetChar)
CX_FWD(WsCmdCompare)
CX_FWD(WsCmdCompareW)
CX_FWD(WsCmdCompareSelf)
CX_FWD(WsCmdLength)
CX_FWD(WsCmdClone)
CX_FWD(WsCmdUpper)
CX_FWD(WsCmdLower)
CX_FWD(WsCmdFull)
CX_FWD(WsCmdHalf)
CX_FWD(WsCmdFindW)
CX_FWD(WsCmdFind)
CX_FWD(WsCmdFindSelf)
CX_FWD(WsCmdRFindW)
CX_FWD(WsCmdRFind)
CX_FWD(WsCmdRFindSelf)
CX_FWD(WsCmdReplaceW)
CX_FWD(WsCmdReplace)
CX_FWD(WsCmdReplaceSelf)
CX_FWD(WsCmdSubW)
CX_FWD(WsCmdSub)
CX_FWD(WsCmdSubSelf)
CX_FWD(WsCmdMid)
CX_FWD(WsCmdLeft)
CX_FWD(WsCmdRight)
CX_FWD(WsCmdTrimW)
CX_FWD(WsCmdTrim)
CX_FWD(WsCmdTrimSelf)
CX_FWD(WsCmdTrimSpace)
CX_FWD(WsCmdToInt)
CX_FWD(WsCmdSplitSelf)
CX_FWD(WsCmdSplitW)
CX_FWD(WsCmdSplit)
CX_FWD(RxConstruct)
CX_FWD(RxCopy)
CX_FWD(RxDestruct)
CX_FWD(RxCmdCreate)
CX_FWD(RxCmdView)
CX_FWD(RxCmdPattern)
CX_FWD(RxCmdReplace)
CX_FWD(RxCmdCount)
CX_FWD(RxCmdValueW)
CX_FWD(RxCmdSubValueW)
CX_FWD(RxCmdSubCount)
CX_FWD(RxcConstruct)
CX_FWD(RxcCopy)
CX_FWD(RxcDestruct)
CX_FWD(RxcCreate)
CX_FWD(RxcCreateW)
CX_FWD(RxcView)
CX_FWD(RxcPattern)
CX_FWD(RxcReplace)
CX_FWD(RxcCount)
CX_FWD(RxcValue)
CX_FWD(RxcValueW)
CX_FWD(RxcSubValue)
CX_FWD(RxcSubValueW)
CX_FWD(RxcSubCount)
CX_FWD(Rxc2Construct)
CX_FWD(Rxc2Copy)
CX_FWD(Rxc2Destruct)
CX_FWD(Rxc2Create)
CX_FWD(Rxc2CreateW)
CX_FWD(Rxc2Pattern)
CX_FWD(Rxc2PatternW)
CX_FWD(Rxc2Replace)
CX_FWD(Rxc2ReplaceW)
CX_FWD(Rxc2Count)
CX_FWD(Rxc2Value)
CX_FWD(Rxc2SubValue)
CX_FWD(Rxc2SubCount)
CX_FWD(RxsConstruct)
CX_FWD(RxsCopy)
CX_FWD(RxsDestruct)
CX_FWD(RxsCreate)
CX_FWD(RxsMatch)
CX_FWD(RxsSearchAll)
CX_FWD(RxsValue)
CX_FWD(RxsSubCount)
CX_FWD(RxsSubValue)
CX_FWD(RxsCreateW)
CX_FWD(RxsSearchAllW)
CX_FWD(RxsValueW)
CX_FWD(RxsSubValueW)
CX_FWD(UcW)
CX_FWD(UcA)
CX_FWD(UcA2W)
CX_FWD(UcA2WNo)
CX_FWD(UcW2A)
CX_FWD(UcW2Utf8)
CX_FWD(UcW2Utf8T)
CX_FWD(UcUtf82W)
CX_FWD(UcUtf82WT)
CX_FWD(UcUtf82A)
CX_FWD(UcUtf82AT)
CX_FWD(UcA2Utf8)
CX_FWD(UcA2Utf8T)
CX_FWD(UcUtf8ToGb)
CX_FWD(UcGbToUtf8)
CX_FWD(UcWideTrimNull)
CX_FWD(UcUrlEncodeW)
CX_FWD(UcUrlDecode)
CX_FWD(UcUrlDecodeJs)
CX_FWD(UcHtmlDecodeA)
CX_FWD(UwSplitAry)
CX_FWD(LibVersionTag)
CX_FWD(J2ObjConstruct)
CX_FWD(J2ObjCopy)
CX_FWD(J2ObjDestruct)
CX_FWD(J2SetCode)
CX_FWD(J2SetCodeUtf8)
CX_FWD(J2SetCodeW)
CX_FWD(J2Release)
CX_FWD(J2SetTopFlag)
CX_FWD(J2GetPtr)
CX_FWD(J2SetPtr)
CX_FWD(J2CreateObject)
CX_FWD(J2CreateArray)
CX_FWD(J2CreateNull)
CX_FWD(J2CreateBool)
CX_FWD(J2CreateTextW)
CX_FWD(J2CreateText)
CX_FWD(J2CreateDouble)
CX_FWD(J2CreateLong)
CX_FWD(J2ParseW)
CX_FWD(J2Parse)
CX_FWD(J2ToTextW)
CX_FWD(J2ToText)
CX_FWD(J2GetTyp)
CX_FWD(J2GetTypW)
CX_FWD(J2GetParent)
CX_FWD(J2GetParentW)
CX_FWD(J2GetKeyCount)
CX_FWD(J2GetKeyCountW)
CX_FWD(J2Detach)
CX_FWD(J2RemoveAttrW)
CX_FWD(J2RemoveAttr)
CX_FWD(J2RemoveItem)
CX_FWD(J2RemoveItemW)
CX_FWD(J2Clear)
CX_FWD(J2ClearW)
CX_FWD(J2SetAttr)
CX_FWD(J2SetAttrW)
CX_FWD(J2K)
CX_FWD(J2KUtf8)
CX_FWD(J2KW)
CX_FWD(J2K2)
CX_FWD(J2K2W)
CX_FWD(J2GetAttrNameW)
CX_FWD(J2GetAttrName)
CX_FWD(J2AppendItem)
CX_FWD(J2AppendItemW)
CX_FWD(J2InsertItem)
CX_FWD(J2InsertItemW)
CX_FWD(J2I)
CX_FWD(J2IW)
CX_FWD(J2I2)
CX_FWD(J2I2W)
CX_FWD(J2SetItem)
CX_FWD(J2SetItemW)
CX_FWD(J2ExistsW)
CX_FWD(J2Exists)
CX_FWD(J2Select)
CX_FWD(J2SelectW)
CX_FWD(J2SetObject)
CX_FWD(J2SetObjectW)
CX_FWD(J2SetArray)
CX_FWD(J2SetArrayW)
CX_FWD(J2SetText)
CX_FWD(J2SetTextUtf8)
CX_FWD(J2SetTextW)
CX_FWD(J2SetNullW)
CX_FWD(J2SetNull)
CX_FWD(J2SetBoolW)
CX_FWD(J2SetBool)
CX_FWD(J2SetDoubleW)
CX_FWD(J2SetDouble)
CX_FWD(J2SetNumberW)
CX_FWD(J2SetNumber)
CX_FWD(J2SetLongW)
CX_FWD(J2SetLong)
CX_FWD(J2SetIntW)
CX_FWD(J2SetInt)
CX_FWD(J2GetRaw)
CX_FWD(J2GetRawW)
CX_FWD(J2GetText)
CX_FWD(J2GetTextUtf8)
CX_FWD(J2GetTextW)
CX_FWD(J2GetBoolW)
CX_FWD(J2GetBoolA)
CX_FWD(J2GetDoubleW)
CX_FWD(J2GetDoubleA)
CX_FWD(J2GetNumberW)
CX_FWD(J2GetNumberA)
CX_FWD(J2GetLongW)
CX_FWD(J2GetLongA)
CX_FWD(J2GetIntW)
CX_FWD(J2GetIntA)
CX_FWD(J2GetAllKeysW)
CX_FWD(J2GetAllKeys)
CX_FWD(J2GetAllValuesW)
CX_FWD(J2GetAllValues)
CX_FWD(J2CopyW)
CX_FWD(J2Copy)
CX_FWD(J2SortObjW)
CX_FWD(J2SortObj)
CX_FWD(J2SortW)
CX_FWD(J2Sort)
CX_FWD(J2MergeW)
CX_FWD(J2Merge)
CX_FWD(J2MergeObjPlus)
CX_FWD(J2MergeObj)
CX_FWD(J2MergeArr)
CX_FWD(J2InsertAttrW)
CX_FWD(J2InsertAttr)
CX_FWD(J2SetTextAry)
CX_FWD(J2SetTextAryW)
CX_FWD(J2SetTextAryUtf8)
CX_FWD(J2SetNumberAryUtf8)
CX_FWD(J2SetIntAryUtf8)
CX_FWD(J2SetBoolAryUtf8)
CX_FWD(CurlEasyConstruct)
CX_FWD(CurlEasyCopy)
CX_FWD(CurlEasyDestruct)
CX_FWD(CurlEasy_Create)
CX_FWD(CurlEasy_Destroy)
CX_FWD(CurlEasy_GetHandle)
CX_FWD(CurlEasy_SetHandle)
CX_FWD(CurlEasy_DupHandle)
CX_FWD(CurlEasy_SetoptInt)
CX_FWD(CurlEasy_SetoptLong)
CX_FWD(CurlEasy_SetoptText)
CX_FWD(CurlEasy_SetoptBin)
CX_FWD(CurlEasy_Perform)
CX_FWD(CurlEasy_GetinfoInt)
CX_FWD(CurlEasy_GetinfoDouble)
CX_FWD(CurlEasy_GetinfoText)
CX_FWD(CurlEasy_GetinfoTextAry)
CX_FWD(CurlEasy_GetinfoCert)
CX_FWD(CurlEasy_Reset)
CX_FWD(CurlEasy_Escape)
CX_FWD(CurlEasy_Unescape)
CX_FWD(CurlHttpConstruct)
CX_FWD(CurlHttpCopy)
CX_FWD(CurlHttpDestruct)
CX_FWD(CurlHttp_Create)
CX_FWD(CurlHttp_Destroy)
CX_FWD(CurlHttp_GetHandle)
CX_FWD(CurlHttp_SetHandle)
CX_FWD(CurlHttp_DupHandle)
CX_FWD(CurlHttp_SetoptInt)
CX_FWD(CurlHttp_SetoptLong)
CX_FWD(CurlHttp_SetoptText)
CX_FWD(CurlHttp_SetoptBin)
CX_FWD(CurlHttp_Perform)
CX_FWD(CurlHttp_GetinfoInt)
CX_FWD(CurlHttp_GetinfoDouble)
CX_FWD(CurlHttp_GetinfoText)
CX_FWD(CurlHttp_GetinfoTextAry)
CX_FWD(CurlHttp_GetinfoCert)
CX_FWD(CurlHttp_Reset)
CX_FWD(CurlHttp_Escape)
CX_FWD(CurlHttp_Unescape)
CX_FWD(CurlHttp_SetXferinfo)
CX_FWD(CurlHttp_SetXferinfoAddr)
CX_FWD(CurlHttp_SetUrl)
CX_FWD(CurlHttp_SetProxy)
CX_FWD(CurlHttp_Impersonate)
CX_FWD(CurlHttp_SetUserPwd)
CX_FWD(CurlHttp_SetCookieFile)
CX_FWD(CurlHttp_SetCookieList)
CX_FWD(CurlHttp_SetCookieText)
CX_FWD(CurlHttp_SetTimeoutMs)
CX_FWD(CurlHttp_SetTimeout)
CX_FWD(CurlHttp_SetPost)
CX_FWD(CurlHttp_SetPostBin)
CX_FWD(CurlHttp_SetPostText)
CX_FWD(CurlHttp_SetFollow)
CX_FWD(CurlHttp_SetKeepAlive)
CX_FWD(CurlHttp_SetEncoding)
CX_FWD(CurlHttp_SetHeaders)
CX_FWD(CurlHttp_SetBufSize)
CX_FWD(CurlHttp_SetReferer)
CX_FWD(CurlHttp_SetHttpVersion)
CX_FWD(CurlHttp_SetUserAgent)
CX_FWD(CurlHttp_SetCaInfo)
CX_FWD(CurlHttp_SetVerify)
CX_FWD(CurlHttp_RespBody)
CX_FWD(CurlHttp_RespHead)
CX_FWD(CurlHttp_RespHeadBin)
CX_FWD(CurlHttp_RespCode)
CX_FWD(CurlHttp_RespTime)
CX_FWD(CurlHttp_RespUrl)
CX_FWD(CurlHttp_RespRedirectUrl)
CX_FWD(CurlHttp_RespRemoteIp)
CX_FWD(CurlHttp_RespRemotePort)
CX_FWD(CurlHttp_RespLocalIp)
CX_FWD(CurlHttp_RespLocalPort)
CX_FWD(CurlHttp_RespCookies)
CX_FWD(CurlSmtpConstruct)
CX_FWD(CurlSmtpCopy)
CX_FWD(CurlSmtpDestruct)
CX_FWD(CurlSmtp_Create)
CX_FWD(CurlSmtp_Destroy)
CX_FWD(CurlSmtp_GetHandle)
CX_FWD(CurlSmtp_SetHandle)
CX_FWD(CurlSmtp_DupHandle)
CX_FWD(CurlSmtp_SetoptInt)
CX_FWD(CurlSmtp_SetoptLong)
CX_FWD(CurlSmtp_SetoptText)
CX_FWD(CurlSmtp_SetoptBin)
CX_FWD(CurlSmtp_Perform)
CX_FWD(CurlSmtp_GetinfoInt)
CX_FWD(CurlSmtp_GetinfoDouble)
CX_FWD(CurlSmtp_GetinfoText)
CX_FWD(CurlSmtp_GetinfoTextAry)
CX_FWD(CurlSmtp_GetinfoCert)
CX_FWD(CurlSmtp_Reset)
CX_FWD(CurlSmtp_Escape)
CX_FWD(CurlSmtp_Unescape)
CX_FWD(CurlSmtp_SetXferinfo)
CX_FWD(CurlSmtp_SetXferinfoAddr)
CX_FWD(CurlSmtp_SetServer)
CX_FWD(CurlSmtp_SetCaInfo)
CX_FWD(CurlSmtp_SetVerify)
CX_FWD(CurlSmtp_SetMail)
CX_FWD(CurlMultiConstruct)
CX_FWD(CurlMultiCopy)
CX_FWD(CurlMultiDestruct)
CX_FWD(CurlMulti_Create)
CX_FWD(CurlMulti_Destroy)
CX_FWD(CurlMulti_GetHandle)
CX_FWD(CurlMulti_SetHandle)
CX_FWD(CurlMulti_AddHandle)
CX_FWD(CurlMulti_RemoveHandle)
CX_FWD(CurlMulti_Perform)
CX_FWD(CurlMulti_Wait)
CX_FWD(CurlMulti_InfoRead)
CX_FWD(CurlMulti_Setopt)
CX_FWD(UrlParamConstruct)
CX_FWD(UrlParamCopy)
CX_FWD(UrlParamDestruct)
CX_FWD(UrlParam_InitUrl)
CX_FWD(UrlParam_AddOrModify)
CX_FWD(UrlParam_GetUrl)
CX_FWD(UrlParam_ModifyValue)
CX_FWD(UrlParam_ModifyName)
CX_FWD(UrlParam_RemoveParam)
CX_FWD(UrlParam_AddParam)
CX_FWD(UrlParam_FindParam)
CX_FWD(AccessCurlConstruct)
CX_FWD(AccessCurlCopy)
CX_FWD(AccessCurlDestruct)
CX_FWD(AccessCurl_SetRecvChunk)
CX_FWD(AccessCurl_SetCookie)
CX_FWD(AccessCurl_SetDns)
CX_FWD(AccessCurl_GetSubmitCookie)
CX_FWD(AccessCurl_GetRetCookie)
CX_FWD(AccessCurl_SetCurlHandle)
CX_FWD(AccessCurl_SetShareDns)
CX_FWD(AccessCurl_SetOption)
CX_FWD(AccessCurl_SetOptionInt)
CX_FWD(AccessCurl_Impersonate)
CX_FWD(AccessCurl_AccessPageCurl)
CX_FWD(AccessWinHttpConstruct)
CX_FWD(AccessWinHttpCopy)
CX_FWD(AccessWinHttpDestruct)
CX_FWD(AccessWinHttp_GetLastHeader)
CX_FWD(AccessWinHttp_SetRecvChunk)
CX_FWD(AccessWinHttp_SetCookie)
CX_FWD(AccessWinHttp_GetSubmitCookie)
CX_FWD(AccessWinHttp_GetRetCookie)
CX_FWD(AccessWinHttp_AccessPageWinHttp)
CX_FWD(AccessWinInetConstruct)
CX_FWD(AccessWinInetCopy)
CX_FWD(AccessWinInetDestruct)
CX_FWD(AccessWinInet_SetRecvChunk)
CX_FWD(AccessWinInet_SetCookie)
CX_FWD(AccessWinInet_GetSubmitCookie)
CX_FWD(AccessWinInet_GetRetCookie)
CX_FWD(AccessWinInet_AccessPageWinInet)
CX_FWD(G_WebAccessCurl)
CX_FWD(G_WebAccessWinHttp)
CX_FWD(G_WebAccessWinInet)
CX_FWD(G_WebCookieMerge)
CX_FWD(G_WebCookieFromHead)
CX_FWD(G_WebPickHeader)
CX_FWD(G_WebBuildHeaders)
CX_FWD(G_WebSystemProxy)
CX_FWD(G_JudgeHeadValid)
CX_FWD(G_GetHeadCode)
CX_FWD(G_WebStatusCode)
CX_FWD(G_WebBuildForm)
CX_FWD(G_WebContentType)
CX_FWD(G_AnalyseHeadCookies)
CX_FWD(G_GetMiddleText)
CX_FWD(G_CookieMerge)
CX_FWD(G_WebCookieInner)
CX_FWD(G_WebCookieOne)
CX_FWD(G_TextCountOccur)
CX_FWD(G_CompressAmazonPage)
CX_FWD(G_CurlPreload)
CX_FWD(G_CurlShutdown)
CX_FWD(CxLockConstruct)
CX_FWD(CxLockCopy)
CX_FWD(CxLockDestruct)
CX_FWD(CxLock_Create)
CX_FWD(CxLock_Destroy)
CX_FWD(CxLock_Enter)
CX_FWD(CxLock_Leave)
CX_FWD(CxLock_TryEnter)
CX_FWD(CxLock_Wait)
CX_FWD(CxLock_Signal)
CX_FWD(CxLock_Broadcast)
CX_FWD(CxLock_IsLocked)
CX_FWD(CxLock_Owner)
CX_FWD(CxLock_Recursion)
CX_FWD(CxLock_Waiters)
CX_FWD(CxLock_Named)
CX_FWD(CxLock_SetHandle)
CX_FWD(CxLock_GetHandle)
CX_FWD(CxSemConstruct)
CX_FWD(CxSemCopy)
CX_FWD(CxSemDestruct)
CX_FWD(CxSem_Create)
CX_FWD(CxSem_Destroy)
CX_FWD(CxSem_Wait)
CX_FWD(CxSem_Release)
CX_FWD(CxSem_TryWait)
CX_FWD(CxSem_Value)
CX_FWD(CxCondConstruct)
CX_FWD(CxCondCopy)
CX_FWD(CxCondDestruct)
CX_FWD(CxCond_Create)
CX_FWD(CxCond_Destroy)
CX_FWD(CxCond_Wait)
CX_FWD(CxCond_Signal)
CX_FWD(CxCond_Broadcast)
CX_FWD(CxCond_Waiters)
CX_FWD(AccessCurl_BindStop)
CX_FWD(AccessCurl_BindStopPointer)
CX_FWD(AccessCurl_GetLastError)
CX_FWD(AccessWinHttp_BindStop)
CX_FWD(AccessWinHttp_BindStopPointer)
CX_FWD(AccessWinHttp_GetLastError)
CX_FWD(AccessWinInet_BindStop)
CX_FWD(AccessWinInet_BindStopPointer)
CX_FWD(AccessWinInet_GetLastError)
CX_FWD(G_HttpExchangeStop)
CX_FWD(CxJoinConstruct)
CX_FWD(CxJoinCopy)
CX_FWD(CxJoinDestruct)
CX_FWD(CxJoin_Create)
CX_FWD(CxJoin_Wait)
CX_FWD(CxJoin_Cancel)
CX_FWD(CxJoin_Destroy)
CX_FWD(CxJoin_Arrived)
CX_FWD(CxJoin_IsComplete)
CX_FWD(CxJoin_IsCancelled)
CX_FWD(CxLock_IsCreated)
CX_FWD(CxSem_IsCreated)
CX_FWD(CxCond_IsCreated)
CX_FWD(CxJoin_IsCreated)
CX_FWD(CxJoin_EnsureCreated)
CX_FWD(CxCancelConstruct)
CX_FWD(CxCancelCopy)
CX_FWD(CxCancelDestruct)
CX_FWD(CxCancel_Create)
CX_FWD(CxCancel_Cancel)
CX_FWD(CxCancel_Pause)
CX_FWD(CxCancel_Resume)
CX_FWD(CxCancel_IsCancelled)
CX_FWD(CxCancel_IsPaused)
CX_FWD(CxCancel_WaitCancel)
CX_FWD(CxCancel_WaitContinue)
CX_FWD(CxCancel_IsCreated)
CX_FWD(CxCancel_Destroy)
CX_FWD(AccessCurl_BindToken)
CX_FWD(AccessCurl_UnbindToken)
CX_FWD(AccessWinHttp_BindToken)
CX_FWD(AccessWinHttp_UnbindToken)
CX_FWD(AccessWinInet_BindToken)
CX_FWD(AccessWinInet_UnbindToken)

#undef CX_FWD

extern "C" INT WINAPI cxsoft_ProcessNotifyLib (INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    return cx::cxsoft_ProcessNotifyLib (nMsg, dwParam1, dwParam2);
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
