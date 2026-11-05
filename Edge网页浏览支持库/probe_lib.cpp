// 探针：装载一个 .fne，打印它自己声明的窗口单元登记信息，用来和成品组件库逐项对比。
// 用法：probe_lib <xxx.fne>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef double DATE;
typedef double DOUBLE;
typedef float FLOAT;
typedef long long INT64;

#include <lib2.h>
#include <lang.h>

INT WINAPI FakeNotify(INT nMsg, DWORD dwParam1, DWORD dwParam2)
{
    UNREFERENCED_PARAMETER(dwParam2);
    if (nMsg == NRS_MALLOC) return (INT)(INT_PTR)malloc(dwParam1 ? dwParam1 : 1);
    if (nMsg == NRS_MFREE) { if (dwParam1) free((void*)(INT_PTR)dwParam1); return 0; }
    return 0;
}

void DumpBitmap(HMODULE h, DWORD id)
{
    if (id == 0) { printf("      图标ID=0（无位图）\n"); return; }
    HRSRC hR = FindResourceA(h, MAKEINTRESOURCEA(id), RT_BITMAP);
    if (hR == NULL) { printf("      图标ID=%u 找不到 BITMAP 资源\n", (unsigned)id); return; }
    DWORD sz = SizeofResource(h, hR);
    const char* p = (const char*)LockResource(LoadResource(h, hR));
    int biSize = 0, w = 0, hgt = 0;
    short planes = 0, bpp = 0;
    if (p != NULL && sz >= 40)
    {
        memcpy(&biSize, p, 4);
        memcpy(&w, p + 4, 4);
        memcpy(&hgt, p + 8, 4);
        memcpy(&planes, p + 12, 2);
        memcpy(&bpp, p + 14, 2);
    }
    printf("      图标ID=%u 资源%u字节 DIB头=%d %dx%d planes=%d bpp=%d\n",
           (unsigned)id, (unsigned)sz, biSize, w, hgt, (int)planes, (int)bpp);
}

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    SetConsoleOutputCP(936);
    if (argc < 2) { printf("用法: probe_lib <fne>\n"); return 2; }

    HMODULE h = LoadLibraryA(argv[1]);
    if (h == NULL) { printf("装载失败 %u\n", (unsigned)GetLastError()); return 1; }

    typedef PLIB_INFO(WINAPI* FnGet)(void);
    FnGet pGet = (FnGet)GetProcAddress(h, "GetNewInf");
    if (pGet == NULL) { printf("没有 GetNewInf\n"); return 1; }

    PLIB_INFO inf = pGet();
    if (inf == NULL) { printf("GetNewInf 返回空\n"); return 1; }
    if (inf->m_pfnNotify) inf->m_pfnNotify(NL_SYS_NOTIFY_FUNCTION, (DWORD)&FakeNotify, 0);

    printf("== %s\n", argv[1]);
    printf("  库名=%s 版本=%d.%d.%d 要求易=%d.%d 要求核心=%d.%d 语言=%d 库状态=0x%08X\n",
           (LPCSTR)inf->m_szName, inf->m_nMajorVersion, inf->m_nMinorVersion, inf->m_nBuildNumber,
           inf->m_nRqSysMajorVer, inf->m_nRqSysMinorVer, inf->m_nRqSysKrnlLibMajorVer, inf->m_nRqSysKrnlLibMinorVer,
           inf->m_nLanguage, (unsigned)inf->m_dwState);
    printf("  数据类型数=%d 命令数=%d 类别数=%d 常量数=%d\n",
           inf->m_nDataTypeCount, inf->m_nCmdCount, inf->m_nCategoryCount, inf->m_nLibConstCount);

    for (int i = 0; i < inf->m_nDataTypeCount; i++)
    {
        PLIB_DATA_TYPE_INFO d = &inf->m_pDataType[i];
        printf("  [%d] %s / %s 状态=0x%08X 窗口单元=%d 容器=%d 功能提供=%d 不可获焦=%d 默认停TAB=%d\n",
               i, (LPCSTR)d->m_szName, d->m_szEgName ? (LPCSTR)d->m_szEgName : "-",
               (unsigned)d->m_dwState,
               (d->m_dwState & LDT_WIN_UNIT) ? 1 : 0,
               (d->m_dwState & LDT_IS_CONTAINER) ? 1 : 0,
               (d->m_dwState & LDT_IS_FUNCTION_PROVIDER) ? 1 : 0,
               (d->m_dwState & LDT_CANNOT_GET_FOCUS) ? 1 : 0,
               (d->m_dwState & LDT_DEFAULT_NO_TABSTOP) ? 1 : 0);
        printf("      成员命令=%d 索引表=%s 属性=%d 事件=%d 接口函数=%p 子成员=%d\n",
               d->m_nCmdCount, d->m_pnCmdsIndex ? "有" : "NULL", d->m_nPropertyCount,
               d->m_nEventCount, (void*)d->m_pfnGetInterface, d->m_nElementCount);
        DumpBitmap(h, d->m_dwUnitBmpID);

        if (d->m_pnCmdsIndex != NULL)
        {
            printf("      成员命令名:");
            for (int k = 0; k < d->m_nCmdCount && k < 8; k++)
            {
                int idx = d->m_pnCmdsIndex[k];
                if (idx >= 0 && idx < inf->m_nCmdCount)
                    printf(" %s", (LPCSTR)inf->m_pBeginCmdInfo[idx].m_szName);
            }
            printf(" ...\n");
        }
        if (d->m_pPropertyBegin != NULL)
        {
            for (int k = 0; k < d->m_nPropertyCount && k < 12; k++)
                printf("      属性#%-2d %-10s(%-14s) 类型=%d 状态=0x%X\n", k,
                       (LPCSTR)d->m_pPropertyBegin[k].m_szName,
                       d->m_pPropertyBegin[k].m_szEgName ? (LPCSTR)d->m_pPropertyBegin[k].m_szEgName : "-",
                       (int)d->m_pPropertyBegin[k].m_shtType, (unsigned)d->m_pPropertyBegin[k].m_wState);
        }
    }
    return 0;
}
