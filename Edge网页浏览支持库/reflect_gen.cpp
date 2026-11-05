// 由 gen_reflect.mjs 从 WebView2.h 自动生成，勿手改。
// 每条显式 QueryInterface(声明接口) → 调用 → Release；签名由编译器校验，不靠 vtable 序号。
// 不能干净映射成 整数/逻辑/小数/文本 的成员见 uncovered.txt，由专用命令负责。

#include "reflect.h"
#include <string.h>

namespace wv {

static int ReflPutText(LPSTR outText, int textCap, LPCWSTR s)
{
    if (outText == NULL || textCap <= 0) return 0;
    outText[0] = '\0';
    if (s == NULL || s[0] == L'\0') return 1;
    return WideCharToMultiByte(CP_UTF8, 0, s, -1, outText, textCap, NULL, NULL) != 0;
}

// ==== CORE: 读 13 / 写 1 / 调 16 ====
int ReflGet_CORE(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{
  if (!_stricmp(name, "Source")) { ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_Source(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "BrowserProcessId")) { ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; UINT32 v{}; HRESULT h = o->get_BrowserProcessId(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "CanGoBack")) { ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_CanGoBack(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "CanGoForward")) { ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_CanGoForward(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "DocumentTitle")) { ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_DocumentTitle(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "ContainsFullScreenElement")) { ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_ContainsFullScreenElement(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsSuspended")) { ICoreWebView2_3* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2_3), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsSuspended(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsMuted")) { ICoreWebView2_8* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2_8), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsMuted(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsDocumentPlayingAudio")) { ICoreWebView2_8* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2_8), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsDocumentPlayingAudio(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsDefaultDownloadDialogOpen")) { ICoreWebView2_9* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2_9), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsDefaultDownloadDialogOpen(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "StatusBarText")) { ICoreWebView2_12* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2_12), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_StatusBarText(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "FaviconUri")) { ICoreWebView2_15* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2_15), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_FaviconUri(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "FrameId")) { ICoreWebView2_20* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2_20), (void**)&o))) return RF_ERR; UINT32 v{}; HRESULT h = o->get_FrameId(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap);
  return RF_MISS;
}

