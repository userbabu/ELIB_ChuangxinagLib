// 全命令覆盖宿主：按命令表的参数类型自动造实参，把每条全局命令与每个类的每条成员命令
// 都真调用一遍，检查三件事：不崩、返回值数据类型与命令表声明一致、没有 NRS_RUNTIME_ERR。
// 语义正确性由 host_uni / host_test 的点名断言负责，本宿主负责"没有没跑过的命令"。
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <tchar.h>

typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;

#include <lib2.h>
#include <lang.h>

#define AS_TEXT(p) ((const char*)(p))

typedef PLIB_INFO(WINAPI* fn_GetNewInf)(void);

static PLIB_INFO g_inf = NULL;
static HMODULE g_hMod = NULL;

// 崩了也要把出错位置交出来：打印异常地址与它在 .fne 里的 RVA。
static LONG CALLBACK Veh(PEXCEPTION_POINTERS p)
{
    DWORD code = p->ExceptionRecord->ExceptionCode;
    if (code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_ILLEGAL_INSTRUCTION ||
        code == EXCEPTION_STACK_OVERFLOW || code == EXCEPTION_IN_PAGE_ERROR)
    {
        char* addr = (char*)p->ExceptionRecord->ExceptionAddress;
        printf("[AV] code=0x%X addr=%p", code, addr);
        if (g_hMod != NULL && addr >= (char*)g_hMod && addr < (char*)g_hMod + 0x400000)
            printf(" rva=0x%X (在 cxsoft.fne 内)", (unsigned)(addr - (char*)g_hMod));
        else
            printf(" (在库外)");
        if (code == EXCEPTION_ACCESS_VIOLATION)
            printf(" 操作=%d 目标=%p", (int)p->ExceptionRecord->ExceptionInformation[0],
                   (void*)p->ExceptionRecord->ExceptionInformation[1]);
        printf("\n");
        fflush(stdout);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
static int g_failed = 0;
static int g_runtimeErr = 0;
static int g_called = 0, g_skipped = 0, g_malloc = 0;
// 逐条命令的覆盖标记：跑过 / 有理由跳过。收尾按位核对，不再用「调用次数 + 跳过次数 == 总数」
// 这种会被重复调用（构造函数、复制函数）打穿的算术断言。
static BYTE g_covered[2048];
static BYTE g_skipMark[2048];
static void MarkCovered(int i) { if (i >= 0 && i < (int)(sizeof(g_covered))) g_covered[i] = 1; }
static void MarkSkipped(int i) { if (i >= 0 && i < (int)(sizeof(g_skipMark))) g_skipMark[i] = 1; }
static bool g_bPrepopAry = false;   // 参考数组是否预置旧元素

// 有理由跳过的命令（类名 + 命令名；类名为空表示全局命令）。
struct SkipItem { const char* szClass; const char* szCmd; const char* szWhy; };
static const SkipItem g_skips[] = {
    { "",        "快速请求",     "走真实网络，由 host_test 点名覆盖" },
    { "浏览器会话", "执行",       "走真实网络与 libcurl，由 host_test 点名覆盖" },
    { "宽文本类",  "添加RC资源",   "需要带 RC 资源的模块句柄，离线宿主没有" },
    { "线程锁", "等待", "无限等待会把覆盖宿主挂死；超时/唤醒语义由 host_lock 并发宿主专测" },
    { "信号量", "等待", "计数为 0 时阻塞会把覆盖宿主挂死；由 host_lock 并发宿主专测" },
    { "条件变量", "等待", "同上：无唤醒者会永久挂起" },
    { "线程会合", "等待", "需要多个线程同时到达，由 host_lock 的会合并发验证覆盖" },
    { "取消令牌", "等待取消", "阻塞及超时语义由host_cancel的并发取消测试覆盖" },
    { "取消令牌", "等待继续", "暂停时阻塞，由host_cancel的暂停恢复取消测试覆盖" },
    { "",        "ThisCall",     "要调真实函数地址，离线无法安全构造调用约定" },
    { "",        "_取变量地址",  "语义是取易语言解释器里的变量地址，离线只能验不崩" },
};

// 一条命令崩掉不该带走整个覆盖跑：可定位的异常就地接住，记下命令继续跑。
static DWORD IsFilteredCrash(DWORD code)
{
    if (code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_ILLEGAL_INSTRUCTION ||
        code == EXCEPTION_PRIV_INSTRUCTION || code == EXCEPTION_INT_DIVIDE_BY_ZERO)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

static void Fail(const char* fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    printf("[FAIL] "); vprintf(fmt, ap); printf("\n");
    va_end(ap); g_failed++;
}

static void Ok(const char* fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    printf("       "); vprintf(fmt, ap); printf("\n");
    va_end(ap);
}

INT WINAPI FakeNotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    switch (nMsg)
    {
    case NRS_MALLOC: g_malloc++; return (INT)(INT_PTR)malloc(dwParam1 > 0 ? dwParam1 : 1);   // malloc(0) 可能返回 NULL，空文本就成"指针为空"
    case NRS_MFREE: if (dwParam1) free((void*)(INT_PTR)dwParam1); return 0;
    case NRS_FREE_ARY:
    {
        void* p = (void*)(INT_PTR)dwParam2;
        if (p != NULL)
        {
            int dim = *(int*)p;
            int n = 1;
            for (int i = 0; i < dim; i++) n *= *((int*)p + 1 + i);
            char* e = (char*)p + 4 * (dim + 1);
            DATA_TYPE et = (DATA_TYPE)dwParam1 & ~DT_IS_ARY;
            // DTM_SYS_DATA_TYPE_MASK 是标志位不是掩码：只有 文本/字节集 这类指针元素才逐个释放。
            if (et == SDT_TEXT || et == SDT_BIN)
                for (int i = 0; i < n; i++) if (*(void**)(e + i * 4)) free(*(void**)(e + i * 4));
            free(p);
        }
        return 0;
    }
    case NRS_RUNTIME_ERR: g_runtimeErr++; printf("    [运行时错误] %s\n", (const char*)(INT_PTR)dwParam1); return 0;
    default: return 0;
    }
}

// ============================================================ 夹具

// 中文夹具按 UTF-8 存放；宽文本夹具是 UTF-16 字节集，末尾带 {0,0}。
static const char g_szFixText[] = { 'a', 'b', (char)0xE4, (char)0xB8, (char)0xAD, 0 };        // ab中
static const char g_szFixUrl[]  = { 'h','t','t','p','s',':','/','/','w','w','w','.',
                                    'e','x','a','m','p','l','e','.','c','o','m','/','a','%','4','1', 0 };
static const char g_szFixPath[] = { 'o','u','t','\\','c','o','v','e','r','_','t','m','p','.','t','x','t', 0 };
static const wchar_t g_wFix[8] = { L'a', L'b', 0x4E2D, 0x6587, 0, 0, 0, 0 };

static int IsGbkName(const char* szName, const char* utf8)
{
    // 参数名在 .fne 里是 GBK，源码里的中文会被编译成 GBK，可直接比。
    return szName != NULL && strstr(szName, utf8) != NULL;
}

static LPBYTE MakeBin(const void* pData, int nBytes)
{
    char* p = (char*)malloc(8 + nBytes);
    *(int*)p = 1; *(int*)(p + 4) = nBytes;
    if (nBytes > 0) memcpy(p + 8, pData, nBytes);
    return (LPBYTE)p;
}

static LPBYTE MakeWideBin(int* pnBytes)
{
    int n = (int)(lstrlenW(g_wFix) + 1) * 2;
    *pnBytes = n;
    return MakeBin(g_wFix, n);
}

static void* MakeAryData(DATA_TYPE elemType, int nElements)
{
    char* p = (char*)malloc(8 + (size_t)nElements * 4);
    *(int*)p = 1; *(int*)(p + 4) = nElements;
    for (int i = 0; i < nElements; i++)
    {
        void* e = NULL;
        DATA_TYPE et = elemType & ~DT_IS_ARY;
        if (et == SDT_TEXT)
        {
            // 必须是堆地址：命令写回参考数组时会对每个元素做 free，指静态串就崩
            size_t len = strlen(g_szFixText) + 1;
            char* q = (char*)malloc(len);
            memcpy(q, g_szFixText, len);
            e = q;
        }
        else if (et == SDT_BIN)
        {
            int n = 0;
            e = MakeWideBin(&n);
        }
        if (e != NULL) *(void**)(p + 8 + i * 4) = e;
        else *(int*)(p + 8 + i * 4) = 1;   // 标量元素直接内联
    }
    return p;
}

// ============================================================ 对象

struct ObjRef
{
    void* pSlot;        // 对象数据 = 一个 INT，存 C++ 结构指针
    MDATA_INF arg;
    int nTypeIdx;       // 属于哪个类，-1 表示未构造
};

static void ObjBind(ObjRef* p, int nTypeIdx)
{
    memset(p, 0, sizeof(*p));
    p->pSlot = NULL;
    p->nTypeIdx = nTypeIdx;
    p->arg.m_dtDataType = MAKELONG(nTypeIdx + 1, 0);
    p->arg.m_pCompoundData = &p->pSlot;
}

static int FindCtor(int nTypeIdx);      // 前置声明
static void CallCmd(int nCmdIdx, MDATA_INF* pArgs, int nArgs, MDATA_INF* pRet, const char* szCtx);
static void SeedObject(int nTypeIdx, MDATA_INF* pSelf);

static void ConstructObj(ObjRef* p)
{
    if (g_inf->m_pDataType[p->nTypeIdx].m_nCmdCount == 0)
    {
        // 纯数据类型（如 CURL_证书信息）没有构造函数：给一块零初始化的成员数据即可
        const int nEl = g_inf->m_pDataType[p->nTypeIdx].m_nElementCount;
        p->arg.m_pCompoundData = (PINT)LocalAlloc(LPTR, (nEl > 0 ? nEl : 1) * sizeof(INT) + 8);
        p->arg.m_dtDataType = MAKELONG(p->nTypeIdx + 1, 0x8000);
        return;
    }
    int ctor = FindCtor(p->nTypeIdx);
    if (ctor < 0) { Fail("类型 %d 没有构造函数", p->nTypeIdx + 1); return; }
    MDATA_INF r;
    printf("       call [%d] %s.构造函数\n", ctor, AS_TEXT(g_inf->m_pDataType[p->nTypeIdx].m_szName));
    CallCmd(ctor, &p->arg, 1, &r, "构造函数");
    g_called++;
    memset(&r, 0, sizeof(r));
}

static int FindDtor(int nTypeIdx)
{
    PLIB_DATA_TYPE_INFO d = &g_inf->m_pDataType[nTypeIdx];
    for (int i = 0; i < d->m_nCmdCount; i++)
    {
        int idx = d->m_pnCmdsIndex[i];
        if (g_inf->m_pBeginCmdInfo[idx].m_wState & CT_IS_OBJ_FREE_CMD) return idx;
    }
    return -1;
}

static void DestructObj(ObjRef* p)
{
    int dtor = FindDtor(p->nTypeIdx);
    if (dtor < 0) return;
    MDATA_INF r;
    printf("       call [%d] %s.析构函数\n", dtor, AS_TEXT(g_inf->m_pDataType[p->nTypeIdx].m_szName));
    CallCmd(dtor, &p->arg, 1, &r, "析构函数");
    g_called++;
}

static int FindCtor(int nTypeIdx)
{
    PLIB_DATA_TYPE_INFO d = &g_inf->m_pDataType[nTypeIdx];
    for (int i = 0; i < d->m_nCmdCount; i++)
    {
        int idx = d->m_pnCmdsIndex[i];
        if (g_inf->m_pBeginCmdInfo[idx].m_wState & CT_IS_OBJ_CONSTURCT_CMD) return idx;
    }
    return -1;
}

// ============================================================ 实参构造

static int g_objPoolIdx = 0;    // 造实参用的类型游标

static void MakeArg(PARG_INFO a, MDATA_INF* pOut, int nSelfTypeIdx, MDATA_INF* pSelfArg)
{
    memset(pOut, 0, sizeof(*pOut));
    DATA_TYPE dt = a->m_dtType;
    const char* nm = AS_TEXT(a->m_szName);
    DWORD st = a->m_dwState;

    if (dt == _SDT_NULL) { pOut->m_dtDataType = _SDT_NULL; return; }

    // 通用型：给段可读写缓冲，命令只取地址
    if (dt == _SDT_ALL)
    {
        pOut->m_dtDataType = _SDT_ALL;
        pOut->m_pText = (char*)g_szFixText;
        return;
    }

    // 数组性看 m_dwState，不看 m_dtType：SDK 规定 ARG_INFO 里参数类型只写元素类型（不带 DT_IS_ARY），
    // 运行时才通过 DT_IS_ARY 标记传进来的是不是数组。
    BOOL bIsAryParam = (st & (AS_RECEIVE_VAR_ARRAY | AS_RECEIVE_VAR_OR_ARRAY |
                              AS_RECEIVE_ARRAY_DATA | AS_RECEIVE_ALL_TYPE_DATA)) != 0;
    if (bIsAryParam)
    {
        DATA_TYPE et = dt & ~DT_IS_ARY;
        if (st & (AS_RECEIVE_VAR_ARRAY | AS_RECEIVE_VAR_OR_ARRAY))
        {
            // 参考数组：交出一个变量地址。默认给空数组（真实调用几乎都是先声明再传），
            // 传 COVER_PREPOP 时才预置元素，用来压"写回前先释放旧内容"这条路。
            static void* s_slots[16];
            static int s_used = 0;
            void** pp = &s_slots[s_used % 16];
            s_used++;
            *pp = g_bPrepopAry ? MakeAryData(et, 2) : NULL;
            pOut->m_dtDataType = et | DT_IS_ARY;
            pOut->m_ppAryData = pp;
        }
        else
        {
            pOut->m_dtDataType = et | DT_IS_ARY;
            pOut->m_pAryData = MakeAryData(et, 2);
        }
        return;
    }

    // 自定义数据类型（对象）
    if (HIWORD(dt) == 0 && LOWORD(dt) >= 4)
    {
        int nTypeIdx = (int)LOWORD(dt) - 1;
        if (nTypeIdx == nSelfTypeIdx && pSelfArg != NULL) { *pOut = *pSelfArg; return; }
        if (nTypeIdx >= 0 && nTypeIdx < g_inf->m_nDataTypeCount)
        {
            static ObjRef s_pool[24];
            ObjRef* p = &s_pool[g_objPoolIdx % 24];
            g_objPoolIdx++;
            ObjBind(p, nTypeIdx);
            ConstructObj(p);
            SeedObject(nTypeIdx, &p->arg);   // 对象型实参也要处于「已创建」状态，否则只能在失效分支上打转
            pOut->m_dtDataType = p->arg.m_dtDataType;
            pOut->m_pCompoundData = p->arg.m_pCompoundData;
            // 令牌绑定参数是复合类型变量引用，与成员命令的self不是同一层指针。
            if (st & AS_RECEIVE_VAR && lstrcmpA(AS_TEXT(g_inf->m_pDataType[nTypeIdx].m_szName), "取消令牌") == 0)
                pOut->m_ppCompoundData = &p->arg.m_pCompoundData;
            return;
        }
    }

    // 传址（参考）标量/字节集/文本：交出的必须是变量地址，按值传会让实现解引用假指针。
    if (st & AS_RECEIVE_VAR)
    {
        static void* s_varSlots[24];
        static int s_varUsed = 0;
        if (s_varUsed >= 24) s_varUsed = 0;
        void** pSlot = &s_varSlots[s_varUsed++];
        *pSlot = NULL;
        if (dt == SDT_TEXT)
        {
            char* q = (char*)malloc(strlen(g_szFixText) + 1);
            strcpy(q, g_szFixText);
            *pSlot = q;
            pOut->m_dtDataType = SDT_TEXT;
            pOut->m_ppText = (char**)pSlot;
            return;
        }
        if (dt == SDT_BIN)
        {
            int n = 0;
            *pSlot = MakeWideBin(&n);      // 与宿主 fake 的 NRS_MALLOC 同一分配器，实现释放也安全
            pOut->m_dtDataType = SDT_BIN;
            pOut->m_ppBin = (LPBYTE*)pSlot;
            return;
        }
        pOut->m_dtDataType = dt;
        pOut->m_pInt = (INT*)pSlot;       // 标量：*(INT*)pSlot 给 0
        return;
    }

    switch (dt)
    {
    case SDT_TEXT:
        pOut->m_dtDataType = SDT_TEXT;
        if (IsGbkName(nm, "网址") || IsGbkName(nm, "URL") || IsGbkName(nm, "url")) pOut->m_pText = (char*)g_szFixUrl;
        else if (IsGbkName(nm, "文件") || IsGbkName(nm, "路径")) pOut->m_pText = (char*)g_szFixPath;
        else pOut->m_pText = (char*)g_szFixText;
        break;
    case SDT_BIN:
        pOut->m_dtDataType = SDT_BIN;
        {
            int n = 0;
            pOut->m_pBin = MakeWideBin(&n);
        }
        break;
    case SDT_BOOL:
        pOut->m_dtDataType = SDT_BOOL;
        pOut->m_bool = TRUE;
        break;
    case SDT_INT:
    case SDT_SHORT:
    case SDT_BYTE:
    case SDT_INT64:
    case SDT_DATE_TIME:
        pOut->m_dtDataType = dt;
        if (IsGbkName(nm, "指针") || IsGbkName(nm, "地址"))
        {
            // 冒烟扫描一律给 0：假地址会被 置指针 这类写入命令存进对象，后面任何一条命令解引用它都炸。
            // 真指针的正例由 host_uni 的定点断言负责。
            pOut->m_int = 0;
        }
        else if (IsGbkName(nm, "代码页")) pOut->m_int = 936;
        else if (IsGbkName(nm, "序号") || IsGbkName(nm, "索引") || IsGbkName(nm, "位置") ||
                 IsGbkName(nm, "次数") || IsGbkName(nm, "长度") || IsGbkName(nm, "数目") ||
                 IsGbkName(nm, "偏移")) pOut->m_int = 1;
        else pOut->m_int = 1;
        break;
    case SDT_FLOAT:
    case SDT_DOUBLE:
        pOut->m_dtDataType = dt;
        pOut->m_double = 1.25;
        break;
    default:
        pOut->m_dtDataType = SDT_INT;
        pOut->m_int = 1;
        break;
    }
}

// ============================================================ 调用与校验

static bool IsSkipped(const char* szClass, const char* szCmd)
{
    for (const SkipItem* s = g_skips; s < g_skips + sizeof(g_skips) / sizeof(g_skips[0]); s++)
        if (lstrcmpA(s->szClass, szClass) == 0 && lstrcmpA(s->szCmd, szCmd) == 0) return true;
    return false;
}

static void CheckRet(PCMD_INFO c, MDATA_INF* pRet, const char* szWhat)
{
    DATA_TYPE want = c->m_dtRetValType;
    if (c->m_wState & CT_RETRUN_ARY_TYPE_DATA)
    {
        if (pRet->m_pAryData == NULL) return;   // 空结果按约定返回 NULL 块
        int dim = *(int*)pRet->m_pAryData;
        int n = *((int*)pRet->m_pAryData + 1);
        if (dim != 1 || n < 0) Fail("%s 返回数组块格式不对 维数%d 元素%d", szWhat, dim, n);
        return;
    }
    if (want == _SDT_NULL)
    {
        if (pRet->m_dtDataType != _SDT_NULL && pRet->m_dtDataType != 0)
            Fail("%s 声明无返回值，实际填了类型 0x%X", szWhat, pRet->m_dtDataType);
        return;
    }
    if (HIWORD(want) == 0 && LOWORD(want) >= 4)   // 返回对象
    {
        if (pRet->m_pCompoundData == NULL) Ok("%s 返回空对象指针（未初始化对象时的降级）", szWhat);
        return;
    }
    // 定长返回类型由命令表决定，实现通常不回填 m_dtDataType（SDK 例程如此），所以 0 也算合格。
    if (pRet->m_dtDataType != want && pRet->m_dtDataType != 0)
    {
        Fail("%s 返回类型 0x%X，声明 0x%X", szWhat, pRet->m_dtDataType, want);
        return;
    }
    if (want == SDT_TEXT && pRet->m_pText == NULL) Fail("%s 返回文本型但指针为空", szWhat);
}

static void FreeRet(MDATA_INF* p)
{
    if (p->m_dtDataType == SDT_TEXT && p->m_pText) free(p->m_pText);
    else if (p->m_dtDataType == SDT_BIN && p->m_pBin) free(p->m_pBin);
    if (HIWORD(p->m_dtDataType) == 0 && LOWORD(p->m_dtDataType) >= 4 && p->m_pCompoundData)
        free(p->m_pCompoundData);
    memset(p, 0, sizeof(MDATA_INF));
}

static void CallCmd(int nCmdIdx, MDATA_INF* pArgs, int nArgs, MDATA_INF* pRet, const char* szCtx)
{
    MarkCovered(nCmdIdx);
    memset(pRet, 0, sizeof(MDATA_INF));
    g_inf->m_pCmdsFunc[nCmdIdx](pRet, nArgs, pArgs);
    UNREFERENCED_PARAMETER(szCtx);
}

// 跑一条命令：造齐参数、调用、校验返回类型。
static void RunCmd(int nCmdIdx, int nSelfTypeIdx, MDATA_INF* pSelfArg, const char* szClassTag)
{
    PCMD_INFO c = &g_inf->m_pBeginCmdInfo[nCmdIdx];
    const char* szName = AS_TEXT(c->m_szName);
    if (IsSkipped(szClassTag, szName))
    {
        for (const SkipItem* s = g_skips; s < g_skips + sizeof(g_skips) / sizeof(g_skips[0]); s++)
            if (lstrcmpA(s->szClass, szClassTag) == 0 && lstrcmpA(s->szCmd, szName) == 0)
                printf("       [SKIP] %s%s -> %s\n", *szClassTag ? szClassTag : "全局", *szClassTag ? "." : "", s->szWhy);
        g_skipped++;
        MarkSkipped(nCmdIdx);
        return;
    }

    MDATA_INF args[24];
    int n = 0;
    if (c->m_shtCategory == -1)
    {
        args[n++] = *pSelfArg;                        // 成员命令第 0 参是对象自身
    }
    for (int i = 0; i < c->m_nArgCount && n < 24; i++, n++)
    {
        if (getenv("COVER_TRACE_ARG")) printf("         arg[%d] dt=0x%X state=0x%X name=%s\n", i,
            c->m_pBeginArgInfo[i].m_dtType, c->m_pBeginArgInfo[i].m_dwState, AS_TEXT(c->m_pBeginArgInfo[i].m_szName));
        MakeArg(&c->m_pBeginArgInfo[i], &args[n], nSelfTypeIdx, pSelfArg);
        if (getenv("COVER_TRACE_ARG")) printf("           -> dt=0x%X union=%p\n", args[n].m_dtDataType, (void*)args[n].m_pText);
    }

    MDATA_INF r;
    printf("       call [%d] %s%s %s\n", nCmdIdx, *szClassTag ? szClassTag : "全局", *szClassTag ? "." : "", szName);
    memset(&r, 0, sizeof(r));
    __try
    {
        g_inf->m_pCmdsFunc[nCmdIdx](&r, n, args);
    }
    __except (IsFilteredCrash(GetExceptionCode()))
    {
        Fail("%s%s 调用中异常 code=0x%X", *szClassTag ? szClassTag : "全局", *szClassTag ? "." : "", GetExceptionCode());
        memset(&r, 0, sizeof(r));
    }
    MarkCovered(nCmdIdx);
    CheckRet(c, &r, szName);
    g_called++;
    FreeRet(&r);
    // 只有"按值"的 字节集 实参是宿主自己 malloc 的；传址参数的 union 里放的是变量地址，
    // 参考数组的 union 是元素块，都不能当成堆块 free（free 一个非堆地址会在几条之后把堆打坏）。
    int nSelf = (c->m_shtCategory == -1) ? 1 : 0;
    for (int i = nSelf; i < n; i++)
    {
        DWORD st = c->m_pBeginArgInfo[i - nSelf].m_dwState;
        BOOL bByRef = (st & (AS_RECEIVE_VAR | AS_RECEIVE_VAR_ARRAY | AS_RECEIVE_VAR_OR_ARRAY |
                             AS_RECEIVE_ARRAY_DATA | AS_RECEIVE_ALL_TYPE_DATA)) != 0;
        if (!bByRef && args[i].m_dtDataType == SDT_BIN && args[i].m_pBin) free(args[i].m_pBin);
    }
}

// 正则/宽文本类先播种一次可用状态，让后续命令走"有数据"分支。
static void SeedObject(int nTypeIdx, MDATA_INF* pSelf)
{
    PLIB_DATA_TYPE_INFO d = &g_inf->m_pDataType[nTypeIdx];
    for (int i = 0; i < d->m_nCmdCount; i++)
    {
        int idx = d->m_pnCmdsIndex[i];
        PCMD_INFO c = &g_inf->m_pBeginCmdInfo[idx];
        const char* nm = AS_TEXT(c->m_szName);
        if (lstrcmpA(nm, "创建W") != 0 && lstrcmpA(nm, "创建") != 0 && lstrcmpA(nm, "添加") != 0) continue;
        MDATA_INF args[8];
        int n = 0;
        args[n++] = *pSelf;
        for (int k = 0; k < c->m_nArgCount && n < 8; k++, n++) MakeArg(&c->m_pBeginArgInfo[k], &args[n], nTypeIdx, pSelf);
        MDATA_INF r;
        CallCmd(idx, args, n, &r, nm);
        FreeRet(&r);
        for (int j = 1; j < n; j++)
        {
            DWORD jst = c->m_pBeginArgInfo[j - 1].m_dwState;
            if (jst & (AS_RECEIVE_VAR | AS_RECEIVE_VAR_ARRAY | AS_RECEIVE_VAR_OR_ARRAY | AS_RECEIVE_ARRAY_DATA)) continue;
            if (args[j].m_dtDataType == SDT_BIN && args[j].m_pBin) free(args[j].m_pBin);
        }
        if (lstrcmpA(nm, "添加") == 0) continue;   // 宽文本类播一次就够
        return;                                    // 正则类创建完即返回
    }
}

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    char dllPath[MAX_PATH] = { 0 };
    if (argc > 1) lstrcpynA(dllPath, argv[1], MAX_PATH);
    else lstrcpynA(dllPath, "out\\cxsoft.fne", MAX_PATH);

    AddVectoredExceptionHandler(1, Veh);
    HMODULE h = LoadLibraryA(dllPath);
    if (h == NULL) { printf("[FAIL] 加载 %s 失败，错误码 %lu\n", dllPath, GetLastError()); return 1; }
    fn_GetNewInf pGetNewInf = (fn_GetNewInf)GetProcAddress(h, "GetNewInf");
    g_hMod = h;
    if (!pGetNewInf) { printf("[FAIL] 找不到 GetNewInf\n"); return 1; }
    g_inf = pGetNewInf();
    g_inf->m_pfnNotify(NL_SYS_NOTIFY_FUNCTION, (DWORD)(INT_PTR)&FakeNotifySys, 0);

    printf("[host_cover] %s  命令 %d 条 / 类 %d 个\n", dllPath, g_inf->m_nCmdCount, g_inf->m_nDataTypeCount);

    for (int i = 2; i < argc; i++) if (lstrcmpA(argv[i], "-prepop") == 0) g_bPrepopAry = true;
    int nFrom = argc > 2 ? atoi(argv[2]) : 0;
    int nTo = argc > 3 ? atoi(argv[3]) : g_inf->m_nCmdCount - 1;
    // ---- 全局命令 ----
    MDATA_INF dummy; memset(&dummy, 0, sizeof(dummy));
    for (int i = 0; i < g_inf->m_nCmdCount; i++)
    {
        if (g_inf->m_pBeginCmdInfo[i].m_shtCategory > 0 && i >= nFrom && i <= nTo) RunCmd(i, -1, &dummy, "");
    }

    // ---- 每个类的成员命令 ----
    for (int t = 0; t < g_inf->m_nDataTypeCount; t++)
    {
        PLIB_DATA_TYPE_INFO d = &g_inf->m_pDataType[t];
        const char* szClass = AS_TEXT(d->m_szName);
        if (d->m_nCmdCount == 0) { printf("       [SKIP] %s is pure data type\n", szClass); continue; }
        ObjRef o; ObjBind(&o, t);
        ConstructObj(&o);
        SeedObject(t, &o.arg);

        // 句柄型类（curl_easy / curl_http / curl_smtp / curl_multi）：销毁 会把句柄收回，必须留到最后一条，
        // 否则后面的成员命令全在"句柄已失效"分支上跑，测不到真实行为。
        int bHandleClass = 0;
        for (int i = 0; i < d->m_nCmdCount; i++)
        {
            const char* nm = AS_TEXT(g_inf->m_pBeginCmdInfo[d->m_pnCmdsIndex[i]].m_szName);
            if (lstrcmpA(nm, "销毁") == 0) bHandleClass = 1;
        }
        for (int i = 0; i < d->m_nCmdCount; i++)
        {
            int idx = d->m_pnCmdsIndex[i];
            if (idx < nFrom || idx > nTo) continue;
            PCMD_INFO c = &g_inf->m_pBeginCmdInfo[idx];
            const char* nm = AS_TEXT(c->m_szName);
            if (c->m_wState & CT_IS_OBJ_CONSTURCT_CMD) continue;                    // 已由 ConstructObj 覆盖
            if (c->m_wState & CT_IS_OBJ_FREE_CMD) continue;                          // 最后统一析构
            if (bHandleClass && (lstrcmpA(nm, "销毁") == 0 || lstrcmpA(nm, "置句柄") == 0)) continue;
            if (c->m_wState & CT_IS_OBJ_COPY_CMD)
            {
                // 复制函数：目标对象必须处于未初始化状态，参数 = [目标, 源]
                ObjRef cp; ObjBind(&cp, t);
                MDATA_INF args[2] = { cp.arg, o.arg };
                MDATA_INF r;
                printf("       call [%d] %s.%s\n", idx, szClass, nm);
                CallCmd(idx, args, 2, &r, nm);
                g_called++;
                int dtor = FindDtor(t);
                if (dtor >= 0)
                {
                    MDATA_INF a1[1] = { cp.arg };
                    MDATA_INF r1;
                    CallCmd(dtor, a1, 1, &r1, "析构函数");
                }
                continue;
            }
            RunCmd(idx, t, &o.arg, szClass);
        }
        if (bHandleClass)
        {
            // 收尾两条：置句柄 用当前活句柄回灌（验正常路径而非拒收），再 销毁
            int idxSet = -1, idxDead = -1, idxGet = -1;
            for (int i = 0; i < d->m_nCmdCount; i++)
            {
                int idx = d->m_pnCmdsIndex[i];
                const char* nm = AS_TEXT(g_inf->m_pBeginCmdInfo[idx].m_szName);
                if (lstrcmpA(nm, "置句柄") == 0) idxSet = idx;
                else if (lstrcmpA(nm, "销毁") == 0) idxDead = idx;
                else if (lstrcmpA(nm, "取句柄") == 0) idxGet = idx;
            }
            INT hLive = 0;
            if (idxGet >= 0)
            {
                MDATA_INF a1[1] = { o.arg };
                MDATA_INF r1;
                CallCmd(idxGet, a1, 1, &r1, "取句柄");
                hLive = r1.m_int;
                FreeRet(&r1);
            }
            if (idxSet >= 0 && hLive != 0)
            {
                MDATA_INF a2[2] = { o.arg, *(MDATA_INF*)0 };
                a2[1] = a2[0];
                a2[1].m_pCompoundData = NULL;
                MDATA_INF hArg;
                memset(&hArg, 0, sizeof(hArg));
                hArg.m_dtDataType = SDT_INT;
                hArg.m_int = hLive;
                MDATA_INF args[2] = { o.arg, hArg };
                MDATA_INF r;
                printf("       call [%d] %s. 置句柄(活句柄)\n", idxSet, szClass);
                CallCmd(idxSet, args, 2, &r, "置句柄");
                FreeRet(&r);
                g_called++;
                UNREFERENCED_PARAMETER(a2);
            }
            if (idxDead >= 0)
            {
                MDATA_INF a[1] = { o.arg };
                MDATA_INF r;
                printf("       call [%d] %s. 销毁\n", idxDead, szClass);
                CallCmd(idxDead, a, 1, &r, "销毁");
                FreeRet(&r);
                g_called++;
            }
        }
        DestructObj(&o);
    }

    printf("\n实际调用 %d 条，有理由跳过 %d 条，运行时错误 %d 次\n", g_called, g_skipped, g_runtimeErr);
    int total = 0;
    for (int i = 0; i < g_inf->m_nCmdCount; i++) total++;
    printf("命令表共 %d 条\n", total);
    // 按命令下标逐条核对：既没跑过、又没写理由跳过的，就是漏测
    int nUncovered = 0;
    for (int i = 0; i < g_inf->m_nCmdCount; i++)
    {
        if (g_covered[i] || g_skipMark[i]) continue;
        if (nUncovered < 20)
            Fail("命令 [%d] %s 既没跑到也没有跳过理由", i, AS_TEXT(g_inf->m_pBeginCmdInfo[i].m_szName));
        nUncovered++;
    }
    if (nUncovered > 20) Fail("另有 %d 条命令同样漏测", nUncovered - 20);
    if (g_runtimeErr) Fail("出现运行时错误");
    printf(g_failed ? "\n结论: %d 项不合格\n" : "\n结论: 全部命令已覆盖且通过\n", g_failed);
    FreeLibrary(h);
    return g_failed ? 1 : 0;
}
