// 创想浏览器插件对接 —— 库内部接口声明。
// 编码口径：进出易语言的文本一律 UTF-8 原样透传，库内不做 GBK 转换（只有日志与文件路径按 UTF-16 走系统调用）。
// 静态编译约束：不用 STL、不做 double 与 64 位整数的互换、系统 DLL 入口一律运行期解析并写成 WINAPI。

#ifndef CXBROW_BRIDGE_H
#define CXBROW_BRIDGE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;

#include <lib2.h>
#include <lang.h>

namespace cx {

// 自带格式化：新 CRT 的 _snprintf/_vsnprintf 会引用 ___stdio_common_vsprintf，
// 易语言静态编译用的 VC6 LIBC 里没有这个符号，链接期才炸；这里只支持用得到的 %s %d %u %x 与补零宽度。
INT  CxVSnprintf(char* dst, INT cchDst, const char* pszFmt, va_list ap);
INT  CxSnprintf(char* dst, INT cchDst, const char* pszFmt, ...);

// ===================== 动态文本缓冲 =====================

struct Buf {
    char*  p;
    INT    nLen;
    INT    nCap;
};

void BufInit(Buf* b);
void BufFree(Buf* b);
void BufReset(Buf* b);
void BufReserve(Buf* b, INT nNeed);
void BufAdd(Buf* b, const char* psz, INT nLen);
// 按实际长度追加字面量：手写长度算错一次就毁掉整份 HTTP 报文，一律走这里。
void BufAddSz(Buf* b, const char* psz);
void BufAddCh(Buf* b, char c);
void BufAddF(Buf* b, const char* pszFmt, ...);
// JSON 字符串值转义后追加（引号、反斜杠、控制字符、非 ASCII 原样保留 UTF-8 字节）
void BufAddJsonStr(Buf* b, const char* psz);
// 追加一个键：bComma 为真时先补逗号。值请紧跟 BufAddJsonStr / BufAddF 写。
void JsonKey(Buf* b, const char* pszKey, BOOL bComma);
void JsonStr(Buf* b, const char* pszKey, const char* pszVal, BOOL bComma);
void JsonBool(Buf* b, const char* pszKey, INT nVal, BOOL bComma);
void JsonInt(Buf* b, const char* pszKey, INT nVal, BOOL bComma);

// ===================== 文本工具 =====================

INT  StrLen(const char* psz);
BOOL StrEqI(const char* a, const char* b);
BOOL StrNCmpI(const char* a, const char* b, INT n);
BOOL StrHasI(const char* hay, const char* needle);
const char* StrFindI(const char* hay, const char* needle);
// /execution-charset:gbk 下库内中文窄字面量是 GBK 字节；跨库边界（易语言文本、JSON、日志文件）前统一走这里转 UTF-8。
// 返回轮转静态缓冲，只在本条语句内有效。
const char* Zh(const char* pszGbk);
// 发给浏览器的每一段文本都必须先是合法 UTF-8：采集程序是 GBK 口径时（取运行目录/程序名/站点名/提示消息），
// 原样塞进 JSON 会让插件显示成乱码。合法 UTF-8 原样返回，非法则按 936 转一次。
const char* ToUtf8(const char* psz);
void StrCpyN(char* dst, INT cchDst, const char* src);
void StrTrim(char* psz);
// UTF-8 文本 -> UTF-16（仅文件路径等系统调用用）
LPWSTR Utf8ToWide(const char* pszUtf8);
// 取当前时间 "2026-10-21 13:20:31"
void NowText(char* dst, INT cchDst);
// 极简 JSON 取值：只找顶层成员；返回真表示找到。
BOOL JsonGetString(const char* pszJson, const char* pszKey, char* dst, INT cchDst);
BOOL JsonGetInt(const char* pszJson, const char* pszKey, INT* pOut);
BOOL JsonGetBool(const char* pszJson, const char* pszKey, INT* pOut);
// 取顶层成员原始值文本（数组/对象原样搬出），用于把扩展传来的数据留给易语言侧继续解
BOOL JsonGetRaw(const char* pszJson, const char* pszKey, char* dst, INT cchDst);

// ===================== 站点与网址识别 =====================

struct BrSite {
    char host[64];         // www.amazon.com
    char platform[48];     // AmazonUs
    char display[48];      // 美国站
    char country[16];
    char currency[16];
    char language[16];
    INT  bUtf8;            // 0 表内中文是 GBK 字面量（出口要转），1 已经是 UTF-8
};

// 页面类型与 EC 侧 pageTypes.type 保持同一口径：1 搜索结果 2 店铺 3 详情页
enum BrPageType { PT_UNKNOWN = 0, PT_LIST = 1, PT_SHOP = 2, PT_DETAIL = 3 };
// 采集模式：0 未知 1 列表 2 详情（协议里的 colModType 字符串）
enum BrColMode { CM_UNKNOWN = 0, CM_LIST = 1, CM_DETAIL = 2 };

struct BrMatch {
    BOOL  bOk;
    BOOL  bIdentifiable;
    char  host[64];
    char  platform[48];
    char  display[48];
    char  country[16];
    char  currency[16];
    char  language[16];
    char  asin[16];
    INT   nPageType;
    INT   nColMode;
};

void SiteSetFromText(const char* pszListUtf8);   // 覆盖内置站点名单：每行 平台名|站点名|域名|国家|货币|语言
INT  SiteCount();
void SiteGet(INT nIndex, BrSite* pOut);
void SiteJson(Buf* bOut);                       // 生成 /cxa/sites 的 sites 数组
void MatchUrl(const char* pszUrl, BrMatch* pOut);

// ===================== 配置与运行状态 =====================

struct BrCfg {
    char szVersion[128];
    char szApp[128];
    char szService[64];
    char szRunDir[1024];      // UTF-8
    INT  nPortFirst;
    INT  nPortCount;
    INT  nOfflineSec;
    INT  nReplyWaitMs;
    INT  nCallbackMode;    // 0 库内线程自动派发（默认） 1 由程序调泵
    INT  nConcurrent;      // 0 事件串行派发（默认，保证回调原子性） 1 允许多线程并发派发
    INT  bInited;
    INT  nDirMode;         // 运行目录编码判定：0 未判定 1 UTF-8 2 ANSI(GBK)
};

struct BrStat {
    INT  listening;           // 0 未监听 1 已监听
    INT  port;
    INT  connected;           // 0 插件离线 1 插件在线
    char szKey[40];
    DWORD dwLastReq;          // GetTickCount
    char szExtVer[64];
    char szExtBrowser[96];
    char szExtId[160];
    char szErr[512];
    char szLogErr[320];       // 日志写不进去时的原因（否则整份对接过程无声丢失）
    INT  nTask;               // 累计收到的采集请求
    INT  nConn;               // 累计连接次数
    INT  nReq;                // 累计请求数
    DWORD dwConnectedAt;
    DWORD dwBindRetryAt;      // 下次允许重试绑定的 tick
};

extern BrCfg  g_cfg;
extern BrStat g_stat;
extern CRITICAL_SECTION g_cs;
extern CRITICAL_SECTION g_csDispatch;   // 串行派发用户子程序用的锁（与 g_cs 不同一把，避免回调里再取表锁造成嵌套）

void BrInitOnce();
void CbSetName(const char* pszName);    // 绑定成功后把子程序名交进来，日志与报错里要点名
INT  BrNextSeq();

// 自增/自减：这套 SDK 头里 Interlocked* 宏要的是 volatile long*，直接投 INT* 会重载找不到，
// 32 位下 INT 就是 long，按指针转一下即可。
#define ATOM_INC(p) ((INT)InterlockedIncrement((volatile long*)(void*)(p)))
#define ATOM_DEC(p) ((INT)InterlockedDecrement((volatile long*)(void*)(p)))

void BrLog(const char* pszFmt, ...);
void BrLogOnce(const char* pszKey, const char* pszFmt, ...);
void BrSetError(const char* pszFmt, ...);
void BrAppendError(const char* pszFmt, ...);   // 追在已设的错后面（用来附参数表原样这类取证数据）
void LogPathBytes(char* dst, INT cchDst);   // 按运行目录的编码判定拼出日志路径

// lib.cpp 里定义：NotifySys 通道与易语言内存池
extern PFN_NOTIFY_SYS g_pfnNotifySys;
INT   NotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2);
char* CloneTextData(LPCTSTR pszText);
void* PoolMalloc(INT nBytes);
void  PoolFree(void* pData);

