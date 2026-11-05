// 离线宿主：验证从 创想软件UNICODE模块 迁移进来的宽文本/正则命令。
// 与 host_test.cpp 一样直接装载 cxsoft.fne，按中文命令名调用，不依赖易语言 IDE。
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

static HMODULE g_hMod = NULL;

static LONG CALLBACK Veh(PEXCEPTION_POINTERS ep)
{
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    if (code != 0)          // 什么都打，堆坏了走的是 0xC0000409 fastfail
    {
        char* addr = (char*)ep->ExceptionRecord->ExceptionAddress;
        printf("[AV] code=0x%X addr=%p", code, addr);
        if (g_hMod != NULL && addr >= (char*)g_hMod && addr < (char*)g_hMod + 0x400000)
            printf(" rva=0x%X 在库内", (unsigned)(addr - (char*)g_hMod));
        else printf(" 在库外");
        if (code == EXCEPTION_ACCESS_VIOLATION)
            printf(" 操作=%d 目标=%p", (int)ep->ExceptionRecord->ExceptionInformation[0],
                   (void*)ep->ExceptionRecord->ExceptionInformation[1]);
        printf("\n");
        fflush(stdout);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static int g_mallocCount = 0;
static int g_freeCount = 0;
static int g_runtimeErr = 0;
static int g_failed = 0;

INT WINAPI FakeNotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    UNREFERENCED_PARAMETER(dwParam2);
    switch (nMsg)
    {
    case NRS_MALLOC: g_mallocCount++; return (INT)(INT_PTR)malloc(dwParam1 ? dwParam1 : 1);
    case NRS_MFREE: if (dwParam1) { g_freeCount++; free((void*)(INT_PTR)dwParam1); } return 0;
    case NRS_FREE_ARY:
    {
        // dwParam1 是数据类型，dwParam2 才是数组数据指针（宿主按真实语义释放）。
        void* p = (void*)(INT_PTR)dwParam2;
        if (p != NULL)
        {
            int dim = *(int*)p;
            int n = 1;
            for (int i = 0; i < dim; i++) n *= *((int*)p + 1 + i);
            char* e = (char*)p + 4 * (dim + 1);
            DATA_TYPE et = (DATA_TYPE)dwParam1 & ~DT_IS_ARY;
            // 只有元素是指针的类型才逐个释放；DTM_SYS_DATA_TYPE_MASK 是标志位不是掩码，不能拿来比类型。
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

void Fail(const char* fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    printf("[FAIL] "); vprintf(fmt, ap); printf("\n");
    va_end(ap); g_failed++;
}

void Ok(const char* fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    printf("       "); vprintf(fmt, ap); printf("\n");
    va_end(ap);
}

static PLIB_INFO g_inf = NULL;
typedef PLIB_INFO(WINAPI* fn_GetNewInf)(void);

struct Obj
{
    MDATA_INF self;
    INT slot;
    Obj() { memset(this, 0, sizeof(*this)); }
    void Bind() { self.m_dtDataType = SDT_INT; self.m_pCompoundData = (void*)&slot; }
    bool Alive() const { return slot != 0; }
};

static wchar_t g_wStore[8192];
static int g_wUsed = 0;

// 用码点构造宽文本夹具，避开源文件编码歧义（末位写 0 结束）。
static const wchar_t* W(int first, ...)
{
    va_list ap; va_start(ap, first);
    wchar_t* p = g_wStore + g_wUsed;
    int i = 0;
    int c = first;
    while (c != 0 && i < 500) { p[i++] = (wchar_t)c; c = va_arg(ap, int); }
    p[i] = 0;
    g_wUsed += i + 1;
    va_end(ap);
    return p;
}

static void RetBinToUtf8(MDATA_INF* p, char* out, int cap)
{
    out[0] = 0;
    if (p->m_dtDataType != SDT_BIN || p->m_pBin == NULL) return;
    int nBytes = *(int*)(p->m_pBin + 4);
    const wchar_t* w = (const wchar_t*)(p->m_pBin + 8);
    int chars = 0;
    while (chars * 2 < nBytes && w[chars] != 0) chars++;
    char* tmp = (char*)malloc((size_t)chars * 4 + 4);
    int n = WideCharToMultiByte(CP_UTF8, 0, w, chars, tmp, chars * 4 + 3, NULL, NULL);
    if (n > 0) { tmp[n] = 0; lstrcpynA(out, tmp, cap); }
    free(tmp);
}

static MDATA_INF BinWArg(const wchar_t* pw)
{
    int nBytes = (int)(lstrlenW(pw) + 1) * 2;
    MDATA_INF m; memset(&m, 0, sizeof(m));
    m.m_dtDataType = SDT_BIN;
    char* p = (char*)malloc(8 + nBytes);
    *(int*)p = 1; *(int*)(p + 4) = nBytes;
    memcpy(p + 8, pw, nBytes);
    m.m_pBin = (LPBYTE)p;
    return m;
}

static void FreeBinArg(MDATA_INF* p) { if (p->m_pBin) free(p->m_pBin); p->m_pBin = NULL; }
static MDATA_INF IntArg(int v) { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = SDT_INT; m.m_int = v; return m; }
static MDATA_INF TextArg(const char* s) { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = SDT_TEXT; m.m_pText = (char*)s; return m; }
static MDATA_INF BoolArg(bool v) { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = SDT_BOOL; m.m_bool = v ? TRUE : FALSE; return m; }
static MDATA_INF NullArg() { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = _SDT_NULL; return m; }

// 参考数组参数：m_ppAryData 指向「数组数据指针」变量
struct AryVar
{
    void* pData;
    MDATA_INF arg;

    AryVar(DATA_TYPE elemType, int nElements = 0)
    {
        pData = NULL;
        memset(&arg, 0, sizeof(arg));
        arg.m_dtDataType = elemType | DT_IS_ARY;
        arg.m_ppAryData = &pData;
        if (nElements > 0)
        {
            char* p = (char*)malloc(8 + (size_t)nElements * 4);
            *(int*)p = 1; *(int*)(p + 4) = nElements;
            for (int i = 0; i < nElements; i++) *(void**)(p + 8 + i * 4) = NULL;
            pData = p;
        }
    }
    int Count() const
    {
        if (pData == NULL) return 0;
        int dim = *(int*)pData;
        if (dim <= 0) return 0;
        int n = 1;
        for (int i = 0; i < dim; i++) n *= *((int*)pData + 1 + i);
        return n;
    }
    void* Elem(int i) const
    {
        int dim = *(int*)pData;
        return *(void**)((char*)pData + 4 * (dim + 1) + i * 4);
    }
    void ElemToUtf8(int i, char* out, int cap) const
    {
        out[0] = 0;
        void* e = Elem(i);
        if (e == NULL) return;
        MDATA_INF m; memset(&m, 0, sizeof(m));
        m.m_dtDataType = SDT_BIN; m.m_pBin = (LPBYTE)e;
        RetBinToUtf8(&m, out, cap);
    }
    void SetElem(int i, void* p) { int dim = *(int*)pData; *(void**)((char*)pData + 4 * (dim + 1) + i * 4) = p; }
};

static void* MakeElemW(const wchar_t* pw)
{
    int nBytes = (int)(lstrlenW(pw) + 1) * 2;
    char* p = (char*)malloc(8 + nBytes);
    *(int*)p = 1; *(int*)(p + 4) = nBytes;
    memcpy(p + 8, pw, nBytes);
    return p;
}

INT FindGlobalCmd(const char* cnName)
{
    for (int i = 0; i < g_inf->m_nCmdCount; i++)
        if (g_inf->m_pBeginCmdInfo[i].m_shtCategory > 0 &&
            lstrcmpA(AS_TEXT(g_inf->m_pBeginCmdInfo[i].m_szName), cnName) == 0)
            return i;
    Fail("找不到全局命令「%s」", cnName);
    return -1;
}

INT FindCmd(INT typeIdx, const char* cnName)
{
    if (typeIdx < 0 || typeIdx >= g_inf->m_nDataTypeCount) { Fail("类型下标 %d 越界", typeIdx); return -1; }
    PLIB_DATA_TYPE_INFO d = &g_inf->m_pDataType[typeIdx];
    for (int i = 0; i < d->m_nCmdCount; i++)
    {
        INT idx = d->m_pnCmdsIndex[i];
        if (idx >= 0 && idx < g_inf->m_nCmdCount && lstrcmpA(AS_TEXT(g_inf->m_pBeginCmdInfo[idx].m_szName), cnName) == 0)
            return idx;
    }
    Fail("类型 %d 里找不到命令「%s」", typeIdx + 1, cnName);
    return -1;
}

static void FreeRet(MDATA_INF* p)
{
    if (p->m_dtDataType == SDT_TEXT && p->m_pText) free(p->m_pText);
    if (p->m_dtDataType == SDT_BIN && p->m_pBin) free(p->m_pBin);
    if (HIWORD(p->m_dtDataType) == 0 && LOWORD(p->m_dtDataType) >= 4 && p->m_pCompoundData) free(p->m_pCompoundData);
    memset(p, 0, sizeof(MDATA_INF));
}

// 调用约定：CallX(命令号, 参数个数, 参数数组)，返回 ret 由调用方 FreeRet。
// 命令表声明几个参数就按几个调用：测试里少传的那几个补 _SDT_NULL，
// 否则实现按表读 pArgInf[k] 会越界读栈（文本_取中间_批量W 曾这样跑崩）。
static MDATA_INF g_callPad[24];

static void Call(INT cmd, MDATA_INF* a, INT nArgs, MDATA_INF* pRet)
{
    memset(pRet, 0, sizeof(MDATA_INF));
    MDATA_INF* pArgs = a;
    INT nPass = nArgs;
    INT nWant = (cmd >= 0 && cmd < g_inf->m_nCmdCount) ? g_inf->m_pBeginCmdInfo[cmd].m_nArgCount : 0;
    if (nWant > nArgs)
    {
        if (nWant > 24) nWant = 24;
        memcpy(g_callPad, a, (size_t)nArgs * sizeof(MDATA_INF));
        for (int i = nArgs; i < nWant; i++) g_callPad[i] = NullArg();
        pArgs = g_callPad;
        nPass = nWant;
    }
    if (getenv("HOST_TRACE")) printf("[call] idx=%d nArgs=%d/%d name=%s\n", cmd, nArgs, nWant,
        (cmd >= 0 && cmd < g_inf->m_nCmdCount) ? AS_TEXT(g_inf->m_pBeginCmdInfo[cmd].m_szName) : "?");
    g_inf->m_pCmdsFunc[cmd](pRet, nPass, pArgs);
}

// 参考(传址) 字节集参数：m_ppBin 指向「数据指针变量」
static MDATA_INF BinWArgRef(const wchar_t* pw, void** ppSlot)
{
    int nBytes = (int)(lstrlenW(pw) + 1) * 2;
    char* p = (char*)malloc(8 + nBytes);
    *(int*)p = 1; *(int*)(p + 4) = nBytes;
    memcpy(p + 8, pw, nBytes);
    *ppSlot = p;
    MDATA_INF m; memset(&m, 0, sizeof(m));
    m.m_dtDataType = SDT_BIN;
    m.m_ppBin = (LPBYTE*)ppSlot;
    return m;
}

static void SlotToUtf8(void* pSlot, char* out, int cap)
{
    MDATA_INF m; memset(&m, 0, sizeof(m));
    m.m_dtDataType = SDT_BIN; m.m_pBin = (LPBYTE)pSlot;
    RetBinToUtf8(&m, out, cap);
}

// 返回数组的命令（CT_RETRUN_ARY_TYPE_DATA）：块格式 [1][元素数][元素指针...]，元素由易语言接管，宿主按同样规则释放。
// 用独立堆缓冲造宽文本实参：W() 共用一块增长区，同一条语句里调两次会互相踩长度，
// 造出来的字节集块头就比内容长，库侧按块头读会把模式/主题读成脏数据。
static MDATA_INF WB(const char* szUtf8)
{
    int nChars = (szUtf8 != NULL && *szUtf8) ? MultiByteToWideChar(CP_UTF8, 0, szUtf8, -1, NULL, 0) : 1;
    wchar_t* w = (wchar_t*)malloc((size_t)nChars * sizeof(wchar_t));
    if (szUtf8 != NULL && *szUtf8) MultiByteToWideChar(CP_UTF8, 0, szUtf8, -1, w, nChars);
    else w[0] = 0;
    int nBytes = nChars * 2;                       // 含结尾 {0,0}
    MDATA_INF m; memset(&m, 0, sizeof(m));
    m.m_dtDataType = SDT_BIN;
    char* q = (char*)malloc(8 + nBytes);
    *(int*)q = 1; *(int*)(q + 4) = nBytes;
    memcpy(q + 8, w, nBytes);
    free(w);
    m.m_pBin = (LPBYTE)q;
    return m;
}


static void ExpectRetTextAry(MDATA_INF* pRet, const char* const* ppWant, int nWant, const char* what)
{
    char* p = (char*)pRet->m_pAryData;
    if (p == NULL) { Fail("%s 返回数组指针为空", what); return; }
    int dim = *(int*)p;
    int n = *(int*)(p + 4);
    char** ppElem = (char**)(p + 8);
    if (dim != 1 || n != nWant)
    {
        Fail("%s 期望 %d 段，实际 维数%d 元素%d", what, nWant, dim, n);
    }
    else
    {
        for (int i = 0; i < n; i++)
        {
            const char* got = ppElem[i] ? ppElem[i] : "";
            if (strcmp(got, ppWant[i]) != 0) { Fail("%s 第%d段 期望「%s」实际「%s」", what, i + 1, ppWant[i], got); n = -1; break; }
        }
        if (n >= 0) Ok("%s -> %d 段 %s / %s", what, nWant, ppWant[0], ppWant[nWant - 1]);
    }
    for (int i = 0; i < *(int*)(p + 4); i++) if (ppElem[i]) free(ppElem[i]);
    free(p);
    pRet->m_pAryData = NULL;
}

static void CallVoid(INT cmd, MDATA_INF* a, INT nArgs)
{
    MDATA_INF r; Call(cmd, a, nArgs, &r);
    FreeRet(&r);
}

static void ExpectRetText(MDATA_INF* pRet, const char* want, const char* what)
{
    char buf[512];
    RetBinToUtf8(pRet, buf, sizeof(buf));
    if (strcmp(buf, want) != 0) Fail("%s 期望「%s」实际「%s」", what, want, buf);
    else Ok("%s -> %s", what, buf);
}

// ---- 编码转换断言辅助：按原始字节比对（GBK/UTF-8/UTF-16 都可能含非 ASCII）----
static MDATA_INF RawBinArg(const void* pData, int nBytes)
{
    if (nBytes < 0 || pData == NULL) nBytes = 0;   // 夹具造错时别把宿主搞崩
    MDATA_INF m; memset(&m, 0, sizeof(m));
    m.m_dtDataType = SDT_BIN;
    char* p = (char*)malloc(8 + (size_t)nBytes);
    *(int*)p = 1; *(int*)(p + 4) = nBytes;
    if (nBytes > 0) memcpy(p + 8, pData, (size_t)nBytes);
    m.m_pBin = (LPBYTE)p;
    return m;
}

static void RetRaw(MDATA_INF* p, char* out, int cap, int* pnLen)
{
    if (pnLen) *pnLen = 0;
    out[0] = 0;
    int n = 0;
    if (p->m_dtDataType == SDT_TEXT && p->m_pText != NULL) n = (int)lstrlenA(p->m_pText);
    else if (p->m_dtDataType == SDT_BIN && p->m_pBin != NULL) n = *(int*)(p->m_pBin + 4);
    if (n > cap - 1) n = cap - 1;
    if (n > 0)
    {
        const char* src = (p->m_dtDataType == SDT_TEXT) ? p->m_pText : (const char*)(p->m_pBin + 8);
        memcpy(out, src, n);
    }
    out[n] = 0;
    if (pnLen) *pnLen = n;
}

static void ToHex(const char* p, int n, char* out, int cap)
{
    static const char hx[] = "0123456789ABCDEF";
    int j = 0;
    for (int i = 0; i < n && j + 4 < cap; i++)
    {
        unsigned char b = (unsigned char)p[i];
        out[j++] = ' '; out[j++] = '%'; out[j++] = hx[b >> 4]; out[j++] = hx[b & 15];
    }
    out[j] = 0;
}

static void ExpectRetBytes(MDATA_INF* pRet, const void* pWant, int nWant, const char* what)
{
    char got[1024], a[512], b[512];
    int nGot = 0;
    RetRaw(pRet, got, sizeof(got), &nGot);
    if (nGot != nWant || (nWant > 0 && memcmp(got, pWant, nWant) != 0))
    {
        ToHex(got, nGot > 40 ? 40 : nGot, a, sizeof(a));
        ToHex((const char*)pWant, nWant > 40 ? 40 : nWant, b, sizeof(b));
        Fail("%s 字节不符 实际(%d)=%s 期望(%d)=%s", what, nGot, a, nWant, b);
    }
    else Ok("%s -> %d 字节", what, nGot);
}

// 返回文本型（SDT_TEXT）的命令不能用 ExpectRetText，那条路只认宽文本字节集。
static void ExpectRetAscii(MDATA_INF* pRet, const char* want, const char* what)
{
    char got[1024], a[512];
    int nGot = 0;
    RetRaw(pRet, got, sizeof(got), &nGot);
    int nWant = (int)lstrlenA(want);
    if (nGot != nWant || (nWant > 0 && memcmp(got, want, nWant) != 0))
    {
        ToHex(got, nGot > 40 ? 40 : nGot, a, sizeof(a));
        Fail("%s 期望「%s」实际(%d)=%s", what, want, nGot, a);
    }
    else Ok("%s -> %s", what, got);
}

static void ExpectRetWide(MDATA_INF* pRet, const wchar_t* pWant, int nChars, const char* what)
{
    ExpectRetBytes(pRet, (const void*)pWant, nChars * 2, what);
}

// 期望值直接用 Win32 算，不复用库自身的转换，避免自证。
static int ToCp(const wchar_t* pW, int nChars, UINT uCP, char* out, int cap)
{
    int n = WideCharToMultiByte(uCP, 0, pW, nChars, out, cap, NULL, NULL);
    if (n < 0) n = 0;
    if (n < cap) out[n] = 0;
    return n;
}

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);     // 崩溃时也能看到最后一条输出
    printf("[host_uni] start\n");
    char dllPath[MAX_PATH] = { 0 };
    if (argc > 1) lstrcpynA(dllPath, argv[1], MAX_PATH);
    else lstrcpynA(dllPath, "out\\cxsoft.fne", MAX_PATH);

    AddVectoredExceptionHandler(1, Veh);
    HMODULE h = LoadLibraryA(dllPath);
    if (h == NULL) { printf("[FAIL] 加载 %s 失败，错误码 %lu\n", dllPath, GetLastError()); return 1; }
    fn_GetNewInf pGetNewInf = (fn_GetNewInf)GetProcAddress(h, "GetNewInf");
    if (!pGetNewInf) { printf("[FAIL] 找不到 GetNewInf\n"); return 1; }
    g_inf = pGetNewInf();
    g_hMod = h;
    if (!g_inf) { printf("[FAIL] GetNewInf 返回空\n"); return 1; }
    g_inf->m_pfnNotify(NL_SYS_NOTIFY_FUNCTION, (DWORD)(INT_PTR)&FakeNotifySys, 0);

    printf("命令 %d 条 / 类 %d 个 / 类别 %d 个\n", g_inf->m_nCmdCount, g_inf->m_nDataTypeCount, g_inf->m_nCategoryCount);
    if (g_inf->m_nDataTypeCount != 25) Fail("类型数量应为 25（包含取消令牌），实际 %d", g_inf->m_nDataTypeCount);

    // 宽文本命令的数量：全局 75 条（文本处理 55 + 扩展 19 + 分割文本W_数组，弃用未迁 寻找文本W_旧版）
    int nCatWide = 0, nCatReg = 0, nCatConv = 0;
    for (int i = 0; i < g_inf->m_nCmdCount; i++)
    {
        short cat = g_inf->m_pBeginCmdInfo[i].m_shtCategory;
        if (cat == 3) nCatWide++;
        if (cat == 4) nCatReg++;
        if (cat == 5) nCatConv++;
    }
    printf("类别3(宽文本) 全局命令 %d 条，类别4(正则) 全局命令 %d 条，类别5(编码转换) %d 条\n", nCatWide, nCatReg, nCatConv);
    if (nCatWide != 75) Fail("宽文本全局命令应为 75 条，实际 %d", nCatWide);
    if (nCatReg != 0) Fail("正则命令都做成成员命令，类别 4 不应有全局命令");
    if (nCatConv != 20) Fail("编码转换全局命令应为 20 条，实际 %d", nCatConv);

    char text[512];

    printf("\n---- 宽文本基础命令 ----\n");
    {
        MDATA_INF a[1] = { BinWArg(W(0x4E2D, 0x6587, 'a', 'b', 'c', 0)) };
        MDATA_INF r; Call(FindGlobalCmd("取文本长度W"), a, 1, &r);
        if (r.m_int != 5) Fail("取文本长度W 应为 5，实际 %d", r.m_int); else Ok("取文本长度W -> 5");
        FreeRet(&r); FreeBinArg(&a[0]);
    }
    {
        MDATA_INF a[4] = { BinWArg(W('1','2','3','4','5','6','7','8','9','0',0)), BinWArg(W('7','8','9',0)), NullArg(), NullArg() };
        MDATA_INF r; Call(FindGlobalCmd("寻找文本W"), a, 4, &r);
        if (r.m_int != 7) Fail("寻找文本W 应为 7，实际 %d", r.m_int); else Ok("寻找文本W -> %d", r.m_int);
        FreeRet(&r); for (int i = 0; i < 2; i++) FreeBinArg(&a[i]);
    }
    {
        MDATA_INF a[4] = { BinWArg(W('a','X','b','X','c',0)), BinWArg(W('X',0)), NullArg(), NullArg() };
        MDATA_INF r; Call(FindGlobalCmd("倒找文本W"), a, 4, &r);
        if (r.m_int != 4) Fail("倒找文本W 应为 4，实际 %d", r.m_int); else Ok("倒找文本W -> %d", r.m_int);
        FreeRet(&r); for (int i = 0; i < 2; i++) FreeBinArg(&a[i]);
    }
    {
        MDATA_INF a[3] = { BinWArg(W('1','2','3','4','5',0)), IntArg(2), IntArg(3) };
        MDATA_INF r; Call(FindGlobalCmd("取文本中间W"), a, 3, &r);
        ExpectRetText(&r, "234", "取文本中间W");
        FreeRet(&r); FreeBinArg(&a[0]);
    }
    {
        MDATA_INF a[6] = { BinWArg(W('1','2','3','4','5',0)), BinWArg(W('2','3',0)), BinWArg(W('A','B',0)), NullArg(), NullArg(), NullArg() };
        MDATA_INF r; Call(FindGlobalCmd("子文本替换W"), a, 6, &r);
        ExpectRetText(&r, "1AB45", "子文本替换W");
        FreeRet(&r); for (int i = 0; i < 3; i++) FreeBinArg(&a[i]);
    }
    {
        MDATA_INF a[6] = { BinWArg(W('1','2','3','2','3',0)), TextArg("23"), TextArg("AB"), NullArg(), IntArg(1), BoolArg(true) };
        MDATA_INF r; Call(FindGlobalCmd("子文本替换2W"), a, 6, &r);
        ExpectRetText(&r, "1AB23", "子文本替换2W(替换1次)");
        FreeRet(&r); FreeBinArg(&a[0]);   // a[1]/a[2] 是静态文本字面量，不能 free
    }
    {
        MDATA_INF a[1] = { BinWArg(W('a','B','c',0)) };
        MDATA_INF r; Call(FindGlobalCmd("到大写W"), a, 1, &r);
        ExpectRetText(&r, "ABC", "到大写W");
        FreeRet(&r); FreeBinArg(&a[0]);
    }
    {
        MDATA_INF a[1] = { BinWArg(W(' ',' ','a','b',' ',0)) };
        MDATA_INF r; Call(FindGlobalCmd("删首尾空W"), a, 1, &r);
        ExpectRetText(&r, "ab", "删首尾空W");
        FreeRet(&r); FreeBinArg(&a[0]);
    }
    {
        MDATA_INF a[1] = { BinWArg(W('a','b',0)) };
        MDATA_INF r; Call(FindGlobalCmd("到全角W"), a, 1, &r);
        if (r.m_pBin == NULL) Fail("到全角W 返回空");
        else
        {
            const wchar_t* w = (const wchar_t*)(r.m_pBin + 8);
            if (w[0] != 0xFF41 || w[1] != 0xFF42) Fail("到全角W 码点不对：%X %X", w[0], w[1]);
            else Ok("到全角W -> U+%04X U+%04X", w[0], w[1]);
            MDATA_INF b[1] = { BinWArg(w) };
            MDATA_INF r2; Call(FindGlobalCmd("到半角W"), b, 1, &r2);
            ExpectRetText(&r2, "ab", "到半角W(还原)");
            FreeRet(&r2); FreeBinArg(&b[0]);
        }
        FreeRet(&r); FreeBinArg(&a[0]);
    }
    {
        MDATA_INF a[2] = { BinWArg(W('1',0)), BinWArg(W('2',0)) };
        MDATA_INF r; Call(FindGlobalCmd("文本连接W"), a, 2, &r);
        ExpectRetText(&r, "12", "文本连接W");
        FreeRet(&r); FreeBinArg(&a[0]); FreeBinArg(&a[1]);
    }
    {
        MDATA_INF a[3] = { BinWArg(W('a','b','c',0)), BinWArg(W('a','b','c',0)), NullArg() };
        MDATA_INF r; Call(FindGlobalCmd("文本比较W"), a, 3, &r);
        if (r.m_int != 0) Fail("文本比较W 应为 0，实际 %d", r.m_int); else Ok("文本比较W -> 0");
        FreeRet(&r); FreeBinArg(&a[0]); FreeBinArg(&a[1]);
    }
    {
        MDATA_INF a[1] = { IntArg('中') };
        MDATA_INF r; Call(FindGlobalCmd("整数到文本W"), a, 1, &r);
        if (r.m_pBin == NULL || *(int*)(r.m_pBin + 4) < 4) Fail("整数到文本W 没有产出宽文本");
        else Ok("整数到文本W(0x4E2D) -> %d 字节", *(int*)(r.m_pBin + 4));
        FreeRet(&r);
    }
    {
        MDATA_INF a[2] = { BinWArg(W('0','x','1','F',0)), BoolArg(true) };
        MDATA_INF r; Call(FindGlobalCmd("文本到整数W"), a, 2, &r);
        if (r.m_int != 31) Fail("文本到整数W(0x1F) 应为 31，实际 %d", r.m_int); else Ok("文本到整数W(0x1F) -> 31");
        FreeRet(&r); FreeBinArg(&a[0]);
    }
    {
        MDATA_INF a[1] = { BinWArg(W('x','y','z',0)) };
        MDATA_INF r; Call(FindGlobalCmd("取文本指针W"), a, 1, &r);
        INT ptr = r.m_int;
        MDATA_INF b[1] = { IntArg((int)ptr) };
        MDATA_INF r2; Call(FindGlobalCmd("指针到文本W"), b, 1, &r2);
        ExpectRetText(&r2, "xyz", "取文本指针W/指针到文本W 往返");
        FreeRet(&r); FreeRet(&r2); FreeBinArg(&a[0]);
    }
    {
        MDATA_INF d; memset(&d, 0, sizeof(d)); d.m_dtDataType = SDT_DOUBLE; d.m_double = 1.25;
        MDATA_INF r; Call(FindGlobalCmd("数值到文本W"), &d, 1, &r);
        ExpectRetText(&r, "1.25", "数值到文本W(1.25)");
        FreeRet(&r);
        MDATA_INF a[1] = { BinWArg(W('2','.','5',0)) };
        MDATA_INF r2; Call(FindGlobalCmd("到数值W"), a, 1, &r2);
        if (r2.m_double != 2.5) Fail("到数值W 应为 2.5，实际 %g", r2.m_double); else Ok("到数值W(2.5) -> %g", r2.m_double);
        FreeRet(&r2); FreeBinArg(&a[0]);
    }
    {
        MDATA_INF a[2] = { BinWArg(W('a','b',0)), BinWArg(W('a','b',0)) };
        MDATA_INF r; Call(FindGlobalCmd("宽文本_是否相等"), a, 2, &r);
        if (!r.m_bool) Fail("宽文本_是否相等 应为真"); else Ok("宽文本_是否相等 -> 真");
        FreeRet(&r); FreeBinArg(&a[0]); FreeBinArg(&a[1]);
    }
    {
        MDATA_INF a[2] = { BinWArg(W('a','b',0)), BinWArg(W('a','b',0)) };
        MDATA_INF r; Call(FindGlobalCmd("文本比较W"), a, 2, &r);
        FreeRet(&r); FreeBinArg(&a[0]); FreeBinArg(&a[1]);
        MDATA_INF b[1] = { BinWArg(W(0)) };
        MDATA_INF r2; Call(FindGlobalCmd("宽文本_是否为空"), b, 1, &r2);
        if (!r2.m_bool) Fail("宽文本_是否为空({0,0}) 应为真"); else Ok("宽文本_是否为空 -> 真");
        FreeRet(&r2); FreeBinArg(&b[0]);
    }
    {   // 读写文件
        MDATA_INF name = BinWArg(W('o','u','t','\\','h','o','s','t','_','w','t','.','b','i','n',0));
        MDATA_INF data; memset(&data, 0, sizeof(data));
        data.m_dtDataType = SDT_BIN;
        char* blk = (char*)malloc(8 + 6);
        *(int*)blk = 1; *(int*)(blk + 4) = 6;
        memcpy(blk + 8, "abcdef", 6);
        data.m_pBin = (LPBYTE)blk;
        MDATA_INF a[2] = { name, data };
        MDATA_INF r; Call(FindGlobalCmd("写到文件W"), a, 2, &r);
        if (!r.m_bool) Fail("写到文件W 返回假"); else Ok("写到文件W -> 真");
        FreeRet(&r);
        MDATA_INF r2; Call(FindGlobalCmd("读入文件W"), &name, 1, &r2);
        if (r2.m_pBin == NULL || *(int*)(r2.m_pBin + 4) != 6) Fail("读入文件W 长度不对");
        else Ok("读入文件W -> %d 字节", *(int*)(r2.m_pBin + 4));
        FreeRet(&r2); FreeBinArg(&name); free(blk);
        DeleteFileA("out\\host_wt.bin");
    }

    printf("\n---- 分割 / 批量 / 数组 ----\n");
    {
        AryVar ary(SDT_BIN);
        MDATA_INF a[3] = { BinWArg(W('a',',','b',',',',','c',0)), BinWArg(W(',',0)), ary.arg };
        MDATA_INF r; Call(FindGlobalCmd("分割文本W"), a, 3, &r);
        if (r.m_int != 4 || ary.Count() != 4) Fail("分割文本W 应 4 段，实际 返回%d 数组%d", r.m_int, ary.Count());
        else
        {
            char one[64];
            ary.ElemToUtf8(0, one, sizeof(one)); if (strcmp(one, "a") != 0) Fail("分割1=%s", one);
            ary.ElemToUtf8(1, one, sizeof(one)); if (strcmp(one, "b") != 0) Fail("分割2=%s", one);
            ary.ElemToUtf8(2, one, sizeof(one)); if (one[0] != 0) Fail("分割3 应为空=%s", one);
            ary.ElemToUtf8(3, one, sizeof(one)); if (strcmp(one, "c") != 0) Fail("分割4=%s", one);
            Ok("分割文本W -> 4 段 a/b//c");
        }
        FreeRet(&r); FreeBinArg(&a[0]); FreeBinArg(&a[1]);
    }
    {
        // 分割文本W_数组：宽文本进，直接返回 UTF-8 文本型数组，不用先声明数组变量
        MDATA_INF a[3] = { BinWArg(W(0x4E2D, ',', 0x6587, ',', 'x', 0)), BinWArg(W(',', 0)), BoolArg(false) };
        MDATA_INF r; Call(FindGlobalCmd("分割文本W_数组"), a, 3, &r);
        const char* want3[3] = { "\xE4\xB8\xAD", "\xE6\x96\x87", "x" };
        ExpectRetTextAry(&r, want3, 3, "分割文本W_数组");
        FreeBinArg(&a[0]); FreeBinArg(&a[1]);
    }
    {
        MDATA_INF a[3] = { BinWArg(W('a', ',', ',', 'b', 0)), BinWArg(W(',', 0)), BoolArg(false) };
        MDATA_INF r; Call(FindGlobalCmd("分割文本W_数组"), a, 3, &r);
        const char* want[3] = { "a", "", "b" };
        ExpectRetTextAry(&r, want, 3, "分割文本W_数组(保留空段)");
        FreeBinArg(&a[0]); FreeBinArg(&a[1]);
        MDATA_INF a2[3] = { BinWArg(W('a', ',', ',', 'b', 0)), BinWArg(W(',', 0)), BoolArg(true) };
        MDATA_INF r2; Call(FindGlobalCmd("分割文本W_数组"), a2, 3, &r2);
        const char* want2[2] = { "a", "b" };
        ExpectRetTextAry(&r2, want2, 2, "分割文本W_数组(移除空段)");
        FreeBinArg(&a2[0]); FreeBinArg(&a2[1]);
    }
    {
        AryVar ary(SDT_BIN);
        MDATA_INF a[5] = { BinWArg(W('<','b','>','x','<','/','b','>','<','b','>','y','<','/','b','>',0)),
                           BinWArg(W('<','b','>',0)), BinWArg(W('<','/','b','>',0)), ary.arg, BoolArg(true) };
        MDATA_INF r; Call(FindGlobalCmd("文本_取中间_批量W"), a, 5, &r);
        if (r.m_int != 2 || ary.Count() != 2) Fail("批量取中间 应 2 项，实际 返回%d 数组%d", r.m_int, ary.Count());
        else
        {
            char one[64];
            ary.ElemToUtf8(0, one, sizeof(one)); if (strcmp(one, "x") != 0) Fail("批量1=%s", one);
            ary.ElemToUtf8(1, one, sizeof(one)); if (strcmp(one, "y") != 0) Fail("批量2=%s", one);
            Ok("文本_取中间_批量W -> x / y");
        }
        FreeRet(&r); for (int i = 0; i < 3; i++) FreeBinArg(&a[i]);
    }
    {
        AryVar ary(SDT_BIN, 3);
        ary.SetElem(0, MakeElemW(W('a','b',0)));
        ary.SetElem(1, MakeElemW(W('A','B',0)));
        ary.SetElem(2, MakeElemW(W('c',0)));
        MDATA_INF a[2] = { ary.arg, BoolArg(true) };
        MDATA_INF r; Call(FindGlobalCmd("数组_去重复W"), a, 2, &r);
        if (r.m_int != 2 || ary.Count() != 2) Fail("数组_去重复W 应剩 2，实际 %d/%d", r.m_int, ary.Count());
        else Ok("数组_去重复W(不区分) -> 2");
        FreeRet(&r);
        MDATA_INF b[2] = { ary.arg, BinWArg(W('|',0)) };
        MDATA_INF r2; Call(FindGlobalCmd("数组_输出文本W"), b, 2, &r2);
        ExpectRetText(&r2, "ab|c", "数组_输出文本W");
        FreeRet(&r2); FreeBinArg(&b[1]);
    }

    printf("\n---- 正则全局命令 ----\n");
    {
        MDATA_INF a[6] = { BinWArg(W('a','b','c','1','2','3','d','e','f',0)), BinWArg(W('[','0','-','9',']','+',0)),
                           NullArg(), NullArg(), NullArg(), NullArg() };
        MDATA_INF r; Call(FindGlobalCmd("文本_取文本_正则W"), a, 6, &r);
        ExpectRetText(&r, "123", "文本_取文本_正则W");
        FreeRet(&r); FreeBinArg(&a[0]); FreeBinArg(&a[1]);
    }
    {
        MDATA_INF a[1] = { BinWArg(W('a','b','1','2','c','d','3','4',0)) };
        MDATA_INF r; Call(FindGlobalCmd("文本_只取数字W"), a, 1, &r);
        ExpectRetText(&r, "12", "文本_只取数字W");
        FreeRet(&r); FreeBinArg(&a[0]);
    }
    {
        AryVar ary(SDT_BIN);
        MDATA_INF a[7] = { BinWArg(W('a','1','b','2',0)), BinWArg(W('(','[','0','-','9',']',')',0)), IntArg(1),
                           IntArg(0), NullArg(), NullArg(), ary.arg };
        MDATA_INF r; Call(FindGlobalCmd("文本_取文本_正则_取全部子匹配W"), a, 7, &r);
        ExpectRetText(&r, "12", "正则取全部子匹配");
        if (ary.Count() != 2) Fail("子匹配数组应 2 项，实际 %d", ary.Count());
        FreeRet(&r); FreeBinArg(&a[0]); FreeBinArg(&a[1]);
    }
    {
        MDATA_INF a[4] = { BinWArg(W('<','d','i','v','>','<','p','>','1','<','/','p','>','<','/','d','i','v','>','z','z',0)),
                           BinWArg(W('<','d','i','v','>',0)), NullArg(), NullArg() };
        MDATA_INF r; Call(FindGlobalCmd("文本_HTML取中间W"), a, 4, &r);
        ExpectRetText(&r, "<p>1</p>", "文本_HTML取中间W");
        FreeRet(&r); FreeBinArg(&a[0]); FreeBinArg(&a[1]);
    }
    {   // 参考替换：源文本被就地改写（传址参数必须给变量地址）
        void* pSlot = NULL;
        MDATA_INF a[6] = { BinWArgRef(W('a','1','b','2',0), &pSlot), BinWArg(W('[','0','-','9',']',0)),
                           BinWArg(W('X',0)), NullArg(), NullArg(), BoolArg(true) };
        MDATA_INF r; Call(FindGlobalCmd("文本_替换_正则W"), a, 6, &r);
        Ok("文本_替换_正则W -> 次数 %d", r.m_int);
        if (r.m_int < 2) Fail("替换次数应 >=2，实际 %d", r.m_int);
        char buf[256];
        SlotToUtf8(pSlot, buf, sizeof(buf));
        if (strcmp(buf, "aXbX") != 0) Fail("源文本未就地替换：%s", buf);
        else Ok("源文本 -> %s", buf);
        FreeRet(&r);
        for (int i = 1; i < 3; i++) FreeBinArg(&a[i]);
        if (pSlot) free(pSlot);
    }
    {   // 格式化宽字符W2：补齐末尾 {0,0} 并写回
        void* pSlot = NULL;
        MDATA_INF a[1] = { BinWArgRef(W('a','b',0), &pSlot) };
        // 去掉末尾终止符，模拟不带 {0,0} 的宽文本
        char* blk = (char*)pSlot;
        *(int*)(blk + 4) -= 2;
        MDATA_INF r; Call(FindGlobalCmd("格式化宽字符W2"), a, 1, &r);
        char buf[64];
        SlotToUtf8(*a[0].m_ppBin, buf, sizeof(buf));
        if (strcmp(buf, "ab") != 0) Fail("格式化宽字符W2 结果：%s", buf);
        else Ok("格式化宽字符W2 -> %s，字节数 %d", buf, *(int*)((char*)*a[0].m_ppBin + 4));
        FreeRet(&r);
        if (*a[0].m_ppBin) free(*a[0].m_ppBin);
        UNREFERENCED_PARAMETER(pSlot);
    }

    printf("\n---- 宽文本类 ----\n");
    {
        Obj w; w.Bind();
        CallVoid(FindCmd(3, "宽文本类构造函数"), &w.self, 1);
        { MDATA_INF a[2] = { w.self, NullArg() }; MDATA_INF r; Call(FindCmd(3, "创建"), a, 2, &r);
          if (!r.m_bool) Fail("宽文本.创建 返回假"); else Ok("宽文本.创建 -> 真"); FreeRet(&r); }
        { MDATA_INF a[2] = { w.self, TextArg("hi") }; MDATA_INF r; Call(FindCmd(3, "添加"), a, 2, &r);
          if (!r.m_bool) Fail("宽文本.添加 返回假"); FreeRet(&r); }
        { MDATA_INF a[2] = { w.self, BinWArg(W('A','B',0)) }; MDATA_INF r; Call(FindCmd(3, "添加W"), a, 2, &r);
          FreeRet(&r); FreeBinArg(&a[1]); }
        { MDATA_INF a[2] = { w.self, IntArg(7) }; MDATA_INF r; Call(FindCmd(3, "添加整数"), a, 2, &r);
          FreeRet(&r); }
        { MDATA_INF a[1] = { w.self }; MDATA_INF r; Call(FindCmd(3, "取文本W"), a, 1, &r);
          ExpectRetText(&r, "hiAB7", "宽文本 取文本W"); FreeRet(&r); }
        { MDATA_INF a[1] = { w.self }; MDATA_INF r; Call(FindCmd(3, "取文本长度"), a, 1, &r);
          if (r.m_int != 5) Fail("宽文本 取文本长度 应为 5，实际 %d", r.m_int); else Ok("宽文本 取文本长度 -> 5"); FreeRet(&r); }
        { MDATA_INF a[3] = { w.self, BinWArg(W('A','B',0)), IntArg(0) }; MDATA_INF r; Call(FindCmd(3, "寻找W"), a, 3, &r);
          if (r.m_int != 2) Fail("宽文本 寻找W 应为 2，实际 %d", r.m_int); else Ok("宽文本 寻找W -> 2");
          FreeRet(&r); FreeBinArg(&a[1]); }
        { MDATA_INF a[6] = { w.self, BinWArg(W('A','B',0)), BinWArg(W('C','D',0)), NullArg(), NullArg(), NullArg() };
          MDATA_INF r; Call(FindCmd(3, "子替换W"), a, 6, &r);
          if (!r.m_bool) Fail("宽文本 子替换W 返回假"); FreeRet(&r);
          for (int i = 1; i <= 2; i++) FreeBinArg(&a[i]); }
        { MDATA_INF a[1] = { w.self }; MDATA_INF r; Call(FindCmd(3, "取文本"), a, 1, &r);
          if (!r.m_pText || strcmp(r.m_pText, "hiCD7") != 0) Fail("宽文本 子替换后内容不对：%s", r.m_pText);
          else Ok("宽文本 子替换后 取文本 -> %s", r.m_pText);
          FreeRet(&r); }
        { MDATA_INF a[3] = { w.self, IntArg(1), IntArg(2) }; MDATA_INF r; Call(FindCmd(3, "取中间"), a, 3, &r);
          if (r.m_pCompoundData == NULL) Fail("宽文本 取中间 没返回对象");
          else
          {
              Obj mid; mid.Bind(); mid.slot = *(INT*)r.m_pCompoundData;
              MDATA_INF b[1] = { mid.self }; MDATA_INF r2; Call(FindCmd(3, "取文本W"), b, 1, &r2);
              ExpectRetText(&r2, "iC", "宽文本 取中间(1,2)");
              FreeRet(&r2);
              MDATA_INF d[1] = { mid.self }; MDATA_INF r3; Call(FindCmd(3, "宽文本类析构函数"), d, 1, &r3);
              FreeRet(&r3);
          }
          FreeRet(&r); }
        { AryVar ary(SDT_BIN);
          Obj w2; w2.Bind();
          CallVoid(FindCmd(3, "宽文本类构造函数"), &w2.self, 1);
          { MDATA_INF a[2] = { w2.self, NullArg() }; MDATA_INF r; Call(FindCmd(3, "创建"), a, 2, &r); FreeRet(&r); }
          { MDATA_INF a[2] = { w2.self, BinWArg(W('x',':',':','y',0)) }; MDATA_INF r; Call(FindCmd(3, "添加W"), a, 2, &r);
            FreeRet(&r); FreeBinArg(&a[1]); }
          { MDATA_INF a[4] = { w2.self, BinWArg(W(':',0)), ary.arg, BoolArg(false) }; MDATA_INF r; Call(FindCmd(3, "分割文本W"), a, 4, &r);
            if (r.m_int != 3 || ary.Count() != 3) Fail("宽文本 分割文本W 应 3 段，实际 %d/%d", r.m_int, ary.Count());
            else Ok("宽文本 分割文本W -> 3 段");
            FreeRet(&r); FreeBinArg(&a[1]); }
          CallVoid(FindCmd(3, "宽文本类析构函数"), &w2.self, 1); }
        {   // 复制函数应给出独立内容
          Obj wc; wc.Bind();
          MDATA_INF a[2] = { wc.self, w.self }; MDATA_INF r;
          Call(FindCmd(3, "宽文本类复制函数"), a, 2, &r);
          FreeRet(&r);
          if (!wc.Alive() || wc.slot == w.slot) Fail("宽文本 复制没有得到独立对象");
          else
          {
              MDATA_INF b[1] = { wc.self }; MDATA_INF r2; Call(FindCmd(3, "取文本W"), b, 1, &r2);
              ExpectRetText(&r2, "hiCD7", "宽文本 复制后内容");
              FreeRet(&r2);
          }
          CallVoid(FindCmd(3, "宽文本类析构函数"), &wc.self, 1); }
        CallVoid(FindCmd(3, "宽文本类析构函数"), &w.self, 1);
        if (w.Alive()) Fail("宽文本 析构后数据槽没清零");
    }

    printf("\n---- 正则四个类 ----\n");
    {
        Obj rx; rx.Bind();
        CallVoid(FindCmd(4, "类_正则表达式类W构造函数"), &rx.self, 1);
        { MDATA_INF a[7] = { rx.self, BinWArg(W('[','0','-','9',']','+',0)), BinWArg(W('a','b','1','2','3','c','4','5','6',0)),
                             BoolArg(true), NullArg(), NullArg(), NullArg() };
          MDATA_INF r; Call(FindCmd(4, "创建"), a, 7, &r);
          if (!r.m_bool) Fail("正则W.创建 返回假"); else Ok("正则W.创建 -> 真");
          FreeRet(&r); FreeBinArg(&a[1]); FreeBinArg(&a[2]); }
        { MDATA_INF r; Call(FindCmd(4, "取匹配数量"), &rx.self, 1, &r);
          if (r.m_int != 2) Fail("正则W 匹配数量应为 2，实际 %d", r.m_int); else Ok("正则W 取匹配数量 -> %d", r.m_int);
          FreeRet(&r); }
        { int pos = -99;
          MDATA_INF a[3] = { rx.self, IntArg(1), NullArg() };
          a[2].m_dtDataType = SDT_INT; a[2].m_pInt = &pos;
          MDATA_INF r; Call(FindCmd(4, "取匹配文本W"), a, 3, &r);
          ExpectRetText(&r, "123", "正则W 取匹配文本W(1)");
          if (pos != 2) Fail("正则W 起始位置写回应为 2，实际 %d", pos); else Ok("正则W 起始位置 -> %d", pos);
          FreeRet(&r); }
        { MDATA_INF a[2] = { rx.self, BinWArg(W('X',0)) }; MDATA_INF r; Call(FindCmd(4, "替换"), a, 2, &r);
          ExpectRetText(&r, "abXcX", "正则W 替换");
          FreeRet(&r); FreeBinArg(&a[1]); }
        CallVoid(FindCmd(4, "类_正则表达式类W析构函数"), &rx.self, 1);
    }
    {
        Obj rx; rx.Bind();
        CallVoid(FindCmd(5, "类_正则表达式_兼容版W构造函数"), &rx.self, 1);
        { MDATA_INF a[6] = { rx.self, BinWArg(W('(','a',')','(','b',')',0)), BinWArg(W('a','b','a','b',0)),
                             NullArg(), NullArg(), NullArg() };
          MDATA_INF r; Call(FindCmd(5, "创建W"), a, 6, &r);
          if (!r.m_bool) Fail("兼容版W.创建W 返回假"); else Ok("兼容版W.创建W -> 真");
          FreeRet(&r); FreeBinArg(&a[1]); FreeBinArg(&a[2]); }
        { MDATA_INF r; Call(FindCmd(5, "取匹配数量"), &rx.self, 1, &r);
          if (r.m_int != 2) Fail("兼容版W 匹配数量应为 2，实际 %d", r.m_int); else Ok("兼容版W 取匹配数量 -> %d", r.m_int);
          FreeRet(&r); }
        { MDATA_INF a[4] = { rx.self, IntArg(1), IntArg(2), NullArg() };
          MDATA_INF r; Call(FindCmd(5, "取子匹配文本W"), a, 4, &r);
          ExpectRetText(&r, "b", "兼容版W 取子匹配文本W(1,2)");
          FreeRet(&r); }
        { MDATA_INF a[1] = { rx.self }; MDATA_INF r; Call(FindCmd(5, "取正则文本"), a, 1, &r);
          Ok("兼容版W 取正则文本 -> %s", r.m_pText ? r.m_pText : "(空)");
          FreeRet(&r); }
        CallVoid(FindCmd(5, "类_正则表达式_兼容版W析构函数"), &rx.self, 1);
    }
    {
        Obj rx; rx.Bind();
        CallVoid(FindCmd(6, "类_正则表达式_兼容版2构造函数"), &rx.self, 1);
        { MDATA_INF a[5] = { rx.self, BinWArg(W('(','[','0','-','9',']','+',')',0)), BinWArg(W('a','1','b','2','2',0)),
                             NullArg(), NullArg() };
          MDATA_INF r; Call(FindCmd(6, "创建W"), a, 5, &r);
          FreeRet(&r); FreeBinArg(&a[1]); FreeBinArg(&a[2]); }
        { MDATA_INF r; Call(FindCmd(6, "取匹配数量"), &rx.self, 1, &r);
          if (r.m_int != 2) Fail("兼容版2 匹配数量应为 2，实际 %d", r.m_int); else Ok("兼容版2 取匹配数量 -> %d", r.m_int);
          FreeRet(&r); }
        { MDATA_INF a[3] = { rx.self, IntArg(2), IntArg(1) };
          MDATA_INF r; Call(FindCmd(6, "取子匹配文本"), a, 3, &r);
          if (!r.m_pText || strcmp(r.m_pText, "22") != 0) Fail("兼容版2 子匹配(2,1)=%s", r.m_pText);
          else Ok("兼容版2 取子匹配文本(2,1) -> %s", r.m_pText);
          FreeRet(&r); }
        CallVoid(FindCmd(6, "类_正则表达式_兼容版2析构函数"), &rx.self, 1);
    }
    {
        Obj rx; rx.Bind();
        CallVoid(FindCmd(7, "类_正则表达式_简单版W构造函数"), &rx.self, 1);
        { MDATA_INF a[4] = { rx.self, BinWArg(W('[','a','-','z',']','+',0)), BoolArg(true), BoolArg(true) };
          MDATA_INF r; Call(FindCmd(7, "创建W"), a, 4, &r);
          if (!r.m_bool) Fail("简单版W.创建W 返回假"); else Ok("简单版W.创建W -> 真");
          FreeRet(&r); FreeBinArg(&a[1]); }
        { MDATA_INF a[2] = { rx.self, BinWArg(W('a','b','1','c','d','2',0)) };
          MDATA_INF r; Call(FindCmd(7, "搜索全部W"), a, 2, &r);
          if (r.m_int != 2) Fail("简单版W 搜索全部应为 2，实际 %d", r.m_int); else Ok("简单版W 搜索全部W -> %d", r.m_int);
          FreeRet(&r); FreeBinArg(&a[1]); }
        { MDATA_INF a[2] = { rx.self, IntArg(2) }; MDATA_INF r; Call(FindCmd(7, "取匹配文本W"), a, 2, &r);
          ExpectRetText(&r, "cd", "简单版W 取匹配文本W(2)");
          FreeRet(&r); }
        CallVoid(FindCmd(7, "类_正则表达式_简单版W析构函数"), &rx.self, 1);
    }

    // ==================== 编码转换（23 条）====================
    printf("  -- 编码转换 --\n");
    {   // W / A 往返（按 65001 走，库内文本口径）
        const wchar_t* src = W(0x4E2D, 0x6587, 'a', 'b', 'c', 0);
        char u8[64];
        int nU8 = ToCp(src, 5, CP_UTF8, u8, sizeof(u8));
        MDATA_INF a[2] = { TextArg(u8), IntArg(CP_UTF8) };
        MDATA_INF r; Call(FindGlobalCmd("W"), a, 2, &r);
        ExpectRetWide(&r, src, 6, "W(65001)");
        MDATA_INF a2[2] = { r, IntArg(CP_UTF8) };
        MDATA_INF r2; Call(FindGlobalCmd("A"), a2, 2, &r2);
        ExpectRetBytes(&r2, u8, nU8, "A(65001) 还原");
        FreeRet(&r2); FreeRet(&r);
    }
    {   // A2W / W2A 默认 936，ANSI 期望值由 Win32 现算
        const wchar_t* src = W(0x4E2D, 0x6587, 0);
        char gbk[64];
        int nGbk = ToCp(src, 2, 936, gbk, sizeof(gbk));
        MDATA_INF a[2] = { TextArg(gbk), NullArg() };
        MDATA_INF r; Call(FindGlobalCmd("A2W"), a, 2, &r);
        ExpectRetWide(&r, src, 3, "A2W(默认936)");
        MDATA_INF a2[2] = { r, NullArg() };
        MDATA_INF r2; Call(FindGlobalCmd("W2A"), a2, 2, &r2);
        ExpectRetBytes(&r2, gbk, nGbk, "W2A(默认936) 还原");
        FreeRet(&r2); FreeRet(&r);
    }
    {   // A2W_no 末尾不带 {0,0}
        const wchar_t* src = W(0x4E2D, 0x6587, 0);
        char gbk[64]; ToCp(src, 2, 936, gbk, sizeof(gbk));
        MDATA_INF a[2] = { TextArg(gbk), IntArg(936) };
        MDATA_INF r; Call(FindGlobalCmd("A2W_no"), a, 2, &r);
        ExpectRetWide(&r, src, 2, "A2W_no(无终止符)");
        FreeRet(&r);
    }
    {   // W2UTF8 / W2UTF8_T
        const wchar_t* src = W(0x4E2D, 0x6587, 'x', 0);
        char u8[64]; int nU8 = ToCp(src, 3, CP_UTF8, u8, sizeof(u8));
        MDATA_INF a[1] = { BinWArg(src) };
        MDATA_INF r; Call(FindGlobalCmd("W2UTF8"), a, 1, &r);
        ExpectRetBytes(&r, u8, nU8, "W2UTF8");
        FreeRet(&r); FreeBinArg(&a[0]);
        MDATA_INF a2[1] = { BinWArg(src) };
        MDATA_INF r2; Call(FindGlobalCmd("W2UTF8_T"), a2, 1, &r2);
        ExpectRetBytes(&r2, u8, nU8, "W2UTF8_T");
        FreeRet(&r2); FreeBinArg(&a2[0]);
    }
    {   // UTF82W(字节集) / UTF82W_T(文本)
        const wchar_t* src = W(0x4E2D, 0x6587, 0);
        char u8[64]; int nU8 = ToCp(src, 2, CP_UTF8, u8, sizeof(u8));
        MDATA_INF a[1] = { RawBinArg(u8, nU8) };
        MDATA_INF r; Call(FindGlobalCmd("UTF82W"), a, 1, &r);
        ExpectRetWide(&r, src, 3, "UTF82W(字节集)");
        FreeRet(&r); FreeBinArg(&a[0]);
        MDATA_INF a2[1] = { TextArg(u8) };
        MDATA_INF r2; Call(FindGlobalCmd("UTF82W_T"), a2, 1, &r2);
        ExpectRetWide(&r2, src, 3, "UTF82W_T(文本)");
        FreeRet(&r2);
    }
    {   // UTF82A / UTF82A_T / A2UTF8 / A2UTF8_T
        const wchar_t* src = W(0x4E2D, 0x6587, 0);
        char u8[64], gbk[64];
        int nU8 = ToCp(src, 2, CP_UTF8, u8, sizeof(u8));
        int nGbk = ToCp(src, 2, 936, gbk, sizeof(gbk));
        MDATA_INF a[2] = { RawBinArg(u8, nU8), IntArg(936) };
        MDATA_INF r; Call(FindGlobalCmd("UTF82A"), a, 2, &r);
        ExpectRetBytes(&r, gbk, nGbk, "UTF82A(936)");
        FreeRet(&r); FreeBinArg(&a[0]);
        MDATA_INF a2[2] = { TextArg(u8), IntArg(936) };
        MDATA_INF r2; Call(FindGlobalCmd("UTF82A_T"), a2, 2, &r2);
        ExpectRetBytes(&r2, gbk, nGbk, "UTF82A_T(936)");
        FreeRet(&r2);
        MDATA_INF a3[2] = { TextArg(gbk), IntArg(936) };
        MDATA_INF r3; Call(FindGlobalCmd("A2UTF8"), a3, 2, &r3);
        ExpectRetBytes(&r3, u8, nU8, "A2UTF8(936)");
        FreeRet(&r3);   // 参数是 TextArg，不是堆缓冲，不能 FreeBinArg
        MDATA_INF a4[2] = { TextArg(gbk), IntArg(936) };
        MDATA_INF r4; Call(FindGlobalCmd("A2UTF8_T"), a4, 2, &r4);
        ExpectRetBytes(&r4, u8, nU8, "A2UTF8_T(936)");
        FreeRet(&r4);
    }
    {   // 编码_utf8到gb2312 / 编码_gb2312到utf8
        const wchar_t* src = W(0x4E2D, 0x6587, 'a', 0);
        char u8[64], gbk[64];
        int nU8 = ToCp(src, 3, CP_UTF8, u8, sizeof(u8));
        int nGbk = ToCp(src, 3, 936, gbk, sizeof(gbk));
        MDATA_INF a[1] = { TextArg(u8) };
        MDATA_INF r; Call(FindGlobalCmd("编码_utf8到gb2312"), a, 1, &r);
        ExpectRetBytes(&r, gbk, nGbk, "编码_utf8到gb2312");
        FreeRet(&r);
        MDATA_INF a2[1] = { TextArg(gbk) };
        MDATA_INF r2; Call(FindGlobalCmd("编码_gb2312到utf8"), a2, 1, &r2);
        ExpectRetBytes(&r2, u8, nU8, "编码_gb2312到utf8");
        FreeRet(&r2);
    }
    {   // 宽文本_处理末尾空字符：假只去末尾，真按终止符补齐
        const wchar_t* src = W('a', 'b', 0);
        MDATA_INF a[2] = { BinWArg(src), BoolArg(false) };
        MDATA_INF r; Call(FindGlobalCmd("宽文本_处理末尾空字符"), a, 2, &r);
        ExpectRetWide(&r, src, 2, "宽文本_处理末尾空字符(非标准)");
        FreeRet(&r); FreeBinArg(&a[0]);
        MDATA_INF a2[2] = { BinWArg(src), BoolArg(true) };
        MDATA_INF r2; Call(FindGlobalCmd("宽文本_处理末尾空字符"), a2, 2, &r2);
        ExpectRetWide(&r2, src, 3, "宽文本_处理末尾空字符(标准)");
        FreeRet(&r2); FreeBinArg(&a2[0]);
    }
    {   // 编码_URL编码W：空格/<//> 都要转义，字母数字与 -_.!~*'() 保留
        MDATA_INF a[1] = { BinWArg(W(0x4E2D, 0x6587, 'a', ' ', 'b', '<', 'c', '>', '-', '_', '!', 0)) };
        MDATA_INF r; Call(FindGlobalCmd("编码_URL编码W"), a, 1, &r);
        ExpectRetAscii(&r, "%E4%B8%AD%E6%96%87a%20b%3Cc%3E-_!", "编码_URL编码W");
        FreeRet(&r); FreeBinArg(&a[0]);
    }
    {   // 编码_URL解码：假原样 UTF-8，真再转 936
        const wchar_t* src = W(0x4E2D, 0x6587, ' ', 'a', 0);
        char u8[64], gbk[64];
        int nU8 = ToCp(src, 4, CP_UTF8, u8, sizeof(u8));
        int nGbk = ToCp(src, 4, 936, gbk, sizeof(gbk));
        MDATA_INF a[2] = { TextArg("%E4%B8%AD%E6%96%87%20a"), BoolArg(false) };
        MDATA_INF r; Call(FindGlobalCmd("编码_URL解码"), a, 2, &r);
        ExpectRetBytes(&r, u8, nU8, "编码_URL解码(UTF-8)");
        FreeRet(&r);
        MDATA_INF a2[2] = { TextArg("%E4%B8%AD%E6%96%87%20a"), BoolArg(true) };
        MDATA_INF r2; Call(FindGlobalCmd("编码_URL解码"), a2, 2, &r2);
        ExpectRetBytes(&r2, gbk, nGbk, "编码_URL解码(转936)");
        FreeRet(&r2);
    }
    {   // 编码_URL解码_JS：0=decodeURI 保留 %2F，1=decodeURIComponent 全解
        MDATA_INF a[2] = { TextArg("a%2Fb%3Cc"), IntArg(0) };
        MDATA_INF r; Call(FindGlobalCmd("编码_URL解码_JS"), a, 2, &r);
        ExpectRetAscii(&r, "a%2Fb<c", "编码_URL解码_JS(decodeURI)");
        FreeRet(&r);
        MDATA_INF a2[2] = { TextArg("a%2Fb%3Cc"), IntArg(1) };
        MDATA_INF r2; Call(FindGlobalCmd("编码_URL解码_JS"), a2, 2, &r2);
        ExpectRetAscii(&r2, "a/b<c", "编码_URL解码_JS(decodeURIComponent)");
        FreeRet(&r2);
    }
    {   // HTML 实体：数字实体、命名实体、未知实体原样保留（宽文本版原版标废弃，未迁移）
        const wchar_t* jj = W(0x7CBE, 0x6613, 0);      // 精简 = &#31934;&#26131;
        char gbk[64]; int nGbk = ToCp(jj, 2, 936, gbk, sizeof(gbk));
        MDATA_INF a[1] = { TextArg("&#31934;&#26131;") };
        MDATA_INF r; Call(FindGlobalCmd("编码_HTML实体解码A"), a, 1, &r);
        ExpectRetBytes(&r, gbk, nGbk, "编码_HTML实体解码A");
        FreeRet(&r);
        MDATA_INF a2[1] = { TextArg("&amp;&#39;A") };
        MDATA_INF r2; Call(FindGlobalCmd("编码_HTML实体解码A"), a2, 1, &r2);
        ExpectRetAscii(&r2, "&'A", "编码_HTML实体解码A(命名+数字实体)");
        FreeRet(&r2);
        MDATA_INF a3[1] = { TextArg("&notreal;A&") };
        MDATA_INF r3; Call(FindGlobalCmd("编码_HTML实体解码A"), a3, 1, &r3);
        ExpectRetAscii(&r3, "&notreal;A&", "编码_HTML实体解码A(未知实体原样)");
        FreeRet(&r3);
    }

    {
        // 覆盖宿主报的崩点：分隔符与源文本相同（src == sep）
        AryVar ary(SDT_BIN);
        MDATA_INF a[3] = { BinWArg(W('a','b',0x4E2D,0x6587,0)), BinWArg(W('a','b',0x4E2D,0x6587,0)), ary.arg };
        MDATA_INF r; Call(FindGlobalCmd("分割文本W2"), a, 3, &r);
        Ok("分割文本W2(src==sep) -> %d 段，数组 %d 项", r.m_int, ary.Count());
        FreeRet(&r); FreeBinArg(&a[0]); FreeBinArg(&a[1]);
    }
    {
        AryVar ary(SDT_BIN, 2);
        ary.SetElem(0, MakeElemW(W('x',0)));
        ary.SetElem(1, MakeElemW(W('y',0)));
        MDATA_INF a[3] = { BinWArg(W('a','b',0x4E2D,0x6587,0)), BinWArg(W(',',0)), ary.arg };
        MDATA_INF r; Call(FindGlobalCmd("分割文本W2"), a, 3, &r);
        Ok("分割文本W2(预置旧数组) -> %d 段，数组 %d 项", r.m_int, ary.Count());
        FreeRet(&r); FreeBinArg(&a[0]); FreeBinArg(&a[1]);
    }

    printf("\n---- 正则匹配回归（用户报的失败样例）----\n");
    {   // 用户原样：跨行惰性匹配到第一个 [[ 为止（UTF-8 表达式入口）
        MDATA_INF a[6] = { WB(")]}'\r\n\r\n521\r\n[[\"wrb.fr\""), TextArg("(.|\\n)+?\\[\\["),
                           IntArg(1), NullArg(), NullArg(), NullArg() };
        MDATA_INF r; Call(FindGlobalCmd("文本_取文本_正则2W"), a, 6, &r);
        char buf[512]; RetBinToUtf8(&r, buf, sizeof(buf));
        for (char* q = buf; *q; q++) if (*q == '\r') *q = 'R';
        for (char* q = buf; *q; q++) if (*q == '\n') *q = 'N';
        if (strcmp(buf, ")]}'RNRN521RN[[") != 0)
            Fail("正则2W 跨行惰性匹配得「%s」，期望「)]}'RNRN521RN[[」", buf);
        else Ok("正则2W 跨行惰性匹配 -> 「%s」", buf);
        FreeRet(&r); FreeBinArg(&a[0]);
    }
    {   // 同一模式走宽文本表达式入口
        MDATA_INF a[6] = { WB("a\r\nb[[x"), WB("(.|\\n)+?\\[\\["),
                           IntArg(1), NullArg(), NullArg(), NullArg() };
        MDATA_INF r; Call(FindGlobalCmd("文本_取文本_正则W"), a, 6, &r);
        char buf[256]; RetBinToUtf8(&r, buf, sizeof(buf));
        for (char* q = buf; *q; q++) if (*q == '\r') *q = 'R';
        for (char* q = buf; *q; q++) if (*q == '\n') *q = 'N';
        if (strcmp(buf, "aRNb[[") != 0) Fail("正则W 跨行匹配得「%s」，期望「aRNb[[」", buf);
        else Ok("正则W 跨行匹配 -> 「%s」", buf);
        FreeRet(&r); FreeBinArg(&a[0]); FreeBinArg(&a[1]);
    }
    {   // 宽文本表达式入口 + 简单模式
        MDATA_INF a[6] = { WB("a12b3"), WB("[0-9]+"), IntArg(1), NullArg(), NullArg(), NullArg() };
        MDATA_INF r; Call(FindGlobalCmd("文本_取文本_正则W"), a, 6, &r);
        char buf[256]; RetBinToUtf8(&r, buf, sizeof(buf));
        if (strcmp(buf, "12") != 0) Fail("正则W [0-9]+ 得「%s」，期望「12」", buf);
        else Ok("正则W [0-9]+ -> 「%s」", buf);
        FreeRet(&r); FreeBinArg(&a[0]); FreeBinArg(&a[1]);
    }
    {   // 返回数量=0 取全部，多条结果用分隔符连接
        MDATA_INF a[6] = { WB("x[[1]y[[2]"), TextArg("\\[\\[(\\d)\\]"), IntArg(0), WB("|"),
                           NullArg(), NullArg() };
        MDATA_INF r; Call(FindGlobalCmd("文本_取文本_正则2W"), a, 6, &r);
        char buf[256]; RetBinToUtf8(&r, buf, sizeof(buf));
        if (strcmp(buf, "[[1]|[[2]") != 0) Fail("正则2W 取全部得「%s」，期望「[[1]|[[2]」", buf);
        else Ok("正则2W 取全部 -> 「%s」", buf);
        FreeRet(&r); FreeBinArg(&a[0]); FreeBinArg(&a[3]);
    }
    {   // Multiline 开着，^ 应能命中换行后的行首
        MDATA_INF a[6] = { WB("a\nbc"), TextArg("^bc"), IntArg(1), NullArg(), NullArg(), NullArg() };
        MDATA_INF r; Call(FindGlobalCmd("文本_取文本_正则2W"), a, 6, &r);
        char buf[256]; RetBinToUtf8(&r, buf, sizeof(buf));
        if (strcmp(buf, "bc") != 0) Fail("正则2W ^ 行首得「%s」，期望「bc」", buf);
        else Ok("正则2W ^ 行首 -> 「%s」", buf);
        FreeRet(&r); FreeBinArg(&a[0]);
    }
    {   // 真实报文：取 [[ 加 wrb.xx 那一段
        MDATA_INF a[6] = { WB("521\n[[\"wrb.fr\",\"MkEWBc\",\"[[null]]\"\n25\n[x]"),
                           TextArg("\\[\\[\"wrb\\.[a-z]+\""), IntArg(1), NullArg(), NullArg(), NullArg() };
        MDATA_INF r; Call(FindGlobalCmd("文本_取文本_正则2W"), a, 6, &r);
        char buf[512]; RetBinToUtf8(&r, buf, sizeof(buf));
        if (strcmp(buf, "[[\"wrb.fr\"") != 0) Fail("真实报文匹配得「%s」，期望「[[\"wrb.fr\"」", buf);
        else Ok("真实报文 [[wrb.xx -> 「%s」", buf);
        FreeRet(&r); FreeBinArg(&a[0]);
    }
    {   // 跨行惰性匹配整体替换：源文本是「参考 字节集」，必须交变量地址而不是按值传块
        // W() 以 0 结尾，末位不写 0 会把栈上的垃圾当字符带进夹具。
        void* slotOld = NULL;
        MDATA_INF a[6] = { BinWArgRef(W('a', '\r', '\n', 'b', '[', '[', 'x', 0), &slotOld),
                           WB("(.|\\n)+?\\[\\["), WB("<head>"), NullArg(), NullArg(), NullArg() };
        MDATA_INF r; Call(FindGlobalCmd("文本_替换_正则W"), a, 6, &r);
        char b1[256]; SlotToUtf8(slotOld, b1, sizeof(b1));
        for (char* q = b1; *q; q++) { if (*q == '\r') *q = 'R'; else if (*q == '\n') *q = 'N'; }
        if (r.m_int != 1 || strcmp(b1, "<head>x") != 0)
            Fail("文本_替换_正则W 得「%s」次数 %d，期望「<head>x」次数 1", b1, r.m_int);
        else Ok("文本_替换_正则W 跨行替换 -> 「%s」1 次", b1);
        FreeRet(&r); FreeBinArg(&a[1]); FreeBinArg(&a[2]);
        if (slotOld) free(slotOld);
    }

    printf("\n易语言内存池: 申请 %d / 释放 %d / 运行时错误 %d\n", g_mallocCount, g_freeCount, g_runtimeErr);
    if (g_runtimeErr) Fail("出现运行时错误");
    printf(g_failed ? "\n结论: %d 项不合格\n" : "\n结论: 全部通过\n", g_failed);
    FreeLibrary(h);
    return g_failed ? 1 : 0;
}