int ReflSet_CORE(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
  if (!_stricmp(name, "IsMuted")) { ICoreWebView2_8* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2_8), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsMuted(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

int ReflCall_CORE(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
  if (!_stricmp(name, "Navigate")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; HRESULT h = o->Navigate((LPCWSTR)wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "NavigateToString")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; HRESULT h = o->NavigateToString((LPCWSTR)wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "RemoveScriptToExecuteOnDocumentCreated")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; HRESULT h = o->RemoveScriptToExecuteOnDocumentCreated((LPCWSTR)wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "Reload")) { ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; HRESULT h = o->Reload(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "PostWebMessageAsJson")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; HRESULT h = o->PostWebMessageAsJson((LPCWSTR)wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "PostWebMessageAsString")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; HRESULT h = o->PostWebMessageAsString((LPCWSTR)wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "GoBack")) { ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; HRESULT h = o->GoBack(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "GoForward")) { ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; HRESULT h = o->GoForward(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "Stop")) { ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; HRESULT h = o->Stop(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "RemoveHostObjectFromScript")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; HRESULT h = o->RemoveHostObjectFromScript((LPCWSTR)wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "OpenDevToolsWindow")) { ICoreWebView2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return RF_ERR; HRESULT h = o->OpenDevToolsWindow(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "Resume")) { ICoreWebView2_3* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2_3), (void**)&o))) return RF_ERR; HRESULT h = o->Resume(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "ClearVirtualHostNameToFolderMapping")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2_3* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2_3), (void**)&o))) return RF_ERR; HRESULT h = o->ClearVirtualHostNameToFolderMapping((LPCWSTR)wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "OpenTaskManagerWindow")) { ICoreWebView2_6* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2_6), (void**)&o))) return RF_ERR; HRESULT h = o->OpenTaskManagerWindow(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "OpenDefaultDownloadDialog")) { ICoreWebView2_9* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2_9), (void**)&o))) return RF_ERR; HRESULT h = o->OpenDefaultDownloadDialog(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "CloseDefaultDownloadDialog")) { ICoreWebView2_9* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2_9), (void**)&o))) return RF_ERR; HRESULT h = o->CloseDefaultDownloadDialog(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

void ReflNames_CORE(char* buf, INT cap)
{
  static const char* items[] = {
    "Source",     "Navigate",     "NavigateToString",     "RemoveScriptToExecuteOnDocumentCreated",     "Reload",     "PostWebMessageAsJson",
    "PostWebMessageAsString",     "BrowserProcessId",     "CanGoBack",     "CanGoForward",     "GoBack",     "GoForward",
    "Stop",     "DocumentTitle",     "RemoveHostObjectFromScript",     "OpenDevToolsWindow",     "ContainsFullScreenElement",     "Resume",
    "IsSuspended",     "ClearVirtualHostNameToFolderMapping",     "OpenTaskManagerWindow",     "IsMuted",     "IsMuted",     "IsDocumentPlayingAudio",
    "IsDefaultDownloadDialogOpen",     "OpenDefaultDownloadDialog",     "CloseDefaultDownloadDialog",     "StatusBarText",     "FaviconUri",     "FrameId",

  };
  ReflJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

// ==== SETTINGS: 读 17 / 写 17 / 调 0 ====
int ReflGet_SETTINGS(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{
  if (!_stricmp(name, "IsScriptEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsScriptEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsWebMessageEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsWebMessageEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "AreDefaultScriptDialogsEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_AreDefaultScriptDialogsEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsStatusBarEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsStatusBarEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "AreDevToolsEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_AreDevToolsEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "AreDefaultContextMenusEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_AreDefaultContextMenusEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "AreHostObjectsAllowed")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_AreHostObjectsAllowed(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsZoomControlEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsZoomControlEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsBuiltInErrorPageEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsBuiltInErrorPageEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "UserAgent")) { ICoreWebView2Settings2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings2), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_UserAgent(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "AreBrowserAcceleratorKeysEnabled")) { ICoreWebView2Settings3* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings3), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_AreBrowserAcceleratorKeysEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsPasswordAutosaveEnabled")) { ICoreWebView2Settings4* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings4), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsPasswordAutosaveEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsGeneralAutofillEnabled")) { ICoreWebView2Settings4* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings4), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsGeneralAutofillEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsPinchZoomEnabled")) { ICoreWebView2Settings5* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings5), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsPinchZoomEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsSwipeNavigationEnabled")) { ICoreWebView2Settings6* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings6), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsSwipeNavigationEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsReputationCheckingRequired")) { ICoreWebView2Settings8* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings8), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsReputationCheckingRequired(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsNonClientRegionSupportEnabled")) { ICoreWebView2Settings9* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings9), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsNonClientRegionSupportEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap);
  return RF_MISS;
}

