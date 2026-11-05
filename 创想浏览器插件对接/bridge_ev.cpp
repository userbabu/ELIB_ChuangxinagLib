// 事件队列与「子程序绑定」派发。
// 为什么不是直接把结构体交给子程序：易语言编译期规定「被取地址的子程序形参必须是基本数据类型」
// （错误 10063），自定义类型哪怕加参考也不行。所以回调只收一个整数句柄，
// 读数据用 还原事件信息（写进调用者自己的结构体），回应答用 应答采集事件。
// 生命周期：句柄只在回调期间挂在句柄表上，回调一返回就摘掉；过期句柄一律返回假，绝不解引用。
// 内存：事件与易语言变量各自持有独立文本，变量赋值和销毁不会影响事件及 HTTP 回执。
#include "bridge.h"
#include <stdio.h>

namespace cx {

// 易语言子程序以 stdcall 调用；形参是一个整数型句柄。
typedef INT (WINAPI *PFN_BROW_CB)(INT nHandle);

#define EV_SLOT_COUNT 64

static DWORD   g_cbAddr = 0;
static char    g_cbName[128] = { 0 };   // 绑定时按地址反查到的子程序名，只进日志，认不出留空
static BrEv*   g_pHead = NULL;
static BrEv*   g_pTail = NULL;
static HANDLE  g_hQueue = NULL;
static INT     g_nPending = 0;
static BrEv*   g_slots[EV_SLOT_COUNT];
static INT     g_nNextHandle = 1;

static INT OwnedIndexOf(INT nMember)
{
    for (INT i = 0; i < BM_OWNED_COUNT; ++i)
    {
        if (BM_OWNED[i] == nMember) return i;
    }
    return -1;
}

// 句柄表：只在 g_cs 里改，摘掉之后任何线程再拿这个号都查不到
static INT HandleAttach(BrEv* pEv)
{
    INT nRet = 0;
    EnterCriticalSection(&g_cs);
    for (INT i = 0; i < EV_SLOT_COUNT; ++i)
    {
        if (g_slots[i] == NULL)
        {
            g_slots[i] = pEv;
            pEv->nHandle = g_nNextHandle++;
            nRet = pEv->nHandle;
            break;
        }
    }
    LeaveCriticalSection(&g_cs);
    return nRet;
}

static void HandleDetach(BrEv* pEv)
{
    EnterCriticalSection(&g_cs);
    for (INT i = 0; i < EV_SLOT_COUNT; ++i)
    {
        if (g_slots[i] == pEv) g_slots[i] = NULL;
    }
    pEv->nHandle = 0;
    LeaveCriticalSection(&g_cs);
}

static BrEv* HandleFind(INT nHandle)
{
    BrEv* pRet = NULL;
    if (nHandle <= 0) return NULL;
    EnterCriticalSection(&g_cs);
    for (INT i = 0; i < EV_SLOT_COUNT; ++i)
    {
        if (g_slots[i] != NULL && g_slots[i]->nHandle == nHandle) { pRet = g_slots[i]; break; }
    }
    LeaveCriticalSection(&g_cs);
    return pRet;
}

static void SlotsClear()
{
    EnterCriticalSection(&g_cs);
    for (INT i = 0; i < EV_SLOT_COUNT; ++i) g_slots[i] = NULL;
    LeaveCriticalSection(&g_cs);
}

BrEv* EvNew(INT nType)
{
    BrInitOnce();
    BrEv* p = (BrEv*)calloc(1, sizeof(BrEv));
    if (p == NULL) return NULL;
    p->m[BM_TYPE] = nType;
    p->nSeq = BrNextSeq();
    p->m[BM_SEQ] = p->nSeq;
    p->nRefs = 1;
    // 应答默认：接受任务、未确认开始、未在队列。易语言的 真 = -1，别写 1。
    p->bAccept = BL_TRUE;
    return p;
}

void EvSetText(BrEv* pEv, INT nMember, const char* pszUtf8)
{
    if (pEv == NULL) return;
    const INT i = OwnedIndexOf(nMember);
    if (i < 0) return;
    if (pEv->pszOwn[i] != NULL)
    {
        PoolFree(pEv->pszOwn[i]);
        pEv->pszOwn[i] = NULL;
    }
    pEv->m[nMember] = 0;
    if (pszUtf8 == NULL || pszUtf8[0] == '\0') return;
    const INT n = StrLen(pszUtf8);
    char* p = (char*)PoolMalloc(n + 1);
    if (p == NULL) return;
    memcpy(p, pszUtf8, n);
    p[n] = '\0';
    pEv->m[nMember] = (INT)(INT_PTR)p;
    pEv->pszOwn[i] = p;
}

// 造一条与真实 task 同形的采集事件：走同一套入队/句柄/派发路径，
// 这样不装浏览器、不点插件按钮也能验「调泵 → 回调 → 应答」这条链通不通。
BrEv* EvMakeTask(const char* pszUrl, INT bStart, const char* pszTitle, const char* pszAsin, const char* pszRaw,
                 const char* pszTaskName)
{
    if (pszUrl == NULL || pszUrl[0] == '\0') return NULL;
    BrMatch m;
    MatchUrl(pszUrl, &m);
    if (!m.bIdentifiable) return NULL;

    BrEv* pEv = EvNew(EV_TASK);
    char szTime[32];
    NowText(szTime, sizeof(szTime));
    EvSetText(pEv, BM_ARRIVE, szTime);
    EvSetText(pEv, BM_URL, pszUrl);
    EvSetText(pEv, BM_TITLE, pszTitle == NULL ? "" : pszTitle);
    EvSetText(pEv, BM_PLATFORM, m.platform);
    EvSetText(pEv, BM_SITE, m.display);
    EvSetText(pEv, BM_COUNTRY, m.country);
    EvSetText(pEv, BM_CURRENCY, m.currency);
    EvSetText(pEv, BM_LANGUAGE, m.language);
    EvSetText(pEv, BM_ASIN, (pszAsin != NULL && pszAsin[0] != 0) ? pszAsin : m.asin);
    EvSetText(pEv, BM_EXT_VER, g_stat.szExtVer);
    EvSetText(pEv, BM_BROWSER, g_stat.szExtBrowser);
    EvSetText(pEv, BM_EXT_ID, g_stat.szExtId);
    EvSetText(pEv, BM_RAW, pszRaw == NULL ? "" : pszRaw);
    // 任务名：插件提交任务时可以自带（ac-client 里 addTask(url, start, { taskName: "…" })，也认 info.name）。
    // 没带就按「站点名 + ASIN」补一个，列表页/店铺页用页面类型顶上，别让易语言那边拿到空任务名。
    char szAuto[256];
    szAuto[0] = '\0';
    if (pszTaskName == NULL || pszTaskName[0] == '\0')
    {
        const char* pszAsinFinal = (pszAsin != NULL && pszAsin[0] != 0) ? pszAsin : m.asin;
        const char* pszTail = (pszAsinFinal[0] != 0) ? pszAsinFinal :
            (m.nPageType == PT_DETAIL ? Zh("详情页") : (m.nPageType == PT_SHOP ? Zh("店铺页") : Zh("列表页")));
        StrCpyN(szAuto, (INT)sizeof(szAuto) - 32, m.display);
        INT n = StrLen(szAuto);
        INT nTail = StrLen(pszTail);
        if (n + 1 + nTail < (INT)sizeof(szAuto))
        {
            szAuto[n] = ' ';
            memcpy(szAuto + n + 1, pszTail, nTail);
            szAuto[n + 1 + nTail] = '\0';
        }
    }
    EvSetText(pEv, BM_TASKNAME, (pszTaskName != NULL && pszTaskName[0] != '\0') ? pszTaskName : szAuto);
    pEv->m[BM_PAGETYPE] = m.nPageType;
    pEv->m[BM_COLMODE] = m.nColMode;
    pEv->m[BM_START] = bStart ? BL_TRUE : BL_FALSE;
    return pEv;
}

static void EvSetAnswerText(char** ppDst, const char* pszUtf8)
{
    if (ppDst == NULL) return;
    if (*ppDst != NULL)
    {
        PoolFree(*ppDst);
        *ppDst = NULL;
    }
    if (pszUtf8 == NULL || pszUtf8[0] == '\0') return;
    const INT n = StrLen(pszUtf8);
    char* p = (char*)PoolMalloc(n + 1);
    if (p == NULL) return;
    memcpy(p, pszUtf8, n);
    p[n] = '\0';
    *ppDst = p;
}

void EvFree(BrEv* pEv)
{
    if (pEv == NULL) return;
    for (INT i = 0; i < BM_OWNED_COUNT; ++i)
    {
        if (pEv->pszOwn[i] != NULL) PoolFree(pEv->pszOwn[i]);
        pEv->pszOwn[i] = NULL;
        pEv->m[BM_OWNED[i]] = 0;
    }
    EvSetAnswerText(&pEv->pszTaskId, NULL);
    EvSetAnswerText(&pEv->pszMessage, NULL);
    if (pEv->hDone != NULL)
    {
        CloseHandle(pEv->hDone);
        pEv->hDone = NULL;
    }
    free(pEv);
}

void EvRelease(BrEv* pEv)
{
    if (pEv == NULL) return;
    if (ATOM_DEC(&pEv->nRefs) <= 0) EvFree(pEv);
}

static void EvAddRef(BrEv* pEv) { ATOM_INC(&pEv->nRefs); }

BOOL EvBindSub(DWORD dwCodeAddr)
{
    BrInitOnce();
    if (g_hQueue == NULL) g_hQueue = CreateEventA(NULL, FALSE, FALSE, NULL);
    EnterCriticalSection(&g_cs);
    g_cbAddr = dwCodeAddr;
    LeaveCriticalSection(&g_cs);
    if (dwCodeAddr != 0) BrLog("%s (%u)", Zh("已绑定接收子程序"), dwCodeAddr);
    else BrLog("%s", Zh("已解除接收子程序"));
    return dwCodeAddr != 0;
}

void EvUnbind()
{
    EnterCriticalSection(&g_cs);
    g_cbAddr = 0;
    LeaveCriticalSection(&g_cs);
}

INT EvBound() { return g_cbAddr != 0 ? 1 : 0; }

// 绑定成功后由命令层把「取子程序地址」时用的名字交进来，日志里就能直接指出是哪个子程序出的错。
void CbSetName(const char* pszName)
{
    if (pszName == NULL) { g_cbName[0] = '\0'; return; }
    lstrcpynA(g_cbName, pszName, (int)sizeof(g_cbName));
    if (g_cbAddr != 0) BrLog("%s：%s（地址 %u）", Zh("已绑定接收子程序"), g_cbName, g_cbAddr);
}
static const char* CbWho() { return g_cbName[0] ? g_cbName : Zh("(未取子程序名)"); }

INT EvPending() { return g_nPending; }

// 校验一段内存真的可写：IsBadWritePtr 在优化编译下会误判（会顺着 4 字节的槽写 84 字节踩坏邻居），
// 所以按页查 VirtualQuery，确认已提交、可写、且剩余区域够大才算通过。
BOOL RegionCheck(const void* p, INT cb)
{
    if (p == NULL || cb <= 0) return FALSE;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((LPVOID)p, &mbi, sizeof(mbi)) == 0) return FALSE;
    if (mbi.State != MEM_COMMIT) return FALSE;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return FALSE;
    if ((mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) == 0)
        return FALSE;
    const SIZE_T off = (SIZE_T)((const BYTE*)p - (const BYTE*)mbi.BaseAddress);
    return mbi.RegionSize >= (SIZE_T)cb + off;
}

