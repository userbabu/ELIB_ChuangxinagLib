// 宽文本（UNICODE 模块）原生实现层的共享声明。
// 约定：字节集型宽文本 = UTF-16LE 数据，惯例末尾带 {0,0}；文本型一律按 UTF-8 进出，不做 GBK 转换。
#ifndef CX_UNI_H
#define CX_UNI_H

#include <cx_pch.h>

// 库定义数据类型序号（m_pDataType 下标 + 1）；1-3 是网络请求三类，4 起给宽文本与正则。
#define CX_TYPE_WSTR   4   // 宽文本类（原模块同名）
#define CX_TYPE_RXW    5   // 类_正则表达式类W
#define CX_TYPE_RXCW   6   // 类_正则表达式_兼容版W
#define CX_TYPE_RXC2   7   // 类_正则表达式_兼容版2
#define CX_TYPE_RXSW   8   // 类_正则表达式_简单版W

namespace cx {

// ============================================================ 运行期系统函数
// 静态编译时易语言用 VC6 链接器与 VC6 导入库，shlwapi/LCMapStringEx/ole32 都可能取不到符号，
// 所以这些入口全部运行期解析，.fne 与 _static.lib 都不新增导入依赖。

typedef int UniStrToIntFlag;          // STIF_DEFAULT=0 STIF_SUPPORT_HEX=1
#define UNI_STIF_DEFAULT 0
#define UNI_STIF_SUPPORT_HEX 1

struct UniApi
{
    bool bReady;
    // shlwapi
    const WCHAR* (WINAPI *StrStrW)(const WCHAR*, const WCHAR*);
    const WCHAR* (WINAPI *StrStrIW)(const WCHAR*, const WCHAR*);
    const WCHAR* (WINAPI *StrRStrW)(const WCHAR*, const WCHAR*, const WCHAR*);
    const WCHAR* (WINAPI *StrRStrIW)(const WCHAR*, const WCHAR*, const WCHAR*);
    BOOL (WINAPI *StrTrimW)(WCHAR*, const WCHAR*);
    BOOL (WINAPI *StrToIntExW)(const WCHAR*, UniStrToIntFlag, INT*);
    int (WINAPI *wvsprintfW)(WCHAR*, const WCHAR*, const void*);
    // kernel32（Vista+，取不到就用 LCMapStringW 兜底）
    int (WINAPI *LCMapStringEx)(const WCHAR*, DWORD, const WCHAR*, int, WCHAR*, int, void*, void*, int);
    // ole32 / oleaut32
    HRESULT (WINAPI *CoCreateInstance)(const void*, void*, DWORD, const void*, void**);
    void* (WINAPI *SysAllocString)(const WCHAR*);
    void (WINAPI *SysFreeString)(void*);
    HRESULT (WINAPI *VariantClear)(void*);
    HRESULT (WINAPI *VarBstrFromR8)(double, unsigned long, unsigned int, void**);   // 数值转宽文本，不走 CRT
    HRESULT (WINAPI *CoInitialize)(void*);
    void (WINAPI *CoUninitialize)(void);
};

const UniApi& UniGetApi();

// ============================================================ 参数读取

bool UniIsNull(const MDATA_INF& arg);
INT  UniArgInt(const MDATA_INF& arg, INT nDefault);
BOOL UniArgBool(const MDATA_INF& arg, BOOL bDefault);
LPCSTR UniArgText(const MDATA_INF& arg);        // UTF-8，可能为 NULL（参数留空）
LPBYTE UniArgBinPtr(const MDATA_INF& arg, INT* pnBytes);  // 字节集数据指针与字节长度

// 字节集 → 以 NUL 结束的宽文本视图；原数据不足时复制到内部临时缓冲。
struct UniWView
{
    const WCHAR* p;      // 一定以 {0,0} 结束
    INT nChars;          // 到第一个 {0,0} 为止的字符数
    WCHAR* pTmp;
    const WCHAR* pRaw;   // 补齐末尾 {0,0} 之后的原始区域
    INT nRawChars;       // 原始区域可安全访问的字符数（含终止符）
    INT nOrigBytes;      // 传入字节集本身的字节数