int ReflSet_SETTINGS(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
  if (!_stricmp(name, "IsScriptEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsScriptEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "IsWebMessageEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsWebMessageEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "AreDefaultScriptDialogsEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; HRESULT h = o->put_AreDefaultScriptDialogsEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "IsStatusBarEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsStatusBarEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "AreDevToolsEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; HRESULT h = o->put_AreDevToolsEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "AreDefaultContextMenusEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; HRESULT h = o->put_AreDefaultContextMenusEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "AreHostObjectsAllowed")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; HRESULT h = o->put_AreHostObjectsAllowed(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "IsZoomControlEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsZoomControlEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "IsBuiltInErrorPageEnabled")) { ICoreWebView2Settings* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsBuiltInErrorPageEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "UserAgent")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2Settings2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings2), (void**)&o))) return RF_ERR; HRESULT h = o->put_UserAgent(wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "AreBrowserAcceleratorKeysEnabled")) { ICoreWebView2Settings3* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings3), (void**)&o))) return RF_ERR; HRESULT h = o->put_AreBrowserAcceleratorKeysEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "IsPasswordAutosaveEnabled")) { ICoreWebView2Settings4* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings4), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsPasswordAutosaveEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "IsGeneralAutofillEnabled")) { ICoreWebView2Settings4* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings4), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsGeneralAutofillEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "IsPinchZoomEnabled")) { ICoreWebView2Settings5* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings5), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsPinchZoomEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "IsSwipeNavigationEnabled")) { ICoreWebView2Settings6* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings6), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsSwipeNavigationEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "IsReputationCheckingRequired")) { ICoreWebView2Settings8* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings8), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsReputationCheckingRequired(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "IsNonClientRegionSupportEnabled")) { ICoreWebView2Settings9* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Settings9), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsNonClientRegionSupportEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

int ReflCall_SETTINGS(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{

  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

void ReflNames_SETTINGS(char* buf, INT cap)
{
  static const char* items[] = {
    "IsScriptEnabled",     "IsScriptEnabled",     "IsWebMessageEnabled",     "IsWebMessageEnabled",     "AreDefaultScriptDialogsEnabled",     "AreDefaultScriptDialogsEnabled",
    "IsStatusBarEnabled",     "IsStatusBarEnabled",     "AreDevToolsEnabled",     "AreDevToolsEnabled",     "AreDefaultContextMenusEnabled",     "AreDefaultContextMenusEnabled",
    "AreHostObjectsAllowed",     "AreHostObjectsAllowed",     "IsZoomControlEnabled",     "IsZoomControlEnabled",     "IsBuiltInErrorPageEnabled",     "IsBuiltInErrorPageEnabled",
    "UserAgent",     "UserAgent",     "AreBrowserAcceleratorKeysEnabled",     "AreBrowserAcceleratorKeysEnabled",     "IsPasswordAutosaveEnabled",     "IsPasswordAutosaveEnabled",
    "IsGeneralAutofillEnabled",     "IsGeneralAutofillEnabled",     "IsPinchZoomEnabled",     "IsPinchZoomEnabled",     "IsSwipeNavigationEnabled",     "IsSwipeNavigationEnabled",
    "IsReputationCheckingRequired",     "IsReputationCheckingRequired",     "IsNonClientRegionSupportEnabled",     "IsNonClientRegionSupportEnabled", 
  };
  ReflJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

// ==== CONTROLLER: 读 5 / 写 5 / 调 2 ====
int ReflGet_CONTROLLER(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{
  if (!_stricmp(name, "IsVisible")) { ICoreWebView2Controller* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsVisible(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "ZoomFactor")) { ICoreWebView2Controller* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return RF_ERR; double v{}; HRESULT h = o->get_ZoomFactor(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = (double)v; return RF_OK; }
  if (!_stricmp(name, "RasterizationScale")) { ICoreWebView2Controller3* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Controller3), (void**)&o))) return RF_ERR; double v{}; HRESULT h = o->get_RasterizationScale(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = (double)v; return RF_OK; }
  if (!_stricmp(name, "ShouldDetectMonitorScaleChanges")) { ICoreWebView2Controller3* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Controller3), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_ShouldDetectMonitorScaleChanges(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "AllowExternalDrop")) { ICoreWebView2Controller4* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Controller4), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_AllowExternalDrop(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap);
  return RF_MISS;
}

int ReflSet_CONTROLLER(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
  if (!_stricmp(name, "IsVisible")) { ICoreWebView2Controller* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsVisible(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "ZoomFactor")) { ICoreWebView2Controller* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return RF_ERR; HRESULT h = o->put_ZoomFactor(inDouble); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "RasterizationScale")) { ICoreWebView2Controller3* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Controller3), (void**)&o))) return RF_ERR; HRESULT h = o->put_RasterizationScale(inDouble); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "ShouldDetectMonitorScaleChanges")) { ICoreWebView2Controller3* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Controller3), (void**)&o))) return RF_ERR; HRESULT h = o->put_ShouldDetectMonitorScaleChanges(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "AllowExternalDrop")) { ICoreWebView2Controller4* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Controller4), (void**)&o))) return RF_ERR; HRESULT h = o->put_AllowExternalDrop(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

int ReflCall_CONTROLLER(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
  if (!_stricmp(name, "NotifyParentWindowPositionChanged")) { ICoreWebView2Controller* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return RF_ERR; HRESULT h = o->NotifyParentWindowPositionChanged(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "Close")) { ICoreWebView2Controller* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return RF_ERR; HRESULT h = o->Close(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

void ReflNames_CONTROLLER(char* buf, INT cap)
{
  static const char* items[] = {
    "IsVisible",     "IsVisible",     "ZoomFactor",     "ZoomFactor",     "NotifyParentWindowPositionChanged",     "Close",
    "RasterizationScale",     "RasterizationScale",     "ShouldDetectMonitorScaleChanges",     "ShouldDetectMonitorScaleChanges",     "AllowExternalDrop",     "AllowExternalDrop",

  };
  ReflJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

// ==== ENV: 读 3 / 写 0 / 调 0 ====
int ReflGet_ENV(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{
  if (!_stricmp(name, "BrowserVersionString")) { ICoreWebView2Environment* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Environment), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_BrowserVersionString(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "UserDataFolder")) { ICoreWebView2Environment7* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Environment7), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_UserDataFolder(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "FailureReportFolderPath")) { ICoreWebView2Environment11* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Environment11), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_FailureReportFolderPath(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap);
  return RF_MISS;
}

int ReflSet_ENV(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{

  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

int ReflCall_ENV(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{

  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

void ReflNames_ENV(char* buf, INT cap)
{
  static const char* items[] = {
    "BrowserVersionString",     "UserDataFolder",     "FailureReportFolderPath", 
  };
  ReflJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

// ==== OPTIONS: 读 4 / 写 4 / 调 0 ====
int ReflGet_OPTIONS(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{
  if (!_stricmp(name, "AdditionalBrowserArguments")) { ICoreWebView2EnvironmentOptions* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2EnvironmentOptions), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_AdditionalBrowserArguments(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "Language")) { ICoreWebView2EnvironmentOptions* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2EnvironmentOptions), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_Language(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "TargetCompatibleBrowserVersion")) { ICoreWebView2EnvironmentOptions* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2EnvironmentOptions), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_TargetCompatibleBrowserVersion(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "AllowSingleSignOnUsingOSPrimaryAccount")) { ICoreWebView2EnvironmentOptions* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2EnvironmentOptions), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_AllowSingleSignOnUsingOSPrimaryAccount(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap);
  return RF_MISS;
}

int ReflSet_OPTIONS(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
  if (!_stricmp(name, "AdditionalBrowserArguments")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2EnvironmentOptions* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2EnvironmentOptions), (void**)&o))) return RF_ERR; HRESULT h = o->put_AdditionalBrowserArguments(wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "Language")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2EnvironmentOptions* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2EnvironmentOptions), (void**)&o))) return RF_ERR; HRESULT h = o->put_Language(wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "TargetCompatibleBrowserVersion")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2EnvironmentOptions* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2EnvironmentOptions), (void**)&o))) return RF_ERR; HRESULT h = o->put_TargetCompatibleBrowserVersion(wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "AllowSingleSignOnUsingOSPrimaryAccount")) { ICoreWebView2EnvironmentOptions* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2EnvironmentOptions), (void**)&o))) return RF_ERR; HRESULT h = o->put_AllowSingleSignOnUsingOSPrimaryAccount(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

int ReflCall_OPTIONS(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{

  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

void ReflNames_OPTIONS(char* buf, INT cap)
{
  static const char* items[] = {
    "AdditionalBrowserArguments",     "AdditionalBrowserArguments",     "Language",     "Language",     "TargetCompatibleBrowserVersion",     "TargetCompatibleBrowserVersion",
    "AllowSingleSignOnUsingOSPrimaryAccount",     "AllowSingleSignOnUsingOSPrimaryAccount", 
  };
  ReflJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

// ==== COOKIEMGR: 读 0 / 写 0 / 调 1 ====
int ReflGet_COOKIEMGR(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{

  UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap);
  return RF_MISS;
}

int ReflSet_COOKIEMGR(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{

  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

int ReflCall_COOKIEMGR(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
  if (!_stricmp(name, "DeleteAllCookies")) { ICoreWebView2CookieManager* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2CookieManager), (void**)&o))) return RF_ERR; HRESULT h = o->DeleteAllCookies(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

void ReflNames_COOKIEMGR(char* buf, INT cap)
{
  static const char* items[] = {
    "DeleteAllCookies", 
  };
  ReflJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

// ==== COOKIE: 读 8 / 写 4 / 调 0 ====
int ReflGet_COOKIE(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{
  if (!_stricmp(name, "Name")) { ICoreWebView2Cookie* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Cookie), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_Name(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "Value")) { ICoreWebView2Cookie* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Cookie), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_Value(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "Domain")) { ICoreWebView2Cookie* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Cookie), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_Domain(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "Path")) { ICoreWebView2Cookie* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Cookie), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_Path(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "Expires")) { ICoreWebView2Cookie* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Cookie), (void**)&o))) return RF_ERR; double v{}; HRESULT h = o->get_Expires(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = (double)v; return RF_OK; }
  if (!_stricmp(name, "IsHttpOnly")) { ICoreWebView2Cookie* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Cookie), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsHttpOnly(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsSecure")) { ICoreWebView2Cookie* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Cookie), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsSecure(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsSession")) { ICoreWebView2Cookie* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Cookie), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsSession(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap);
  return RF_MISS;
}