// 参考参数指向易语言变量槽；空槽必须分配独立数据块，不能借用事件对象的内部地址。
static int* ResolveBlock(void* pVarSlot, const char** ppszHow, INT* pnSlot, const char** ppszWhy)
{
    *ppszHow = "none";
    *pnSlot = 0;
    *ppszWhy = "slot4";
    if (!RegionCheck(pVarSlot, (INT)sizeof(void*))) return NULL;
    void* p = *(void**)pVarSlot;
    *pnSlot = (INT)(INT_PTR)p;
    if (p == NULL)
    {
        int* pBlock = (int*)PoolMalloc(sizeof(INT) * BM_COUNT);
        if (pBlock == NULL) { *ppszWhy = "alloc"; return NULL; }
        memset(pBlock, 0, sizeof(INT) * BM_COUNT);
        *(void**)pVarSlot = pBlock;
        *ppszHow = "C";
        return pBlock;
    }
    *ppszWhy = "block84";
    if (RegionCheck(p, sizeof(INT) * BM_COUNT))
    {
        *ppszHow = "A";
        return (int*)p;
    }
    const INT nAsType = (INT)(INT_PTR)p;
    if (nAsType >= EV_TASK && nAsType <= EV_ALL_DISCONNECT &&
        RegionCheck(pVarSlot, sizeof(INT) * BM_COUNT))
    {
        *ppszHow = "B";
        return (int*)pVarSlot;
    }
    return NULL;
}

