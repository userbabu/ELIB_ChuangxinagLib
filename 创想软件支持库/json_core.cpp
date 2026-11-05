// zyJson2.51 引擎：值模型 / 容器 / 强转 / 深拷贝 / 合并 / 排序。
// 解析状态机与序列化在 json_parse.cpp（共用这里的数据结构）。
#include <cx_json.h>

namespace cx {

// ============================================================ 进程堆分配
// 引擎内部一律用进程堆，绝不混用 operator new / LocalAlloc：
// 静态编译时易语言自带 new 来自 nafxcw（VC6 堆），跨堆释放会让 EXE 一启动就崩。

void* JvMalloc(SIZE_T nBytes)
{
    if (nBytes == 0) nBytes = 1;
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nBytes);
}

void JvFree(void* p)
{
    if (p != NULL) HeapFree(GetProcessHeap(), 0, p);
}

// ============================================================ 宽文本基础

INT JvWideLen(const WCHAR* pW)
{
    if (pW == NULL) return 0;
    return (INT)lstrlenW(pW);
}

INT JvWideCmp(const WCHAR* a, const WCHAR* b)
{
    if (a == NULL) a = L"";
    if (b == NULL) b = L"";
    return lstrcmpW(a, b);
}

WCHAR* JvWideDup(const WCHAR* pSrc, INT nChars)
{
    if (nChars < 0) nChars = (pSrc == NULL) ? 0 : JvWideLen(pSrc);
    WCHAR* p = (WCHAR*)JvMalloc((SIZE_T)(nChars + 1) * sizeof(WCHAR));
    if (p == NULL) return NULL;
    if (nChars > 0 && pSrc != NULL) memcpy(p, pSrc, (SIZE_T)nChars * sizeof(WCHAR));
    p[nChars] = 0;
    return p;
}

// ANSI → UTF-16：原版 zyA2W 把结尾 NUL 一起转换，结果总是「原文 + {0,0}」。
WCHAR* JvAnsiToWide(LPCSTR pA, INT nBytes, UINT uCP)
{
    if (pA == NULL || nBytes < 0) return NULL;
    INT nWide = MultiByteToWideChar(uCP, 0, pA, nBytes + 1, NULL, 0);
    if (nWide <= 0)
    {
        nWide = MultiByteToWideChar(uCP, 0, pA, nBytes, NULL, 0);
        if (nWide <= 0) return NULL;
        WCHAR* p = (WCHAR*)JvMalloc((SIZE_T)(nWide + 1) * sizeof(WCHAR));
        if (p == NULL) return NULL;
        MultiByteToWideChar(uCP, 0, pA, nBytes, p, nWide);
        p[nWide] = 0;
        return p;
    }
    WCHAR* p = (WCHAR*)JvMalloc((SIZE_T)nWide * sizeof(WCHAR));
    if (p == NULL) return NULL;
    MultiByteToWideChar(uCP, 0, pA, nBytes + 1, p, nWide);
    p[nWide - 1] = 0;
    return p;
}

char* JvWideToAnsi(const WCHAR* pW, UINT uCP, INT* pnBytes)
{
    if (pnBytes != NULL) *pnBytes = 0;
    if (pW == NULL) return NULL;
    INT nChars = JvWideLen(pW);
    INT n = WideCharToMultiByte(uCP, 0, pW, nChars, NULL, 0, NULL, NULL);
    if (n <= 0) return NULL;
    char* p = (char*)JvMalloc((SIZE_T)n + 1);
    if (p == NULL) return NULL;
    WideCharToMultiByte(uCP, 0, pW, nChars, p, n, NULL, NULL);
    p[n] = '\0';
    if (pnBytes != NULL) *pnBytes = n;
    return p;
}

// ============================================================ 节点构造与释放

static CxJsonNode* JvAllocNode(INT nType)
{
    CxJsonNode* n = (CxJsonNode*)JvMalloc(sizeof(CxJsonNode));
    if (n == NULL) return NULL;
    n->type = nType;
    n->refCount = 1;
    return n;
}

static CxJsonObj* JvObjNew()
{
    CxJsonObj* o = (CxJsonObj*)JvMalloc(sizeof(CxJsonObj));
    if (o == NULL) return NULL;
    o->cap = 8;
    o->bucketCap = 11;                                  // 与原版初始桶数一致
    o->order = (CxJsonNode**)JvMalloc((SIZE_T)o->cap * sizeof(CxJsonNode*));
    o->bucket = (CxJsonNode**)JvMalloc((SIZE_T)o->bucketCap * sizeof(CxJsonNode*));
    if (o->order == NULL || o->bucket == NULL)
    {
        JvFree(o->order); JvFree(o->bucket); JvFree(o);
        return NULL;
    }
    return o;
}

static CxJsonArr* JvArrNew()
{
    CxJsonArr* a = (CxJsonArr*)JvMalloc(sizeof(CxJsonArr));
    if (a == NULL) return NULL;
    a->cap = 8;
    a->items = (CxJsonNode**)JvMalloc((SIZE_T)a->cap * sizeof(CxJsonNode*));
    if (a->items == NULL) { JvFree(a); return NULL; }
    return a;
}

static void JvObjFree(CxJsonObj* o)
{
    if (o == NULL) return;
    for (INT i = 0; i < o->count; i++)
    {
        if (o->order[i] != NULL && o->order[i]->key != NULL)
        {
            JvFree(o->order[i]->key);
            o->order[i]->key = NULL;
        }
    }
    JvFree(o->order);
    JvFree(o->bucket);
    JvFree(o);
}

