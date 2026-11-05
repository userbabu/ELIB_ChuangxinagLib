// 离线宿主：Json2 类（zyJson2.51 移植）的语义断言。
// 直接装载 out\cxsoft.fne，按中文命令名调用，不依赖易语言 IDE。
// 期望值口径来自 zyJson2.51 源码分析：libs\zyJson支持库\spec-core.md / spec-output.md / api-spec.md。
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <tchar.h>

typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;

#include <lib2.h>
#include <lang.h>
#include <cx_json.h>
#include <cx_json.h>          // 只为拿 CxJsonNode 布局（宿主不解引用库内部指针之外的东西）

using namespace cx;   // 只为拿 CxJsonNode 布局与 CXJV_* 常量

#define AS_TEXT(p) ((const char*)(p))
#define J2_TYPE 8             // m_pDataType 下标：第 9 个类是 Json2

typedef PLIB_INFO (WINAPI *fn_GetNewInf)(void);

static PLIB_INFO g_inf = NULL;
static HMODULE g_hMod = NULL;
static int g_failed = 0, g_runtimeErr = 0, g_malloc = 0;
static int g_hit[512];

static LONG CALLBACK Veh(PEXCEPTION_POINTERS ep)
{
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    char* addr = (char*)ep->ExceptionRecord->ExceptionAddress;
    printf("[AV] code=0x%X addr=%p", code, addr);
    if (g_hMod && addr >= (char*)g_hMod && addr < (char*)g_hMod + 0x800000)
        printf(" rva=0x%X 在库内", (unsigned)(addr - (char*)g_hMod));
    printf("\n");
    fflush(stdout);
    return EXCEPTION_CONTINUE_SEARCH;
}

INT WINAPI FakeNotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    switch (nMsg)
    {
    case NRS_MALLOC: g_malloc++; return (INT)(INT_PTR)calloc(1, dwParam1 ? dwParam1 : 1);
    case NRS_MFREE: if (dwParam1) free((void*)(INT_PTR)dwParam1); return 0;
    case NRS_FREE_ARY:
    {
        void* p = (void*)(INT_PTR)dwParam2;
        if (p == NULL) return 0;
        int dim = *(int*)p, n = 1;
        for (int i = 0; i < dim; i++) n *= *((int*)p + 1 + i);
        char* e = (char*)p + 4 * (dim + 1);
        DATA_TYPE et = (DATA_TYPE)dwParam1 & ~DT_IS_ARY;
        if (et == SDT_TEXT || et == SDT_BIN || et == MAKELONG(CX_TYPE_JSON2, 0))
            for (int i = 0; i < n; i++) if (*(void**)(e + i * 4)) free(*(void**)(e + i * 4));
        free(p);
        return 0;
    }
    case NRS_RUNTIME_ERR: g_runtimeErr++; printf("    [运行时错误] %s\n", (const char*)(INT_PTR)dwParam1); return 0;
    default: return 0;
    }
}

static void Fail(const char* fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    printf("[FAIL] "); vprintf(fmt, ap); printf("\n"); va_end(ap);
    g_failed++;
}

static void Ok(const char* fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    printf("       "); vprintf(fmt, ap); printf("\n"); va_end(ap);
}

static void PrintWide(const wchar_t* p)
{
    if (p == NULL) { printf("(null)"); return; }
    for (; *p; p++)
    {
        wchar_t c = *p;
        if (c == 13) printf("\\r");
        else if (c == 10) printf("\\n");
        else if (c == 9) printf("\\t");
        else if (c < 32 || c > 126) printf("\\u%04X", (unsigned)c);
        else printf("%c", (char)c);
    }
}

// ============================================================ 命令查找与调用
static int FindType(const char* szName)
{
    for (int i = 0; i < g_inf->m_nDataTypeCount; i++)
        if (lstrcmpA(AS_TEXT(g_inf->m_pDataType[i].m_szName), szName) == 0) return i;
    Fail("找不到类「%s」", szName);
    return -1;
}

static int FindCmd(int typeIdx, const char* szName)
{
    if (typeIdx < 0) return -1;
    PLIB_DATA_TYPE_INFO d = &g_inf->m_pDataType[typeIdx];
    for (int i = 0; i < d->m_nCmdCount; i++)
    {
        int idx = d->m_pnCmdsIndex[i];
        if (lstrcmpA(AS_TEXT(g_inf->m_pBeginCmdInfo[idx].m_szName), szName) == 0) return idx;
    }
    Fail("Json2 里找不到命令「%s」", szName);
    return -1;
}

static MDATA_INF NullArg() { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = _SDT_NULL; return m; }
static MDATA_INF TextArg(const char* s) { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = SDT_TEXT; m.m_pText = (char*)s; return m; }
static MDATA_INF IntArg(int v) { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = SDT_INT; m.m_int = v; return m; }
static MDATA_INF Int64Arg(__int64 v) { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = SDT_INT64; m.m_int64 = v; return m; }
static MDATA_INF DoubleArg(double v) { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = SDT_DOUBLE; m.m_double = v; return m; }
static MDATA_INF BoolArg(int v) { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = SDT_BOOL; m.m_bool = v ? TRUE : FALSE; return m; }

// 字节集实参（UTF-16 宽文本，末尾带 {0,0}）
static MDATA_INF BinArg(const wchar_t* pw)
{
    int nBytes = (int)((lstrlenW(pw) + 1) * sizeof(wchar_t));
    char* p = (char*)calloc(1, 8 + nBytes);
    *(int*)p = 1; *(int*)(p + 4) = nBytes;
    memcpy(p + 8, pw, nBytes);
    MDATA_INF m; memset(&m, 0, sizeof(m));
    m.m_dtDataType = SDT_BIN;
    m.m_pBin = (LPBYTE)p;
    return m;
}

static void FreeBin(MDATA_INF* p) { if (p->m_pBin) free(p->m_pBin); p->m_pBin = NULL; }

// 参考 整数型 出参
static MDATA_INF RefIntArg(int* pOut)
{
    MDATA_INF m; memset(&m, 0, sizeof(m));
    m.m_dtDataType = SDT_INT;
    m.m_pInt = pOut;
    return m;
}

static MDATA_INF g_pad[24];

static void Call(int cmd, MDATA_INF* a, int nArgs, MDATA_INF* pRet)
{
    memset(pRet, 0, sizeof(MDATA_INF));
    if (cmd < 0) return;
    if (cmd < 512) g_hit[cmd]++;
    int nWant = g_inf->m_pBeginCmdInfo[cmd].m_nArgCount;
    MDATA_INF* pArgs = a;
    int nPass = nArgs;
    if (nWant > nArgs)
    {
        if (nWant > 24) { Fail("命令 %d 参数过多", cmd); return; }
        memcpy(g_pad, a, (size_t)nArgs * sizeof(MDATA_INF));
        for (int i = nArgs; i < nWant; i++) g_pad[i] = NullArg();
        pArgs = g_pad;
        nPass = nWant;
    }
    g_inf->m_pCmdsFunc[cmd](pRet, nPass, pArgs);
}

// ============================================================ Json2 对象夹具
struct J2
{
    void* slot;
    MDATA_INF self;

    void New()
    {
        slot = NULL;
        memset(&self, 0, sizeof(self));
        self.m_dtDataType = MAKELONG(J2_TYPE + 1, 0);
        self.m_pCompoundData = &slot;
        MDATA_INF r;
        Call(FindCmd(J2_TYPE, "Json2构造函数"), &self, 1, &r);
    }
    void Delete()
    {
        MDATA_INF r;
        Call(FindCmd(J2_TYPE, "Json2析构函数"), &self, 1, &r);
        slot = NULL;
    }
    MDATA_INF Ref() { MDATA_INF m = self; return m; }
};

// 命令返回的对象：把返回缓冲包成一个可复用的对象实参
struct J2Child
{
    MDATA_INF ret;
    MDATA_INF arg;
    BOOL bValid;

    void Bind(MDATA_INF* pRet)
    {
        ret = *pRet;
        memset(&arg, 0, sizeof(arg));
        arg.m_dtDataType = MAKELONG(J2_TYPE + 1, 0);
        arg.m_pCompoundData = pRet->m_pCompoundData;
        bValid = 0;
        if (pRet->m_pCompoundData != NULL && *(int*)pRet->m_pCompoundData != 0)
        {
            CxJson2* pH = (CxJson2*)(INT_PTR)*(int*)pRet->m_pCompoundData;
            bValid = (pH != NULL && pH->node != NULL) ? 1 : 0;
        }
    }
    void Free()
    {
        if (arg.m_pCompoundData == NULL) return;
        MDATA_INF r;
        Call(FindCmd(J2_TYPE, "Json2析构函数"), &arg, 1, &r);
        free(arg.m_pCompoundData);
        arg.m_pCompoundData = NULL;
    }
    MDATA_INF Ref() { MDATA_INF m = arg; return m; }
};

// ============================================================ 断言
static void HexBytes(const char* p, char* out, int cap)
{
    int n = (int)lstrlenA(p);
    int o = 0;
    for (int i = 0; i < n && o + 4 < cap; i++) o += sprintf(out + o, "%02X ", (unsigned char)p[i]);
    out[o] = 0;
}

static void EqS(const char* what, const char* got, const char* want)
{
    if (got == NULL || lstrcmpA(got, want) != 0)
    {
        char a[512], b[512];
        HexBytes(want, a, sizeof(a));
        HexBytes(got ? got : "", b, sizeof(b));
        Fail("%s 期望[%s] 实际[%s]", what, a, b);
    }
    else Ok("%s -> %s", what, got);
}

static void EqW(const char* what, const wchar_t* got, const wchar_t* want)
{
    if (got == NULL || lstrcmpW(got, want) != 0)
    {
        printf("[FAIL] %s 期望 ", what); PrintWide(want); printf(" 实际 "); PrintWide(got); printf("\n");
        g_failed++;
    }
    else { printf("       %s -> ", what); PrintWide(got); printf("\n"); }
}

static void EqI(const char* what, int got, int want)
{
    if (got != want) Fail("%s 期望 %d 实际 %d", what, want, got);
    else Ok("%s -> %d", what, got);
}

static void EqB(const char* what, int got, int want)
{
    if ((got ? 1 : 0) != (want ? 1 : 0)) Fail("%s 期望 %d 实际 %d", what, want, got);
    else Ok("%s -> %d", what, got);
}

// 返回的 文本型 / 字节集(UTF-16) 视图
static const char* RetText(MDATA_INF* p) { return (p->m_dtDataType == SDT_TEXT) ? p->m_pText : ""; }
static const wchar_t* RetWide(MDATA_INF* p)
{
    if (p->m_dtDataType != SDT_BIN || p->m_pBin == NULL) return L"";
    int nBytes = *(int*)(p->m_pBin + 4);
    wchar_t* w = (wchar_t*)(p->m_pBin + 8);
    if (nBytes >= 2) w[nBytes / 2 - 1] = 0;            // 保证可当 C 串读
    return w;
}