// 还原：pVarSlot 是「参考」实参的 m_pCompoundData，nArgType 是它的 m_dtDataType（只进报错，用来分清
// 「易语言传了空白数据」和「传了个用不了的地址」）
BOOL EvRestore(INT nHandle, void* pVarSlot, INT nArgType)
{
    BrEv* pEv = HandleFind(nHandle);
    if (pEv == NULL)
    {
        BrSetError(Zh("还原事件信息失败：句柄 %d 不在表里（在回调外调用，或回调已返回）。"), nHandle);
        BrLogOnce("restore-stale", Zh("%s 句柄=%d 槽=%x"), Zh("还原事件信息：句柄不在表里（回调外调用或已过期）"), nHandle, (INT)(INT_PTR)pVarSlot);
        return FALSE;
    }
    const char* pszHow = "none";
    const char* pszWhy = "none";
    INT nSlot = 0;
    if (pVarSlot == NULL)
    {
        BrSetError(Zh("还原事件信息失败：第 2 个参数是空白数据（数据类型码 %x），没有可写的地方。请确认这里传的是 浏览器事件信息 类型的变量，并在调用前先给它赋一次值，例如：事件.事件类型 = 99"), nArgType);
        BrLogOnce("restore-empty", Zh("%s 句柄=%d 数据类型码=%x"), Zh("还原事件信息：第 2 个参数是空白数据"), nHandle, nArgType);
        return FALSE;
    }
    int* pBlock = ResolveBlock(pVarSlot, &pszHow, &nSlot, &pszWhy);
    if (pBlock == NULL)
    {
        BrSetError(Zh("还原事件信息失败：拿不到「事件」变量的可写内存（变量地址 %x，槽里的值 %x，数据类型码 %x，卡在 %s）。"),
            (INT)(INT_PTR)pVarSlot, nSlot, nArgType, pszWhy);
        BrLogOnce("restore-none", Zh("%s 句柄=%d 槽=%x 值=%x 类型=%x 卡在=%s"),
            Zh("还原事件信息：拿不到数据块地址"), nHandle, (INT)(INT_PTR)pVarSlot, nSlot, nArgType, pszWhy);
        return FALSE;
    }
    char szKey[24];
    CxSnprintf(szKey, sizeof(szKey), "restore-%s", pszHow);
    BrLogOnce(szKey, "%s = %s %s 0x%x", Zh("还原事件信息取块方式"), pszHow, Zh("变量地址"), (INT)(INT_PTR)pVarSlot);
    // 先完整复制，再替换调用者变量；分配失败时保留旧值，不交付半条事件。
    INT copy[BM_COUNT];
    memcpy(copy, pEv->m, sizeof(copy));
    for (INT i = 0; i < BM_OWNED_COUNT; ++i) copy[BM_OWNED[i]] = 0;
    for (INT i = 0; i < BM_OWNED_COUNT; ++i)
    {
        const INT member = BM_OWNED[i];
        const char* src = (const char*)(INT_PTR)pEv->m[member];
        if (src == NULL) continue;
        const INT bytes = StrLen(src) + 1;
        char* dst = (char*)PoolMalloc(bytes);
        if (dst == NULL)
        {
            for (INT j = 0; j < BM_OWNED_COUNT; ++j)
                PoolFree((void*)(INT_PTR)copy[BM_OWNED[j]]);
            BrSetError("%s", Zh("还原事件信息失败：无法分配事件文本内存。"));
            return FALSE;
        }
        memcpy(dst, src, bytes);
        copy[member] = (INT)(INT_PTR)dst;
    }
    // SDK 的参考复合类型由易语言正常管理，覆盖成员前必须释放原来的文本。
    for (INT i = 0; i < BM_OWNED_COUNT; ++i)
        PoolFree((void*)(INT_PTR)pBlock[BM_OWNED[i]]);
    memcpy(pBlock, copy, sizeof(copy));
    return TRUE;
}

