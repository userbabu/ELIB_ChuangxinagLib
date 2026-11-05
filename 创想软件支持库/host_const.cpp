// 常量表宿主：把 .fne 自报的常量表与 MSVC 探针给出的 libcurl 真值逐条比对。
// 探针由 `node http访问\gen_http_consts.mjs --verify` 生成到 out\consts_probe.txt。
// 断言三件事：每条 curl 常量都在表里且值一致；表内无重名；文本常量之外一律 CT_NUM。
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

typedef PLIB_INFO(WINAPI* fn_GetNewInf)(void);

static int g_nPass = 0, g_nFail = 0;

static void Fail(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf("  [FAIL] ");
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
    g_nFail++;
}

static void Ok(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf("  [ok]   ");
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
    g_nPass++;
}

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    char dllPath[MAX_PATH] = { 0 };
    char probePath[MAX_PATH] = { 0 };
    lstrcpynA(dllPath, argc > 1 ? argv[1] : "out\\cxsoft.fne", MAX_PATH);
    lstrcpynA(probePath, argc > 2 ? argv[2] : "out\\consts_probe.txt", MAX_PATH);

    HMODULE h = LoadLibraryA(dllPath);
    if (h == NULL) { printf("[FAIL] 加载 %s 失败，错误码 %lu\n", dllPath, GetLastError()); return 1; }
    fn_GetNewInf pGetNewInf = (fn_GetNewInf)GetProcAddress(h, "GetNewInf");
    if (!pGetNewInf) { printf("[FAIL] 找不到 GetNewInf\n"); return 1; }
    PLIB_INFO inf = pGetNewInf();
    int nConst = inf->m_nLibConstCount;
    PLIB_CONST_INFO p = inf->m_pLibConst;
    if (nConst <= 0 || p == NULL) { printf("[FAIL] 库未登记任何常量\n"); return 1; }
    printf("[host_const] %s  常量 %d 条\n", dllPath, nConst);

    // 表内重名会让易语言按名取到错的那条
    for (int i = 0; i < nConst; i++)
    {
        for (int j = i + 1; j < nConst; j++)
        {
            if (p[i].m_szName && p[j].m_szName && strcmp(p[i].m_szName, p[j].m_szName) == 0)
            {
                Fail("常量重名：%s（下标 %d 与 %d）", p[i].m_szName, i, j);
                break;
            }
        }
    }

    // 读探针：NAME=VALUE
    FILE* fp = fopen(probePath, "rb");
    if (!fp) { printf("[FAIL] 打不开探针 %s（先跑 gen_http_consts.mjs --verify）\n", probePath); return 1; }
    int nProbe = 0, nHit = 0, nDiff = 0, nCurlInLib = 0;
    char line[256];
    while (fgets(line, sizeof(line), fp))
    {
        char* eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        char name[128];
        lstrcpynA(name, line, sizeof(name));
        long long want = _atoi64(eq + 1);
        nProbe++;
        if (strncmp(name, "CURL", 4) != 0) continue;
        nCurlInLib++;                       // 探针里的每一条都应当进库
        const char* got = NULL;
        for (int i = 0; i < nConst; i++)
        {
            if (p[i].m_szName && strcmp(p[i].m_szName, name) == 0)
            {
                got = p[i].m_szName;
                if (p[i].m_shtType != CT_NUM) Fail("%s 不是数值常量（type=%d）", name, p[i].m_shtType);
                if ((long long)(INT)p[i].m_dbValue != want)
                {
                    Fail("%s 库内 %lld，编译器 %lld", name, (long long)(INT)p[i].m_dbValue, want);
                    nDiff++;
                }
                else nHit++;
                break;
            }
        }
        if (got == NULL) { Fail("库内缺常量 %s（编译器探针有）", name); nDiff++; }
    }
    fclose(fp);

    if (nDiff == 0 && nHit == nCurlInLib) Ok("curl 常量 %d 条与编译器取值逐条一致", nHit);
    else Fail("curl 常量比对：命中 %d / 应 %d，差异 %d", nHit, nCurlInLib, nDiff);
    if (nCurlInLib > 700) Ok("探针覆盖条数 %d（>=700，未被误裁）", nCurlInLib);
    else Fail("探针只有 %d 条，疑似生成环节退化", nCurlInLib);
    printf("%s: 通过 %d，失败 %d，探针 %d 条\n", g_nFail ? "[ERROR]" : "[OK]", g_nPass, g_nFail, nProbe);
    FreeLibrary(h);
    return g_nFail ? 1 : 0;
}