static void FreeRet(MDATA_INF* p)
{
    if (p->m_dtDataType == SDT_TEXT && p->m_pText) free(p->m_pText);
    else if (p->m_dtDataType == SDT_BIN && p->m_pBin) free(p->m_pBin);
    else if (HIWORD(p->m_dtDataType) == 0 && LOWORD(p->m_dtDataType) >= 4 && p->m_pCompoundData) free(p->m_pCompoundData);
    memset(p, 0, sizeof(MDATA_INF));
}

// ============================================================ 常用调用封装
static int J2SetCode(J2* o, const char* szGbk)
{
    MDATA_INF a[2]; a[0] = o->Ref(); a[1] = TextArg(szGbk);
    MDATA_INF r; Call(FindCmd(J2_TYPE, "置代码"), a, 2, &r);
    int b = r.m_bool; FreeRet(&r); return b;
}

static int J2SetCodeW(J2* o, const wchar_t* pw)
{
    MDATA_INF a[2]; a[0] = o->Ref(); a[1] = BinArg(pw);
    MDATA_INF r; Call(FindCmd(J2_TYPE, "置代码W"), a, 2, &r);
    int b = r.m_bool; FreeBin(&a[1]); FreeRet(&r); return b;
}

static void J2ToText(J2* o, const char* szPath, int nFmt, int nTrans, MDATA_INF* pRet)
{
    MDATA_INF a[4];
    a[0] = o->Ref();
    a[1] = TextArg(szPath);
    a[2] = (nFmt < 0) ? NullArg() : BoolArg(nFmt);
    a[3] = (nTrans < 0) ? NullArg() : BoolArg(nTrans);
    Call(FindCmd(J2_TYPE, "到文本"), a, 4, pRet);
}

static void ExpectJson(J2* o, const char* szPath, const char* want, const char* what)
{
    MDATA_INF r;
    J2ToText(o, szPath, -1, -1, &r);
    EqS(what, RetText(&r), want);
    FreeRet(&r);
}

static void ExpectJsonFmt(J2* o, const wchar_t* want, const char* what)
{
    MDATA_INF r;
    J2ToText(o, "", 1, 1, &r);
    EqW(what, RetWide(&r), want);
    FreeRet(&r);
}

static int J2Type(J2* o, const char* szPath)
{
    MDATA_INF a[2]; a[0] = o->Ref(); a[1] = TextArg(szPath);
    MDATA_INF r; Call(FindCmd(J2_TYPE, "取类型"), a, 2, &r);
    int v = r.m_int; FreeRet(&r); return v;
}

static int J2Count(J2* o, const char* szPath)
{
    MDATA_INF a[2]; a[0] = o->Ref(); a[1] = TextArg(szPath);
    MDATA_INF r; Call(FindCmd(J2_TYPE, "取键数量"), a, 2, &r);
    int v = r.m_int; FreeRet(&r); return v;
}

static void J2GetText(J2* o, const char* szPath, MDATA_INF* pRet)
{
    MDATA_INF a[2]; a[0] = o->Ref(); a[1] = TextArg(szPath);
    Call(FindCmd(J2_TYPE, "取文本"), a, 2, pRet);
}

static void ExpectGetText(J2* o, const char* szPath, const char* want, const char* what)
{
    MDATA_INF r; J2GetText(o, szPath, &r);
    EqS(what, RetText(&r), want);
    FreeRet(&r);
}

// 取子对象（K / K2 / I / I2 / 取父 / 选择 / 复制 / 创建X）
static int J2ChildOf(J2* o, const char* szCmd, J2Child* pOut, const MDATA_INF* pExtra, const char* szPath)
{
    MDATA_INF a[3];
    a[0] = o->Ref();
    a[1] = pExtra ? *pExtra : NullArg();
    a[2] = TextArg(szPath ? szPath : "");
    MDATA_INF r;
    Call(FindCmd(J2_TYPE, szCmd), a, 3, &r);
    pOut->Bind(&r);
    return pOut->bValid ? 1 : 0;
}

static int J2ChildByPath(J2* o, const char* szCmd, J2Child* pOut, const char* szPath)
{
    MDATA_INF a[2];
    a[0] = o->Ref();
    a[1] = TextArg(szPath);
    MDATA_INF r;
    Call(FindCmd(J2_TYPE, szCmd), a, 2, &r);
    pOut->Bind(&r);
    return pOut->bValid ? 1 : 0;
}

static int J2ChildByIndex(J2* o, const char* szCmd, J2Child* pOut, int nIdx, const char* szPath)
{
    MDATA_INF a[3];
    a[0] = o->Ref();
    a[1] = IntArg(nIdx);
    a[2] = TextArg(szPath);
    MDATA_INF r;
    Call(FindCmd(J2_TYPE, szCmd), a, 3, &r);
    pOut->Bind(&r);
    return pOut->bValid ? 1 : 0;
}

static int J2CreateNew(J2* o, const char* szCmd, J2Child* pOut, const MDATA_INF* pArg)
{
    MDATA_INF a[2];
    a[0] = o->Ref();
    a[1] = pArg ? *pArg : NullArg();
    MDATA_INF r;
    Call(FindCmd(J2_TYPE, szCmd), a, (pArg ? 2 : 1), &r);
    pOut->Bind(&r);
    return pOut->bValid ? 1 : 0;
}

// ============================================================ 测试：解析与回写
// 期望值里的非 ASCII 文本在运行期用 Win32 现算：源码窄串里的非 ASCII 字节会被
// /execution-charset:gbk 改写，写死字节不可靠（踩过）。
static const char* U8Of(const wchar_t* w, char* buf, int cap)
{
    int nChars = (int)lstrlenW(w);
    int n = WideCharToMultiByte(CP_UTF8, 0, w, nChars, buf, cap - 1, NULL, NULL);
    if (n < 0) n = 0;
    buf[n] = 0;
    return buf;
}

static const char* GbkOf(const wchar_t* w, char* buf, int cap)
{
    int nChars = (int)lstrlenW(w);
    int n = WideCharToMultiByte(936, 0, w, nChars, buf, cap - 1, NULL, NULL);
    if (n < 0) n = 0;
    buf[n] = 0;
    return buf;
}

static void TestParse()
{
    printf("\n[1] 解析与回写（原文回放）\n");
    const char* cases[] = {
        "{\"a\":1,\"b\":[1,2,{\"c\":\"x\"}],\"d\":{\"e\":true}}",
        "{\"z\":1,\"a\":2}",
        "{}", "[]", "[1,2,3]",
        "{\"n\":null,\"f\":false,\"t\":true}",
        "{\"a\":1.50}", "{\"a\":-0}", "{\"a\":+5}", "{\"a\":007}", "{\"a\":1e5}", "{\"a\":1E+5}",
        "{\"a\":[]}", "{\"a\":{}}", "[\"a\",\"b\"]",
        "{\"a\":1,\"a\":2}",
        "{\"a\":\"has space\",\"b c\":1}",
    };
    for (int i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++)
    {
        J2 o; o.New();
        EqI("置代码 返回真", J2SetCode(&o, cases[i]), 1);
        MDATA_INF r;
        J2ToText(&o, "", -1, -1, &r);
        // 重复键 {"a":1,"a":2} 只留一个成员
        const char* want = cases[i];
        static const char* dedup[] = { "{\"a\":1,\"a\":2}", "{\"a\":2}" };
        if (lstrcmpA(cases[i], dedup[0]) == 0) want = dedup[1];
        EqS("到文本 回写", RetText(&r), want);
        FreeRet(&r);
        o.Delete();
    }
}

static void TestLenientSyntax()
{
    printf("\n[2] 宽松语法（原版接受的行为）\n");
    struct { const char* szIn; const char* szWant; } t[] = {
        { "{\"a\":1,}", "{\"a\":1}" },
        { "[1,2,]", "[1,2]" },
        { "{a:'b'}", "{\"a\":\"b\"}" },
        { "{'a':'b'}", "{\"a\":\"b\"}" },
        { "[1,,2]", "[1,2]" },
        { "{\"a\":1}{\"b\":2}", "{\"a\":1,\"b\":2}" },
        { "[1][2]", "[1,2]" },
        { "123", "123" },
        { "true", "true" },
        { "null", "null" },
    };
    for (int i = 0; i < (int)(sizeof(t) / sizeof(t[0])); i++)
    {
        J2 o; o.New();
        EqI("置代码", J2SetCode(&o, t[i].szIn), 1);
        MDATA_INF r; J2ToText(&o, "", -1, -1, &r);
        EqS(t[i].szIn, RetText(&r), t[i].szWant);
        FreeRet(&r);
        o.Delete();
    }
    // 根标量前置空白：原版切片起点写死串首，会静默丢尾字符
    {
        J2 o; o.New();
        J2SetCode(&o, " 123");
        MDATA_INF r; J2ToText(&o, "", -1, -1, &r);
        EqS("根标量带前置空白", RetText(&r), " 12");
        FreeRet(&r);
        o.Delete();
    }
    // 输出侧转义
    {
        J2 o; o.New();
        J2SetCode(&o, "{\"a\":\"x/y\"}");
        ExpectJson(&o, "", "{\"a\":\"x\\/y\"}", "斜杠恒被转义");
        o.Delete();
        o.New();
        J2SetCode(&o, "{\"a\":\"p\\nq\"}");
        ExpectJson(&o, "", "{\"a\":\"p\\r\\nq\"}", "\\n 解析成 CRLF 再输出");
        o.Delete();
        o.New();
        J2SetCode(&o, "{\"a\":\"\\x41\"}");
        ExpectJson(&o, "", "{\"a\":\"\\\\x41\"}", "未知转义原样保留并再转义");
        o.Delete();
    }
    // 错误码与位置
    {
        struct { const char* szBad; int nCode; } e[] = {
            { " null", -6 }, { "{\"a\":}", -6 }, { "{\"a\"}", -6 },
            { "{\"a\":NaN}", -6 }, { "{\"a\":1.2.3}", -6 }, { "{\"a\"1}", -4 },
        };
        for (int i = 0; i < (int)(sizeof(e) / sizeof(e[0])); i++)
        {
            J2 o; o.New();
            J2SetCode(&o, "{\"ok\":1}");
            MDATA_INF a[5];
            int err = 0, pos = -1;
            a[0] = o.Ref(); a[1] = TextArg(e[i].szBad); a[2] = NullArg();
            a[3] = RefIntArg(&err); a[4] = RefIntArg(&pos);
            MDATA_INF r; Call(FindCmd(J2_TYPE, "置代码"), a, 5, &r);
            EqB("坏语法返回假", r.m_bool, 0);
            EqI("错误码", err, e[i].nCode);
            FreeRet(&r);
            ExpectJson(&o, "", "{}", "解析失败后节点被清空");
            o.Delete();
        }
        // 未闭合容器
        J2 o; o.New();
        MDATA_INF a[5]; int err = 0, pos = 0;
        a[0] = o.Ref(); a[1] = TextArg("{\"a\":1"); a[2] = NullArg();
        a[3] = RefIntArg(&err); a[4] = RefIntArg(&pos);
        MDATA_INF r; Call(FindCmd(J2_TYPE, "置代码"), a, 5, &r);
        EqI("未闭合容器错误码", err, -5);
        FreeRet(&r);
        o.Delete();
    }
}