    UniWView() : p((const WCHAR*)L""), nChars(0), pTmp(NULL),
        pRaw((const WCHAR*)L""), nRawChars(1), nOrigBytes(0) {}
    ~UniWView();
    void Set(LPBYTE pData, INT nBytes);
    void FromBin(const MDATA_INF& arg);
    void FromUtf8(LPCSTR szUtf8);
    void FromPtr(INT ptr, INT nChars);          // nChars<=0 时按 lstrlenW 计算
    INT Capacity() const { return nChars + 1; }
};

// ============================================================ 返回值写入

void UniRetWideBin(PMDATA_INF pRet, const WCHAR* pW, INT nChars);   // 末尾带 {0,0}
void UniRetWideBinNoNull(PMDATA_INF pRet, const WCHAR* pW, INT nChars);
void UniRetRawBin(PMDATA_INF pRet, LPCSTR pData, INT nBytes);
void UniRetWideText(PMDATA_INF pRet, const WCHAR* pW, INT nChars);  // 转 UTF-8 文本
void UniRetBool(PMDATA_INF pRet, BOOL b);

WCHAR* UniUtf8ToWide(LPCSTR szUtf8, INT* pnChars);   // LocalAlloc，调用者 LocalFree
char*  UniWideToUtf8(const WCHAR* pW, INT nChars);   // LocalAlloc，调用者 LocalFree

// 宽文本增长缓冲（结果拼装用，LocalAlloc 所有，调用者 Free）。
struct UniWBuf
{
    WCHAR* p;
    INT nLen;      // 字符数
    INT nCap;      // 字符容量

    UniWBuf() : p(NULL), nLen(0), nCap(0) {}
    ~UniWBuf();
    bool Reserve(INT nAddChars);
    bool Add(const WCHAR* s, INT nChars);
    bool AddChar(WCHAR c);
    void Clear();
    void Steal(UniWBuf& other);       // 接管对方缓冲，避免浅拷贝造成二次释放
};

// 全角/半角映射；返回 LocalAlloc 缓冲，*pnOut 为字符数，失败返回 NULL。
WCHAR* UniMapWidth(const WCHAR* pSrc, INT nChars, DWORD dwMap, INT* pnOut);

// 宽文本查找/替换基础函数（位置一律从 1 开始，失败返回 -1）。
INT  UniFindWide(const WCHAR* pSrc, INT nSrcChars, const WCHAR* pFind, INT nPos1, BOOL bCaseInsensitive);
INT  UniRFindWide(const WCHAR* pSrc, INT nSrcChars, const WCHAR* pFind, INT nPos1, BOOL bCaseInsensitive);
// 子文本替换：nCount=0 全部；bCaseSensitive 真为区分大小写。找不到返回假且不写 pOut。
bool UniSubReplaceWide(const WCHAR* pSrc, INT nSrcChars, const WCHAR* pOld, const WCHAR* pNew,
                       INT nPos1, INT nCount, BOOL bCaseSensitive, UniWBuf* pOut);
// WC 语义：删除所有中间的 {0,0} 后补终止符，直接作为 字节集 返回。
void UniRetWC(PMDATA_INF pRet, const WCHAR* p, INT nChars);

// ============================================================ 参考数组参数
// 元素指针列表：文本/字节集数组的元素本身就是指针，对象数组的元素是指向成员数据的指针。
struct UniAryList
{
    void** pItem;
    INT nCount;
    INT nCap;