BOOL EvAnswer(INT nHandle, INT bAccept, const char* pszTaskId, const char* pszMessage,
              INT bAlready, INT nQueue, INT bStarted)
{
    BrEv* pEv = HandleFind(nHandle);
    if (pEv == NULL) return FALSE;
    pEv->bAccept = bAccept ? BL_TRUE : BL_FALSE;
    pEv->bAlready = bAlready ? BL_TRUE : BL_FALSE;
    pEv->bStarted = bStarted ? BL_TRUE : BL_FALSE;
    pEv->nQueue = nQueue;
    EvSetAnswerText(&pEv->pszTaskId, pszTaskId);
    EvSetAnswerText(&pEv->pszMessage, pszMessage);
    return TRUE;
}

static void EvDispatch(BrEv* pEv);

static void QueueEnsure()
{
    BrInitOnce();
    if (g_hQueue == NULL) g_hQueue = CreateEventA(NULL, FALSE, FALSE, NULL);
}

// 从队列里摘掉一条还没被派发的事件（超时后才需要）
static BOOL QueueRemove(BrEv* pEv)
{
    BOOL bRemoved = FALSE;
    EnterCriticalSection(&g_cs);
    BrEv* pPrev = NULL;
    for (BrEv* p = g_pHead; p != NULL; p = p->pNext)
    {
        if (p == pEv)
        {
            if (pPrev == NULL) g_pHead = p->pNext;
            else pPrev->pNext = p->pNext;
            if (g_pTail == p) g_pTail = (pPrev == NULL) ? NULL : pPrev;
            --g_nPending;
            bRemoved = TRUE;
            break;
        }
        pPrev = p;
    }
    LeaveCriticalSection(&g_cs);
    return bRemoved;
}