static void JvArrFree(CxJsonArr* a)
{
    if (a == NULL) return;
    JvFree(a->items);
    JvFree(a);
}

CxJsonNode* JvNewObject()
{
    CxJsonNode* n = JvAllocNode(CXJV_OBJECT);
    if (n == NULL) return NULL;
    n->v.obj = JvObjNew();
    if (n->v.obj == NULL) { JvFree(n); return NULL; }
    return n;
}

CxJsonNode* JvNewArray()
{
    CxJsonNode* n = JvAllocNode(CXJV_ARRAY);
    if (n == NULL) return NULL;
    n->v.arr = JvArrNew();
    if (n->v.arr == NULL) { JvFree(n); return NULL; }
    return n;
}

static CxJsonNode* JvNewScalar(INT nType, const WCHAR* pRawOpt, INT nRawChars)
{
    CxJsonNode* n = JvAllocNode(nType);
    if (n == NULL) return NULL;
    if (pRawOpt != NULL)
    {
        n->raw = JvWideDup(pRawOpt, nRawChars);
        if (n->raw == NULL) { JvFree(n); return NULL; }
    }
    return n;
}

CxJsonNode* JvNewNull()
{
    static const WCHAR kNull[] = { 'n', 'u', 'l', 'l', 0 };
    return JvNewScalar(CXJV_NULL, kNull, 4);
}

CxJsonNode* JvNewBool(INT bTrue)
{
    static const WCHAR kTrue[]  = { 't', 'r', 'u', 'e', 0 };
    static const WCHAR kFalse[] = { 'f', 'a', 'l', 's', 'e', 0 };
    INT b = bTrue ? 1 : 0;
    CxJsonNode* n = JvNewScalar(CXJV_BOOL, b ? kTrue : kFalse, 4 + b);
    if (n != NULL) n->v.b = b;
    return n;
}

CxJsonNode* JvNewUndefined(const WCHAR* raw)
{
    return JvNewScalar(CXJV_UNDEFINED, raw, raw == NULL ? 0 : JvWideLen(raw));
}

CxJsonNode* JvNewDouble(double d, const WCHAR* rawOpt)
{
    WCHAR szTmp[40];
    if (rawOpt == NULL) { JvDoubleToText(d, szTmp, 40); rawOpt = szTmp; }
    CxJsonNode* n = JvNewScalar(CXJV_DOUBLE, rawOpt, rawOpt == szTmp ? -1 : JvWideLen(rawOpt));
    if (n != NULL) n->v.d = d;
    return n;
}

CxJsonNode* JvNewLong(__int64 l, const WCHAR* rawOpt)
{
    WCHAR szTmp[32];
    if (rawOpt == NULL) { JvInt64ToText(l, szTmp, 32); rawOpt = szTmp; }
    CxJsonNode* n = JvNewScalar(CXJV_LONG, rawOpt, rawOpt == szTmp ? -1 : JvWideLen(rawOpt));
    if (n != NULL) n->v.l = l;
    return n;
}

CxJsonNode* JvNewString(const WCHAR* pSrc, INT nChars)
{
    if (nChars < 0) nChars = (pSrc == NULL) ? 0 : JvWideLen(pSrc);
    CxJsonNode* n = JvAllocNode(CXJV_STRING);
    if (n == NULL) return NULL;
    n->v.str = JvWideDup(pSrc, nChars);
    if (n->v.str == NULL) { JvFree(n); return NULL; }
    return n;
}

void JvRetain(CxJsonNode* n)
{
    if (n != NULL) InterlockedIncrement((LONG*)&n->refCount);
}

static void JvFreeSelf(CxJsonNode* n)
{
    if (n == NULL) return;
    if (n->raw != NULL) { JvFree(n->raw); n->raw = NULL; }
    if (n->type == CXJV_STRING)
    {
        if (n->v.str != NULL) JvFree(n->v.str);
        n->v.str = NULL;
    }
    else if (n->type == CXJV_ARRAY) { JvArrFree(n->v.arr); n->v.arr = NULL; }
    else if (n->type == CXJV_OBJECT) { JvObjFree(n->v.obj); n->v.obj = NULL; }
    JvFree(n);
}

// 原版 JsonValue_Clear：递归释放子内容并清掉 raw，节点类型保持不变。
void JvClear(CxJsonNode* n)
{
    if (n == NULL) return;
    if (n->type == CXJV_OBJECT && n->v.obj != NULL)
    {
        CxJsonObj* o = n->v.obj;
        for (INT i = 0; i < o->count; i++)
        {
            CxJsonNode* c = o->order[i];
            o->order[i] = NULL;
            if (c == NULL) continue;
            if (c->key != NULL) { JvFree(c->key); c->key = NULL; }
            c->hashNext = NULL;
            c->parent = NULL;
            JvRelease(c);
        }
        o->count = 0;
        memset(o->bucket, 0, (SIZE_T)o->bucketCap * sizeof(CxJsonNode*));
    }
    else if (n->type == CXJV_ARRAY && n->v.arr != NULL)
    {
        CxJsonArr* a = n->v.arr;
        for (INT i = 0; i < a->count; i++)
        {
            CxJsonNode* c = a->items[i];
            a->items[i] = NULL;
            if (c == NULL) continue;
            c->parent = NULL;
            JvRelease(c);
        }
        a->count = 0;
    }
    else if (n->type == CXJV_STRING && n->v.str != NULL)
    {
        JvFree(n->v.str);
        n->v.str = NULL;
    }
    else
    {
        n->v.l = 0;
    }
    if (n->raw != NULL) { JvFree(n->raw); n->raw = NULL; }
}

