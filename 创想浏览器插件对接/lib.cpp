// 创想浏览器插件对接 —— 原生 C++ 支持库（.fne），32 位、静态 CRT、GBK 输出。
// 源码一律 UTF-8 保存，由 /source-charset:utf-8 /execution-charset:gbk 转成 GBK 产物；
// IDE 里的命令名 / 说明走 GBK（_T），跨库边界的运行期文本一律 UTF-8 原样透传，中文常量出口前过 Zh()。
// 实现分四个文件：bridge_util（文本与 JSON）、bridge_site（站点与网址识别）、bridge_ev（事件队列与子程序绑定）、bridge_srv（本机 HTTP 服务）。

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

// lib2.h 用到 DATE / DOUBLE / FLOAT / INT64，它们原本来自 SDK 的 mtypes.h，但 mtypes.h
// 是 VC6 时代写的，自己重定义了 HWND / FARPROC / MAKEWORD / _T，与 windows.h 冲突，
// 所以这里只补这四个类型定义（重复 typedef 同类型在 C++ 合法）。
typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;

#include <lib2.h>
#include <lang.h>
#include "bridge.h"

#define LIB_NOTIFY_FUNC_NAME "cxbrow_ProcessNotifyLib"

// 自定义类型「浏览器事件信息」在本库数据类型表里的序号（1 起）
#define CX_TYPE_BROW_EVENT 1

namespace cx {

PFN_NOTIFY_SYS g_pfnNotifySys = NULL;

INT NotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    if (g_pfnNotifySys == NULL) return 0;
    return g_pfnNotifySys(nMsg, dwParam1, dwParam2);
}

// 在易语言内存池上建立文本数据，命令返回后由系统释放。
// 必须走 NRS_MALLOC：返回静态缓冲区或 LocalAlloc 指针会让系统释放时踩坏堆。
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

void* PoolMalloc(INT nBytes)
{
    if (nBytes <= 0 || g_pfnNotifySys == NULL) return NULL;
    return (void*)NotifySys(NRS_MALLOC, (DWORD)nBytes, 0);
}

void PoolFree(void* pData)
{
    if (pData == NULL || g_pfnNotifySys == NULL) return;
    NotifySys(NRS_MFREE, (DWORD)(INT_PTR)pData, 0);
}

// ===================== 参数与返回辅助 =====================

// 易语言内核按 1 字节对齐传参：参数数组里每个 MDATA_INF 只占 12 个字节（8 字节 union + 4 字节数据类型码），
// 但本机 MSVC 不认 lib2.h 里那两行 Borland 写法的 pack 指示，sizeof(MDATA_INF) 编出来是 16。
// 于是 pArgInf[i]（i>=1）会整体读高 4 个字节 —— 真机取证：raw=[1 0 80000301 813fd2c 0 20001 1002ec11 ffffffff]，
// 块指针在第 3 个字、类型码 MAKELONG(1,2) 在第 5 个字，正是 12 字节步长的位置。所以一律按 12 取。
// 单个元素内部两个字段仍在 +0 和 +8，所以只有「数组下标」受影响，pRetData 这些单值不受影响。
#define EPL_ARG_BYTES 12
static_assert(sizeof(MDATA_INF) == EPL_ARG_BYTES,
    "参数步长必须等于 MDATA_INF 的大小；lib2.h 的 pack(1) 一旦被关掉，这里就会编译不过");

static PMDATA_INF ArgAt(PMDATA_INF pArgs, INT i)
{
    return (PMDATA_INF)((BYTE*)pArgs + (SIZE_T)i * EPL_ARG_BYTES);
}

static const char* ArgText(PMDATA_INF pArgs, INT i)
{
    if (pArgs == NULL) return "";
    const PMDATA_INF p = ArgAt(pArgs, i);
    if (p->m_dtDataType == _SDT_NULL) return "";
    if (p->m_dtDataType != SDT_TEXT) return "";
    return p->m_pText == NULL ? "" : p->m_pText;
}

static INT ArgInt(PMDATA_INF pArgs, INT i, INT nDefault)
{
    if (pArgs == NULL) return nDefault;
    const PMDATA_INF p = ArgAt(pArgs, i);
    if (p->m_dtDataType == _SDT_NULL) return nDefault;
    return p->m_int;
}

// 逻辑型可空参数：省略时按默认值，非省略时按「非 0 即真」（易语言的 真 是 -1）
static INT ArgBool(PMDATA_INF pArgs, INT i, INT nDefault)
{
    if (pArgs == NULL) return nDefault;
    const PMDATA_INF p = ArgAt(pArgs, i);
    if (p->m_dtDataType == _SDT_NULL) return nDefault;
    return p->m_bool != BL_FALSE ? 1 : 0;
}

static void RetText(PMDATA_INF pRet, const char* pszUtf8)
{
    pRet->m_dtDataType = SDT_TEXT;
    pRet->m_pText = CloneTextData((pszUtf8 != NULL && pszUtf8[0] != '\0') ? pszUtf8 : "");
}

static void RetInt(PMDATA_INF pRet, INT n)
{
    pRet->m_dtDataType = SDT_INT;
    pRet->m_int = n;
}

static void RetBool(PMDATA_INF pRet, INT n)
{
    pRet->m_dtDataType = SDT_BOOL;
    pRet->m_bool = n ? BL_TRUE : BL_FALSE;
}

static void CfgDefaults()
{
    BrInitOnce();
    if (g_cfg.szService[0] == '\0') StrCpyN(g_cfg.szService, sizeof(g_cfg.szService), "cxac");
    if (g_cfg.szApp[0] == '\0') StrCpyN(g_cfg.szApp, sizeof(g_cfg.szApp), Zh("亚马逊ASIN采集软件"));
    if (g_cfg.szVersion[0] == '\0') StrCpyN(g_cfg.szVersion, sizeof(g_cfg.szVersion), "0.0.0");
    if (g_cfg.nPortFirst <= 0) g_cfg.nPortFirst = 47630;
    if (g_cfg.nPortCount <= 0) g_cfg.nPortCount = 4;
    if (g_cfg.nOfflineSec <= 0) g_cfg.nOfflineSec = 120;
    if (g_cfg.nReplyWaitMs <= 0) g_cfg.nReplyWaitMs = 15000;
    g_cfg.bInited = 1;
}