// ===================== 浏览器会话表 =====================
// 一台浏览器一条记录，按「扩展 ID + 浏览器名」认领：Chrome 和 Edge 装同一个打包 CRX 时扩展 ID 相同，
// 只按 ID 分会把两台浏览器并成一台，所以浏览器名必须一起参与。
// g_stat.connected / szExt* 仍然维护（含义改成「至少一台在线」和「最后一次上报的那台」），老程序不受影响。
// 入口全部自己加 g_cs，调用方不要在已持有 g_cs 时进来。

#define BR_MAX_SESSIONS 16

struct BrSess {
    char  szExtId[160];
    char  szBrowser[96];
    char  szExtVer[64];
    DWORD dwFirstAt;      // 这次上线的时刻
    DWORD dwLastReq;      // 最后一次收到它任何请求
    INT   nReq;           // 它累计发了多少请求
};

// 登记并刷新心跳。返回真表示这是一台新上线的浏览器（表里原本没有它）。
// bCreate 为假时只刷新已在清单里的那台：插件点「断开」也走心跳口径，但绝不能因为一条断开请求
// 就把陌生身份登记成上线（那会连带派发「已连接」，还可能把最后在线的那台挤成「全部断开」）。
// pOut 非空时带出这条记录（新建时是新记录，已存在时是刷新后的旧记录）；*pnCount 带出刷新后的在线台数。
BOOL SessTouch(const char* pszExtId, const char* pszBrowser, const char* pszExtVer, BOOL bCreate, BrSess* pOut, INT* pnCount);
INT  SessOnline();                                        // 当前在线台数
// 摘掉一台。返回真表示确实摘掉了一条记录（身份对不上就是假，说明它早被判离线了）。
BOOL SessDrop(const char* pszExtId, const char* pszBrowser, BrSess* pOut, INT* pnCount);
// 取一条超时会话并摘掉它；没有超时的返回假。*pnCount 带出摘掉后剩余台数。
BOOL SessTakeExpired(DWORD dwTimeoutMs, BrSess* pOut, INT* pnCount);
void SessClearAll();                                      // 停止服务：静默清空，不派发事件
// 「Chrome 139 / 扩展ID / 已在线 3 分钟 / 12 次请求」每行一台，UTF-8
void SessListText(char* dst, INT cchDst);

