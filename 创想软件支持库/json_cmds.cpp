// Json2 类（zyJson2.51 同源）的易语言命令实现层。
// 每条命令的行为逐句照抄原模块 类 Json2 的源码：可空参数的默认值、是否懒初始化、
// 是否 设置是否为顶层(假)、错误码写到哪个 参考 参数，全部保持原样，以便历史工程直接替换。
// 表与转发由 gen_json2_table.mjs 生成（include/cx_json2_stubs.inc），本文件只写语义。
#include <cx_json.h>

namespace cx {

// ============================================================ 基础辅助

static CxJson2* J2Self(PMDATA_INF pArgInf)
{
    if (pArgInf == NULL || pArgInf[0].m_pCompoundData == NULL) return NULL;
    return (CxJson2*)(INT_PTR)*(PINT)pArgInf[0].m_pCompoundData;
}

// 原版 初始化()：已初始化就直接返回真（销毁后标记不复位，照抄）。
static CxJsonNode* J2Init(CxJson2* p)
{
    if (p == NULL) return NULL;
    if (p->inited) return p->node;
    CxJsonNode* n = JvNewObject();
    if (n == NULL) return NULL;
    p->node = n;
    p->inited = 1;
    return n;
}

static CxJsonNode* J2SelfNode(PMDATA_INF pArgInf)
{
    CxJson2* p = J2Self(pArgInf);
    return (p == NULL) ? NULL : p->node;
}

// 文本型（GBK）→ 宽文本；空参数返回 NULL。
static WCHAR* J2TextToWide(const MDATA_INF& arg, UINT uCP)
{
    if (UniIsNull(arg)) return NULL;
    LPCSTR p = arg.m_pText;
    if (p == NULL) return NULL;
    return JvAnsiToWide(p, (INT)lstrlenA(p), uCP);
}

// 字节集（UTF-16LE，惯例末尾带 {0,0}）→ 可当 C 串用的宽文本；空参数返回 NULL。
static WCHAR* J2BinToWide(const MDATA_INF& arg)
{
    if (UniIsNull(arg)) return NULL;
    INT nBytes = 0;
    LPBYTE p = UniArgBinPtr(arg, &nBytes);
    if (p == NULL || nBytes <= 0) return NULL;
    INT nChars = nBytes / 2;
    WCHAR* pW = (WCHAR*)JvMalloc((SIZE_T)(nChars + 1) * sizeof(WCHAR));
    if (pW == NULL) return NULL;
    memcpy(pW, p, (SIZE_T)nChars * sizeof(WCHAR));
    pW[nChars] = 0;                            // 缺末尾 0,0 时补齐（原版 W末尾增加NULL）
    return pW;
}

static const WCHAR* J2ArgWide(const MDATA_INF& arg, BOOL bIsBin, UINT uCP, WCHAR** ppTmp)
{
    *ppTmp = bIsBin ? J2BinToWide(arg) : J2TextToWide(arg, uCP);
    return *ppTmp;
}

static void J2FreeWide(WCHAR* p) { JvFree(p); }

// 返回值写入
static void J2RetBool(PMDATA_INF pRet, BOOL b)
{
    UniRetBool(pRet, b);      // 与库内其它命令同一套逻辑型返回口径
}

static void J2RetInt(PMDATA_INF pRet, INT n)
{
    pRet->m_dtDataType = SDT_INT;
    pRet->m_int = n;
}

static void J2RetInt64(PMDATA_INF pRet, __int64 l)
{
    pRet->m_dtDataType = SDT_INT64;
    pRet->m_int64 = l;
}

static void J2RetDouble(PMDATA_INF pRet, double d)
{
    pRet->m_dtDataType = SDT_DOUBLE;
    pRet->m_double = d;
}

// 文本型返回：GBK 字节（原版 zyW2A_Point 走 CP_ACP/936）。
static void J2RetTextA(PMDATA_INF pRet, const WCHAR* pW)
{
    pRet->m_dtDataType = SDT_TEXT;
    char* pA = (pW == NULL) ? NULL : JvWideToAnsi(pW, 936, NULL);
    pRet->m_pText = CloneTextData(pA == NULL ? "" : pA);
    if (pA != NULL) JvFree(pA);
}

static void J2RetTextUtf8(PMDATA_INF pRet, const WCHAR* pW)
{
    pRet->m_dtDataType = SDT_TEXT;
    if (pW == NULL) { pRet->m_pText = CloneTextData("", 0); return; }
    INT nChars = JvWideLen(pW);
    INT n = WideCharToMultiByte(CP_UTF8, 0, pW, nChars, NULL, 0, NULL, NULL);
    if (n <= 0) { pRet->m_pText = CloneTextData("", 0); return; }
    char* pA = (char*)JvMalloc((SIZE_T)n + 1);
    if (pA == NULL) { pRet->m_pText = CloneTextData("", 0); return; }
    WideCharToMultiByte(CP_UTF8, 0, pW, nChars, pA, n, NULL, NULL);
    pA[n] = '\0';
    pRet->m_pText = CloneTextData(pA, n);
    JvFree(pA);
}

// 字节集返回：UTF-16LE 且末尾带 {0,0}（原版 指针到字节集(p, lstrlenW×2＋2)）。
static void J2RetWideBin(PMDATA_INF pRet, const WCHAR* pW)
{
    INT nChars = (pW == NULL) ? 0 : JvWideLen(pW);
    UniRetWideBin(pRet, (pW == NULL) ? L"" : pW, nChars);
}

// 参考 整数型 出参写回（未传参时什么都不做，与原版一致）。
static void J2OutInt(const MDATA_INF& arg, INT n)
{
    if (UniIsNull(arg)) return;
    if (arg.m_pInt != NULL) *arg.m_pInt = n;
}

// Json2 对象参数 → 宿主结构 / 节点。
static CxJson2* J2ObjArg(const MDATA_INF& arg)
{
    if (UniIsNull(arg)) return NULL;
    return (CxJson2*)(INT_PTR)((arg.m_pCompoundData != NULL)
        ? *(PINT)arg.m_pCompoundData : 0);
}

static CxJsonNode* J2NodeOfArg(const MDATA_INF& arg)
{
    CxJson2* p = J2ObjArg(arg);
    return (p == NULL) ? NULL : p->node;
}

// 命令返回一个 Json2 对象：数据块由易语言内存池分配，内容是宿主结构指针。
static void J2RetObj(PMDATA_INF pRet, CxJsonNode* node, INT bTop, INT bOwned)
{
    CxJson2* p = (CxJson2*)JvMalloc(sizeof(CxJson2));
    if (p == NULL)
    {
        pRet->m_dtDataType = _SDT_NULL;
        return;
    }
    p->node = node;
    p->isTop = bTop;
    p->inited = (node != NULL) ? 1 : 0;
    if (node != NULL && !bOwned) JvRetain(node);
    pRet->m_dtDataType = MAKELONG(CX_TYPE_JSON2, 0);
    DWORD* pSlot = (DWORD*)NotifySys(NRS_MALLOC, sizeof(DWORD), 0);
    if (pSlot == NULL)
    {
        JvFree(p);
        pRet->m_dtDataType = _SDT_NULL;
        return;
    }
    *pSlot = (DWORD)(INT_PTR)p;
    pRet->m_pCompoundData = pSlot;
}

// ============================================================ 序列化 / 取文本 公共实现

static WCHAR* J2Serialize(CxJsonNode* jv, INT bFmt, INT bTrans)
{
    UniWBuf buf;
    JvToString(jv, &buf, bFmt, bTrans);
    if (buf.p == NULL || buf.nLen <= 0) return NULL;
    return JvWideDup(buf.p, buf.nLen);
}

static BOOL J2WideEq(const WCHAR* p, const WCHAR* q)
{
    if (p == NULL || q == NULL) return FALSE;
    return lstrcmpW(p, q) == 0;
}

// 取文本 的口径：先取文本内容/raw；为空则退化为整棵子树的序列化，
// 再把 ""、{}、[]、null 统一抹成空（原版 A/W 两版逻辑一致）。
static WCHAR* J2GetTextCommon(CxJsonNode* jv, INT bFmt, INT bTrans)
{
    static const WCHAR kQ[]  = { '"', '"', 0 };
    static const WCHAR kOb[] = { '{', '}', 0 };
    static const WCHAR kAr[] = { '[', ']', 0 };
    static const WCHAR kNu[] = { 'n', 'u', 'l', 'l', 0 };
    WCHAR* s = JvGetStringEx(jv);
    if (s == NULL || s[0] == 0)
    {
        if (s != NULL) JvFree(s);
        s = J2Serialize(jv, bFmt, bTrans);
        if (s != NULL && (J2WideEq(s, kQ) || J2WideEq(s, kOb) || J2WideEq(s, kAr)))
        {
            JvFree(s);
            s = NULL;
        }
    }
    if (s != NULL && J2WideEq(s, kNu))
    {
        JvFree(s);
        s = NULL;
    }
    return s;
}

// ============================================================ 解析入口公共实现
// bSource: 0=文本型 GBK，1=字节集 UTF-16，2=文本型 UTF-8
static void J2SetCodeCore(INT op, int bSource, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(op);
    CxJson2* self = J2Self(pArgInf);
    if (self == NULL) { J2RetBool(pRetData, FALSE); return; }

    WCHAR* pText = NULL;
    switch (bSource)
    {
    case 0: pText = J2TextToWide(pArgInf[1], 936); break;
    case 1: pText = J2BinToWide(pArgInf[1]); break;
    default: pText = J2TextToWide(pArgInf[1], CP_UTF8); break;
    }

    if (pText == NULL || pText[0] == 0)             // 空文本：换一个全新的空对象
    {
        if (pText != NULL) JvFree(pText);
        CxJsonNode* n = JvNewObject();
        if (n == NULL) { J2OutInt(pArgInf[3], CXJSON_MEMORY); J2RetBool(pRetData, FALSE); return; }
        if (self->node != NULL) { JvRelease(self->node); self->node = NULL; }
        self->node = n;
        J2RetBool(pRetData, TRUE);
        return;
    }

    INT nTransU = UniIsNull(pArgInf[2]) ? 1 : (UniArgBool(pArgInf[2], 0) ? 1 : 0);
    if (bSource == 2 && op == J2OP_SetCodeUtf8) nTransU = UniIsNull(pArgInf[2]) ? 0 : (UniArgBool(pArgInf[2], 0) ? 1 : 0);

    if (!self->isTop)                              // 子对象：就地清空 + 合并（是否添加＝真）
    {
        JvClear(self->node);
        BOOL r = JvCombine(self->node, pText, JvWideLen(pText), nTransU, 1);
        JvFree(pText);
        J2RetBool(pRetData, r);
        return;
    }

    CxJsonNode* n = JvNewObject();
    if (n == NULL)
    {
        JvFree(pText);
        J2OutInt(pArgInf[3], CXJSON_MEMORY);
        J2RetBool(pRetData, FALSE);
        return;
    }
    if (self->node != NULL) JvRelease(self->node);
    self->node = n;
    self->inited = 1;
    INT errPos = 0;
    INT err = JvParse(n, pText, JvWideLen(pText), nTransU, &errPos);
    JvFree(pText);
    J2OutInt(pArgInf[3], err);
    J2OutInt(pArgInf[4], errPos);
    J2RetBool(pRetData, err == CXJSON_OK);
}

static void J2ParseCore(int bSource, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJson2* self = J2Self(pArgInf);
    if (self == NULL) { J2RetBool(pRetData, FALSE); return; }
    if (self->node == NULL)
    {
        self->node = JvNewObject();               // 解析时按需建根，但不复位 inited（照抄）
        if (self->node == NULL) { J2RetBool(pRetData, FALSE); return; }
    }
    WCHAR* pText = (bSource == 0) ? J2TextToWide(pArgInf[1], 936) : J2BinToWide(pArgInf[1]);
    INT nTransU = UniArgBool(pArgInf[2], 0) ? 1 : 0;
    INT errPos = 0;
    INT err = (pText == NULL) ? CXJSON_INVALIDARG : JvParse(self->node, pText, JvWideLen(pText), nTransU, &errPos);
    if (pText != NULL) JvFree(pText);
    J2OutInt(pArgInf[3], err);
    J2OutInt(pArgInf[4], errPos);
    J2RetBool(pRetData, err == CXJSON_OK);
}

// 序列化到文本（0=W 字节集，1=A GBK 文本）
static void J2ToTextCore(int bWide, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJsonNode* node = J2SelfNode(pArgInf);
    WCHAR* pPath = bWide ? J2BinToWide(pArgInf[1]) : J2TextToWide(pArgInf[1], 936);
    INT bFmt = UniArgBool(pArgInf[2], 0) ? 1 : 0;
    INT bTrans = UniIsNull(pArgInf[3]) ? 1 : (UniArgBool(pArgInf[3], 0) ? 1 : 0);
    CxJsonNode* jv = JvSelect(node, pPath, 0);
    WCHAR* s = J2Serialize(jv, bFmt, bTrans);
    if (bWide) J2RetWideBin(pRetData, s);
    else J2RetTextA(pRetData, s);
    if (s != NULL) JvFree(s);
    if (pPath != NULL) JvFree(pPath);
}

// 只有 参数_路径 的只读访问：取类型 / 取键数量 / 取源数据 / 取逻辑 / 取数值 …
enum J2ReadOnlyKind { RO_TYP, RO_COUNT, RO_RAW, RO_TEXT, RO_BOOL, RO_DOUBLE, RO_LONG, RO_INT };
static void J2ReadOnlyCore(J2ReadOnlyKind kind, int bWide, int bUtf8Out, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJsonNode* node = J2SelfNode(pArgInf);
    WCHAR* pPath = bWide ? J2BinToWide(pArgInf[1]) : J2TextToWide(pArgInf[1], 936);
    CxJsonNode* jv = JvSelect(node, pPath, 0);
    if (pPath != NULL) JvFree(pPath);
    switch (kind)
    {
    case RO_TYP:    J2RetInt(pRetData, JvTypeOf(jv)); break;
    case RO_COUNT:  J2RetInt(pRetData, JvCount(jv)); break;
    case RO_BOOL:   J2RetBool(pRetData, JvGetBool(jv)); break;
    case RO_DOUBLE: J2RetDouble(pRetData, JvGetDouble(jv)); break;
    case RO_LONG:   J2RetInt64(pRetData, JvGetLong(jv)); break;
    case RO_INT:    J2RetInt(pRetData, JvGetInt(jv)); break;
    case RO_RAW:
    {
        const WCHAR* s = JvRaw(jv);
        WCHAR* c = (s != NULL) ? JvWideDup(s, -1) : NULL;
        if (bWide) J2RetWideBin(pRetData, c);
        else J2RetTextA(pRetData, c);
        if (c != NULL) JvFree(c);
        break;
    }
    case RO_TEXT:
    {
        INT bFmt = UniArgBool(pArgInf[2], 0) ? 1 : 0;
        INT bTrans = UniIsNull(pArgInf[3]) ? 1 : (UniArgBool(pArgInf[3], 0) ? 1 : 0);
        WCHAR* s = J2GetTextCommon(jv, bFmt, bTrans);
        if (bUtf8Out) J2RetTextUtf8(pRetData, s);
        else if (bWide) J2RetWideBin(pRetData, s);
        else J2RetTextA(pRetData, s);
        if (s != NULL) JvFree(s);
        break;
    }
    default: break;
    }
}

// 返回子对象的访问：0=K（创建），1=K2（不创建），2=I（创建），3=I2（不创建），4=取父，5=选择
enum J2ChildKind { CH_K, CH_K2, CH_I, CH_I2, CH_PARENT, CH_SELECT };
static void J2ChildCore(J2ChildKind kind, int bWide, int bUtf8, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJson2* self = J2Self(pArgInf);
    if (self == NULL) { J2RetObj(pRetData, NULL, 1, 1); return; }
    CxJsonNode* node = self->node;
    if (kind == CH_K || kind == CH_K2 || kind == CH_I || kind == CH_I2) node = J2Init(self);
    UINT uCP = bUtf8 ? CP_UTF8 : 936;

    INT iPath = (kind == CH_PARENT || kind == CH_SELECT) ? 1 : 2;
    WCHAR* pPath = bWide ? J2BinToWide(pArgInf[iPath]) : J2TextToWide(pArgInf[iPath], uCP);
    CxJsonNode* jv = NULL;
    if (kind == CH_PARENT) jv = JvSelect(node, pPath, 0);
    else jv = JvSelect(node, pPath, (kind == CH_K || kind == CH_I || kind == CH_SELECT) ? 1 : 0);
    if (pPath != NULL) JvFree(pPath);

    CxJsonNode* target = NULL;
    if (kind == CH_K || kind == CH_K2)
    {
        WCHAR* pKey = bWide ? J2BinToWide(pArgInf[1]) : J2TextToWide(pArgInf[1], uCP);
        target = (kind == CH_K) ? JvChildOrCreate(jv, pKey) : JvChildReadOnly(jv, pKey);
        if (pKey != NULL) JvFree(pKey);
    }
    else if (kind == CH_I || kind == CH_I2)
    {
        INT idx = UniArgInt(pArgInf[1], 0);
        target = (kind == CH_I) ? JvAtOrCreate(jv, idx) : JvAtReadOnly(jv, idx);
    }
    else if (kind == CH_PARENT) target = JvGetParent(jv);
    else target = jv;

    // K2/I2/取父…里只有 K/IW/选择/取父 调了 设置是否为顶层(假)，其余保持新建对象的「真」。
    INT bTop = (kind == CH_K2 || kind == CH_I2) ? 1 : 0;
    J2RetObj(pRetData, target, bTop, 0);
}

// ============================================================ 变更类命令
// 0=置文本 1=置空 2=置逻辑 3=置双精度 4=置长整数 5=置整数 6=置对象 7=置数组
enum J2SetKind { ST_TEXT, ST_NULL, ST_BOOL, ST_DOUBLE, ST_LONG, ST_INT, ST_OBJECT, ST_ARRAY };
static void J2SetCore(J2SetKind kind, int bWide, int bUtf8In, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJson2* self = J2Self(pArgInf);
    if (self == NULL) { J2RetBool(pRetData, FALSE); return; }
    CxJsonNode* node = J2Init(self);
    // 置空/置对象/置数组：路径即第 1 参；置文本/置逻辑/置双精度/置数值/置长整数/置整数：值在前、路径第 2 参
    INT iPath = (kind == ST_NULL || kind == ST_OBJECT || kind == ST_ARRAY) ? 1 : 2;
    // 路径：W 版是 UTF-16 字节集，其余是 GBK 文本；置文本UTF8 例外（值 UTF-8、路径字节集）。
    WCHAR* pPath = (kind == ST_TEXT && bUtf8In) ? J2BinToWide(pArgInf[iPath])
               : (bWide ? J2BinToWide(pArgInf[iPath]) : J2TextToWide(pArgInf[iPath], 936));
    CxJsonNode* jv = JvSelect(node, pPath, 1);
    if (pPath != NULL) JvFree(pPath);
    BOOL r = FALSE;
    switch (kind)
    {
    case ST_TEXT:
    {
        // 置文本＝GBK 文本值；置文本W＝UTF-16 字节集值；置文本UTF8＝UTF-8 文本值
        WCHAR* pVal = bUtf8In ? J2TextToWide(pArgInf[1], CP_UTF8)
                  : (bWide ? J2BinToWide(pArgInf[1]) : J2TextToWide(pArgInf[1], 936));
        r = JvSetString(jv, pVal, -1);
        if (pVal != NULL) JvFree(pVal);
        break;
    }
    case ST_NULL:    r = JvSetNull(jv); break;
    case ST_BOOL:    r = JvSetBool(jv, UniArgBool(pArgInf[1], 0)); break;
    case ST_DOUBLE:  r = JvSetDouble(jv, pArgInf[1].m_double, NULL); break;
    case ST_LONG:    r = JvSetLong(jv, pArgInf[1].m_int64, NULL); break;
    case ST_INT:     r = JvSetInt(jv, UniArgInt(pArgInf[1], 0)); break;
    case ST_OBJECT:  r = JvSetObject(jv); break;
    case ST_ARRAY:   r = JvSetArray(jv); break;
    default: break;
    }
    J2RetBool(pRetData, r);
}

// 置属性(0)/插入属性(1)/置成员(2)/添加成员(3)/插入成员(4)：值来源都是另一个 Json2（深拷贝）
enum J2PutKind { PU_ATTR, PU_INSERT_ATTR, PU_SET_AT, PU_APPEND, PU_INSERT };
static void J2PutCore(J2PutKind kind, int bWide, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJsonNode* node = J2SelfNode(pArgInf);
    INT iPath = 1, iKey = 2, iVal = 3, iIdx = -1;
    switch (kind)
    {
    case PU_ATTR:        iPath = 1; iKey = 2; iVal = 3; break;   // 路径, 属性名, 值
    case PU_INSERT_ATTR: iPath = 1; iIdx = 2; iKey = 3; iVal = 4; break;
    case PU_SET_AT:      iIdx = 1; iPath = 2; iVal = 3; break;   // 索引, 路径, 值
    case PU_APPEND:      iPath = 1; iVal = 2; break;
    case PU_INSERT:      iPath = 1; iIdx = 2; iVal = 3; break;
    default: break;
    }
    WCHAR* pPath = bWide ? J2BinToWide(pArgInf[iPath]) : J2TextToWide(pArgInf[iPath], 936);
    CxJsonNode* jv = JvSelect(node, pPath, 1);
    if (pPath != NULL) JvFree(pPath);
    if (jv == NULL) { J2RetBool(pRetData, FALSE); return; }
    CxJsonNode* src = J2NodeOfArg(pArgInf[iVal]);
    if (src == NULL) { J2RetBool(pRetData, FALSE); return; }
    CxJsonNode* copy = JvClone(src);
    if (copy == NULL) { J2RetBool(pRetData, FALSE); return; }
    BOOL r = FALSE;
    if (kind == PU_ATTR || kind == PU_INSERT_ATTR)
    {
        WCHAR* pKey = bWide ? J2BinToWide(pArgInf[iKey]) : J2TextToWide(pArgInf[iKey], 936);
        if (pKey != NULL)
        {
            r = (kind == PU_ATTR) ? JvChildSet(jv, pKey, copy)
                                  : JvChildInsert(jv, UniArgInt(pArgInf[iIdx], 0), pKey, copy);
        }
        if (pKey != NULL) JvFree(pKey);
    }
    else if (kind == PU_SET_AT) r = JvAtSet(jv, UniArgInt(pArgInf[iIdx], 0), copy);
    else if (kind == PU_APPEND) r = JvAppend(jv, copy);
    else r = JvInsertAt(jv, UniArgInt(pArgInf[iIdx], 0), copy);
    if (!r) JvRelease(copy);
    J2RetBool(pRetData, r);
}

static void J2RemoveCore(int bAttr, int bWide, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJsonNode* node = J2SelfNode(pArgInf);
    // 移除属性(名, 路径) / 移除成员(索引, 路径)
    WCHAR* pPath = bWide ? J2BinToWide(pArgInf[2]) : J2TextToWide(pArgInf[2], 936);
    CxJsonNode* jv = JvSelect(node, pPath, 1);
    if (pPath != NULL) JvFree(pPath);
    BOOL r = FALSE;
    if (bAttr)
    {
        WCHAR* pKey = bWide ? J2BinToWide(pArgInf[1]) : J2TextToWide(pArgInf[1], 936);
        if (pKey != NULL) r = JvChildRemove(jv, pKey);
        if (pKey != NULL) JvFree(pKey);
    }
    else r = JvRemoveAt(jv, UniArgInt(pArgInf[1], 0));
    J2RetBool(pRetData, r);
}

static void J2ClearCore(int bWide, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJsonNode* node = J2SelfNode(pArgInf);
    WCHAR* pPath = bWide ? J2BinToWide(pArgInf[1]) : J2TextToWide(pArgInf[1], 936);
    CxJsonNode* jv = JvSelect(node, pPath, 1);
    if (pPath != NULL) JvFree(pPath);
    pRetData->m_dtDataType = _SDT_NULL;
    JvClear(jv);
}

// 取所有键：空数组＝查数量，非空＝填充（顺序＝插入序）
static void J2GetAllKeysCore(int bWide, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJsonNode* node = J2SelfNode(pArgInf);
    if (UniIsNull(pArgInf[1]))
    {
        WCHAR* pPath = bWide ? J2BinToWide(pArgInf[2]) : J2TextToWide(pArgInf[2], 936);
        CxJsonNode* jv = JvSelect(node, pPath, 0);
        if (pPath != NULL) JvFree(pPath);
        J2RetInt(pRetData, JvCount(jv));
        return;
    }
    WCHAR* pPath = bWide ? J2BinToWide(pArgInf[2]) : J2TextToWide(pArgInf[2], 936);
    CxJsonNode* jv = JvSelect(node, pPath, 0);
    if (pPath != NULL) JvFree(pPath);
    if (jv == NULL || jv->type != CXJV_OBJECT || jv->v.obj == NULL) { J2RetInt(pRetData, 0); return; }
    CxJsonObj* o = jv->v.obj;
    UniAryList list;
    for (INT i = 0; i < o->count; i++)
    {
        const WCHAR* k = (o->order[i] != NULL) ? o->order[i]->key : NULL;
        if (bWide)
        {
            INT nChars = k == NULL ? 0 : JvWideLen(k);
            list.Add(CloneBinData((LPCSTR)k, (nChars + 1) * 2));
        }
        else
        {
            char* pA = (k == NULL) ? NULL : JvWideToAnsi(k, 936, NULL);
            list.Add(CloneTextData(pA == NULL ? "" : pA));
            if (pA != NULL) JvFree(pA);
        }
    }
    UniAryWrite(pArgInf[1], &list);
    J2RetInt(pRetData, o->count);
}

// 取所有值：元素是与原树共享节点的 Json2 对象
static void J2GetAllValuesCore(int bWide, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJsonNode* node = J2SelfNode(pArgInf);
    INT iPath = bWide ? 1 : 1, iAry = 2;
    WCHAR* pPath = bWide ? J2BinToWide(pArgInf[iPath]) : J2TextToWide(pArgInf[iPath], 936);
    CxJsonNode* jv = JvSelect(node, pPath, 0);
    if (pPath != NULL) JvFree(pPath);
    INT n = JvCount(jv);
    if (jv == NULL || (jv->type != CXJV_OBJECT && jv->type != CXJV_ARRAY)) { J2RetInt(pRetData, 0); return; }
    if (n <= 0) { J2RetInt(pRetData, 0); return; }
    UniAryList list;
    for (INT i = 0; i < n; i++)
    {
        CxJsonNode* c = JvOrderBy(jv, i);
        CxJson2* p = (CxJson2*)JvMalloc(sizeof(CxJson2));
        DWORD* pSlot = (DWORD*)NotifySys(NRS_MALLOC, sizeof(DWORD), 0);
        if (p == NULL || pSlot == NULL)
        {
            if (p != NULL) JvFree(p);
            continue;
        }
        p->node = c;
        p->isTop = 1;                              // 原版未对元素调 设置是否为顶层(假)
        p->inited = (c != NULL) ? 1 : 0;
        JvRetain(c);
        *pSlot = (DWORD)(INT_PTR)p;
        list.Add(pSlot);
    }
    UniAryWrite(pArgInf[iAry], &list);
    J2RetInt(pRetData, n);
}

static void J2MergeCore(int bWide, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJson2* self = J2Self(pArgInf);
    if (self == NULL) { J2RetBool(pRetData, FALSE); return; }
    CxJsonNode* node = J2Init(self);
    INT nTransU = UniIsNull(pArgInf[3]) ? 1 : (UniArgBool(pArgInf[3], 0) ? 1 : 0);
    INT bAdd = UniArgBool(pArgInf[4], 0) ? 1 : 0;
    WCHAR* pPath = bWide ? J2BinToWide(pArgInf[2]) : J2TextToWide(pArgInf[2], 936);
    CxJsonNode* jv = JvSelect(node, pPath, 1);
    if (pPath != NULL) JvFree(pPath);
    if (jv == NULL) { J2RetBool(pRetData, FALSE); return; }
    WCHAR* pText = bWide ? J2BinToWide(pArgInf[1]) : J2TextToWide(pArgInf[1], 936);
    if (pText == NULL) { J2RetBool(pRetData, FALSE); return; }
    BOOL r = JvCombine(jv, pText, JvWideLen(pText), nTransU, bAdd);
    if (!r)
    {
        // 兜底：另建一个对象解析同一串文本，再按对象浅覆盖合并（原版行为）
        CxJsonNode* tmp = JvNewObject();
        INT errPos = 0;
        if (tmp != NULL && JvParse(tmp, pText, JvWideLen(pText), nTransU, &errPos) == CXJSON_OK)
            r = JvCombineObject(jv, tmp);
        if (tmp != NULL) JvRelease(tmp);
    }
    JvFree(pText);
    J2RetBool(pRetData, r);
}

static void J2MergeObjCore(int bPlus, int bArray, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJsonNode* node = J2SelfNode(pArgInf);
    if (!bArray) { CxJson2* p = J2Self(pArgInf); if (p != NULL) node = J2Init(p); }   // 合并数组原版不懒初始化
    INT iSrc = 1, iPath = 2;
    CxJsonNode* src = J2NodeOfArg(pArgInf[iSrc]);
    WCHAR* pPath = J2TextToWide(pArgInf[iPath], 936);
    CxJsonNode* jv = JvSelect(node, pPath, 1);
    if (pPath != NULL) JvFree(pPath);
    if (jv == NULL || src == NULL) { J2RetBool(pRetData, FALSE); return; }
    BOOL r;
    if (bArray) r = JvCombineArray(jv, src);
    else
    {
        r = JvCombineObject(jv, src);
        if (bPlus && !r) r = JvCombineArray(jv, src);
    }
    J2RetBool(pRetData, r);
}

// 用户比较回调（排序）：EPL 子程序指针，三个整数参数（节点1, 节点2, 回调参数）
typedef INT (*PFN_EPL_SUB3)(INT, INT, INT);
static PFN_JV_CMP g_j2CmpProc = NULL;
static PFN_EPL_SUB3 g_j2CmpFn = NULL;
static INT g_j2CmpParam = 0;

static INT JvUserCmp(CxJsonNode* a, CxJsonNode* b, INT)
{
    if (g_j2CmpFn == NULL) return 0;
    return g_j2CmpFn((INT)(INT_PTR)a, (INT)(INT_PTR)b, g_j2CmpParam);
}

static void J2SortCore(int bWide, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJsonNode* node = J2SelfNode(pArgInf);
    WCHAR* pPath = bWide ? J2BinToWide(pArgInf[1]) : J2TextToWide(pArgInf[1], 936);
    CxJsonNode* jv = JvSelect(node, pPath, 0);
    if (pPath != NULL) JvFree(pPath);
    pRetData->m_dtDataType = _SDT_NULL;
    if (jv == NULL) return;
    g_j2CmpFn = (PFN_EPL_SUB3)(INT_PTR)UniArgInt(pArgInf[2], 0);
    g_j2CmpParam = UniArgInt(pArgInf[3], 0);
    JvSort(jv, JvUserCmp, 0);                       // 参数_算法 原版写死快速排序
    g_j2CmpFn = NULL;
}

static void J2SortObjCore(int bWide, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJsonNode* node = J2SelfNode(pArgInf);
    WCHAR* pPath = bWide ? J2BinToWide(pArgInf[1]) : J2TextToWide(pArgInf[1], 936);
    CxJsonNode* jv = JvSelect(node, pPath, 0);
    if (pPath != NULL) JvFree(pPath);
    INT bDesc = UniArgBool(pArgInf[2], 0) ? 1 : 0;
    J2RetBool(pRetData, (jv != NULL) ? JvSortObject(jv, bDesc ? 0 : 1) : FALSE);
}

// 复制：深拷贝一份独立成根的子树
static void J2CopyCore(int bWide, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJsonNode* node = J2SelfNode(pArgInf);
    WCHAR* pPath = bWide ? J2BinToWide(pArgInf[1]) : J2TextToWide(pArgInf[1], 936);
    CxJsonNode* jv = JvSelect(node, pPath, 1);
    if (pPath != NULL) JvFree(pPath);
    CxJsonNode* c = (jv != NULL) ? JvClone(jv) : NULL;
    J2RetObj(pRetData, c, 0, 1);
}

// 置X数组：置数组() 后逐个 I(i).置X(元素)
enum J2AryKind { ARY_TEXT, ARY_TEXT_W, ARY_TEXT_UTF8, ARY_NUMBER, ARY_INT, ARY_BOOL };
static void J2FillAryCore(J2AryKind kind, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount);
    CxJson2* self = J2Self(pArgInf);
    pRetData->m_dtDataType = _SDT_NULL;
    if (self == NULL) return;
    CxJsonNode* node = J2Init(self);
    if (node == NULL || !JvSetArray(node)) return;
    INT n = 0;
    UniAryRead(pArgInf[1], 0, &n);
    LPBYTE pItems = UniAryRead(pArgInf[1], 0, NULL);
    if (pItems == NULL || n <= 0) return;
    for (INT i = 0; i < n; i++)
    {
        CxJsonNode* item = JvAtOrCreate(node, i);
        if (item == NULL) continue;
        MDATA_INF el;
        memset(&el, 0, sizeof(el));
        switch (kind)
        {
        case ARY_TEXT:
            el.m_dtDataType = SDT_TEXT;
            el.m_pText = (char*)((LPCSTR*)pItems)[i];
            { WCHAR* pV = J2TextToWide(el, 936); JvSetString(item, pV, -1); if (pV) JvFree(pV); }
            break;
        case ARY_TEXT_W:
            el.m_dtDataType = SDT_BIN;
            el.m_pBin = ((LPBYTE*)pItems)[i];
            { WCHAR* pV = J2BinToWide(el); JvSetString(item, pV, -1); if (pV) JvFree(pV); }
            break;
        case ARY_TEXT_UTF8:
            el.m_dtDataType = SDT_TEXT;
            el.m_pText = (char*)((LPCSTR*)pItems)[i];
            { WCHAR* pV = J2TextToWide(el, CP_UTF8); JvSetString(item, pV, -1); if (pV) JvFree(pV); }
            break;
        case ARY_NUMBER:  JvSetDouble(item, ((double*)pItems)[i], NULL); break;
        case ARY_INT:     JvSetInt(item, ((INT*)pItems)[i]); break;
        case ARY_BOOL:    JvSetBool(item, ((const DTBOOL*)pItems)[i] ? 1 : 0); break;   // 逻辑型数组元素是 2 字节 DTBOOL
        default: break;
        }
    }
}