static void TestUnicodeAndEncodings()
{
    printf("\n[3] 三种编码入口（GBK / UTF-16 字节集 / UTF-8）\n");
    // GBK 文本入口
    {
        J2 o; o.New();
        wchar_t w[64];
        lstrcpyW(w, L"{\"名\":\"值\"}");
        char gbk[128];
        WideCharToMultiByte(936, 0, w, -1, gbk, sizeof(gbk), NULL, NULL);
        EqI("置代码(GBK)", J2SetCode(&o, gbk), 1);
        ExpectGetText(&o, "名", "值", "取文本 GBK");
        MDATA_INF r; J2ToText(&o, "", -1, -1, &r);
        EqS("到文本 保留中文", RetText(&r), gbk);
        FreeRet(&r);
        o.Delete();
    }
    // UTF-16 字节集入口
    {
        J2 o; o.New();
        EqI("置代码W", J2SetCodeW(&o, L"{\"名\":\"值\"}"), 1);
        MDATA_INF a[2]; a[0] = o.Ref(); a[1] = BinArg(L"名");
        MDATA_INF r; Call(FindCmd(J2_TYPE, "取文本W"), a, 2, &r);
        EqW("取文本W", RetWide(&r), L"值");
        FreeRet(&r); FreeBin(&a[1]);
        MDATA_INF b[2]; b[0] = o.Ref(); b[1] = BinArg(L"");
        Call(FindCmd(J2_TYPE, "到文本W"), b, 2, &r);
        EqW("到文本W", RetWide(&r), L"{\"\u540d\":\"\u503c\"}");
        FreeRet(&r); FreeBin(&b[1]);
        o.Delete();
    }
    // UTF-8 入口
    {
        J2 o; o.New();
        MDATA_INF a[2];
        a[0] = o.Ref();
        a[1] = TextArg("{\"a\":\"\\u4e2d\"}");
        MDATA_INF r;
        Call(FindCmd(J2_TYPE, "置代码UTF8"), a, 2, &r);
        EqB("置代码UTF8", r.m_bool, 1);
        FreeRet(&r);
        ExpectJson(&o, "", "{\"a\":\"\\\\u4e2d\"}", "置代码UTF8 默认不解码 \\u");
        o.Delete();
        o.New();
        MDATA_INF b[3];
        b[0] = o.Ref();
        {
            static wchar_t wSrc[] = L"{\"k\":\"中\"}";
            char szU8[128]; U8Of(wSrc, szU8, sizeof(szU8));
            b[1] = TextArg(szU8);
        }
        b[2] = BoolArg(1);
        Call(FindCmd(J2_TYPE, "置代码UTF8"), b, 3, &r);
        FreeRet(&r);
        MDATA_INF c[2]; c[0] = o.Ref(); c[1] = NullArg();
        Call(FindCmd(J2_TYPE, "取文本UTF8"), c, 2, &r);
        { char w8[128]; EqS("取文本UTF8 整棵子树", RetText(&r), U8Of(L"{\"k\":\"中\"}", w8, sizeof(w8))); }
        FreeRet(&r);
        o.Delete();
    }
    // \u 解码与代理对
    {
        J2 o; o.New();
        J2SetCodeW(&o, L"{\"a\":\"\\uD83D\\uDE00\"}");
        MDATA_INF aW[2]; aW[0] = o.Ref(); aW[1] = BinArg(L"a");
        MDATA_INF r;
        Call(FindCmd(J2_TYPE, "取文本W"), aW, 2, &r);
        EqW("代理对解码成两个码元", RetWide(&r), L"\xD83D\xDE00");
        FreeBin(&aW[1]);
        FreeRet(&r);
        o.Delete();
    }
}

static void TestTypesAndCoercion()
{
    printf("\n[4] 类型标记与跨类型强转\n");
    J2 o; o.New();
    J2SetCode(&o, "{\"s\":\"78\",\"t\":\"abc\",\"d\":1.75,\"l\":-5,\"b\":true,\"z\":null,\"e\":\"\",\"o\":{},\"arr\":[1]}");
    EqI("文本型", J2Type(&o, "s"), 4);
    EqI("容器内整数按原版是双精度型", J2Type(&o, "l"), 1);
    EqI("空值", J2Type(&o, "z"), 0);
    EqI("逻辑值", J2Type(&o, "b"), 3);
    EqI("对象", J2Type(&o, "o"), 6);
    EqI("数组", J2Type(&o, "arr"), 5);
    EqI("不存在的键", J2Type(&o, "nope"), -1);
    EqI("根成员数", J2Count(&o, ""), 9);
    EqI("数组成员数", J2Count(&o, "arr"), 1);
    EqI("标量成员数", J2Count(&o, "s"), 0);

    ExpectGetText(&o, "s", "78", "取文本 字符串");
    ExpectGetText(&o, "d", "1.75", "取文本 数字原文");
    ExpectGetText(&o, "b", "true", "取文本 布尔原文");
    ExpectGetText(&o, "z", "", "取文本 null 被抹空");
    ExpectGetText(&o, "e", "", "取文本 空串");
    ExpectGetText(&o, "o", "", "取文本 空对象抹空");
    ExpectGetText(&o, "arr", "[1]", "取文本 数组取整棵子树");
    ExpectGetText(&o, "nope", "", "取文本 不存在为空");
    ExpectGetText(&o, "t", "abc", "取文本 非数字字符串");

    {
        MDATA_INF a[2]; a[0] = o.Ref();
        MDATA_INF r;
        a[1] = TextArg("s");
        Call(FindCmd(J2_TYPE, "取整数"), a, 2, &r); EqI("文本取整数", r.m_int, 78); FreeRet(&r);
        Call(FindCmd(J2_TYPE, "取双精度"), a, 2, &r); EqI("文本取双精度", (int)r.m_double, 78); FreeRet(&r);
        a[1] = TextArg("t");
        Call(FindCmd(J2_TYPE, "取整数"), a, 2, &r); EqI("非数字文本取整数", r.m_int, 0); FreeRet(&r);
        a[1] = TextArg("d");
        Call(FindCmd(J2_TYPE, "取整数"), a, 2, &r); EqI("双精度取整数截断", r.m_int, 1); FreeRet(&r);
        Call(FindCmd(J2_TYPE, "取长整数"), a, 2, &r); EqI("双精度取长整数截断", (int)r.m_int64, 1); FreeRet(&r);
        a[1] = TextArg("l");
        Call(FindCmd(J2_TYPE, "取长整数"), a, 2, &r); EqI("取长整数", (int)r.m_int64, -5); FreeRet(&r);
        a[1] = TextArg("b");
        Call(FindCmd(J2_TYPE, "取逻辑"), a, 2, &r); EqB("取逻辑 真", r.m_bool, 1); FreeRet(&r);
        Call(FindCmd(J2_TYPE, "取数值"), a, 2, &r); EqI("布尔取数值", (int)r.m_double, 1); FreeRet(&r);
        a[1] = TextArg("s");
        Call(FindCmd(J2_TYPE, "取逻辑"), a, 2, &r); EqB("文本取逻辑一律假", r.m_bool, 0); FreeRet(&r);
        a[1] = TextArg("z");
        Call(FindCmd(J2_TYPE, "取逻辑"), a, 2, &r); EqB("空值取逻辑假", r.m_bool, 0); FreeRet(&r);
        // 取源数据
        a[1] = TextArg("d");
        Call(FindCmd(J2_TYPE, "取源数据"), a, 2, &r); EqS("取源数据 数字原文", RetText(&r), "1.75"); FreeRet(&r);
        a[1] = TextArg("s");
        Call(FindCmd(J2_TYPE, "取源数据"), a, 2, &r); EqS("取源数据 文本型无原文", RetText(&r), ""); FreeRet(&r);
        a[1] = TextArg("z");
        Call(FindCmd(J2_TYPE, "取源数据"), a, 2, &r); EqS("取源数据 空值", RetText(&r), "null"); FreeRet(&r);
        a[1] = TextArg("o");
        Call(FindCmd(J2_TYPE, "取源数据"), a, 2, &r); EqS("取源数据 对象无原文", RetText(&r), ""); FreeRet(&r);
    }
    o.Delete();
}

// ============================================================ 测试辅助：布尔与数组块
static void ExpectTrue_(int b, const char* what)
{
    if (!b) Fail("%s 期望真", what); else Ok("%s -> 真", what);
}

// 数组数据块：[INT 维数=1][INT 元素数][元素区]
// 文本/字节集/对象数组的元素是指针；数值/整数/逻辑数组的元素是内联值。
static void* MakeAryBlock(int n, int nElemBytes)
{
    char* p = (char*)calloc(1, 8 + (size_t)n * nElemBytes);
    *(int*)p = 1;
    *(int*)(p + 4) = n;
    return p;
}

static void ArySetPtr(void* blk, int i, void* pElem) { *(void**)((char*)blk + 8 + i * 4) = pElem; }
static char* AryNewElemText(const char* s)
{
    size_t n = strlen(s) + 1;
    char* p = (char*)calloc(1, n);
    memcpy(p, s, n);
    return p;
}
static char* AryNewElemBin(const wchar_t* w)
{
    int nb = (int)((lstrlenW(w) + 1) * sizeof(wchar_t));
    char* p = (char*)calloc(1, 8 + nb);
    *(int*)p = 1; *(int*)(p + 4) = nb;
    memcpy(p + 8, w, nb);
    return p;
}
static void AryFreePtrs(void* blk, int n)
{
    for (int i = 0; i < n; i++) if (*(void**)((char*)blk + 8 + i * 4)) free(*(void**)((char*)blk + 8 + i * 4));
    free(blk);
}
static int AryCount(void* blk) { return blk ? *(int*)((char*)blk + 4) : 0; }

static MDATA_INF RefAryArg(MDATA_INF* pOut, void** ppBlk, DATA_TYPE elemType)
{
    memset(pOut, 0, sizeof(*pOut));
    pOut->m_dtDataType = elemType | DT_IS_ARY;
    pOut->m_ppAryData = ppBlk;
    return *pOut;
}

// 按数据接收的数组入参（AS_RECEIVE_ARRAY_DATA）：直接交出块指针，不经过变量槽。
static MDATA_INF DataAryArg(MDATA_INF* pOut, void* pBlk, DATA_TYPE elemType)
{
    memset(pOut, 0, sizeof(*pOut));
    pOut->m_dtDataType = elemType | DT_IS_ARY;
    pOut->m_pAryData = (LPSTR)pBlk;
    return *pOut;
}