// ===================== 命令实现 =====================

void BrwInit(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    BrInitOnce();
    StrCpyN(g_cfg.szVersion, sizeof(g_cfg.szVersion), ArgText(pArgInf, 0));
    StrCpyN(g_cfg.szRunDir, sizeof(g_cfg.szRunDir), ArgText(pArgInf, 1));
    g_cfg.nDirMode = 0;   // 目录换了，编码判定要重来
    if (ArgAt(pArgInf, 2)->m_dtDataType != _SDT_NULL) StrCpyN(g_cfg.szApp, sizeof(g_cfg.szApp), ArgText(pArgInf, 2));
    if (ArgAt(pArgInf, 3)->m_dtDataType != _SDT_NULL && ArgAt(pArgInf, 3)->m_pText != NULL && ArgAt(pArgInf, 3)->m_pText[0] != '\0')
    {
        StrCpyN(g_cfg.szService, sizeof(g_cfg.szService), ArgText(pArgInf, 3));
    }
    const INT nPort = ArgInt(pArgInf, 4, 0);
    if (nPort > 0 && nPort < 65536) g_cfg.nPortFirst = nPort;
    const INT nCount = ArgInt(pArgInf, 5, 0);
    if (nCount > 0 && nCount <= 16) g_cfg.nPortCount = nCount;
    CfgDefaults();
    // g_cfg 里的字符串是易语言原样传进来的（他的程序是 ANSI，中文目录/程序名就是 GBK 字节），
    // 日志整份是 UTF-8，所以出口过一次 ToUtf8：已是 UTF-8 的原样走，不会二次转码。
    BrLog(Zh("初始化对接信息：版本=%s 程序=%s 标识=%s 端口=%d起%d个 运行目录=%s"),
        ToUtf8(g_cfg.szVersion), ToUtf8(g_cfg.szApp), ToUtf8(g_cfg.szService),
        g_cfg.nPortFirst, g_cfg.nPortCount, ToUtf8(g_cfg.szRunDir));
    RetBool(pRetData, 1);
}

void BrwStart(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    CfgDefaults();
    RetBool(pRetData, SrvStart());
}

void BrwStop(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    SrvStop();
    EvUnbind();
}

void BrwIsRunning(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    RetBool(pRetData, g_stat.listening);
}

void BrwGetPort(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    RetInt(pRetData, g_stat.port);
}

void BrwGetState(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    RetInt(pRetData, g_stat.listening ? (g_stat.connected ? 2 : 1) : 0);
}

void BrwGetError(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    // 监听本身没问题但日志写不进目录，同样是要报的错：否则整份对接过程无声丢失
    RetText(pRetData, g_stat.szErr[0] != '\0' ? g_stat.szErr : g_stat.szLogErr);
}

void BrwSetSites(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    SiteSetFromText(ArgText(pArgInf, 0));
    RetInt(pRetData, SiteCount());
}

void BrwGetSites(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    Buf b;
    BufInit(&b);
    const INT n = SiteCount();
    for (INT i = 0; i < n; ++i)
    {
        BrSite s;
        SiteGet(i, &s);
        if (s.host[0] == '\0') continue;
        if (i) BufAddCh(&b, '\n');
        // 出口统一 UTF-8：内置表里的中文是 GBK 字面量，逐条转一遍
        BufAddF(&b, "%s|%s|%s|%s|%s|%s", s.platform, s.bUtf8 ? s.display : Zh(s.display),
            s.host, s.country, s.currency, s.language);
    }
    RetText(pRetData, b.p == NULL ? "" : b.p);
    BufFree(&b);
}

void BrwMatch(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    BrMatch m;
    MatchUrl(ArgText(pArgInf, 0), &m);
    Buf b;
    BufInit(&b);
    BufAddCh(&b, '{');
    JsonBool(&b, "identifiable", m.bIdentifiable, FALSE);
    JsonStr(&b, "host", m.host, TRUE);
    JsonStr(&b, "platformName", m.platform, TRUE);
    JsonStr(&b, "displayName", m.display, TRUE);
    JsonInt(&b, "pageType", m.nPageType, TRUE);
    JsonInt(&b, "colModType", m.nColMode, TRUE);
    JsonStr(&b, "asin", m.asin, TRUE);
    JsonStr(&b, "country", m.country, TRUE);
    JsonStr(&b, "currency", m.currency, TRUE);
    JsonStr(&b, "language", m.language, TRUE);
    BufAddCh(&b, '}');
    RetText(pRetData, b.p == NULL ? "{}" : b.p);
    BufFree(&b);
}

void BrwSetOffline(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    const INT n = ArgInt(pArgInf, 0, 120);
    if (n < 5 || n > 86400) { RetBool(pRetData, 0); return; }
    EnterCriticalSection(&g_cs);
    g_cfg.nOfflineSec = n;
    LeaveCriticalSection(&g_cs);
    RetBool(pRetData, 1);
}

void BrwSetReplyWait(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    const INT n = ArgInt(pArgInf, 0, 15000);
    if (n < 200 || n > 120000) { RetBool(pRetData, 0); return; }
    EnterCriticalSection(&g_cs);
    g_cfg.nReplyWaitMs = n;
    LeaveCriticalSection(&g_cs);
    RetBool(pRetData, 1);
}

void BrwBindSub(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    DWORD dw = 0;
    if (ArgAt(pArgInf, 0)->m_dtDataType == SDT_SUB_PTR) dw = ArgAt(pArgInf, 0)->m_dwSubCodeAdr;
    const BOOL bOk = EvBindSub(dw);
    // 第 2 参是可选的子程序名：绑定成功就把它记下来，之后派发异常/超时能在日志里点名
    if (bOk && nArgCount > 1 && ArgAt(pArgInf, 1)->m_dtDataType == SDT_TEXT)
        CbSetName(ArgText(pArgInf, 1));
    else
        CbSetName(NULL);
    RetBool(pRetData, bOk);
}

// 事件并发开关：默认假 = 库内串行派发（回调原子性由库保证）；真 = 放开并发，线程安全归程序自己。
void BrwSetConcurrent(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    const INT n = ArgInt(pArgInf, 0, 0);
    EnterCriticalSection(&g_cs);
    g_cfg.nConcurrent = (n == 0) ? 0 : 1;
    LeaveCriticalSection(&g_cs);
    BrLog(Zh("%s：%s"), Zh("事件并发"), n == 0 ? Zh("串行派发（默认，回调不重入）") : Zh("允许并发（线程安全由程序负责）"));
    RetBool(pRetData, 1);
}

