// zyJson2.51 引擎：数字文本、转义、路径、解析状态机、序列化。
// 全部按源码口径实现：内部 UTF-16；对象成员插入序；数字序列化优先回放原文（raw）。
#include <cx_json.h>

namespace cx {

// ============================================================ 整数 / 长整数 → 文本

void JvIntToText(INT i, WCHAR* pOut, INT nCapChars)
{
    CHAR szA[16];
    INT n = 0, start = 0;
    UINT u;
    if (i < 0) { szA[n++] = '-'; start = 1; u = (UINT)(-(i + 1)) + 1u; }
    else u = (UINT)i;
    CHAR szTmp[12];
    INT nTmp = 0;
    do { szTmp[nTmp++] = (CHAR)('0' + (u % 10)); u /= 10; } while (u != 0);
    for (INT k = nTmp - 1; k >= 0; k--) szA[n++] = szTmp[k];
    szA[n] = '\0';
    if (nCapChars > 0)
    {
        INT m = (n < nCapChars - 1) ? n : nCapChars - 1;
        for (INT k = 0; k < m; k++) pOut[k] = (WCHAR)(BYTE)szA[k];
        pOut[m] = 0;
    }
}

void JvInt64ToText(__int64 l, WCHAR* pOut, INT nCapChars)
{
    CHAR szA[24];
    INT n = 0, start = 0;
    UINT64 u;
    if (l < 0) { szA[n++] = '-'; start = 1; u = (UINT64)(-(l + 1)) + 1ull; }
    else u = (UINT64)l;
    CHAR szTmp[21];
    INT nTmp = 0;
    do { szTmp[nTmp++] = (CHAR)('0' + (u % 10)); u /= 10; } while (u != 0);
    for (INT k = nTmp - 1; k >= 0; k--) szA[n++] = szTmp[k];
    szA[n] = '\0';
    if (nCapChars > 0)
    {
        INT m = (n < nCapChars - 1) ? n : nCapChars - 1;
        for (INT k = 0; k < m; k++) pOut[k] = (WCHAR)(BYTE)szA[k];
        pOut[m] = 0;
    }
}

// ============================================================ 双精度 → 文本
// 对齐易语言 到文本(双精度小数型)：15 位有效数字、去尾零、必要时科学计数。
// 用精确十进制展开（value = 尾数 × 5^k / 10^k）后再舍入，不用 CRT printf：
// 一是静态编译拿不到 ___stdio_common_v*，二是保证同一台机器上结果恒定。

#define JV_BIG_BASE      1000000000        // 1e9
#define JV_BIG_LIMBS     120               // 9*120 = 1080 位十进制，够放 5^1074

static void JvBigMulSmall(INT* pLimbs, INT* pnLen, INT nFactor)
{
    INT64 carry = 0;
    for (INT i = 0; i < *pnLen; i++)
    {
        INT64 cur = (INT64)pLimbs[i] * nFactor + carry;
        pLimbs[i] = (INT)(cur % JV_BIG_BASE);
        carry = cur / JV_BIG_BASE;
    }
    while (carry != 0)
    {
        if (*pnLen >= JV_BIG_LIMBS) break;
        pLimbs[(*pnLen)++] = (INT)(carry % JV_BIG_BASE);
        carry /= JV_BIG_BASE;
    }
}

static INT JvBigToDigits(const INT* pLimbs, INT nLen, CHAR* pDigits)   // 返回十进制位数
{
    if (nLen <= 0) { pDigits[0] = '0'; return 1; }
    CHAR szTmp[10];
    INT nTop = pLimbs[nLen - 1];
    INT n = 0;
    INT nTmp = 0;
    do { szTmp[nTmp++] = (CHAR)('0' + (nTop % 10)); nTop /= 10; } while (nTop != 0);
    for (INT k = nTmp - 1; k >= 0; k--) pDigits[n++] = szTmp[k];
    for (INT i = nLen - 2; i >= 0; i--)
    {
        INT v = pLimbs[i];
        for (INT k = 8; k >= 0; k--)
        {
            szTmp[k] = (CHAR)('0' + (v % 10));
            v /= 10;
        }
        for (INT k = 0; k < 9; k++) pDigits[n++] = szTmp[k];
    }
    return n;
}

void JvDoubleToText(double d, WCHAR* pOut, INT nCapChars)
{
    if (nCapChars <= 0) return;
    pOut[0] = 0;

    UINT64 bits = 0;
    memcpy(&bits, &d, sizeof(bits));
    INT bNeg = (bits >> 63) != 0;
    INT nExpBits = (INT)((bits >> 52) & 0x7ff);
    UINT64 nMant = bits & 0xfffffffffffffull;

    CHAR szOut[80];
    INT nOut = 0;
    CHAR szSign = 0;

    if (nExpBits == 0x7ff)                        // NaN / Inf
    {
        LPCSTR psz = bNeg ? "-nan" : "nan";
        if (nMant == 0) psz = bNeg ? "-inf" : "inf";
        for (INT i = 0; psz[i] != '\0'; i++) szOut[nOut++] = psz[i];
        szOut[nOut] = '\0';
    }
    else
    {
        INT e2;
        if (nExpBits == 0) e2 = -1074;            // 非规格化
        else { nMant |= (1ull << 52); e2 = nExpBits - 1075; }
        while (e2 > 0 && (nMant & 1ull) == 0) { nMant >>= 1; e2++; }

        INT limbs[JV_BIG_LIMBS];
        INT nLimbs = 0;
        UINT64 m = nMant;
        while (m != 0) { limbs[nLimbs++] = (INT)(m % JV_BIG_BASE); m /= JV_BIG_BASE; }
        if (nLimbs == 0) limbs[nLimbs++] = 0;
        INT nScale = 0;
        if (e2 >= 0) { for (INT i = 0; i < e2; i++) JvBigMulSmall(limbs, &nLimbs, 2); }
        else { for (INT i = 0; i < -e2; i++) JvBigMulSmall(limbs, &nLimbs, 5); nScale = -e2; }

        CHAR szDig[1100];
        INT nDig = JvBigToDigits(limbs, nLimbs, szDig);
        while (nDig > 1 && nScale > 0 && szDig[nDig - 1] == '0') { nDig--; nScale--; }   // 去尾零必须同时减标度

        INT i0 = 0;
        while (i0 < nDig && szDig[i0] == '0') i0++;
        if (i0 >= nDig)
        {
            nOut = 0;
            if (bNeg) szOut[nOut++] = '-';          // -0 与 %g 一致保留负号
            szOut[nOut++] = '0';
            szOut[nOut] = '\0';
        }
        else
        {
            INT nExp10 = (nDig - nScale - 1) - i0;             // 第一位有效数字的 10 次幂
            const INT nSig = 15;
            CHAR szSig[24];
            INT nSigLen = 0;
            for (INT k = 0; k < nSig; k++)
            {
                INT idx = i0 + k;
                szSig[nSigLen++] = (idx < nDig) ? szDig[idx] : '0';
            }
            INT nNextIdx = i0 + nSig;
            CHAR cNext = (nNextIdx < nDig) ? szDig[nNextIdx] : '0';
            if (cNext >= '5')                                   // 四舍五入 + 连进位
            {
                INT k = nSigLen - 1;
                while (k >= 0)
                {
                    if (szSig[k] == '9') { szSig[k] = '0'; k--; }
                    else { szSig[k] = (CHAR)(szSig[k] + 1); break; }
                }
                if (k < 0)
                {
                    memmove(szSig + 1, szSig, (SIZE_T)nSigLen);
                    szSig[0] = '1';
                    nSigLen++;
                    nExp10++;
                }
            }
            while (nSigLen > 1 && szSig[nSigLen - 1] == '0') nSigLen--;

            BOOL bSci = (nExp10 < -4 || nExp10 >= nSig);
            if (bNeg && szSign == 0) { szOut[nOut++] = '-'; szSign = '-'; }
            if (bSci)
            {
                szOut[nOut++] = szSig[0];
                if (nSigLen > 1)
                {
                    szOut[nOut++] = '.';
                    for (INT k = 1; k < nSigLen; k++) szOut[nOut++] = szSig[k];
                }
                szOut[nOut++] = 'e';
                INT nE = nExp10;
                if (nE < 0) { szOut[nOut++] = '-'; nE = -nE; }
                else szOut[nOut++] = '+';
                CHAR szE[8];
                INT nElen = 0;
                do { szE[nElen++] = (CHAR)('0' + (nE % 10)); nE /= 10; } while (nE != 0);
                while (nElen < 2) szE[nElen++] = '0';
                for (INT k = nElen - 1; k >= 0; k--) szOut[nOut++] = szE[k];
            }
            else if (nExp10 >= 0)
            {
                INT nInt = nExp10 + 1;
                if (nInt > nSigLen)
                {
                    for (INT k = 0; k < nInt; k++)
                        szOut[nOut++] = (k < nSigLen) ? szSig[k] : '0';
                }
                else
                {
                    for (INT k = 0; k < nInt; k++) szOut[nOut++] = szSig[k];
                    if (nSigLen > nInt)
                    {
                        szOut[nOut++] = '.';
                        for (INT k = nInt; k < nSigLen; k++) szOut[nOut++] = szSig[k];
                    }
                }
            }
            else
            {
                szOut[nOut++] = '0';
                szOut[nOut++] = '.';
                for (INT k = 0; k < -nExp10 - 1; k++) szOut[nOut++] = '0';
                for (INT k = 0; k < nSigLen; k++) szOut[nOut++] = szSig[k];
            }
            szOut[nOut] = '\0';
            nOut = (INT)lstrlenA(szOut);
        }
    }

    INT m = (nOut < nCapChars - 1) ? nOut : nCapChars - 1;
    for (INT i = 0; i < m; i++) pOut[i] = (WCHAR)(BYTE)szOut[i];
    pOut[m] = 0;
}

// ============================================================ 文本 → 数值
// 对齐原版：到数值(GBK) / _wtoi64 / StrToInt —— 跳前导空白、可选符号、遇非数字即停。

static INT JvSkipSpace(const WCHAR* p, INT i)
{
    while (p[i] == ' ' || p[i] == '\t' || p[i] == 13 || p[i] == 10) i++;
    return i;
}

double JvTextToDouble(const WCHAR* p)
{
    if (p == NULL) return 0.0;
    INT i = JvSkipSpace(p, 0);
    INT bNeg = 0;
    if (p[i] == '-' || p[i] == '+') { bNeg = (p[i] == '-'); i++; }
    double d = 0.0;
    INT nDigits = 0;
    while (p[i] >= '0' && p[i] <= '9') { d = d * 10.0 + (double)(p[i] - '0'); i++; nDigits++; }
    if (p[i] == '.')
    {
        i++;
        double f = 0.1;
        while (p[i] >= '0' && p[i] <= '9') { d += (double)(p[i] - '0') * f; f *= 0.1; i++; nDigits++; }
    }
    if (nDigits == 0) return 0.0;
    if ((p[i] == 'e' || p[i] == 'E'))
    {
        INT j = i + 1;
        INT bENeg = 0;
        if (p[j] == '-' || p[j] == '+') { bENeg = (p[j] == '-'); j++; }
        INT nE = 0, nEDig = 0;
        while (p[j] >= '0' && p[j] <= '9') { nE = nE * 10 + (p[j] - '0'); j++; nEDig++; }
        if (nEDig > 0)
        {
            INT nLimit = 320;
            if (nE > nLimit) nE = nLimit;
            while (nE-- > 0) d = bENeg ? d / 10.0 : d * 10.0;
        }
    }
    return bNeg ? -d : d;
}

__int64 JvTextToI64(const WCHAR* p)
{
    if (p == NULL) return 0;
    INT i = JvSkipSpace(p, 0);
    INT bNeg = 0;
    if (p[i] == '-' || p[i] == '+') { bNeg = (p[i] == '-'); i++; }
    UINT64 u = 0;
    while (p[i] >= '0' && p[i] <= '9') { u = u * 10ull + (UINT64)(p[i] - '0'); i++; }
    __int64 l = (__int64)u;
    return bNeg ? -l : l;
}

INT JvTextToInt(const WCHAR* p)
{
    if (p == NULL) return 0;
    INT i = JvSkipSpace(p, 0);
    INT bNeg = 0;
    if (p[i] == '-' || p[i] == '+') { bNeg = (p[i] == '-'); i++; }
    UINT u = 0;
    while (p[i] >= '0' && p[i] <= '9') { u = u * 10u + (UINT)(p[i] - '0'); i++; }
    INT r = (INT)u;
    return bNeg ? -r : r;
}

// ============================================================ 转义

static INT JvHex4(const WCHAR* p, INT* pnVal)     // 4 位十六进制；非法返回假
{
    INT v = 0;
    for (INT k = 0; k < 4; k++)
    {
        WCHAR c = p[k];
        INT d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
        else return FALSE;
        v = v * 16 + d;
    }
    *pnVal = v;
    return TRUE;
}

// 原地解码（缓冲区长度足够，只会变短）：与原版 TranslationIn 一致。
void JvTranslationIn(WCHAR* pBuf, INT bTransU)
{
    if (pBuf == NULL) return;
    INT len = JvWideLen(pBuf);
    INT i = 0;
    while (i < len)
    {
        if (pBuf[i] != '\\') { i++; continue; }
        WCHAR nxt = (i + 1 < len) ? pBuf[i + 1] : 0;
        INT nNew = 0;
        WCHAR ch1 = 0;
        BOOL bRemove = FALSE;
        if (nxt == '\\' || nxt == '/' || nxt == '"')
        {
            bRemove = TRUE;                       // 吃掉反斜杠
        }
        else if (nxt == 'b') { ch1 = 8;  nNew = 1; }
        else if (nxt == 't') { ch1 = 9;  nNew = 1; }
        else if (nxt == 'r') { ch1 = 13; nNew = 1; }
        else if (nxt == 'f') { ch1 = 12; nNew = 1; }
        else if (nxt == 'n')
        {
            if (i > 0 && pBuf[i - 1] == 13) { ch1 = 10; nNew = 1; }   // 前面已是 CR → 只写 LF
            else { pBuf[i] = 13; pBuf[i + 1] = 10; i += 2; continue; } // 变成 CRLF，长度不变
        }
        else if (nxt == 'u' && bTransU)
        {
            INT v = 0;
            if (i + 5 < len && JvHex4(pBuf + i + 2, &v))
            {
                pBuf[i] = (WCHAR)v;
                memmove(pBuf + i + 1, pBuf + i + 6, (SIZE_T)(len - i - 6) * sizeof(WCHAR));
                len -= 5;
                pBuf[len] = 0;
                i++;
                continue;
            }
            pBuf[i] = 0;                          // 非法十六进制 → 写 0，字符串在此截断
            len = i;
            pBuf[len] = 0;
            return;
        }
        if (bRemove)
        {
            memmove(pBuf + i, pBuf + i + 1, (SIZE_T)(len - i - 1) * sizeof(WCHAR));
            len--;
            pBuf[len] = 0;
            i++;
            continue;
        }
        if (nNew == 1)
        {
            pBuf[i] = ch1;
            memmove(pBuf + i + 1, pBuf + i + 2, (SIZE_T)(len - i - 2) * sizeof(WCHAR));
            len--;
            pBuf[len] = 0;
            i++;
            continue;
        }
        i += 2;                                   // 未知转义原样保留
    }
}

void JvTranslationOut(const WCHAR* pSrc, UniWBuf* pOut)
{
    if (pOut == NULL) return;
    if (pSrc == NULL) return;
    INT n = JvWideLen(pSrc);
    for (INT i = 0; i < n; i++)
    {
        WCHAR c = pSrc[i];
        if (c == '\\' || c == '/' || c == '"')
        {
            pOut->AddChar(L'\\');
            pOut->AddChar(c);
        }
        else if (c == 8)  { pOut->AddChar(L'\\'); pOut->AddChar(L'b'); }
        else if (c == 9)  { pOut->AddChar(L'\\'); pOut->AddChar(L't'); }
        else if (c == 10) { pOut->AddChar(L'\\'); pOut->AddChar(L'n'); }
        else if (c == 13) { pOut->AddChar(L'\\'); pOut->AddChar(L'r'); }
        else if (c == 12) { pOut->AddChar(L'\\'); pOut->AddChar(L'f'); }
        else pOut->AddChar(c);
    }
}

// ============================================================ 路径

#define JVPS_NAME 1
#define JVPS_INDEX 2

struct JvPathStep
{
    INT    flags;
    WCHAR* name;
    INT    index;
};

static void JvPathFree(JvPathStep* p, INT nCount)
{
    if (p == NULL) return;
    for (INT i = 0; i < nCount; i++) if (p[i].name != NULL) JvFree(p[i].name);
    JvFree(p);
}

// 返回步骤数（0 表示空路径），*ppSteps 由调用者 JvPathFree 释放。
static INT JvPathParse(const WCHAR* pPathIn, WCHAR** ppBuf, JvPathStep** ppSteps)
{
    *ppSteps = NULL;
    *ppBuf = NULL;
    if (pPathIn == NULL) return 0;
    INT nLen = JvWideLen(pPathIn);
    if (nLen == 0) return 0;
    WCHAR* p = JvWideDup(pPathIn, nLen);          // 转义是原地删反斜杠，必须可写副本
    if (p == NULL) return 0;
    *ppBuf = p;

    JvPathStep* steps = (JvPathStep*)JvMalloc((SIZE_T)(nLen + 1) * sizeof(JvPathStep));
    if (steps == NULL) return 0;
    *ppSteps = steps;
    INT count = 0;

    INT pos = 0, namePos = 0, indexPos = 0;
    BOOL bHasIndex = FALSE;
    while (pos <= nLen)
    {
        WCHAR w = (pos < nLen) ? p[pos] : 0;
        if (w == 92)                               /* \ */
        {
            WCHAR next = (pos + 1 < nLen) ? p[pos + 1] : 0;
            if (next == '.' || next == '[' || next == ']' || next == '\\')
            {
                memmove(p + pos, p + pos + 1, (SIZE_T)(nLen - pos - 1) * sizeof(WCHAR));
                p[nLen - 1] = 0;
                nLen--;
            }
        }
        else if (w == '.' || pos == nLen)
        {
            if (namePos < pos)
            {
                steps[count].flags = JVPS_NAME;
                steps[count].name = JvWideDup(p + namePos, pos - namePos);
                steps[count].index = 0;
                count++;
            }
            namePos = pos + 1;
        }
        else if (w == '[')
        {
            indexPos = pos + 1;
            bHasIndex = TRUE;
            if (namePos < pos)
            {
                steps[count].flags = JVPS_NAME;
                steps[count].name = JvWideDup(p + namePos, pos - namePos);
                steps[count].index = 0;
                count++;
            }
            namePos = pos + 1;
        }
        else if (w == ']')
        {
            if (bHasIndex)
            {
                WCHAR* pIdx = JvWideDup(p + indexPos, pos - indexPos);
                INT nIdx = (pIdx == NULL) ? 0 : JvTextToInt(pIdx);
                if (pIdx != NULL) JvFree(pIdx);
                steps[count].flags = JVPS_INDEX;
                steps[count].name = NULL;
                steps[count].index = nIdx;
                count++;
            }
            namePos = pos + 1;
        }
        pos++;
    }
    return count;
}

CxJsonNode* JvSelect(CxJsonNode* n, const WCHAR* pPath, INT bCreate)
{
    WCHAR* pBuf = NULL;
    JvPathStep* steps = NULL;
    INT count = JvPathParse(pPath, &pBuf, &steps);
    if (count <= 0) { if (steps) JvPathFree(steps, 0); if (pBuf) JvFree(pBuf); return n; }
    CxJsonNode* jv = n;
    for (INT i = 0; i < count; i++)
    {
        if (steps[i].flags == JVPS_NAME && steps[i].name != NULL)
        {
            jv = bCreate ? JvChildOrCreate(jv, steps[i].name) : JvChildReadOnly(jv, steps[i].name);
            if (!bCreate && jv == NULL) break;
        }
        else if (steps[i].flags == JVPS_INDEX)
        {
            if (bCreate && i == 0 && (steps[i].name == NULL))
            {
                if (!JvSetArray(jv)) { jv = NULL; break; }
            }
            jv = bCreate ? JvAtOrCreate(jv, steps[i].index) : JvAtReadOnly(jv, steps[i].index);
            if (!bCreate && jv == NULL) break;
        }
    }
    JvPathFree(steps, count);
    JvFree(pBuf);
    return jv;
}

BOOL JvQuery(CxJsonNode* n, const WCHAR* pPath)
{
    WCHAR* pBuf = NULL;
    JvPathStep* steps = NULL;
    INT count = JvPathParse(pPath, &pBuf, &steps);
    if (count <= 0) { if (steps) JvPathFree(steps, 0); if (pBuf) JvFree(pBuf); return FALSE; }
    CxJsonNode* jv = n;
    BOOL ret = TRUE;
    for (INT i = 0; i < count; i++)
    {
        if (steps[i].flags == JVPS_NAME && steps[i].name != NULL)
        {
            jv = JvChildReadOnly(jv, steps[i].name);
            if (jv == NULL) { ret = FALSE; break; }
        }
        else if (steps[i].flags == JVPS_INDEX)
        {
            jv = JvAtReadOnly(jv, steps[i].index);
            if (jv == NULL) { ret = FALSE; break; }
        }
    }
    JvPathFree(steps, count);
    JvFree(pBuf);
    return ret;
}

// ============================================================ 序列化

static void JvToStringProc(const CxJsonNode* n, UniWBuf* pOut, INT bAutoFormat, INT nIndent, INT bIsTrans)
{
    if (n == NULL || pOut == NULL) return;
    switch (n->type)
    {
    case CXJV_NULL: pOut->Add(L"null", 4); break;
    case CXJV_BOOL: pOut->Add(n->v.b ? L"true" : L"false", n->v.b ? 4 : 5); break;
    case CXJV_DOUBLE:
    case CXJV_LONG:
    case CXJV_UNDEFINED:
        if (n->raw != NULL) pOut->Add(n->raw, JvWideLen(n->raw));
        break;
    case CXJV_STRING:
        pOut->AddChar(L'"');
        if (bIsTrans) JvTranslationOut(n->v.str, pOut);
        else if (n->v.str != NULL) pOut->Add(n->v.str, JvWideLen(n->v.str));
        pOut->AddChar(L'"');
        break;
    case CXJV_ARRAY:
    {
        if (n->v.arr == NULL) break;
        CxJsonArr* a = n->v.arr;
        pOut->AddChar(L'[');
        nIndent++;
        for (INT i = 0; i < a->count; i++)
        {
            JvToStringProc(a->items[i], pOut, bAutoFormat, nIndent, bIsTrans);
            if (i != a->count - 1) pOut->AddChar(L',');
        }
        nIndent--;
        pOut->AddChar(L']');
        break;
    }
    case CXJV_OBJECT:
    {
        if (n->v.obj == NULL) break;
        CxJsonObj* o = n->v.obj;
        pOut->AddChar(L'{');
        if (bAutoFormat) pOut->Add(L"\r\n", 2);
        nIndent++;
        for (INT i = 0; i < o->count; i++)
        {
            CxJsonNode* child = o->order[i];
            if (bAutoFormat && nIndent > 0)
            {
                for (INT k = 0; k < nIndent; k++) pOut->AddChar(L'\t');
            }
            pOut->AddChar(L'"');
            if (bIsTrans) JvTranslationOut((child != NULL) ? child->key : NULL, pOut);
            else if (child != NULL && child->key != NULL) pOut->Add(child->key, JvWideLen(child->key));
            pOut->AddChar(L'"');
            pOut->AddChar(L':');
            JvToStringProc(child, pOut, bAutoFormat, nIndent, bIsTrans);
            if (i != o->count - 1) pOut->AddChar(L',');
            if (bAutoFormat) pOut->Add(L"\r\n", 2);
        }
        nIndent--;
        if (bAutoFormat && nIndent > 0)
        {
            for (INT k = 0; k < nIndent; k++) pOut->AddChar(L'\t');
        }
        pOut->AddChar(L'}');
        break;
    }
    default: break;
    }
}

void JvToString(const CxJsonNode* n, UniWBuf* pOut, INT bAutoFormat, INT bIsTrans)
{
    if (pOut == NULL) return;
    pOut->Clear();
    if (n == NULL) return;
    JvToStringProc(n, pOut, bAutoFormat, 0, bIsTrans);
}

// ============================================================ 数字判定 / token 类型
// 与原版 JsonValue_Parsed_ValueIsNumber 一致：允许 0-9、至多一个 . / e / +-，
// 空白只跳过不计数；isDouble 当且仅当 . e +- 各出现过恰好一次。
BOOL JvIsNumberToken(const WCHAR* p, INT nLen, INT* pbIsDouble)
{
    INT i = 0, nDot = 0, nE = 0, nSign = 0;
    if (p == NULL || nLen <= 0) return FALSE;
    while (i < nLen)
    {
        WCHAR w = p[i];
        if (w <= 32) { i++; continue; }             // 空白只跳过，不计数
        if (i == 0)                                  // 「首字符」看的是当前位置，空白会推开它
        {
            if (w != '-' && w != '+' && (w < '0' || w > '9'))
            {
                if (w == '.') { nDot++; if (nDot > 1) return FALSE; i++; continue; }
                return FALSE;
            }
        }
        else if (w < '0' || w > '9')
        {
            if (w == '.') { nDot++; if (nDot > 1) return FALSE; i++; continue; }
            if (w == 'e' || w == 'E') { nE++; if (nE > 1) return FALSE; i++; continue; }
            if (w == '+' || w == '-') { nSign++; if (nSign > 1) return FALSE; i++; continue; }
            return FALSE;
        }
        i++;
    }
    if (pbIsDouble != NULL) *pbIsDouble = (nDot == 1 || nE == 1 || nSign == 1) ? 1 : 0;
    return TRUE;
}

INT JvTokenType(const WCHAR* pValue, INT nValueLen)
{
    if (pValue == NULL || nValueLen <= 0) return CXJV_UNDEFINED;
    if (nValueLen == 4 && pValue[0] == 'n' && pValue[1] == 'u' && pValue[2] == 'l' && pValue[3] == 'l') return CXJV_NULL;
    if (nValueLen == 4 && pValue[0] == 't' && pValue[1] == 'r' && pValue[2] == 'u' && pValue[3] == 'e') return CXJV_BOOL;
    if (nValueLen == 5 && pValue[0] == 'f' && pValue[1] == 'a' && pValue[2] == 'l' && pValue[3] == 's' && pValue[4] == 'e') return CXJV_BOOL;
    INT bDouble = 0;
    if (JvIsNumberToken(pValue, nValueLen, &bDouble)) return bDouble ? CXJV_DOUBLE : CXJV_LONG;
    return CXJV_UNDEFINED;
}

// ============================================================ 解析状态机
#define JVP_NEED_NAME  1
#define JVP_NEED_VALUE 2
#define JVP_IS_ENDED   3

struct JvParseCtx
{
    CxJsonNode* root;
    WCHAR*      p;
    INT         strLen;
    INT         bTransU;
    INT         error;
    CxJsonNode** stackNode;
    INT*         stackStatus;
    INT          top;
    INT          cap;
    INT         namePos;
    INT         nameLen;
};

static BOOL JvPush(JvParseCtx* c, CxJsonNode* n, INT status)
{
    if (c->top >= c->cap)
    {
        INT nNew = (c->cap == 0) ? 1024 : c->cap * 2;      // 原版容器栈按 8 字节/项翻倍扩容
        CxJsonNode** pN = (CxJsonNode**)JvMalloc((SIZE_T)nNew * sizeof(CxJsonNode*));
        INT* pS = (INT*)JvMalloc((SIZE_T)nNew * sizeof(INT));
        if (pN == NULL || pS == NULL)
        {
            if (pN != NULL) JvFree(pN);
            if (pS != NULL) JvFree(pS);
            return FALSE;
        }
        for (INT i = 0; i < c->top; i++) { pN[i] = c->stackNode[i]; pS[i] = c->stackStatus[i]; }
        if (c->stackNode != NULL) JvFree(c->stackNode);
        if (c->stackStatus != NULL) JvFree(c->stackStatus);
        c->stackNode = pN;
        c->stackStatus = pS;
        c->cap = nNew;
    }
    c->stackNode[c->top] = n;
    c->stackStatus[c->top] = status;
    c->top++;
    return TRUE;
}

static BOOL JvPeek(JvParseCtx* c, CxJsonNode** ppNode, INT* pStatus)
{
    if (c->top == 0) { *ppNode = NULL; *pStatus = 0; return FALSE; }
    *ppNode = c->stackNode[c->top - 1];
    *pStatus = c->stackStatus[c->top - 1];
    return TRUE;
}

static void JvSetTop(JvParseCtx* c, CxJsonNode* n, INT status)
{
    if (c->top == 0) return;
    c->stackNode[c->top - 1] = n;
    c->stackStatus[c->top - 1] = status;
}

static BOOL JvPop(JvParseCtx* c, CxJsonNode** ppNode, INT* pStatus)
{
    if (c->top == 0) return FALSE;
    c->top--;
    *ppNode = c->stackNode[c->top];
    *pStatus = c->stackStatus[c->top];
    return TRUE;
}

static WCHAR* JvSlice(JvParseCtx* c, INT nFrom, INT nChars)   // 切出可写副本（末尾带 {0,0}）
{
    WCHAR* p = (WCHAR*)JvMalloc((SIZE_T)(nChars + 1) * sizeof(WCHAR));
    if (p == NULL) return NULL;
    if (nChars > 0 && nFrom >= 0 && nFrom + nChars <= c->strLen)
        memcpy(p, c->p + nFrom, (SIZE_T)nChars * sizeof(WCHAR));
    p[nChars] = 0;
    return p;
}

static INT JvParseObject(JvParseCtx* c);
static INT JvParseArray(JvParseCtx* c);
static INT JvParseOther(JvParseCtx* c, INT* pPos);

static INT JvAttachChild(JvParseCtx* c, CxJsonNode* jvParent, INT nTypeParent, CxJsonNode* jv, INT valueStart, INT* pPos)
{
    if (jv == NULL) { *pPos = valueStart; return CXJSON_MEMORY; }
    if (nTypeParent == CXJV_ARRAY)
    {
        if (!JvAppend(jvParent, jv)) { JvRelease(jv); *pPos = valueStart; return CXJSON_MEMORY; }
    }
    else if (nTypeParent == CXJV_OBJECT)
    {
        WCHAR* pName = JvSlice(c, c->namePos, c->nameLen);
        if (pName == NULL) { JvRelease(jv); *pPos = valueStart; return CXJSON_MEMORY; }
        JvTranslationIn(pName, c->bTransU);
        if (!JvChildSet(jvParent, pName, jv))
        {
            JvFree(pName);
            JvRelease(jv);
            *pPos = valueStart;
            return CXJSON_MEMORY;
        }
        JvFree(pName);
        c->namePos = -1;
        c->nameLen = 0;
    }
    return CXJSON_OK;
}

static INT JvParseObject(JvParseCtx* c)
{
    CxJsonNode* jvParent = NULL;
    INT status = 0;
    JvPeek(c, &jvParent, &status);
    if (jvParent == NULL)                                  // 根对象
    {
        if (JvTypeOf(c->root) != CXJV_OBJECT && !JvSetObject(c->root)) return CXJSON_MEMORY;
        if (!JvPush(c, c->root, JVP_NEED_NAME)) return CXJSON_MEMORY;
        return CXJSON_OK;
    }
    if (status != JVP_NEED_VALUE) return CXJSON_INVALIDNAME;
    INT nType = JvTypeOf(jvParent);
    CxJsonNode* jv = JvNewObject();
    if (jv == NULL) return CXJSON_MEMORY;
    if (nType == CXJV_ARRAY)
    {
        if (!JvAppend(jvParent, jv)) { JvRelease(jv); return CXJSON_MEMORY; }
    }
    else
    {
        if (c->namePos == -1 || c->nameLen == 0) { JvRelease(jv); return CXJSON_INVALIDNAME; }
        WCHAR* pName = JvSlice(c, c->namePos, c->nameLen);
        if (pName == NULL) { JvRelease(jv); return CXJSON_MEMORY; }
        JvTranslationIn(pName, c->bTransU);
        if (!JvChildSet(jvParent, pName, jv)) { JvFree(pName); JvRelease(jv); return CXJSON_MEMORY; }
        JvFree(pName);
    }
    if (!JvPush(c, jv, JVP_NEED_NAME)) { JvRelease(jv); return CXJSON_MEMORY; }
    return CXJSON_OK;
}

static INT JvParseArray(JvParseCtx* c)
{
    CxJsonNode* jvParent = NULL;
    INT status = 0;
    JvPeek(c, &jvParent, &status);
    if (jvParent == NULL)
    {
        if (JvTypeOf(c->root) != CXJV_ARRAY && !JvSetArray(c->root)) return CXJSON_MEMORY;
        if (!JvPush(c, c->root, JVP_NEED_VALUE)) return CXJSON_MEMORY;
        return CXJSON_OK;
    }
    if (status != JVP_NEED_VALUE) return CXJSON_INVALIDNAME;
    INT nType = JvTypeOf(jvParent);
    CxJsonNode* jv = JvNewArray();
    if (jv == NULL) return CXJSON_MEMORY;
    if (nType == CXJV_ARRAY)
    {
        if (!JvAppend(jvParent, jv)) { JvRelease(jv); return CXJSON_MEMORY; }
    }
    else
    {
        if (c->namePos == -1 || c->nameLen == 0) { JvRelease(jv); return CXJSON_INVALIDNAME; }
        WCHAR* pName = JvSlice(c, c->namePos, c->nameLen);
        if (pName == NULL) { JvRelease(jv); return CXJSON_MEMORY; }
        JvTranslationIn(pName, c->bTransU);
        if (!JvChildSet(jvParent, pName, jv)) { JvFree(pName); JvRelease(jv); return CXJSON_MEMORY; }
        JvFree(pName);
    }
    if (!JvPush(c, jv, JVP_NEED_VALUE)) { JvRelease(jv); return CXJSON_MEMORY; }
    return CXJSON_OK;
}

// 值 token → 节点。jvParent 为 0 表示改写根节点（根标量）。
static INT JvStoreValue(JvParseCtx* c, CxJsonNode* jvParent, INT nTypeParent, WCHAR* pValue,
                        INT nValueLen, INT nValueStart, INT* pPos)
{
    WCHAR w0 = (nValueLen > 0) ? pValue[0] : 0;
    INT childType;
    if (w0 == '"' || w0 == '\'')
    {
        WCHAR wLast = (nValueLen > 0) ? pValue[nValueLen - 1] : 0;
        if (wLast != w0) { *pPos = nValueStart; return CXJSON_INVALIDVALUE; }
        childType = CXJV_STRING;
    }
    else childType = JvTokenType(pValue, nValueLen);

    CxJsonNode* jv = NULL;
    BOOL bRoot = (jvParent == NULL);
    switch (childType)
    {
    case CXJV_NULL:
        if (bRoot)
        {
            if (!JvSetNull(c->root)) { *pPos = nValueStart; return CXJSON_MEMORY; }
            return CXJSON_OK;
        }
        jv = JvNewNull();
        break;
    case CXJV_BOOL:
    {
        INT bTrue = (nValueLen == 4 && w0 == 't');
        if (bRoot)
        {
            if (!JvSetBool(c->root, bTrue)) { *pPos = nValueStart; return CXJSON_MEMORY; }
            return CXJSON_OK;
        }
        jv = JvNewBool(bTrue);
        break;
    }
    case CXJV_DOUBLE:
    {
        double d = JvTextToDouble(pValue);
        if (bRoot)
        {
            if (!JvSetDouble(c->root, d, pValue)) { *pPos = nValueStart; return CXJSON_MEMORY; }
            return CXJSON_OK;
        }
        jv = JvNewDouble(d, pValue);
        break;
    }
    case CXJV_LONG:
    {
        __int64 l = JvTextToI64(pValue);
        if (bRoot)
        {
            if (!JvSetLong(c->root, l, pValue)) { *pPos = nValueStart; return CXJSON_MEMORY; }
            return CXJSON_OK;
        }
        // 原版把容器里的整数存成双精度（取类型报 1），序列化仍回放原文。
        jv = JvNewDouble(JvI64ToDouble(l), pValue);
        break;
    }
    case CXJV_STRING:
    {
        JvTranslationIn(pValue, c->bTransU);
        INT nLen = JvWideLen(pValue) - 2;
        WCHAR* pBody = pValue + 1;
        if (nLen < 0) nLen = 0;
        if (bRoot)
        {
            if (!JvSetString(c->root, pBody, nLen)) { *pPos = nValueStart; return CXJSON_MEMORY; }
            return CXJSON_OK;
        }
        jv = JvNewString(pBody, nLen);
        break;
    }
    default:
        *pPos = nValueStart;
        return CXJSON_INVALIDVALUE;
    }
    if (jv == NULL) { *pPos = nValueStart; return CXJSON_MEMORY; }
    return JvAttachChild(c, jvParent, nTypeParent, jv, nValueStart, pPos);
}

static INT JvParseOther(JvParseCtx* c, INT* pPos)
{
    CxJsonNode* jvParent = NULL;
    INT status = 0;
    JvPeek(c, &jvParent, &status);

    if (jvParent == NULL)                       // 根标量：整段剩余当成一个 token
    {
        INT nValueStart = *pPos;
        INT nValueLen = c->strLen - nValueStart;
        if (nValueLen < 0) nValueLen = 0;
        WCHAR* pValue = JvSlice(c, 0, nValueLen);   // 原版切片起点写死 pStr，前置空白会丢尾字符
        if (pValue == NULL) return CXJSON_MEMORY;
        INT nPos = nValueStart + nValueLen;
        INT err = JvStoreValue(c, NULL, 0, pValue, nValueLen, nValueStart, &nPos);
        JvFree(pValue);
        *pPos = nPos;
        return err;
    }

    INT nType = JvTypeOf(jvParent);
    INT nValueStart = 0, nValueLen = 0, nQuote = 0, nameEnd = 0;
    INT preStatus = 0;
    WCHAR* pValue = NULL;

    if (nType == CXJV_ARRAY)
    {
        INT pos = *pPos;
        WCHAR w = (pos < c->strLen) ? c->p[pos] : 0;
        if (status == JVP_IS_ENDED)
        {
            status = JVP_NEED_VALUE;
            JvSetTop(c, jvParent, status);
            if (w == ',') { *pPos = pos; return CXJSON_OK; }
        }
        if (status != JVP_NEED_VALUE) { *pPos = pos; return CXJSON_INVALIDVALUE; }
        nValueStart = pos;
        BOOL bBreak = FALSE;
        while (pos < c->strLen)
        {
            w = c->p[pos];
            if ((w == ' ' || w == 9 || w == 13 || w == 10) && nQuote == 0)
            {
                pos++;
                if (nValueLen == 0) nValueStart = pos;
                continue;
            }
            if (w == '"' || w == '\'')
            {
                if (nQuote == 0) nQuote = w;
                else if (nQuote == w) nQuote = 0;
            }
            else if (w == '\\')
            {
                WCHAR next = (pos + 1 < c->strLen) ? c->p[pos + 1] : 0;
                if (next == 'u') { pos += 6; nValueLen += 6; }
                else { pos += 2; nValueLen += 2; }
                continue;
            }
            else if (w == '{' && nQuote == 0)
            {
                if (nValueLen != 0) { *pPos = nValueStart; return CXJSON_INVALIDVALUE; }
                *pPos = pos;
                return JvParseObject(c);
            }
            else if (w == '[' && nQuote == 0)
            {
                if (nValueLen != 0) { *pPos = nValueStart; return CXJSON_INVALIDVALUE; }
                *pPos = pos;
                return JvParseArray(c);
            }
            else if (w == ']' && nQuote == 0)
            {
                CxJsonNode* popped = NULL; INT st = 0;
                JvPop(c, &popped, &st);
                CxJsonNode* gp = NULL; INT gst = 0;
                if (JvPeek(c, &gp, &gst)) { preStatus = JVP_IS_ENDED; JvSetTop(c, gp, preStatus); }
                if (nValueLen == 0) { *pPos = pos; return CXJSON_OK; }
                bBreak = TRUE;
                break;
            }
            else if (w == ',' && nQuote == 0) { bBreak = TRUE; break; }
            pos++;
            nValueLen++;
        }
        *pPos = pos;
        if (!bBreak && nQuote != 0) { *pPos = nValueStart; return CXJSON_INVALIDVALUE; }
        if (nValueLen == 0) return CXJSON_OK;
        pValue = JvSlice(c, nValueStart, nValueLen);
        if (pValue == NULL) return CXJSON_MEMORY;
        INT errArr = JvStoreValue(c, jvParent, nType, pValue, nValueLen, nValueStart, pPos);
        JvFree(pValue);
        return errArr;
    }

    if (nType == CXJV_OBJECT)
    {
        INT pos = *pPos;
        WCHAR w = (pos < c->strLen) ? c->p[pos] : 0;
        if (status == JVP_IS_ENDED)
        {
            status = JVP_NEED_NAME;
            JvSetTop(c, jvParent, status);
            if (w == ',') { *pPos = pos; return CXJSON_OK; }
        }
        if (status != JVP_NEED_NAME) { *pPos = pos; return CXJSON_INVALIDNAME; }
        c->namePos = pos;
        c->nameLen = 0;
        nValueStart = 0;
        nValueLen = 0;
        BOOL bBreak = FALSE;
        while (pos < c->strLen)
        {
            w = c->p[pos];
            if ((w == ' ' || w == 9 || w == 13 || w == 10) && nQuote == 0)
            {
                pos++;
                if (status == JVP_NEED_NAME) { if (c->nameLen == 0) c->namePos = pos; }
                else { if (nValueLen == 0) nValueStart = pos; }
                continue;
            }
            if (w == '"' || w == '\'')
            {
                if (nQuote == 0)
                {
                    nQuote = w;
                    pos++;
                    if (status == JVP_NEED_NAME)
                    {
                        if (c->nameLen != 0) { *pPos = pos; return CXJSON_INVALIDNAME; }
                        c->namePos = pos;
                    }
                    else nValueLen++;
                    continue;
                }
                if (nQuote == w) nQuote = 0;
                pos++;
                if (status == JVP_NEED_VALUE) nValueLen++;
                else nameEnd = 1;
                continue;
            }
            if (w == '\\')
            {
                WCHAR next = (pos + 1 < c->strLen) ? c->p[pos + 1] : 0;
                if (next == 'u') { pos += 6; if (status == JVP_NEED_NAME) c->nameLen += 6; else nValueLen += 6; }
                else { pos += 2; if (status == JVP_NEED_NAME) c->nameLen += 2; else nValueLen += 2; }
                continue;
            }
            if (w == ':' && nQuote == 0)
            {
                if (status == JVP_NEED_NAME)
                {
                    if (c->nameLen == 0) { *pPos = pos; return CXJSON_INVALIDNAME; }
                    pos++;
                    nValueStart = pos;
                    nValueLen = 0;
                    status = JVP_NEED_VALUE;
                    JvSetTop(c, jvParent, status);
                    continue;
                }
                // 值里出现的冒号落到表尾，成为值的一部分
            }
            if (w == '{' && nQuote == 0)
            {
                if (nValueLen != 0) { *pPos = nValueStart; return CXJSON_INVALIDVALUE; }
                *pPos = pos;
                return JvParseObject(c);
            }
            if (w == '[' && nQuote == 0)
            {
                if (nValueLen != 0) { *pPos = nValueStart; return CXJSON_INVALIDVALUE; }
                *pPos = pos;
                return JvParseArray(c);
            }
            if (w == '}' && nQuote == 0)
            {
                CxJsonNode* popped = NULL; INT st = 0;
                JvPop(c, &popped, &st);
                CxJsonNode* gp = NULL; INT gst = 0;
                if (JvPeek(c, &gp, &gst)) { preStatus = JVP_IS_ENDED; JvSetTop(c, gp, preStatus); }
                else preStatus = 0;
                if (c->nameLen == 0) { *pPos = pos; return CXJSON_OK; }
                bBreak = TRUE;
                break;
            }
            if (w == ',' && nQuote == 0) { bBreak = TRUE; break; }
            if (status == JVP_NEED_NAME && nameEnd != 0) { *pPos = pos; return CXJSON_INVALIDNAME; }
            pos++;
            if (status == JVP_NEED_NAME) c->nameLen++;
            else nValueLen++;
        }
        *pPos = pos;
        if (c->nameLen == 0 && nValueLen != 0) { *pPos = c->namePos; return CXJSON_INVALIDNAME; }
        if (c->nameLen != 0 && nValueLen == 0) { *pPos = nValueStart; return CXJSON_INVALIDVALUE; }
        pValue = JvSlice(c, nValueStart, nValueLen);
        if (pValue == NULL) return CXJSON_MEMORY;
        if (preStatus != JVP_IS_ENDED) JvSetTop(c, jvParent, JVP_NEED_NAME);
        INT err = JvStoreValue(c, jvParent, nType, pValue, nValueLen, nValueStart, pPos);
        JvFree(pValue);
        return err;
    }

    UNREFERENCED_PARAMETER(nameEnd);
    UNREFERENCED_PARAMETER(preStatus);
    return CXJSON_OK;
}

INT JvParse(CxJsonNode* n, const WCHAR* pStr, INT nLen, INT bTransU, INT* pnErrPos)
{
    if (pnErrPos != NULL) *pnErrPos = 0;
    if (n == NULL || pStr == NULL || nLen <= 0) return CXJSON_INVALIDARG;
    JvParseCtx c;
    memset(&c, 0, sizeof(c));
    c.root = n;
    c.p = (WCHAR*)pStr;
    c.strLen = nLen;
    c.bTransU = bTransU;
    c.error = CXJSON_OK;
    c.stackNode = NULL;
    c.stackStatus = NULL;
    c.top = 0;
    c.cap = 0;
    c.namePos = -1;
    c.nameLen = 0;
    JvClear(n);

    INT i = 0;
    while (i <= c.strLen)
    {
        WCHAR w = (i < c.strLen) ? c.p[i] : 0;
        if (w == ' ' || w == 9 || w == 13 || w == 10 || w == 0) { i++; continue; }
        if (w == '{')
        {
            c.error = JvParseObject(&c);
            if (c.error != CXJSON_OK) break;
            i++;
            continue;
        }
        if (w == '[')
        {
            c.error = JvParseArray(&c);
            if (c.error != CXJSON_OK) break;
            i++;
            continue;
        }
        c.error = JvParseOther(&c, &i);
        if (c.error != CXJSON_OK) break;
        i++;
    }
    if (c.stackNode != NULL) JvFree(c.stackNode);
    if (c.stackStatus != NULL) JvFree(c.stackStatus);
    if (c.error != CXJSON_OK)
    {
        JvClear(n);
        if (pnErrPos != NULL) *pnErrPos = i;
        return c.error;
    }
    if (c.top != 0)
    {
        JvClear(n);
        if (pnErrPos != NULL) *pnErrPos = i;
        return CXJSON_OBJECTPARENT;
    }
    return c.error;
}

// ============================================================ 合并入口
BOOL JvCombine(CxJsonNode* n, const WCHAR* pStr, INT nLen, INT bTransU, INT bIsAdd)
{
    if (n == NULL) return FALSE;
    CxJsonNode* desc = JvNewObject();
    if (desc == NULL) return FALSE;
    INT errPos = 0;
    if (JvParse(desc, pStr, nLen, bTransU, &errPos) != CXJSON_OK)
    {
        JvRelease(desc);
        return FALSE;
    }
    JvRetain(desc);                                 // 被挂上树时这份引用留给容器
    BOOL ret = JvCombineProc(n, desc, bIsAdd);
    JvRelease(desc);
    return ret;
}

}  // namespace cx