// ============================================================ 测试：路径与子对象
static void TestPaths()
{
    printf("\n[5] 路径解析与子对象\n");
    {   // 名→对象、[i]→数组 的自动创建
        J2 o; o.New();
        MDATA_INF a[3]; a[0] = o.Ref(); a[1] = TextArg("x"); a[2] = TextArg("a.b[0].c");
        MDATA_INF r; Call(FindCmd(J2_TYPE, "置文本"), a, 3, &r);
        EqB("置文本(路径)", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "{\"a\":{\"b\":[{\"c\":\"x\"}]}}", "路径自动创建容器");
        o.Delete();
    }
    {   // 转义：\. 让点名留在键里；点分路径查不到它
        J2 o; o.New();
        MDATA_INF a[3]; a[0] = o.Ref(); a[1] = TextArg("v"); a[2] = TextArg("k\\.m");
        MDATA_INF r; Call(FindCmd(J2_TYPE, "置文本"), a, 3, &r); FreeRet(&r);
        ExpectJson(&o, "", "{\"k.m\":\"v\"}", "转义点名");
        J2GetText(&o, "k.m", &r);
        EqS("点分路径查不到带点的键", RetText(&r), "");
        FreeRet(&r);
        EqI("是否存在 带点键", J2Type(&o, "k\\.m"), 4);
        o.Delete();
    }
    {   // K 读缺失键补 null；K2 不补
        J2 o; o.New();
        J2SetCode(&o, "{\"a\":1}");
        MDATA_INF key = TextArg("fresh");
        J2Child c1;
        EqI("K2 缺失键返回空对象", J2ChildOf(&o, "K2", &c1, &key, ""), 0);
        c1.Free();
        EqI("K2 没有改文档", J2Count(&o, ""), 1);
        J2Child c2;
        EqI("K 缺失键返回对象", J2ChildOf(&o, "K", &c2, &key, ""), 1);
        c2.Free();
        EqI("K 补了成员", J2Count(&o, ""), 2);
        ExpectJson(&o, "", "{\"a\":1,\"fresh\":null}", "读操作写文档");
        o.Delete();
    }
    {   // I 越界补齐；I2 越界不补
        J2 o; o.New();
        J2SetCode(&o, "{\"arr\":[1]}");
        J2Child c;
        EqI("I 越界仍返回对象", J2ChildByIndex(&o, "I", &c, 3, "arr"), 1);
        c.Free();
        EqI("I 补齐到 4 个", J2Count(&o, "arr"), 4);
        ExpectJson(&o, "", "{\"arr\":[1,null,null,null]}", "I 越界补 null");
        J2Child c2;
        EqI("I2 越界不返回对象", J2ChildByIndex(&o, "I2", &c2, 9, "arr"), 0);
        c2.Free();
        EqI("I2 不改长度", J2Count(&o, "arr"), 4);
        o.Delete();
    }
    {   // 是否存在 / 选择 / 取父 / 取属性名
        J2 o; o.New();
        J2SetCode(&o, "{\"a\":{\"b\":[{\"c\":\"x\"}]}}");
        MDATA_INF a[2]; a[0] = o.Ref();
        MDATA_INF r;
        a[1] = TextArg("a.b[0].c");
        Call(FindCmd(J2_TYPE, "是否存在"), a, 2, &r); EqB("是否存在 命中", r.m_bool, 1); FreeRet(&r);
        a[1] = TextArg("a.b[0].nope");
        Call(FindCmd(J2_TYPE, "是否存在"), a, 2, &r); EqB("是否存在 未命中", r.m_bool, 0); FreeRet(&r);
        a[1] = TextArg("");
        Call(FindCmd(J2_TYPE, "是否存在"), a, 2, &r); EqB("空路径的存在性按原版返回假", r.m_bool, 0); FreeRet(&r);

        J2Child sel;
        EqI("选择 命中", J2ChildByPath(&o, "选择", &sel, "a.b[0]"), 1);
        MDATA_INF b[2]; b[0] = sel.Ref(); b[1] = TextArg("");
        Call(FindCmd(J2_TYPE, "取属性名"), b, 2, &r);
        EqS("数组元素没有属性名", RetText(&r), "");
        FreeRet(&r);
        sel.Free();

        J2Child par;
        EqI("取父 命中", J2ChildByPath(&o, "取父", &par, "a.b[0].c"), 1);
        MDATA_INF c[2]; c[0] = par.Ref(); c[1] = TextArg("");
        Call(FindCmd(J2_TYPE, "到文本"), c, 2, &r);
        EqS("取父 拿到父级子树", RetText(&r), "{\"c\":\"x\"}");
        FreeRet(&r);
        par.Free();

        MDATA_INF key = TextArg("a");
        J2Child kid;
        EqI("K 取属性", J2ChildOf(&o, "K", &kid, &key, ""), 1);
        MDATA_INF d[2]; d[0] = kid.Ref(); d[1] = TextArg("");
        Call(FindCmd(J2_TYPE, "取属性名"), d, 2, &r);
        EqS("取属性名 是 a", RetText(&r), "a");
        FreeRet(&r);
        kid.Free();
        o.Delete();
    }
    {   // 取指针 / 置指针 / 设置是否为顶层
        J2 o; o.New();
        J2SetCode(&o, "{\"a\":1}");
        MDATA_INF a[1]; a[0] = o.Ref();
        MDATA_INF r; Call(FindCmd(J2_TYPE, "取指针"), a, 1, &r);
        int nPtr = r.m_int; FreeRet(&r);
        ExpectTrue_(nPtr != 0, "取指针 非空");
        J2 o2; o2.New();
        MDATA_INF b[2]; b[0] = o2.Ref(); b[1] = IntArg(nPtr);
        MDATA_INF rv; Call(FindCmd(J2_TYPE, "置指针"), b, 2, &rv); FreeRet(&rv);
        ExpectJson(&o2, "", "{\"a\":1}", "置指针 与原文档共享同一棵树");
        MDATA_INF c[2]; c[0] = o2.Ref(); c[1] = BoolArg(0);
        MDATA_INF rt; Call(FindCmd(J2_TYPE, "设置是否为顶层"), c, 2, &rt); FreeRet(&rt);
        MDATA_INF d[2]; d[0] = o2.Ref(); d[1] = TextArg("{\"b\":2}");
        MDATA_INF rc; Call(FindCmd(J2_TYPE, "置代码"), d, 2, &rc);
        EqB("非顶层置代码走清空+合并", rc.m_bool, 1); FreeRet(&rc);
        ExpectJson(&o2, "", "{\"b\":2}", "非顶层置代码就地改写");
        ExpectJson(&o, "", "{\"b\":2}", "改写落到原文档（共享节点）");
        o2.Delete();
        o.Delete();
    }
}