// ============================================================ 命令实现

static void J2Invoke(INT op, PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    if (op == 0)                                    // 构造函数
    {
        CxJson2* p = (CxJson2*)JvMalloc(sizeof(CxJson2));
        if (pArgInf != NULL && pArgInf[0].m_pCompoundData != NULL)
            *(PINT)pArgInf[0].m_pCompoundData = (p == NULL) ? 0 : (INT)(INT_PTR)p;
        if (p != NULL) { p->node = NULL; p->isTop = 1; p->inited = 0; }
        pRetData->m_dtDataType = _SDT_NULL;
        return;
    }
    if (op == -1)                                   // 复制函数
    {
        CxJson2* pSrc = J2ObjArg(pArgInf[1]);
        CxJson2* p = (CxJson2*)JvMalloc(sizeof(CxJson2));
        if (pArgInf[0].m_pCompoundData != NULL)
            *(PINT)pArgInf[0].m_pCompoundData = (p == NULL) ? 0 : (INT)(INT_PTR)p;
        if (p == NULL) { pRetData->m_dtDataType = _SDT_NULL; return; }
        p->isTop = (pSrc != NULL) ? pSrc->isTop : 1;
        p->inited = (pSrc != NULL) ? pSrc->inited : 0;
        p->node = (pSrc != NULL) ? pSrc->node : NULL;
        JvRetain(p->node);
        pRetData->m_dtDataType = _SDT_NULL;
        return;
    }
    if (op == -2)                                   // 析构函数
    {
        CxJson2* p = J2Self(pArgInf);
        if (p != NULL)
        {
            JvRelease(p->node);
            JvFree(p);
            if (pArgInf[0].m_pCompoundData != NULL) *(PINT)pArgInf[0].m_pCompoundData = 0;
        }
        pRetData->m_dtDataType = _SDT_NULL;
        return;
    }

    switch (op)
    {
    case J2OP_SetCode:            J2SetCodeCore(op, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetCodeUtf8:        J2SetCodeCore(op, 2, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetCodeW:           J2SetCodeCore(op, 1, pRetData, nArgCount, pArgInf); break;
    case J2OP_Parse:              J2ParseCore(0, pRetData, nArgCount, pArgInf); break;
    case J2OP_ParseW:             J2ParseCore(1, pRetData, nArgCount, pArgInf); break;
    case J2OP_ToText:             J2ToTextCore(0, pRetData, nArgCount, pArgInf); break;
    case J2OP_ToTextW:            J2ToTextCore(1, pRetData, nArgCount, pArgInf); break;

    case J2OP_Release:
    {
        CxJson2* p = J2Self(pArgInf);
        if (p != NULL) { JvRelease(p->node); p->node = NULL; }   // inited 不复位（照抄原版）
        pRetData->m_dtDataType = _SDT_NULL;
        break;
    }
    case J2OP_SetTopFlag:
    {
        CxJson2* p = J2Self(pArgInf);
        if (p != NULL) p->isTop = UniArgBool(pArgInf[1], 0) ? 1 : 0;
        pRetData->m_dtDataType = _SDT_NULL;
        break;
    }
    case J2OP_GetPtr:   J2RetInt(pRetData, (INT)(INT_PTR)J2SelfNode(pArgInf)); break;
    case J2OP_SetPtr:
    {
        CxJson2* p = J2Self(pArgInf);
        INT nPtr = UniArgInt(pArgInf[1], 0);
        if (p != NULL && nPtr != 0)
        {
            CxJsonNode* n = (CxJsonNode*)(INT_PTR)nPtr;
            JvRetain(n);
            JvRelease(p->node);
            p->node = n;
            p->inited = 1;
        }
        pRetData->m_dtDataType = _SDT_NULL;
        break;
    }
    case J2OP_Detach:   JvDetach(J2SelfNode(pArgInf)); pRetData->m_dtDataType = _SDT_NULL; break;

    case J2OP_CreateObject:  J2RetObj(pRetData, JvNewObject(), 0, 1); break;
    case J2OP_CreateArray:   J2RetObj(pRetData, JvNewArray(), 0, 1); break;
    case J2OP_CreateNull:    J2RetObj(pRetData, JvNewNull(), 0, 1); break;
    case J2OP_CreateBool:    J2RetObj(pRetData, JvNewBool(UniArgBool(pArgInf[1], 0)), 0, 1); break;
    case J2OP_CreateText:
    {
        WCHAR* pV = J2TextToWide(pArgInf[1], 936);
        J2RetObj(pRetData, JvNewString(pV, -1), 0, 1);
        if (pV) JvFree(pV);
        break;
    }
    case J2OP_CreateTextW:
    {
        WCHAR* pV = J2BinToWide(pArgInf[1]);
        J2RetObj(pRetData, JvNewString(pV, -1), 0, 1);
        if (pV) JvFree(pV);
        break;
    }
    case J2OP_CreateDouble:  J2RetObj(pRetData, JvNewDouble(pArgInf[1].m_double, NULL), 0, 1); break;
    case J2OP_CreateLong:    J2RetObj(pRetData, JvNewLong(pArgInf[1].m_int64, NULL), 0, 1); break;

    case J2OP_GetTyp:        J2ReadOnlyCore(RO_TYP, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetTypW:       J2ReadOnlyCore(RO_TYP, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetKeyCount:   J2ReadOnlyCore(RO_COUNT, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetKeyCountW:  J2ReadOnlyCore(RO_COUNT, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetRaw:        J2ReadOnlyCore(RO_RAW, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetRawW:       J2ReadOnlyCore(RO_RAW, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetText:       J2ReadOnlyCore(RO_TEXT, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetTextW:      J2ReadOnlyCore(RO_TEXT, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetTextUtf8:   J2ReadOnlyCore(RO_TEXT, 1, 1, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetBoolA:      J2ReadOnlyCore(RO_BOOL, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetBoolW:      J2ReadOnlyCore(RO_BOOL, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetDoubleA:    J2ReadOnlyCore(RO_DOUBLE, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetDoubleW:    J2ReadOnlyCore(RO_DOUBLE, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetNumberA:    J2ReadOnlyCore(RO_DOUBLE, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetNumberW:    J2ReadOnlyCore(RO_DOUBLE, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetLongA:      J2ReadOnlyCore(RO_LONG, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetLongW:      J2ReadOnlyCore(RO_LONG, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetIntA:       J2ReadOnlyCore(RO_INT, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetIntW:       J2ReadOnlyCore(RO_INT, 1, 0, pRetData, nArgCount, pArgInf); break;

    case J2OP_GetParent:     J2ChildCore(CH_PARENT, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetParentW:    J2ChildCore(CH_PARENT, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_K:               J2ChildCore(CH_K, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_KW:              J2ChildCore(CH_K, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_KUtf8:           J2ChildCore(CH_K, 0, 1, pRetData, nArgCount, pArgInf); break;
    case J2OP_K2:              J2ChildCore(CH_K2, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_K2W:             J2ChildCore(CH_K2, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_I:               J2ChildCore(CH_I, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_IW:              J2ChildCore(CH_I, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_I2:              J2ChildCore(CH_I2, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_I2W:             J2ChildCore(CH_I2, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_Select:
    {
        CxJsonNode* node = J2SelfNode(pArgInf);
        WCHAR* pPath = J2TextToWide(pArgInf[1], 936);
        J2RetObj(pRetData, JvSelect(node, pPath, 1), 0, 0);
        if (pPath) JvFree(pPath);
        break;
    }
    case J2OP_SelectW:
    {
        CxJsonNode* node = J2SelfNode(pArgInf);
        WCHAR* pPath = J2BinToWide(pArgInf[1]);
        J2RetObj(pRetData, JvSelect(node, pPath, 1), 0, 0);
        if (pPath) JvFree(pPath);
        break;
    }
    case J2OP_Copy:          J2CopyCore(0, pRetData, nArgCount, pArgInf); break;
    case J2OP_CopyW:         J2CopyCore(1, pRetData, nArgCount, pArgInf); break;

    case J2OP_GetAttrName:
    case J2OP_GetAttrNameW:
    {
        int bWide = (op == J2OP_GetAttrNameW);
        CxJsonNode* node = J2SelfNode(pArgInf);
        WCHAR* pPath = bWide ? J2BinToWide(pArgInf[1]) : J2TextToWide(pArgInf[1], 936);
        CxJsonNode* jv = JvSelect(node, pPath, 0);
        if (pPath) JvFree(pPath);
        const WCHAR* k = JvSelfKey(jv);
        WCHAR* c = (k != NULL) ? JvWideDup(k, -1) : NULL;
        if (bWide) J2RetWideBin(pRetData, c);
        else J2RetTextA(pRetData, c);
        if (c) JvFree(c);
        break;
    }

    case J2OP_Exists:
    {
        WCHAR* pPath = J2TextToWide(pArgInf[1], 936);
        J2RetBool(pRetData, JvQuery(J2SelfNode(pArgInf), pPath));
        if (pPath) JvFree(pPath);
        break;
    }
    case J2OP_ExistsW:
    {
        WCHAR* pPath = J2BinToWide(pArgInf[1]);
        J2RetBool(pRetData, JvQuery(J2SelfNode(pArgInf), pPath));
        if (pPath) JvFree(pPath);
        break;
    }

    case J2OP_Clear:         J2ClearCore(0, pRetData, nArgCount, pArgInf); break;
    case J2OP_ClearW:        J2ClearCore(1, pRetData, nArgCount, pArgInf); break;
    case J2OP_RemoveAttr:    J2RemoveCore(1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_RemoveAttrW:   J2RemoveCore(1, 1, pRetData, nArgCount, pArgInf); break;
    case J2OP_RemoveItem:    J2RemoveCore(0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_RemoveItemW:   J2RemoveCore(0, 1, pRetData, nArgCount, pArgInf); break;

    case J2OP_SetAttr:       J2PutCore(PU_ATTR, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetAttrW:      J2PutCore(PU_ATTR, 1, pRetData, nArgCount, pArgInf); break;
    case J2OP_InsertAttr:    J2PutCore(PU_INSERT_ATTR, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_InsertAttrW:   J2PutCore(PU_INSERT_ATTR, 1, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetItem:       J2PutCore(PU_SET_AT, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetItemW:      J2PutCore(PU_SET_AT, 1, pRetData, nArgCount, pArgInf); break;
    case J2OP_AppendItem:    J2PutCore(PU_APPEND, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_AppendItemW:   J2PutCore(PU_APPEND, 1, pRetData, nArgCount, pArgInf); break;
    case J2OP_InsertItem:    J2PutCore(PU_INSERT, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_InsertItemW:   J2PutCore(PU_INSERT, 1, pRetData, nArgCount, pArgInf); break;

    case J2OP_SetText:       J2SetCore(ST_TEXT, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetTextW:      J2SetCore(ST_TEXT, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetTextUtf8:   J2SetCore(ST_TEXT, 1, 1, pRetData, nArgCount, pArgInf); break;  // 值 UTF-8 文本，路径字节集
    case J2OP_SetNull:       J2SetCore(ST_NULL, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetNullW:      J2SetCore(ST_NULL, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetBool:       J2SetCore(ST_BOOL, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetBoolW:      J2SetCore(ST_BOOL, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetDouble:     J2SetCore(ST_DOUBLE, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetDoubleW:    J2SetCore(ST_DOUBLE, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetNumber:     J2SetCore(ST_DOUBLE, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetNumberW:    J2SetCore(ST_DOUBLE, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetLong:       J2SetCore(ST_LONG, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetLongW:      J2SetCore(ST_LONG, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetInt:        J2SetCore(ST_INT, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetIntW:       J2SetCore(ST_INT, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetObject:     J2SetCore(ST_OBJECT, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetObjectW:    J2SetCore(ST_OBJECT, 1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetArray:      J2SetCore(ST_ARRAY, 0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetArrayW:     J2SetCore(ST_ARRAY, 1, 0, pRetData, nArgCount, pArgInf); break;

    case J2OP_GetAllKeys:    J2GetAllKeysCore(0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetAllKeysW:   J2GetAllKeysCore(1, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetAllValues:  J2GetAllValuesCore(0, pRetData, nArgCount, pArgInf); break;
    case J2OP_GetAllValuesW: J2GetAllValuesCore(1, pRetData, nArgCount, pArgInf); break;

    case J2OP_Merge:         J2MergeCore(0, pRetData, nArgCount, pArgInf); break;
    case J2OP_MergeW:        J2MergeCore(1, pRetData, nArgCount, pArgInf); break;
    case J2OP_MergeObj:      J2MergeObjCore(0, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_MergeObjPlus:  J2MergeObjCore(1, 0, pRetData, nArgCount, pArgInf); break;
    case J2OP_MergeArr:      J2MergeObjCore(0, 1, pRetData, nArgCount, pArgInf); break;

    case J2OP_Sort:          J2SortCore(0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SortW:         J2SortCore(1, pRetData, nArgCount, pArgInf); break;
    case J2OP_SortObj:       J2SortObjCore(0, pRetData, nArgCount, pArgInf); break;
    case J2OP_SortObjW:      J2SortObjCore(1, pRetData, nArgCount, pArgInf); break;

    case J2OP_SetTextAry:      J2FillAryCore(ARY_TEXT, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetTextAryW:     J2FillAryCore(ARY_TEXT_W, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetTextAryUtf8:  J2FillAryCore(ARY_TEXT_UTF8, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetNumberAryUtf8: J2FillAryCore(ARY_NUMBER, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetIntAryUtf8:   J2FillAryCore(ARY_INT, pRetData, nArgCount, pArgInf); break;
    case J2OP_SetBoolAryUtf8:  J2FillAryCore(ARY_BOOL, pRetData, nArgCount, pArgInf); break;

    default:
        ReportError("创想软件支持库：未知的 Json2 成员命令");
        pRetData->m_dtDataType = _SDT_NULL;
        break;
    }
}

#include <cx_json2_stubs.inc>

}  // namespace cx