int ReflSet_COOKIE(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
  if (!_stricmp(name, "Value")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2Cookie* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Cookie), (void**)&o))) return RF_ERR; HRESULT h = o->put_Value(wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "Expires")) { ICoreWebView2Cookie* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Cookie), (void**)&o))) return RF_ERR; HRESULT h = o->put_Expires(inDouble); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "IsHttpOnly")) { ICoreWebView2Cookie* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Cookie), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsHttpOnly(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "IsSecure")) { ICoreWebView2Cookie* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Cookie), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsSecure(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

int ReflCall_COOKIE(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{

  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

void ReflNames_COOKIE(char* buf, INT cap)
{
  static const char* items[] = {
    "Name",     "Value",     "Value",     "Domain",     "Path",     "Expires",
    "Expires",     "IsHttpOnly",     "IsHttpOnly",     "IsSecure",     "IsSecure",     "IsSession",

  };
  ReflJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

// ==== PROFILE: 读 7 / 写 4 / 调 1 ====
int ReflGet_PROFILE(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{
  if (!_stricmp(name, "ProfileName")) { ICoreWebView2Profile* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Profile), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_ProfileName(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "IsInPrivateModeEnabled")) { ICoreWebView2Profile* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Profile), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsInPrivateModeEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "ProfilePath")) { ICoreWebView2Profile* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Profile), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_ProfilePath(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "DefaultDownloadFolderPath")) { ICoreWebView2Profile* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Profile), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_DefaultDownloadFolderPath(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "IsPasswordAutosaveEnabled")) { ICoreWebView2Profile6* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Profile6), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsPasswordAutosaveEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "IsGeneralAutofillEnabled")) { ICoreWebView2Profile6* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Profile6), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_IsGeneralAutofillEnabled(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "AreWebViewScriptApisEnabledForServiceWorkers")) { ICoreWebView2Profile9* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Profile9), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_AreWebViewScriptApisEnabledForServiceWorkers(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap);
  return RF_MISS;
}

