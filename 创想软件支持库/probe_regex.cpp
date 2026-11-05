// 最小探针：只跑用户那两行（正则2W 取开头文本 + 子文本替换W），排除宿主其它用例的干扰。
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

typedef double DATE; typedef double DOUBLE; typedef float FLOAT; typedef long long INT64;
#include <lib2.h>
#include <lang.h>

#define AS_TEXT(p) ((const char*)(p))
typedef PLIB_INFO(WINAPI* fn_GetNewInf)(void);
static PLIB_INFO g_inf = NULL;
static HMODULE g_hMod = NULL;

static INT WINAPI FakeNotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    switch (nMsg)
    {
    case NRS_MALLOC: return (INT)(INT_PTR)malloc(dwParam1 ? dwParam1 : 1);
    case NRS_MFREE: if (dwParam1) free((void*)(INT_PTR)dwParam1); return 0;
    case NRS_FREE_ARY: { void* p = (void*)(INT_PTR)dwParam2; if (p) free(p); return 0; }
    case NRS_RUNTIME_ERR: printf("    [运行时错误] %s\n", (const char*)(INT_PTR)dwParam1); return 0;
    default: return 0;
    }
}

static LONG CALLBACK Veh(PEXCEPTION_POINTERS ep)
{
    char* addr = (char*)ep->ExceptionRecord->ExceptionAddress;
    printf("[EXC] code=0x%X addr=%p", ep->ExceptionRecord->ExceptionCode, addr);
    if (g_hMod && addr >= (char*)g_hMod && addr < (char*)g_hMod + 0x400000)
        printf(" rva=0x%X 在库内", (unsigned)(addr - (char*)g_hMod));
    else printf(" 在库外");
    if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
        printf(" 操作=%d 目标=%p", (int)ep->ExceptionRecord->ExceptionInformation[0],
               (void*)ep->ExceptionRecord->ExceptionInformation[1]);
    printf("\n"); fflush(stdout);
    return EXCEPTION_CONTINUE_SEARCH;
}

static int FindCmd(const char* name)
{
    for (int i = 0; i < g_inf->m_nCmdCount; i++)
        if (g_inf->m_pBeginCmdInfo[i].m_shtCategory > 0 &&
            lstrcmpA(AS_TEXT(g_inf->m_pBeginCmdInfo[i].m_szName), name) == 0) return i;
    printf("[FAIL] 找不到命令 %s\n", name); return -1;
}

static MDATA_INF WideArgFromUtf8(const char* u8)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, u8, -1, NULL, 0);
    wchar_t* w = (wchar_t*)malloc((size_t)(n > 0 ? n : 1) * sizeof(wchar_t));
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, u8, -1, w, n); else w[0] = 0;
    int nBytes = (n > 0 ? n : 1) * 2;
    MDATA_INF m; memset(&m, 0, sizeof(m));
    m.m_dtDataType = SDT_BIN;
    char* p = (char*)malloc(8 + nBytes);
    *(int*)p = 1; *(int*)(p + 4) = nBytes;
    memcpy(p + 8, w, nBytes);
    m.m_pBin = (LPBYTE)p;
    free(w);
    return m;
}

