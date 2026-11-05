// 探针：把用户从易语言里导出的 处理文本 原始字节（UTF-16，含末尾 0000）原样喂给库，
// 不做任何编码转换，逐条复现他那两行调用。用法：probe_bin.exe <fne> <bin文件>
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
    case NRS_RUNTIME_ERR: printf("    [RUNTIME-ERR] %s\n", (const char*)(INT_PTR)dwParam1); return 0;
    default: return 0;
    }
}

static LONG CALLBACK Veh(PEXCEPTION_POINTERS ep)
{
    char* addr = (char*)ep->ExceptionRecord->ExceptionAddress;
    printf("[EXC] code=0x%X addr=%p", ep->ExceptionRecord->ExceptionCode, addr);
    if (g_hMod && addr >= (char*)g_hMod && addr < (char*)g_hMod + 0x400000)
        printf(" rva=0x%X INLIB\n", (unsigned)(addr - (char*)g_hMod));
    else printf(" OUTLIB\n");
    fflush(stdout);
    return EXCEPTION_CONTINUE_SEARCH;
}

static int FindCmd(const char* name)
{
    for (int i = 0; i < g_inf->m_nCmdCount; i++)
        if (g_inf->m_pBeginCmdInfo[i].m_shtCategory > 0 &&
            lstrcmpA(AS_TEXT(g_inf->m_pBeginCmdInfo[i].m_szName), name) == 0) return i;
    printf("[FAIL] no command %s\n", name); return -1;
}

// 把文件里的原始字节当 字节集 内容（块头 = 维数 1 + 字节数），不转换、不补终止符
static MDATA_INF RawBinBlock(const char* data, int nBytes)
{
    MDATA_INF m; memset(&m, 0, sizeof(m));
    m.m_dtDataType = SDT_BIN;
    char* p = (char*)malloc(8 + (size_t)nBytes);
    *(int*)p = 1; *(int*)(p + 4) = nBytes;
    memcpy(p + 8, data, (size_t)nBytes);
    m.m_pBin = (LPBYTE)p;
    return m;
}

static MDATA_INF TextArg(const char* s)
{
    MDATA_INF m; memset(&m, 0, sizeof(m));
    m.m_dtDataType = SDT_TEXT; m.m_pText = (char*)s; return m;
}
static MDATA_INF IntArg(int v)
{
    MDATA_INF m; memset(&m, 0, sizeof(m));
    m.m_dtDataType = SDT_INT; m.m_int = v; return m;
}
static MDATA_INF NullArg()
{
    MDATA_INF m; memset(&m, 0, sizeof(m));
    m.m_dtDataType = _SDT_NULL; return m;
}
static MDATA_INF BoolArgF()
{
    MDATA_INF m; memset(&m, 0, sizeof(m));
    m.m_dtDataType = SDT_BOOL; m.m_bool = FALSE; return m;
}

