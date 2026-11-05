// 离线宿主：不经易语言 IDE，直接装载 cxsoft.fne，校验库信息表并驱动 会话/请求/响应 三个类。
// 命令一律按中文名在类型自己的命令索引表里查找，改表号不用改宿主。
// 编译并运行见 build_host.bat。
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <tchar.h>

typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;

#include <lib2.h>
#include <lang.h>

#define AS_TEXT(p) ((const char*)(p))

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

void FreeRet(MDATA_INF* p)
{
    if (p->m_dtDataType == SDT_TEXT && p->m_pText) free(p->m_pText);
    if (p->m_dtDataType == SDT_BIN && p->m_pBin) free(p->m_pBin);
    memset(p, 0, sizeof(MDATA_INF));
}

typedef PLIB_INFO(WINAPI* fn_GetNewInf)(void);

// 一个易语言对象：系统给的是一个 INT 成员，我们用它存 C++ 结构指针。
struct Obj
{
    MDATA_INF self;
    INT slot;
    Obj() { memset(this, 0, sizeof(*this)); }
    void Bind() { self.m_dtDataType = SDT_INT; self.m_pCompoundData = (void*)&slot; }
    bool Alive() const { return slot != 0; }
};

static PLIB_INFO g_inf = NULL;

MDATA_INF TextArg(const char* s) { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = SDT_TEXT; m.m_pText = (char*)s; return m; }
MDATA_INF IntArg(int v) { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = SDT_INT; m.m_int = v; return m; }
MDATA_INF BoolArg(bool v) { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = SDT_BOOL; m.m_bool = v ? TRUE : FALSE; return m; }
MDATA_INF ObjArg(const Obj& o) { MDATA_INF m; memset(&m, 0, sizeof(m)); m.m_dtDataType = MAKELONG(1, 0); m.m_pCompoundData = (void*)&o.slot; return m; }

// 在指定类型（0 起）的成员命令里按中文名找下标
INT FindCmd(INT typeNo, const char* cnName)
{
    if (typeNo < 0 || typeNo >= g_inf->m_nDataTypeCount) { Fail("类型序号 %d 越界", typeNo); return -1; }
    PLIB_DATA_TYPE_INFO d = &g_inf->m_pDataType[typeNo];
    for (int i = 0; i < d->m_nCmdCount; i++)
    {
        INT idx = d->m_pnCmdsIndex[i];
        if (idx >= 0 && idx < g_inf->m_nCmdCount && lstrcmpA(AS_TEXT(g_inf->m_pBeginCmdInfo[idx].m_szName), cnName) == 0)
            return idx;
    }
    Fail("类型 %d 里找不到命令「%s」", typeNo + 1, cnName);
    return -1;
}

void CallSelf(INT cmd, Obj& o)
{
    MDATA_INF a[1]; a[0] = o.self;
    MDATA_INF ret; memset(&ret, 0, sizeof(ret));
    g_inf->m_pCmdsFunc[cmd](&ret, 1, a);
    FreeRet(&ret);
}

void CallSelfText(INT cmd, Obj& o, const char* text)
{
    MDATA_INF a[2]; a[0] = o.self; a[1] = TextArg(text);
    MDATA_INF ret; memset(&ret, 0, sizeof(ret));
    g_inf->m_pCmdsFunc[cmd](&ret, 2, a);
    FreeRet(&ret);
}

void CallGlobalText(INT cmd, const char* text, char* pOut, int nOutCap)
{
    MDATA_INF a[1]; a[0] = TextArg(text);
    MDATA_INF ret; memset(&ret, 0, sizeof(ret));
    g_inf->m_pCmdsFunc[cmd](&ret, 1, a);
    if (pOut && nOutCap > 0) lstrcpynA(pOut, ret.m_pText ? ret.m_pText : "", nOutCap);
    FreeRet(&ret);
}

INT CallSelfInt(INT cmd, Obj& o)
{
    MDATA_INF a[1]; a[0] = o.self;
    MDATA_INF ret; memset(&ret, 0, sizeof(ret));
    g_inf->m_pCmdsFunc[cmd](&ret, 1, a);
    const INT v = ret.m_int;
    FreeRet(&ret);
    return v;
}

// ============================================================ 宽文本/正则 测试辅助

static wchar_t g_wStore[8192];
static int g_wUsed = 0;