    UniAryList() : pItem(NULL), nCount(0), nCap(0) {}
    ~UniAryList();
    void Add(void* pElement);
};

// 把已有数组内容按元素类型克隆进列表（克隆体归易语言内存池所有，可安全跨 NRS_FREE_ARY 存活）。
void UniAryCopyFrom(const MDATA_INF& arg, UniAryList* pList);
// 读数组数据（bByRef：命令参数声明为 参考 数组 时用 m_ppAryData，否则用 m_pAryData）。
LPBYTE UniAryRead(const MDATA_INF& arg, INT bByRef, INT* pnCount);
// 释放数组变量原内容并写回新列表。
void UniAryWrite(const MDATA_INF& arg, UniAryList* pList);
// 写回数组：bAppend 为真时保留原成员（原版 加入成员 的追加语义）。
void UniAryAppend(const MDATA_INF& arg, UniAryList* pList, INT bAppend);
// 命令直接返回数组数据（命令表带 CT_RETRUN_ARY_TYPE_DATA）。
void UniRetAryData(PMDATA_INF pRet, UniAryList* pList);
// 空数组（长度为 0）也要能正确写回。
INT UniAryCount(const MDATA_INF& arg);

// ============================================================ 宽文本对象（对应模块里的 zyString）

struct CxWStr
{
    WCHAR* pStr;
    INT nLen;      // 字符数
    INT nCap;      // 字节容量
};

CxWStr* WsNew(INT nInitCapChars);
CxWStr* WsClone(const CxWStr* pSrc);
void WsFree(CxWStr* p);
void WsClear(CxWStr* p);
bool WsReserve(CxWStr* p, INT nAddChars);
bool WsAppend(CxWStr* p, const WCHAR* s, INT nChars);
bool WsAppendLine(CxWStr* p, const WCHAR* s, INT nChars);
bool WsAppendInt(CxWStr* p, INT n);
bool WsInsert(CxWStr* p, INT nPos, const WCHAR* s, INT nChars);
bool WsReplaceAt(CxWStr* p, INT nPos, INT nLen, const WCHAR* s, INT nNewChars);
bool WsSubReplace(CxWStr* p, const WCHAR* pOld, const WCHAR* pNew, INT nPos, INT nCount, BOOL bCaseInsensitive);
bool WsTrimChars(CxWStr* p, const WCHAR* pTrim);
INT  WsFind(const CxWStr* p, const WCHAR* s, INT nPos, BOOL bCaseInsensitive);
INT  WsRFind(const CxWStr* p, const WCHAR* s, INT nPos, BOOL bCaseInsensitive);
void WsToUpper(CxWStr* p);
void WsToLower(CxWStr* p);
void WsToFullWidth(CxWStr* p);
void WsToHalfWidth(CxWStr* p);
INT  WsToInt(const CxWStr* p, BOOL bHex);
// 分割结果：源宽文本里的若干连续片段（起止以字符计），由调用方编译成字节集/文本/对象元素。
struct UniWRangeList
{
    INT* pStart;
    INT* pChars;
    INT nCount;
    INT nCap;