static void Show(const char* tag, MDATA_INF* r)
{
    if (r->m_dtDataType != SDT_BIN) { printf("  %s: dt=0x%X (非字节集)\n", tag, r->m_dtDataType); return; }
    if (r->m_pBin == NULL) { printf("  %s: NULL 块（空字节集）\n", tag); return; }
    int nBytes = *(int*)(r->m_pBin + 4);
    const wchar_t* w = (const wchar_t*)(r->m_pBin + 8);
    int chars = 0; while (chars * 2 < nBytes && w[chars] != 0) chars++;
    char* u8 = (char*)malloc((size_t)chars * 4 + 4);
    int n = WideCharToMultiByte(CP_UTF8, 0, w, chars, u8, chars * 4 + 3, NULL, NULL);
    if (n < 0) n = 0;
    u8[n] = 0;
    for (int i = 0; i < n; i++) if (u8[i] == '\r') u8[i] = 'R';
    for (int i = 0; i < n; i++) if (u8[i] == '\n') u8[i] = 'N';
    if (n > 64) { u8[64] = 0; }
    printf("  %s: 块 %d 字节 / %d 字符 -> 「%s」\n", tag, nBytes, chars, u8);
    free(u8);
}

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    AddVectoredExceptionHandler(1, Veh);
    const char* dll = (argc > 1) ? argv[1] : "out\\cxsoft.fne";
    const char* file = (argc > 2) ? argv[2] : "..\\test.bin";
    FILE* fp = fopen(file, "rb");
    if (fp == NULL) { printf("open %s failed\n", file); return 1; }
    fseek(fp, 0, SEEK_END); long nRaw = ftell(fp); fseek(fp, 0, SEEK_SET);
    char* raw = (char*)malloc((size_t)nRaw + 2);
    size_t got = fread(raw, 1, (size_t)nRaw, fp); raw[got] = 0; fclose(fp);
    printf("file %s = %lu 字节 (%d 宽字符)\n", file, (unsigned long)got, (int)(got / 2));

    HMODULE h = LoadLibraryA(dll);
    if (h == NULL) { printf("load %s failed %lu\n", dll, GetLastError()); return 1; }
    g_hMod = h;
    g_inf = ((fn_GetNewInf)GetProcAddress(h, "GetNewInf"))();
    g_inf->m_pfnNotify(NL_SYS_NOTIFY_FUNCTION, (DWORD)(INT_PTR)&FakeNotifySys, 0);
    printf("lib cmd=%d type=%d\n", g_inf->m_nCmdCount, g_inf->m_nDataTypeCount);

    {   // 取库版本标识：证明这份 .fne 是哪一次链接产物、参数结构步长是不是 12
        int iVer = FindCmd("取库版本标识");
        MDATA_INF rv; memset(&rv, 0, sizeof(rv));
        g_inf->m_pCmdsFunc[iVer](&rv, 0, NULL);
        printf("取库版本标识 -> 「%s」\n",
               (rv.m_dtDataType == SDT_TEXT && rv.m_pText) ? rv.m_pText : "(空)");
        if (rv.m_pText) free(rv.m_pText);
    }

    int iFind = FindCmd("寻找文本W");
    int iM2   = FindCmd("文本_取文本_正则2W");
    int iMW   = FindCmd("文本_取文本_正则W");
    int iRepl = FindCmd("子文本替换W");
    int iW    = FindCmd("W");
    int iA2W  = FindCmd("A2W");
    int iLen  = FindCmd("取文本长度W");

    MDATA_INF src = RawBinBlock(raw, (int)got);

    {   // 自检单里的第 1 段：A2W 造的块长度与 取文本长度W
        MDATA_INF aw[2] = { TextArg("1[[1"), NullArg() };
        MDATA_INF w; memset(&w, 0, sizeof(w));
        g_inf->m_pCmdsFunc[iA2W](&w, 2, aw);
        MDATA_INF la[1] = { w };
        MDATA_INF lr; memset(&lr, 0, sizeof(lr));
        g_inf->m_pCmdsFunc[iLen](&lr, 1, la);
        printf("A2W(\"1[[1\") -> 块 %d 字节，取文本长度W -> %d（期望 10 / 4）\n",
               *(int*)(w.m_pBin + 4), lr.m_int);
        MDATA_INF aa[2] = { TextArg("[["), NullArg() };
        MDATA_INF w2; memset(&w2, 0, sizeof(w2));
        g_inf->m_pCmdsFunc[iA2W](&w2, 2, aa);
        printf("A2W(\"[[\") -> 块 %d 字节（期望 6）\n", *(int*)(w2.m_pBin + 4));
        MDATA_INF fa[4] = { w, w2, IntArg(1), BoolArgF() };
        MDATA_INF fr; memset(&fr, 0, sizeof(fr));
        g_inf->m_pCmdsFunc[iFind](&fr, 4, fa);
        printf("寻找文本W(A2W(\"1[[1\"), A2W(\"[[\"), 1, 假) -> %d（期望 2）\n", fr.m_int);
        MDATA_INF ls[1] = { src };
        MDATA_INF lsr; memset(&lsr, 0, sizeof(lsr));
        g_inf->m_pCmdsFunc[iLen](&lsr, 1, ls);
        printf("取文本长度W(test.bin) -> %d（期望 557）\n", lsr.m_int);
        int iW2A = FindCmd("W2A");
        MDATA_INF ba[2] = { w, IntArg (936) };
        MDATA_INF br; memset(&br, 0, sizeof(br));
        g_inf->m_pCmdsFunc[iW2A](&br, 2, ba);
        printf("W2A(A2W(\"1[[1\"),936) -> 「%s」（期望 1[[1）\n",
               (br.m_dtDataType == SDT_TEXT && br.m_pText != NULL) ? br.m_pText : "(非文本)");

        {   // 第 2 个字节集参数能不能被库正常读到：用 子文本替换W / 正则2W 交叉验
            int iSub = FindCmd("子文本替换W");
            MDATA_INF s1; memset(&s1, 0, sizeof(s1));
            MDATA_INF t1[2] = { TextArg("abc"), NullArg() };
            g_inf->m_pCmdsFunc[iA2W](&s1, 2, t1);
            MDATA_INF s2; memset(&s2, 0, sizeof(s2));
            MDATA_INF t2[2] = { TextArg("b"), NullArg() };
            g_inf->m_pCmdsFunc[iA2W](&s2, 2, t2);
            MDATA_INF s3; memset(&s3, 0, sizeof(s3));
            MDATA_INF t3[2] = { TextArg("X"), NullArg() };
            g_inf->m_pCmdsFunc[iA2W](&s3, 2, t3);
            MDATA_INF sa[6] = { s1, s2, s3, NullArg(), NullArg(), NullArg() };
            MDATA_INF sr; memset(&sr, 0, sizeof(sr));
            g_inf->m_pCmdsFunc[iSub](&sr, 6, sa);
            MDATA_INF wa[2] = { sr, IntArg(936) };
            MDATA_INF wr; memset(&wr, 0, sizeof(wr));
            g_inf->m_pCmdsFunc[FindCmd("W2A")](&wr, 2, wa);
            printf("W2A(子文本替换W(A2W(abc),A2W(b),A2W(X))) -> 「%s」（期望 aXc）\n",
                   (wr.m_dtDataType == SDT_TEXT && wr.m_pText) ? wr.m_pText : "(非文本)");
            MDATA_INF ra[6] = { s1, TextArg("[bc]"), IntArg(1), NullArg(), NullArg(), NullArg() };
            MDATA_INF rr; memset(&rr, 0, sizeof(rr));
            g_inf->m_pCmdsFunc[iM2](&rr, 6, ra);
            MDATA_INF wa2[2] = { rr, IntArg(936) };
            MDATA_INF wr2; memset(&wr2, 0, sizeof(wr2));
            g_inf->m_pCmdsFunc[FindCmd("W2A")](&wr2, 2, wa2);
            printf("W2A(正则2W(A2W(abc),\"[bc]\")) -> 「%s」（期望 b）\n",
                   (wr2.m_dtDataType == SDT_TEXT && wr2.m_pText) ? wr2.m_pText : "(非文本)");
            MDATA_INF la2[1] = { s2 };
            MDATA_INF lr2; memset(&lr2, 0, sizeof(lr2));
            g_inf->m_pCmdsFunc[iLen](&lr2, 1, la2);
            printf("取文本长度W(A2W(\"b\")) -> %d（期望 1）\n", lr2.m_int);
            free(s1.m_pBin); free(s2.m_pBin); free(s3.m_pBin);
            if (sr.m_pBin) free(sr.m_pBin);
            if (rr.m_pBin) free(rr.m_pBin);
        }
        free(w.m_pBin); free(w2.m_pBin);
    }

    {   // 用户在 IDE 里那条最小复现：寻找文本W(「1[[1」, 「[[」, 1, 假) 应为 2
        wchar_t s1[5] = { L'1', L'[', L'[', L'1', 0 };
        wchar_t n1[3] = { L'[', L'[', 0 };
        MDATA_INF bs = RawBinBlock((const char*)s1, (int)(lstrlenW(s1) + 1) * 2);
        MDATA_INF bn = RawBinBlock((const char*)n1, (int)(lstrlenW(n1) + 1) * 2);
        printf("夹具 源 %d 字节 / 针 %d 字节\n", *(int*)(bs.m_pBin + 4), *(int*)(bn.m_pBin + 4));
        MDATA_INF a[4] = { bs, bn, IntArg(1), BoolArgF() };
        MDATA_INF r; memset(&r, 0, sizeof(r));
        g_inf->m_pCmdsFunc[iFind](&r, 4, a);
        printf("寻找文本W(\"1[[1\",\"[[\",1,假) -> %d（应为 2）\n", r.m_int);
        MDATA_INF a2[4] = { bs, bn, IntArg(1), NullArg() };
        g_inf->m_pCmdsFunc[iFind](&r, 4, a2);
        printf("   同上但第4参留空 -> %d\n", r.m_int);
        free(bs.m_pBin); free(bn.m_pBin);
    }

    {   // 用户那条自检：寻找文本W( W("[[") , W("[[") , 1, 假 ) —— 自己找自己必须命中
        MDATA_INF aW[2] = { TextArg("[["), NullArg() };
        MDATA_INF w1, w2; memset(&w1, 0, sizeof(w1)); memset(&w2, 0, sizeof(w2));
        g_inf->m_pCmdsFunc[iW](&w1, 2, aW);
        g_inf->m_pCmdsFunc[iW](&w2, 2, aW);
        Show("W(\"[[\") #1", &w1);
        Show("W(\"[[\") #2", &w2);
        MDATA_INF a[4] = { w1, w2, IntArg(1), BoolArgF() };
        MDATA_INF r; memset(&r, 0, sizeof(r));
        g_inf->m_pCmdsFunc[iFind](&r, 4, a);
        printf("寻找文本W(W,W) -> %d（应为 1）\n", r.m_int);
        MDATA_INF a2[4] = { src, w2, IntArg(1), BoolArgF() };
        g_inf->m_pCmdsFunc[iFind](&r, 4, a2);
        printf("寻找文本W(用户字节集, W) -> %d（应为 11）\n", r.m_int);
        if (w1.m_pBin) free(w1.m_pBin);
        if (w2.m_pBin) free(w2.m_pBin);
    }

    {   // 先确认库能在这份数据里定位到 "[["
        wchar_t needle[3] = { L'[', L'[', 0 };
        MDATA_INF nd = RawBinBlock((const char*)needle, 6);
        MDATA_INF a[4] = { src, nd, IntArg(1), NullArg() };
        MDATA_INF r; memset(&r, 0, sizeof(r));
        g_inf->m_pCmdsFunc[iFind](&r, 4, a);
        printf("寻找文本W(\"[[\") -> %d\n", r.m_int);
        free(nd.m_pBin);
    }
    {   // 用户原样那条（模式按 文本型 UTF-8 交进去）
        MDATA_INF a[6] = { src, TextArg("(.|\\n)+?\\[\\["), IntArg(1), NullArg(), NullArg(), NullArg() };
        MDATA_INF r; memset(&r, 0, sizeof(r));
        __try { g_inf->m_pCmdsFunc[iM2](&r, 6, a); }
        __except (EXCEPTION_EXECUTE_HANDLER) { printf("  正则2W 调用异常 0x%X\n", GetExceptionCode()); }
        Show("正则2W (.|\\n)+?\\[\\[", &r);
        if (r.m_pBin) free(r.m_pBin);
    }
    {   // 退化模式：只找 \[\[
        MDATA_INF a[6] = { src, TextArg("\\[\\["), IntArg(1), NullArg(), NullArg(), NullArg() };
        MDATA_INF r; memset(&r, 0, sizeof(r));
        __try { g_inf->m_pCmdsFunc[iM2](&r, 6, a); }
        __except (EXCEPTION_EXECUTE_HANDLER) { printf("  正则2W 调用异常 0x%X\n", GetExceptionCode()); }
        Show("正则2W \\[\\[", &r);
        if (r.m_pBin) free(r.m_pBin);
    }
    {   // 模式改走宽文本入口（文本_取文本_正则W）
        wchar_t pat[] = { '(', '.', '|', '\\', 'n', ')', '+', '?', '\\', '[', '\\', '[', 0 };
        MDATA_INF pw = RawBinBlock((const char*)pat, (int)lstrlenW(pat) * 2 + 2);
        MDATA_INF a[6] = { src, pw, IntArg(1), NullArg(), NullArg(), NullArg() };
        MDATA_INF r; memset(&r, 0, sizeof(r));
        __try { g_inf->m_pCmdsFunc[iMW](&r, 6, a); }
        __except (EXCEPTION_EXECUTE_HANDLER) { printf("  正则W 调用异常 0x%X\n", GetExceptionCode()); }
        Show("正则W (.|\\n)+?\\[\\[ 宽模式", &r);
        if (r.m_pBin) free(r.m_pBin);
        free(pw.m_pBin);
    }
    {   // 他那第二行原样：子文本替换W(处理文本, 开头文本, A2W("[["), , , )
        MDATA_INF head; memset(&head, 0, sizeof(head));
        MDATA_INF a1[6] = { src, TextArg("(.|\\n)+?\\[\\["), IntArg(1), NullArg(), NullArg(), NullArg() };
        __try { g_inf->m_pCmdsFunc[iM2](&head, 6, a1); }
        __except (EXCEPTION_EXECUTE_HANDLER) { printf("  取开头文本异常\n"); }
        Show("开头文本 = 正则2W(...)", &head);
        MDATA_INF aW[2] = { TextArg("[["), NullArg() };
        MDATA_INF rw; memset(&rw, 0, sizeof(rw));
        g_inf->m_pCmdsFunc[iA2W](&rw, 2, aW);
        MDATA_INF a2[6] = { src, head, rw, NullArg(), NullArg(), NullArg() };
        MDATA_INF r2; memset(&r2, 0, sizeof(r2));
        __try { g_inf->m_pCmdsFunc[iRepl](&r2, 6, a2); }
        __except (EXCEPTION_EXECUTE_HANDLER) { printf("  子文本替换W 调用异常 0x%X\n", GetExceptionCode()); }
        Show("子文本替换W 结果", &r2);
        if (head.m_pBin) free(head.m_pBin);
        if (r2.m_pBin) free(r2.m_pBin);
        if (rw.m_pBin) free(rw.m_pBin);
    }
    free(src.m_pBin);
    free(raw);
    printf("done\n");
    FreeLibrary(h);
    return 0;
}