void JvDetach(CxJsonNode* n)
{
    if (n == NULL || n->parent == NULL) return;
    CxJsonNode* p = n->parent;
    if (p->type == CXJV_OBJECT && p->v.obj != NULL)
    {
        CxJsonObj* o = p->v.obj;
        for (INT i = 0; i < o->count; i++)
        {
            if (o->order[i] == n)
            {
                memmove(o->order + i, o->order + i + 1, (SIZE_T)(o->count - i - 1) * sizeof(CxJsonNode*));
                o->count--;
                o->order[o->count] = NULL;
                break;
            }
        }
        INT bucket = (INT)((UINT)n->hashCode & 0x7fffffffu) % o->bucketCap;
        CxJsonNode* prev = NULL;
        for (CxJsonNode* it = o->bucket[bucket]; it != NULL; prev = it, it = it->hashNext)
        {
            if (it == n)
            {
                if (prev == NULL) o->bucket[bucket] = it->hashNext;
                else prev->hashNext = it->hashNext;
                n->hashNext = NULL;
                break;
            }
        }
        if (n->key != NULL) { JvFree(n->key); n->key = NULL; }
    }
    else if (p->type == CXJV_ARRAY && p->v.arr != NULL)
    {
        CxJsonArr* a = p->v.arr;
        for (INT i = 0; i < a->count; i++)
        {
            if (a->items[i] == n)
            {
                memmove(a->items + i, a->items + i + 1, (SIZE_T)(a->count - i - 1) * sizeof(CxJsonNode*));
                a->count--;
                a->items[a->count] = NULL;
                break;
            }
        }
    }
    n->parent = NULL;
}

void JvRelease(CxJsonNode* n)
{
    if (n == NULL) return;
    if (InterlockedDecrement((LONG*)&n->refCount) > 0) return;
    JvDetach(n);
    if (n->type == CXJV_OBJECT && n->v.obj != NULL)
    {
        CxJsonObj* o = n->v.obj;
        for (INT i = 0; i < o->count; i++)
        {
            CxJsonNode* c = o->order[i];
            o->order[i] = NULL;
            if (c == NULL) continue;
            if (c->key != NULL) { JvFree(c->key); c->key = NULL; }
            c->hashNext = NULL;
            c->parent = NULL;
            JvRelease(c);
        }
        o->count = 0;
    }
    else if (n->type == CXJV_ARRAY && n->v.arr != NULL)
    {
        CxJsonArr* a = n->v.arr;
        for (INT i = 0; i < a->count; i++)
        {
            CxJsonNode* c = a->items[i];
            a->items[i] = NULL;
            if (c == NULL) continue;
            c->parent = NULL;
            JvRelease(c);
        }
        a->count = 0;
    }
    JvFreeSelf(n);
}

// ============================================================ 就地改类型

static BOOL JvSetScalar(CxJsonNode* n, INT nType, const WCHAR* pRawOpt)
{
    if (n == NULL) return FALSE;
    WCHAR* pRaw = (pRawOpt == NULL) ? NULL : JvWideDup(pRawOpt, -1);
    JvClear(n);
    n->type = nType;
    if (pRaw == NULL && pRawOpt != NULL) return FALSE;
    n->raw = pRaw;
    return TRUE;
}

BOOL JvSetNull(CxJsonNode* n)
{
    static const WCHAR kNull[] = { 'n', 'u', 'l', 'l', 0 };
    return JvSetScalar(n, CXJV_NULL, kNull);
}

BOOL JvSetBool(CxJsonNode* n, INT bTrue)
{
    static const WCHAR kTrue[]  = { 't', 'r', 'u', 'e', 0 };
    static const WCHAR kFalse[] = { 'f', 'a', 'l', 's', 'e', 0 };
    INT b = bTrue ? 1 : 0;
    if (!JvSetScalar(n, CXJV_BOOL, b ? kTrue : kFalse)) return FALSE;
    n->v.b = b;
    return TRUE;
}

BOOL JvSetDouble(CxJsonNode* n, double d, const WCHAR* rawOpt)
{
    if (n == NULL) return FALSE;
    WCHAR szTmp[40];
    if (rawOpt == NULL) { JvDoubleToText(d, szTmp, 40); rawOpt = szTmp; }
    if (!JvSetScalar(n, CXJV_DOUBLE, rawOpt)) return FALSE;
    n->v.d = d;
    return TRUE;
}

BOOL JvSetLong(CxJsonNode* n, __int64 l, const WCHAR* rawOpt)
{
    if (n == NULL) return FALSE;
    WCHAR szTmp[32];
    if (rawOpt == NULL) { JvInt64ToText(l, szTmp, 32); rawOpt = szTmp; }
    if (!JvSetScalar(n, CXJV_LONG, rawOpt)) return FALSE;
    n->v.l = l;
    return TRUE;
}

BOOL JvSetInt(CxJsonNode* n, INT i)
{
    return JvSetLong(n, (__int64)i, NULL);
}