// ============================================================ 测试：变更操作
static void TestMutation()
{
    printf("\n[6] 变更操作\n");
    {   // 工厂族 + 置属性
        J2 o; o.New();
        struct { const char* cn; int typ; int kind; } fac[] = {
            { "创建对象", 6, 0 }, { "创建数组", 5, 0 }, { "创建空", 0, 0 },
            { "创建逻辑", 3, 3 }, { "创建文本", 4, 1 }, { "创建文本W", 4, 2 },
            { "创建双精度", 1, 4 }, { "创建长整数", 2, 5 },
        };
        for (int i = 0; i < 8; i++)
        {
            MDATA_INF arg;
            switch (fac[i].kind)
            {
            case 1: arg = TextArg("s"); break;
            case 2: arg = BinArg(L"s"); break;
            case 3: arg = BoolArg(1); break;
            case 4: arg = DoubleArg(1.5); break;
            case 5: arg = Int64Arg(7); break;
            default: arg = NullArg(); break;
            }
            J2Child c;
            EqI(fac[i].cn, J2CreateNew(&o, fac[i].cn, &c, &arg), 1);
            MDATA_INF b[2]; b[0] = c.Ref(); b[1] = TextArg("");
            MDATA_INF r; Call(FindCmd(J2_TYPE, "取类型"), b, 2, &r);
            EqI(fac[i].cn, r.m_int, fac[i].typ);
            FreeRet(&r);
            if (fac[i].kind == 2) FreeBin(&arg);
            c.Free();
        }
        // 置属性：写入克隆值，同名覆盖保持键序
        J2SetCode(&o, "{}");                      // 置属性 原版不懒建根，先自己建
        J2Child obj;
        J2CreateNew(&o, "创建对象", &obj, NULL);
        MDATA_INF a[2]; a[0] = obj.Ref(); a[1] = TextArg("k");
        MDATA_INF r; Call(FindCmd(J2_TYPE, "置空"), a, 2, &r);
        EqB("子对象置空", r.m_bool, 1); FreeRet(&r);
        MDATA_INF b[4]; b[0] = o.Ref(); b[1] = TextArg(""); b[2] = TextArg("new"); b[3] = obj.Ref();
        Call(FindCmd(J2_TYPE, "置属性"), b, 4, &r);
        EqB("置属性", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "{\"new\":{\"k\":null}}", "置属性 写入克隆值");
        Call(FindCmd(J2_TYPE, "置属性"), b, 4, &r); FreeRet(&r);
        EqI("重复置属性不新增成员", J2Count(&o, ""), 1);
        ExpectJson(&o, "", "{\"new\":{\"k\":null}}", "重复置属性保持键序");
        obj.Free();
        MDATA_INF c[4]; c[0] = o.Ref(); c[1] = TextArg(""); c[2] = TextArg("bad"); c[3] = NullArg();
        Call(FindCmd(J2_TYPE, "置属性"), c, 4, &r);
        EqB("置属性 传空对象返回假", r.m_bool, 0); FreeRet(&r);
        o.Delete();
    }
    {   // 数组族：插入成员 / 置成员 / 移除成员 / 添加成员
        J2 o; o.New();
        J2SetCode(&o, "{\"arr\":[1,2,3]}");
        MDATA_INF r;
        MDATA_INF a[4]; a[0] = o.Ref(); a[1] = TextArg("arr"); a[2] = IntArg(1);
        J2Child item;
        MDATA_INF t = TextArg("m");
        J2CreateNew(&o, "创建文本", &item, &t);
        a[3] = item.Ref();
        Call(FindCmd(J2_TYPE, "插入成员"), a, 4, &r);
        EqB("插入成员", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "{\"arr\":[1,\"m\",2,3]}", "插入成员后");
        MDATA_INF b[4]; b[0] = o.Ref(); b[1] = IntArg(0); b[2] = TextArg("arr"); b[3] = item.Ref();
        Call(FindCmd(J2_TYPE, "置成员"), b, 4, &r);
        EqB("置成员", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "{\"arr\":[\"m\",\"m\",2,3]}", "置成员 长度不变");
        MDATA_INF c[3]; c[0] = o.Ref(); c[1] = IntArg(3); c[2] = TextArg("arr");
        Call(FindCmd(J2_TYPE, "移除成员"), c, 3, &r);
        EqB("移除成员", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "{\"arr\":[\"m\",\"m\",2]}", "移除成员后");
        MDATA_INF d[3]; d[0] = o.Ref(); d[1] = TextArg("arr"); d[2] = item.Ref();
        Call(FindCmd(J2_TYPE, "添加成员"), d, 3, &r);
        EqB("添加成员", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "{\"arr\":[\"m\",\"m\",2,\"m\"]}", "添加成员后");
        MDATA_INF e[4]; e[0] = o.Ref(); e[1] = IntArg(6); e[2] = TextArg("arr"); e[3] = item.Ref();
        Call(FindCmd(J2_TYPE, "置成员"), e, 4, &r);
        EqB("置成员 越界扩张", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "{\"arr\":[\"m\",\"m\",2,\"m\",null,null,\"m\"]}", "越界补 null 后写入");
        item.Free();
        MDATA_INF f[4]; f[0] = o.Ref(); f[1] = TextArg("arr"); f[2] = IntArg(99);
        J2Child one;
        MDATA_INF oneTxt = TextArg("1");
        J2CreateNew(&o, "创建文本", &one, &oneTxt);
        f[3] = one.Ref();
        Call(FindCmd(J2_TYPE, "插入成员"), f, 4, &r);
        EqB("插入成员 越界返回假", r.m_bool, 0); FreeRet(&r);
        one.Free();
        o.Delete();
    }
    {   // 插入属性 / 移除属性 / 排序对象 / 复制 / 分离 / 清空 / 销毁
        J2 o; o.New();
        J2SetCode(&o, "{\"a\":1,\"arr\":[2]}");
        MDATA_INF r;
        J2Child head;
        MDATA_INF t = TextArg("h");
        J2CreateNew(&o, "创建文本", &head, &t);
        MDATA_INF a[5]; a[0] = o.Ref(); a[1] = TextArg(""); a[2] = IntArg(0); a[3] = TextArg("head"); a[4] = head.Ref();
        Call(FindCmd(J2_TYPE, "插入属性"), a, 5, &r);
        EqB("插入属性", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "{\"head\":\"h\",\"a\":1,\"arr\":[2]}", "插入属性排在首位");
        head.Free();
        MDATA_INF b[3]; b[0] = o.Ref(); b[1] = TextArg("head"); b[2] = TextArg("");
        Call(FindCmd(J2_TYPE, "移除属性"), b, 3, &r);
        EqB("移除属性", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "{\"a\":1,\"arr\":[2]}", "移除属性后");
        b[1] = TextArg("nokey");
        Call(FindCmd(J2_TYPE, "移除属性"), b, 3, &r);
        EqB("移除不存在的键返回假", r.m_bool, 0); FreeRet(&r);
        MDATA_INF c[3]; c[0] = o.Ref(); c[1] = TextArg(""); c[2] = BoolArg(1);
        Call(FindCmd(J2_TYPE, "排序对象"), c, 3, &r);
        EqB("排序对象 降序", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "{\"arr\":[2],\"a\":1}", "排序对象 降序结果");
        c[2] = BoolArg(0);
        Call(FindCmd(J2_TYPE, "排序对象W"), c, 3, &r); FreeRet(&r);
        ExpectJson(&o, "", "{\"a\":1,\"arr\":[2]}", "排序对象W 升序结果");

        J2Child cp;
        EqI("复制 子树", J2ChildByPath(&o, "复制", &cp, "arr"), 1);
        MDATA_INF d[2]; d[0] = cp.Ref(); d[1] = TextArg("");
        Call(FindCmd(J2_TYPE, "到文本"), d, 2, &r);
        EqS("复制 得到独立子树", RetText(&r), "[2]");
        FreeRet(&r);
        MDATA_INF e[3]; e[0] = cp.Ref(); e[1] = TextArg(""); e[2] = TextArg("q");
        MDATA_INF re; MDATA_INF q[2]; q[0] = cp.Ref(); q[1] = TextArg("q");
        Call(FindCmd(J2_TYPE, "置空"), q, 2, &re); FreeRet(&re);
        ExpectJson(&o, "", "{\"a\":1,\"arr\":[2]}", "复制 与原树隔离");
        cp.Free();
        MDATA_INF k[2]; k[0] = o.Ref(); k[1] = TextArg("a");
        Call(FindCmd(J2_TYPE, "清空"), k, 2, &r); FreeRet(&r);
        EqI("清空 标量后仍是该类型", J2Type(&o, "a"), 1);

        MDATA_INF key = TextArg("a");
        J2Child kid;
        EqI("K 取成员", J2ChildOf(&o, "K", &kid, &key, ""), 1);
        MDATA_INF g[1]; g[0] = kid.Ref();
        MDATA_INF rg; Call(FindCmd(J2_TYPE, "分离"), g, 1, &rg); FreeRet(&rg);
        ExpectJson(&o, "", "{\"arr\":[2]}", "分离 把成员从父中摘出");
        kid.Free();

        MDATA_INF h[2]; h[0] = o.Ref(); h[1] = TextArg("");
        MDATA_INF rh; Call(FindCmd(J2_TYPE, "清空"), h, 2, &rh); FreeRet(&rh);
        ExpectJson(&o, "", "{}", "清空 根对象");
        MDATA_INF hw[2]; hw[0] = o.Ref(); hw[1] = BinArg(L"");
        MDATA_INF rw2; Call(FindCmd(J2_TYPE, "清空W"), hw, 2, &rw2); FreeRet(&rw2); FreeBin(&hw[1]);
        MDATA_INF rz; Call(FindCmd(J2_TYPE, "销毁"), h, 2, &rz); FreeRet(&rz);
        EqI("销毁 后取类型未定义", J2Type(&o, ""), -1);
        MDATA_INF rn; Call(FindCmd(J2_TYPE, "取指针"), h, 1, &rn);
        EqI("销毁 后指针为 0", rn.m_int, 0);
        FreeRet(&rn);
        o.Delete();
    }
    {   // 未初始化对象的降级：读为空、写不崩
        J2 o; o.New();
        EqI("未初始化 取类型", J2Type(&o, "x"), -1);
        EqI("未初始化 取键数量", J2Count(&o, "x"), 0);
        MDATA_INF r;
        MDATA_INF a[2]; a[0] = o.Ref(); a[1] = TextArg("x");
        Call(FindCmd(J2_TYPE, "取文本"), a, 2, &r); EqS("未初始化 取文本为空", RetText(&r), ""); FreeRet(&r);
        Call(FindCmd(J2_TYPE, "到文本"), a, 2, &r); EqS("未初始化 到文本为空", RetText(&r), ""); FreeRet(&r);
        Call(FindCmd(J2_TYPE, "是否存在"), a, 2, &r); EqB("未初始化 是否存在假", r.m_bool, 0); FreeRet(&r);
        Call(FindCmd(J2_TYPE, "移除属性"), a, 2, &r); EqB("未初始化 移除属性假", r.m_bool, 0); FreeRet(&r);
        MDATA_INF b[3]; b[0] = o.Ref(); b[1] = TextArg("v"); b[2] = TextArg("p.q");
        Call(FindCmd(J2_TYPE, "置文本"), b, 3, &r);
        EqB("未初始化 置文本会懒建根", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "{\"p\":{\"q\":\"v\"}}", "懒建根后写入");
        o.Delete();
    }
}

// ============================================================ 测试：合并 / 排序 / 数组出入参
// ============================================================ 余下变体覆盖（前置声明见下）

// ============================================================ 余下 W 变体与标量写入逐条覆盖
static int Bool1(const char* szCmd, J2* o, MDATA_INF a1)
{
    MDATA_INF a[2]; a[0] = o->Ref(); a[1] = a1;
    MDATA_INF r; Call(FindCmd(J2_TYPE, szCmd), a, 2, &r);
    int b = r.m_bool; FreeRet(&r); return b;
}

static int Bool2(const char* szCmd, J2* o, MDATA_INF a1, MDATA_INF a2)
{
    MDATA_INF a[3]; a[0] = o->Ref(); a[1] = a1; a[2] = a2;
    MDATA_INF r; Call(FindCmd(J2_TYPE, szCmd), a, 3, &r);
    int b = r.m_bool; FreeRet(&r); return b;
}

static int Int1(const char* szCmd, J2* o, MDATA_INF a1)
{
    MDATA_INF a[2]; a[0] = o->Ref(); a[1] = a1;
    MDATA_INF r; Call(FindCmd(J2_TYPE, szCmd), a, 2, &r);
    int v = r.m_int; FreeRet(&r); return v;
}

static int Int2(const char* szCmd, J2* o, MDATA_INF a1, MDATA_INF a2)
{
    MDATA_INF a[3]; a[0] = o->Ref(); a[1] = a1; a[2] = a2;
    MDATA_INF r; Call(FindCmd(J2_TYPE, szCmd), a, 3, &r);
    int v = r.m_int; FreeRet(&r); return v;
}

static double Dbl1(const char* szCmd, J2* o, MDATA_INF a1)
{
    MDATA_INF a[2]; a[0] = o->Ref(); a[1] = a1;
    MDATA_INF r; Call(FindCmd(J2_TYPE, szCmd), a, 2, &r);
    double v = r.m_double; FreeRet(&r); return v;
}

static __int64 I64_1(const char* szCmd, J2* o, MDATA_INF a1)
{
    MDATA_INF a[2]; a[0] = o->Ref(); a[1] = a1;
    MDATA_INF r; Call(FindCmd(J2_TYPE, szCmd), a, 2, &r);
    __int64 v = r.m_int64; FreeRet(&r); return v;
}

static void Wide1(const char* szCmd, J2* o, MDATA_INF a1, wchar_t* out, int cap)
{
    MDATA_INF a[2]; a[0] = o->Ref(); a[1] = a1;
    MDATA_INF r; Call(FindCmd(J2_TYPE, szCmd), a, 2, &r);
    lstrcpynW(out, RetWide(&r), cap);
    FreeRet(&r);
}