BOOL EvPost(BrEv* pEv, INT nWaitMs)
{
    if (pEv == NULL) return FALSE;
    QueueEnsure();

    // 自动派发（默认）：就在投递这条事件的线程里跑回调，不排队、不等泵。
    // 等应答的事件由投递方读完应答后自己减引用；不等应答的这里就回收掉。
    if (g_cfg.nCallbackMode == 0)
    {
        EvDispatch(pEv);
        if (nWaitMs <= 0) EvRelease(pEv);
        return TRUE;
    }

    const BOOL bWait = (nWaitMs > 0);
    if (bWait)
    {
        pEv->hDone = CreateEventA(NULL, FALSE, FALSE, NULL);
        if (pEv->hDone == NULL) return FALSE;
        EvAddRef(pEv);          // 投递者 + 派发者各一份
    }

    EnterCriticalSection(&g_cs);
    if (g_pTail == NULL) g_pHead = pEv;
    else g_pTail->pNext = pEv;
    g_pTail = pEv;
    pEv->pNext = NULL;
    ++g_nPending;
    LeaveCriticalSection(&g_cs);

    if (g_hQueue != NULL) SetEvent(g_hQueue);
    if (!bWait) return TRUE;

    const DWORD dw = WaitForSingleObject(pEv->hDone, (DWORD)nWaitMs);
    if (dw != WAIT_OBJECT_0)
    {
        // 只有仍在队列里的事件才能取消；已派发的事件由派发线程收尾。
        if (QueueRemove(pEv))
        {
            pEv->nDone = 2;
            EvRelease(pEv);   // 释放从队列取回的派发者引用
        }
    }
    // 调用者持有原始引用，必须读取完回执后再释放；等待函数不代替调用者释放。
    return dw == WAIT_OBJECT_0 && pEv->nDone == 1;
}