BOOL JvSetString(CxJsonNode* n, const WCHAR* pSrc, INT nChars)
{
    if (n == NULL) return FALSE;
    if (nChars < 0) nChars = (pSrc == NULL) ? 0 : JvWideLen(pSrc);
    WCHAR* pNew = JvWideDup(pSrc, nChars);
    if (pNew == NULL) return FALSE;
    JvClear(n);                                   // 文本不写 raw（原版 SetString 也清 raw）
    n->type = CXJV_STRING;
    n->v.str = pNew;
    return TRUE;
}

BOOL JvSetObject(CxJsonNode* n)
{
    if (n == NULL) return FALSE;
    if (n->type == CXJV_OBJECT) return TRUE;      // 同类型：成员原样保留
    CxJsonObj* pNew = JvObjNew();
    if (pNew == NULL) return FALSE;
    JvClear(n);
    n->type = CXJV_OBJECT;
    n->v.obj = pNew;
    return TRUE;
}

BOOL JvSetArray(CxJsonNode* n)
{
    if (n == NULL) return FALSE;
    if (n->type == CXJV_ARRAY) return TRUE;
    CxJsonArr* pNew = JvArrNew();
    if (pNew == NULL) return FALSE;
    JvClear(n);
    n->type = CXJV_ARRAY;
    n->v.arr = pNew;
    return TRUE;
}

// ============================================================ 取值与强转（口径照原版）

// 长整数 → 双精度：拆成高低 32 位，避开 __ltod3。
double JvI64ToDouble(__int64 l)
{
    INT nHi = (INT)(l >> 32);
    UINT uLo = (UINT)(l & 0xffffffffi64);
    return (double)nHi * 4294967296.0 + (double)uLo;
}

// 双精度 → 长整数：向零截断，逐级降到 32 位转换，避开 __dtol3。
__int64 JvDoubleToI64(double d)
{
    if (!(d < 9223372036854775807.0)) return (__int64)0x7fffffffffffffffui64;
    if (d < -9223372036854775808.0) return (__int64)0x8000000000000000ui64;
    BOOL bNeg = (d < 0.0) ? TRUE : FALSE;
    if (bNeg) d = -d;
    __int64 r;
    if (d >= 4294967296.0)
    {
        // 只允许 double→int32（编译成内联 cvttsd2si）：低 32 位先减去 2^31 再转，避免越界。
        INT nHi = (INT)(d / 4294967296.0);
        double dRest = d - (double)nHi * 4294967296.0;
        INT nLo = (INT)(dRest - 2147483648.0);
        r = ((__int64)nHi << 32) + ((__int64)nLo + 2147483648i64);
    }
    else
    {
        r = (__int64)(INT)d;
    }
    return bNeg ? -r : r;
}

INT JvTypeOf(const CxJsonNode* n)
{
    return (n == NULL) ? CXJV_UNDEFINED : n->type;
}

const WCHAR* JvRaw(const CxJsonNode* n)
{
    return (n == NULL) ? NULL : n->raw;
}

const WCHAR* JvStringPtr(const CxJsonNode* n)
{
    if (n == NULL || n->type != CXJV_STRING) return NULL;
    return n->v.str;
}

BOOL JvGetBool(const CxJsonNode* n)
{
    if (n == NULL) return FALSE;
    switch (n->type)
    {
    case CXJV_LONG:   return (n->v.l & 0xffffffffi64) != 0;   // 原版 _Get 只看低 32 位
    case CXJV_DOUBLE: return n->v.d != 0.0;
    case CXJV_BOOL:   return n->v.b != 0;
    default:          return FALSE;                            // 文本/空/数组/对象一律假
    }
}

double JvGetDouble(const CxJsonNode* n)
{
    if (n == NULL) return 0.0;
    switch (n->type)
    {
    case CXJV_DOUBLE: return n->v.d;
    case CXJV_LONG:   return JvI64ToDouble(n->v.l);
    case CXJV_BOOL:   return (double)n->v.b;
    case CXJV_STRING: return (n->v.str != NULL) ? JvTextToDouble(n->v.str) : 0.0;
    default:          return 0.0;
    }
}

__int64 JvGetLong(const CxJsonNode* n)
{
    if (n == NULL) return 0;
    switch (n->type)
    {
    case CXJV_LONG:   return n->v.l;
    case CXJV_DOUBLE: return JvDoubleToI64(n->v.d);                // 向零截断
    case CXJV_BOOL:   return (__int64)n->v.b;
    case CXJV_STRING: return (n->v.str != NULL) ? JvTextToI64(n->v.str) : 0;
    default:          return 0;
    }
}

INT JvGetInt(const CxJsonNode* n)
{
    if (n == NULL) return 0;
    switch (n->type)
    {
    case CXJV_LONG:   return (INT)(UINT)(n->v.l & 0xffffffffi64);  // 只取低 32 位
    case CXJV_DOUBLE: return (INT)n->v.d;
    case CXJV_BOOL:   return n->v.b;
    case CXJV_STRING: return (n->v.str != NULL) ? JvTextToInt(n->v.str) : 0;
    default:          return 0;
    }
}

WCHAR* JvGetStringEx(const CxJsonNode* n)
{
    if (n == NULL) return NULL;
    if (n->type == CXJV_STRING) return JvWideDup(n->v.str, -1);
    if (n->type == CXJV_NULL || n->type == CXJV_BOOL || n->type == CXJV_LONG || n->type == CXJV_DOUBLE)
    {
        return JvWideDup(n->raw, -1);
    }
    return NULL;
}

// ============================================================ 容器