// 用码点构造宽文本夹具，避开源文件编码带来的歧义。
static const wchar_t* W(int nFirst, ...)
{
    va_list ap; va_start(ap, nFirst);
    wchar_t* p = g_wStore + g_wUsed;
    int i = 0;
    int c = nFirst;
    while (c != 0 && i < 512) { p[i++] = (wchar_t)c; c = va_arg(ap, int); }
    p[i] = 0;
    g_wUsed += i + 1;
    va_end(ap);
    return p;
}

static void WToUtf8(const wchar_t* pw, char* out, int cap)
{
    if (!pw) { out[0] = 0; return; }
    int n = WideCharToMultiByte(CP_UTF8, 0, pw, -1, out, cap, NULL, NULL);
    if (n <= 0) out[0] = 0;
}

// 字节集(宽文本) 返回值 -> UTF-8 文本，便于断言与打印
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
    if (n > 0) { tmp[n < cap ? n : cap - 1] = 0; lstrcpynA(out, tmp, cap); }
    free(tmp);
}

// 宽文本字节集参数（末尾带 {0,0}，与模块惯例一致）
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
        int nBytes = *(int*)((char*)e + 4);
        MDATA_INF m; memset(&m, 0, sizeof(m));
        m.m_dtDataType = SDT_BIN; m.m_pBin = (LPBYTE)e;
        RetBinToUtf8(&m, out, cap);
        UNREFERENCED_PARAMETER(nBytes);
    }
    const wchar_t* ElemW(int i) const
    {
        void* e = Elem(i);
        return (const wchar_t*)((char*)e + 8);
    }
};

static void* MakeElemW(const wchar_t* pw)
{
    int nBytes = (int)(lstrlenW(pw) + 1) * 2;
    char* p = (char*)malloc(8 + nBytes);
    *(int*)p = 1; *(int*)(p + 4) = nBytes;
    memcpy(p + 8, pw, nBytes);
    return p;
}

// 按中文名找全局命令
INT FindGlobalCmd(const char* cnName)
{
    for (int i = 0; i < g_inf->m_nCmdCount; i++)
        if (g_inf->m_pBeginCmdInfo[i].m_shtCategory > 0 &&
            lstrcmpA(AS_TEXT(g_inf->m_pBeginCmdInfo[i].m_szName), cnName) == 0)
            return i;
    Fail("找不到全局命令「%s」", cnName);
    return -1;
}

// 宽文本入 -> 宽文本出
static void CallUu(INT cmd, const wchar_t* a1, const wchar_t* a2, char* out, int cap)
{
    MDATA_INF arg[2]; arg[0] = BinWArg(a1); arg[1] = BinWArg(a2);
    MDATA_INF ret; memset(&ret, 0, sizeof(ret));
    g_inf->m_pCmdsFunc[cmd](&ret, 2, arg);
    RetBinToUtf8(&ret, out, cap);
    FreeRet(&ret);
    FreeBinArg(&arg[0]); FreeBinArg(&arg[1]);
}

static int CallUiInt(INT cmd, const wchar_t* a1, const wchar_t* a2)
{
    MDATA_INF arg[2]; arg[0] = BinWArg(a1); arg[1] = BinWArg(a2);
    MDATA_INF ret; memset(&ret, 0, sizeof(ret));
    g_inf->m_pCmdsFunc[cmd](&ret, 2, arg);
    int v = ret.m_int;
    FreeRet(&ret);
    FreeBinArg(&arg[0]); FreeBinArg(&arg[1]);
    return v;
}

static int CallU1Int(INT cmd, const wchar_t* a1)
{
    MDATA_INF arg[1]; arg[0] = BinWArg(a1);
    MDATA_INF ret; memset(&ret, 0, sizeof(ret));
    g_inf->m_pCmdsFunc[cmd](&ret, 1, arg);
    int v = ret.m_int;
    FreeRet(&ret);
    FreeBinArg(&arg[0]);
    return v;
}

static void CallU1(INT cmd, const wchar_t* a1, char* out, int cap)
{
    MDATA_INF arg[1]; arg[0] = BinWArg(a1);
    MDATA_INF ret; memset(&ret, 0, sizeof(ret));
    g_inf->m_pCmdsFunc[cmd](&ret, 1, arg);
    RetBinToUtf8(&ret, out, cap);
    FreeRet(&ret);
    FreeBinArg(&arg[0]);
}