int ReflSet_PROFILE(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
  if (!_stricmp(name, "DefaultDownloadFolderPath")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2Profile* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Profile), (void**)&o))) return RF_ERR; HRESULT h = o->put_DefaultDownloadFolderPath(wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "IsPasswordAutosaveEnabled")) { ICoreWebView2Profile6* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Profile6), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsPasswordAutosaveEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "IsGeneralAutofillEnabled")) { ICoreWebView2Profile6* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Profile6), (void**)&o))) return RF_ERR; HRESULT h = o->put_IsGeneralAutofillEnabled(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "AreWebViewScriptApisEnabledForServiceWorkers")) { ICoreWebView2Profile9* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Profile9), (void**)&o))) return RF_ERR; HRESULT h = o->put_AreWebViewScriptApisEnabledForServiceWorkers(inInt ? TRUE : FALSE); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

int ReflCall_PROFILE(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
  if (!_stricmp(name, "Delete")) { ICoreWebView2Profile8* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Profile8), (void**)&o))) return RF_ERR; HRESULT h = o->Delete(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

void ReflNames_PROFILE(char* buf, INT cap)
{
  static const char* items[] = {
    "ProfileName",     "IsInPrivateModeEnabled",     "ProfilePath",     "DefaultDownloadFolderPath",     "DefaultDownloadFolderPath",     "IsPasswordAutosaveEnabled",
    "IsPasswordAutosaveEnabled",     "IsGeneralAutofillEnabled",     "IsGeneralAutofillEnabled",     "Delete",     "AreWebViewScriptApisEnabledForServiceWorkers",     "AreWebViewScriptApisEnabledForServiceWorkers",

  };
  ReflJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

// ==== FRAME: 读 2 / 写 0 / 调 3 ====
int ReflGet_FRAME(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{
  if (!_stricmp(name, "Name")) { ICoreWebView2Frame* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Frame), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_Name(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "FrameId")) { ICoreWebView2Frame5* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Frame5), (void**)&o))) return RF_ERR; UINT32 v{}; HRESULT h = o->get_FrameId(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap);
  return RF_MISS;
}