static INT JvKeyHash(const WCHAR* pKey, INT nChars)
{
    // 原版按字节从尾往前：h = h*31 + byte，UTF-16 每字符贡献低字节、高字节。
    INT h = 0;
    const BYTE* p = (const BYTE*)pKey;
    for (int i = nChars * 2 - 1; i >= 0; i--) h = h * 31 + (INT)p[i];
    return h;
}

INT JvCount(const CxJsonNode* n)
{
    if (n == NULL) return 0;
    if (n->type == CXJV_OBJECT && n->v.obj != NULL) return n->v.obj->count;
    if (n->type == CXJV_ARRAY  && n->v.arr != NULL) return n->v.arr->count;
    return 0;
}

static CxJsonNode* JvObjFind(CxJsonObj* o, const WCHAR* pKey)
{
    if (o == NULL || pKey == NULL) return NULL;
    INT h = JvKeyHash(pKey, JvWideLen(pKey));
    INT bucket = (INT)((UINT)h & 0x7fffffffu) % o->bucketCap;
    for (CxJsonNode* it = o->bucket[bucket]; it != NULL; it = it->hashNext)
    {
        if (it->hashCode == h && it->key != NULL && JvWideCmp(it->key, pKey) == 0) return it;
    }
    return NULL;
}

BOOL JvContainsKey(const CxJsonNode* n, const WCHAR* k)
{
    if (n == NULL || n->type != CXJV_OBJECT) return FALSE;
    return JvObjFind(n->v.obj, k) != NULL;
}

static BOOL JvOrderGrow(CxJsonObj* o)
{
    if (o->count < o->cap) return TRUE;
    INT nNew = o->cap * 2 + 1;
    CxJsonNode** p = (CxJsonNode**)JvMalloc((SIZE_T)nNew * sizeof(CxJsonNode*));
    if (p == NULL) return FALSE;
    memcpy(p, o->order, (SIZE_T)o->cap * sizeof(CxJsonNode*));
    JvFree(o->order);
    o->order = p;
    o->cap = nNew;
    return TRUE;
}

static BOOL JvObjRehash(CxJsonObj* o, INT nNewCap)
{
    CxJsonNode** pNew = (CxJsonNode**)JvMalloc((SIZE_T)nNewCap * sizeof(CxJsonNode*));
    if (pNew == NULL) return FALSE;
    for (INT i = 0; i < o->count; i++)              // 只重挂桶链，顺序仍看 order
    {
        CxJsonNode* it = o->order[i];
        INT b = (INT)((UINT)it->hashCode & 0x7fffffffu) % nNewCap;
        it->hashNext = pNew[b];
        pNew[b] = it;
    }
    JvFree(o->bucket);
    o->bucket = pNew;
    o->bucketCap = nNewCap;
    return TRUE;
}

// 新键追加到有序序列末尾；同名键在原位置换值、沿用首次插入的键名（大小写以首次为准）。
static BOOL JvObjPut(CxJsonObj* o, const WCHAR* pKey, CxJsonNode* child, INT idxInsert, BOOL bInsertAt)
{
    if (o == NULL || pKey == NULL || child == NULL) return FALSE;
    INT nChars = JvWideLen(pKey);
    INT h = JvKeyHash(pKey, nChars);
    INT bucket = (INT)((UINT)h & 0x7fffffffu) % o->bucketCap;
    CxJsonNode* prev = NULL;
    for (CxJsonNode* it = o->bucket[bucket]; it != NULL; prev = it, it = it->hashNext)
    {
        if (it->hashCode == h && it->key != NULL && JvWideCmp(it->key, pKey) == 0)
        {
            WCHAR* pKeyCopy = JvWideDup(it->key, nChars);
            if (pKeyCopy == NULL) return FALSE;
            for (INT i = 0; i < o->count; i++)
            {
                if (o->order[i] == it) { o->order[i] = child; break; }
            }
            child->key = pKeyCopy;
            child->hashCode = h;
            child->hashNext = it->hashNext;
            if (prev == NULL) o->bucket[bucket] = child;
            else prev->hashNext = child;
            it->hashNext = NULL;
            return TRUE;
        }
    }
    WCHAR* pKeyNew = JvWideDup(pKey, nChars);
    if (pKeyNew == NULL) return FALSE;
    if (bInsertAt)
    {
        if (idxInsert < 0 || idxInsert > o->count) { JvFree(pKeyNew); return FALSE; }
        if (!JvOrderGrow(o)) { JvFree(pKeyNew); return FALSE; }
        memmove(o->order + idxInsert + 1, o->order + idxInsert,
                (SIZE_T)(o->count - idxInsert) * sizeof(CxJsonNode*));
        o->order[idxInsert] = child;
    }
    else
    {
        if (!JvOrderGrow(o)) { JvFree(pKeyNew); return FALSE; }
        o->order[o->count] = child;
    }
    o->count++;
    child->key = pKeyNew;
    child->hashCode = h;
    if (o->count * 4 >= o->bucketCap * 3)
    {
        // 扩容会把 order 里所有节点（含刚加入的这一个）重挂到新桶链，不能再手工插入一次，
        // 否则 child->hashNext 指回自己，JvObjFind 就死循环了。
        if (!JvObjRehash(o, o->bucketCap * 2 + 1)) return FALSE;
        return TRUE;
    }
    child->hashNext = o->bucket[bucket];
    o->bucket[bucket] = child;
    return TRUE;
}