// 派发一条（调用方已把它摘出队列）
static void EvDispatch(BrEv* pEv)
{
    const DWORD dwAddr = g_cbAddr;
    if (dwAddr == 0)
    {
        // 没绑定子程序：采集请求按「不接受」回，其余事件直接丢弃
        pEv->bAccept = BL_FALSE;
        pEv->nDone = 1;
        if (pEv->hDone != NULL) SetEvent(pEv->hDone);
        return;
    }

    const INT nHandle = HandleAttach(pEv);
    if (nHandle == 0)
    {
        // 句柄表满了（同时派发的事件超过 64 条）：宁可拒收也不给一个用不了的号
        BrLogOnce("slot", "%s", Zh("同时派发的事件太多，本条已按拒绝处理。"));
        pEv->bAccept = BL_FALSE;
        pEv->nDone = 1;
        if (pEv->hDone != NULL) SetEvent(pEv->hDone);
        return;
    }

    // 「已连接浏览器数量」在派发这一刻取当前的在线台数：所有事件类型都带这一个字段，
    // 程序不用按类型分支，也不会在事件入队时就把台数定死。
    pEv->m[BM_BROWSER_COUNT] = SessOnline();

    PFN_BROW_CB pfn = (PFN_BROW_CB)(INT_PTR)dwAddr;
    // 原子性：服务是每个请求一条工作线程，默认必须把回调串起来 ——
    // 易语言子程序不是可重入的，两个事件同时进同一个子程序会互相踩全局变量与界面。
    // 需要并发吞吐时由 置事件并发 (真) 显式打开，那时线程安全归调用方负责。
    const BOOL bSerial = (g_cfg.nConcurrent == 0);
    if (bSerial) EnterCriticalSection(&g_csDispatch);
    __try
    {
        pfn(nHandle);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        BrSetError("%s", Zh("接收子程序执行出错，请检查子程序里的代码。"));
        BrLogOnce("cb", "%s：接收子程序「%s」访问了非法内存，本次事件已丢弃", Zh("派发异常"), CbWho());
        pEv->bAccept = BL_FALSE;
    }
    if (bSerial) LeaveCriticalSection(&g_csDispatch);
    HandleDetach(pEv);

    pEv->nDone = 1;
    if (pEv->hDone != NULL) SetEvent(pEv->hDone);
    // 唤醒后投递者可能随时回收，这里不能再碰 pEv
}

void EvDispatchNow(BrEv* pEv)
{
    if (pEv == NULL) return;
    EvDispatch(pEv);
}

INT EvPump(INT nMax)
{
    QueueEnsure();
    if (nMax <= 0) nMax = 64;
    INT n = 0;
    while (n < nMax)
    {
        EnterCriticalSection(&g_cs);
        BrEv* pEv = g_pHead;
        if (pEv != NULL)
        {
            g_pHead = pEv->pNext;
            if (g_pHead == NULL) g_pTail = NULL;
            --g_nPending;
        }
        LeaveCriticalSection(&g_cs);
        if (pEv == NULL) break;
        pEv->pNext = NULL;
        ++n;
        EvDispatch(pEv);
        EvRelease(pEv);      // 每次入队都交给派发者一份引用，派发完成统一释放
    }
    return n;
}

BOOL EvWait(INT nTimeoutMs)
{
    QueueEnsure();
    if (g_nPending <= 0)
    {
        WaitForSingleObject(g_hQueue, (DWORD)(nTimeoutMs > 0 ? nTimeoutMs : 1000));
    }
    return EvPump(1) > 0;
}

void EvClearAll()
{
    QueueEnsure();
    SlotsClear();
    EnterCriticalSection(&g_cs);
    BrEv* p = g_pHead;
    g_pHead = NULL;
    g_pTail = NULL;
    g_nPending = 0;
    LeaveCriticalSection(&g_cs);
    while (p != NULL)
    {
        BrEv* pNext = p->pNext;
        p->pNext = NULL;
        p->nDone = 2;                 // 服务停了，等应答的一方立刻收工
        if (p->hDone != NULL) SetEvent(p->hDone);
        EvRelease(p);
        p = pNext;
    }
}

}  // namespace cx