    UniWRangeList() : pStart(NULL), pChars(NULL), nCount(0), nCap(0) {}
    ~UniWRangeList();
    void Add(INT nStart, INT nChars);
};

void WsSplit(const CxWStr* p, const WCHAR* pSep, INT nSepChars, BOOL bRemoveEmpty, UniWRangeList* pOut);
CxWStr* WsSub(const CxWStr* p, INT nStart, INT nLen);
INT  WsCompare(const CxWStr* p, const WCHAR* pOther);

// ============================================================ 正则表达式（VBScript.RegExp 的 IDispatch 封装）

struct CxRegex
{
    void* pReg;            // IDispatch (VBScript.RegExp)
    void* pMatches;        // IDispatch (Execute 返回的 Match 集合)
    WCHAR* pSubject;       // 被搜索文本副本（含 {0,0}）
    INT nSubjectChars;
    BOOL bIgnoreCase;
    BOOL bComInited;       // 本对象是否成功初始化过 COM，保证 CoUninitialize 成对
    WCHAR* pTmpVal;        // 最近一次取值的宽文本缓冲
    INT nTmpValChars;
};

bool RxNew(CxRegex* p, const WCHAR* pPattern, const WCHAR* pSubject, INT nSubjectChars,
           BOOL bIgnoreCase, BOOL bMultiline, BOOL bSingleline, BOOL bGlobal);
bool RxExecute(CxRegex* p, const WCHAR* pSubject, INT nChars);
void RxCopyObject(CxRegex* pDst, const CxRegex* pSrc);
void RxFree(CxRegex* p);
void RxClear(CxRegex* p);
INT  RxMatchCount(CxRegex* p);
INT  RxSubCount(CxRegex* p);                    // 第 1 个匹配的子表达式个数
INT  RxSubCountAt(CxRegex* p, INT nMatchIndex);
const WCHAR* RxMatchValue(CxRegex* p, INT nMatchIndex);   // nMatchIndex 从 1 开始
const WCHAR* RxSubValue(CxRegex* p, INT nMatchIndex, INT nSubIndex);
INT  RxMatchFirstIndex(CxRegex* p, INT nMatchIndex);
// 原版 起始位置 写回值：寻找文本W(被搜索文本, 值, FirstIndex, 与 IgnoreCase 取反) - 1
INT  RxStartPosAt(CxRegex* p, INT nMatchIndex, const WCHAR* pVal);
WCHAR* RxReplace(CxRegex* p, const WCHAR* pRepl, INT* pnChars);  // LocalAlloc，调用者释放
bool RxTest(CxRegex* p, const WCHAR* pText);
void RxGetPattern(CxRegex* p, WCHAR** ppOut, INT* pnChars);      // LocalAlloc，调用者释放

// ============================================================ 命令实现声明

#define UNI_CMD(fn) void fn(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf);

// ---- 文本处理模块（55，弃用的 寻找文本W_旧版 未迁移）
UNI_CMD(UwTextLen) UNI_CMD(UwLeft) UNI_CMD(UwRight) UNI_CMD(UwMid) UNI_CMD(UwCharW) UNI_CMD(UwGetCode)
UNI_CMD(UwFind) UNI_CMD(UwRFind) UNI_CMD(UwUpper) UNI_CMD(UwLower)
UNI_CMD(UwFullWidth) UNI_CMD(UwHalfWidth) UNI_CMD(UwCat) UNI_CMD(UwCompare) UNI_CMD(UwTrim)
UNI_CMD(UwTrimSpace) UNI_CMD(UwReplace) UNI_CMD(UwSubReplace) UNI_CMD(UwSubReplace2)
UNI_CMD(UwBlankText) UNI_CMD(UwRepeatText) UNI_CMD(UwSplit2) UNI_CMD(UwSplit) UNI_CMD(UwSplitAry)
UNI_CMD(UwPtrToText) UNI_CMD(UwTextPtr) UNI_CMD(UwTextToInt) UNI_CMD(UwIntToText)
UNI_CMD(UwReadFile) UNI_CMD(UwWriteFile)
UNI_CMD(UwMidBetween) UNI_CMD(UwMidBetween2) UNI_CMD(UwRMidBetween) UNI_CMD(UwRMidBetween2)
UNI_CMD(UwTextBefore) UNI_CMD(UwTextBefore2) UNI_CMD(UwTextAfter) UNI_CMD(UwTextAfter2)
UNI_CMD(UwTextExists) UNI_CMD(UwRegMatch) UNI_CMD(UwRegMatchAllSub) UNI_CMD(UwRegMatch2)
UNI_CMD(UwPickDigits) UNI_CMD(UwRegReplace) UNI_CMD(UwRegSubReplace) UNI_CMD(UwAryJoin)
UNI_CMD(UwMidBatch) UNI_CMD(UwMidBatch3) UNI_CMD(UwCountOccur) UNI_CMD(UwMidBatch2)
UNI_CMD(UwCountOccur2) UNI_CMD(UwHtmlMid) UNI_CMD(UwAryShuffle) UNI_CMD(UwAryJoinNull)
UNI_CMD(UwSubReplaceAry) UNI_CMD(UwAryUnique)

// ---- 扩展模块（19）
UNI_CMD(UexFormatW) UNI_CMD(UexFormatW2) UNI_CMD(UexVarAddr) UNI_CMD(UexThisCall)
UNI_CMD(UexStructPtr) UNI_CMD(UexWTextFix) UNI_CMD(UexWF) UNI_CMD(UexWC)
UNI_CMD(UexWTextIsEmpty) UNI_CMD(UexWIsNull) UNI_CMD(UexW_ISNULL) UNI_CMD(UexWTextEmpty)
UNI_CMD(UexWToInt) UNI_CMD(UexWToDouble) UNI_CMD(UexDoubleToW) UNI_CMD(UexBinBlank)
UNI_CMD(UexWTextEqual) UNI_CMD(UexCoFree) UNI_CMD(UexCoInit)

// ---- 宽文本类（57 + 3）
UNI_CMD(WsConstruct) UNI_CMD(WsCopy) UNI_CMD(WsDestruct)
UNI_CMD(WsGetPtr) UNI_CMD(WsSetPtr) UNI_CMD(WsCreate) UNI_CMD(WsAddRes) UNI_CMD(WsAddPtrW)
UNI_CMD(WsAddPtrA) UNI_CMD(WsAdd) UNI_CMD(WsAddW) UNI_CMD(WsAddSelf) UNI_CMD(WsAddLine)
UNI_CMD(WsAddLineW) UNI_CMD(WsAddLineSelf) UNI_CMD(WsAddInt) UNI_CMD(WsInsertText)
UNI_CMD(WsInsertW) UNI_CMD(WsInsertSelf) UNI_CMD(WsGetText) UNI_CMD(WsGetTextW)
UNI_CMD(WsGetTextWN) UNI_CMD(WsGetStrPtr) UNI_CMD(WsCmdClear) UNI_CMD(WsCmdDestroy)
UNI_CMD(WsAddChar) UNI_CMD(WsGetChar) UNI_CMD(WsSetChar) UNI_CMD(WsCmdCompare)
UNI_CMD(WsCmdCompareW) UNI_CMD(WsCmdCompareSelf) UNI_CMD(WsCmdLength) UNI_CMD(WsCmdClone)
UNI_CMD(WsCmdUpper) UNI_CMD(WsCmdLower) UNI_CMD(WsCmdFull) UNI_CMD(WsCmdHalf)
UNI_CMD(WsCmdFindW) UNI_CMD(WsCmdFind) UNI_CMD(WsCmdFindSelf) UNI_CMD(WsCmdRFindW)
UNI_CMD(WsCmdRFind) UNI_CMD(WsCmdRFindSelf) UNI_CMD(WsCmdReplaceW) UNI_CMD(WsCmdReplace)
UNI_CMD(WsCmdReplaceSelf) UNI_CMD(WsCmdSubW) UNI_CMD(WsCmdSub) UNI_CMD(WsCmdSubSelf)
UNI_CMD(WsCmdMid) UNI_CMD(WsCmdLeft) UNI_CMD(WsCmdRight) UNI_CMD(WsCmdTrimW)
UNI_CMD(WsCmdTrim) UNI_CMD(WsCmdTrimSelf) UNI_CMD(WsCmdTrimSpace) UNI_CMD(WsCmdToInt)
UNI_CMD(WsCmdSplitSelf) UNI_CMD(WsCmdSplitW) UNI_CMD(WsCmdSplit)

// ---- 正则表达式W（类_正则表达式类W，8 + 3）
UNI_CMD(RxConstruct) UNI_CMD(RxCopy) UNI_CMD(RxDestruct)
UNI_CMD(RxCmdCreate) UNI_CMD(RxCmdView) UNI_CMD(RxCmdPattern) UNI_CMD(RxCmdReplace)
UNI_CMD(RxCmdCount) UNI_CMD(RxCmdValueW) UNI_CMD(RxCmdSubValueW) UNI_CMD(RxCmdSubCount)

// ---- 正则表达式兼容版W（11 + 3）
UNI_CMD(RxcConstruct) UNI_CMD(RxcCopy) UNI_CMD(RxcDestruct)
UNI_CMD(RxcCreate) UNI_CMD(RxcCreateW) UNI_CMD(RxcView) UNI_CMD(RxcPattern)
UNI_CMD(RxcReplace) UNI_CMD(RxcCount) UNI_CMD(RxcValue) UNI_CMD(RxcValueW)
UNI_CMD(RxcSubValue) UNI_CMD(RxcSubValueW) UNI_CMD(RxcSubCount)

// ---- 正则表达式兼容版2（10 + 3）
UNI_CMD(Rxc2Construct) UNI_CMD(Rxc2Copy) UNI_CMD(Rxc2Destruct)
UNI_CMD(Rxc2Create) UNI_CMD(Rxc2CreateW) UNI_CMD(Rxc2Pattern) UNI_CMD(Rxc2PatternW)
UNI_CMD(Rxc2Replace) UNI_CMD(Rxc2ReplaceW) UNI_CMD(Rxc2Count) UNI_CMD(Rxc2Value)
UNI_CMD(Rxc2SubValue) UNI_CMD(Rxc2SubCount)

// ---- 正则表达式简单版W（10 + 3）
UNI_CMD(RxsConstruct) UNI_CMD(RxsCopy) UNI_CMD(RxsDestruct)
UNI_CMD(RxsCreate) UNI_CMD(RxsMatch) UNI_CMD(RxsSearchAll) UNI_CMD(RxsValue)
UNI_CMD(RxsSubCount) UNI_CMD(RxsSubValue) UNI_CMD(RxsCreateW) UNI_CMD(RxsSearchAllW)
UNI_CMD(RxsValueW) UNI_CMD(RxsSubValueW)

// ---- 编码转换（迁移自 编码转换通用模块，20 条；原版标废弃/尽量不用的 3 条未迁移）
UNI_CMD(UcW) UNI_CMD(UcA) UNI_CMD(UcA2W) UNI_CMD(UcA2WNo) UNI_CMD(UcW2A)
UNI_CMD(UcW2Utf8) UNI_CMD(UcW2Utf8T) UNI_CMD(UcUtf82W) UNI_CMD(UcUtf82WT)
UNI_CMD(UcUtf82A) UNI_CMD(UcUtf82AT) UNI_CMD(UcA2Utf8) UNI_CMD(UcA2Utf8T)
UNI_CMD(UcUtf8ToGb) UNI_CMD(UcGbToUtf8) UNI_CMD(UcWideTrimNull)
UNI_CMD(UcUrlEncodeW) UNI_CMD(UcUrlDecode) UNI_CMD(UcUrlDecodeJs)
UNI_CMD(UcHtmlDecodeA)

}  // namespace cx

#endif  // CX_UNI_H