// ===================== 事件队列与子程序回调 =====================

enum BrEventType {
    EV_TASK = 1,       // 扩展点了「发送到采集」
    EV_CONNECT = 2,    // 一台浏览器连上（每台新上线都派发一次）
    EV_DISCONNECT = 3, // 一台浏览器断开（主动断开或超过超时秒无请求）
    EV_RECALL = 4,     // 插件点「撤回」：要把某个任务从采集队列里撤掉
    EV_ALL_DISCONNECT = 5  // 最后一台也掉了：在线台数从 ≥1 变成 0 时派发一次
};

// 自定义类型「浏览器事件信息」的成员序号，必须与 lib.cpp 里的成员表同序。
// 块布局 = 21 个 4 字节槽位按声明顺序排列（整数直接放，文本放 char* 指针）。
// 出参不在这里：易语言不允许把带自定义类型形参的子程序取地址（编译期 10063），
// 所以回调只收一个整数句柄，读用 还原事件信息、写用 应答采集事件。
enum BrEvMember {
    BM_TYPE = 0, BM_SEQ, BM_ARRIVE, BM_URL, BM_TITLE, BM_PAGETYPE, BM_COLMODE,
    BM_PLATFORM, BM_SITE, BM_COUNTRY, BM_CURRENCY, BM_LANGUAGE, BM_ASIN, BM_START,
    BM_EXT_VER, BM_BROWSER, BM_EXT_ID, BM_RAW, BM_TASKNAME, BM_TASKID, BM_BROWSER_COUNT,
    BM_COUNT
};

// 文本成员里由库分配（易语言内存池）的那些
#define BM_OWNED_COUNT 15
static const INT BM_OWNED[BM_OWNED_COUNT] = {
    BM_ARRIVE, BM_URL, BM_TITLE, BM_PLATFORM, BM_SITE, BM_COUNTRY, BM_CURRENCY,
    BM_LANGUAGE, BM_ASIN, BM_EXT_VER, BM_BROWSER, BM_EXT_ID, BM_RAW, BM_TASKNAME, BM_TASKID
};