// 取一个成员对象并断言其 到文本，随后释放
static void ChildTextIs(const char* szCmd, J2* o, MDATA_INF a1, MDATA_INF a2, const char* want, const char* what)
{
    MDATA_INF a[3]; a[0] = o->Ref(); a[1] = a1; a[2] = a2;
    MDATA_INF r; Call(FindCmd(J2_TYPE, szCmd), a, 3, &r);
    CxJson2* pH = (r.m_pCompoundData && *(int*)r.m_pCompoundData) ? (CxJson2*)(INT_PTR)*(int*)r.m_pCompoundData : NULL;
    if (pH == NULL || pH->node == NULL) { Fail("%s 没拿到对象", what); FreeRet(&r); return; }
    MDATA_INF arg; memset(&arg, 0, sizeof(arg));
    arg.m_dtDataType = MAKELONG(J2_TYPE + 1, 0);
    arg.m_pCompoundData = r.m_pCompoundData;
    MDATA_INF b[2]; b[0] = arg; b[1] = NullArg();
    MDATA_INF t; Call(FindCmd(J2_TYPE, "到文本"), b, 2, &t);
    EqS(what, RetText(&t), want);
    FreeRet(&t);
    MDATA_INF rf; Call(FindCmd(J2_TYPE, "Json2析构函数"), &arg, 1, &rf);
    free(r.m_pCompoundData);
    r.m_pCompoundData = NULL;            // 槽已释放，别让 FreeRet 再释放一次
    r.m_dtDataType = _SDT_NULL;
    FreeRet(&r);
}

// 返回任意类型的单参调用（调用方负责 FreeRet）
static MDATA_INF Call1(const char* szCmd, J2* o, MDATA_INF a1)
{
    MDATA_INF a[2]; a[0] = o->Ref(); a[1] = a1;
    MDATA_INF r; memset(&r, 0, sizeof(r));
    Call(FindCmd(J2_TYPE, szCmd), a, 2, &r);
    return r;
}

static void TestRemainingVariants()
{
    printf("\n[10] W 变体与标量写入逐条覆盖\n");
    {   // 解析 / 解析W：每次解析都会先清空目标节点
        J2 p; p.New();
        MDATA_INF a[5]; int err = 0, pos = 0;
        a[0] = p.Ref(); a[1] = TextArg("{\"n\":5}"); a[2] = NullArg();
        a[3] = RefIntArg(&err); a[4] = RefIntArg(&pos);
        MDATA_INF r; Call(FindCmd(J2_TYPE, "解析"), a, 5, &r);
        EqB("解析", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&p, "", "{\"n\":5}", "解析 之后");
        MDATA_INF b[5]; b[0] = p.Ref(); b[1] = BinArg(L"{\"m\":\"x\"}"); b[2] = NullArg();
        b[3] = RefIntArg(&err); b[4] = RefIntArg(&pos);
        Call(FindCmd(J2_TYPE, "解析W"), b, 5, &r);
        EqB("解析W", r.m_bool, 1); FreeRet(&r); FreeBin(&b[1]);
        ExpectJson(&p, "", "{\"m\":\"x\"}", "解析W 先清空再解析（与原版一致）");
        p.Delete();
    }

    J2 o; o.New();
    J2SetCode(&o, "{\"n\":5,\"m\":\"x\",\"arr\":[1],\"sub\":{\"k\":1}}");
    {   // 只读 W / UTF-8 变体
        wchar_t wbuf[128];
        EqI("取类型W", Int1("取类型W", &o, BinArg(L"m")), 4);
        EqI("取键数量W 标量为 0", Int1("取键数量W", &o, BinArg(L"m")), 0);
        EqI("取键数量W 数组", Int1("取键数量W", &o, BinArg(L"arr")), 1);
        Wide1("取源数据W", &o, BinArg(L"n"), wbuf, 128); EqW("取源数据W", wbuf, L"5");
        Wide1("取属性名W", &o, BinArg(L"m"), wbuf, 128); EqW("取属性名W", wbuf, L"m");
        EqB("是否存在W", Bool1("是否存在W", &o, BinArg(L"sub")), 1);
        EqB("是否存在W 未命中", Bool1("是否存在W", &o, BinArg(L"gone")), 0);
        EqB("取逻辑W 文本一律假", Bool1("取逻辑W", &o, BinArg(L"m")), 0);
        EqB("取逻辑W 数字", Bool1("取逻辑W", &o, BinArg(L"n")), 1);
        EqI("取双精度W", (int)Dbl1("取双精度W", &o, BinArg(L"n")), 5);
        EqI("取数值W", (int)Dbl1("取数值W", &o, BinArg(L"n")), 5);
        EqI("取长整数W", (int)I64_1("取长整数W", &o, BinArg(L"n")), 5);
        EqI("取整数W", Int1("取整数W", &o, BinArg(L"n")), 5);
        Wide1("取文本W", &o, BinArg(L"m"), wbuf, 128); EqW("取文本W", wbuf, L"x");
        {
            MDATA_INF r = Call1("取文本UTF8", &o, BinArg(L"m"));
            EqS("取文本UTF8 单个成员", RetText(&r), "x");
            FreeRet(&r);
        }
    }
    {   // K / I / 选择 / 取父 / 复制 的 W 与 UTF-8 变体
        ChildTextIs("KW", &o, BinArg(L"m"), BinArg(L""), "\"x\"", "KW");
        ChildTextIs("K2W", &o, BinArg(L"m"), BinArg(L""), "\"x\"", "K2W");
        ChildTextIs("K_UTF8", &o, TextArg("m"), TextArg(""), "\"x\"", "K_UTF8");
        ChildTextIs("选择W", &o, BinArg(L"sub"), NullArg(), "{\"k\":1}", "选择W");
        ChildTextIs("取父W", &o, BinArg(L"sub.k"), NullArg(), "{\"k\":1}", "取父W");
        ChildTextIs("IW", &o, IntArg(0), BinArg(L"arr"), "1", "IW");
        ChildTextIs("I2W", &o, IntArg(0), BinArg(L"arr"), "1", "I2W");
        ChildTextIs("复制W", &o, BinArg(L"sub"), NullArg(), "{\"k\":1}", "复制W");
        ChildTextIs("K", &o, TextArg("sub"), TextArg(""), "{\"k\":1}", "K");
    }
    {   // 写入族（路径一律显式给出，避免把根节点改掉）
        J2Child v;
        MDATA_INF txt = TextArg("W");
        J2CreateNew(&o, "创建文本", &v, &txt);
        {
            MDATA_INF pa[4]; pa[0] = o.Ref(); pa[1] = BinArg(L""); pa[2] = BinArg(L"pw"); pa[3] = NullArg();
            MDATA_INF pr; Call(FindCmd(J2_TYPE, "置属性W"), pa, 4, &pr);
            EqB("置属性W 值参数为空对象时返回假", pr.m_bool, 0); FreeRet(&pr);
            FreeBin(&pa[1]); FreeBin(&pa[2]);
        }
        MDATA_INF a4[4]; MDATA_INF rr;
        a4[0] = o.Ref(); a4[1] = BinArg(L""); a4[2] = BinArg(L"pw"); a4[3] = v.Ref();
        Call(FindCmd(J2_TYPE, "置属性W"), a4, 4, &rr);
        EqB("置属性W 带对象值", rr.m_bool, 1); FreeRet(&rr);
        MDATA_INF a5[5];
        a5[0] = o.Ref(); a5[1] = BinArg(L""); a5[2] = IntArg(0); a5[3] = BinArg(L"iw"); a5[4] = v.Ref();
        Call(FindCmd(J2_TYPE, "插入属性W"), a5, 5, &rr);
        EqB("插入属性W", rr.m_bool, 1); FreeRet(&rr);
        MDATA_INF a3[4];
        a3[0] = o.Ref(); a3[1] = BinArg(L"arr"); a3[2] = IntArg(1); a3[3] = v.Ref();
        Call(FindCmd(J2_TYPE, "插入成员W"), a3, 4, &rr);
        EqB("插入成员W", rr.m_bool, 1); FreeRet(&rr);
        MDATA_INF a3b[4];   // 置成员W 的参数序是（索引, 路径, 成员），与插入成员W 相反
        a3b[0] = o.Ref(); a3b[1] = IntArg(1); a3b[2] = BinArg(L"arr"); a3b[3] = v.Ref();
        Call(FindCmd(J2_TYPE, "置成员W"), a3b, 4, &rr);
        EqB("置成员W", rr.m_bool, 1); FreeRet(&rr);
        MDATA_INF a2[3];
        a2[0] = o.Ref(); a2[1] = BinArg(L"arr"); a2[2] = v.Ref();
        Call(FindCmd(J2_TYPE, "添加成员W"), a2, 3, &rr);
        EqB("添加成员W", rr.m_bool, 1); FreeRet(&rr);
        MDATA_INF a2b[3]; a2b[0] = o.Ref(); a2b[1] = IntArg(2); a2b[2] = BinArg(L"arr");   // arr 此刻 3 个成员
        Call(FindCmd(J2_TYPE, "移除成员W"), a2b, 3, &rr);
        EqB("移除成员W", rr.m_bool, 1); FreeRet(&rr);
        MDATA_INF rm[3]; rm[0] = o.Ref(); rm[1] = BinArg(L"pw"); rm[2] = BinArg(L"");
        Call(FindCmd(J2_TYPE, "移除属性W"), rm, 3, &rr);
        EqB("移除属性W", rr.m_bool, 1); FreeRet(&rr);
        MDATA_INF i2[3]; i2[0] = o.Ref(); i2[1] = IntArg(0); i2[2] = TextArg("arr");
        Call(FindCmd(J2_TYPE, "移除成员"), i2, 3, &rr);
        EqB("移除成员", rr.m_bool, 1); FreeRet(&rr);
        v.Free();

        EqB("置文本W", Bool2("置文本W", &o, BinArg(L"V"), BinArg(L"tw")), 1);
        { char u8[64]; EqB("置文本UTF8", Bool2("置文本UTF8", &o, TextArg(U8Of(L"中", u8, sizeof(u8))), BinArg(L"tu")), 1); }
        EqB("置文本", Bool2("置文本", &o, TextArg("T"), TextArg("ta")), 1);
        EqB("置空W", Bool1("置空W", &o, BinArg(L"nw")), 1);
        EqB("置空", Bool1("置空", &o, TextArg("n2")), 1);
        EqB("置逻辑", Bool2("置逻辑", &o, BoolArg(1), TextArg("nb")), 1);
        EqB("置逻辑W", Bool2("置逻辑W", &o, BoolArg(0), BinArg(L"nbw")), 1);
        EqB("置双精度", Bool2("置双精度", &o, DoubleArg(2.5), TextArg("da")), 1);
        EqB("置双精度W", Bool2("置双精度W", &o, DoubleArg(3.5), BinArg(L"daw")), 1);
        EqB("置数值", Bool2("置数值", &o, DoubleArg(4.5), TextArg("nu")), 1);
        EqB("置数值W", Bool2("置数值W", &o, DoubleArg(5.5), BinArg(L"nuw")), 1);
        EqB("置长整数", Bool2("置长整数", &o, Int64Arg(6), TextArg("lg")), 1);
        EqB("置长整数W", Bool2("置长整数W", &o, Int64Arg(7), BinArg(L"lgw")), 1);
        EqB("置整数", Bool2("置整数", &o, IntArg(8), TextArg("it")), 1);
        EqB("置整数W", Bool2("置整数W", &o, IntArg(9), BinArg(L"itw")), 1);
        EqB("置对象", Bool1("置对象", &o, TextArg("ob")), 1);
        EqB("置对象W", Bool1("置对象W", &o, BinArg(L"obw")), 1);
        EqB("置数组W", Bool1("置数组W", &o, BinArg(L"arw")), 1);
        EqB("置数组", Bool1("置数组", &o, TextArg("ar2")), 1);
        ExpectGetText(&o, "tw", "V", "置文本W 读回");
        { char gbk[64]; ExpectGetText(&o, "tu", GbkOf(L"中", gbk, sizeof(gbk)), "置文本UTF8 读回（出口按原版走 GBK）"); }
        ExpectGetText(&o, "it", "8", "置整数 读回");
        ExpectGetText(&o, "itw", "9", "置整数W 读回");
        ExpectGetText(&o, "daw", "3.5", "置双精度W 读回");
        ExpectJson(&o, "ob", "{}", "置对象 得到空对象");
        ExpectJson(&o, "arw", "[]", "置数组W 得到空数组");
    }
    o.Delete();
}