void BrwPump(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    RetInt(pRetData, EvPump(ArgInt(pArgInf, 0, 0)));
}

void BrwWait(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    RetBool(pRetData, EvWait(ArgInt(pArgInf, 0, 1000)));
}

void BrwPending(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    RetInt(pRetData, EvPending());
}

void BrwExtText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf, INT nWhich)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    EnterCriticalSection(&g_cs);
    const char* psz = (nWhich == 0) ? g_stat.szExtVer : (nWhich == 1) ? g_stat.szExtBrowser : g_stat.szExtId;
    char szCopy[256];
    StrCpyN(szCopy, sizeof(szCopy), psz);
    LeaveCriticalSection(&g_cs);
    RetText(pRetData, szCopy);
}

void BrwCounter(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf, INT nWhich)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    RetInt(pRetData, (nWhich == 0) ? g_stat.nTask : (nWhich == 1) ? g_stat.nConn : g_stat.nReq);
}

void BrwLogText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    BrLog(Zh("[程序] %s"), ToUtf8(ArgText(pArgInf, 0)));
    RetBool(pRetData, 1);
}

void BrwLogPath(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    char szPath[1400];
    LogPathBytes(szPath, sizeof(szPath));
    RetText(pRetData, szPath);
}

// 1.1.0 追加：回调只收整数句柄（易语言不允许把带自定义类型形参的子程序取地址），
// 数据用 还原事件信息 写进调用者自己的结构体，结果用 应答采集事件 回写。

