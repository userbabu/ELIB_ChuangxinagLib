// zyJson2.51 的 C++ 引擎声明（Json2 类）。
// 行为口径逐条对齐原模块：内部一律 UTF-16LE，文本型按 cp936(GBK) 进出，W 后缀命令的
// 字节集＝UTF-16LE 且末尾带 {0,0}；对象成员＝插入序；数字序列化优先回放原文（raw）。
#ifndef CX_JSON_H
#define CX_JSON_H

#include <cx_uni.h>

// Json2 类在库定义数据类型表里的序号（m_pDataType 下标 + 1），网络 1-3、宽文本与正则 4-8。
#define CX_TYPE_JSON2 9

namespace cx {

// lib.cpp 提供的系统接口与内存池助手（命令实现层共用）。
INT NotifySys(INT nMsg, DWORD dwParam1, DWORD dwParam2);
void ReportError(LPCSTR szErrText);
char* CloneTextData(LPCSTR pszText, INT nLen);
char* CloneTextData(LPCSTR pszText);
LPBYTE CloneBinData(LPCSTR pData, INT nDataSize);

// ============================================================ 值类型（与库常量 #JV类型_* 同值）
enum
{
    CXJV_UNDEFINED = -1,
    CXJV_NULL      = 0,
    CXJV_DOUBLE    = 1,
    CXJV_LONG      = 2,
    CXJV_BOOL      = 3,
    CXJV_STRING    = 4,
    CXJV_ARRAY     = 5,
    CXJV_OBJECT    = 6
};

// ============================================================ 错误码（与 #JSON_* 同值）
enum
{
    CXJSON_OK             = 0,
    CXJSON_ERROR          = -1,
    CXJSON_INVALIDARG     = -2,
    CXJSON_MEMORY         = -3,
    CXJSON_INVALIDNAME    = -4,
    CXJSON_OBJECTPARENT   = -5,
    CXJSON_INVALIDVALUE   = -6
};

struct CxJsonNode;
struct CxJsonObj;
struct CxJsonArr;

struct CxJsonNode
{
    INT          type;          // CXJV_*
    INT          refCount;      // 初始 1；容器持有子节点的那一份
    INT          hashCode;      // 属性名哈希（仅对象成员有意义）
    CxJsonNode*  hashNext;      // 桶内链
    CxJsonNode*  parent;        // 裸指针，不持引用
    WCHAR*       key;           // 在父对象里的属性名（归父对象所有）
    WCHAR*       raw;           // 元数据：数字/布尔/空的原文，序列化时原样回放
    union
    {
        WCHAR*     str;
        double     d;
        __int64    l;
        INT        b;
        CxJsonObj* obj;
        CxJsonArr* arr;
    } v;
};

struct CxJsonArr
{
    CxJsonNode** items;
    INT count;
    INT cap;
};

struct CxJsonObj
{
    CxJsonNode** order;         // 插入序（序列化/取所有键/取所有值都按它）
    INT          count;
    INT          cap;
    CxJsonNode** bucket;        // 桶链头，节点用 hashNext 串联
    INT          bucketCap;
};

// 易语言侧 Json2 对象的宿主结构：对象数据里存的是它的指针。
struct CxJson2
{
    CxJsonNode* node;
    INT         isTop;          // 集_是否为顶层
    INT         inited;         // 集_是否已初始化
};

// ============================================================ 生命周期
CxJsonNode* JvNewObject();
CxJsonNode* JvNewArray();
CxJsonNode* JvNewNull();
CxJsonNode* JvNewBool(INT bTrue);
CxJsonNode* JvNewUndefined(const WCHAR* raw);
CxJsonNode* JvNewDouble(double d, const WCHAR* rawOpt);
CxJsonNode* JvNewLong(__int64 l, const WCHAR* rawOpt);
CxJsonNode* JvNewString(const WCHAR* pSrc, INT nChars);
void        JvRetain(CxJsonNode* n);
void        JvRelease(CxJsonNode* n);          // 归零：脱父 → 递归释放子树 → 本体
void        JvDetach(CxJsonNode* n);
void        JvClear(CxJsonNode* n);            // 清空子内容，类型不变

// ============================================================ 就地改类型（先销毁旧内容）
BOOL JvSetNull(CxJsonNode* n);
BOOL JvSetBool(CxJsonNode* n, INT bTrue);
BOOL JvSetDouble(CxJsonNode* n, double d, const WCHAR* rawOpt);
BOOL JvSetLong(CxJsonNode* n, __int64 l, const WCHAR* rawOpt);
BOOL JvSetInt(CxJsonNode* n, INT i);           // 存成 CXJV_LONG
BOOL JvSetString(CxJsonNode* n, const WCHAR* pSrc, INT nChars);
BOOL JvSetObject(CxJsonNode* n);               // 已是对象则原样保留
BOOL JvSetArray(CxJsonNode* n);                // 已是数组则原样保留

// ============================================================ 取值（含原版跨类型强转）
INT         JvTypeOf(const CxJsonNode* n);      // n==NULL → CXJV_UNDEFINED
const WCHAR* JvRaw(const CxJsonNode* n);        // 文本/数组/对象没有 raw
const WCHAR* JvStringPtr(const CxJsonNode* n);  // 仅 CXJV_STRING
BOOL        JvGetBool(const CxJsonNode* n);
double      JvGetDouble(const CxJsonNode* n);
__int64     JvGetLong(const CxJsonNode* n);
INT         JvGetInt(const CxJsonNode* n);
// GetStringEx：文本→内容，空/布尔/长整/双精→raw，其它→空。返回堆缓冲（JvFree），可为 NULL。
WCHAR*      JvGetStringEx(const CxJsonNode* n);

// ============================================================ 容器操作
INT         JvCount(const CxJsonNode* n);                       // 对象成员数 / 数组元素数
BOOL        JvContainsKey(const CxJsonNode* n, const WCHAR* k);
CxJsonNode* JvChildReadOnly(CxJsonNode* n, const WCHAR* k);     // 不创建、不改类型
CxJsonNode* JvChildOrCreate(CxJsonNode* n, const WCHAR* k);     // 非对象→转对象；缺键→补 null
BOOL        JvChildSet(CxJsonNode* n, const WCHAR* k, CxJsonNode* child);   // 子树所有权交给 n
BOOL        JvChildInsert(CxJsonNode* n, INT idx, const WCHAR* k, CxJsonNode* child);
BOOL        JvChildRemove(CxJsonNode* n, const WCHAR* k);       // 仅对象；成功才真
CxJsonNode* JvAtReadOnly(CxJsonNode* n, INT idx);               // 数组按下标 / 对象按位置
CxJsonNode* JvAtOrCreate(CxJsonNode* n, INT idx);               // 非数组→转数组；越界→补 null
BOOL        JvAtSet(CxJsonNode* n, INT idx, CxJsonNode* child);
BOOL        JvAppend(CxJsonNode* n, CxJsonNode* child);
BOOL        JvInsertAt(CxJsonNode* n, INT idx, CxJsonNode* child);
BOOL        JvRemoveAt(CxJsonNode* n, INT idx);
CxJsonNode* JvOrderBy(const CxJsonNode* n, INT idx);            // 第 idx 个成员节点（0-based）
const WCHAR* JvSelfKey(const CxJsonNode* n);                    // 自身属性名
CxJsonNode* JvGetParent(const CxJsonNode* n);

// ============================================================ 树操作
CxJsonNode* JvClone(const CxJsonNode* n);                       // 深拷贝，不带父；数字重走 double→text
// 路径解析：a.b.c[0] / [0].a；空路径 → 当前节点。createCreate 为真时沿途创建。
CxJsonNode* JvSelect(CxJsonNode* n, const WCHAR* pPath, INT bCreate);
BOOL        JvQuery(CxJsonNode* n, const WCHAR* pPath);         // 是否存在（不创建；空路径→假）

// ============================================================ 解析 / 序列化
// pStr 必须可读 len 个宽字符；返回 CXJSON_*，*pnErrPos 为宽字符下标。
INT   JvParse(CxJsonNode* n, const WCHAR* pStr, INT nLen, INT bTransU, INT* pnErrPos);
void  JvToString(const CxJsonNode* n, UniWBuf* pOut, INT bAutoFormat, INT bIsTrans);
// 合并 JSON 文本：解析到临时对象再逐键并入；isAdd 控制缺失键是否追加。
BOOL  JvCombine(CxJsonNode* n, const WCHAR* pStr, INT nLen, INT bTransU, INT bIsAdd);
BOOL  JvCombineProc(CxJsonNode* dst, CxJsonNode* src, INT bIsAdd);   // 递归并键（对象×对象）
BOOL  JvCombineObject(CxJsonNode* dst, CxJsonNode* src);        // 浅覆盖
BOOL  JvCombineArray(CxJsonNode* dst, CxJsonNode* src);         // 尾部拼接

typedef INT (*PFN_JV_CMP)(CxJsonNode* a, CxJsonNode* b, INT lParam);
void  JvSort(CxJsonNode* n, PFN_JV_CMP proc, INT lParam);
BOOL  JvSortObject(CxJsonNode* n, INT bAsc);

// ============================================================ 文本 / 数字转换
void*  JvMalloc(SIZE_T nBytes);                 // 进程堆，零初始化
void   JvFree(void* p);
WCHAR* JvWideDup(const WCHAR* pSrc, INT nChars);
WCHAR* JvAnsiToWide(LPCSTR pA, INT nBytes, UINT uCP);   // 结果末尾带 {0,0}，JvFree 释放
// 宽→单字节；返回 JvMalloc 缓冲（不含尾部 NUL），*pnBytes 给长度。
char*  JvWideToAnsi(const WCHAR* pW, UINT uCP, INT* pnBytes);
INT    JvWideLen(const WCHAR* pW);              // 宽字符数（不含终止符）
INT    JvWideCmp(const WCHAR* a, const WCHAR* b);        // lstrcmpW 语义
// double→文本（对齐易语言 到文本(双精度小数型)：15 位有效数字，去尾零，必要时科学计数）
void   JvDoubleToText(double d, WCHAR* pOut, INT nCapChars);
void   JvInt64ToText(__int64 l, WCHAR* pOut, INT nCapChars);
void   JvIntToText(INT i, WCHAR* pOut, INT nCapChars);
// 文本→数值（对齐原版：到数值 / _wtoi64 / StrToInt）
// 长整数 ↔ 双精度：静态编译走 VC6 链接器，MSVC 的 __ltod3/__dtol3 辅助函数它没有，
// 所以这两个转换必须自己写（double→int32 是内联 cvttsd2si，安全）。
double    JvI64ToDouble(__int64 l);
__int64   JvDoubleToI64(double d);

double   JvTextToDouble(const WCHAR* p);
__int64  JvTextToI64(const WCHAR* p);
INT      JvTextToInt(const WCHAR* p);
// 转义：解析时原地解码，输出时展开到 pOut。
void   JvTranslationIn(WCHAR* pBuf, INT bTransU);
void   JvTranslationOut(const WCHAR* pSrc, UniWBuf* pOut);

BOOL   JvIsNumberToken(const WCHAR* p, INT nLen, INT* pbIsDouble);
INT    JvTokenType(const WCHAR* pValue, INT nValueLen);   // 返回 CXJV_NULL/BOOL/DOUBLE/LONG/UNDEFINED

// ============================================================ 命令实现声明
#define JSON_CMD(n) void n (PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)

#include <cx_json2_table.h>

}  // namespace cx

#endif  // CX_JSON_H