static int HostCmpNumberAsc(INT a, INT b, INT)
{
    CxJsonNode* x = (CxJsonNode*)(INT_PTR)a;
    CxJsonNode* y = (CxJsonNode*)(INT_PTR)b;
    double dx = (x && (x->type == CXJV_DOUBLE || x->type == CXJV_LONG)) ? x->v.d : 0.0;
    double dy = (y && (y->type == CXJV_DOUBLE || y->type == CXJV_LONG)) ? y->v.d : 0.0;
    if (dx < dy) return -1;
    if (dx > dy) return 1;
    return 0;
}

static void TestMergeSortArrays()
{
    printf("\n[7] 合并 / 排序 / 数组出入参\n");
    {   // 合并：递归同名键 + 缺失键按 是否添加 追加
        J2 o; o.New();
        J2SetCode(&o, "{\"a\":1,\"b\":{\"c\":2}}");
        MDATA_INF a[5]; MDATA_INF r;
        a[0] = o.Ref(); a[1] = TextArg("{\"b\":{\"c\":9,\"d\":8},\"e\":7}");
        a[2] = TextArg(""); a[3] = NullArg(); a[4] = BoolArg(1);
        Call(FindCmd(J2_TYPE, "合并"), a, 5, &r);
        EqB("合并", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "{\"a\":1,\"b\":{\"c\":9,\"d\":8},\"e\":7}", "合并 递归并追加新键");
        a[1] = TextArg("{\"x\":1}");
        a[4] = BoolArg(0);
        Call(FindCmd(J2_TYPE, "合并"), a, 5, &r); FreeRet(&r);
        ExpectJson(&o, "", "{\"a\":1,\"b\":{\"c\":9,\"d\":8},\"e\":7,\"x\":1}",
                   "合并 零命中时按原版走 合并对象 兜底");
        a[1] = TextArg("[1,2]");
        a[4] = BoolArg(1);
        Call(FindCmd(J2_TYPE, "合并"), a, 5, &r); FreeRet(&r);
        o.Delete();
    }
    {   // 合并W / 合并对象 / 合并对象Plus / 合并数组
        J2 o; o.New();
        J2SetCodeW(&o, L"{\"a\":1}");
        MDATA_INF a[5]; MDATA_INF r;
        a[0] = o.Ref(); a[1] = BinArg(L"{\"b\":2}"); a[2] = BinArg(L"");
        a[3] = NullArg(); a[4] = BoolArg(1);
        Call(FindCmd(J2_TYPE, "合并W"), a, 5, &r);
        EqB("合并W", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "{\"a\":1,\"b\":2}", "合并W");
        FreeBin(&a[1]); FreeBin(&a[2]);
        J2 src; src.New();
        J2SetCode(&src, "{\"b\":9,\"c\":8}");
        MDATA_INF b[3]; b[0] = o.Ref(); b[1] = src.Ref(); b[2] = TextArg("");
        Call(FindCmd(J2_TYPE, "合并对象"), b, 3, &r);
        EqB("合并对象", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "{\"a\":1,\"b\":9,\"c\":8}", "合并对象 浅覆盖同名键");
        Call(FindCmd(J2_TYPE, "合并对象Plus"), b, 3, &r);
        EqB("合并对象Plus 源是对象", r.m_bool, 1); FreeRet(&r);
        J2 arr; arr.New();
        J2SetCode(&arr, "{\"z\":1}");
        MDATA_INF d2[3]; d2[0] = o.Ref(); d2[1] = arr.Ref(); d2[2] = TextArg("");
        Call(FindCmd(J2_TYPE, "合并数组"), d2, 3, &r);
        EqB("合并数组 源不是数组返回假", r.m_bool, 0); FreeRet(&r);
        ExpectJson(&o, "", "{\"a\":1,\"b\":9,\"c\":8}", "合并数组 失败时不动目标");
        MDATA_INF d3[2]; d3[0] = arr.Ref(); d3[1] = TextArg("");
        MDATA_INF rd3; Call(FindCmd(J2_TYPE, "置数组"), d3, 2, &rd3); FreeRet(&rd3);
        ExpectJson(&arr, "", "[]", "置数组 丢掉对象内容");
        Call(FindCmd(J2_TYPE, "合并对象Plus"), d2, 3, &r);
        EqB("合并对象Plus 源是数组时回落数组语义", r.m_bool, 1); FreeRet(&r);
        ExpectJson(&o, "", "[]", "合并对象Plus 把对象目标转成数组");
        src.Delete(); arr.Delete(); o.Delete();
    }
    {   // 排序（用户回调）
        J2 o; o.New();
        J2SetCode(&o, "[3,1,2]");
        ExpectJson(&o, "", "[3,1,2]", "排序前");
        MDATA_INF a[5]; MDATA_INF r;
        a[0] = o.Ref(); a[1] = TextArg("");
        a[2] = IntArg((int)(INT_PTR)&HostCmpNumberAsc);
        a[3] = IntArg(0); a[4] = IntArg(0);
        Call(FindCmd(J2_TYPE, "排序"), a, 5, &r); FreeRet(&r);
        ExpectJson(&o, "", "[1,2,3]", "排序 按回调升序");
        MDATA_INF b[5]; b[0] = o.Ref(); b[1] = BinArg(L"");
        b[2] = a[2]; b[3] = NullArg(); b[4] = NullArg();
        Call(FindCmd(J2_TYPE, "排序W"), b, 5, &r); FreeRet(&r);
        FreeBin(&b[1]);
        ExpectJson(&o, "", "[1,2,3]", "排序W 幂等");
        o.Delete();
    }
    {   // 取所有键 / 取所有值
        J2 o; o.New();
        J2SetCode(&o, "{\"k1\":1,\"k2\":[7],\"k3\":\"x\"}");
        MDATA_INF a[3]; MDATA_INF r;
        a[0] = o.Ref(); a[1] = NullArg(); a[2] = TextArg("");
        Call(FindCmd(J2_TYPE, "取所有键"), a, 3, &r);
        EqI("取所有键 留空＝查数量", r.m_int, 3); FreeRet(&r);

        void* pText = NULL;
        MDATA_INF ary;
        RefAryArg(&ary, &pText, SDT_TEXT);
        pText = MakeAryBlock(0, 4);
        a[1] = ary;
        Call(FindCmd(J2_TYPE, "取所有键"), a, 3, &r);
        EqI("取所有键 填充数量", r.m_int, 3); FreeRet(&r);
        EqI("取所有键 数组长度", AryCount(pText), 3);
        if (AryCount(pText) == 3)
        {
            char** pp = (char**)((char*)pText + 8);
            EqS("取所有键[0]", pp[0], "k1");
            EqS("取所有键[1]", pp[1], "k2");
            EqS("取所有键[2]", pp[2], "k3");
        }
        AryFreePtrs(pText, AryCount(pText));
        pText = NULL;

        void* pBin = NULL;
        MDATA_INF aryW;
        RefAryArg(&aryW, &pBin, SDT_BIN);
        MDATA_INF b[3]; b[0] = o.Ref(); b[1] = aryW; b[2] = BinArg(L"");
        Call(FindCmd(J2_TYPE, "取所有键W"), b, 3, &r);
        EqI("取所有键W 填充数量", r.m_int, 3); FreeRet(&r);
        if (pBin)
        {
            int nW = AryCount(pBin);
            EqI("取所有键W 数组长度", nW, 3);
            char** pp = (char**)((char*)pBin + 8);
            const wchar_t* exp[3] = { L"k1", L"k2", L"k3" };
            for (int i = 0; i < nW && i < 3; i++)
            {
                if (pp[i] == NULL) { Fail("取所有键W 第 %d 个元素为空", i); continue; }
                int nb = *(int*)(pp[i] + 4);
                wchar_t* w = (wchar_t*)(pp[i] + 8);
                if (nb >= 2) w[nb / 2 - 1] = 0;
                EqW("取所有键W 元素", w, exp[i]);
            }
            AryFreePtrs(pBin, nW);
        }
        FreeBin(&b[2]);

        void* pVals = NULL;
        MDATA_INF vals;
        RefAryArg(&vals, &pVals, MAKELONG(CX_TYPE_JSON2, 0));
        MDATA_INF c[3]; c[0] = o.Ref(); c[1] = TextArg(""); c[2] = vals;
        Call(FindCmd(J2_TYPE, "取所有值"), c, 3, &r);
        EqI("取所有值 数量", r.m_int, 3); FreeRet(&r);
        if (pVals)
        {
            int nV = AryCount(pVals);
            EqI("取所有值 数组长度", nV, 3);
            void** pp = (void**)((char*)pVals + 8);
            static const int expType[3] = { 1, 5, 4 };
            for (int i = 0; i < nV && i < 3; i++)
            {
                MDATA_INF arg; memset(&arg, 0, sizeof(arg));
                arg.m_dtDataType = MAKELONG(J2_TYPE + 1, 0);
                arg.m_pCompoundData = pp[i];
                MDATA_INF d[2]; d[0] = arg; d[1] = TextArg("");
                MDATA_INF rd; Call(FindCmd(J2_TYPE, "取类型"), d, 2, &rd);
                EqI("取所有值 元素类型", rd.m_int, expType[i]);
                FreeRet(&rd);
                MDATA_INF rf; Call(FindCmd(J2_TYPE, "Json2析构函数"), &arg, 1, &rf);
                free(pp[i]);
                pp[i] = NULL;
            }
            free(pVals);
        }
        void* pValsW = NULL;
        MDATA_INF valsW;
        RefAryArg(&valsW, &pValsW, MAKELONG(CX_TYPE_JSON2, 0));
        MDATA_INF e2[3]; e2[0] = o.Ref(); e2[1] = BinArg(L""); e2[2] = valsW;
        Call(FindCmd(J2_TYPE, "取所有值W"), e2, 3, &r);
        EqI("取所有值W 数量", r.m_int, 3); FreeRet(&r);
        if (pValsW)
        {
            int nV = AryCount(pValsW);
            void** pp = (void**)((char*)pValsW + 8);
            for (int i = 0; i < nV; i++)
            {
                MDATA_INF arg; memset(&arg, 0, sizeof(arg));
                arg.m_dtDataType = MAKELONG(J2_TYPE + 1, 0);
                arg.m_pCompoundData = pp[i];
                MDATA_INF rf; Call(FindCmd(J2_TYPE, "Json2析构函数"), &arg, 1, &rf);
                free(pp[i]);
            }
            free(pValsW);
        }
        FreeBin(&e2[1]);
        o.Delete();
    }
    {   // 置X数组
        J2 o; o.New();
        MDATA_INF r;
        {   // 文本数组
            void* blk = MakeAryBlock(3, 4);
            ArySetPtr(blk, 0, AryNewElemText("a"));
            ArySetPtr(blk, 1, AryNewElemText("b"));
            ArySetPtr(blk, 2, AryNewElemText("c"));
            MDATA_INF ary;
            DataAryArg(&ary, blk, SDT_TEXT);
            MDATA_INF a[2]; a[0] = o.Ref(); a[1] = ary;
            Call(FindCmd(J2_TYPE, "置文本数组"), a, 2, &r); FreeRet(&r);
            ExpectJson(&o, "", "[\"a\",\"b\",\"c\"]", "置文本数组");
            AryFreePtrs(blk, 3);
        }
        {   // 宽文本数组
            void* blk = MakeAryBlock(2, 4);
            ArySetPtr(blk, 0, AryNewElemBin(L"中"));
            ArySetPtr(blk, 1, AryNewElemBin(L"w"));
            MDATA_INF ary;
            DataAryArg(&ary, blk, SDT_BIN);
            MDATA_INF a[2]; a[0] = o.Ref(); a[1] = ary;
            Call(FindCmd(J2_TYPE, "置文本数组W"), a, 2, &r); FreeRet(&r);
            { char gbk[128]; ExpectJson(&o, "", GbkOf(L"[\"中\",\"w\",\"c\"]", gbk, sizeof(gbk)), "置文本数组W 不截断原成员"); }
            AryFreePtrs(blk, 2);
        }
        {   // UTF-8 文本数组
            void* blk = MakeAryBlock(1, 4);
            { char u8[64]; ArySetPtr(blk, 0, AryNewElemText(U8Of(L"中", u8, sizeof(u8)))); }
            MDATA_INF ary;
            DataAryArg(&ary, blk, SDT_TEXT);
            MDATA_INF a[2]; a[0] = o.Ref(); a[1] = ary;
            Call(FindCmd(J2_TYPE, "置文本数组UTF8"), a, 2, &r); FreeRet(&r);
            { char gbk[128]; ExpectJson(&o, "", GbkOf(L"[\"中\",\"w\",\"c\"]", gbk, sizeof(gbk)), "置文本数组UTF8 不截断原成员"); }
            AryFreePtrs(blk, 1);
        }
        {   // 双精度数组（元素内联 8 字节）
            void* blk = MakeAryBlock(2, 8);
            double* pD = (double*)((char*)blk + 8);
            pD[0] = 1.5; pD[1] = 2.5;
            MDATA_INF ary;
            DataAryArg(&ary, blk, SDT_DOUBLE);
            MDATA_INF a[2]; a[0] = o.Ref(); a[1] = ary;
            Call(FindCmd(J2_TYPE, "置数值数组UTF8"), a, 2, &r); FreeRet(&r);
            ExpectJson(&o, "", "[1.5,2.5,\"c\"]", "置数值数组UTF8 不截断原成员");
            free(blk);
        }
        {   // 整数数组（元素内联 4 字节）
            void* blk = MakeAryBlock(2, 4);
            int* pI = (int*)((char*)blk + 8);
            pI[0] = 7; pI[1] = 8;
            MDATA_INF ary;
            DataAryArg(&ary, blk, SDT_INT);
            MDATA_INF a[2]; a[0] = o.Ref(); a[1] = ary;
            Call(FindCmd(J2_TYPE, "置整数数组UTF8"), a, 2, &r); FreeRet(&r);
            ExpectJson(&o, "", "[7,8,\"c\"]", "置整数数组UTF8 不截断原成员");
            free(blk);
        }
        {   // 逻辑数组（元素内联 2 字节 DTBOOL，真＝-1）
            void* blk = MakeAryBlock(2, sizeof(DTBOOL));
            DTBOOL* pB = (DTBOOL*)((char*)blk + 8);
            pB[0] = -1; pB[1] = 0;
            MDATA_INF ary;
            DataAryArg(&ary, blk, SDT_BOOL);
            MDATA_INF a[2]; a[0] = o.Ref(); a[1] = ary;
            Call(FindCmd(J2_TYPE, "置逻辑数组UTF8"), a, 2, &r); FreeRet(&r);
            ExpectJson(&o, "", "[true,false,\"c\"]", "置逻辑数组UTF8 不截断原成员");
            free(blk);
        }
        o.Delete();
    }
}