CxJsonNode* JvChildReadOnly(CxJsonNode* n, const WCHAR* k)
{
    if (n == NULL || n->type != CXJV_OBJECT) return NULL;
    return JvObjFind(n->v.obj, k);
}

CxJsonNode* JvChildOrCreate(CxJsonNode* n, const WCHAR* k)
{
    if (n == NULL || k == NULL) return NULL;
    if (n->type != CXJV_OBJECT && !JvSetObject(n)) return NULL;
    CxJsonNode* child = JvObjFind(n->v.obj, k);
    if (child != NULL) return child;
    CxJsonNode* jv = JvNewNull();                   // 读操作会改文档：缺键补 null
    if (jv == NULL) return NULL;
    jv->parent = n;
    if (!JvObjPut(n->v.obj, k, jv, 0, FALSE))
    {
        jv->parent = NULL;
        JvRelease(jv);
        return NULL;
    }
    return jv;
}

BOOL JvChildSet(CxJsonNode* n, const WCHAR* k, CxJsonNode* child)
{
    if (n == NULL || k == NULL || child == NULL) return FALSE;
    if (n->type != CXJV_OBJECT && !JvSetObject(n)) return FALSE;
    CxJsonNode* old = JvObjFind(n->v.obj, k);
    child->parent = n;
    if (!JvObjPut(n->v.obj, k, child, 0, FALSE))
    {
        child->parent = NULL;
        return FALSE;
    }
    if (old != NULL) JvRelease(old);
    return TRUE;
}

BOOL JvChildInsert(CxJsonNode* n, INT idx, const WCHAR* k, CxJsonNode* child)
{
    if (n == NULL || k == NULL || child == NULL) return FALSE;
    if (n->type != CXJV_OBJECT && !JvSetObject(n)) return FALSE;
    CxJsonNode* old = JvObjFind(n->v.obj, k);
    child->parent = n;
    BOOL bOk = (old != NULL) ? JvObjPut(n->v.obj, k, child, 0, FALSE)
                             : JvObjPut(n->v.obj, k, child, idx, TRUE);
    if (!bOk) { child->parent = NULL; return FALSE; }
    if (old != NULL) JvRelease(old);
    return TRUE;
}

BOOL JvChildRemove(CxJsonNode* n, const WCHAR* k)
{
    if (n == NULL || n->type != CXJV_OBJECT || n->v.obj == NULL) return FALSE;
    CxJsonNode* child = JvObjFind(n->v.obj, k);
    if (child == NULL) return FALSE;
    child->parent = NULL;                           // 先脱链，避免 JvRelease 再走一遍 Detch
    CxJsonObj* o = n->v.obj;
    for (INT i = 0; i < o->count; i++)
    {
        if (o->order[i] == child)
        {
            memmove(o->order + i, o->order + i + 1, (SIZE_T)(o->count - i - 1) * sizeof(CxJsonNode*));
            o->count--;
            o->order[o->count] = NULL;
            break;
        }
    }
    INT bucket = (INT)((UINT)child->hashCode & 0x7fffffffu) % o->bucketCap;
    CxJsonNode* prev = NULL;
    for (CxJsonNode* it = o->bucket[bucket]; it != NULL; prev = it, it = it->hashNext)
    {
        if (it == child)
        {
            if (prev == NULL) o->bucket[bucket] = it->hashNext;
            else prev->hashNext = it->hashNext;
            child->hashNext = NULL;
            break;
        }
    }
    if (child->key != NULL) { JvFree(child->key); child->key = NULL; }
    JvRelease(child);
    return TRUE;
}

static BOOL JvArrGrow(CxJsonArr* a, INT nNeed)
{
    if (nNeed <= a->cap) return TRUE;
    INT nNew = a->cap * 2 + nNeed + 1;
    CxJsonNode** p = (CxJsonNode**)JvMalloc((SIZE_T)nNew * sizeof(CxJsonNode*));
    if (p == NULL) return FALSE;
    memcpy(p, a->items, (SIZE_T)a->cap * sizeof(CxJsonNode*));
    JvFree(a->items);
    a->items = p;
    a->cap = nNew;
    return TRUE;
}

CxJsonNode* JvAtReadOnly(CxJsonNode* n, INT idx)
{
    if (n == NULL) return NULL;
    if (n->type == CXJV_ARRAY)
    {
        if (n->v.arr == NULL || idx < 0 || idx >= n->v.arr->count) return NULL;
        return n->v.arr->items[idx];
    }
    if (n->type == CXJV_OBJECT)
    {
        if (n->v.obj == NULL || idx < 0 || idx >= n->v.obj->count) return NULL;
        return n->v.obj->order[idx];                // 对象按位置取成员
    }
    return NULL;
}

CxJsonNode* JvAtOrCreate(CxJsonNode* n, INT idx)
{
    if (n == NULL) return NULL;
    if (n->type != CXJV_ARRAY && !JvSetArray(n)) return NULL;
    CxJsonArr* a = n->v.arr;
    if (idx < 0) return NULL;
    if (idx >= a->count)                            // 越界读会变成写：补 null 到该下标
    {
        if (!JvArrGrow(a, idx + 1)) return NULL;
        for (INT i = a->count; i <= idx; i++)
        {
            CxJsonNode* jv = JvNewNull();
            if (jv == NULL) return NULL;
            jv->parent = n;
            a->items[i] = jv;
            a->count++;
        }
    }
    return a->items[idx];
}

