// WebView2 按名反射层的契约。reflect_gen.cpp 由 gen_reflect.mjs 生成并实现这些函数。
#pragma once

#include "wv_pre.h"

// 返回值：区分「成员不存在」和「存在但调用失败」，前者要提示可用名，后者给 HRESULT 语义。
#define RF_ERR       0
#define RF_OK        1
#define RF_MISS      2       // 这个名字在本组里没有
#define RF_OK_TEXT   3       // 读到了文本，已写入 outText

#ifndef UNREF
#define UNREF(x) (void)(x)
#endif

namespace wv {

// 目标组编号（与 reflect_gen.cpp 里的函数后缀一一对应）
enum WvReflTarget
{
    RF_T_CORE = 0,      // ICoreWebView2 全链
    RF_T_SETTINGS,      // ICoreWebView2Settings 全链
    RF_T_CONTROLLER,    // ICoreWebView2Controller 全链
    RF_T_ENV,           // ICoreWebView2Environment 全链
    RF_T_OPTIONS,       // ICoreWebView2EnvironmentOptions
    RF_T_COOKIEMGR,     // ICoreWebView2CookieManager 全链
    RF_T_COOKIE,        // ICoreWebView2Cookie
    RF_T_PROFILE,       // ICoreWebView2Profile 全链
    RF_T_FRAME,         // ICoreWebView2Frame 全链（iframe）
    RF_T_DOWNLOAD,      // ICoreWebView2DownloadOperation 全链
    RF_T_REQUEST,       // ICoreWebView2WebResourceRequest
    RF_T_RESPONSE,      // ICoreWebView2WebResourceResponse 全链
    RF_T_PROCESSINFO,   // ICoreWebView2ProcessInfo 全链
    RF_T_RECEIVER,      // ICoreWebView2DevToolsProtocolEventReceiver
    RF_T_CERT,          // ICoreWebView2ClientCertificate
    RF_T_EVENT,         // 最近一次事件带来的对象（帧/下载/请求…）
    RF_T_COUNT
};

// 反射层里的 pObj 一律是该组「最新可用版本」的接口指针，内部再按需 QueryInterface 到声明接口。
#define WV_REFLECT_GROUP_DECL(g)                                                              \
    int  ReflGet_##g(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap); \
    int  ReflSet_##g(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText);  \
    int  ReflCall_##g(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText); \
    void ReflNames_##g(char* buf, INT cap);

WV_REFLECT_GROUP_DECL(CORE)
WV_REFLECT_GROUP_DECL(SETTINGS)
WV_REFLECT_GROUP_DECL(CONTROLLER)
WV_REFLECT_GROUP_DECL(ENV)
WV_REFLECT_GROUP_DECL(OPTIONS)
WV_REFLECT_GROUP_DECL(COOKIEMGR)
WV_REFLECT_GROUP_DECL(COOKIE)
WV_REFLECT_GROUP_DECL(PROFILE)
WV_REFLECT_GROUP_DECL(FRAME)
WV_REFLECT_GROUP_DECL(DOWNLOAD)
WV_REFLECT_GROUP_DECL(REQUEST)
WV_REFLECT_GROUP_DECL(RESPONSE)
WV_REFLECT_GROUP_DECL(PROCESSINFO)
WV_REFLECT_GROUP_DECL(RECEIVER)
WV_REFLECT_GROUP_DECL(CERT)

// 把名字数组拼成换行分隔文本，写进 buf（截断安全）
void ReflJoinNames(char* buf, INT cap, const char** items, INT count);

// 组名解析：支持中文与英文别名，返回 FALSE 表示未知组
BOOL ReflTargetFromName(LPCSTR szName, WvReflTarget* pOut);
LPCSTR ReflTargetName(WvReflTarget t);

// 不确定对象属于哪一组时（比如事件带出来的帧/下载/请求对象）逐组试，第一个非 RF_MISS 即命中。
int  ReflDispatchAny(IUnknown* pObj, INT mode, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText,
                     INT* outInt, DOUBLE* outDouble, LPSTR outText, INT cap, WvReflTarget* pHitGroup);

// 单组派发（实现按组名分派到上面生成的函数；mode 0 读 1 写 2 调用）
int  ReflDispatch(WvReflTarget t, IUnknown* pObj, INT mode, LPCSTR name,
                  INT inInt, DOUBLE inDouble, LPCSTR inText,
                  INT* outInt, DOUBLE* outDouble, LPSTR outText, INT cap);

}  // namespace wv
