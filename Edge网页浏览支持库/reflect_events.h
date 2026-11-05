// 事件反射层对外契约：按「组名 + 事件名」订阅 WebView2 事件，回调带参数 JSON 快照。
// 实现由 gen_events.mjs 生成（reflect_events.cpp），勿手改。
#pragma once

namespace wv {

#ifndef UNREF
#define UNREF(x) (void)(x)
#endif

#define WV_EVT_JSON_MAX  (24 * 1024)

// szJson 为参数快照（无参数事件为空串）；pArgs 是原始事件参数对象，仅在本线程内使用，
// 需要跨回调保存必须自己 AddRef（应答类命令就是这么做的）。
typedef void (*WvEventFn)(void* pCtx, const char* szEventName, const char* szJson, void* pArgs);

BOOL WvEvtSubscribe(IUnknown* pTarget, LPCSTR group, LPCSTR evtName, void* pCtx, WvEventFn fn, INT64* pToken);
BOOL WvEvtUnsubscribe(IUnknown* pTarget, LPCSTR group, LPCSTR evtName, INT64 token);
void WvEvtNames(LPCSTR group, char* buf, INT cap);
INT  WvEvtGroupCount();
LPCSTR WvEvtGroupName(INT i);

// 按接口名给任意对象做 JSON 快照（列表 / 配置文件 / 下载对象…），pObj 不AddRef，调用期间保持有效。
BOOL WvDescribeObject(LPCSTR ifaceName, void* pObj, char* buf, INT cap);

// ---- 事件参数对象化：按属性名逐个取，能顺着子对象与列表往下钻 ----
// 返回值：0 = 没有这个属性 / 取不到；1 = 取到数值（outInt / pDbl）；2 = 取到文本（outText）。
BOOL WvObjKnown(LPCSTR ifaceName);
INT  WvObjPropGet(LPCSTR ifaceName, void* p, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap);
// 子对象：返回 1 表示取到，*ppChild 带一份新引用（调用方 Release），*ppIface 是它的接口名。
INT  WvObjPropChild(LPCSTR ifaceName, void* p, LPCSTR name, void** ppChild, LPCSTR* ppIface);
INT  WvObjPropChildAt(LPCSTR ifaceName, void* p, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface);
INT  WvObjListCount(LPCSTR ifaceName, void* p, LPCSTR name, INT* pCount);
void WvObjPropNames(LPCSTR ifaceName, char* buf, INT cap);

// 写属性（应答）与异步延后：返回 1 表示成功；延后对象带一份新引用，由调用方持有的对象表负责放。
INT  WvObjPropSet(LPCSTR ifaceName, void* p, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText);
INT  WvObjDeferral(LPCSTR ifaceName, void* p, void** ppOut);
// 事件名 -> 参数对象的接口名；该事件没有参数对象时回 NULL。
LPCSTR WvEvtArgsIface(LPCSTR evtName);

}  // namespace wv
