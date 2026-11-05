// HTTP访问 族：文本/Cookie/协议头 工具箱的对外声明。
// 实现体在 http访问\cxhttp_util.inc；声明都放在 namespace cx 内（lib.cpp 里 include）。
// 这些函数是原模块 通用公开函数/通用模块/WinHTTP专用 里那些「公开子程序」的 C++ 等价物，
// 语义与边界一律以 http访问\规格_全局命令.md 为准（那份规格是从源码逐句抄出来的）。
//
// 编码口径（全局约定，别自创）：
//  · 易语言 文本型 = GBK 单字节串（LPCSTR），库内所有文本入参/出参都按 GBK 处理；
//  · 只有 完整网址 这一路原模块要求 UTF-8（访问网页_* 的 完整网址 参数），
//    需要时在本工具集内显式转换，调用方不感知；
//  · 所有出参文本一律用 SetRetText(pRetData, sz)（内部 NotifySys NRS_MALLOC）；
//    内部拼接缓冲用本库的 CxBuf（BufInit/BufAppend/BufReset/BufFree）。
#ifndef CXHTTP_UTIL_H
#define CXHTTP_UTIL_H

// ------------------------------------------------------------ 文本原子
// 这些是原模块私有工具子程序的等价实现，供上面各命令与三个访问类共用。
void UxAryJoin(CxBuf* pOut, LPCSTR szSep, LPCSTR const* pItems, int nCount);
void UxReplaceAll(CxBuf* pOut, LPCSTR szSrc, LPCSTR szFrom, LPCSTR szTo);        // 子文本替换(全, 区分大小写)
void UxReplaceAllNoCase(CxBuf* pOut, LPCSTR szSrc, LPCSTR szFrom, LPCSTR szTo);  // 忽略大小写
void UxLower(CxBuf* pOut, LPCSTR sz);           // 到小写（ASCII 字节）
void UxUpper(CxBuf* pOut, LPCSTR sz);           // 到大写
void UxTrim(CxBuf* pOut, LPCSTR sz);            // 去首尾空白
void UxNoSpace(CxBuf* pOut, LPCSTR sz);         // 删全部空：半角空格/制表/CR/LF + 全角空格 A1A1（GBK 双字节成对识别）
BOOL UxContains(LPCSTR szHay, LPCSTR szNeed);   // 文本是否存在（区分大小写）
void UxRightOf(CxBuf* pOut, LPCSTR szSrc, LPCSTR szMark);        // 文本_取右边：最后一个 标记 之后
BOOL UxBetween(CxBuf* pOut, LPCSTR szSrc, LPCSTR szLeft, LPCSTR szRight);        // 文本_取出中间文本
void UxSplit(CxBuf* pOutItem, LPCSTR szSrc, char chSep, int nWant);              // 取第 nWant 段（0 起），写进 pOutItem
int  UxSplitCount(LPCSTR szSrc, char chSep);
void UxUrlEncode(CxBuf* pOut, LPCSTR szText, BOOL bKeepAlnum, BOOL bToUtf8);      // 编码_URL编码（规格 P1）
void UxGbkToUtf8(CxBuf* pOut, LPCSTR szGbk);
void UxUtf8ToGbk(CxBuf* pOut, LPCSTR szUtf8);
int  UxCountOccur(LPCSTR szSrc, LPCSTR szSub, BOOL bCase);   // 文本_取出现次数（bCase=真 区分大小写）
void UxRandChars(CxBuf* pOut, int nLen);                                         // 文本_取随机字符 A-Za-z0-9

// ------------------------------------------------------------ 网址拆解
// 网页_取域名：去协议头、去路径、去端口；异常输入返回空串。
void UxHostOf(CxBuf* pOut, LPCSTR szUrl);
// 网页_取页面地址：? 之前的部分（含协议与域名）。
void UxPageOf(CxBuf* pOut, LPCSTR szUrl);
// 网页_取端口：http=80 https=443，显式端口优先。
int  UxPortOf(LPCSTR szUrl);

// ------------------------------------------------------------ 协议头
// 网页_构建协议头(附加协议头, 完整网址, 提交的COOKIE, 是否POST, 是否采用Gzip, 提交数据长度)
// 规则、默认值、行分隔（CRLF）、返回编码一律照 规格_全局命令.md 第 7 节。
void UxBuildHeaders(CxBuf* pOut, LPCSTR szExtra, LPCSTR szUrl, LPCSTR szCookie,
    BOOL bPost, BOOL bGzip, int nPostLen);
// 网页_提取头(头文本, 字段名) —— 返回字段值，找不到返回空串（规格 第 6 节）。
void UxPickHeader(CxBuf* pOut, LPCSTR szHead, LPCSTR szName);
// 判断响应头是否有效 / 取响应头代码 / 网页_取返回协议代码（规格 9~11 节）。
BOOL UxHeadIsOk(LPCSTR szHead);
void UxHeadCodeText(CxBuf* pOut, LPCSTR szHead);      // 原「取响应头代码」返回的是文本（声明写的是逻辑型，属源码 bug，见规格）
void UxStatusCode(CxBuf* pOut, LPCSTR szHead);        // 网页_取返回协议代码：状态行里的三位数

// ------------------------------------------------------------ Cookie
// 网页_提取返回头COOKIE(返回头) —— 把响应头里所有 Set-Cookie 收成 a=b; c=d
void UxCookieFromHead(CxBuf* pOut, LPCSTR szHead);
// 网页_Cookie合并更新(旧, 新) —— 同名新值赢、剔除 deleted、去重；结果写进 pOut
void UxCookieMerge(CxBuf* pOut, LPCSTR szOld, LPCSTR szNew);
// 网页_取单条Cookie(_cookie串, 名称) —— 从左往右取第一条
void UxCookieOne(CxBuf* pOut, LPCSTR szCookie, LPCSTR szName);
// 分析返回头COOKIES / 网页_取Cookie_内部（走 wininet 的 InternetGetCookieA / InternetGetCookieExA，flag 8192 取 HttpOnly）
void UxCookieFromUrl(CxBuf* pOut, LPCSTR szUrlHost, LPCSTR szName, BOOL bHttps);

// ------------------------------------------------------------ 其它
// 网页_取常见ContentType(文件名或扩展名) —— 34 项映射表，规格 第 13 节
void UxContentType(CxBuf* pOut, LPCSTR szNameOrExt);
// 网页_构建提交表单数据(名称数组, 值数组, 文件数组, 边界输出) —— 规格 第 12 节
void UxBuildForm(CxBuf* pOut, LPCSTR* pNames, LPCSTR* pValues, LPCSTR* pFiles, int nCount);
// 网页_取系统代理EX —— 注册表读法，返回 ip:port 或空串（规格 第 8 节）
void UxSystemProxy(CxBuf* pOut);
// 网页_GZIP解压(数据, 长度) —— 返回 malloc 的字节集指针与长度；失败返回 NULL
LPBYTE UxGzipInflate(const void* pData, int nLen, int* pOutLen);
// CURL取错误文本(码) —— curl_easy_strerror
void UxCurlErrorText(CxBuf* pOut, int nCode);
// 压缩亚马逊页面数据(字节集, 长度) —— 原模块的正文瘦身，找不到标记时按原样返回（规格 第 19 节）
LPBYTE UxCompressAmazonPage(const void* pData, int nLen, int* pOutLen);

#endif  // CXHTTP_UTIL_H