// 易语言传参表的真实布局（每个参数占几个字、数据指针与数据类型码各在第几个字）只能拿真机的原始
// 数据来定：猜错一次就要多来回一轮。失败时把参数表前 8 个字按十六进制原样附在报错后面，日志同步留一份；
// 再标出「第几个字指向一块可写的 84 字节内存、那块的首字是什么」—— 只读不写，避免照猜测踩坏他的内存。
static void BrwRestoreProbe(INT nArgCount, PMDATA_INF pArgInf)
{
    char szRaw[96];
    char szBlocks[96];
    szRaw[0] = '\0';
    szBlocks[0] = '\0';
    const DWORD* pd = (const DWORD*)(const void*)pArgInf;
    INT nFound = 0;
    __try
    {
        CxSnprintf(szRaw, sizeof(szRaw), "%x %x %x %x %x %x %x %x",
            pd[0], pd[1], pd[2], pd[3], pd[4], pd[5], pd[6], pd[7]);
        for (INT i = 0; i < 8 && nFound < 4; ++i)
        {
            if (pd[i] == 0) continue;
            if (!RegionCheck((const void*)(INT_PTR)pd[i], (INT)(4 * BM_COUNT))) continue;
            INT nFirst = *(const INT*)(INT_PTR)pd[i];
            INT nLen = (INT)strlen(szBlocks);
            CxSnprintf(szBlocks + nLen, sizeof(szBlocks) - nLen, nLen ? " %d:%d" : "%d:%d", i, nFirst);
            ++nFound;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        StrCpyN(szRaw, sizeof(szRaw), "读不到");
    }
    // 取证字段用 ASCII 键名：宿主源码里的中文是 GBK，拿去匹配这段 UTF-8 文本永远对不上。
    // szof 是本库里 MDATA_INF 的步长：SDK 头写的是 1 字节对齐（12），要是编出来是 16，
    // 那第 2 个参数起就整体读高了 4 个字 —— 现象正好是「数据指针读到 0、类型码读到垃圾」。
    BrAppendError("nArg=%d szof=%d tbl=%x raw=[%s] blk=[%s]",
        nArgCount, (INT)sizeof(MDATA_INF), (INT)(INT_PTR)pd, szRaw, szBlocks);
    BrLog("%s", g_stat.szErr);
}

void BrwRestore(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    // 「参考」实参给的是变量的块地址；易语言对没赋过值的变长复合类型变量传空白数据，
    // 具体是二次解栈(A)还是一次解栈(B)由 EvRestore 两种形态都试，取不到才附取证数据
    const BOOL bOk = EvRestore(ArgInt(pArgInf, 0, 0), ArgAt(pArgInf, 1)->m_pCompoundData,
                               (INT)ArgAt(pArgInf, 1)->m_dtDataType);
    if (!bOk) BrwRestoreProbe(nArgCount, pArgInf);
    RetBool(pRetData, bOk);
}

void BrwAnswer(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    const BOOL bOk = EvAnswer(ArgInt(pArgInf, 0, 0), ArgBool(pArgInf, 1, 1),
        ArgText(pArgInf, 2), ArgText(pArgInf, 3),
        ArgBool(pArgInf, 4, 0), ArgInt(pArgInf, 5, 0), ArgBool(pArgInf, 6, 0));
    RetBool(pRetData, bOk);
}

void BrwSetCbMode(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    const INT n = ArgInt(pArgInf, 0, 0);
    if (n != 0 && n != 1) { RetBool(pRetData, 0); return; }
    EnterCriticalSection(&g_cs);
    g_cfg.nCallbackMode = n;
    LeaveCriticalSection(&g_cs);
    BrLog(Zh("%s：%s"), Zh("回调派发模式"), n == 0 ? Zh("库内线程自动派发") : Zh("由程序调泵"));
    RetBool(pRetData, 1);
}

void BrwGetCbMode(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    RetInt(pRetData, g_cfg.nCallbackMode);
}

// 自检用：造一条与真实 task 同形的事件入队，返回事件序号（0 = 网址不被识别或入队失败）。
// 注意序号要在入队前先抄出来：不等应答的事件由派发者回收，入队后随时可能被释放。
void BrwSimTask(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    BrEv* pEv = EvMakeTask(ArgText(pArgInf, 0), ArgBool(pArgInf, 1, 1), Zh("自检事件"), "", "",
        nArgCount > 2 ? ArgText(pArgInf, 2) : "");
    if (pEv == NULL) { RetInt(pRetData, 0); return; }
    const INT nSeq = pEv->nSeq;
    EnterCriticalSection(&g_cs);
    ++g_stat.nTask;
    LeaveCriticalSection(&g_cs);
    BrLog(Zh("%s：%s"), Zh("模拟插件事件"), ToUtf8(ArgText(pArgInf, 0)));
    RetInt(pRetData, EvPost(pEv, 0) ? nSeq : 0);
}

// 当前有几台浏览器连着：一台浏览器一条记录，Chrome 和 Edge 各算一台。
void BrwBrowserCount(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    RetInt(pRetData, SessOnline());
}

// 在线浏览器清单，每行一台：浏览器名|插件版本|扩展ID|已在线秒数；没一台在线时返回空串。
void BrwBrowserList(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    char szList[4096];
    SessListText(szList, sizeof(szList));
    RetText(pRetData, szList);
}

// 下面这些薄桩必须在命令表之前定型：静态编译（__E_STATIC_LIB）不编表，但照样要这些实现符号。
void BrwExtVer(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf) { BrwExtText(pRetData, nArgCount, pArgInf, 0); }
void BrwExtBrowser(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf) { BrwExtText(pRetData, nArgCount, pArgInf, 1); }
void BrwExtId(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf) { BrwExtText(pRetData, nArgCount, pArgInf, 2); }
void BrwTaskCount(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf) { BrwCounter(pRetData, nArgCount, pArgInf, 0); }
void BrwConnCount(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf) { BrwCounter(pRetData, nArgCount, pArgInf, 1); }
void BrwReqCount(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf) { BrwCounter(pRetData, nArgCount, pArgInf, 2); }

// 静态编译只要命令实现：表与库信息用 __E_STATIC_LIB 排除。
#ifndef __E_STATIC_LIB

// ===================== 参数表 =====================
// 可空参数只置 AS_DEFAULT_VALUE_IS_EMPTY（栏位可以留空），调用时仍要把逗号补满。

#define ARG_T(nm, expl)        { _T(nm), _T(expl), 0, 0, SDT_TEXT, 0, 0 },
#define ARG_TE(nm, expl)       { _T(nm), _T(expl), 0, 0, SDT_TEXT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
#define ARG_I(nm, expl)        { _T(nm), _T(expl), 0, 0, SDT_INT, 0, 0 },
#define ARG_IE(nm, expl)       { _T(nm), _T(expl), 0, 0, SDT_INT, 0, AS_DEFAULT_VALUE_IS_EMPTY },
#define ARG_B(nm, expl)        { _T(nm), _T(expl), 0, 0, SDT_BOOL, 0, 0 },
#define ARG_BE(nm, expl)       { _T(nm), _T(expl), 0, 0, SDT_BOOL, 0, AS_DEFAULT_VALUE_IS_EMPTY },

static ARG_INFO g_argNone[] = { { NULL, NULL, 0, 0, _SDT_NULL, 0, 0 } };

static ARG_INFO g_argInit[] = {
    ARG_T("软件版本", "写进握手信息的版本号，采集程序自己的版本，如 20260022；插件设置页与对接日志都会带上它。")
    ARG_T("运行目录", "一般是 取运行目录()；对接日志写在它下面的 浏览器插件对接.log，留空则不写日志。")
    ARG_TE("程序名称", "显示给插件侧的程序名，留空按 亚马逊ASIN采集软件。")
    ARG_TE("服务标识", "本机对接的身份串，插件按它认领服务，留空按 cxac；同一台机器上多个采集程序必须各不相同。")
    ARG_IE("起始端口", "监听端口段的第一位，留空按 47630；端口段要与插件那边的候选端口一致。")
    ARG_IE("端口数量", "端口段长度，留空按 4，即 47630 到 47633；被占用的端口自动顺延。")
};
static ARG_INFO g_argSites[] = {
    ARG_TE("站点名单", "每行一条：平台名称|站点显示名|域名|国家|货币|语言，# 开头的行忽略。留空恢复库内置的 21 个亚马逊站点；任务国家代码仅接受 US、UK、CA、JP、DE、FR、IT、ES、MX、IN、AU、AE、SG、SA、TR、PL、SE、NL、BR、EG、BE。")
};
static ARG_INFO g_argMatch[] = {
    ARG_T("网址", "要识别的页面地址，返回识别结果的 JSON 文本，用来核对内置规则认不认这个页面。")
};
static ARG_INFO g_argOffline[] = {
    ARG_I("超时秒", "连续这么多秒没有收到插件任何请求就判定断开，并派发一条断开事件；一般 60 到 300。")
};
static ARG_INFO g_argReply[] = {
    ARG_I("等待毫秒", "采集程序最多用这么多毫秒处理一条采集请求；超时插件那边收到「没有及时应答」。一般 5000 到 30000。")
};
static ARG_INFO g_argBind[] = {
    { _T("接收子程序"), _T("形参只有一个整数型「事件句柄」的子程序，用 &子程序名 传入；子程序里用 还原事件信息 取数据、用 应答采集事件 回结果。传 0 解除绑定。"), 0, 0, SDT_SUB_PTR, 0, 0 },
    ARG_TE("子程序名", "建议把你写进去的那个名字原样再传一遍（如 “接收采集事件”）。库内日志与「派发异常」报错会直接点名是哪个子程序，客户环境排障时不用猜；留空只是少了这个名字。")
};
static ARG_INFO g_argConcurrent[] = {
    ARG_B("是否允许并发派发", "假（默认，必须保持）＝ 库内一次只跑一条事件，你的接收子程序不会被两个线程同时进入，全局变量与界面都不会被踩；"
        "真＝放开并发，采集请求多时吞吐更高，但接收子程序必须自己可重入（自己加锁、不碰界面、不用全局变量），事件之间也不再保证先后顺序。")
};
static ARG_INFO g_argPump[] = {
    ARG_IE("最多条数", "一次最多处理这么多条消息，留空表示全部（上限 64 条）。")
};
static ARG_INFO g_argWait[] = {
    ARG_IE("超时毫秒", "最多等这么久，留空按 1000 毫秒；适合放在采集程序自己的子线程里循环取消息。")
};
static ARG_INFO g_argMode[] = {
    ARG_I("模式", "0 表示库在自己的线程里直接回调你的子程序（默认，程序不用调泵）；1 表示由你的程序调 处理插件消息 或 等待插件消息 来派发。")
};
static ARG_INFO g_argSim[] = {
    ARG_T("网址", "要模拟的页面地址，按库内置规则识别列表页/详情页；认不出来的地址返回 0。")
    ARG_BE("是否开始采集", "留空按真，对应插件上点的是「发送到采集」。")
    ARG_TE("任务名", "留空则库内按「站点名 + ASIN」自动补一个；填了就用它，和插件提交任务时带的 taskName 是同一个字段。")
};
static ARG_INFO g_argLog[] = {
    ARG_T("日志内容", "追加到对接日志里的一行，前面自动带时间，用于把程序里的处理结果和对接事件对起来看。")
};
static ARG_INFO g_argRestore[] = {
    ARG_I("事件句柄", "接收子程序收到的那个整数句柄；只在本次回调期间有效，回调返回后就作废。")
    { _T("事件"), _T("用 浏览器事件信息 类型的变量接收，本命令把这条事件的字段逐项填进去；这个变量最好在此之前赋过一次值（如 事件.事件类型 = 99），让易语言自己分配内存；没赋过时库会代管一块，同样能取，但离开本子程序就作废。填进去的文本由库持有，子程序返回后这个变量就不要再用了。"), 0, 0, MAKELONG(CX_TYPE_BROW_EVENT, 0), 0, AS_RECEIVE_VAR },
};
static ARG_INFO g_argAnswer[] = {
    ARG_I("事件句柄", "接收子程序收到的那个整数句柄。")
    ARG_B("是否接受", "假表示拒绝这条任务，插件会弹提示并把「提示消息」显示给客户。")
    ARG_TE("任务编号", "自己任务表的主键，插件顶栏显示 任务 #这个值；留空按本条序号回。")
    ARG_TE("提示消息", "回给插件的说明文字，如「该 ASIN 已在任务列表」。")
    ARG_BE("已在队列", "这个网址已经在任务列表里时置真，插件按「已在采集」提示而不是重复提交。")
    ARG_IE("队列数量", "自己采集队列里的任务数，插件的提示里会带上；留空按 0。")
    ARG_BE("是否已开始", "采集线程真的取走这条任务后置真；不置则插件提示「已入队未开始」。")
};

// ===================== 命令表 =====================
// 命令只能往数组末尾追加：插入、删除、改名、改参数顺序都会让老程序按序号调用时错乱。

#define CX_CMD(cn, eg, explain, cat, ret, argc, args) \
    { _T(cn), _T(eg), _T(explain), (cat), _CMD_OS(__OS_WIN), (ret), 0, LVL_SIMPLE, 0, 0, (argc), (args) },

CMD_INFO g_commands[] = {
    CX_CMD("初始化对接", "BrwInit",
        "写入软件版本、运行目录等必要信息；必须在 启动对接服务 之前调用，参数留空的项走库内默认值。",
        1, SDT_BOOL, 6, g_argInit)
    CX_CMD("启动对接服务", "BrwStart",
        "绑定本机端口并启动监听线程，之后插件随时可以连进来；端口都被占用时后台会每 5 秒重试，不用程序自己管。",
        1, SDT_BOOL, 0, NULL)
    CX_CMD("停止对接服务", "BrwStop",
        "停止监听并清空未处理的消息；退出程序前调用，同时解除接收子程序的绑定。",
        1, _SDT_NULL, 0, NULL)
    CX_CMD("服务是否运行", "BrwIsRunning",
        "本机端口是否在监听；真表示已在监听，不代表插件已经连上。",
        1, SDT_BOOL, 0, NULL)
    CX_CMD("取监听端口", "BrwGetPort",
        "实际占用的端口号，没监听时返回 0。",
        1, SDT_INT, 0, NULL)
    CX_CMD("取连接状态", "BrwGetState",
        "0 服务未启动，1 已监听但一台浏览器都还没连上，2 至少一台浏览器已连上；要具体几台用 取已连接浏览器数。取 #连接状态 系列常量比对。",
        1, SDT_INT, 0, NULL)
    CX_CMD("取对接错误", "BrwGetError",
        "最近一次启动或识别失败的原因文本，正常时为空串。",
        1, SDT_TEXT, 0, NULL)
    CX_CMD("置站点名单", "BrwSetSites",
        "用程序自己的站点名单覆盖库内置的亚马逊站点，插件顶部的按钮按这份名单出现；返回实际载入的站点数。",
        1, SDT_INT, 1, g_argSites)
    CX_CMD("取站点名单", "BrwGetSites",
        "当前生效的站点名单，每行一条 平台名称|站点显示名|域名|国家|货币|语言。",
        1, SDT_TEXT, 0, NULL)
    CX_CMD("取网址识别", "BrwMatch",
        "按内置规则识别一个网址，返回 JSON 文本；用来核对列表页、详情页能不能被认出来。",
        1, SDT_TEXT, 1, g_argMatch)
    CX_CMD("置连接超时秒", "BrwSetOffline",
        "一台浏览器连续这么多秒没发任何请求就把这一台判断开（每台各自计时，别的浏览器不受影响）；默认 120 秒，改小了断开事件会更灵敏。",
        1, SDT_BOOL, 1, g_argOffline)
    CX_CMD("置应答等待毫秒", "BrwSetReplyWait",
        "一条采集请求最多等采集程序处理多久，默认 15000 毫秒；插件那边的等待时间比它长。",
        1, SDT_BOOL, 1, g_argReply)
    CX_CMD("绑定事件子程序", "BrwBindSub",
        "登记接收数据的子程序：事件到达后由库内线程自动派发（默认），或在 置回调线程 (1) 后由 处理插件消息 / 等待插件消息 在调用它的线程里回调。"
        "子程序形参是一个整数型事件句柄，再用 还原事件信息 取数据、应答采集事件 回结果。"
        "第二个参数把你写的子程序名原样传一遍，日志和报错就能直接点名是哪个子程序出的问题（可省略）。",
        2, SDT_BOOL, 2, g_argBind)
    CX_CMD("处理插件消息", "BrwPump",
        "取走已到达的消息并回调绑定的子程序，返回实际处理条数；放在主窗口的时钟里每 200 毫秒调一次即可，界面操作都在这个线程里做，安全。",
        2, SDT_INT, 1, g_argPump)
    CX_CMD("等待插件消息", "BrwWait",
        "没有消息时先等一会儿再来取，返回真表示处理了一条；适合放在程序自己的子线程里循环，此时回调发生在该线程，别直接动窗口组件。",
        2, SDT_BOOL, 1, g_argWait)
    CX_CMD("取待处理消息数", "BrwPending",
        "队列里还有多少条消息没送到子程序，处理不及时会堆在这里。",
        2, SDT_INT, 0, NULL)
    CX_CMD("取插件版本", "BrwExtVer",
        "浏览器插件自己上报的版本号，插件每次请求都带，便于按客户环境排障。多台浏览器同时在线时，这三条「取插件…」给的是最后一次上报的那台，逐台要看 取已连接浏览器清单，某条事件是谁发的看事件里的 插件编号 与 浏览器名称。",
        2, SDT_TEXT, 0, NULL)
    CX_CMD("取插件浏览器", "BrwExtBrowser",
        "插件所在浏览器（Chrome / Edge）与其版本。",
        2, SDT_TEXT, 0, NULL)
    CX_CMD("取插件编号", "BrwExtId",
        "插件在浏览器里的扩展 ID，客服报障时用它能确认装的是哪一个。",
        2, SDT_TEXT, 0, NULL)
    CX_CMD("取累计采集请求数", "BrwTaskCount",
        "本次运行里插件一共发来多少条采集请求。",
        2, SDT_INT, 0, NULL)
    CX_CMD("取累计连接次数", "BrwConnCount",
        "插件连上本程序多少次，重连是否频繁看这个。",
        2, SDT_INT, 0, NULL)
    CX_CMD("取累计请求数", "BrwReqCount",
        "本机收到的对接请求总数，含握手、站点、识别与采集。",
        2, SDT_INT, 0, NULL)
    CX_CMD("写对接日志", "BrwLogText",
        "把程序里的一句话写进同一份对接日志，方便和插件事件对照；运行目录没设置时什么都不做。",
        3, SDT_BOOL, 1, g_argLog)
    CX_CMD("取对接日志路径", "BrwLogPath",
        "对接日志的完整路径，直接发给客户取证用；没设置运行目录时返回空串。",
        3, SDT_TEXT, 0, NULL)
    CX_CMD("还原事件信息", "BrwRestore",
        "在接收子程序里把事件句柄还原成 浏览器事件信息 结构体，逐字段取用；句柄只在本次回调期间有效。"
        "调用前先给这个变量赋一次值（如 事件.事件类型 = 99），让易语言自己分配好内存，本命令直接往那块里写。",
        2, SDT_BOOL, 2, g_argRestore)
    CX_CMD("应答采集事件", "BrwAnswer",
        "在接收子程序里把处理结果回给插件：是否接受、任务编号、提示消息、是否已在队列、队列数量、是否已开始采集。",
        2, SDT_BOOL, 7, g_argAnswer)
    CX_CMD("模拟插件事件", "BrwSimTask",
        "不装浏览器也能验「调泵 → 回调 → 应答」这条链：造一条与真实采集请求同形的事件入队，返回事件序号（0 表示网址认不出）；"
        "紧接着调 处理插件消息 就会走你绑定的子程序。第三个参数可以指定任务名（留空则库内自动按站点+ASIN 补）。",
        3, SDT_INT, 3, g_argSim)
    CX_CMD("置回调线程", "BrwSetCbMode",
        "选回调跑在哪个线程：0 库内自动派发（默认，不用调泵，但子程序不在界面线程，别直接操作窗口组件）；"
        "1 由程序调 处理插件消息 派发（放在主窗口时钟里，子程序里可以直接动界面）。",
        2, SDT_BOOL, 1, g_argMode)
    CX_CMD("取回调模式", "BrwGetCbMode",
        "当前派发模式：0 自动派发，1 手动调泵。",
        2, SDT_INT, 0, NULL)
    CX_CMD("置事件并发", "BrwSetConcurrent",
        "要不要允许两条事件同时进你的接收子程序。默认是「假」：库内一次只派发一条，前一条子程序返回后才派发下一条，"
        "所以你的全局变量、界面控件、采集队列不会被两个线程同时改，事件也按到达顺序处理 —— 这就是事件原子性。"
        "只有当你确定接收子程序完全可重入（自己加锁、不碰界面、不依赖调用顺序）时才传「真」换取吞吐；"
        "注意放开后一条慢事件不再挡别的，但串行模式下排队也不会丢事件，一般没必要开。",
        2, SDT_BOOL, 1, g_argConcurrent)
    CX_CMD("取已连接浏览器数", "BrwBrowserCount",
        "当前有几台浏览器连着本程序：同一台机器上 Chrome 和 Edge 各算一台，同一台浏览器的多个窗口只算一台。"
        "没连上返回 0；这个数字也是每条事件里「已连接浏览器数量」的来源。",
        1, SDT_INT, 0, NULL)
    CX_CMD("取已连接浏览器清单", "BrwBrowserList",
        "逐台列出当前在线的浏览器，每行一条：浏览器名|插件版本|扩展ID|已在线秒数，没有在线浏览器时返回空串。"
        "客户装了多个浏览器时用它确认到底哪台连着，比取插件编号（只给最后一次上报的那台）准。",
        2, SDT_TEXT, 0, NULL)
};

#undef CX_CMD

static PFN_EXECUTE_CMD g_commandFuncs[] = {
    BrwInit, BrwStart, BrwStop, BrwIsRunning, BrwGetPort, BrwGetState, BrwGetError,
    BrwSetSites, BrwGetSites, BrwMatch, BrwSetOffline, BrwSetReplyWait,
    BrwBindSub, BrwPump, BrwWait, BrwPending, BrwExtVer, BrwExtBrowser, BrwExtId,
    BrwTaskCount, BrwConnCount, BrwReqCount, BrwLogText, BrwLogPath,
    BrwRestore, BrwAnswer, BrwSimTask, BrwSetCbMode, BrwGetCbMode, BrwSetConcurrent,
    BrwBrowserCount, BrwBrowserList
};

// 静态编译按名字找实现：顺序须与 g_commands 一一对应，名字一经发布不可更改。
const char* g_commandFuncNames[] = {
    "BrwInit", "BrwStart", "BrwStop", "BrwIsRunning", "BrwGetPort", "BrwGetState", "BrwGetError",
    "BrwSetSites", "BrwGetSites", "BrwMatch", "BrwSetOffline", "BrwSetReplyWait",
    "BrwBindSub", "BrwPump", "BrwWait", "BrwPending", "BrwExtVer", "BrwExtBrowser", "BrwExtId",
    "BrwTaskCount", "BrwConnCount", "BrwReqCount", "BrwLogText", "BrwLogPath",
    "BrwRestore", "BrwAnswer", "BrwSimTask", "BrwSetCbMode", "BrwGetCbMode", "BrwSetConcurrent",
    "BrwBrowserCount", "BrwBrowserList"
};

#define BRW_CMD_COUNT 32
static_assert(sizeof(g_commands) / sizeof(g_commands[0]) == BRW_CMD_COUNT, "命令表条数与断言不符");
static_assert(sizeof(g_commandFuncs) / sizeof(g_commandFuncs[0]) == BRW_CMD_COUNT, "实现表与命令表不同长");
static_assert(sizeof(g_commandFuncNames) / sizeof(g_commandFuncNames[0]) == BRW_CMD_COUNT, "名字表与命令表不同长");

// 每个类别单独一条字面量再串起来：写成 "0001对接服务\0002数据接收" 会被 C 当成八进制转义
// \000 吞掉两个 0，类别名整体错位，IDE 里就显示成乱码。
const TCHAR g_categories[] =
    _T("0001对接服务\0")
    _T("0002数据接收\0")
    _T("0003调试维护\0")
    _T("\0");

// ===================== 自定义类型：浏览器事件信息 =====================
// 成员顺序必须与 bridge.h 的 BrEvMember 逐项对应：易语言按声明顺序取块，
// 文本成员存的是指针（4 字节），整数与逻辑成员各占 4 字节。

#define ARGV(nm, expl) { _T(nm), _T(expl), 0, 0, SDT_TEXT, 0, 0 },
static LIB_DATA_TYPE_ELEMENT g_eventElements[] = {
    { SDT_INT,  NULL, _T("事件类型"),       NULL, _T("1 收到采集请求，2 一台浏览器已连接，3 一台浏览器已断开，4 插件撤回任务，5 所有浏览器都已断开；取 #事件类型 常量比对。"), 0, 0 },
    { SDT_INT,  NULL, _T("序号"),           NULL, _T("这条消息的流水号，本次运行内递增，插件那边回的任务编号默认用它。"), 0, 0 },
    { SDT_TEXT, NULL, _T("到达时间"),       NULL, _T("这条消息到达的时刻，形如 2026-10-21 13:20:31。"), 0, 0 },
    { SDT_TEXT, NULL, _T("网址"),           NULL, _T("插件所在页面的完整地址，直接拿去采集。"), 0, 0 },
    { SDT_TEXT, NULL, _T("标题"),           NULL, _T("插件带过来的页面标题，插件没带则为空串。"), 0, 0 },
    { SDT_INT,  NULL, _T("页面类型"),       NULL, _T("1 列表页，2 店铺页，3 详情页，0 未识别；取 #页面类型 常量。"), 0, 0 },
    { SDT_INT,  NULL, _T("采集模式"),       NULL, _T("1 按列表采集，2 按详情采集；取 #采集模式 常量。"), 0, 0 },
    { SDT_TEXT, NULL, _T("平台名称"),       NULL, _T("站点程序名，如 AmazonUs，和自己任务表里的平台字段对上。"), 0, 0 },
    { SDT_TEXT, NULL, _T("站点名称"),       NULL, _T("站点显示名，如 美国站。"), 0, 0 },
    { SDT_TEXT, NULL, _T("国家"),           NULL, _T("两位国家码，如 US、JP。"), 0, 0 },
    { SDT_TEXT, NULL, _T("货币"),           NULL, _T("站点货币，如 USD。"), 0, 0 },
    { SDT_TEXT, NULL, _T("语言"),           NULL, _T("站点语言，如 en。"), 0, 0 },
    { SDT_TEXT, NULL, _T("ASIN"),           NULL, _T("详情页的商品 ASIN；列表页为空串。"), 0, 0 },
    { SDT_BOOL, NULL, _T("是否开始采集"),   NULL, _T("插件上点的是「发送到采集」还是「只添加任务」；假表示只要入库不要立刻跑。"), 0, 0 },
    { SDT_TEXT, NULL, _T("插件版本"),       NULL, _T("浏览器插件自己的版本号，便于按客户环境维护。"), 0, 0 },
    { SDT_TEXT, NULL, _T("浏览器名称"),     NULL, _T("插件所在的浏览器与版本，如 Chrome 139。"), 0, 0 },
    { SDT_TEXT, NULL, _T("插件编号"),       NULL, _T("扩展 ID，客服报障时用这一串确认装的是哪个插件。"), 0, 0 },
    { SDT_TEXT, NULL, _T("请求内容"),       NULL, _T("插件发来的原始 JSON 文本，将来带页面数据时从这里自己解；断开事件里放的是断开原因。"), 0, 0 },
    { SDT_TEXT, NULL, _T("任务名"),         NULL, _T("插件提交任务时带过来的任务名（ac-client 里 addTask 的 info.taskName，也认 info.name）；"
        "插件没带则库内按「站点名 + ASIN」补一个，列表页补页面类型，所以这一项不会是空串。直接拿去填自己任务表的标题列。"), 0, 0 },
    { SDT_TEXT, NULL, _T("任务编号"),       NULL, _T("撤回事件（事件类型=4）里要撤的那个任务号，就是当初 应答采集事件 回给插件、"
        "插件卡片上显示的那个号；其余事件里为空串。撤回事件里库还会按这个号把当初那一页的网址、站点、页面类型、采集模式、ASIN、任务名一并回填好（程序重启后查不到登记时就只剩号，那时按号查你自己的任务表）。"), 0, 0 },
    { SDT_INT,  NULL, _T("已连接浏览器数量"), NULL, _T("这条事件被派发给你的那一刻，本机还有几台浏览器连着本程序（Chrome 和 Edge 各算一台，同一台浏览器多窗口只算一台）。"
        "事件类型=2 时它含刚连上的这台；=3 时是不含刚断开那台的剩余台数；=5 时恒为 0；采集与撤回事件里它用来判断「是不是只有这一台在用」。"), 0, 0 },
};
#undef ARGV

LIB_DATA_TYPE_INFO g_dataTypes[] = {
    { _T("浏览器事件信息"), "BrowserEvent",
      _T("插件传来的一条对接数据：网址、站点、页面类型、插件版本等。用 还原事件信息 从事件句柄填进本类型的变量，"
         "只在接收子程序执行期间有效，要留下的值自己逐字段复制；回给插件的结果用 应答采集事件 写，不在本类型里。"),
      0, NULL, 0, 0, 0, NULL, 0, NULL, NULL,
      sizeof(g_eventElements) / sizeof(g_eventElements[0]), g_eventElements },
};

// ===================== 库常量 =====================

#define LIBC(nm, val) { _T(nm), _T(#nm), NULL, 0, CT_NUM, NULL, (DOUBLE)(val) },
static LIB_CONST_INFO g_consts[] = {
    LIBC("事件类型_收到采集请求", 1)
    LIBC("事件类型_插件已连接", 2)
    LIBC("事件类型_插件已断开", 3)
    LIBC("事件类型_撤回任务", 4)
    LIBC("事件类型_全部浏览器断开", 5)
    LIBC("页面类型_未识别", 0)
    LIBC("页面类型_列表页", 1)
    LIBC("页面类型_店铺页", 2)
    LIBC("页面类型_详情页", 3)
    LIBC("采集模式_未识别", 0)
    LIBC("采集模式_列表", 1)
    LIBC("采集模式_详情", 2)
    LIBC("连接状态_未启动", 0)
    LIBC("连接状态_等待插件", 1)
    LIBC("连接状态_插件已连接", 2)
};
#undef LIBC

#endif  // !__E_STATIC_LIB

// ===================== 库通知 =====================

INT WINAPI cxbrow_ProcessNotifyLib(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    UNREFERENCED_PARAMETER(dwParam2);

    switch (nMsg)
    {
    case NL_SYS_NOTIFY_FUNCTION:
        // 装载支持库前通知，可能来多次，后值覆盖前值。
        g_pfnNotifySys = (PFN_NOTIFY_SYS)dwParam1;
        return NR_OK;

#ifndef __E_STATIC_LIB
    case NL_GET_CMD_FUNC_NAMES:
        // 静态编译必须处理：返回 char*[] 函数名数组。
        return (INT)g_commandFuncNames;

    case NL_GET_NOTIFY_LIB_FUNC_NAME:
        // 静态编译必须处理：返回本通知函数的名字。
        return (INT)LIB_NOTIFY_FUNC_NAME;

    case NL_GET_DEPENDENT_LIBS:
        // 本库不依赖外部 DLL：winsock 入口在运行期从 ws2_32.dll 取。
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

// ===================== 库信息 =====================
#ifndef __E_STATIC_LIB
// 字段顺序固定，见 lib2.h 的 LIB_INFO。

LIB_INFO g_libraryInfo = {
    LIB_FORMAT_VER,
    _T("46F4540A8C8D04F7E6296993C0119653"),
    1, 8, 2,                       // 主/次/构建版本号
    3, 0,                          // 要求的易语言系统版本：只可 ≤ 目标机实际版本，填高了 IDE 会拒载（勾选不保持）
    3, 0,                          // 要求的系统核心支持库版本（同上；本机 krnln 为 5.7，别要求 5.9）
    _T("创想浏览器插件对接"),
    __GBK_LANG_VER,
    _T("与创想浏览器插件对接：本机监听、随时连接、断线自动重试，并把插件传来的采集数据转成易语言结构交给绑定的子程序。"),
    _LIB_OS(__OS_WIN),             // 纯命令库不要加 LBS_IDE_PLUGIN
    _T("创想软件"),
    _T(""), _T(""), _T(""), _T(""), _T(""), _T(""), _T(""),
    sizeof(g_dataTypes) / sizeof(g_dataTypes[0]), g_dataTypes,
    3, const_cast<TCHAR*>(g_categories),
    sizeof(g_commands) / sizeof(g_commands[0]),
    g_commands,
    g_commandFuncs,
    NULL, NULL,                    // AddIn
    cxbrow_ProcessNotifyLib,
    NULL, NULL,                    // 超级模板
    sizeof(g_consts) / sizeof(g_consts[0]), g_consts,
    NULL,                          // 依赖文件
};

#endif  // !__E_STATIC_LIB

}  // namespace cx

// ==== 以下由 gen_static_trampolines.mjs 生成，勿手改 ====
// 静态编译时易语言按名字到 cxbrow_static.lib 里找命令实现，
// 而实现在 namespace cx 内，故每个命令生成一份 extern "C" 全局转发。
#define CX_FWD(fn) extern "C" void fn (PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf) { cx::fn (pRetData, nArgCount, pArgInf); }

CX_FWD(BrwInit)
CX_FWD(BrwStart)
CX_FWD(BrwStop)
CX_FWD(BrwIsRunning)
CX_FWD(BrwGetPort)
CX_FWD(BrwGetState)
CX_FWD(BrwGetError)
CX_FWD(BrwSetSites)
CX_FWD(BrwGetSites)
CX_FWD(BrwMatch)
CX_FWD(BrwSetOffline)
CX_FWD(BrwSetReplyWait)
CX_FWD(BrwBindSub)
CX_FWD(BrwPump)
CX_FWD(BrwWait)
CX_FWD(BrwPending)
CX_FWD(BrwExtVer)
CX_FWD(BrwExtBrowser)
CX_FWD(BrwExtId)
CX_FWD(BrwTaskCount)
CX_FWD(BrwConnCount)
CX_FWD(BrwReqCount)
CX_FWD(BrwLogText)
CX_FWD(BrwLogPath)
CX_FWD(BrwRestore)
CX_FWD(BrwAnswer)
CX_FWD(BrwSimTask)
CX_FWD(BrwSetCbMode)
CX_FWD(BrwGetCbMode)
CX_FWD(BrwSetConcurrent)
CX_FWD(BrwBrowserCount)
CX_FWD(BrwBrowserList)

#undef CX_FWD

extern "C" INT WINAPI cxbrow_ProcessNotifyLib (INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    return cx::cxbrow_ProcessNotifyLib (nMsg, dwParam1, dwParam2);
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