// 提交时回给插件的任务号 → 这一页的身份。撤回事件只带号：插件隔了页面刷新、隔了半小时来撤，
// 采集程序那边光凭号不知道撤的是列表页还是详情页，所以库把提交时算好的字段抄一份存着，撤回时回填。
struct BrTaskRef {
    char szId[128];
    char szUrl[1024];
    char szAsin[32];
    char szSite[64];
    char szPlatform[64];
    char szCountry[8];
    char szCurrency[8];
    char szLanguage[8];
    char szName[192];
    INT  nPageType;
    INT  nColMode;
    INT  nSeq;
    DWORD dwAt;
};
void TaskRefPut(const BrTaskRef* pRef);                  // 登记（同号覆盖，满了淘汰最老的）
BOOL TaskRefGet(const char* pszId, BrTaskRef* pOut);     // 查不到返回假（程序重启过 / 号是外来的）

struct BrEv {
    INT   m[BM_COUNT];             // 21 个入参成员（就是数据块本身）
    // ---- 以下字段不参与块布局 ----
    INT   bAccept;                 // 应答：是否接受（默认 -1 真）
    INT   bStarted;                // 应答：采集线程是否已取走
    INT   bAlready;                // 应答：是否已在队列
    INT   nQueue;                  // 应答：队列数量
    char* pszTaskId;               // 应答：任务编号（库分配，EvFree 回收）
    char* pszMessage;              // 应答：提示消息
    char* pszOwn[BM_OWNED_COUNT];  // 事件自己的入参文本
    INT   nHandle;                 // 非 0 表示正挂在句柄表上（只在回调期间有效）
    HANDLE hDone;
    volatile INT nDone;    // 0 等待派发 1 已派发 2 已超时或被丢弃
    volatile INT nRefs;    // 投递者与派发者各持一份，归零才释放
    INT   nSeq;
    struct BrEv* pNext;
};

void EvSetText(BrEv* pEv, INT nMember, const char* pszUtf8);
BrEv* EvNew(INT nType);
// 造一条与真实 task 完全同形的采集事件（走同一条入队与派发路径），用于不装浏览器时自检
// pszTaskName 为空时按「站点名 + ASIN/页面类型」自动生成，保证任务名不为空
BrEv* EvMakeTask(const char* pszUrl, INT bStart, const char* pszTitle, const char* pszAsin, const char* pszRaw,
                 const char* pszTaskName);
void  EvFree(BrEv* pEv);
void  EvRelease(BrEv* pEv);   // 用完自己那一份引用；归零才真正释放
// 把事件还原进调用者自己的结构体变量：pVarSlot 是「参考」实参的 m_pCompoundData（变量地址）。
// 数据块与文本使用易语言内存池独立复制，交给调用者变量正常释放；句柄仅在回调期间有效。
BOOL  EvRestore(INT nHandle, void* pVarSlot, INT nArgType);
// 按页校验一段内存真的已提交且可写（bridge_ev.cpp 实现；取证探针也要用）
BOOL  RegionCheck(const void* p, INT cb);
// 回写应答。句柄只在回调期间有效，过期句柄返回假且不解引用。
BOOL  EvAnswer(INT nHandle, INT bAccept, const char* pszTaskId, const char* pszMessage,
               INT bAlready, INT nQueue, INT bStarted);
BOOL EvBindSub(DWORD dwCodeAddr);
void EvUnbind();
INT  EvBound();
// 入队一条事件；nWaitMs > 0 时同步等待采集程序应答，返回真表示已应答。
BOOL EvPost(BrEv* pEv, INT nWaitMs);
// 自动派发：就在调用线程（本机请求线程）里跑回调，不排队、不等泵。
void EvDispatchNow(BrEv* pEv);
// 采集程序侧派发：最多处理 nMax 条，返回实际处理条数。
INT  EvPump(INT nMax);
// 等待并派发一条，返回真表示派发了一条。
BOOL EvWait(INT nTimeoutMs);
INT  EvPending();
void EvClearAll();

// ===================== 服务 =====================

BOOL SrvStart();          // 绑定端口并起线程；已监听直接返回真
void SrvStop();
void SrvTick();           // 定时：端口未绑上就持续重试；插件超时判离线
BOOL SrvTryBind();        // 尝试绑定一个可用端口（不启线程，供宿主与重试使用）

}  // namespace cx

#endif  // CXBROW_BRIDGE_H