int ReflSet_FRAME(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{

  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

int ReflCall_FRAME(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
  if (!_stricmp(name, "RemoveHostObjectFromScript")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2Frame* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Frame), (void**)&o))) return RF_ERR; HRESULT h = o->RemoveHostObjectFromScript((LPCWSTR)wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "PostWebMessageAsJson")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2Frame2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Frame2), (void**)&o))) return RF_ERR; HRESULT h = o->PostWebMessageAsJson((LPCWSTR)wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "PostWebMessageAsString")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2Frame2* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2Frame2), (void**)&o))) return RF_ERR; HRESULT h = o->PostWebMessageAsString((LPCWSTR)wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

void ReflNames_FRAME(char* buf, INT cap)
{
  static const char* items[] = {
    "Name",     "RemoveHostObjectFromScript",     "PostWebMessageAsJson",     "PostWebMessageAsString",     "FrameId", 
  };
  ReflJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

// ==== DOWNLOAD: 读 6 / 写 0 / 调 3 ====
int ReflGet_DOWNLOAD(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{
  if (!_stricmp(name, "Uri")) { ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_Uri(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "ContentDisposition")) { ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_ContentDisposition(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "MimeType")) { ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_MimeType(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "EstimatedEndTime")) { ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_EstimatedEndTime(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "ResultFilePath")) { ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_ResultFilePath(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "CanResume")) { ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return RF_ERR; BOOL v{}; HRESULT h = o->get_CanResume(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap);
  return RF_MISS;
}

int ReflSet_DOWNLOAD(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{

  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

int ReflCall_DOWNLOAD(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
  if (!_stricmp(name, "Cancel")) { ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return RF_ERR; HRESULT h = o->Cancel(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "Pause")) { ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return RF_ERR; HRESULT h = o->Pause(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "Resume")) { ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return RF_ERR; HRESULT h = o->Resume(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

void ReflNames_DOWNLOAD(char* buf, INT cap)
{
  static const char* items[] = {
    "Uri",     "ContentDisposition",     "MimeType",     "EstimatedEndTime",     "ResultFilePath",     "Cancel",
    "Pause",     "Resume",     "CanResume", 
  };
  ReflJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

// ==== REQUEST: 读 2 / 写 2 / 调 0 ====
int ReflGet_REQUEST(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{
  if (!_stricmp(name, "Uri")) { ICoreWebView2WebResourceRequest* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2WebResourceRequest), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_Uri(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "Method")) { ICoreWebView2WebResourceRequest* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2WebResourceRequest), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_Method(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap);
  return RF_MISS;
}

int ReflSet_REQUEST(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
  if (!_stricmp(name, "Uri")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2WebResourceRequest* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2WebResourceRequest), (void**)&o))) return RF_ERR; HRESULT h = o->put_Uri(wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "Method")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2WebResourceRequest* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2WebResourceRequest), (void**)&o))) return RF_ERR; HRESULT h = o->put_Method(wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

int ReflCall_REQUEST(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{

  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

void ReflNames_REQUEST(char* buf, INT cap)
{
  static const char* items[] = {
    "Uri",     "Uri",     "Method",     "Method", 
  };
  ReflJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

// ==== RESPONSE: 读 2 / 写 2 / 调 0 ====
int ReflGet_RESPONSE(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{
  if (!_stricmp(name, "StatusCode")) { ICoreWebView2WebResourceResponse* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2WebResourceResponse), (void**)&o))) return RF_ERR; int v{}; HRESULT h = o->get_StatusCode(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = 0; return RF_OK; }
  if (!_stricmp(name, "ReasonPhrase")) { ICoreWebView2WebResourceResponse* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2WebResourceResponse), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_ReasonPhrase(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap);
  return RF_MISS;
}

int ReflSet_RESPONSE(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
  if (!_stricmp(name, "StatusCode")) { ICoreWebView2WebResourceResponse* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2WebResourceResponse), (void**)&o))) return RF_ERR; HRESULT h = o->put_StatusCode((int)inInt); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  if (!_stricmp(name, "ReasonPhrase")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; ICoreWebView2WebResourceResponse* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2WebResourceResponse), (void**)&o))) return RF_ERR; HRESULT h = o->put_ReasonPhrase(wz); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }
  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