static void Show(const char* tag, MDATA_INF* r)
{
    if (r->m_dtDataType != SDT_BIN || r->m_pBin == NULL) { printf("  %s: 非字节集或空指针 dt=0x%X\n", tag, r->m_dtDataType); return; }
    int nBytes = *(int*)(r->m_pBin + 4);
    const wchar_t* w = (const wchar_t*)(r->m_pBin + 8);
    int chars = 0; while (chars * 2 < nBytes && w[chars] != 0) chars++;
    char* u8 = (char*)malloc((size_t)chars * 4 + 4);
    int n = WideCharToMultiByte(CP_UTF8, 0, w, chars, u8, chars * 4 + 3, NULL, NULL);
    if (n < 0) n = 0;
    u8[n] = 0;
    for (int i = 0; i < n; i++) if (u8[i] == '\r') u8[i] = 'R';
    for (int i = 0; i < n; i++) if (u8[i] == '\n') u8[i] = 'N';
    if (n > 48) u8[48] = 0;
    printf("  %s: %d 字符 -> 「%s」\n", tag, chars, u8);
    free(u8);
}

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    AddVectoredExceptionHandler(1, Veh);
    const char* dll = (argc > 1) ? argv[1] : "out\\cxsoft.fne";
    const char* file = (argc > 2) ? argv[2] : "..\\..\\tmp\\regex_payload.txt";
    HMODULE h = LoadLibraryA(dll);
    if (h == NULL) { printf("加载 %s 失败 %lu\n", dll, GetLastError()); return 1; }
    g_hMod = h;
    g_inf = ((fn_GetNewInf)GetProcAddress(h, "GetNewInf"))();
    g_inf->m_pfnNotify(NL_SYS_NOTIFY_FUNCTION, (DWORD)(INT_PTR)&FakeNotifySys, 0);

    FILE* fp = fopen(file, "rb");
    if (fp == NULL) { printf("读不到 %s\n", file); return 1; }
    fseek(fp, 0, SEEK_END); long nRaw = ftell(fp); fseek(fp, 0, SEEK_SET);
    char* raw = (char*)malloc((size_t)nRaw + 1);
    size_t got = fread(raw, 1, (size_t)nRaw, fp); raw[got] = 0; fclose(fp);
    printf("payload %lu 字节\n", (unsigned long)got);

    int iMatch = FindCmd("文本_取文本_正则2W");
    int iRepl  = FindCmd("子文本替换W");
    int iW     = FindCmd("W");

    MDATA_INF src = WideArgFromUtf8(raw);
    MDATA_INF pat; memset(&pat, 0, sizeof(pat)); pat.m_dtDataType = SDT_TEXT;
    pat.m_pText = (char*)"(.|\\n)+?\\[\\[";
    MDATA_INF nWant; memset(&nWant, 0, sizeof(nWant)); nWant.m_dtDataType = SDT_INT; nWant.m_int = 1;
    MDATA_INF nul; memset(&nul, 0, sizeof(nul)); nul.m_dtDataType = _SDT_NULL;

    MDATA_INF a1[6] = { src, pat, nWant, nul, nul, nul };
    MDATA_INF r1; memset(&r1, 0, sizeof(r1));
    printf("call 文本_取文本_正则2W\n"); fflush(stdout);
    __try { g_inf->m_pCmdsFunc[iMatch](&r1, 6, a1); }
    __except (EXCEPTION_EXECUTE_HANDLER) { printf("  调用异常 0x%X\n", GetExceptionCode()); }
    Show("开头文本", &r1);

    // 第二行：子文本替换W(处理文本, 开头文本, W("[["))
    MDATA_INF blank; memset(&blank, 0, sizeof(blank)); blank.m_dtDataType = SDT_TEXT;
    blank.m_pText = (char*)"[[";
    MDATA_INF cp; memset(&cp, 0, sizeof(cp)); cp.m_dtDataType = SDT_INT; cp.m_int = 65001;
    MDATA_INF aW[2] = { blank, cp };
    MDATA_INF rW; memset(&rW, 0, sizeof(rW));
    printf("call W(\"[[\",65001)\n"); fflush(stdout);
    g_inf->m_pCmdsFunc[iW](&rW, 2, aW);
    Show("W([[)", &rW);

    MDATA_INF a2[6] = { src, r1, rW, nul, nul, nul };
    MDATA_INF r2; memset(&r2, 0, sizeof(r2));
    printf("call 子文本替换W\n"); fflush(stdout);
    __try { g_inf->m_pCmdsFunc[iRepl](&r2, 6, a2); }
    __except (EXCEPTION_EXECUTE_HANDLER) { printf("  调用异常 0x%X\n", GetExceptionCode()); }
    Show("结果", &r2);

    if (r1.m_pBin) free(r1.m_pBin);
    if (r2.m_pBin) free(r2.m_pBin);
    if (rW.m_pBin) free(rW.m_pBin);
    free(src.m_pBin); free(raw);
    printf("done\n");
    FreeLibrary(h);
    return 0;
}