// ============================================================ 测试：生命周期与引用共享
static void TestLifecycle()
{
    printf("\n[8] 构造 / 复制 / 析构 与共享语义\n");
    J2 a; a.New();
    J2SetCode(&a, "{\"a\":1}");
    J2 b; b.New();
    MDATA_INF c[2]; c[0] = b.Ref(); c[1] = a.Ref();
    MDATA_INF r;
    Call(FindCmd(J2_TYPE, "Json2复制函数"), c, 2, &r); FreeRet(&r);
    ExpectJson(&b, "", "{\"a\":1}", "复制函数 得到同一棵树");
    MDATA_INF k[2]; k[0] = b.Ref(); k[1] = TextArg("n");
    Call(FindCmd(J2_TYPE, "置空"), k, 2, &r); FreeRet(&r);
    ExpectJson(&a, "", "{\"a\":1,\"n\":null}", "复制出的变量与原变量共享节点");
    b.Delete();
    ExpectJson(&a, "", "{\"a\":1,\"n\":null}", "复制变量析构不影响原变量");
    a.Delete();
}

// ============================================================ 台账
static void TestLedger()
{
    printf("\n[9] 覆盖台账（每条命令都必须被真跑到）\n");
    PLIB_DATA_TYPE_INFO d = &g_inf->m_pDataType[J2_TYPE];
    int nMiss = 0;
    for (int i = 0; i < d->m_nCmdCount; i++)
    {
        int idx = d->m_pnCmdsIndex[i];
        if (g_hit[idx] == 0)
        {
            Fail("命令「%s」(编号 %d) 没有被任何断言跑到",
                 AS_TEXT(g_inf->m_pBeginCmdInfo[idx].m_szName), idx);
            nMiss++;
        }
    }
    Ok("Json2 成员命令 %d 条，未覆盖 %d 条", d->m_nCmdCount, nMiss);
    if (nMiss != 0) g_failed++;
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
    if (!pGetNewInf) { printf("[FAIL] 找不到 GetNewInf\n"); return 1; }
    g_inf = pGetNewInf();
    g_hMod = h;
    if (!g_inf) { printf("[FAIL] GetNewInf 返回空\n"); return 1; }
    g_inf->m_pfnNotify(NL_SYS_NOTIFY_FUNCTION, (DWORD)(INT_PTR)&FakeNotifySys, 0);

    printf("[host_json] %s  命令 %d 条 / 类 %d 个\n", dllPath, g_inf->m_nCmdCount, g_inf->m_nDataTypeCount);
    if (g_inf->m_nDataTypeCount != 25) Fail("类型数量应为 25（包含取消令牌），实际 %d", g_inf->m_nDataTypeCount);
    int t = FindType("Json2");
    if (t != J2_TYPE) Fail("Json2 应排在第 9 个类，实际下标 %d", t);
    PLIB_DATA_TYPE_INFO d = &g_inf->m_pDataType[J2_TYPE];
    if (d->m_nCmdCount != 115) Fail("Json2 成员命令应为 115 条（构造/复制/析构 + 112 方法），实际 %d", d->m_nCmdCount);
    // 常量表：3.4 的 15 条 Json2 常量逐条按名核值；curl 常量族只核数量下限（值由 host_const 核）。
    static const struct { const char* name; double value; } kJson2Consts[] = {
        { "JV类型_未定义", -1 }, { "JV类型_空", 0 }, { "JV类型_双精度型", 1 },
        { "JV类型_长整数型", 2 }, { "JV类型_逻辑型", 3 }, { "JV类型_文本型", 4 },
        { "JV类型_数组", 5 }, { "JV类型_对象", 6 }, { "JSON_OK", 0 },
        { "JSON_ERROR", -1 }, { "JSON_INVALIDARG", -2 }, { "JSON_MEMORY", -3 },
        { "JSON_INVALIDNAME", -4 }, { "JSON_OBJECTPARENT", -5 }, { "JSON_INVALIDVALUE", -6 },
    };
    int nCurlConst = 0;
    for (int i = 0; i < g_inf->m_nLibConstCount; i++)
    {
        if (strncmp(AS_TEXT(g_inf->m_pLibConst[i].m_szName), "CURL", 4) == 0) nCurlConst++;
    }
    for (unsigned k = 0; k < sizeof(kJson2Consts) / sizeof(kJson2Consts[0]); k++)
    {
        int found = -1;
        for (int i = 0; i < g_inf->m_nLibConstCount; i++)
        {
            PLIB_CONST_INFO c = &g_inf->m_pLibConst[i];
            if (strcmp(AS_TEXT(c->m_szName), kJson2Consts[k].name) == 0) { found = i; break; }
        }
        if (found < 0) { Fail("缺常量 %s", kJson2Consts[k].name); continue; }
        double got = g_inf->m_pLibConst[found].m_dbValue;
        if (got != kJson2Consts[k].value) Fail("%s 值应为 %g，实际 %g", kJson2Consts[k].name, kJson2Consts[k].value, got);
    }
    Ok("Json2 常量 15 条按名核值通过；curl 常量族 %d 条", nCurlConst);
    if (nCurlConst < 800) Fail("curl 常量族只剩 %d 条，疑似被裁", nCurlConst);

    TestParse();
    TestLenientSyntax();
    TestUnicodeAndEncodings();
    TestTypesAndCoercion();
    TestPaths();
    TestMutation();
    TestMergeSortArrays();
    TestLifecycle();
    TestRemainingVariants();
    TestLedger();

    if (g_runtimeErr) Fail("出现 %d 次运行时错误", g_runtimeErr);
    printf("\n[host_json] 断言失败 %d 项，运行时错误 %d 次\n", g_failed, g_runtimeErr);
    printf("%s\n", g_failed == 0 ? "结论: Json2 语义断言全部通过" : "结论: 有不合格项");
    return g_failed == 0 ? 0 : 1;
}