BOOL JvAppend(CxJsonNode* n, CxJsonNode* child)
{
    if (n == NULL || child == NULL) return FALSE;
    if (n->type != CXJV_ARRAY && !JvSetArray(n)) return FALSE;
    if (!JvArrGrow(n->v.arr, n->v.arr->count + 1)) return FALSE;
    child->parent = n;
    n->v.arr->items[n->v.arr->count++] = child;
    return TRUE;
}

BOOL JvInsertAt(CxJsonNode* n, INT idx, CxJsonNode* child)
{
    if (n == NULL || child == NULL) return FALSE;
    if (n->type != CXJV_ARRAY && !JvSetArray(n)) return FALSE;
    CxJsonArr* a = n->v.arr;
    if (idx < 0 || idx > a->count) return FALSE;
    if (!JvArrGrow(a, a->count + 1)) return FALSE;
    memmove(a->items + idx + 1, a->items + idx, (SIZE_T)(a->count - idx) * sizeof(CxJsonNode*));
    a->items[idx] = child;
    a->count++;
    child->parent = n;
    return TRUE;
}

BOOL JvAtSet(CxJsonNode* n, INT idx, CxJsonNode* child)
{
    if (n == NULL || child == NULL) return FALSE;
    if (n->type != CXJV_ARRAY && !JvSetArray(n)) return FALSE;
    CxJsonArr* a = n->v.arr;
    if (idx < 0) return FALSE;
    if (idx >= a->count)
    {
        if (!JvArrGrow(a, idx + 1)) return FALSE;
        for (INT i = a->count; i < idx; i++)
        {
            CxJsonNode* jv = JvNewNull();
            if (jv == NULL) return FALSE;
            jv->parent = n;
            a->items[i] = jv;
            a->count++;
        }
        child->parent = n;
        a->items[idx] = child;
        a->count = idx + 1;
        return TRUE;
    }
    CxJsonNode* old = a->items[idx];
    child->parent = n;
    a->items[idx] = child;
    if (old != NULL) { old->parent = NULL; JvRelease(old); }
    return TRUE;
}

BOOL JvRemoveAt(CxJsonNode* n, INT idx)
{
    if (n == NULL || n->type != CXJV_ARRAY || n->v.arr == NULL) return FALSE;
    CxJsonArr* a = n->v.arr;
    if (idx < 0 || idx >= a->count) return FALSE;
    CxJsonNode* child = a->items[idx];
    memmove(a->items + idx, a->items + idx + 1, (SIZE_T)(a->count - idx - 1) * sizeof(CxJsonNode*));
    a->count--;
    a->items[a->count] = NULL;
    if (child != NULL) { child->parent = NULL; JvRelease(child); }
    return TRUE;
}

CxJsonNode* JvOrderBy(const CxJsonNode* n, INT idx)
{
    if (n == NULL) return NULL;
    if (n->type == CXJV_OBJECT && n->v.obj != NULL)
    {
        if (idx < 0 || idx >= n->v.obj->count) return NULL;
        return n->v.obj->order[idx];
    }
    if (n->type == CXJV_ARRAY && n->v.arr != NULL)
    {
        if (idx < 0 || idx >= n->v.arr->count) return NULL;
        return n->v.arr->items[idx];
    }
    return NULL;
}

const WCHAR* JvSelfKey(const CxJsonNode* n)
{
    return (n == NULL) ? NULL : n->key;
}

CxJsonNode* JvGetParent(const CxJsonNode* n)
{
    return (n == NULL) ? NULL : n->parent;
}

// ============================================================ 深拷贝
// 数字不带原 raw，改由 double/long→文本重新生成（与原版 Clone 一致）。
CxJsonNode* JvClone(const CxJsonNode* n)
{
    if (n == NULL) return NULL;
    CxJsonNode* d = NULL;
    switch (n->type)
    {
    case CXJV_NULL:      d = JvNewNull(); break;
    case CXJV_BOOL:      d = JvNewBool(n->v.b); break;
    case CXJV_DOUBLE:    d = JvNewDouble(n->v.d, NULL); break;
    case CXJV_LONG:      d = JvNewLong(n->v.l, NULL); break;
    case CXJV_UNDEFINED: d = JvNewUndefined(n->raw); break;
    case CXJV_STRING:    d = JvNewString(n->v.str, -1); break;
    case CXJV_ARRAY:
        d = JvNewArray();
        if (d != NULL && n->v.arr != NULL)
        {
            for (INT i = 0; i < n->v.arr->count; i++)
            {
                CxJsonNode* c = JvClone(n->v.arr->items[i]);
                if (c == NULL) continue;
                if (!JvAppend(d, c)) JvRelease(c);
            }
        }
        break;
    case CXJV_OBJECT:
        d = JvNewObject();
        if (d != NULL && n->v.obj != NULL)
        {
            for (INT i = 0; i < n->v.obj->count; i++)
            {
                CxJsonNode* src = n->v.obj->order[i];
                if (src == NULL || src->key == NULL) continue;
                CxJsonNode* c = JvClone(src);
                if (c == NULL) continue;
                if (!JvChildSet(d, src->key, c)) JvRelease(c);
            }
        }
        break;
    default: break;
    }
    return d;
}

// ============================================================ 合并