int ReflCall_RESPONSE(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{

  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

void ReflNames_RESPONSE(char* buf, INT cap)
{
  static const char* items[] = {
    "StatusCode",     "StatusCode",     "ReasonPhrase",     "ReasonPhrase", 
  };
  ReflJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

int ReflGet_PROCESSINFO(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{ UNREF(pObj); UNREF(name); UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap); return RF_MISS; }
int ReflSet_PROCESSINFO(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{ UNREF(pObj); UNREF(name); UNREF(inInt); UNREF(inDouble); UNREF(inText); return RF_MISS; }
int ReflCall_PROCESSINFO(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{ UNREF(pObj); UNREF(name); UNREF(inInt); UNREF(inDouble); UNREF(inText); return RF_MISS; }
void ReflNames_PROCESSINFO(char* buf, INT cap)
{ if (buf != NULL && cap > 0) buf[0] = '\0'; UNREF(cap); }

int ReflGet_RECEIVER(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{ UNREF(pObj); UNREF(name); UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap); return RF_MISS; }
int ReflSet_RECEIVER(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{ UNREF(pObj); UNREF(name); UNREF(inInt); UNREF(inDouble); UNREF(inText); return RF_MISS; }
int ReflCall_RECEIVER(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{ UNREF(pObj); UNREF(name); UNREF(inInt); UNREF(inDouble); UNREF(inText); return RF_MISS; }
void ReflNames_RECEIVER(char* buf, INT cap)
{ if (buf != NULL && cap > 0) buf[0] = '\0'; UNREF(cap); }

// ==== CERT: 读 6 / 写 0 / 调 0 ====
int ReflGet_CERT(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{
  if (!_stricmp(name, "Subject")) { ICoreWebView2ClientCertificate* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2ClientCertificate), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_Subject(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "Issuer")) { ICoreWebView2ClientCertificate* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2ClientCertificate), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_Issuer(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "ValidFrom")) { ICoreWebView2ClientCertificate* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2ClientCertificate), (void**)&o))) return RF_ERR; double v{}; HRESULT h = o->get_ValidFrom(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = (double)v; return RF_OK; }
  if (!_stricmp(name, "ValidTo")) { ICoreWebView2ClientCertificate* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2ClientCertificate), (void**)&o))) return RF_ERR; double v{}; HRESULT h = o->get_ValidTo(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = (double)v; return RF_OK; }
  if (!_stricmp(name, "DerEncodedSerialNumber")) { ICoreWebView2ClientCertificate* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2ClientCertificate), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_DerEncodedSerialNumber(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  if (!_stricmp(name, "DisplayName")) { ICoreWebView2ClientCertificate* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(ICoreWebView2ClientCertificate), (void**)&o))) return RF_ERR; LPWSTR s = nullptr; HRESULT h = o->get_DisplayName(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }
  UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap);
  return RF_MISS;
}

int ReflSet_CERT(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{

  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

int ReflCall_CERT(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{

  UNREF(inInt); UNREF(inDouble); UNREF(inText);
  return RF_MISS;
}

void ReflNames_CERT(char* buf, INT cap)
{
  static const char* items[] = {
    "Subject",     "Issuer",     "ValidFrom",     "ValidTo",     "DerEncodedSerialNumber",     "DisplayName",

  };
  ReflJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

}  // namespace wv