void CallSelfTextRet(INT cmd, Obj& o, char* pOut, int nOutCap)
{
    MDATA_INF a[1]; a[0] = o.self;
    MDATA_INF ret; memset(&ret, 0, sizeof(ret));
    g_inf->m_pCmdsFunc[cmd](&ret, 1, a);
    if (pOut && nOutCap > 0) lstrcpynA(pOut, ret.m_pText ? ret.m_pText : "", nOutCap);
    FreeRet(&ret);
}

int main(int argc, char** argv)
{
    char dllPath[MAX_PATH] = { 0 };
    if (argc > 1) lstrcpynA(dllPath, argv[1], MAX_PATH);
    else
    {
        char dir[MAX_PATH];
        GetModuleFileNameA(NULL, dir, MAX_PATH);
        char* p = strrchr(dir, '\\');
        if (p) *p = '\0';
        wsprintfA(dllPath, "%s\\cxsoft.fne", dir);
    }

    HMODULE h = LoadLibraryA(dllPath);
    if (h == NULL) { printf("[FAIL] 加载 %s 失败，错误码 %lu\n", dllPath, GetLastError()); return 1; }
    printf("[OK] 已加载 %s\n", dllPath);

    fn_GetNewInf pGetNewInf = (fn_GetNewInf)GetProcAddress(h, "GetNewInf");
    if (!pGetNewInf) { Fail("找不到 GetNewInf"); return 1; }
    g_inf = pGetNewInf();
    if (!g_inf) { Fail("GetNewInf 返回空"); return 1; }

    // ---------------- 表结构自洽 ----------------
    if (g_inf->m_dwLibFormatVer != LIB_FORMAT_VER) Fail("库格式号不匹配");
    if (g_inf->m_szGuid == NULL || lstrlen(g_inf->m_szGuid) != 32) Fail("GUID 长度不是 32");
    if (g_inf->m_nDataTypeCount != 25) Fail("应为 25 个类型（包含取消令牌），实际 %d", g_inf->m_nDataTypeCount);

    printf("库名: %s  GUID: %s  版本: %d.%d.%d  命令: %d  类: %d  类别: %d\n",
        AS_TEXT(g_inf->m_szName), AS_TEXT(g_inf->m_szGuid), g_inf->m_nMajorVersion,
        g_inf->m_nMinorVersion, g_inf->m_nBuildNumber, g_inf->m_nCmdCount,
        g_inf->m_nDataTypeCount, g_inf->m_nCategoryCount);

    const char** names = (const char**)(INT_PTR)g_inf->m_pfnNotify(NL_GET_CMD_FUNC_NAMES, 0, 0);
    if (names == NULL) Fail("没有函数名表");

    int nGlobal = 0;
    for (int i = 0; i < g_inf->m_nCmdCount; i++)
    {
        PCMD_INFO c = &g_inf->m_pBeginCmdInfo[i];
        if (c->m_szName == NULL || c->m_szName[0] == 0) Fail("命令 %d 无名", i);
        if (g_inf->m_pCmdsFunc[i] == NULL) Fail("命令 %d 无实现", i);
        if (names != NULL && names[i] == NULL) Fail("命令 %d 无名表项", i);
        if (c->m_shtCategory == -1) continue;
        nGlobal++;
        if (c->m_shtCategory < 1 || c->m_shtCategory > g_inf->m_nCategoryCount)
            Fail("全局命令 %d(%s) 类别 %d 越界", i, AS_TEXT(c->m_szName), c->m_shtCategory);
        for (int a = 0; a < c->m_nArgCount; a++)
        {
            PARG_INFO p = &c->m_pBeginArgInfo[a];
            if (p->m_szName == NULL) Fail("命令 %d 参数 %d 无名", i, a);
            if ((p->m_dwState & AS_HAS_DEFAULT_VALUE) && (p->m_dwState & AS_DEFAULT_VALUE_IS_EMPTY))
                Fail("命令 %d 参数 %d 默认值标志互斥", i, a);
        }
    }
    printf("[OK] 全局命令 %d 条\n", nGlobal);

    // 每个类的成员命令：索引合法、构造/复制/析构各一个、参数里引用的类型序号合法
    int seenCmd[1024] = { 0 };
    for (int t = 0; t < g_inf->m_nDataTypeCount; t++)
    {
        PLIB_DATA_TYPE_INFO d = &g_inf->m_pDataType[t];
        printf("类 %d: %s (%s) 成员命令 %d 条\n", t + 1, AS_TEXT(d->m_szName),
            d->m_szEgName ? AS_TEXT(d->m_szEgName) : "", d->m_nCmdCount);
        if (d->m_nCmdCount == 0)
        {
            // 纯数据类型（如 CURL_证书信息）：只校验成员定义，不要求隐藏指针与生命周期三件套
            if (d->m_nElementCount < 1 || d->m_pElementBegin == NULL)
                Fail("纯数据类型 %d 没有成员定义", t + 1);
            continue;
        }
        if (d->m_nElementCount != 1) Fail("类 %d 应有 1 个隐藏成员", t);
        int nC = 0, nF = 0, nCopy = 0;
        for (int i = 0; i < d->m_nCmdCount; i++)
        {
            INT idx = d->m_pnCmdsIndex[i];
            if (idx < 0 || idx >= g_inf->m_nCmdCount) { Fail("类 %d 索引 %d 越界", t, idx); continue; }
            if (idx >= (int)(sizeof(seenCmd) / sizeof(seenCmd[0]))) { Fail("命令表过大"); break; }
            if (seenCmd[idx]) Fail("命令 %d 被多个类重复登记", idx);
            seenCmd[idx] = 1;
            PCMD_INFO c = &g_inf->m_pBeginCmdInfo[idx];
            if (c->m_shtCategory != -1) Fail("类 %d 的索引 %d 不是成员命令", t, idx);
            if (c->m_wState & CT_IS_OBJ_CONSTURCT_CMD) nC++;
            if (c->m_wState & CT_IS_OBJ_FREE_CMD) nF++;
            if (c->m_wState & CT_IS_OBJ_COPY_CMD) nCopy++;
            for (int a = 0; a < c->m_nArgCount; a++)
            {
                DATA_TYPE dt = c->m_pBeginArgInfo[a].m_dtType;
                if ((dt & DTM_SYS_DATA_TYPE_MASK) == 0 && (dt & DTM_LIB_DATA_TYPE_MASK) == 0
                    && HIWORD(dt) == 0 && LOWORD(dt) != 0)
                {
                    INT typeRef = LOWORD(dt);
                    if (typeRef > g_inf->m_nDataTypeCount)
                        Fail("类 %d 命令 %s 参数 %d 引用了不存在的类型 %d", t, AS_TEXT(c->m_szName), a, typeRef);
                }
            }
        }
        if (nC != 1) Fail("类 %d 构造函数应恰为 1，实际 %d", t, nC);
        if (nF != 1) Fail("类 %d 析构函数应恰为 1，实际 %d", t, nF);
        if (nCopy != 1) Fail("类 %d 复制函数应恰为 1，实际 %d", t, nCopy);
    }
    for (int i = 0; i < g_inf->m_nCmdCount; i++)
        if (i < (int)(sizeof(seenCmd) / sizeof(seenCmd[0])) && g_inf->m_pBeginCmdInfo[i].m_shtCategory == -1 && !seenCmd[i])
            Fail("成员命令 %d(%s) 没有归属任何类", i, AS_TEXT(g_inf->m_pBeginCmdInfo[i].m_szName));

    // ---------------- 实跑 ----------------
    g_inf->m_pfnNotify(NL_SYS_NOTIFY_FUNCTION, (DWORD)(INT_PTR)&FakeNotifySys, 0);
    char text[4096];

    printf("\n---- 全局命令 ----\n");
    MDATA_INF ret;
    memset(&ret, 0, sizeof(ret));
    MDATA_INF nullArg; memset(&nullArg, 0, sizeof(nullArg)); nullArg.m_dtDataType = _SDT_NULL;
    g_inf->m_pCmdsFunc[0](&ret, 1, &nullArg);
    if (!ret.m_pText || strcmp(ret.m_pText, "helloworld") != 0) Fail("测试命令返回异常");
    printf("测试命令 -> %s\n", ret.m_pText);
    FreeRet(&ret);

    memset(&ret, 0, sizeof(ret));
    g_inf->m_pCmdsFunc[1](&ret, 0, NULL);            // 取指纹型号列表
    int nLines = 0;
    if (ret.m_pText) for (const char* s = ret.m_pText; *s; s++) if (*s == '\n') nLines++;
    printf("指纹型号 -> %d 行\n", nLines);
    if (nLines < 10) Fail("指纹型号太少");
    FreeRet(&ret);

    memset(&ret, 0, sizeof(ret));
    g_inf->m_pCmdsFunc[2](&ret, 0, NULL);
    printf("网络引擎 -> %s\n", ret.m_pText);
    if (!ret.m_pText || !strstr(ret.m_pText, "IMPERSONATE")) Fail("引擎信息里没有 IMPERSONATE");
    FreeRet(&ret);

    CallGlobalText(3, "a b&c=中文", text, sizeof(text));
    printf("URL编码 -> %s\n", text);
    if (!strstr(text, "%")) Fail("URL编码 没有产生百分号转义");
    CallGlobalText(4, text, text, sizeof(text));
    printf("URL解码 -> %s\n", text);

    CallGlobalText(5, "中文", text, sizeof(text));
    printf("UTF8转GBK -> %s\n", text);

    const char* proxy = getenv("PROXY") ? getenv("PROXY") : "http://127.0.0.1:10809";
    {
        MDATA_INF a[3]; a[0] = TextArg("https://www.wikipedia.org"); a[1] = TextArg("chrome133a"); a[2] = TextArg(proxy);
        memset(&ret, 0, sizeof(ret));
        g_inf->m_pCmdsFunc[6](&ret, 3, a);
        printf("快速请求 -> %d 字节\n", ret.m_pText ? (int)lstrlenA(ret.m_pText) : -1);
        if (!ret.m_pText || lstrlenA(ret.m_pText) < 100) Fail("快速请求 正文过短：%s", ret.m_pText);
        FreeRet(&ret);
    }

    printf("\n---- 会话 / 请求 / 响应 ----\n");
    Obj sess, req, resp, resp2;
    sess.Bind(); req.Bind(); resp.Bind(); resp2.Bind();
    CallSelf(FindCmd(0, "浏览器会话构造函数"), sess);
    CallSelf(FindCmd(1, "浏览器请求构造函数"), req);
    CallSelf(FindCmd(2, "浏览器响应构造函数"), resp);
    CallSelf(FindCmd(2, "浏览器响应构造函数"), resp2);
    if (!sess.Alive() || !req.Alive() || !resp.Alive()) Fail("构造函数没有写入内部数据");

    CallSelfText(FindCmd(0, "设置指纹型号"), sess, "chrome133a");
    CallSelfText(FindCmd(0, "设置代理"), sess, proxy);
    if (getenv("NO_PROXY_ARG") != NULL) CallSelfText(FindCmd(0, "设置代理"), sess, "");
    if (getenv("NO_PROFILE") != NULL) CallSelfText(FindCmd(0, "设置指纹型号"), sess, "");
    {
        MDATA_INF a[2]; a[0] = sess.self; a[1] = IntArg(20);
        memset(&ret, 0, sizeof(ret));
        g_inf->m_pCmdsFunc[FindCmd(0, "设置超时")](&ret, 2, a);
    }
    {
        MDATA_INF a[3]; a[0] = sess.self; a[1] = TextArg("Accept-Language"); a[2] = TextArg("zh-CN,zh;q=0.9");
        memset(&ret, 0, sizeof(ret));
        g_inf->m_pCmdsFunc[FindCmd(0, "设置请求头")](&ret, 3, a);
    }
    CallSelfText(FindCmd(1, "设置网址"), req, "https://www.wikipedia.org");

    // 会话.执行(请求, 响应)
    {
        MDATA_INF a[3]; a[0] = sess.self; a[1] = ObjArg(req); a[2] = ObjArg(resp);
        memset(&ret, 0, sizeof(ret));
        g_inf->m_pCmdsFunc[FindCmd(0, "执行")](&ret, 3, a);
        if (!ret.m_bool) Fail("执行 返回假");
        FreeRet(&ret);
    }
    CallSelfTextRet(FindCmd(2, "取错误文本"), resp, text, sizeof(text));
    const int nStatus = CallSelfInt(FindCmd(2, "取状态码"), resp);
    const int nRecvRightAfter = CallSelfInt(FindCmd(2, "取下载字节数"), resp);
    if (nRecvRightAfter <= 0) Fail("取下载字节数 应大于 0，实际 %d", nRecvRightAfter);
    printf("第一次 GET -> 状态 %d 错误「%s」\n", nStatus, text);
    if (nStatus != 200) Fail("状态码不是 200（错误：%s）", text);

    CallSelfTextRet(FindCmd(2, "取响应文本"), resp, text, sizeof(text));
    printf("响应文本 -> %d 字节, 开头: %.40s\n", (int)lstrlenA(text), text);
    if (lstrlenA(text) < 100) Fail("响应文本过短");

    const int nHeads = CallSelfInt(FindCmd(2, "取响应头数量"), resp);
    printf("响应头 -> %d 条\n", nHeads);
    if (nHeads < 3) Fail("响应头解析太少");

    {
        MDATA_INF a[2]; a[0] = resp.self; a[1] = TextArg("content-type");
        memset(&ret, 0, sizeof(ret));
        g_inf->m_pCmdsFunc[FindCmd(2, "按名取响应头")](&ret, 2, a);
        printf("Content-Type -> %s\n", ret.m_pText);
        if (!ret.m_pText || !strstr(ret.m_pText, "text/html")) Fail("按名取响应头 拿不到 content-type");
        FreeRet(&ret);
    }
    const int nMs = CallSelfInt(FindCmd(2, "取耗时毫秒"), resp);
    const int nRecv = CallSelfInt(FindCmd(2, "取下载字节数"), resp);
    printf("耗时 -> %d 毫秒，下载 -> %d 字节\n", nMs, nRecv);
    if (nRecv <= 0) Fail("取下载字节数 应大于 0，实际 %d", nRecv);
    if (!CallSelfInt(FindCmd(2, "是否成功"), resp)) Fail("是否成功 应为真");

    // 同一会话、换一个请求：验证连接复用路径不炸
    CallSelfText(FindCmd(1, "设置网址"), req, "https://www.wikipedia.org");
    {
        MDATA_INF a[3]; a[0] = sess.self; a[1] = ObjArg(req); a[2] = ObjArg(resp2);
        memset(&ret, 0, sizeof(ret));
        g_inf->m_pCmdsFunc[FindCmd(0, "执行")](&ret, 3, a);
        const BOOL ok = ret.m_bool;
        FreeRet(&ret);
        const int n2 = CallSelfInt(FindCmd(2, "取状态码"), resp2);
        printf("第二次 GET wikipedia -> %s 状态 %d\n", ok ? "成功" : "失败", n2);
        if (!ok) Fail("同会话第二次执行失败");
    }

    // 下载落盘
    CallSelfText(FindCmd(1, "设置下载文件"), req, "out\\host_download.html");
    {
        MDATA_INF a[3]; a[0] = sess.self; a[1] = ObjArg(req); a[2] = ObjArg(resp);
        memset(&ret, 0, sizeof(ret));
        g_inf->m_pCmdsFunc[FindCmd(0, "执行")](&ret, 3, a);
        FreeRet(&ret);
    }
    {
        WIN32_FIND_DATAA fd;
        HANDLE hf = FindFirstFileA("out\\host_download.html", &fd);
        const bool exists = (hf != INVALID_HANDLE_VALUE);
        if (exists) FindClose(hf);
        printf("下载落盘 -> %s %lu 字节\n", exists ? "有" : "无", exists ? fd.nFileSizeLow : 0);
        if (!exists || fd.nFileSizeLow < 100) Fail("下载文件没写出来");
        DeleteFileA("out\\host_download.html");
    }

    // 复制与析构
    Obj sessCopy; sessCopy.Bind();
    {
        MDATA_INF a[2]; a[0] = sessCopy.self; a[1] = ObjArg(sess);
        memset(&ret, 0, sizeof(ret));
        g_inf->m_pCmdsFunc[FindCmd(0, "浏览器会话复制函数")](&ret, 2, a);
    }
    if (!sessCopy.Alive() || sessCopy.slot == sess.slot) Fail("会话复制没有得到独立对象");
    CallSelfTextRet(FindCmd(0, "取指纹型号"), sessCopy, text, sizeof(text));
    if (strcmp(text, "chrome133a") != 0) Fail("复制后指纹型号丢失：%s", text);
    CallSelf(FindCmd(0, "浏览器会话析构函数"), sessCopy);
    if (sessCopy.Alive()) Fail("析构后数据槽没清零");

    CallSelf(FindCmd(2, "浏览器响应析构函数"), resp2);
    CallSelf(FindCmd(1, "浏览器请求析构函数"), req);
    CallSelf(FindCmd(2, "浏览器响应析构函数"), resp);
    CallSelf(FindCmd(0, "浏览器会话析构函数"), sess);

    printf("\n易语言内存池: 申请 %d / 释放 %d / 运行时错误 %d\n", g_mallocCount, g_freeCount, g_runtimeErr);
    if (g_runtimeErr) Fail("出现运行时错误");

    printf(g_failed ? "\n结论: %d 项不合格\n" : "\n结论: 全部通过\n", g_failed);
    FreeLibrary(h);
    return g_failed ? 1 : 0;
}
