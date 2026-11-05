// JSON 追加小工具：全部带容量钳位。
// 写在这里而不是生成器里，是为了让「重新生成」不会把安全改动冲掉。
#pragma once

namespace wv {

#ifndef UNREF
#define UNREF(x) (void)(x)
#endif

inline void J_Comma(char* b, int cap, int* pn)
{
    if (b == NULL || pn == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (*pn > 0 && b[*pn - 1] != '{' && b[*pn - 1] != '[')
    {
        b[*pn] = ',';
        b[*pn + 1] = '\0';
        (*pn)++;
    }
}

inline void J_Key(char* b, int cap, int* pn, LPCSTR k)
{
    if (b == NULL || pn == NULL || cap <= 0 || *pn >= cap - 1) return;
    J_Comma(b, cap, pn);
    if (*pn >= cap - 1) return;
    const int n = _snprintf(b + *pn, (size_t)(cap - *pn - 1), "\"%s\":", k);
    if (n > 0) *pn += n;
    if (*pn > cap - 1) *pn = cap - 1;
    b[*pn] = '\0';
}

inline void J_Open(char* b, int cap, int* pn, char ch)
{
    if (b == NULL || pn == NULL || cap <= 0 || *pn >= cap - 1) return;
    J_Comma(b, cap, pn);
    b[*pn] = ch;
    b[*pn + 1] = '\0';
    (*pn)++;
}

inline void J_Close(char* b, int cap, int* pn, char ch)
{
    if (b == NULL || pn == NULL || cap <= 0 || *pn >= cap - 1) return;
    J_Comma(b, cap, pn);
    b[*pn] = ch;
    b[*pn + 1] = '\0';
    (*pn)++;
}

inline void J_Num(char* b, int cap, int* pn, LPCSTR k, double v, BOOL blDbl)
{
    J_Key(b, cap, pn, k);
    if (b == NULL || pn == NULL || cap <= 0 || *pn >= cap - 1) return;
    const int n = blDbl ? _snprintf(b + *pn, (size_t)(cap - *pn - 1), "%g", v)
                        : _snprintf(b + *pn, (size_t)(cap - *pn - 1), "%d", (int)v);
    if (n > 0) *pn += n;
    if (*pn > cap - 1) *pn = cap - 1;
    b[*pn] = '\0';
}

inline void J_Bool(char* b, int cap, int* pn, LPCSTR k, BOOL v)
{
    J_Key(b, cap, pn, k);
    if (b == NULL || pn == NULL || cap <= 0 || *pn >= cap - 1) return;
    const int n = _snprintf(b + *pn, (size_t)(cap - *pn - 1), v ? "true" : "false");
    if (n > 0) *pn += n;
    if (*pn > cap - 1) *pn = cap - 1;
    b[*pn] = '\0';
}

// UTF-16 -> UTF-8 并做 JSON 转义；单条最长 4KB 原文，超出按截断处理。
inline void J_StrW(char* b, int cap, int* pn, LPCSTR k, LPCWSTR v)
{
    J_Key(b, cap, pn, k);
    if (b == NULL || pn == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (v == NULL)
    {
        const int n0 = _snprintf(b + *pn, (size_t)(cap - *pn - 1), "null");
        if (n0 > 0) *pn += n0;
        b[*pn > cap - 1 ? cap - 1 : *pn] = '\0';
        return;
    }

    char tmp[4096];
    tmp[0] = '\0';
    if (WideCharToMultiByte(CP_UTF8, 0, v, -1, tmp, (int)sizeof(tmp) - 1, NULL, NULL) == 0) tmp[0] = '\0';

    char esc[8192];
    int e = 0;
    for (int i = 0; tmp[i] && e < (int)sizeof(esc) - 7; i++)
    {
        const unsigned char c = (unsigned char)tmp[i];
        if (c == '"' || c == '\\') { esc[e++] = '\\'; esc[e++] = (char)c; }
        else if (c == '\n') { esc[e++] = '\\'; esc[e++] = 'n'; }
        else if (c == '\r') { esc[e++] = '\\'; esc[e++] = 'r'; }
        else if (c == '\t') { esc[e++] = '\\'; esc[e++] = 't'; }
        else if (c < 0x20) { esc[e++] = '?'; }
        else esc[e++] = (char)c;
    }
    esc[e] = '\0';

    char q[8200];
    q[0] = '"';
    memcpy(q + 1, esc, (size_t)e);
    q[1 + e] = '"';
    q[2 + e] = '\0';

    const int n = _snprintf(b + *pn, (size_t)(cap - *pn - 1), "%s", q);
    if (n > 0) *pn += n;
    if (*pn > cap - 1) *pn = cap - 1;
    b[*pn] = '\0';
}

// 参数对象之间存在 A→B→A 的互相引用（请求 ↔ 上下文之类），快照必须限深，
// 否则递归到爆栈，现象是事件一触发就 0xC0000005。
inline int& JDepth() { static int d = 0; return d; }
inline BOOL JEnter() { if (JDepth() >= 6) return FALSE; JDepth()++; return TRUE; }
inline void JLeave() { if (JDepth() > 0) JDepth()--; }

inline int JPutText(LPSTR outText, INT cap, LPCWSTR s)
{
    if (outText == NULL || cap <= 0) return 0;
    outText[0] = '\0';
    if (s == NULL || s[0] == L'\0') return 1;
    return WideCharToMultiByte(CP_UTF8, 0, s, -1, outText, cap - 1, NULL, NULL) != 0;
}

}  // namespace wv