BOOL JvCombineProc(CxJsonNode* src, CxJsonNode* desc, INT bIsAdd)
{
    if (src == NULL || desc == NULL) return FALSE;
    BOOL ret = FALSE;
    if (src->type == CXJV_OBJECT && desc->type == CXJV_OBJECT)
    {
        CxJsonObj* o = desc->v.obj;
        for (INT i = 0; o != NULL && i < o->count; i++)
        {
            CxJsonNode* child = o->order[i];
            if (child == NULL || child->key == NULL) continue;
            CxJsonNode* srcChild = JvObjFind(src->v.obj, child->key);
            CxJsonNode* copy = JvClone(child);
            if (copy == NULL) return FALSE;
            if (srcChild != NULL)
            {
                if (!JvCombineProc(srcChild, copy, bIsAdd))
                {
                    JvRelease(copy);
                    return FALSE;
                }
                ret = TRUE;
            }
            else if (bIsAdd)
            {
                if (!JvChildSet(src, child->key, copy)) { JvRelease(copy); return FALSE; }
                ret = TRUE;
            }
            else JvRelease(copy);
        }
        return ret;                                   // 一次都没成功时保持假（原版如此）
    }
    if (src->key != NULL && src->parent != NULL)      // 类型不同：整节点替换
    {
        return JvChildSet(src->parent, src->key, desc);
    }
    JvRelease(desc);
    return FALSE;
}

BOOL JvCombineObject(CxJsonNode* dst, CxJsonNode* src)
{
    if (dst == NULL || src == NULL || src->type != CXJV_OBJECT || src->v.obj == NULL) return FALSE;
    if (dst->type != CXJV_OBJECT && !JvSetObject(dst)) return FALSE;
    CxJsonObj* o = src->v.obj;
    for (INT i = 0; i < o->count; i++)
    {
        CxJsonNode* child = o->order[i];
        if (child == NULL || child->key == NULL) continue;
        CxJsonNode* copy = JvClone(child);
        if (copy == NULL) return FALSE;
        if (!JvChildSet(dst, child->key, copy)) { JvRelease(copy); return FALSE; }
    }
    return TRUE;
}

BOOL JvCombineArray(CxJsonNode* dst, CxJsonNode* src)
{
    if (dst == NULL || src == NULL || src->type != CXJV_ARRAY || src->v.arr == NULL) return FALSE;
    if (dst->type != CXJV_ARRAY && !JvSetArray(dst)) return FALSE;
    CxJsonArr* a = src->v.arr;
    for (INT i = 0; i < a->count; i++)
    {
        CxJsonNode* copy = JvClone(a->items[i]);
        if (copy == NULL) return FALSE;
        if (!JvAppend(dst, copy)) { JvRelease(copy); return FALSE; }
    }
    return TRUE;
}

// ============================================================ 排序
// 复刻原版 _Array_FastSort：三数取中 + 挖坑 Hoare 分区；比较为 0 时不移动，
// 相等元素的最终次序由此决定，换成标准库快排就会对不上。

static void JvSwap(CxJsonNode** p, INT i, INT j)
{
    CxJsonNode* t = p[i]; p[i] = p[j]; p[j] = t;
}

static void JvFastSort(CxJsonNode** p, INT nLeft, INT nRight, PFN_JV_CMP proc, INT lParam)
{
    if (proc == NULL || nLeft >= nRight) return;
    INT nM = nLeft + (nRight - nLeft) / 2;
    if (proc(p[nM], p[nLeft], lParam) > 0) JvSwap(p, nLeft, nM);
    if (proc(p[nRight], p[nM], lParam) > 0)
    {
        if (proc(p[nRight], p[nLeft], lParam) > 0) JvSwap(p, nLeft, nRight);
        else JvSwap(p, nM, nRight);
    }
    INT i = nLeft, j = nRight;
    CxJsonNode* base = p[nLeft];
    while (i < j)
    {
        while (i < j && proc(p[j], base, lParam) >= 0) j--;
        if (i < j) p[i++] = p[j];
        while (i < j && proc(p[i], base, lParam) <= 0) i++;
        if (i < j) p[j--] = p[i];
    }
    p[i] = base;
    JvFastSort(p, nLeft, i - 1, proc, lParam);
    JvFastSort(p, i + 1, nRight, proc, lParam);
}

void JvSort(CxJsonNode* n, PFN_JV_CMP proc, INT lParam)
{
    if (n == NULL || proc == NULL) return;
    if (n->type == CXJV_OBJECT && n->v.obj != NULL)
    {
        CxJsonObj* o = n->v.obj;
        if (o->count > 1) JvFastSort(o->order, 0, o->count - 1, proc, lParam);
    }
    else if (n->type == CXJV_ARRAY && n->v.arr != NULL)
    {
        CxJsonArr* a = n->v.arr;
        if (a->count > 1) JvFastSort(a->items, 0, a->count - 1, proc, lParam);
    }
}

static INT JvSortProcAsc(CxJsonNode* a, CxJsonNode* b, INT)
{
    return JvWideCmp((a == NULL) ? NULL : a->key, (b == NULL) ? NULL : b->key);
}

static INT JvSortProcDesc(CxJsonNode* a, CxJsonNode* b, INT)
{
    return JvWideCmp((b == NULL) ? NULL : b->key, (a == NULL) ? NULL : a->key);
}

BOOL JvSortObject(CxJsonNode* n, INT bAsc)
{
    if (n == NULL || n->type != CXJV_OBJECT || n->v.obj == NULL) return FALSE;
    CxJsonObj* o = n->v.obj;
    if (o->count > 1) JvFastSort(o->order, 0, o->count - 1, bAsc ? JvSortProcAsc : JvSortProcDesc, 0);
    return TRUE;
}

}  // namespace cx
