// 由 gen_events.mjs 从 WebView2.h 自动生成，勿手改。
// 事件按名订阅 + 参数 JSON 快照 + 参数按名读写 / 子对象 / 列表 / 延后。

#include "wv_pre.h"
#include "reflect_events.h"
#include "wv_json.h"
#include <string.h>
#include <stdio.h>

namespace wv {

static void WvJoinNames(char* buf, INT cap, const char** items, INT count)
{ if (buf == NULL || cap <= 0) return; buf[0] = '\0'; INT used = 0;
  for (INT i = 0; i < count; i++) { INT n = (INT)lstrlenA(items[i]); if (used + n + 2 > cap) break;
    lstrcpyA(buf + used, items[i]); used += n; buf[used++] = '\n'; buf[used] = '\0'; } }

static void Desc_ICoreWebView2AcceleratorKeyPressedEventArgs(ICoreWebView2AcceleratorKeyPressedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { COREWEBVIEW2_KEY_EVENT_KIND v{}; if (p->get_KeyEventKind(&v) == S_OK) J_Num(buf, cap, pn, "KeyEventKind", (double)(long)v, FALSE); }
    { UINT v{}; if (p->get_VirtualKey(&v) == S_OK) J_Num(buf, cap, pn, "VirtualKey", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_Handled(&v) == S_OK) J_Num(buf, cap, pn, "Handled", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2MoveFocusRequestedEventArgs(ICoreWebView2MoveFocusRequestedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { COREWEBVIEW2_MOVE_FOCUS_REASON v{}; if (p->get_Reason(&v) == S_OK) J_Num(buf, cap, pn, "Reason", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_Handled(&v) == S_OK) J_Num(buf, cap, pn, "Handled", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2ContentLoadingEventArgs(ICoreWebView2ContentLoadingEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { BOOL v{}; if (p->get_IsErrorPage(&v) == S_OK) J_Num(buf, cap, pn, "IsErrorPage", (double)(long)v, FALSE); }
    { UINT64 v{}; if (p->get_NavigationId(&v) == S_OK) J_Num(buf, cap, pn, "NavigationId", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2DOMContentLoadedEventArgs(ICoreWebView2DOMContentLoadedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { UINT64 v{}; if (p->get_NavigationId(&v) == S_OK) J_Num(buf, cap, pn, "NavigationId", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2NavigationCompletedEventArgs(ICoreWebView2NavigationCompletedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { BOOL v{}; if (p->get_IsSuccess(&v) == S_OK) J_Num(buf, cap, pn, "IsSuccess", (double)(long)v, FALSE); }
    { COREWEBVIEW2_WEB_ERROR_STATUS v{}; if (p->get_WebErrorStatus(&v) == S_OK) J_Num(buf, cap, pn, "WebErrorStatus", (double)(long)v, FALSE); }
    { UINT64 v{}; if (p->get_NavigationId(&v) == S_OK) J_Num(buf, cap, pn, "NavigationId", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2NavigationStartingEventArgs(ICoreWebView2NavigationStartingEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Uri(&v) == S_OK) { J_StrW(buf, cap, pn, "Uri", v); CoTaskMemFree(v); } }
    { BOOL v{}; if (p->get_IsUserInitiated(&v) == S_OK) J_Num(buf, cap, pn, "IsUserInitiated", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_IsRedirected(&v) == S_OK) J_Num(buf, cap, pn, "IsRedirected", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_Cancel(&v) == S_OK) J_Num(buf, cap, pn, "Cancel", (double)(long)v, FALSE); }
    { UINT64 v{}; if (p->get_NavigationId(&v) == S_OK) J_Num(buf, cap, pn, "NavigationId", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2LaunchingExternalUriSchemeEventArgs(ICoreWebView2LaunchingExternalUriSchemeEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Uri(&v) == S_OK) { J_StrW(buf, cap, pn, "Uri", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_InitiatingOrigin(&v) == S_OK) { J_StrW(buf, cap, pn, "InitiatingOrigin", v); CoTaskMemFree(v); } }
    { BOOL v{}; if (p->get_IsUserInitiated(&v) == S_OK) J_Num(buf, cap, pn, "IsUserInitiated", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_Cancel(&v) == S_OK) J_Num(buf, cap, pn, "Cancel", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2PermissionRequestedEventArgs(ICoreWebView2PermissionRequestedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Uri(&v) == S_OK) { J_StrW(buf, cap, pn, "Uri", v); CoTaskMemFree(v); } }
    { COREWEBVIEW2_PERMISSION_KIND v{}; if (p->get_PermissionKind(&v) == S_OK) J_Num(buf, cap, pn, "PermissionKind", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_IsUserInitiated(&v) == S_OK) J_Num(buf, cap, pn, "IsUserInitiated", (double)(long)v, FALSE); }
    { COREWEBVIEW2_PERMISSION_STATE v{}; if (p->get_State(&v) == S_OK) J_Num(buf, cap, pn, "State", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2ProcessFailedEventArgs(ICoreWebView2ProcessFailedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { COREWEBVIEW2_PROCESS_FAILED_KIND v{}; if (p->get_ProcessFailedKind(&v) == S_OK) J_Num(buf, cap, pn, "ProcessFailedKind", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2SaveAsUIShowingEventArgs(ICoreWebView2SaveAsUIShowingEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_ContentMimeType(&v) == S_OK) { J_StrW(buf, cap, pn, "ContentMimeType", v); CoTaskMemFree(v); } }
    { BOOL v{}; if (p->get_Cancel(&v) == S_OK) J_Num(buf, cap, pn, "Cancel", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_SuppressDefaultDialog(&v) == S_OK) J_Num(buf, cap, pn, "SuppressDefaultDialog", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_SaveAsFilePath(&v) == S_OK) { J_StrW(buf, cap, pn, "SaveAsFilePath", v); CoTaskMemFree(v); } }
    { BOOL v{}; if (p->get_AllowReplace(&v) == S_OK) J_Num(buf, cap, pn, "AllowReplace", (double)(long)v, FALSE); }
    { COREWEBVIEW2_SAVE_AS_KIND v{}; if (p->get_Kind(&v) == S_OK) J_Num(buf, cap, pn, "Kind", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2SaveFileSecurityCheckStartingEventArgs(ICoreWebView2SaveFileSecurityCheckStartingEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { BOOL v{}; if (p->get_CancelSave(&v) == S_OK) J_Num(buf, cap, pn, "CancelSave", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_DocumentOriginUri(&v) == S_OK) { J_StrW(buf, cap, pn, "DocumentOriginUri", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_FileExtension(&v) == S_OK) { J_StrW(buf, cap, pn, "FileExtension", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_FilePath(&v) == S_OK) { J_StrW(buf, cap, pn, "FilePath", v); CoTaskMemFree(v); } }
    { BOOL v{}; if (p->get_SuppressDefaultPolicy(&v) == S_OK) J_Num(buf, cap, pn, "SuppressDefaultPolicy", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2ScriptDialogOpeningEventArgs(ICoreWebView2ScriptDialogOpeningEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Uri(&v) == S_OK) { J_StrW(buf, cap, pn, "Uri", v); CoTaskMemFree(v); } }
    { COREWEBVIEW2_SCRIPT_DIALOG_KIND v{}; if (p->get_Kind(&v) == S_OK) J_Num(buf, cap, pn, "Kind", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_Message(&v) == S_OK) { J_StrW(buf, cap, pn, "Message", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_DefaultText(&v) == S_OK) { J_StrW(buf, cap, pn, "DefaultText", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_ResultText(&v) == S_OK) { J_StrW(buf, cap, pn, "ResultText", v); CoTaskMemFree(v); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2SourceChangedEventArgs(ICoreWebView2SourceChangedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { BOOL v{}; if (p->get_IsNewDocument(&v) == S_OK) J_Num(buf, cap, pn, "IsNewDocument", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2WebMessageReceivedEventArgs(ICoreWebView2WebMessageReceivedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Source(&v) == S_OK) { J_StrW(buf, cap, pn, "Source", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_WebMessageAsJson(&v) == S_OK) { J_StrW(buf, cap, pn, "WebMessageAsJson", v); CoTaskMemFree(v); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2BrowserProcessExitedEventArgs(ICoreWebView2BrowserProcessExitedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { COREWEBVIEW2_BROWSER_PROCESS_EXIT_KIND v{}; if (p->get_BrowserProcessExitKind(&v) == S_OK) J_Num(buf, cap, pn, "BrowserProcessExitKind", (double)(long)v, FALSE); }
    { UINT32 v{}; if (p->get_BrowserProcessId(&v) == S_OK) J_Num(buf, cap, pn, "BrowserProcessId", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2PermissionRequestedEventArgs2(ICoreWebView2PermissionRequestedEventArgs2* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { BOOL v{}; if (p->get_Handled(&v) == S_OK) J_Num(buf, cap, pn, "Handled", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2DevToolsProtocolEventReceivedEventArgs(ICoreWebView2DevToolsProtocolEventReceivedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_ParameterObjectAsJson(&v) == S_OK) { J_StrW(buf, cap, pn, "ParameterObjectAsJson", v); CoTaskMemFree(v); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2ProcessExtendedInfoCollection(ICoreWebView2ProcessExtendedInfoCollection* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { UINT32 v{}; if (p->get_Count(&v) == S_OK) J_Num(buf, cap, pn, "Count", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2FrameInfoCollection(ICoreWebView2FrameInfoCollection* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2Cookie(ICoreWebView2Cookie* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Name(&v) == S_OK) { J_StrW(buf, cap, pn, "Name", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_Value(&v) == S_OK) { J_StrW(buf, cap, pn, "Value", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_Domain(&v) == S_OK) { J_StrW(buf, cap, pn, "Domain", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_Path(&v) == S_OK) { J_StrW(buf, cap, pn, "Path", v); CoTaskMemFree(v); } }
    { double v{}; if (p->get_Expires(&v) == S_OK) J_Num(buf, cap, pn, "Expires", (double)v, TRUE); }
    { BOOL v{}; if (p->get_IsHttpOnly(&v) == S_OK) J_Num(buf, cap, pn, "IsHttpOnly", (double)(long)v, FALSE); }
    { COREWEBVIEW2_COOKIE_SAME_SITE_KIND v{}; if (p->get_SameSite(&v) == S_OK) J_Num(buf, cap, pn, "SameSite", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_IsSecure(&v) == S_OK) J_Num(buf, cap, pn, "IsSecure", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_IsSession(&v) == S_OK) J_Num(buf, cap, pn, "IsSession", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2Profile(ICoreWebView2Profile* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_ProfileName(&v) == S_OK) { J_StrW(buf, cap, pn, "ProfileName", v); CoTaskMemFree(v); } }
    { BOOL v{}; if (p->get_IsInPrivateModeEnabled(&v) == S_OK) J_Num(buf, cap, pn, "IsInPrivateModeEnabled", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_ProfilePath(&v) == S_OK) { J_StrW(buf, cap, pn, "ProfilePath", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_DefaultDownloadFolderPath(&v) == S_OK) { J_StrW(buf, cap, pn, "DefaultDownloadFolderPath", v); CoTaskMemFree(v); } }
    { COREWEBVIEW2_PREFERRED_COLOR_SCHEME v{}; if (p->get_PreferredColorScheme(&v) == S_OK) J_Num(buf, cap, pn, "PreferredColorScheme", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2DownloadOperation(ICoreWebView2DownloadOperation* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Uri(&v) == S_OK) { J_StrW(buf, cap, pn, "Uri", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_ContentDisposition(&v) == S_OK) { J_StrW(buf, cap, pn, "ContentDisposition", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_MimeType(&v) == S_OK) { J_StrW(buf, cap, pn, "MimeType", v); CoTaskMemFree(v); } }
    { INT64 v{}; if (p->get_TotalBytesToReceive(&v) == S_OK) J_Num(buf, cap, pn, "TotalBytesToReceive", (double)(long)v, FALSE); }
    { INT64 v{}; if (p->get_BytesReceived(&v) == S_OK) J_Num(buf, cap, pn, "BytesReceived", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_EstimatedEndTime(&v) == S_OK) { J_StrW(buf, cap, pn, "EstimatedEndTime", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_ResultFilePath(&v) == S_OK) { J_StrW(buf, cap, pn, "ResultFilePath", v); CoTaskMemFree(v); } }
    { COREWEBVIEW2_DOWNLOAD_STATE v{}; if (p->get_State(&v) == S_OK) J_Num(buf, cap, pn, "State", (double)(long)v, FALSE); }
    { COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON v{}; if (p->get_InterruptReason(&v) == S_OK) J_Num(buf, cap, pn, "InterruptReason", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_CanResume(&v) == S_OK) J_Num(buf, cap, pn, "CanResume", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2Frame(ICoreWebView2Frame* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Name(&v) == S_OK) { J_StrW(buf, cap, pn, "Name", v); CoTaskMemFree(v); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2WebResourceResponse(ICoreWebView2WebResourceResponse* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { int v{}; if (p->get_StatusCode(&v) == S_OK) J_Num(buf, cap, pn, "StatusCode", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_ReasonPhrase(&v) == S_OK) { J_StrW(buf, cap, pn, "ReasonPhrase", v); CoTaskMemFree(v); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2WebResourceRequest(ICoreWebView2WebResourceRequest* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Uri(&v) == S_OK) { J_StrW(buf, cap, pn, "Uri", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_Method(&v) == S_OK) { J_StrW(buf, cap, pn, "Method", v); CoTaskMemFree(v); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2BrowserExtension(ICoreWebView2BrowserExtension* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Id(&v) == S_OK) { J_StrW(buf, cap, pn, "Id", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_Name(&v) == S_OK) { J_StrW(buf, cap, pn, "Name", v); CoTaskMemFree(v); } }
    { BOOL v{}; if (p->get_IsEnabled(&v) == S_OK) J_Num(buf, cap, pn, "IsEnabled", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2ProcessInfo(ICoreWebView2ProcessInfo* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { INT32 v{}; if (p->get_ProcessId(&v) == S_OK) J_Num(buf, cap, pn, "ProcessId", (double)(long)v, FALSE); }
    { COREWEBVIEW2_PROCESS_KIND v{}; if (p->get_Kind(&v) == S_OK) J_Num(buf, cap, pn, "Kind", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2BasicAuthenticationResponse(ICoreWebView2BasicAuthenticationResponse* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_UserName(&v) == S_OK) { J_StrW(buf, cap, pn, "UserName", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_Password(&v) == S_OK) { J_StrW(buf, cap, pn, "Password", v); CoTaskMemFree(v); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2StringCollection(ICoreWebView2StringCollection* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { UINT32 v{}; if (p->get_Count(&v) == S_OK) J_Num(buf, cap, pn, "Count", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2ContextMenuItemCollection(ICoreWebView2ContextMenuItemCollection* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { UINT32 v{}; if (p->get_Count(&v) == S_OK) J_Num(buf, cap, pn, "Count", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2ContextMenuTarget(ICoreWebView2ContextMenuTarget* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND v{}; if (p->get_Kind(&v) == S_OK) J_Num(buf, cap, pn, "Kind", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_IsEditable(&v) == S_OK) J_Num(buf, cap, pn, "IsEditable", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_IsRequestedForMainFrame(&v) == S_OK) J_Num(buf, cap, pn, "IsRequestedForMainFrame", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_PageUri(&v) == S_OK) { J_StrW(buf, cap, pn, "PageUri", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_FrameUri(&v) == S_OK) { J_StrW(buf, cap, pn, "FrameUri", v); CoTaskMemFree(v); } }
    { BOOL v{}; if (p->get_HasLinkUri(&v) == S_OK) J_Num(buf, cap, pn, "HasLinkUri", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_LinkUri(&v) == S_OK) { J_StrW(buf, cap, pn, "LinkUri", v); CoTaskMemFree(v); } }
    { BOOL v{}; if (p->get_HasLinkText(&v) == S_OK) J_Num(buf, cap, pn, "HasLinkText", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_LinkText(&v) == S_OK) { J_StrW(buf, cap, pn, "LinkText", v); CoTaskMemFree(v); } }
    { BOOL v{}; if (p->get_HasSourceUri(&v) == S_OK) J_Num(buf, cap, pn, "HasSourceUri", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_SourceUri(&v) == S_OK) { J_StrW(buf, cap, pn, "SourceUri", v); CoTaskMemFree(v); } }
    { BOOL v{}; if (p->get_HasSelection(&v) == S_OK) J_Num(buf, cap, pn, "HasSelection", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_SelectionText(&v) == S_OK) { J_StrW(buf, cap, pn, "SelectionText", v); CoTaskMemFree(v); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2FrameInfo(ICoreWebView2FrameInfo* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Name(&v) == S_OK) { J_StrW(buf, cap, pn, "Name", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_Source(&v) == S_OK) { J_StrW(buf, cap, pn, "Source", v); CoTaskMemFree(v); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2DedicatedWorker(ICoreWebView2DedicatedWorker* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_ScriptUri(&v) == S_OK) { J_StrW(buf, cap, pn, "ScriptUri", v); CoTaskMemFree(v); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2WindowFeatures(ICoreWebView2WindowFeatures* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { BOOL v{}; if (p->get_HasPosition(&v) == S_OK) J_Num(buf, cap, pn, "HasPosition", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_HasSize(&v) == S_OK) J_Num(buf, cap, pn, "HasSize", (double)(long)v, FALSE); }
    { UINT32 v{}; if (p->get_Left(&v) == S_OK) J_Num(buf, cap, pn, "Left", (double)(long)v, FALSE); }
    { UINT32 v{}; if (p->get_Top(&v) == S_OK) J_Num(buf, cap, pn, "Top", (double)(long)v, FALSE); }
    { UINT32 v{}; if (p->get_Height(&v) == S_OK) J_Num(buf, cap, pn, "Height", (double)(long)v, FALSE); }
    { UINT32 v{}; if (p->get_Width(&v) == S_OK) J_Num(buf, cap, pn, "Width", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_ShouldDisplayMenuBar(&v) == S_OK) J_Num(buf, cap, pn, "ShouldDisplayMenuBar", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_ShouldDisplayStatus(&v) == S_OK) J_Num(buf, cap, pn, "ShouldDisplayStatus", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_ShouldDisplayToolbar(&v) == S_OK) J_Num(buf, cap, pn, "ShouldDisplayToolbar", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_ShouldDisplayScrollBars(&v) == S_OK) J_Num(buf, cap, pn, "ShouldDisplayScrollBars", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2Notification(ICoreWebView2Notification* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Body(&v) == S_OK) { J_StrW(buf, cap, pn, "Body", v); CoTaskMemFree(v); } }
    { COREWEBVIEW2_TEXT_DIRECTION_KIND v{}; if (p->get_Direction(&v) == S_OK) J_Num(buf, cap, pn, "Direction", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_Language(&v) == S_OK) { J_StrW(buf, cap, pn, "Language", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_Tag(&v) == S_OK) { J_StrW(buf, cap, pn, "Tag", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_IconUri(&v) == S_OK) { J_StrW(buf, cap, pn, "IconUri", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_Title(&v) == S_OK) { J_StrW(buf, cap, pn, "Title", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_BadgeUri(&v) == S_OK) { J_StrW(buf, cap, pn, "BadgeUri", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_BodyImageUri(&v) == S_OK) { J_StrW(buf, cap, pn, "BodyImageUri", v); CoTaskMemFree(v); } }
    { BOOL v{}; if (p->get_ShouldRenotify(&v) == S_OK) J_Num(buf, cap, pn, "ShouldRenotify", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_RequiresInteraction(&v) == S_OK) J_Num(buf, cap, pn, "RequiresInteraction", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_IsSilent(&v) == S_OK) J_Num(buf, cap, pn, "IsSilent", (double)(long)v, FALSE); }
    { double v{}; if (p->get_Timestamp(&v) == S_OK) J_Num(buf, cap, pn, "Timestamp", (double)v, TRUE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2WebResourceResponseView(ICoreWebView2WebResourceResponseView* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { int v{}; if (p->get_StatusCode(&v) == S_OK) J_Num(buf, cap, pn, "StatusCode", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_ReasonPhrase(&v) == S_OK) { J_StrW(buf, cap, pn, "ReasonPhrase", v); CoTaskMemFree(v); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2Settings(ICoreWebView2Settings* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { BOOL v{}; if (p->get_IsScriptEnabled(&v) == S_OK) J_Num(buf, cap, pn, "IsScriptEnabled", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_IsWebMessageEnabled(&v) == S_OK) J_Num(buf, cap, pn, "IsWebMessageEnabled", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_AreDefaultScriptDialogsEnabled(&v) == S_OK) J_Num(buf, cap, pn, "AreDefaultScriptDialogsEnabled", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_IsStatusBarEnabled(&v) == S_OK) J_Num(buf, cap, pn, "IsStatusBarEnabled", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_AreDevToolsEnabled(&v) == S_OK) J_Num(buf, cap, pn, "AreDevToolsEnabled", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_AreDefaultContextMenusEnabled(&v) == S_OK) J_Num(buf, cap, pn, "AreDefaultContextMenusEnabled", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_AreHostObjectsAllowed(&v) == S_OK) J_Num(buf, cap, pn, "AreHostObjectsAllowed", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_IsZoomControlEnabled(&v) == S_OK) J_Num(buf, cap, pn, "IsZoomControlEnabled", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_IsBuiltInErrorPageEnabled(&v) == S_OK) J_Num(buf, cap, pn, "IsBuiltInErrorPageEnabled", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2BasicAuthenticationRequestedEventArgs(ICoreWebView2BasicAuthenticationRequestedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Uri(&v) == S_OK) { J_StrW(buf, cap, pn, "Uri", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_Challenge(&v) == S_OK) { J_StrW(buf, cap, pn, "Challenge", v); CoTaskMemFree(v); } }
    { ICoreWebView2BasicAuthenticationResponse* v = nullptr; if (p->get_Response(&v) == S_OK) { J_Key(buf, cap, pn, "Response"); Desc_ICoreWebView2BasicAuthenticationResponse(v, buf, cap, pn); v->Release(); } }
    { BOOL v{}; if (p->get_Cancel(&v) == S_OK) J_Num(buf, cap, pn, "Cancel", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2ContextMenuRequestedEventArgs(ICoreWebView2ContextMenuRequestedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { ICoreWebView2ContextMenuItemCollection* v = nullptr; if (p->get_MenuItems(&v) == S_OK) { J_Key(buf, cap, pn, "MenuItems"); Desc_ICoreWebView2ContextMenuItemCollection(v, buf, cap, pn); v->Release(); } }
    { ICoreWebView2ContextMenuTarget* v = nullptr; if (p->get_ContextMenuTarget(&v) == S_OK) { J_Key(buf, cap, pn, "ContextMenuTarget"); Desc_ICoreWebView2ContextMenuTarget(v, buf, cap, pn); v->Release(); } }
    { INT32 v{}; if (p->get_SelectedCommandId(&v) == S_OK) J_Num(buf, cap, pn, "SelectedCommandId", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_Handled(&v) == S_OK) J_Num(buf, cap, pn, "Handled", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2DedicatedWorkerCreatedEventArgs(ICoreWebView2DedicatedWorkerCreatedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { ICoreWebView2FrameInfo* v = nullptr; if (p->get_OriginalSourceFrameInfo(&v) == S_OK) { J_Key(buf, cap, pn, "OriginalSourceFrameInfo"); Desc_ICoreWebView2FrameInfo(v, buf, cap, pn); v->Release(); } }
    { ICoreWebView2DedicatedWorker* v = nullptr; if (p->get_Worker(&v) == S_OK) { J_Key(buf, cap, pn, "Worker"); Desc_ICoreWebView2DedicatedWorker(v, buf, cap, pn); v->Release(); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2DownloadStartingEventArgs(ICoreWebView2DownloadStartingEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { ICoreWebView2DownloadOperation* v = nullptr; if (p->get_DownloadOperation(&v) == S_OK) { J_Key(buf, cap, pn, "DownloadOperation"); Desc_ICoreWebView2DownloadOperation(v, buf, cap, pn); v->Release(); } }
    { BOOL v{}; if (p->get_Cancel(&v) == S_OK) J_Num(buf, cap, pn, "Cancel", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_ResultFilePath(&v) == S_OK) { J_StrW(buf, cap, pn, "ResultFilePath", v); CoTaskMemFree(v); } }
    { BOOL v{}; if (p->get_Handled(&v) == S_OK) J_Num(buf, cap, pn, "Handled", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2FrameCreatedEventArgs(ICoreWebView2FrameCreatedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { ICoreWebView2Frame* v = nullptr; if (p->get_Frame(&v) == S_OK) { J_Key(buf, cap, pn, "Frame"); Desc_ICoreWebView2Frame(v, buf, cap, pn); v->Release(); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2NotificationReceivedEventArgs(ICoreWebView2NotificationReceivedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_SenderOrigin(&v) == S_OK) { J_StrW(buf, cap, pn, "SenderOrigin", v); CoTaskMemFree(v); } }
    { ICoreWebView2Notification* v = nullptr; if (p->get_Notification(&v) == S_OK) { J_Key(buf, cap, pn, "Notification"); Desc_ICoreWebView2Notification(v, buf, cap, pn); v->Release(); } }
    { BOOL v{}; if (p->get_Handled(&v) == S_OK) J_Num(buf, cap, pn, "Handled", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2ScreenCaptureStartingEventArgs(ICoreWebView2ScreenCaptureStartingEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { BOOL v{}; if (p->get_Cancel(&v) == S_OK) J_Num(buf, cap, pn, "Cancel", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_Handled(&v) == S_OK) J_Num(buf, cap, pn, "Handled", (double)(long)v, FALSE); }
    { ICoreWebView2FrameInfo* v = nullptr; if (p->get_OriginalSourceFrameInfo(&v) == S_OK) { J_Key(buf, cap, pn, "OriginalSourceFrameInfo"); Desc_ICoreWebView2FrameInfo(v, buf, cap, pn); v->Release(); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2WebResourceRequestedEventArgs(ICoreWebView2WebResourceRequestedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { ICoreWebView2WebResourceRequest* v = nullptr; if (p->get_Request(&v) == S_OK) { J_Key(buf, cap, pn, "Request"); Desc_ICoreWebView2WebResourceRequest(v, buf, cap, pn); v->Release(); } }
    { ICoreWebView2WebResourceResponse* v = nullptr; if (p->get_Response(&v) == S_OK) { J_Key(buf, cap, pn, "Response"); Desc_ICoreWebView2WebResourceResponse(v, buf, cap, pn); v->Release(); } }
    { COREWEBVIEW2_WEB_RESOURCE_CONTEXT v{}; if (p->get_ResourceContext(&v) == S_OK) J_Num(buf, cap, pn, "ResourceContext", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2WebResourceResponseReceivedEventArgs(ICoreWebView2WebResourceResponseReceivedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { ICoreWebView2WebResourceRequest* v = nullptr; if (p->get_Request(&v) == S_OK) { J_Key(buf, cap, pn, "Request"); Desc_ICoreWebView2WebResourceRequest(v, buf, cap, pn); v->Release(); } }
    { ICoreWebView2WebResourceResponseView* v = nullptr; if (p->get_Response(&v) == S_OK) { J_Key(buf, cap, pn, "Response"); Desc_ICoreWebView2WebResourceResponseView(v, buf, cap, pn); v->Release(); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2ProcessInfoCollection(ICoreWebView2ProcessInfoCollection* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { UINT32 v{}; if (p->get_Count(&v) == S_OK) J_Num(buf, cap, pn, "Count", (double)(long)v, FALSE); }
    { UINT32 nRaw{}; if (p->get_Count(&nRaw) == S_OK) { J_Key(buf, cap, pn, "Items"); J_Open(buf, cap, pn, '[');
        for (int i = 0; i < (int)nRaw && i < 32; i++) { ICoreWebView2ProcessInfo* v = nullptr; if (p->GetValueAtIndex(i, &v) == S_OK) { Desc_ICoreWebView2ProcessInfo(v, buf, cap, pn); v->Release(); } }
        J_Close(buf, cap, pn, ']'); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2CookieList(ICoreWebView2CookieList* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { UINT32 v{}; if (p->get_Count(&v) == S_OK) J_Num(buf, cap, pn, "Count", (double)(long)v, FALSE); }
    { UINT32 nRaw{}; if (p->get_Count(&nRaw) == S_OK) { J_Key(buf, cap, pn, "Items"); J_Open(buf, cap, pn, '[');
        for (int i = 0; i < (int)nRaw && i < 32; i++) { ICoreWebView2Cookie* v = nullptr; if (p->GetValueAtIndex(i, &v) == S_OK) { Desc_ICoreWebView2Cookie(v, buf, cap, pn); v->Release(); } }
        J_Close(buf, cap, pn, ']'); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2BrowserExtensionList(ICoreWebView2BrowserExtensionList* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { UINT32 v{}; if (p->get_Count(&v) == S_OK) J_Num(buf, cap, pn, "Count", (double)(long)v, FALSE); }
    { UINT32 nRaw{}; if (p->get_Count(&nRaw) == S_OK) { J_Key(buf, cap, pn, "Items"); J_Open(buf, cap, pn, '[');
        for (int i = 0; i < (int)nRaw && i < 32; i++) { ICoreWebView2BrowserExtension* v = nullptr; if (p->GetValueAtIndex(i, &v) == S_OK) { Desc_ICoreWebView2BrowserExtension(v, buf, cap, pn); v->Release(); } }
        J_Close(buf, cap, pn, ']'); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2ClientCertificate(ICoreWebView2ClientCertificate* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Subject(&v) == S_OK) { J_StrW(buf, cap, pn, "Subject", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_Issuer(&v) == S_OK) { J_StrW(buf, cap, pn, "Issuer", v); CoTaskMemFree(v); } }
    { double v{}; if (p->get_ValidFrom(&v) == S_OK) J_Num(buf, cap, pn, "ValidFrom", (double)v, TRUE); }
    { double v{}; if (p->get_ValidTo(&v) == S_OK) J_Num(buf, cap, pn, "ValidTo", (double)v, TRUE); }
    { LPWSTR v = nullptr; if (p->get_DerEncodedSerialNumber(&v) == S_OK) { J_StrW(buf, cap, pn, "DerEncodedSerialNumber", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_DisplayName(&v) == S_OK) { J_StrW(buf, cap, pn, "DisplayName", v); CoTaskMemFree(v); } }
    { ICoreWebView2StringCollection* v = nullptr; if (p->get_PemEncodedIssuerCertificateChain(&v) == S_OK) { J_Key(buf, cap, pn, "PemEncodedIssuerCertificateChain"); Desc_ICoreWebView2StringCollection(v, buf, cap, pn); v->Release(); } }
    { COREWEBVIEW2_CLIENT_CERTIFICATE_KIND v{}; if (p->get_Kind(&v) == S_OK) J_Num(buf, cap, pn, "Kind", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2(ICoreWebView2* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { ICoreWebView2Settings* v = nullptr; if (p->get_Settings(&v) == S_OK) { J_Key(buf, cap, pn, "Settings"); Desc_ICoreWebView2Settings(v, buf, cap, pn); v->Release(); } }
    { LPWSTR v = nullptr; if (p->get_Source(&v) == S_OK) { J_StrW(buf, cap, pn, "Source", v); CoTaskMemFree(v); } }
    { UINT32 v{}; if (p->get_BrowserProcessId(&v) == S_OK) J_Num(buf, cap, pn, "BrowserProcessId", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_CanGoBack(&v) == S_OK) J_Num(buf, cap, pn, "CanGoBack", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_CanGoForward(&v) == S_OK) J_Num(buf, cap, pn, "CanGoForward", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_DocumentTitle(&v) == S_OK) { J_StrW(buf, cap, pn, "DocumentTitle", v); CoTaskMemFree(v); } }
    { BOOL v{}; if (p->get_ContainsFullScreenElement(&v) == S_OK) J_Num(buf, cap, pn, "ContainsFullScreenElement", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2Certificate(ICoreWebView2Certificate* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Subject(&v) == S_OK) { J_StrW(buf, cap, pn, "Subject", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_Issuer(&v) == S_OK) { J_StrW(buf, cap, pn, "Issuer", v); CoTaskMemFree(v); } }
    { double v{}; if (p->get_ValidFrom(&v) == S_OK) J_Num(buf, cap, pn, "ValidFrom", (double)v, TRUE); }
    { double v{}; if (p->get_ValidTo(&v) == S_OK) J_Num(buf, cap, pn, "ValidTo", (double)v, TRUE); }
    { LPWSTR v = nullptr; if (p->get_DerEncodedSerialNumber(&v) == S_OK) { J_StrW(buf, cap, pn, "DerEncodedSerialNumber", v); CoTaskMemFree(v); } }
    { LPWSTR v = nullptr; if (p->get_DisplayName(&v) == S_OK) { J_StrW(buf, cap, pn, "DisplayName", v); CoTaskMemFree(v); } }
    { ICoreWebView2StringCollection* v = nullptr; if (p->get_PemEncodedIssuerCertificateChain(&v) == S_OK) { J_Key(buf, cap, pn, "PemEncodedIssuerCertificateChain"); Desc_ICoreWebView2StringCollection(v, buf, cap, pn); v->Release(); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2NewWindowRequestedEventArgs(ICoreWebView2NewWindowRequestedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Uri(&v) == S_OK) { J_StrW(buf, cap, pn, "Uri", v); CoTaskMemFree(v); } }
    { ICoreWebView2* v = nullptr; if (p->get_NewWindow(&v) == S_OK) { J_Key(buf, cap, pn, "NewWindow"); Desc_ICoreWebView2(v, buf, cap, pn); v->Release(); } }
    { BOOL v{}; if (p->get_Handled(&v) == S_OK) J_Num(buf, cap, pn, "Handled", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_IsUserInitiated(&v) == S_OK) J_Num(buf, cap, pn, "IsUserInitiated", (double)(long)v, FALSE); }
    { ICoreWebView2WindowFeatures* v = nullptr; if (p->get_WindowFeatures(&v) == S_OK) { J_Key(buf, cap, pn, "WindowFeatures"); Desc_ICoreWebView2WindowFeatures(v, buf, cap, pn); v->Release(); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2ServerCertificateErrorDetectedEventArgs(ICoreWebView2ServerCertificateErrorDetectedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { COREWEBVIEW2_WEB_ERROR_STATUS v{}; if (p->get_ErrorStatus(&v) == S_OK) J_Num(buf, cap, pn, "ErrorStatus", (double)(long)v, FALSE); }
    { LPWSTR v = nullptr; if (p->get_RequestUri(&v) == S_OK) { J_StrW(buf, cap, pn, "RequestUri", v); CoTaskMemFree(v); } }
    { ICoreWebView2Certificate* v = nullptr; if (p->get_ServerCertificate(&v) == S_OK) { J_Key(buf, cap, pn, "ServerCertificate"); Desc_ICoreWebView2Certificate(v, buf, cap, pn); v->Release(); } }
    { COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION v{}; if (p->get_Action(&v) == S_OK) J_Num(buf, cap, pn, "Action", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2ClientCertificateCollection(ICoreWebView2ClientCertificateCollection* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { UINT32 v{}; if (p->get_Count(&v) == S_OK) J_Num(buf, cap, pn, "Count", (double)(long)v, FALSE); }
    { UINT32 nRaw{}; if (p->get_Count(&nRaw) == S_OK) { J_Key(buf, cap, pn, "Items"); J_Open(buf, cap, pn, '[');
        for (int i = 0; i < (int)nRaw && i < 32; i++) { ICoreWebView2ClientCertificate* v = nullptr; if (p->GetValueAtIndex(i, &v) == S_OK) { Desc_ICoreWebView2ClientCertificate(v, buf, cap, pn); v->Release(); } }
        J_Close(buf, cap, pn, ']'); } }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

static void Desc_ICoreWebView2ClientCertificateRequestedEventArgs(ICoreWebView2ClientCertificateRequestedEventArgs* p, char* buf, int cap, int* pn)
{
    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;
    if (!JEnter()) return;
    J_Open(buf, cap, pn, '{');
    { LPWSTR v = nullptr; if (p->get_Host(&v) == S_OK) { J_StrW(buf, cap, pn, "Host", v); CoTaskMemFree(v); } }
    { int v{}; if (p->get_Port(&v) == S_OK) J_Num(buf, cap, pn, "Port", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_IsProxy(&v) == S_OK) J_Num(buf, cap, pn, "IsProxy", (double)(long)v, FALSE); }
    { ICoreWebView2StringCollection* v = nullptr; if (p->get_AllowedCertificateAuthorities(&v) == S_OK) { J_Key(buf, cap, pn, "AllowedCertificateAuthorities"); Desc_ICoreWebView2StringCollection(v, buf, cap, pn); v->Release(); } }
    { ICoreWebView2ClientCertificateCollection* v = nullptr; if (p->get_MutuallyTrustedCertificates(&v) == S_OK) { J_Key(buf, cap, pn, "MutuallyTrustedCertificates"); Desc_ICoreWebView2ClientCertificateCollection(v, buf, cap, pn); v->Release(); } }
    { ICoreWebView2ClientCertificate* v = nullptr; if (p->get_SelectedCertificate(&v) == S_OK) { J_Key(buf, cap, pn, "SelectedCertificate"); Desc_ICoreWebView2ClientCertificate(v, buf, cap, pn); v->Release(); } }
    { BOOL v{}; if (p->get_Cancel(&v) == S_OK) J_Num(buf, cap, pn, "Cancel", (double)(long)v, FALSE); }
    { BOOL v{}; if (p->get_Handled(&v) == S_OK) J_Num(buf, cap, pn, "Handled", (double)(long)v, FALSE); }
    J_Close(buf, cap, pn, '}');
    JLeave();
}

struct WvDescItem { LPCSTR m_szIface; void (*m_pfn)(void*, char*, int, int*); };
static void CDesc_ICoreWebView2AcceleratorKeyPressedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2AcceleratorKeyPressedEventArgs((ICoreWebView2AcceleratorKeyPressedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2MoveFocusRequestedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2MoveFocusRequestedEventArgs((ICoreWebView2MoveFocusRequestedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2ContentLoadingEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2ContentLoadingEventArgs((ICoreWebView2ContentLoadingEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2DOMContentLoadedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2DOMContentLoadedEventArgs((ICoreWebView2DOMContentLoadedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2NavigationCompletedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2NavigationCompletedEventArgs((ICoreWebView2NavigationCompletedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2NavigationStartingEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2NavigationStartingEventArgs((ICoreWebView2NavigationStartingEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2LaunchingExternalUriSchemeEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2LaunchingExternalUriSchemeEventArgs((ICoreWebView2LaunchingExternalUriSchemeEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2PermissionRequestedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2PermissionRequestedEventArgs((ICoreWebView2PermissionRequestedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2ProcessFailedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2ProcessFailedEventArgs((ICoreWebView2ProcessFailedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2SaveAsUIShowingEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2SaveAsUIShowingEventArgs((ICoreWebView2SaveAsUIShowingEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2SaveFileSecurityCheckStartingEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2SaveFileSecurityCheckStartingEventArgs((ICoreWebView2SaveFileSecurityCheckStartingEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2ScriptDialogOpeningEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2ScriptDialogOpeningEventArgs((ICoreWebView2ScriptDialogOpeningEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2SourceChangedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2SourceChangedEventArgs((ICoreWebView2SourceChangedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2WebMessageReceivedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2WebMessageReceivedEventArgs((ICoreWebView2WebMessageReceivedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2BrowserProcessExitedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2BrowserProcessExitedEventArgs((ICoreWebView2BrowserProcessExitedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2PermissionRequestedEventArgs2(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2PermissionRequestedEventArgs2((ICoreWebView2PermissionRequestedEventArgs2*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2DevToolsProtocolEventReceivedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2DevToolsProtocolEventReceivedEventArgs((ICoreWebView2DevToolsProtocolEventReceivedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2ProcessExtendedInfoCollection(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2ProcessExtendedInfoCollection((ICoreWebView2ProcessExtendedInfoCollection*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2FrameInfoCollection(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2FrameInfoCollection((ICoreWebView2FrameInfoCollection*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2Cookie(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2Cookie((ICoreWebView2Cookie*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2Profile(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2Profile((ICoreWebView2Profile*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2DownloadOperation(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2DownloadOperation((ICoreWebView2DownloadOperation*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2Frame(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2Frame((ICoreWebView2Frame*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2WebResourceResponse(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2WebResourceResponse((ICoreWebView2WebResourceResponse*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2WebResourceRequest(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2WebResourceRequest((ICoreWebView2WebResourceRequest*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2BrowserExtension(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2BrowserExtension((ICoreWebView2BrowserExtension*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2ProcessInfo(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2ProcessInfo((ICoreWebView2ProcessInfo*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2BasicAuthenticationResponse(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2BasicAuthenticationResponse((ICoreWebView2BasicAuthenticationResponse*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2StringCollection(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2StringCollection((ICoreWebView2StringCollection*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2ContextMenuItemCollection(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2ContextMenuItemCollection((ICoreWebView2ContextMenuItemCollection*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2ContextMenuTarget(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2ContextMenuTarget((ICoreWebView2ContextMenuTarget*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2FrameInfo(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2FrameInfo((ICoreWebView2FrameInfo*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2DedicatedWorker(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2DedicatedWorker((ICoreWebView2DedicatedWorker*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2WindowFeatures(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2WindowFeatures((ICoreWebView2WindowFeatures*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2Notification(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2Notification((ICoreWebView2Notification*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2WebResourceResponseView(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2WebResourceResponseView((ICoreWebView2WebResourceResponseView*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2Settings(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2Settings((ICoreWebView2Settings*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2BasicAuthenticationRequestedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2BasicAuthenticationRequestedEventArgs((ICoreWebView2BasicAuthenticationRequestedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2ContextMenuRequestedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2ContextMenuRequestedEventArgs((ICoreWebView2ContextMenuRequestedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2DedicatedWorkerCreatedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2DedicatedWorkerCreatedEventArgs((ICoreWebView2DedicatedWorkerCreatedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2DownloadStartingEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2DownloadStartingEventArgs((ICoreWebView2DownloadStartingEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2FrameCreatedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2FrameCreatedEventArgs((ICoreWebView2FrameCreatedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2NotificationReceivedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2NotificationReceivedEventArgs((ICoreWebView2NotificationReceivedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2ScreenCaptureStartingEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2ScreenCaptureStartingEventArgs((ICoreWebView2ScreenCaptureStartingEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2WebResourceRequestedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2WebResourceRequestedEventArgs((ICoreWebView2WebResourceRequestedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2WebResourceResponseReceivedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2WebResourceResponseReceivedEventArgs((ICoreWebView2WebResourceResponseReceivedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2ProcessInfoCollection(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2ProcessInfoCollection((ICoreWebView2ProcessInfoCollection*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2CookieList(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2CookieList((ICoreWebView2CookieList*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2BrowserExtensionList(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2BrowserExtensionList((ICoreWebView2BrowserExtensionList*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2ClientCertificate(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2ClientCertificate((ICoreWebView2ClientCertificate*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2((ICoreWebView2*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2Certificate(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2Certificate((ICoreWebView2Certificate*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2NewWindowRequestedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2NewWindowRequestedEventArgs((ICoreWebView2NewWindowRequestedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2ServerCertificateErrorDetectedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2ServerCertificateErrorDetectedEventArgs((ICoreWebView2ServerCertificateErrorDetectedEventArgs*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2ClientCertificateCollection(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2ClientCertificateCollection((ICoreWebView2ClientCertificateCollection*)p, buf, cap, pn); }
static void CDesc_ICoreWebView2ClientCertificateRequestedEventArgs(void* p, char* buf, int cap, int* pn) { Desc_ICoreWebView2ClientCertificateRequestedEventArgs((ICoreWebView2ClientCertificateRequestedEventArgs*)p, buf, cap, pn); }
static const WvDescItem g_descItems[] = {
    { "ICoreWebView2AcceleratorKeyPressedEventArgs", CDesc_ICoreWebView2AcceleratorKeyPressedEventArgs },
    { "ICoreWebView2MoveFocusRequestedEventArgs", CDesc_ICoreWebView2MoveFocusRequestedEventArgs },
    { "ICoreWebView2ContentLoadingEventArgs", CDesc_ICoreWebView2ContentLoadingEventArgs },
    { "ICoreWebView2DOMContentLoadedEventArgs", CDesc_ICoreWebView2DOMContentLoadedEventArgs },
    { "ICoreWebView2NavigationCompletedEventArgs", CDesc_ICoreWebView2NavigationCompletedEventArgs },
    { "ICoreWebView2NavigationStartingEventArgs", CDesc_ICoreWebView2NavigationStartingEventArgs },
    { "ICoreWebView2LaunchingExternalUriSchemeEventArgs", CDesc_ICoreWebView2LaunchingExternalUriSchemeEventArgs },
    { "ICoreWebView2PermissionRequestedEventArgs", CDesc_ICoreWebView2PermissionRequestedEventArgs },
    { "ICoreWebView2ProcessFailedEventArgs", CDesc_ICoreWebView2ProcessFailedEventArgs },
    { "ICoreWebView2SaveAsUIShowingEventArgs", CDesc_ICoreWebView2SaveAsUIShowingEventArgs },
    { "ICoreWebView2SaveFileSecurityCheckStartingEventArgs", CDesc_ICoreWebView2SaveFileSecurityCheckStartingEventArgs },
    { "ICoreWebView2ScriptDialogOpeningEventArgs", CDesc_ICoreWebView2ScriptDialogOpeningEventArgs },
    { "ICoreWebView2SourceChangedEventArgs", CDesc_ICoreWebView2SourceChangedEventArgs },
    { "ICoreWebView2WebMessageReceivedEventArgs", CDesc_ICoreWebView2WebMessageReceivedEventArgs },
    { "ICoreWebView2BrowserProcessExitedEventArgs", CDesc_ICoreWebView2BrowserProcessExitedEventArgs },
    { "ICoreWebView2PermissionRequestedEventArgs2", CDesc_ICoreWebView2PermissionRequestedEventArgs2 },
    { "ICoreWebView2DevToolsProtocolEventReceivedEventArgs", CDesc_ICoreWebView2DevToolsProtocolEventReceivedEventArgs },
    { "ICoreWebView2ProcessExtendedInfoCollection", CDesc_ICoreWebView2ProcessExtendedInfoCollection },
    { "ICoreWebView2FrameInfoCollection", CDesc_ICoreWebView2FrameInfoCollection },
    { "ICoreWebView2Cookie", CDesc_ICoreWebView2Cookie },
    { "ICoreWebView2Profile", CDesc_ICoreWebView2Profile },
    { "ICoreWebView2DownloadOperation", CDesc_ICoreWebView2DownloadOperation },
    { "ICoreWebView2Frame", CDesc_ICoreWebView2Frame },
    { "ICoreWebView2WebResourceResponse", CDesc_ICoreWebView2WebResourceResponse },
    { "ICoreWebView2WebResourceRequest", CDesc_ICoreWebView2WebResourceRequest },
    { "ICoreWebView2BrowserExtension", CDesc_ICoreWebView2BrowserExtension },
    { "ICoreWebView2ProcessInfo", CDesc_ICoreWebView2ProcessInfo },
    { "ICoreWebView2BasicAuthenticationResponse", CDesc_ICoreWebView2BasicAuthenticationResponse },
    { "ICoreWebView2StringCollection", CDesc_ICoreWebView2StringCollection },
    { "ICoreWebView2ContextMenuItemCollection", CDesc_ICoreWebView2ContextMenuItemCollection },
    { "ICoreWebView2ContextMenuTarget", CDesc_ICoreWebView2ContextMenuTarget },
    { "ICoreWebView2FrameInfo", CDesc_ICoreWebView2FrameInfo },
    { "ICoreWebView2DedicatedWorker", CDesc_ICoreWebView2DedicatedWorker },
    { "ICoreWebView2WindowFeatures", CDesc_ICoreWebView2WindowFeatures },
    { "ICoreWebView2Notification", CDesc_ICoreWebView2Notification },
    { "ICoreWebView2WebResourceResponseView", CDesc_ICoreWebView2WebResourceResponseView },
    { "ICoreWebView2Settings", CDesc_ICoreWebView2Settings },
    { "ICoreWebView2BasicAuthenticationRequestedEventArgs", CDesc_ICoreWebView2BasicAuthenticationRequestedEventArgs },
    { "ICoreWebView2ContextMenuRequestedEventArgs", CDesc_ICoreWebView2ContextMenuRequestedEventArgs },
    { "ICoreWebView2DedicatedWorkerCreatedEventArgs", CDesc_ICoreWebView2DedicatedWorkerCreatedEventArgs },
    { "ICoreWebView2DownloadStartingEventArgs", CDesc_ICoreWebView2DownloadStartingEventArgs },
    { "ICoreWebView2FrameCreatedEventArgs", CDesc_ICoreWebView2FrameCreatedEventArgs },
    { "ICoreWebView2NotificationReceivedEventArgs", CDesc_ICoreWebView2NotificationReceivedEventArgs },
    { "ICoreWebView2ScreenCaptureStartingEventArgs", CDesc_ICoreWebView2ScreenCaptureStartingEventArgs },
    { "ICoreWebView2WebResourceRequestedEventArgs", CDesc_ICoreWebView2WebResourceRequestedEventArgs },
    { "ICoreWebView2WebResourceResponseReceivedEventArgs", CDesc_ICoreWebView2WebResourceResponseReceivedEventArgs },
    { "ICoreWebView2ProcessInfoCollection", CDesc_ICoreWebView2ProcessInfoCollection },
    { "ICoreWebView2CookieList", CDesc_ICoreWebView2CookieList },
    { "ICoreWebView2BrowserExtensionList", CDesc_ICoreWebView2BrowserExtensionList },
    { "ICoreWebView2ClientCertificate", CDesc_ICoreWebView2ClientCertificate },
    { "ICoreWebView2", CDesc_ICoreWebView2 },
    { "ICoreWebView2Certificate", CDesc_ICoreWebView2Certificate },
    { "ICoreWebView2NewWindowRequestedEventArgs", CDesc_ICoreWebView2NewWindowRequestedEventArgs },
    { "ICoreWebView2ServerCertificateErrorDetectedEventArgs", CDesc_ICoreWebView2ServerCertificateErrorDetectedEventArgs },
    { "ICoreWebView2ClientCertificateCollection", CDesc_ICoreWebView2ClientCertificateCollection },
    { "ICoreWebView2ClientCertificateRequestedEventArgs", CDesc_ICoreWebView2ClientCertificateRequestedEventArgs },
};

BOOL WvDescribeObject(LPCSTR ifaceName, void* pObj, char* buf, INT cap)
{
    if (ifaceName == NULL || pObj == NULL || buf == NULL || cap <= 0) return FALSE;
    buf[0] = '\0';
    for (INT i = 0; i < (INT)(sizeof(g_descItems) / sizeof(g_descItems[0])); i++)
        if (lstrcmpA(g_descItems[i].m_szIface, ifaceName) == 0) { INT n = 0; g_descItems[i].m_pfn(pObj, buf, cap, &n); return TRUE; }
    return FALSE;
}

static int GetP_ICoreWebView2AcceleratorKeyPressedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2AcceleratorKeyPressedEventArgs* p = (ICoreWebView2AcceleratorKeyPressedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "KeyEventKind")) { COREWEBVIEW2_KEY_EVENT_KIND v{}; if (FAILED(p->get_KeyEventKind(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "VirtualKey")) { UINT v{}; if (FAILED(p->get_VirtualKey(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Handled")) { BOOL v{}; if (FAILED(p->get_Handled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2AcceleratorKeyPressedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2AcceleratorKeyPressedEventArgs* p = (ICoreWebView2AcceleratorKeyPressedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2AcceleratorKeyPressedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2AcceleratorKeyPressedEventArgs* p = (ICoreWebView2AcceleratorKeyPressedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Handled")) { return SUCCEEDED(p->put_Handled((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2AcceleratorKeyPressedEventArgs(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2AcceleratorKeyPressedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2AcceleratorKeyPressedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2AcceleratorKeyPressedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "KeyEventKind",         "VirtualKey",         "Handled", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2MoveFocusRequestedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2MoveFocusRequestedEventArgs* p = (ICoreWebView2MoveFocusRequestedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Reason")) { COREWEBVIEW2_MOVE_FOCUS_REASON v{}; if (FAILED(p->get_Reason(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Handled")) { BOOL v{}; if (FAILED(p->get_Handled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2MoveFocusRequestedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2MoveFocusRequestedEventArgs* p = (ICoreWebView2MoveFocusRequestedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2MoveFocusRequestedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2MoveFocusRequestedEventArgs* p = (ICoreWebView2MoveFocusRequestedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Handled")) { return SUCCEEDED(p->put_Handled((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2MoveFocusRequestedEventArgs(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2MoveFocusRequestedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2MoveFocusRequestedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2MoveFocusRequestedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "Reason",         "Handled", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2ContentLoadingEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2ContentLoadingEventArgs* p = (ICoreWebView2ContentLoadingEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "IsErrorPage")) { BOOL v{}; if (FAILED(p->get_IsErrorPage(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "NavigationId")) { UINT64 v{}; if (FAILED(p->get_NavigationId(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2ContentLoadingEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ContentLoadingEventArgs* p = (ICoreWebView2ContentLoadingEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2ContentLoadingEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2ContentLoadingEventArgs* p = (ICoreWebView2ContentLoadingEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2ContentLoadingEventArgs(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2ContentLoadingEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2ContentLoadingEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2ContentLoadingEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "IsErrorPage",         "NavigationId", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2DOMContentLoadedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2DOMContentLoadedEventArgs* p = (ICoreWebView2DOMContentLoadedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "NavigationId")) { UINT64 v{}; if (FAILED(p->get_NavigationId(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2DOMContentLoadedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2DOMContentLoadedEventArgs* p = (ICoreWebView2DOMContentLoadedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2DOMContentLoadedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2DOMContentLoadedEventArgs* p = (ICoreWebView2DOMContentLoadedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2DOMContentLoadedEventArgs(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2DOMContentLoadedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2DOMContentLoadedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2DOMContentLoadedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "NavigationId", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2NavigationCompletedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2NavigationCompletedEventArgs* p = (ICoreWebView2NavigationCompletedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "IsSuccess")) { BOOL v{}; if (FAILED(p->get_IsSuccess(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "WebErrorStatus")) { COREWEBVIEW2_WEB_ERROR_STATUS v{}; if (FAILED(p->get_WebErrorStatus(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "NavigationId")) { UINT64 v{}; if (FAILED(p->get_NavigationId(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2NavigationCompletedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2NavigationCompletedEventArgs* p = (ICoreWebView2NavigationCompletedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2NavigationCompletedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2NavigationCompletedEventArgs* p = (ICoreWebView2NavigationCompletedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2NavigationCompletedEventArgs(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2NavigationCompletedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2NavigationCompletedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2NavigationCompletedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "IsSuccess",         "WebErrorStatus",         "NavigationId", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2NavigationStartingEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2NavigationStartingEventArgs* p = (ICoreWebView2NavigationStartingEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Uri")) { LPWSTR v = nullptr; if (FAILED(p->get_Uri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "IsUserInitiated")) { BOOL v{}; if (FAILED(p->get_IsUserInitiated(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "IsRedirected")) { BOOL v{}; if (FAILED(p->get_IsRedirected(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Cancel")) { BOOL v{}; if (FAILED(p->get_Cancel(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "NavigationId")) { UINT64 v{}; if (FAILED(p->get_NavigationId(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2NavigationStartingEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2NavigationStartingEventArgs* p = (ICoreWebView2NavigationStartingEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2NavigationStartingEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2NavigationStartingEventArgs* p = (ICoreWebView2NavigationStartingEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Cancel")) { return SUCCEEDED(p->put_Cancel((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2NavigationStartingEventArgs(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2NavigationStartingEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2NavigationStartingEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2NavigationStartingEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "Uri",         "IsUserInitiated",         "IsRedirected",         "Cancel",         "NavigationId", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2LaunchingExternalUriSchemeEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2LaunchingExternalUriSchemeEventArgs* p = (ICoreWebView2LaunchingExternalUriSchemeEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Uri")) { LPWSTR v = nullptr; if (FAILED(p->get_Uri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "InitiatingOrigin")) { LPWSTR v = nullptr; if (FAILED(p->get_InitiatingOrigin(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "IsUserInitiated")) { BOOL v{}; if (FAILED(p->get_IsUserInitiated(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Cancel")) { BOOL v{}; if (FAILED(p->get_Cancel(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2LaunchingExternalUriSchemeEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2LaunchingExternalUriSchemeEventArgs* p = (ICoreWebView2LaunchingExternalUriSchemeEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2LaunchingExternalUriSchemeEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2LaunchingExternalUriSchemeEventArgs* p = (ICoreWebView2LaunchingExternalUriSchemeEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Cancel")) { return SUCCEEDED(p->put_Cancel((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2LaunchingExternalUriSchemeEventArgs(void* pV, void** ppOut)
{
    ICoreWebView2LaunchingExternalUriSchemeEventArgs* p = (ICoreWebView2LaunchingExternalUriSchemeEventArgs*)pV; UNREF(ppOut);
    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;
    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;
}

static int ChildAt_ICoreWebView2LaunchingExternalUriSchemeEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2LaunchingExternalUriSchemeEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2LaunchingExternalUriSchemeEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "Uri",         "InitiatingOrigin",         "IsUserInitiated",         "Cancel", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2PermissionRequestedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2PermissionRequestedEventArgs* p = (ICoreWebView2PermissionRequestedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Uri")) { LPWSTR v = nullptr; if (FAILED(p->get_Uri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "PermissionKind")) { COREWEBVIEW2_PERMISSION_KIND v{}; if (FAILED(p->get_PermissionKind(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "IsUserInitiated")) { BOOL v{}; if (FAILED(p->get_IsUserInitiated(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "State")) { COREWEBVIEW2_PERMISSION_STATE v{}; if (FAILED(p->get_State(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2PermissionRequestedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2PermissionRequestedEventArgs* p = (ICoreWebView2PermissionRequestedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2PermissionRequestedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2PermissionRequestedEventArgs* p = (ICoreWebView2PermissionRequestedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "State")) { return SUCCEEDED(p->put_State((COREWEBVIEW2_PERMISSION_STATE)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2PermissionRequestedEventArgs(void* pV, void** ppOut)
{
    ICoreWebView2PermissionRequestedEventArgs* p = (ICoreWebView2PermissionRequestedEventArgs*)pV; UNREF(ppOut);
    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;
    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;
}

static int ChildAt_ICoreWebView2PermissionRequestedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2PermissionRequestedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2PermissionRequestedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "Uri",         "PermissionKind",         "IsUserInitiated",         "State", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2ProcessFailedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2ProcessFailedEventArgs* p = (ICoreWebView2ProcessFailedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "ProcessFailedKind")) { COREWEBVIEW2_PROCESS_FAILED_KIND v{}; if (FAILED(p->get_ProcessFailedKind(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2ProcessFailedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ProcessFailedEventArgs* p = (ICoreWebView2ProcessFailedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2ProcessFailedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2ProcessFailedEventArgs* p = (ICoreWebView2ProcessFailedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2ProcessFailedEventArgs(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2ProcessFailedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2ProcessFailedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2ProcessFailedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "ProcessFailedKind", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2SaveAsUIShowingEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2SaveAsUIShowingEventArgs* p = (ICoreWebView2SaveAsUIShowingEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "ContentMimeType")) { LPWSTR v = nullptr; if (FAILED(p->get_ContentMimeType(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Cancel")) { BOOL v{}; if (FAILED(p->get_Cancel(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "SuppressDefaultDialog")) { BOOL v{}; if (FAILED(p->get_SuppressDefaultDialog(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "SaveAsFilePath")) { LPWSTR v = nullptr; if (FAILED(p->get_SaveAsFilePath(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "AllowReplace")) { BOOL v{}; if (FAILED(p->get_AllowReplace(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Kind")) { COREWEBVIEW2_SAVE_AS_KIND v{}; if (FAILED(p->get_Kind(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2SaveAsUIShowingEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2SaveAsUIShowingEventArgs* p = (ICoreWebView2SaveAsUIShowingEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2SaveAsUIShowingEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2SaveAsUIShowingEventArgs* p = (ICoreWebView2SaveAsUIShowingEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Cancel")) { return SUCCEEDED(p->put_Cancel((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "SuppressDefaultDialog")) { return SUCCEEDED(p->put_SuppressDefaultDialog((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "SaveAsFilePath")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return 0; return SUCCEEDED(p->put_SaveAsFilePath((LPCWSTR)wz)) ? 1 : 0; }
    if (!_stricmp(name, "AllowReplace")) { return SUCCEEDED(p->put_AllowReplace((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "Kind")) { return SUCCEEDED(p->put_Kind((COREWEBVIEW2_SAVE_AS_KIND)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2SaveAsUIShowingEventArgs(void* pV, void** ppOut)
{
    ICoreWebView2SaveAsUIShowingEventArgs* p = (ICoreWebView2SaveAsUIShowingEventArgs*)pV; UNREF(ppOut);
    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;
    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;
}

static int ChildAt_ICoreWebView2SaveAsUIShowingEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2SaveAsUIShowingEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2SaveAsUIShowingEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "ContentMimeType",         "Cancel",         "SuppressDefaultDialog",         "SaveAsFilePath",         "AllowReplace",         "Kind",

    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2SaveFileSecurityCheckStartingEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2SaveFileSecurityCheckStartingEventArgs* p = (ICoreWebView2SaveFileSecurityCheckStartingEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "CancelSave")) { BOOL v{}; if (FAILED(p->get_CancelSave(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "DocumentOriginUri")) { LPWSTR v = nullptr; if (FAILED(p->get_DocumentOriginUri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "FileExtension")) { LPWSTR v = nullptr; if (FAILED(p->get_FileExtension(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "FilePath")) { LPWSTR v = nullptr; if (FAILED(p->get_FilePath(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "SuppressDefaultPolicy")) { BOOL v{}; if (FAILED(p->get_SuppressDefaultPolicy(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2SaveFileSecurityCheckStartingEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2SaveFileSecurityCheckStartingEventArgs* p = (ICoreWebView2SaveFileSecurityCheckStartingEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2SaveFileSecurityCheckStartingEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2SaveFileSecurityCheckStartingEventArgs* p = (ICoreWebView2SaveFileSecurityCheckStartingEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "CancelSave")) { return SUCCEEDED(p->put_CancelSave((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "SuppressDefaultPolicy")) { return SUCCEEDED(p->put_SuppressDefaultPolicy((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2SaveFileSecurityCheckStartingEventArgs(void* pV, void** ppOut)
{
    ICoreWebView2SaveFileSecurityCheckStartingEventArgs* p = (ICoreWebView2SaveFileSecurityCheckStartingEventArgs*)pV; UNREF(ppOut);
    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;
    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;
}

static int ChildAt_ICoreWebView2SaveFileSecurityCheckStartingEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2SaveFileSecurityCheckStartingEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2SaveFileSecurityCheckStartingEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "CancelSave",         "DocumentOriginUri",         "FileExtension",         "FilePath",         "SuppressDefaultPolicy", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2ScriptDialogOpeningEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2ScriptDialogOpeningEventArgs* p = (ICoreWebView2ScriptDialogOpeningEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Uri")) { LPWSTR v = nullptr; if (FAILED(p->get_Uri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Kind")) { COREWEBVIEW2_SCRIPT_DIALOG_KIND v{}; if (FAILED(p->get_Kind(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Message")) { LPWSTR v = nullptr; if (FAILED(p->get_Message(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "DefaultText")) { LPWSTR v = nullptr; if (FAILED(p->get_DefaultText(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "ResultText")) { LPWSTR v = nullptr; if (FAILED(p->get_ResultText(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    return 0;
}

static int Child_ICoreWebView2ScriptDialogOpeningEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ScriptDialogOpeningEventArgs* p = (ICoreWebView2ScriptDialogOpeningEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2ScriptDialogOpeningEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2ScriptDialogOpeningEventArgs* p = (ICoreWebView2ScriptDialogOpeningEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "ResultText")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return 0; return SUCCEEDED(p->put_ResultText((LPCWSTR)wz)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2ScriptDialogOpeningEventArgs(void* pV, void** ppOut)
{
    ICoreWebView2ScriptDialogOpeningEventArgs* p = (ICoreWebView2ScriptDialogOpeningEventArgs*)pV; UNREF(ppOut);
    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;
    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;
}

static int ChildAt_ICoreWebView2ScriptDialogOpeningEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2ScriptDialogOpeningEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2ScriptDialogOpeningEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "Uri",         "Kind",         "Message",         "DefaultText",         "ResultText", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2SourceChangedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2SourceChangedEventArgs* p = (ICoreWebView2SourceChangedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "IsNewDocument")) { BOOL v{}; if (FAILED(p->get_IsNewDocument(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2SourceChangedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2SourceChangedEventArgs* p = (ICoreWebView2SourceChangedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2SourceChangedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2SourceChangedEventArgs* p = (ICoreWebView2SourceChangedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2SourceChangedEventArgs(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2SourceChangedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2SourceChangedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2SourceChangedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "IsNewDocument", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2WebMessageReceivedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2WebMessageReceivedEventArgs* p = (ICoreWebView2WebMessageReceivedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Source")) { LPWSTR v = nullptr; if (FAILED(p->get_Source(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "WebMessageAsJson")) { LPWSTR v = nullptr; if (FAILED(p->get_WebMessageAsJson(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    return 0;
}

static int Child_ICoreWebView2WebMessageReceivedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2WebMessageReceivedEventArgs* p = (ICoreWebView2WebMessageReceivedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2WebMessageReceivedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2WebMessageReceivedEventArgs* p = (ICoreWebView2WebMessageReceivedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2WebMessageReceivedEventArgs(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2WebMessageReceivedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2WebMessageReceivedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2WebMessageReceivedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "Source",         "WebMessageAsJson", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2BrowserProcessExitedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2BrowserProcessExitedEventArgs* p = (ICoreWebView2BrowserProcessExitedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "BrowserProcessExitKind")) { COREWEBVIEW2_BROWSER_PROCESS_EXIT_KIND v{}; if (FAILED(p->get_BrowserProcessExitKind(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "BrowserProcessId")) { UINT32 v{}; if (FAILED(p->get_BrowserProcessId(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2BrowserProcessExitedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2BrowserProcessExitedEventArgs* p = (ICoreWebView2BrowserProcessExitedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2BrowserProcessExitedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2BrowserProcessExitedEventArgs* p = (ICoreWebView2BrowserProcessExitedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2BrowserProcessExitedEventArgs(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2BrowserProcessExitedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2BrowserProcessExitedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2BrowserProcessExitedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "BrowserProcessExitKind",         "BrowserProcessId", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2PermissionRequestedEventArgs2(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2PermissionRequestedEventArgs2* p = (ICoreWebView2PermissionRequestedEventArgs2*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Handled")) { BOOL v{}; if (FAILED(p->get_Handled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2PermissionRequestedEventArgs2(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2PermissionRequestedEventArgs2* p = (ICoreWebView2PermissionRequestedEventArgs2*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2PermissionRequestedEventArgs2(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2PermissionRequestedEventArgs2* p = (ICoreWebView2PermissionRequestedEventArgs2*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Handled")) { return SUCCEEDED(p->put_Handled((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2PermissionRequestedEventArgs2(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2PermissionRequestedEventArgs2(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2PermissionRequestedEventArgs2(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2PermissionRequestedEventArgs2(char* buf, INT cap)
{
    static const char* items[] = {
        "Handled", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2DevToolsProtocolEventReceivedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2DevToolsProtocolEventReceivedEventArgs* p = (ICoreWebView2DevToolsProtocolEventReceivedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "ParameterObjectAsJson")) { LPWSTR v = nullptr; if (FAILED(p->get_ParameterObjectAsJson(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    return 0;
}

static int Child_ICoreWebView2DevToolsProtocolEventReceivedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2DevToolsProtocolEventReceivedEventArgs* p = (ICoreWebView2DevToolsProtocolEventReceivedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2DevToolsProtocolEventReceivedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2DevToolsProtocolEventReceivedEventArgs* p = (ICoreWebView2DevToolsProtocolEventReceivedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2DevToolsProtocolEventReceivedEventArgs(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2DevToolsProtocolEventReceivedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2DevToolsProtocolEventReceivedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2DevToolsProtocolEventReceivedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "ParameterObjectAsJson", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2ProcessExtendedInfoCollection(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2ProcessExtendedInfoCollection* p = (ICoreWebView2ProcessExtendedInfoCollection*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Count")) { UINT32 v{}; if (FAILED(p->get_Count(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2ProcessExtendedInfoCollection(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ProcessExtendedInfoCollection* p = (ICoreWebView2ProcessExtendedInfoCollection*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2ProcessExtendedInfoCollection(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2ProcessExtendedInfoCollection* p = (ICoreWebView2ProcessExtendedInfoCollection*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2ProcessExtendedInfoCollection(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2ProcessExtendedInfoCollection(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2ProcessExtendedInfoCollection(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2ProcessExtendedInfoCollection(char* buf, INT cap)
{
    static const char* items[] = {
        "Count", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2FrameInfoCollection(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2FrameInfoCollection* p = (ICoreWebView2FrameInfoCollection*)pV; UNREF(outInt); UNREF(pDbl);
    return 0;
}

static int Child_ICoreWebView2FrameInfoCollection(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2FrameInfoCollection* p = (ICoreWebView2FrameInfoCollection*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2FrameInfoCollection(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2FrameInfoCollection* p = (ICoreWebView2FrameInfoCollection*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2FrameInfoCollection(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2FrameInfoCollection(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2FrameInfoCollection(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2FrameInfoCollection(char* buf, INT cap)
{ UNREF(buf); UNREF(cap); if (buf != NULL && cap > 0) buf[0] = '\0'; }

static int GetP_ICoreWebView2Cookie(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2Cookie* p = (ICoreWebView2Cookie*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Name")) { LPWSTR v = nullptr; if (FAILED(p->get_Name(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Value")) { LPWSTR v = nullptr; if (FAILED(p->get_Value(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Domain")) { LPWSTR v = nullptr; if (FAILED(p->get_Domain(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Path")) { LPWSTR v = nullptr; if (FAILED(p->get_Path(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Expires")) { double v{}; if (FAILED(p->get_Expires(&v))) return 0; if (pDbl) *pDbl = (double)v; if (outInt) *outInt = (INT)v; return 1; }
    if (!_stricmp(name, "IsHttpOnly")) { BOOL v{}; if (FAILED(p->get_IsHttpOnly(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "SameSite")) { COREWEBVIEW2_COOKIE_SAME_SITE_KIND v{}; if (FAILED(p->get_SameSite(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "IsSecure")) { BOOL v{}; if (FAILED(p->get_IsSecure(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "IsSession")) { BOOL v{}; if (FAILED(p->get_IsSession(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2Cookie(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2Cookie* p = (ICoreWebView2Cookie*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2Cookie(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2Cookie* p = (ICoreWebView2Cookie*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Value")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return 0; return SUCCEEDED(p->put_Value((LPCWSTR)wz)) ? 1 : 0; }
    if (!_stricmp(name, "Expires")) { return SUCCEEDED(p->put_Expires((double)inDouble)) ? 1 : 0; }
    if (!_stricmp(name, "IsHttpOnly")) { return SUCCEEDED(p->put_IsHttpOnly((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "SameSite")) { return SUCCEEDED(p->put_SameSite((COREWEBVIEW2_COOKIE_SAME_SITE_KIND)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "IsSecure")) { return SUCCEEDED(p->put_IsSecure((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2Cookie(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2Cookie(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2Cookie(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2Cookie(char* buf, INT cap)
{
    static const char* items[] = {
        "Name",         "Value",         "Domain",         "Path",         "Expires",         "IsHttpOnly",
        "SameSite",         "IsSecure",         "IsSession", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2Profile(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2Profile* p = (ICoreWebView2Profile*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "ProfileName")) { LPWSTR v = nullptr; if (FAILED(p->get_ProfileName(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "IsInPrivateModeEnabled")) { BOOL v{}; if (FAILED(p->get_IsInPrivateModeEnabled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "ProfilePath")) { LPWSTR v = nullptr; if (FAILED(p->get_ProfilePath(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "DefaultDownloadFolderPath")) { LPWSTR v = nullptr; if (FAILED(p->get_DefaultDownloadFolderPath(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "PreferredColorScheme")) { COREWEBVIEW2_PREFERRED_COLOR_SCHEME v{}; if (FAILED(p->get_PreferredColorScheme(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2Profile(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2Profile* p = (ICoreWebView2Profile*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2Profile(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2Profile* p = (ICoreWebView2Profile*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "DefaultDownloadFolderPath")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return 0; return SUCCEEDED(p->put_DefaultDownloadFolderPath((LPCWSTR)wz)) ? 1 : 0; }
    if (!_stricmp(name, "PreferredColorScheme")) { return SUCCEEDED(p->put_PreferredColorScheme((COREWEBVIEW2_PREFERRED_COLOR_SCHEME)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2Profile(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2Profile(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2Profile(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2Profile(char* buf, INT cap)
{
    static const char* items[] = {
        "ProfileName",         "IsInPrivateModeEnabled",         "ProfilePath",         "DefaultDownloadFolderPath",         "PreferredColorScheme", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2DownloadOperation(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2DownloadOperation* p = (ICoreWebView2DownloadOperation*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Uri")) { LPWSTR v = nullptr; if (FAILED(p->get_Uri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "ContentDisposition")) { LPWSTR v = nullptr; if (FAILED(p->get_ContentDisposition(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "MimeType")) { LPWSTR v = nullptr; if (FAILED(p->get_MimeType(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "TotalBytesToReceive")) { INT64 v{}; if (FAILED(p->get_TotalBytesToReceive(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "BytesReceived")) { INT64 v{}; if (FAILED(p->get_BytesReceived(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "EstimatedEndTime")) { LPWSTR v = nullptr; if (FAILED(p->get_EstimatedEndTime(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "ResultFilePath")) { LPWSTR v = nullptr; if (FAILED(p->get_ResultFilePath(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "State")) { COREWEBVIEW2_DOWNLOAD_STATE v{}; if (FAILED(p->get_State(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "InterruptReason")) { COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON v{}; if (FAILED(p->get_InterruptReason(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "CanResume")) { BOOL v{}; if (FAILED(p->get_CanResume(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2DownloadOperation(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2DownloadOperation* p = (ICoreWebView2DownloadOperation*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2DownloadOperation(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2DownloadOperation* p = (ICoreWebView2DownloadOperation*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2DownloadOperation(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2DownloadOperation(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2DownloadOperation(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2DownloadOperation(char* buf, INT cap)
{
    static const char* items[] = {
        "Uri",         "ContentDisposition",         "MimeType",         "TotalBytesToReceive",         "BytesReceived",         "EstimatedEndTime",
        "ResultFilePath",         "State",         "InterruptReason",         "CanResume", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2Frame(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2Frame* p = (ICoreWebView2Frame*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Name")) { LPWSTR v = nullptr; if (FAILED(p->get_Name(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    return 0;
}

static int Child_ICoreWebView2Frame(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2Frame* p = (ICoreWebView2Frame*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2Frame(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2Frame* p = (ICoreWebView2Frame*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2Frame(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2Frame(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2Frame(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2Frame(char* buf, INT cap)
{
    static const char* items[] = {
        "Name", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2WebResourceResponse(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2WebResourceResponse* p = (ICoreWebView2WebResourceResponse*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "StatusCode")) { int v{}; if (FAILED(p->get_StatusCode(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "ReasonPhrase")) { LPWSTR v = nullptr; if (FAILED(p->get_ReasonPhrase(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    return 0;
}

static int Child_ICoreWebView2WebResourceResponse(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2WebResourceResponse* p = (ICoreWebView2WebResourceResponse*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2WebResourceResponse(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2WebResourceResponse* p = (ICoreWebView2WebResourceResponse*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "StatusCode")) { return SUCCEEDED(p->put_StatusCode((int)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "ReasonPhrase")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return 0; return SUCCEEDED(p->put_ReasonPhrase((LPCWSTR)wz)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2WebResourceResponse(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2WebResourceResponse(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2WebResourceResponse(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2WebResourceResponse(char* buf, INT cap)
{
    static const char* items[] = {
        "StatusCode",         "ReasonPhrase", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2WebResourceRequest(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2WebResourceRequest* p = (ICoreWebView2WebResourceRequest*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Uri")) { LPWSTR v = nullptr; if (FAILED(p->get_Uri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Method")) { LPWSTR v = nullptr; if (FAILED(p->get_Method(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    return 0;
}

static int Child_ICoreWebView2WebResourceRequest(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2WebResourceRequest* p = (ICoreWebView2WebResourceRequest*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2WebResourceRequest(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2WebResourceRequest* p = (ICoreWebView2WebResourceRequest*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Uri")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return 0; return SUCCEEDED(p->put_Uri((LPCWSTR)wz)) ? 1 : 0; }
    if (!_stricmp(name, "Method")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return 0; return SUCCEEDED(p->put_Method((LPCWSTR)wz)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2WebResourceRequest(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2WebResourceRequest(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2WebResourceRequest(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2WebResourceRequest(char* buf, INT cap)
{
    static const char* items[] = {
        "Uri",         "Method", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2BrowserExtension(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2BrowserExtension* p = (ICoreWebView2BrowserExtension*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Id")) { LPWSTR v = nullptr; if (FAILED(p->get_Id(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Name")) { LPWSTR v = nullptr; if (FAILED(p->get_Name(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "IsEnabled")) { BOOL v{}; if (FAILED(p->get_IsEnabled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2BrowserExtension(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2BrowserExtension* p = (ICoreWebView2BrowserExtension*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2BrowserExtension(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2BrowserExtension* p = (ICoreWebView2BrowserExtension*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2BrowserExtension(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2BrowserExtension(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2BrowserExtension(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2BrowserExtension(char* buf, INT cap)
{
    static const char* items[] = {
        "Id",         "Name",         "IsEnabled", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2ProcessInfo(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2ProcessInfo* p = (ICoreWebView2ProcessInfo*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "ProcessId")) { INT32 v{}; if (FAILED(p->get_ProcessId(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Kind")) { COREWEBVIEW2_PROCESS_KIND v{}; if (FAILED(p->get_Kind(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2ProcessInfo(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ProcessInfo* p = (ICoreWebView2ProcessInfo*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2ProcessInfo(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2ProcessInfo* p = (ICoreWebView2ProcessInfo*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2ProcessInfo(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2ProcessInfo(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2ProcessInfo(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2ProcessInfo(char* buf, INT cap)
{
    static const char* items[] = {
        "ProcessId",         "Kind", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2BasicAuthenticationResponse(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2BasicAuthenticationResponse* p = (ICoreWebView2BasicAuthenticationResponse*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "UserName")) { LPWSTR v = nullptr; if (FAILED(p->get_UserName(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Password")) { LPWSTR v = nullptr; if (FAILED(p->get_Password(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    return 0;
}

static int Child_ICoreWebView2BasicAuthenticationResponse(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2BasicAuthenticationResponse* p = (ICoreWebView2BasicAuthenticationResponse*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2BasicAuthenticationResponse(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2BasicAuthenticationResponse* p = (ICoreWebView2BasicAuthenticationResponse*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "UserName")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return 0; return SUCCEEDED(p->put_UserName((LPCWSTR)wz)) ? 1 : 0; }
    if (!_stricmp(name, "Password")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return 0; return SUCCEEDED(p->put_Password((LPCWSTR)wz)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2BasicAuthenticationResponse(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2BasicAuthenticationResponse(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2BasicAuthenticationResponse(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2BasicAuthenticationResponse(char* buf, INT cap)
{
    static const char* items[] = {
        "UserName",         "Password", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2StringCollection(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2StringCollection* p = (ICoreWebView2StringCollection*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Count")) { UINT32 v{}; if (FAILED(p->get_Count(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2StringCollection(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2StringCollection* p = (ICoreWebView2StringCollection*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2StringCollection(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2StringCollection* p = (ICoreWebView2StringCollection*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2StringCollection(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2StringCollection(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2StringCollection(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2StringCollection(char* buf, INT cap)
{
    static const char* items[] = {
        "Count", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2ContextMenuItemCollection(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2ContextMenuItemCollection* p = (ICoreWebView2ContextMenuItemCollection*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Count")) { UINT32 v{}; if (FAILED(p->get_Count(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2ContextMenuItemCollection(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ContextMenuItemCollection* p = (ICoreWebView2ContextMenuItemCollection*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2ContextMenuItemCollection(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2ContextMenuItemCollection* p = (ICoreWebView2ContextMenuItemCollection*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2ContextMenuItemCollection(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2ContextMenuItemCollection(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2ContextMenuItemCollection(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2ContextMenuItemCollection(char* buf, INT cap)
{
    static const char* items[] = {
        "Count", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2ContextMenuTarget(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2ContextMenuTarget* p = (ICoreWebView2ContextMenuTarget*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Kind")) { COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND v{}; if (FAILED(p->get_Kind(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "IsEditable")) { BOOL v{}; if (FAILED(p->get_IsEditable(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "IsRequestedForMainFrame")) { BOOL v{}; if (FAILED(p->get_IsRequestedForMainFrame(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "PageUri")) { LPWSTR v = nullptr; if (FAILED(p->get_PageUri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "FrameUri")) { LPWSTR v = nullptr; if (FAILED(p->get_FrameUri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "HasLinkUri")) { BOOL v{}; if (FAILED(p->get_HasLinkUri(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "LinkUri")) { LPWSTR v = nullptr; if (FAILED(p->get_LinkUri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "HasLinkText")) { BOOL v{}; if (FAILED(p->get_HasLinkText(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "LinkText")) { LPWSTR v = nullptr; if (FAILED(p->get_LinkText(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "HasSourceUri")) { BOOL v{}; if (FAILED(p->get_HasSourceUri(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "SourceUri")) { LPWSTR v = nullptr; if (FAILED(p->get_SourceUri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "HasSelection")) { BOOL v{}; if (FAILED(p->get_HasSelection(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "SelectionText")) { LPWSTR v = nullptr; if (FAILED(p->get_SelectionText(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    return 0;
}

static int Child_ICoreWebView2ContextMenuTarget(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ContextMenuTarget* p = (ICoreWebView2ContextMenuTarget*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2ContextMenuTarget(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2ContextMenuTarget* p = (ICoreWebView2ContextMenuTarget*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2ContextMenuTarget(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2ContextMenuTarget(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2ContextMenuTarget(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2ContextMenuTarget(char* buf, INT cap)
{
    static const char* items[] = {
        "Kind",         "IsEditable",         "IsRequestedForMainFrame",         "PageUri",         "FrameUri",         "HasLinkUri",
        "LinkUri",         "HasLinkText",         "LinkText",         "HasSourceUri",         "SourceUri",         "HasSelection",
        "SelectionText", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2FrameInfo(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2FrameInfo* p = (ICoreWebView2FrameInfo*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Name")) { LPWSTR v = nullptr; if (FAILED(p->get_Name(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Source")) { LPWSTR v = nullptr; if (FAILED(p->get_Source(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    return 0;
}

static int Child_ICoreWebView2FrameInfo(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2FrameInfo* p = (ICoreWebView2FrameInfo*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2FrameInfo(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2FrameInfo* p = (ICoreWebView2FrameInfo*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2FrameInfo(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2FrameInfo(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2FrameInfo(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2FrameInfo(char* buf, INT cap)
{
    static const char* items[] = {
        "Name",         "Source", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2DedicatedWorker(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2DedicatedWorker* p = (ICoreWebView2DedicatedWorker*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "ScriptUri")) { LPWSTR v = nullptr; if (FAILED(p->get_ScriptUri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    return 0;
}

static int Child_ICoreWebView2DedicatedWorker(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2DedicatedWorker* p = (ICoreWebView2DedicatedWorker*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2DedicatedWorker(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2DedicatedWorker* p = (ICoreWebView2DedicatedWorker*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2DedicatedWorker(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2DedicatedWorker(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2DedicatedWorker(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2DedicatedWorker(char* buf, INT cap)
{
    static const char* items[] = {
        "ScriptUri", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2WindowFeatures(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2WindowFeatures* p = (ICoreWebView2WindowFeatures*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "HasPosition")) { BOOL v{}; if (FAILED(p->get_HasPosition(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "HasSize")) { BOOL v{}; if (FAILED(p->get_HasSize(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Left")) { UINT32 v{}; if (FAILED(p->get_Left(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Top")) { UINT32 v{}; if (FAILED(p->get_Top(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Height")) { UINT32 v{}; if (FAILED(p->get_Height(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Width")) { UINT32 v{}; if (FAILED(p->get_Width(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "ShouldDisplayMenuBar")) { BOOL v{}; if (FAILED(p->get_ShouldDisplayMenuBar(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "ShouldDisplayStatus")) { BOOL v{}; if (FAILED(p->get_ShouldDisplayStatus(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "ShouldDisplayToolbar")) { BOOL v{}; if (FAILED(p->get_ShouldDisplayToolbar(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "ShouldDisplayScrollBars")) { BOOL v{}; if (FAILED(p->get_ShouldDisplayScrollBars(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2WindowFeatures(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2WindowFeatures* p = (ICoreWebView2WindowFeatures*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2WindowFeatures(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2WindowFeatures* p = (ICoreWebView2WindowFeatures*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2WindowFeatures(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2WindowFeatures(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2WindowFeatures(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2WindowFeatures(char* buf, INT cap)
{
    static const char* items[] = {
        "HasPosition",         "HasSize",         "Left",         "Top",         "Height",         "Width",
        "ShouldDisplayMenuBar",         "ShouldDisplayStatus",         "ShouldDisplayToolbar",         "ShouldDisplayScrollBars", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2Notification(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2Notification* p = (ICoreWebView2Notification*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Body")) { LPWSTR v = nullptr; if (FAILED(p->get_Body(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Direction")) { COREWEBVIEW2_TEXT_DIRECTION_KIND v{}; if (FAILED(p->get_Direction(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Language")) { LPWSTR v = nullptr; if (FAILED(p->get_Language(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Tag")) { LPWSTR v = nullptr; if (FAILED(p->get_Tag(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "IconUri")) { LPWSTR v = nullptr; if (FAILED(p->get_IconUri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Title")) { LPWSTR v = nullptr; if (FAILED(p->get_Title(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "BadgeUri")) { LPWSTR v = nullptr; if (FAILED(p->get_BadgeUri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "BodyImageUri")) { LPWSTR v = nullptr; if (FAILED(p->get_BodyImageUri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "ShouldRenotify")) { BOOL v{}; if (FAILED(p->get_ShouldRenotify(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "RequiresInteraction")) { BOOL v{}; if (FAILED(p->get_RequiresInteraction(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "IsSilent")) { BOOL v{}; if (FAILED(p->get_IsSilent(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Timestamp")) { double v{}; if (FAILED(p->get_Timestamp(&v))) return 0; if (pDbl) *pDbl = (double)v; if (outInt) *outInt = (INT)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2Notification(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2Notification* p = (ICoreWebView2Notification*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2Notification(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2Notification* p = (ICoreWebView2Notification*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2Notification(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2Notification(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2Notification(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2Notification(char* buf, INT cap)
{
    static const char* items[] = {
        "Body",         "Direction",         "Language",         "Tag",         "IconUri",         "Title",
        "BadgeUri",         "BodyImageUri",         "ShouldRenotify",         "RequiresInteraction",         "IsSilent",         "Timestamp",

    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2WebResourceResponseView(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2WebResourceResponseView* p = (ICoreWebView2WebResourceResponseView*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "StatusCode")) { int v{}; if (FAILED(p->get_StatusCode(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "ReasonPhrase")) { LPWSTR v = nullptr; if (FAILED(p->get_ReasonPhrase(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    return 0;
}

static int Child_ICoreWebView2WebResourceResponseView(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2WebResourceResponseView* p = (ICoreWebView2WebResourceResponseView*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2WebResourceResponseView(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2WebResourceResponseView* p = (ICoreWebView2WebResourceResponseView*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2WebResourceResponseView(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2WebResourceResponseView(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2WebResourceResponseView(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2WebResourceResponseView(char* buf, INT cap)
{
    static const char* items[] = {
        "StatusCode",         "ReasonPhrase", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2Settings(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2Settings* p = (ICoreWebView2Settings*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "IsScriptEnabled")) { BOOL v{}; if (FAILED(p->get_IsScriptEnabled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "IsWebMessageEnabled")) { BOOL v{}; if (FAILED(p->get_IsWebMessageEnabled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "AreDefaultScriptDialogsEnabled")) { BOOL v{}; if (FAILED(p->get_AreDefaultScriptDialogsEnabled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "IsStatusBarEnabled")) { BOOL v{}; if (FAILED(p->get_IsStatusBarEnabled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "AreDevToolsEnabled")) { BOOL v{}; if (FAILED(p->get_AreDevToolsEnabled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "AreDefaultContextMenusEnabled")) { BOOL v{}; if (FAILED(p->get_AreDefaultContextMenusEnabled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "AreHostObjectsAllowed")) { BOOL v{}; if (FAILED(p->get_AreHostObjectsAllowed(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "IsZoomControlEnabled")) { BOOL v{}; if (FAILED(p->get_IsZoomControlEnabled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "IsBuiltInErrorPageEnabled")) { BOOL v{}; if (FAILED(p->get_IsBuiltInErrorPageEnabled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2Settings(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2Settings* p = (ICoreWebView2Settings*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2Settings(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2Settings* p = (ICoreWebView2Settings*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "IsScriptEnabled")) { return SUCCEEDED(p->put_IsScriptEnabled((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "IsWebMessageEnabled")) { return SUCCEEDED(p->put_IsWebMessageEnabled((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "AreDefaultScriptDialogsEnabled")) { return SUCCEEDED(p->put_AreDefaultScriptDialogsEnabled((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "IsStatusBarEnabled")) { return SUCCEEDED(p->put_IsStatusBarEnabled((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "AreDevToolsEnabled")) { return SUCCEEDED(p->put_AreDevToolsEnabled((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "AreDefaultContextMenusEnabled")) { return SUCCEEDED(p->put_AreDefaultContextMenusEnabled((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "AreHostObjectsAllowed")) { return SUCCEEDED(p->put_AreHostObjectsAllowed((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "IsZoomControlEnabled")) { return SUCCEEDED(p->put_IsZoomControlEnabled((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "IsBuiltInErrorPageEnabled")) { return SUCCEEDED(p->put_IsBuiltInErrorPageEnabled((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2Settings(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2Settings(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2Settings(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2Settings(char* buf, INT cap)
{
    static const char* items[] = {
        "IsScriptEnabled",         "IsWebMessageEnabled",         "AreDefaultScriptDialogsEnabled",         "IsStatusBarEnabled",         "AreDevToolsEnabled",         "AreDefaultContextMenusEnabled",
        "AreHostObjectsAllowed",         "IsZoomControlEnabled",         "IsBuiltInErrorPageEnabled", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2BasicAuthenticationRequestedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2BasicAuthenticationRequestedEventArgs* p = (ICoreWebView2BasicAuthenticationRequestedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Uri")) { LPWSTR v = nullptr; if (FAILED(p->get_Uri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Challenge")) { LPWSTR v = nullptr; if (FAILED(p->get_Challenge(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Cancel")) { BOOL v{}; if (FAILED(p->get_Cancel(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2BasicAuthenticationRequestedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2BasicAuthenticationRequestedEventArgs* p = (ICoreWebView2BasicAuthenticationRequestedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "Response")) { ICoreWebView2BasicAuthenticationResponse* v = nullptr; if (FAILED(p->get_Response(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2BasicAuthenticationResponse"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2BasicAuthenticationRequestedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2BasicAuthenticationRequestedEventArgs* p = (ICoreWebView2BasicAuthenticationRequestedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Cancel")) { return SUCCEEDED(p->put_Cancel((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2BasicAuthenticationRequestedEventArgs(void* pV, void** ppOut)
{
    ICoreWebView2BasicAuthenticationRequestedEventArgs* p = (ICoreWebView2BasicAuthenticationRequestedEventArgs*)pV; UNREF(ppOut);
    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;
    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;
}

static int ChildAt_ICoreWebView2BasicAuthenticationRequestedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2BasicAuthenticationRequestedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2BasicAuthenticationRequestedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "Uri",         "Challenge",         "Response",         "Cancel", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2ContextMenuRequestedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2ContextMenuRequestedEventArgs* p = (ICoreWebView2ContextMenuRequestedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "SelectedCommandId")) { INT32 v{}; if (FAILED(p->get_SelectedCommandId(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Handled")) { BOOL v{}; if (FAILED(p->get_Handled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2ContextMenuRequestedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ContextMenuRequestedEventArgs* p = (ICoreWebView2ContextMenuRequestedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "MenuItems")) { ICoreWebView2ContextMenuItemCollection* v = nullptr; if (FAILED(p->get_MenuItems(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2ContextMenuItemCollection"; return 1; }
    if (!_stricmp(name, "ContextMenuTarget")) { ICoreWebView2ContextMenuTarget* v = nullptr; if (FAILED(p->get_ContextMenuTarget(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2ContextMenuTarget"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2ContextMenuRequestedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2ContextMenuRequestedEventArgs* p = (ICoreWebView2ContextMenuRequestedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "SelectedCommandId")) { return SUCCEEDED(p->put_SelectedCommandId((INT32)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "Handled")) { return SUCCEEDED(p->put_Handled((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2ContextMenuRequestedEventArgs(void* pV, void** ppOut)
{
    ICoreWebView2ContextMenuRequestedEventArgs* p = (ICoreWebView2ContextMenuRequestedEventArgs*)pV; UNREF(ppOut);
    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;
    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;
}

static int ChildAt_ICoreWebView2ContextMenuRequestedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2ContextMenuRequestedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2ContextMenuRequestedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "MenuItems",         "ContextMenuTarget",         "SelectedCommandId",         "Handled", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2DedicatedWorkerCreatedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2DedicatedWorkerCreatedEventArgs* p = (ICoreWebView2DedicatedWorkerCreatedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    return 0;
}

static int Child_ICoreWebView2DedicatedWorkerCreatedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2DedicatedWorkerCreatedEventArgs* p = (ICoreWebView2DedicatedWorkerCreatedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "OriginalSourceFrameInfo")) { ICoreWebView2FrameInfo* v = nullptr; if (FAILED(p->get_OriginalSourceFrameInfo(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2FrameInfo"; return 1; }
    if (!_stricmp(name, "Worker")) { ICoreWebView2DedicatedWorker* v = nullptr; if (FAILED(p->get_Worker(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2DedicatedWorker"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2DedicatedWorkerCreatedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2DedicatedWorkerCreatedEventArgs* p = (ICoreWebView2DedicatedWorkerCreatedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2DedicatedWorkerCreatedEventArgs(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2DedicatedWorkerCreatedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2DedicatedWorkerCreatedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2DedicatedWorkerCreatedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "OriginalSourceFrameInfo",         "Worker", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2DownloadStartingEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2DownloadStartingEventArgs* p = (ICoreWebView2DownloadStartingEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Cancel")) { BOOL v{}; if (FAILED(p->get_Cancel(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "ResultFilePath")) { LPWSTR v = nullptr; if (FAILED(p->get_ResultFilePath(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Handled")) { BOOL v{}; if (FAILED(p->get_Handled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2DownloadStartingEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2DownloadStartingEventArgs* p = (ICoreWebView2DownloadStartingEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "DownloadOperation")) { ICoreWebView2DownloadOperation* v = nullptr; if (FAILED(p->get_DownloadOperation(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2DownloadOperation"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2DownloadStartingEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2DownloadStartingEventArgs* p = (ICoreWebView2DownloadStartingEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Cancel")) { return SUCCEEDED(p->put_Cancel((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "ResultFilePath")) { WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return 0; return SUCCEEDED(p->put_ResultFilePath((LPCWSTR)wz)) ? 1 : 0; }
    if (!_stricmp(name, "Handled")) { return SUCCEEDED(p->put_Handled((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2DownloadStartingEventArgs(void* pV, void** ppOut)
{
    ICoreWebView2DownloadStartingEventArgs* p = (ICoreWebView2DownloadStartingEventArgs*)pV; UNREF(ppOut);
    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;
    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;
}

static int ChildAt_ICoreWebView2DownloadStartingEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2DownloadStartingEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2DownloadStartingEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "DownloadOperation",         "Cancel",         "ResultFilePath",         "Handled", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2FrameCreatedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2FrameCreatedEventArgs* p = (ICoreWebView2FrameCreatedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    return 0;
}

static int Child_ICoreWebView2FrameCreatedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2FrameCreatedEventArgs* p = (ICoreWebView2FrameCreatedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "Frame")) { ICoreWebView2Frame* v = nullptr; if (FAILED(p->get_Frame(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2Frame"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2FrameCreatedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2FrameCreatedEventArgs* p = (ICoreWebView2FrameCreatedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2FrameCreatedEventArgs(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2FrameCreatedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2FrameCreatedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2FrameCreatedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "Frame", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2NotificationReceivedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2NotificationReceivedEventArgs* p = (ICoreWebView2NotificationReceivedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "SenderOrigin")) { LPWSTR v = nullptr; if (FAILED(p->get_SenderOrigin(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Handled")) { BOOL v{}; if (FAILED(p->get_Handled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2NotificationReceivedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2NotificationReceivedEventArgs* p = (ICoreWebView2NotificationReceivedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "Notification")) { ICoreWebView2Notification* v = nullptr; if (FAILED(p->get_Notification(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2Notification"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2NotificationReceivedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2NotificationReceivedEventArgs* p = (ICoreWebView2NotificationReceivedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Handled")) { return SUCCEEDED(p->put_Handled((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2NotificationReceivedEventArgs(void* pV, void** ppOut)
{
    ICoreWebView2NotificationReceivedEventArgs* p = (ICoreWebView2NotificationReceivedEventArgs*)pV; UNREF(ppOut);
    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;
    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;
}

static int ChildAt_ICoreWebView2NotificationReceivedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2NotificationReceivedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2NotificationReceivedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "SenderOrigin",         "Notification",         "Handled", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2ScreenCaptureStartingEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2ScreenCaptureStartingEventArgs* p = (ICoreWebView2ScreenCaptureStartingEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Cancel")) { BOOL v{}; if (FAILED(p->get_Cancel(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Handled")) { BOOL v{}; if (FAILED(p->get_Handled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2ScreenCaptureStartingEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ScreenCaptureStartingEventArgs* p = (ICoreWebView2ScreenCaptureStartingEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "OriginalSourceFrameInfo")) { ICoreWebView2FrameInfo* v = nullptr; if (FAILED(p->get_OriginalSourceFrameInfo(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2FrameInfo"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2ScreenCaptureStartingEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2ScreenCaptureStartingEventArgs* p = (ICoreWebView2ScreenCaptureStartingEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Cancel")) { return SUCCEEDED(p->put_Cancel((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "Handled")) { return SUCCEEDED(p->put_Handled((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2ScreenCaptureStartingEventArgs(void* pV, void** ppOut)
{
    ICoreWebView2ScreenCaptureStartingEventArgs* p = (ICoreWebView2ScreenCaptureStartingEventArgs*)pV; UNREF(ppOut);
    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;
    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;
}

static int ChildAt_ICoreWebView2ScreenCaptureStartingEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2ScreenCaptureStartingEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2ScreenCaptureStartingEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "Cancel",         "Handled",         "OriginalSourceFrameInfo", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2WebResourceRequestedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2WebResourceRequestedEventArgs* p = (ICoreWebView2WebResourceRequestedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "ResourceContext")) { COREWEBVIEW2_WEB_RESOURCE_CONTEXT v{}; if (FAILED(p->get_ResourceContext(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2WebResourceRequestedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2WebResourceRequestedEventArgs* p = (ICoreWebView2WebResourceRequestedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "Request")) { ICoreWebView2WebResourceRequest* v = nullptr; if (FAILED(p->get_Request(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2WebResourceRequest"; return 1; }
    if (!_stricmp(name, "Response")) { ICoreWebView2WebResourceResponse* v = nullptr; if (FAILED(p->get_Response(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2WebResourceResponse"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2WebResourceRequestedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2WebResourceRequestedEventArgs* p = (ICoreWebView2WebResourceRequestedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2WebResourceRequestedEventArgs(void* pV, void** ppOut)
{
    ICoreWebView2WebResourceRequestedEventArgs* p = (ICoreWebView2WebResourceRequestedEventArgs*)pV; UNREF(ppOut);
    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;
    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;
}

static int ChildAt_ICoreWebView2WebResourceRequestedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2WebResourceRequestedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2WebResourceRequestedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "Request",         "Response",         "ResourceContext", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2WebResourceResponseReceivedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2WebResourceResponseReceivedEventArgs* p = (ICoreWebView2WebResourceResponseReceivedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    return 0;
}

static int Child_ICoreWebView2WebResourceResponseReceivedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2WebResourceResponseReceivedEventArgs* p = (ICoreWebView2WebResourceResponseReceivedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "Request")) { ICoreWebView2WebResourceRequest* v = nullptr; if (FAILED(p->get_Request(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2WebResourceRequest"; return 1; }
    if (!_stricmp(name, "Response")) { ICoreWebView2WebResourceResponseView* v = nullptr; if (FAILED(p->get_Response(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2WebResourceResponseView"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2WebResourceResponseReceivedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2WebResourceResponseReceivedEventArgs* p = (ICoreWebView2WebResourceResponseReceivedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2WebResourceResponseReceivedEventArgs(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2WebResourceResponseReceivedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2WebResourceResponseReceivedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2WebResourceResponseReceivedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "Request",         "Response", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2ProcessInfoCollection(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2ProcessInfoCollection* p = (ICoreWebView2ProcessInfoCollection*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Count")) { UINT32 v{}; if (FAILED(p->get_Count(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2ProcessInfoCollection(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ProcessInfoCollection* p = (ICoreWebView2ProcessInfoCollection*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2ProcessInfoCollection(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2ProcessInfoCollection* p = (ICoreWebView2ProcessInfoCollection*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2ProcessInfoCollection(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2ProcessInfoCollection(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ProcessInfoCollection* p = (ICoreWebView2ProcessInfoCollection*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "Items") || !_stricmp(name, "ValueAtIndex")) { ICoreWebView2ProcessInfo* v = nullptr; if (FAILED(p->GetValueAtIndex((UINT32)index, &v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2ProcessInfo"; return 1; }
    return 0;
}

static int ListCount_ICoreWebView2ProcessInfoCollection(void* pV, LPCSTR name, INT* pCount)
{
    ICoreWebView2ProcessInfoCollection* p = (ICoreWebView2ProcessInfoCollection*)pV; UNREF(name);
    if (!pCount) return 0; UINT32 n{}; if (FAILED(p->get_Count(&n))) return 0; *pCount = (INT)n; return 1;
}

static void Names_ICoreWebView2ProcessInfoCollection(char* buf, INT cap)
{
    static const char* items[] = {
        "Count",         "Items", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2CookieList(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2CookieList* p = (ICoreWebView2CookieList*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Count")) { UINT32 v{}; if (FAILED(p->get_Count(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2CookieList(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2CookieList* p = (ICoreWebView2CookieList*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2CookieList(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2CookieList* p = (ICoreWebView2CookieList*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2CookieList(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2CookieList(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2CookieList* p = (ICoreWebView2CookieList*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "Items") || !_stricmp(name, "ValueAtIndex")) { ICoreWebView2Cookie* v = nullptr; if (FAILED(p->GetValueAtIndex((UINT32)index, &v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2Cookie"; return 1; }
    return 0;
}

static int ListCount_ICoreWebView2CookieList(void* pV, LPCSTR name, INT* pCount)
{
    ICoreWebView2CookieList* p = (ICoreWebView2CookieList*)pV; UNREF(name);
    if (!pCount) return 0; UINT32 n{}; if (FAILED(p->get_Count(&n))) return 0; *pCount = (INT)n; return 1;
}

static void Names_ICoreWebView2CookieList(char* buf, INT cap)
{
    static const char* items[] = {
        "Count",         "Items", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2BrowserExtensionList(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2BrowserExtensionList* p = (ICoreWebView2BrowserExtensionList*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Count")) { UINT32 v{}; if (FAILED(p->get_Count(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2BrowserExtensionList(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2BrowserExtensionList* p = (ICoreWebView2BrowserExtensionList*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2BrowserExtensionList(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2BrowserExtensionList* p = (ICoreWebView2BrowserExtensionList*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2BrowserExtensionList(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2BrowserExtensionList(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2BrowserExtensionList* p = (ICoreWebView2BrowserExtensionList*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "Items") || !_stricmp(name, "ValueAtIndex")) { ICoreWebView2BrowserExtension* v = nullptr; if (FAILED(p->GetValueAtIndex((UINT32)index, &v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2BrowserExtension"; return 1; }
    return 0;
}

static int ListCount_ICoreWebView2BrowserExtensionList(void* pV, LPCSTR name, INT* pCount)
{
    ICoreWebView2BrowserExtensionList* p = (ICoreWebView2BrowserExtensionList*)pV; UNREF(name);
    if (!pCount) return 0; UINT32 n{}; if (FAILED(p->get_Count(&n))) return 0; *pCount = (INT)n; return 1;
}

static void Names_ICoreWebView2BrowserExtensionList(char* buf, INT cap)
{
    static const char* items[] = {
        "Count",         "Items", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2ClientCertificate(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2ClientCertificate* p = (ICoreWebView2ClientCertificate*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Subject")) { LPWSTR v = nullptr; if (FAILED(p->get_Subject(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Issuer")) { LPWSTR v = nullptr; if (FAILED(p->get_Issuer(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "ValidFrom")) { double v{}; if (FAILED(p->get_ValidFrom(&v))) return 0; if (pDbl) *pDbl = (double)v; if (outInt) *outInt = (INT)v; return 1; }
    if (!_stricmp(name, "ValidTo")) { double v{}; if (FAILED(p->get_ValidTo(&v))) return 0; if (pDbl) *pDbl = (double)v; if (outInt) *outInt = (INT)v; return 1; }
    if (!_stricmp(name, "DerEncodedSerialNumber")) { LPWSTR v = nullptr; if (FAILED(p->get_DerEncodedSerialNumber(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "DisplayName")) { LPWSTR v = nullptr; if (FAILED(p->get_DisplayName(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Kind")) { COREWEBVIEW2_CLIENT_CERTIFICATE_KIND v{}; if (FAILED(p->get_Kind(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2ClientCertificate(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ClientCertificate* p = (ICoreWebView2ClientCertificate*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "PemEncodedIssuerCertificateChain")) { ICoreWebView2StringCollection* v = nullptr; if (FAILED(p->get_PemEncodedIssuerCertificateChain(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2StringCollection"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2ClientCertificate(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2ClientCertificate* p = (ICoreWebView2ClientCertificate*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2ClientCertificate(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2ClientCertificate(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2ClientCertificate(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2ClientCertificate(char* buf, INT cap)
{
    static const char* items[] = {
        "Subject",         "Issuer",         "ValidFrom",         "ValidTo",         "DerEncodedSerialNumber",         "DisplayName",
        "PemEncodedIssuerCertificateChain",         "Kind", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2* p = (ICoreWebView2*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Source")) { LPWSTR v = nullptr; if (FAILED(p->get_Source(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "BrowserProcessId")) { UINT32 v{}; if (FAILED(p->get_BrowserProcessId(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "CanGoBack")) { BOOL v{}; if (FAILED(p->get_CanGoBack(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "CanGoForward")) { BOOL v{}; if (FAILED(p->get_CanGoForward(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "DocumentTitle")) { LPWSTR v = nullptr; if (FAILED(p->get_DocumentTitle(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "ContainsFullScreenElement")) { BOOL v{}; if (FAILED(p->get_ContainsFullScreenElement(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2* p = (ICoreWebView2*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "Settings")) { ICoreWebView2Settings* v = nullptr; if (FAILED(p->get_Settings(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2Settings"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2* p = (ICoreWebView2*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2(char* buf, INT cap)
{
    static const char* items[] = {
        "Settings",         "Source",         "BrowserProcessId",         "CanGoBack",         "CanGoForward",         "DocumentTitle",
        "ContainsFullScreenElement", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2Certificate(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2Certificate* p = (ICoreWebView2Certificate*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Subject")) { LPWSTR v = nullptr; if (FAILED(p->get_Subject(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Issuer")) { LPWSTR v = nullptr; if (FAILED(p->get_Issuer(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "ValidFrom")) { double v{}; if (FAILED(p->get_ValidFrom(&v))) return 0; if (pDbl) *pDbl = (double)v; if (outInt) *outInt = (INT)v; return 1; }
    if (!_stricmp(name, "ValidTo")) { double v{}; if (FAILED(p->get_ValidTo(&v))) return 0; if (pDbl) *pDbl = (double)v; if (outInt) *outInt = (INT)v; return 1; }
    if (!_stricmp(name, "DerEncodedSerialNumber")) { LPWSTR v = nullptr; if (FAILED(p->get_DerEncodedSerialNumber(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "DisplayName")) { LPWSTR v = nullptr; if (FAILED(p->get_DisplayName(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    return 0;
}

static int Child_ICoreWebView2Certificate(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2Certificate* p = (ICoreWebView2Certificate*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "PemEncodedIssuerCertificateChain")) { ICoreWebView2StringCollection* v = nullptr; if (FAILED(p->get_PemEncodedIssuerCertificateChain(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2StringCollection"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2Certificate(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2Certificate* p = (ICoreWebView2Certificate*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2Certificate(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2Certificate(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2Certificate(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2Certificate(char* buf, INT cap)
{
    static const char* items[] = {
        "Subject",         "Issuer",         "ValidFrom",         "ValidTo",         "DerEncodedSerialNumber",         "DisplayName",
        "PemEncodedIssuerCertificateChain", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2NewWindowRequestedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2NewWindowRequestedEventArgs* p = (ICoreWebView2NewWindowRequestedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Uri")) { LPWSTR v = nullptr; if (FAILED(p->get_Uri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Handled")) { BOOL v{}; if (FAILED(p->get_Handled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "IsUserInitiated")) { BOOL v{}; if (FAILED(p->get_IsUserInitiated(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2NewWindowRequestedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2NewWindowRequestedEventArgs* p = (ICoreWebView2NewWindowRequestedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "NewWindow")) { ICoreWebView2* v = nullptr; if (FAILED(p->get_NewWindow(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2"; return 1; }
    if (!_stricmp(name, "WindowFeatures")) { ICoreWebView2WindowFeatures* v = nullptr; if (FAILED(p->get_WindowFeatures(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2WindowFeatures"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2NewWindowRequestedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2NewWindowRequestedEventArgs* p = (ICoreWebView2NewWindowRequestedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Handled")) { return SUCCEEDED(p->put_Handled((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2NewWindowRequestedEventArgs(void* pV, void** ppOut)
{
    ICoreWebView2NewWindowRequestedEventArgs* p = (ICoreWebView2NewWindowRequestedEventArgs*)pV; UNREF(ppOut);
    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;
    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;
}

static int ChildAt_ICoreWebView2NewWindowRequestedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2NewWindowRequestedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2NewWindowRequestedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "Uri",         "NewWindow",         "Handled",         "IsUserInitiated",         "WindowFeatures", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2ServerCertificateErrorDetectedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2ServerCertificateErrorDetectedEventArgs* p = (ICoreWebView2ServerCertificateErrorDetectedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "ErrorStatus")) { COREWEBVIEW2_WEB_ERROR_STATUS v{}; if (FAILED(p->get_ErrorStatus(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "RequestUri")) { LPWSTR v = nullptr; if (FAILED(p->get_RequestUri(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Action")) { COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION v{}; if (FAILED(p->get_Action(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2ServerCertificateErrorDetectedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ServerCertificateErrorDetectedEventArgs* p = (ICoreWebView2ServerCertificateErrorDetectedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "ServerCertificate")) { ICoreWebView2Certificate* v = nullptr; if (FAILED(p->get_ServerCertificate(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2Certificate"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2ServerCertificateErrorDetectedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2ServerCertificateErrorDetectedEventArgs* p = (ICoreWebView2ServerCertificateErrorDetectedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Action")) { return SUCCEEDED(p->put_Action((COREWEBVIEW2_SERVER_CERTIFICATE_ERROR_ACTION)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2ServerCertificateErrorDetectedEventArgs(void* pV, void** ppOut)
{
    ICoreWebView2ServerCertificateErrorDetectedEventArgs* p = (ICoreWebView2ServerCertificateErrorDetectedEventArgs*)pV; UNREF(ppOut);
    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;
    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;
}

static int ChildAt_ICoreWebView2ServerCertificateErrorDetectedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2ServerCertificateErrorDetectedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2ServerCertificateErrorDetectedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "ErrorStatus",         "RequestUri",         "ServerCertificate",         "Action", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2ClientCertificateCollection(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2ClientCertificateCollection* p = (ICoreWebView2ClientCertificateCollection*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Count")) { UINT32 v{}; if (FAILED(p->get_Count(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2ClientCertificateCollection(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ClientCertificateCollection* p = (ICoreWebView2ClientCertificateCollection*)pV; UNREF(ppChild); UNREF(ppIface);
    return 0;
}

static int SetP_ICoreWebView2ClientCertificateCollection(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2ClientCertificateCollection* p = (ICoreWebView2ClientCertificateCollection*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    return 0;
}

static int Deferral_ICoreWebView2ClientCertificateCollection(void* pV, void** ppOut)
{
    UNREF(pV); UNREF(ppOut); return 0;
}

static int ChildAt_ICoreWebView2ClientCertificateCollection(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ClientCertificateCollection* p = (ICoreWebView2ClientCertificateCollection*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "Items") || !_stricmp(name, "ValueAtIndex")) { ICoreWebView2ClientCertificate* v = nullptr; if (FAILED(p->GetValueAtIndex((UINT32)index, &v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2ClientCertificate"; return 1; }
    return 0;
}

static int ListCount_ICoreWebView2ClientCertificateCollection(void* pV, LPCSTR name, INT* pCount)
{
    ICoreWebView2ClientCertificateCollection* p = (ICoreWebView2ClientCertificateCollection*)pV; UNREF(name);
    if (!pCount) return 0; UINT32 n{}; if (FAILED(p->get_Count(&n))) return 0; *pCount = (INT)n; return 1;
}

static void Names_ICoreWebView2ClientCertificateCollection(char* buf, INT cap)
{
    static const char* items[] = {
        "Count",         "Items", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

static int GetP_ICoreWebView2ClientCertificateRequestedEventArgs(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{
    ICoreWebView2ClientCertificateRequestedEventArgs* p = (ICoreWebView2ClientCertificateRequestedEventArgs*)pV; UNREF(outInt); UNREF(pDbl);
    if (!_stricmp(name, "Host")) { LPWSTR v = nullptr; if (FAILED(p->get_Host(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }
    if (!_stricmp(name, "Port")) { int v{}; if (FAILED(p->get_Port(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "IsProxy")) { BOOL v{}; if (FAILED(p->get_IsProxy(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Cancel")) { BOOL v{}; if (FAILED(p->get_Cancel(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    if (!_stricmp(name, "Handled")) { BOOL v{}; if (FAILED(p->get_Handled(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }
    return 0;
}

static int Child_ICoreWebView2ClientCertificateRequestedEventArgs(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{
    ICoreWebView2ClientCertificateRequestedEventArgs* p = (ICoreWebView2ClientCertificateRequestedEventArgs*)pV; UNREF(ppChild); UNREF(ppIface);
    if (!_stricmp(name, "AllowedCertificateAuthorities")) { ICoreWebView2StringCollection* v = nullptr; if (FAILED(p->get_AllowedCertificateAuthorities(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2StringCollection"; return 1; }
    if (!_stricmp(name, "MutuallyTrustedCertificates")) { ICoreWebView2ClientCertificateCollection* v = nullptr; if (FAILED(p->get_MutuallyTrustedCertificates(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2ClientCertificateCollection"; return 1; }
    if (!_stricmp(name, "SelectedCertificate")) { ICoreWebView2ClientCertificate* v = nullptr; if (FAILED(p->get_SelectedCertificate(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "ICoreWebView2ClientCertificate"; return 1; }
    return 0;
}

static int SetP_ICoreWebView2ClientCertificateRequestedEventArgs(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{
    ICoreWebView2ClientCertificateRequestedEventArgs* p = (ICoreWebView2ClientCertificateRequestedEventArgs*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);
    if (!_stricmp(name, "Cancel")) { return SUCCEEDED(p->put_Cancel((BOOL)inInt)) ? 1 : 0; }
    if (!_stricmp(name, "Handled")) { return SUCCEEDED(p->put_Handled((BOOL)inInt)) ? 1 : 0; }
    return 0;
}

static int Deferral_ICoreWebView2ClientCertificateRequestedEventArgs(void* pV, void** ppOut)
{
    ICoreWebView2ClientCertificateRequestedEventArgs* p = (ICoreWebView2ClientCertificateRequestedEventArgs*)pV; UNREF(ppOut);
    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;
    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;
}

static int ChildAt_ICoreWebView2ClientCertificateRequestedEventArgs(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }

static int ListCount_ICoreWebView2ClientCertificateRequestedEventArgs(void* pV, LPCSTR name, INT* pCount)
{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }

static void Names_ICoreWebView2ClientCertificateRequestedEventArgs(char* buf, INT cap)
{
    static const char* items[] = {
        "Host",         "Port",         "IsProxy",         "AllowedCertificateAuthorities",         "MutuallyTrustedCertificates",         "SelectedCertificate",
        "Cancel",         "Handled", 
    };
    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));
}

struct WvPropItem { LPCSTR m_szIface;
    int (*m_pfnGet)(void*, LPCSTR, INT*, DOUBLE*, LPSTR, INT);
    int (*m_pfnChild)(void*, LPCSTR, void**, LPCSTR*);
    int (*m_pfnChildAt)(void*, LPCSTR, INT, void**, LPCSTR*);
    int (*m_pfnListCount)(void*, LPCSTR, INT*);
    int (*m_pfnSet)(void*, LPCSTR, INT, DOUBLE, LPCSTR);
    int (*m_pfnDeferral)(void*, void**);
    void (*m_pfnNames)(char*, INT); };
static const WvPropItem g_propItems[] = {
    { "ICoreWebView2AcceleratorKeyPressedEventArgs", GetP_ICoreWebView2AcceleratorKeyPressedEventArgs, Child_ICoreWebView2AcceleratorKeyPressedEventArgs, ChildAt_ICoreWebView2AcceleratorKeyPressedEventArgs, ListCount_ICoreWebView2AcceleratorKeyPressedEventArgs, SetP_ICoreWebView2AcceleratorKeyPressedEventArgs, Deferral_ICoreWebView2AcceleratorKeyPressedEventArgs, Names_ICoreWebView2AcceleratorKeyPressedEventArgs },
    { "ICoreWebView2MoveFocusRequestedEventArgs", GetP_ICoreWebView2MoveFocusRequestedEventArgs, Child_ICoreWebView2MoveFocusRequestedEventArgs, ChildAt_ICoreWebView2MoveFocusRequestedEventArgs, ListCount_ICoreWebView2MoveFocusRequestedEventArgs, SetP_ICoreWebView2MoveFocusRequestedEventArgs, Deferral_ICoreWebView2MoveFocusRequestedEventArgs, Names_ICoreWebView2MoveFocusRequestedEventArgs },
    { "ICoreWebView2ContentLoadingEventArgs", GetP_ICoreWebView2ContentLoadingEventArgs, Child_ICoreWebView2ContentLoadingEventArgs, ChildAt_ICoreWebView2ContentLoadingEventArgs, ListCount_ICoreWebView2ContentLoadingEventArgs, SetP_ICoreWebView2ContentLoadingEventArgs, Deferral_ICoreWebView2ContentLoadingEventArgs, Names_ICoreWebView2ContentLoadingEventArgs },
    { "ICoreWebView2DOMContentLoadedEventArgs", GetP_ICoreWebView2DOMContentLoadedEventArgs, Child_ICoreWebView2DOMContentLoadedEventArgs, ChildAt_ICoreWebView2DOMContentLoadedEventArgs, ListCount_ICoreWebView2DOMContentLoadedEventArgs, SetP_ICoreWebView2DOMContentLoadedEventArgs, Deferral_ICoreWebView2DOMContentLoadedEventArgs, Names_ICoreWebView2DOMContentLoadedEventArgs },
    { "ICoreWebView2NavigationCompletedEventArgs", GetP_ICoreWebView2NavigationCompletedEventArgs, Child_ICoreWebView2NavigationCompletedEventArgs, ChildAt_ICoreWebView2NavigationCompletedEventArgs, ListCount_ICoreWebView2NavigationCompletedEventArgs, SetP_ICoreWebView2NavigationCompletedEventArgs, Deferral_ICoreWebView2NavigationCompletedEventArgs, Names_ICoreWebView2NavigationCompletedEventArgs },
    { "ICoreWebView2NavigationStartingEventArgs", GetP_ICoreWebView2NavigationStartingEventArgs, Child_ICoreWebView2NavigationStartingEventArgs, ChildAt_ICoreWebView2NavigationStartingEventArgs, ListCount_ICoreWebView2NavigationStartingEventArgs, SetP_ICoreWebView2NavigationStartingEventArgs, Deferral_ICoreWebView2NavigationStartingEventArgs, Names_ICoreWebView2NavigationStartingEventArgs },
    { "ICoreWebView2LaunchingExternalUriSchemeEventArgs", GetP_ICoreWebView2LaunchingExternalUriSchemeEventArgs, Child_ICoreWebView2LaunchingExternalUriSchemeEventArgs, ChildAt_ICoreWebView2LaunchingExternalUriSchemeEventArgs, ListCount_ICoreWebView2LaunchingExternalUriSchemeEventArgs, SetP_ICoreWebView2LaunchingExternalUriSchemeEventArgs, Deferral_ICoreWebView2LaunchingExternalUriSchemeEventArgs, Names_ICoreWebView2LaunchingExternalUriSchemeEventArgs },
    { "ICoreWebView2PermissionRequestedEventArgs", GetP_ICoreWebView2PermissionRequestedEventArgs, Child_ICoreWebView2PermissionRequestedEventArgs, ChildAt_ICoreWebView2PermissionRequestedEventArgs, ListCount_ICoreWebView2PermissionRequestedEventArgs, SetP_ICoreWebView2PermissionRequestedEventArgs, Deferral_ICoreWebView2PermissionRequestedEventArgs, Names_ICoreWebView2PermissionRequestedEventArgs },
    { "ICoreWebView2ProcessFailedEventArgs", GetP_ICoreWebView2ProcessFailedEventArgs, Child_ICoreWebView2ProcessFailedEventArgs, ChildAt_ICoreWebView2ProcessFailedEventArgs, ListCount_ICoreWebView2ProcessFailedEventArgs, SetP_ICoreWebView2ProcessFailedEventArgs, Deferral_ICoreWebView2ProcessFailedEventArgs, Names_ICoreWebView2ProcessFailedEventArgs },
    { "ICoreWebView2SaveAsUIShowingEventArgs", GetP_ICoreWebView2SaveAsUIShowingEventArgs, Child_ICoreWebView2SaveAsUIShowingEventArgs, ChildAt_ICoreWebView2SaveAsUIShowingEventArgs, ListCount_ICoreWebView2SaveAsUIShowingEventArgs, SetP_ICoreWebView2SaveAsUIShowingEventArgs, Deferral_ICoreWebView2SaveAsUIShowingEventArgs, Names_ICoreWebView2SaveAsUIShowingEventArgs },
    { "ICoreWebView2SaveFileSecurityCheckStartingEventArgs", GetP_ICoreWebView2SaveFileSecurityCheckStartingEventArgs, Child_ICoreWebView2SaveFileSecurityCheckStartingEventArgs, ChildAt_ICoreWebView2SaveFileSecurityCheckStartingEventArgs, ListCount_ICoreWebView2SaveFileSecurityCheckStartingEventArgs, SetP_ICoreWebView2SaveFileSecurityCheckStartingEventArgs, Deferral_ICoreWebView2SaveFileSecurityCheckStartingEventArgs, Names_ICoreWebView2SaveFileSecurityCheckStartingEventArgs },
    { "ICoreWebView2ScriptDialogOpeningEventArgs", GetP_ICoreWebView2ScriptDialogOpeningEventArgs, Child_ICoreWebView2ScriptDialogOpeningEventArgs, ChildAt_ICoreWebView2ScriptDialogOpeningEventArgs, ListCount_ICoreWebView2ScriptDialogOpeningEventArgs, SetP_ICoreWebView2ScriptDialogOpeningEventArgs, Deferral_ICoreWebView2ScriptDialogOpeningEventArgs, Names_ICoreWebView2ScriptDialogOpeningEventArgs },
    { "ICoreWebView2SourceChangedEventArgs", GetP_ICoreWebView2SourceChangedEventArgs, Child_ICoreWebView2SourceChangedEventArgs, ChildAt_ICoreWebView2SourceChangedEventArgs, ListCount_ICoreWebView2SourceChangedEventArgs, SetP_ICoreWebView2SourceChangedEventArgs, Deferral_ICoreWebView2SourceChangedEventArgs, Names_ICoreWebView2SourceChangedEventArgs },
    { "ICoreWebView2WebMessageReceivedEventArgs", GetP_ICoreWebView2WebMessageReceivedEventArgs, Child_ICoreWebView2WebMessageReceivedEventArgs, ChildAt_ICoreWebView2WebMessageReceivedEventArgs, ListCount_ICoreWebView2WebMessageReceivedEventArgs, SetP_ICoreWebView2WebMessageReceivedEventArgs, Deferral_ICoreWebView2WebMessageReceivedEventArgs, Names_ICoreWebView2WebMessageReceivedEventArgs },
    { "ICoreWebView2BrowserProcessExitedEventArgs", GetP_ICoreWebView2BrowserProcessExitedEventArgs, Child_ICoreWebView2BrowserProcessExitedEventArgs, ChildAt_ICoreWebView2BrowserProcessExitedEventArgs, ListCount_ICoreWebView2BrowserProcessExitedEventArgs, SetP_ICoreWebView2BrowserProcessExitedEventArgs, Deferral_ICoreWebView2BrowserProcessExitedEventArgs, Names_ICoreWebView2BrowserProcessExitedEventArgs },
    { "ICoreWebView2PermissionRequestedEventArgs2", GetP_ICoreWebView2PermissionRequestedEventArgs2, Child_ICoreWebView2PermissionRequestedEventArgs2, ChildAt_ICoreWebView2PermissionRequestedEventArgs2, ListCount_ICoreWebView2PermissionRequestedEventArgs2, SetP_ICoreWebView2PermissionRequestedEventArgs2, Deferral_ICoreWebView2PermissionRequestedEventArgs2, Names_ICoreWebView2PermissionRequestedEventArgs2 },
    { "ICoreWebView2DevToolsProtocolEventReceivedEventArgs", GetP_ICoreWebView2DevToolsProtocolEventReceivedEventArgs, Child_ICoreWebView2DevToolsProtocolEventReceivedEventArgs, ChildAt_ICoreWebView2DevToolsProtocolEventReceivedEventArgs, ListCount_ICoreWebView2DevToolsProtocolEventReceivedEventArgs, SetP_ICoreWebView2DevToolsProtocolEventReceivedEventArgs, Deferral_ICoreWebView2DevToolsProtocolEventReceivedEventArgs, Names_ICoreWebView2DevToolsProtocolEventReceivedEventArgs },
    { "ICoreWebView2ProcessExtendedInfoCollection", GetP_ICoreWebView2ProcessExtendedInfoCollection, Child_ICoreWebView2ProcessExtendedInfoCollection, ChildAt_ICoreWebView2ProcessExtendedInfoCollection, ListCount_ICoreWebView2ProcessExtendedInfoCollection, SetP_ICoreWebView2ProcessExtendedInfoCollection, Deferral_ICoreWebView2ProcessExtendedInfoCollection, Names_ICoreWebView2ProcessExtendedInfoCollection },
    { "ICoreWebView2FrameInfoCollection", GetP_ICoreWebView2FrameInfoCollection, Child_ICoreWebView2FrameInfoCollection, ChildAt_ICoreWebView2FrameInfoCollection, ListCount_ICoreWebView2FrameInfoCollection, SetP_ICoreWebView2FrameInfoCollection, Deferral_ICoreWebView2FrameInfoCollection, Names_ICoreWebView2FrameInfoCollection },
    { "ICoreWebView2Cookie", GetP_ICoreWebView2Cookie, Child_ICoreWebView2Cookie, ChildAt_ICoreWebView2Cookie, ListCount_ICoreWebView2Cookie, SetP_ICoreWebView2Cookie, Deferral_ICoreWebView2Cookie, Names_ICoreWebView2Cookie },
    { "ICoreWebView2Profile", GetP_ICoreWebView2Profile, Child_ICoreWebView2Profile, ChildAt_ICoreWebView2Profile, ListCount_ICoreWebView2Profile, SetP_ICoreWebView2Profile, Deferral_ICoreWebView2Profile, Names_ICoreWebView2Profile },
    { "ICoreWebView2DownloadOperation", GetP_ICoreWebView2DownloadOperation, Child_ICoreWebView2DownloadOperation, ChildAt_ICoreWebView2DownloadOperation, ListCount_ICoreWebView2DownloadOperation, SetP_ICoreWebView2DownloadOperation, Deferral_ICoreWebView2DownloadOperation, Names_ICoreWebView2DownloadOperation },
    { "ICoreWebView2Frame", GetP_ICoreWebView2Frame, Child_ICoreWebView2Frame, ChildAt_ICoreWebView2Frame, ListCount_ICoreWebView2Frame, SetP_ICoreWebView2Frame, Deferral_ICoreWebView2Frame, Names_ICoreWebView2Frame },
    { "ICoreWebView2WebResourceResponse", GetP_ICoreWebView2WebResourceResponse, Child_ICoreWebView2WebResourceResponse, ChildAt_ICoreWebView2WebResourceResponse, ListCount_ICoreWebView2WebResourceResponse, SetP_ICoreWebView2WebResourceResponse, Deferral_ICoreWebView2WebResourceResponse, Names_ICoreWebView2WebResourceResponse },
    { "ICoreWebView2WebResourceRequest", GetP_ICoreWebView2WebResourceRequest, Child_ICoreWebView2WebResourceRequest, ChildAt_ICoreWebView2WebResourceRequest, ListCount_ICoreWebView2WebResourceRequest, SetP_ICoreWebView2WebResourceRequest, Deferral_ICoreWebView2WebResourceRequest, Names_ICoreWebView2WebResourceRequest },
    { "ICoreWebView2BrowserExtension", GetP_ICoreWebView2BrowserExtension, Child_ICoreWebView2BrowserExtension, ChildAt_ICoreWebView2BrowserExtension, ListCount_ICoreWebView2BrowserExtension, SetP_ICoreWebView2BrowserExtension, Deferral_ICoreWebView2BrowserExtension, Names_ICoreWebView2BrowserExtension },
    { "ICoreWebView2ProcessInfo", GetP_ICoreWebView2ProcessInfo, Child_ICoreWebView2ProcessInfo, ChildAt_ICoreWebView2ProcessInfo, ListCount_ICoreWebView2ProcessInfo, SetP_ICoreWebView2ProcessInfo, Deferral_ICoreWebView2ProcessInfo, Names_ICoreWebView2ProcessInfo },
    { "ICoreWebView2BasicAuthenticationResponse", GetP_ICoreWebView2BasicAuthenticationResponse, Child_ICoreWebView2BasicAuthenticationResponse, ChildAt_ICoreWebView2BasicAuthenticationResponse, ListCount_ICoreWebView2BasicAuthenticationResponse, SetP_ICoreWebView2BasicAuthenticationResponse, Deferral_ICoreWebView2BasicAuthenticationResponse, Names_ICoreWebView2BasicAuthenticationResponse },
    { "ICoreWebView2StringCollection", GetP_ICoreWebView2StringCollection, Child_ICoreWebView2StringCollection, ChildAt_ICoreWebView2StringCollection, ListCount_ICoreWebView2StringCollection, SetP_ICoreWebView2StringCollection, Deferral_ICoreWebView2StringCollection, Names_ICoreWebView2StringCollection },
    { "ICoreWebView2ContextMenuItemCollection", GetP_ICoreWebView2ContextMenuItemCollection, Child_ICoreWebView2ContextMenuItemCollection, ChildAt_ICoreWebView2ContextMenuItemCollection, ListCount_ICoreWebView2ContextMenuItemCollection, SetP_ICoreWebView2ContextMenuItemCollection, Deferral_ICoreWebView2ContextMenuItemCollection, Names_ICoreWebView2ContextMenuItemCollection },
    { "ICoreWebView2ContextMenuTarget", GetP_ICoreWebView2ContextMenuTarget, Child_ICoreWebView2ContextMenuTarget, ChildAt_ICoreWebView2ContextMenuTarget, ListCount_ICoreWebView2ContextMenuTarget, SetP_ICoreWebView2ContextMenuTarget, Deferral_ICoreWebView2ContextMenuTarget, Names_ICoreWebView2ContextMenuTarget },
    { "ICoreWebView2FrameInfo", GetP_ICoreWebView2FrameInfo, Child_ICoreWebView2FrameInfo, ChildAt_ICoreWebView2FrameInfo, ListCount_ICoreWebView2FrameInfo, SetP_ICoreWebView2FrameInfo, Deferral_ICoreWebView2FrameInfo, Names_ICoreWebView2FrameInfo },
    { "ICoreWebView2DedicatedWorker", GetP_ICoreWebView2DedicatedWorker, Child_ICoreWebView2DedicatedWorker, ChildAt_ICoreWebView2DedicatedWorker, ListCount_ICoreWebView2DedicatedWorker, SetP_ICoreWebView2DedicatedWorker, Deferral_ICoreWebView2DedicatedWorker, Names_ICoreWebView2DedicatedWorker },
    { "ICoreWebView2WindowFeatures", GetP_ICoreWebView2WindowFeatures, Child_ICoreWebView2WindowFeatures, ChildAt_ICoreWebView2WindowFeatures, ListCount_ICoreWebView2WindowFeatures, SetP_ICoreWebView2WindowFeatures, Deferral_ICoreWebView2WindowFeatures, Names_ICoreWebView2WindowFeatures },
    { "ICoreWebView2Notification", GetP_ICoreWebView2Notification, Child_ICoreWebView2Notification, ChildAt_ICoreWebView2Notification, ListCount_ICoreWebView2Notification, SetP_ICoreWebView2Notification, Deferral_ICoreWebView2Notification, Names_ICoreWebView2Notification },
    { "ICoreWebView2WebResourceResponseView", GetP_ICoreWebView2WebResourceResponseView, Child_ICoreWebView2WebResourceResponseView, ChildAt_ICoreWebView2WebResourceResponseView, ListCount_ICoreWebView2WebResourceResponseView, SetP_ICoreWebView2WebResourceResponseView, Deferral_ICoreWebView2WebResourceResponseView, Names_ICoreWebView2WebResourceResponseView },
    { "ICoreWebView2Settings", GetP_ICoreWebView2Settings, Child_ICoreWebView2Settings, ChildAt_ICoreWebView2Settings, ListCount_ICoreWebView2Settings, SetP_ICoreWebView2Settings, Deferral_ICoreWebView2Settings, Names_ICoreWebView2Settings },
    { "ICoreWebView2BasicAuthenticationRequestedEventArgs", GetP_ICoreWebView2BasicAuthenticationRequestedEventArgs, Child_ICoreWebView2BasicAuthenticationRequestedEventArgs, ChildAt_ICoreWebView2BasicAuthenticationRequestedEventArgs, ListCount_ICoreWebView2BasicAuthenticationRequestedEventArgs, SetP_ICoreWebView2BasicAuthenticationRequestedEventArgs, Deferral_ICoreWebView2BasicAuthenticationRequestedEventArgs, Names_ICoreWebView2BasicAuthenticationRequestedEventArgs },
    { "ICoreWebView2ContextMenuRequestedEventArgs", GetP_ICoreWebView2ContextMenuRequestedEventArgs, Child_ICoreWebView2ContextMenuRequestedEventArgs, ChildAt_ICoreWebView2ContextMenuRequestedEventArgs, ListCount_ICoreWebView2ContextMenuRequestedEventArgs, SetP_ICoreWebView2ContextMenuRequestedEventArgs, Deferral_ICoreWebView2ContextMenuRequestedEventArgs, Names_ICoreWebView2ContextMenuRequestedEventArgs },
    { "ICoreWebView2DedicatedWorkerCreatedEventArgs", GetP_ICoreWebView2DedicatedWorkerCreatedEventArgs, Child_ICoreWebView2DedicatedWorkerCreatedEventArgs, ChildAt_ICoreWebView2DedicatedWorkerCreatedEventArgs, ListCount_ICoreWebView2DedicatedWorkerCreatedEventArgs, SetP_ICoreWebView2DedicatedWorkerCreatedEventArgs, Deferral_ICoreWebView2DedicatedWorkerCreatedEventArgs, Names_ICoreWebView2DedicatedWorkerCreatedEventArgs },
    { "ICoreWebView2DownloadStartingEventArgs", GetP_ICoreWebView2DownloadStartingEventArgs, Child_ICoreWebView2DownloadStartingEventArgs, ChildAt_ICoreWebView2DownloadStartingEventArgs, ListCount_ICoreWebView2DownloadStartingEventArgs, SetP_ICoreWebView2DownloadStartingEventArgs, Deferral_ICoreWebView2DownloadStartingEventArgs, Names_ICoreWebView2DownloadStartingEventArgs },
    { "ICoreWebView2FrameCreatedEventArgs", GetP_ICoreWebView2FrameCreatedEventArgs, Child_ICoreWebView2FrameCreatedEventArgs, ChildAt_ICoreWebView2FrameCreatedEventArgs, ListCount_ICoreWebView2FrameCreatedEventArgs, SetP_ICoreWebView2FrameCreatedEventArgs, Deferral_ICoreWebView2FrameCreatedEventArgs, Names_ICoreWebView2FrameCreatedEventArgs },
    { "ICoreWebView2NotificationReceivedEventArgs", GetP_ICoreWebView2NotificationReceivedEventArgs, Child_ICoreWebView2NotificationReceivedEventArgs, ChildAt_ICoreWebView2NotificationReceivedEventArgs, ListCount_ICoreWebView2NotificationReceivedEventArgs, SetP_ICoreWebView2NotificationReceivedEventArgs, Deferral_ICoreWebView2NotificationReceivedEventArgs, Names_ICoreWebView2NotificationReceivedEventArgs },
    { "ICoreWebView2ScreenCaptureStartingEventArgs", GetP_ICoreWebView2ScreenCaptureStartingEventArgs, Child_ICoreWebView2ScreenCaptureStartingEventArgs, ChildAt_ICoreWebView2ScreenCaptureStartingEventArgs, ListCount_ICoreWebView2ScreenCaptureStartingEventArgs, SetP_ICoreWebView2ScreenCaptureStartingEventArgs, Deferral_ICoreWebView2ScreenCaptureStartingEventArgs, Names_ICoreWebView2ScreenCaptureStartingEventArgs },
    { "ICoreWebView2WebResourceRequestedEventArgs", GetP_ICoreWebView2WebResourceRequestedEventArgs, Child_ICoreWebView2WebResourceRequestedEventArgs, ChildAt_ICoreWebView2WebResourceRequestedEventArgs, ListCount_ICoreWebView2WebResourceRequestedEventArgs, SetP_ICoreWebView2WebResourceRequestedEventArgs, Deferral_ICoreWebView2WebResourceRequestedEventArgs, Names_ICoreWebView2WebResourceRequestedEventArgs },
    { "ICoreWebView2WebResourceResponseReceivedEventArgs", GetP_ICoreWebView2WebResourceResponseReceivedEventArgs, Child_ICoreWebView2WebResourceResponseReceivedEventArgs, ChildAt_ICoreWebView2WebResourceResponseReceivedEventArgs, ListCount_ICoreWebView2WebResourceResponseReceivedEventArgs, SetP_ICoreWebView2WebResourceResponseReceivedEventArgs, Deferral_ICoreWebView2WebResourceResponseReceivedEventArgs, Names_ICoreWebView2WebResourceResponseReceivedEventArgs },
    { "ICoreWebView2ProcessInfoCollection", GetP_ICoreWebView2ProcessInfoCollection, Child_ICoreWebView2ProcessInfoCollection, ChildAt_ICoreWebView2ProcessInfoCollection, ListCount_ICoreWebView2ProcessInfoCollection, SetP_ICoreWebView2ProcessInfoCollection, Deferral_ICoreWebView2ProcessInfoCollection, Names_ICoreWebView2ProcessInfoCollection },
    { "ICoreWebView2CookieList", GetP_ICoreWebView2CookieList, Child_ICoreWebView2CookieList, ChildAt_ICoreWebView2CookieList, ListCount_ICoreWebView2CookieList, SetP_ICoreWebView2CookieList, Deferral_ICoreWebView2CookieList, Names_ICoreWebView2CookieList },
    { "ICoreWebView2BrowserExtensionList", GetP_ICoreWebView2BrowserExtensionList, Child_ICoreWebView2BrowserExtensionList, ChildAt_ICoreWebView2BrowserExtensionList, ListCount_ICoreWebView2BrowserExtensionList, SetP_ICoreWebView2BrowserExtensionList, Deferral_ICoreWebView2BrowserExtensionList, Names_ICoreWebView2BrowserExtensionList },
    { "ICoreWebView2ClientCertificate", GetP_ICoreWebView2ClientCertificate, Child_ICoreWebView2ClientCertificate, ChildAt_ICoreWebView2ClientCertificate, ListCount_ICoreWebView2ClientCertificate, SetP_ICoreWebView2ClientCertificate, Deferral_ICoreWebView2ClientCertificate, Names_ICoreWebView2ClientCertificate },
    { "ICoreWebView2", GetP_ICoreWebView2, Child_ICoreWebView2, ChildAt_ICoreWebView2, ListCount_ICoreWebView2, SetP_ICoreWebView2, Deferral_ICoreWebView2, Names_ICoreWebView2 },
    { "ICoreWebView2Certificate", GetP_ICoreWebView2Certificate, Child_ICoreWebView2Certificate, ChildAt_ICoreWebView2Certificate, ListCount_ICoreWebView2Certificate, SetP_ICoreWebView2Certificate, Deferral_ICoreWebView2Certificate, Names_ICoreWebView2Certificate },
    { "ICoreWebView2NewWindowRequestedEventArgs", GetP_ICoreWebView2NewWindowRequestedEventArgs, Child_ICoreWebView2NewWindowRequestedEventArgs, ChildAt_ICoreWebView2NewWindowRequestedEventArgs, ListCount_ICoreWebView2NewWindowRequestedEventArgs, SetP_ICoreWebView2NewWindowRequestedEventArgs, Deferral_ICoreWebView2NewWindowRequestedEventArgs, Names_ICoreWebView2NewWindowRequestedEventArgs },
    { "ICoreWebView2ServerCertificateErrorDetectedEventArgs", GetP_ICoreWebView2ServerCertificateErrorDetectedEventArgs, Child_ICoreWebView2ServerCertificateErrorDetectedEventArgs, ChildAt_ICoreWebView2ServerCertificateErrorDetectedEventArgs, ListCount_ICoreWebView2ServerCertificateErrorDetectedEventArgs, SetP_ICoreWebView2ServerCertificateErrorDetectedEventArgs, Deferral_ICoreWebView2ServerCertificateErrorDetectedEventArgs, Names_ICoreWebView2ServerCertificateErrorDetectedEventArgs },
    { "ICoreWebView2ClientCertificateCollection", GetP_ICoreWebView2ClientCertificateCollection, Child_ICoreWebView2ClientCertificateCollection, ChildAt_ICoreWebView2ClientCertificateCollection, ListCount_ICoreWebView2ClientCertificateCollection, SetP_ICoreWebView2ClientCertificateCollection, Deferral_ICoreWebView2ClientCertificateCollection, Names_ICoreWebView2ClientCertificateCollection },
    { "ICoreWebView2ClientCertificateRequestedEventArgs", GetP_ICoreWebView2ClientCertificateRequestedEventArgs, Child_ICoreWebView2ClientCertificateRequestedEventArgs, ChildAt_ICoreWebView2ClientCertificateRequestedEventArgs, ListCount_ICoreWebView2ClientCertificateRequestedEventArgs, SetP_ICoreWebView2ClientCertificateRequestedEventArgs, Deferral_ICoreWebView2ClientCertificateRequestedEventArgs, Names_ICoreWebView2ClientCertificateRequestedEventArgs },
};

static const WvPropItem* WvFindProp(LPCSTR iface)
{
    if (iface == NULL) return NULL;
    for (INT i = 0; i < (INT)(sizeof(g_propItems) / sizeof(g_propItems[0])); i++)
        if (lstrcmpA(g_propItems[i].m_szIface, iface) == 0) return &g_propItems[i];
    return NULL;
}

BOOL WvObjKnown(LPCSTR iface) { return WvFindProp(iface) != NULL ? TRUE : FALSE; }
INT WvObjPropGet(LPCSTR iface, void* p, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)
{ const WvPropItem* it = WvFindProp(iface); if (it == NULL || p == NULL || name == NULL) return 0; return it->m_pfnGet(p, name, outInt, pDbl, outText, cap); }
INT WvObjPropChild(LPCSTR iface, void* p, LPCSTR name, void** ppChild, LPCSTR* ppIface)
{ const WvPropItem* it = WvFindProp(iface); if (it == NULL || p == NULL || name == NULL) return 0; return it->m_pfnChild(p, name, ppChild, ppIface); }
INT WvObjPropChildAt(LPCSTR iface, void* p, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)
{ const WvPropItem* it = WvFindProp(iface); if (it == NULL || p == NULL || name == NULL) return 0; return it->m_pfnChildAt(p, name, index, ppChild, ppIface); }
INT WvObjListCount(LPCSTR iface, void* p, LPCSTR name, INT* pCount)
{ const WvPropItem* it = WvFindProp(iface); if (it == NULL || p == NULL) return 0; return it->m_pfnListCount(p, name, pCount); }
INT WvObjPropSet(LPCSTR iface, void* p, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{ const WvPropItem* it = WvFindProp(iface); if (it == NULL || p == NULL || name == NULL) return 0; return it->m_pfnSet(p, name, inInt, inDouble, inText); }
INT WvObjDeferral(LPCSTR iface, void* p, void** ppOut)
{ const WvPropItem* it = WvFindProp(iface); if (it == NULL || p == NULL) return 0; return it->m_pfnDeferral(p, ppOut); }
void WvObjPropNames(LPCSTR iface, char* buf, INT cap)
{ if (buf != NULL && cap > 0) buf[0] = '\0'; const WvPropItem* it = WvFindProp(iface); if (it == NULL) return; it->m_pfnNames(buf, cap); }

struct WvEvtBind { void* m_pCtx; WvEventFn m_fn; };

class WvEv_CONTROLLER_AcceleratorKeyPressed : public ICoreWebView2AcceleratorKeyPressedEventHandler
{
public:
    WvEv_CONTROLLER_AcceleratorKeyPressed(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CONTROLLER_AcceleratorKeyPressed() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2AcceleratorKeyPressedEventHandler)) *ppv = (ICoreWebView2AcceleratorKeyPressedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Controller* sender, ICoreWebView2AcceleratorKeyPressedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2AcceleratorKeyPressedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "AcceleratorKeyPressed", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CONTROLLER_GotFocus : public ICoreWebView2FocusChangedEventHandler
{
public:
    WvEv_CONTROLLER_GotFocus(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CONTROLLER_GotFocus() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FocusChangedEventHandler)) *ppv = (ICoreWebView2FocusChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Controller* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "GotFocus", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CONTROLLER_LostFocus : public ICoreWebView2FocusChangedEventHandler
{
public:
    WvEv_CONTROLLER_LostFocus(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CONTROLLER_LostFocus() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FocusChangedEventHandler)) *ppv = (ICoreWebView2FocusChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Controller* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "LostFocus", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CONTROLLER_MoveFocusRequested : public ICoreWebView2MoveFocusRequestedEventHandler
{
public:
    WvEv_CONTROLLER_MoveFocusRequested(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CONTROLLER_MoveFocusRequested() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2MoveFocusRequestedEventHandler)) *ppv = (ICoreWebView2MoveFocusRequestedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Controller* sender, ICoreWebView2MoveFocusRequestedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2MoveFocusRequestedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "MoveFocusRequested", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CONTROLLER_RasterizationScaleChanged : public ICoreWebView2RasterizationScaleChangedEventHandler
{
public:
    WvEv_CONTROLLER_RasterizationScaleChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CONTROLLER_RasterizationScaleChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2RasterizationScaleChangedEventHandler)) *ppv = (ICoreWebView2RasterizationScaleChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Controller* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "RasterizationScaleChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CONTROLLER_ZoomFactorChanged : public ICoreWebView2ZoomFactorChangedEventHandler
{
public:
    WvEv_CONTROLLER_ZoomFactorChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CONTROLLER_ZoomFactorChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2ZoomFactorChangedEventHandler)) *ppv = (ICoreWebView2ZoomFactorChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Controller* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "ZoomFactorChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_BasicAuthenticationRequested : public ICoreWebView2BasicAuthenticationRequestedEventHandler
{
public:
    WvEv_CORE_BasicAuthenticationRequested(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_BasicAuthenticationRequested() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2BasicAuthenticationRequestedEventHandler)) *ppv = (ICoreWebView2BasicAuthenticationRequestedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2BasicAuthenticationRequestedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2BasicAuthenticationRequestedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "BasicAuthenticationRequested", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_ClientCertificateRequested : public ICoreWebView2ClientCertificateRequestedEventHandler
{
public:
    WvEv_CORE_ClientCertificateRequested(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_ClientCertificateRequested() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2ClientCertificateRequestedEventHandler)) *ppv = (ICoreWebView2ClientCertificateRequestedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2ClientCertificateRequestedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2ClientCertificateRequestedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "ClientCertificateRequested", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_ContainsFullScreenElementChanged : public ICoreWebView2ContainsFullScreenElementChangedEventHandler
{
public:
    WvEv_CORE_ContainsFullScreenElementChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_ContainsFullScreenElementChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2ContainsFullScreenElementChangedEventHandler)) *ppv = (ICoreWebView2ContainsFullScreenElementChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "ContainsFullScreenElementChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_ContentLoading : public ICoreWebView2ContentLoadingEventHandler
{
public:
    WvEv_CORE_ContentLoading(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_ContentLoading() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2ContentLoadingEventHandler)) *ppv = (ICoreWebView2ContentLoadingEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2ContentLoadingEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2ContentLoadingEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "ContentLoading", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_ContextMenuRequested : public ICoreWebView2ContextMenuRequestedEventHandler
{
public:
    WvEv_CORE_ContextMenuRequested(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_ContextMenuRequested() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2ContextMenuRequestedEventHandler)) *ppv = (ICoreWebView2ContextMenuRequestedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2ContextMenuRequestedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2ContextMenuRequestedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "ContextMenuRequested", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_DedicatedWorkerCreated : public ICoreWebView2DedicatedWorkerCreatedEventHandler
{
public:
    WvEv_CORE_DedicatedWorkerCreated(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_DedicatedWorkerCreated() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2DedicatedWorkerCreatedEventHandler)) *ppv = (ICoreWebView2DedicatedWorkerCreatedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2DedicatedWorkerCreatedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2DedicatedWorkerCreatedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "DedicatedWorkerCreated", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_DocumentTitleChanged : public ICoreWebView2DocumentTitleChangedEventHandler
{
public:
    WvEv_CORE_DocumentTitleChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_DocumentTitleChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2DocumentTitleChangedEventHandler)) *ppv = (ICoreWebView2DocumentTitleChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "DocumentTitleChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_DOMContentLoaded : public ICoreWebView2DOMContentLoadedEventHandler
{
public:
    WvEv_CORE_DOMContentLoaded(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_DOMContentLoaded() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2DOMContentLoadedEventHandler)) *ppv = (ICoreWebView2DOMContentLoadedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2DOMContentLoadedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2DOMContentLoadedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "DOMContentLoaded", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_DownloadStarting : public ICoreWebView2DownloadStartingEventHandler
{
public:
    WvEv_CORE_DownloadStarting(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_DownloadStarting() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2DownloadStartingEventHandler)) *ppv = (ICoreWebView2DownloadStartingEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2DownloadStartingEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2DownloadStartingEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "DownloadStarting", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_FaviconChanged : public ICoreWebView2FaviconChangedEventHandler
{
public:
    WvEv_CORE_FaviconChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_FaviconChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FaviconChangedEventHandler)) *ppv = (ICoreWebView2FaviconChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "FaviconChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_FrameCreated : public ICoreWebView2FrameCreatedEventHandler
{
public:
    WvEv_CORE_FrameCreated(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_FrameCreated() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FrameCreatedEventHandler)) *ppv = (ICoreWebView2FrameCreatedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2FrameCreatedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2FrameCreatedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "FrameCreated", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_FrameNavigationCompleted : public ICoreWebView2NavigationCompletedEventHandler
{
public:
    WvEv_CORE_FrameNavigationCompleted(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_FrameNavigationCompleted() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2NavigationCompletedEventHandler)) *ppv = (ICoreWebView2NavigationCompletedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2NavigationCompletedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "FrameNavigationCompleted", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_FrameNavigationStarting : public ICoreWebView2NavigationStartingEventHandler
{
public:
    WvEv_CORE_FrameNavigationStarting(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_FrameNavigationStarting() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2NavigationStartingEventHandler)) *ppv = (ICoreWebView2NavigationStartingEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2NavigationStartingEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "FrameNavigationStarting", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_HistoryChanged : public ICoreWebView2HistoryChangedEventHandler
{
public:
    WvEv_CORE_HistoryChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_HistoryChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2HistoryChangedEventHandler)) *ppv = (ICoreWebView2HistoryChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "HistoryChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_IsDefaultDownloadDialogOpenChanged : public ICoreWebView2IsDefaultDownloadDialogOpenChangedEventHandler
{
public:
    WvEv_CORE_IsDefaultDownloadDialogOpenChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_IsDefaultDownloadDialogOpenChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2IsDefaultDownloadDialogOpenChangedEventHandler)) *ppv = (ICoreWebView2IsDefaultDownloadDialogOpenChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "IsDefaultDownloadDialogOpenChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_IsDocumentPlayingAudioChanged : public ICoreWebView2IsDocumentPlayingAudioChangedEventHandler
{
public:
    WvEv_CORE_IsDocumentPlayingAudioChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_IsDocumentPlayingAudioChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2IsDocumentPlayingAudioChangedEventHandler)) *ppv = (ICoreWebView2IsDocumentPlayingAudioChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "IsDocumentPlayingAudioChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_IsMutedChanged : public ICoreWebView2IsMutedChangedEventHandler
{
public:
    WvEv_CORE_IsMutedChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_IsMutedChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2IsMutedChangedEventHandler)) *ppv = (ICoreWebView2IsMutedChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "IsMutedChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_LaunchingExternalUriScheme : public ICoreWebView2LaunchingExternalUriSchemeEventHandler
{
public:
    WvEv_CORE_LaunchingExternalUriScheme(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_LaunchingExternalUriScheme() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2LaunchingExternalUriSchemeEventHandler)) *ppv = (ICoreWebView2LaunchingExternalUriSchemeEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2LaunchingExternalUriSchemeEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2LaunchingExternalUriSchemeEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "LaunchingExternalUriScheme", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_NavigationCompleted : public ICoreWebView2NavigationCompletedEventHandler
{
public:
    WvEv_CORE_NavigationCompleted(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_NavigationCompleted() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2NavigationCompletedEventHandler)) *ppv = (ICoreWebView2NavigationCompletedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2NavigationCompletedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "NavigationCompleted", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_NavigationStarting : public ICoreWebView2NavigationStartingEventHandler
{
public:
    WvEv_CORE_NavigationStarting(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_NavigationStarting() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2NavigationStartingEventHandler)) *ppv = (ICoreWebView2NavigationStartingEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2NavigationStartingEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "NavigationStarting", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_NewWindowRequested : public ICoreWebView2NewWindowRequestedEventHandler
{
public:
    WvEv_CORE_NewWindowRequested(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_NewWindowRequested() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2NewWindowRequestedEventHandler)) *ppv = (ICoreWebView2NewWindowRequestedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2NewWindowRequestedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "NewWindowRequested", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_NotificationReceived : public ICoreWebView2NotificationReceivedEventHandler
{
public:
    WvEv_CORE_NotificationReceived(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_NotificationReceived() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2NotificationReceivedEventHandler)) *ppv = (ICoreWebView2NotificationReceivedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2NotificationReceivedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2NotificationReceivedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "NotificationReceived", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_PermissionRequested : public ICoreWebView2PermissionRequestedEventHandler
{
public:
    WvEv_CORE_PermissionRequested(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_PermissionRequested() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2PermissionRequestedEventHandler)) *ppv = (ICoreWebView2PermissionRequestedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2PermissionRequestedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2PermissionRequestedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "PermissionRequested", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_ProcessFailed : public ICoreWebView2ProcessFailedEventHandler
{
public:
    WvEv_CORE_ProcessFailed(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_ProcessFailed() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2ProcessFailedEventHandler)) *ppv = (ICoreWebView2ProcessFailedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2ProcessFailedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2ProcessFailedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "ProcessFailed", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_SaveAsUIShowing : public ICoreWebView2SaveAsUIShowingEventHandler
{
public:
    WvEv_CORE_SaveAsUIShowing(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_SaveAsUIShowing() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2SaveAsUIShowingEventHandler)) *ppv = (ICoreWebView2SaveAsUIShowingEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2SaveAsUIShowingEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2SaveAsUIShowingEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "SaveAsUIShowing", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_SaveFileSecurityCheckStarting : public ICoreWebView2SaveFileSecurityCheckStartingEventHandler
{
public:
    WvEv_CORE_SaveFileSecurityCheckStarting(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_SaveFileSecurityCheckStarting() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2SaveFileSecurityCheckStartingEventHandler)) *ppv = (ICoreWebView2SaveFileSecurityCheckStartingEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2SaveFileSecurityCheckStartingEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2SaveFileSecurityCheckStartingEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "SaveFileSecurityCheckStarting", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_ScreenCaptureStarting : public ICoreWebView2ScreenCaptureStartingEventHandler
{
public:
    WvEv_CORE_ScreenCaptureStarting(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_ScreenCaptureStarting() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2ScreenCaptureStartingEventHandler)) *ppv = (ICoreWebView2ScreenCaptureStartingEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2ScreenCaptureStartingEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2ScreenCaptureStartingEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "ScreenCaptureStarting", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_ScriptDialogOpening : public ICoreWebView2ScriptDialogOpeningEventHandler
{
public:
    WvEv_CORE_ScriptDialogOpening(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_ScriptDialogOpening() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2ScriptDialogOpeningEventHandler)) *ppv = (ICoreWebView2ScriptDialogOpeningEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2ScriptDialogOpeningEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2ScriptDialogOpeningEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "ScriptDialogOpening", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_ServerCertificateErrorDetected : public ICoreWebView2ServerCertificateErrorDetectedEventHandler
{
public:
    WvEv_CORE_ServerCertificateErrorDetected(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_ServerCertificateErrorDetected() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2ServerCertificateErrorDetectedEventHandler)) *ppv = (ICoreWebView2ServerCertificateErrorDetectedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2ServerCertificateErrorDetectedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2ServerCertificateErrorDetectedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "ServerCertificateErrorDetected", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_SourceChanged : public ICoreWebView2SourceChangedEventHandler
{
public:
    WvEv_CORE_SourceChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_SourceChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2SourceChangedEventHandler)) *ppv = (ICoreWebView2SourceChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2SourceChangedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "SourceChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_StatusBarTextChanged : public ICoreWebView2StatusBarTextChangedEventHandler
{
public:
    WvEv_CORE_StatusBarTextChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_StatusBarTextChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2StatusBarTextChangedEventHandler)) *ppv = (ICoreWebView2StatusBarTextChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "StatusBarTextChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_WebMessageReceived : public ICoreWebView2WebMessageReceivedEventHandler
{
public:
    WvEv_CORE_WebMessageReceived(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_WebMessageReceived() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2WebMessageReceivedEventHandler)) *ppv = (ICoreWebView2WebMessageReceivedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2WebMessageReceivedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "WebMessageReceived", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_WebResourceRequested : public ICoreWebView2WebResourceRequestedEventHandler
{
public:
    WvEv_CORE_WebResourceRequested(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_WebResourceRequested() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2WebResourceRequestedEventHandler)) *ppv = (ICoreWebView2WebResourceRequestedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2WebResourceRequestedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "WebResourceRequested", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_WebResourceResponseReceived : public ICoreWebView2WebResourceResponseReceivedEventHandler
{
public:
    WvEv_CORE_WebResourceResponseReceived(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_WebResourceResponseReceived() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2WebResourceResponseReceivedEventHandler)) *ppv = (ICoreWebView2WebResourceResponseReceivedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2WebResourceResponseReceivedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2WebResourceResponseReceivedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "WebResourceResponseReceived", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_CORE_WindowCloseRequested : public ICoreWebView2WindowCloseRequestedEventHandler
{
public:
    WvEv_CORE_WindowCloseRequested(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_CORE_WindowCloseRequested() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2WindowCloseRequestedEventHandler)) *ppv = (ICoreWebView2WindowCloseRequestedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "WindowCloseRequested", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_DOWNLOAD_BytesReceivedChanged : public ICoreWebView2BytesReceivedChangedEventHandler
{
public:
    WvEv_DOWNLOAD_BytesReceivedChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_DOWNLOAD_BytesReceivedChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2BytesReceivedChangedEventHandler)) *ppv = (ICoreWebView2BytesReceivedChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2DownloadOperation* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "BytesReceivedChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_DOWNLOAD_EstimatedEndTimeChanged : public ICoreWebView2EstimatedEndTimeChangedEventHandler
{
public:
    WvEv_DOWNLOAD_EstimatedEndTimeChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_DOWNLOAD_EstimatedEndTimeChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2EstimatedEndTimeChangedEventHandler)) *ppv = (ICoreWebView2EstimatedEndTimeChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2DownloadOperation* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "EstimatedEndTimeChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_DOWNLOAD_StateChanged : public ICoreWebView2StateChangedEventHandler
{
public:
    WvEv_DOWNLOAD_StateChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_DOWNLOAD_StateChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2StateChangedEventHandler)) *ppv = (ICoreWebView2StateChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2DownloadOperation* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "StateChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_ENV_BrowserProcessExited : public ICoreWebView2BrowserProcessExitedEventHandler
{
public:
    WvEv_ENV_BrowserProcessExited(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_ENV_BrowserProcessExited() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2BrowserProcessExitedEventHandler)) *ppv = (ICoreWebView2BrowserProcessExitedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Environment* sender, ICoreWebView2BrowserProcessExitedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2BrowserProcessExitedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "BrowserProcessExited", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_ENV_NewBrowserVersionAvailable : public ICoreWebView2NewBrowserVersionAvailableEventHandler
{
public:
    WvEv_ENV_NewBrowserVersionAvailable(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_ENV_NewBrowserVersionAvailable() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2NewBrowserVersionAvailableEventHandler)) *ppv = (ICoreWebView2NewBrowserVersionAvailableEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Environment* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "NewBrowserVersionAvailable", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_ENV_ProcessInfosChanged : public ICoreWebView2ProcessInfosChangedEventHandler
{
public:
    WvEv_ENV_ProcessInfosChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_ENV_ProcessInfosChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2ProcessInfosChangedEventHandler)) *ppv = (ICoreWebView2ProcessInfosChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Environment* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "ProcessInfosChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_FRAME_ContentLoading : public ICoreWebView2FrameContentLoadingEventHandler
{
public:
    WvEv_FRAME_ContentLoading(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_FRAME_ContentLoading() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FrameContentLoadingEventHandler)) *ppv = (ICoreWebView2FrameContentLoadingEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Frame* sender, ICoreWebView2ContentLoadingEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2ContentLoadingEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "ContentLoading", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_FRAME_DedicatedWorkerCreated : public ICoreWebView2FrameDedicatedWorkerCreatedEventHandler
{
public:
    WvEv_FRAME_DedicatedWorkerCreated(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_FRAME_DedicatedWorkerCreated() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FrameDedicatedWorkerCreatedEventHandler)) *ppv = (ICoreWebView2FrameDedicatedWorkerCreatedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Frame* sender, ICoreWebView2DedicatedWorkerCreatedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2DedicatedWorkerCreatedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "DedicatedWorkerCreated", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_FRAME_Destroyed : public ICoreWebView2FrameDestroyedEventHandler
{
public:
    WvEv_FRAME_Destroyed(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_FRAME_Destroyed() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FrameDestroyedEventHandler)) *ppv = (ICoreWebView2FrameDestroyedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Frame* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "Destroyed", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_FRAME_DOMContentLoaded : public ICoreWebView2FrameDOMContentLoadedEventHandler
{
public:
    WvEv_FRAME_DOMContentLoaded(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_FRAME_DOMContentLoaded() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FrameDOMContentLoadedEventHandler)) *ppv = (ICoreWebView2FrameDOMContentLoadedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Frame* sender, ICoreWebView2DOMContentLoadedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2DOMContentLoadedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "DOMContentLoaded", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_FRAME_FrameCreated : public ICoreWebView2FrameChildFrameCreatedEventHandler
{
public:
    WvEv_FRAME_FrameCreated(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_FRAME_FrameCreated() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FrameChildFrameCreatedEventHandler)) *ppv = (ICoreWebView2FrameChildFrameCreatedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Frame* sender, ICoreWebView2FrameCreatedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2FrameCreatedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "FrameCreated", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_FRAME_NameChanged : public ICoreWebView2FrameNameChangedEventHandler
{
public:
    WvEv_FRAME_NameChanged(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_FRAME_NameChanged() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FrameNameChangedEventHandler)) *ppv = (ICoreWebView2FrameNameChangedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Frame* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "NameChanged", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_FRAME_NavigationCompleted : public ICoreWebView2FrameNavigationCompletedEventHandler
{
public:
    WvEv_FRAME_NavigationCompleted(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_FRAME_NavigationCompleted() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FrameNavigationCompletedEventHandler)) *ppv = (ICoreWebView2FrameNavigationCompletedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Frame* sender, ICoreWebView2NavigationCompletedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2NavigationCompletedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "NavigationCompleted", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_FRAME_NavigationStarting : public ICoreWebView2FrameNavigationStartingEventHandler
{
public:
    WvEv_FRAME_NavigationStarting(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_FRAME_NavigationStarting() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FrameNavigationStartingEventHandler)) *ppv = (ICoreWebView2FrameNavigationStartingEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Frame* sender, ICoreWebView2NavigationStartingEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2NavigationStartingEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "NavigationStarting", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_FRAME_PermissionRequested : public ICoreWebView2FramePermissionRequestedEventHandler
{
public:
    WvEv_FRAME_PermissionRequested(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_FRAME_PermissionRequested() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FramePermissionRequestedEventHandler)) *ppv = (ICoreWebView2FramePermissionRequestedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Frame* sender, ICoreWebView2PermissionRequestedEventArgs2* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2PermissionRequestedEventArgs2(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "PermissionRequested", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_FRAME_ScreenCaptureStarting : public ICoreWebView2FrameScreenCaptureStartingEventHandler
{
public:
    WvEv_FRAME_ScreenCaptureStarting(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_FRAME_ScreenCaptureStarting() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FrameScreenCaptureStartingEventHandler)) *ppv = (ICoreWebView2FrameScreenCaptureStartingEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Frame* sender, ICoreWebView2ScreenCaptureStartingEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2ScreenCaptureStartingEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "ScreenCaptureStarting", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_FRAME_WebMessageReceived : public ICoreWebView2FrameWebMessageReceivedEventHandler
{
public:
    WvEv_FRAME_WebMessageReceived(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_FRAME_WebMessageReceived() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2FrameWebMessageReceivedEventHandler)) *ppv = (ICoreWebView2FrameWebMessageReceivedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Frame* sender, ICoreWebView2WebMessageReceivedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2WebMessageReceivedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "WebMessageReceived", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_PROFILE_Deleted : public ICoreWebView2ProfileDeletedEventHandler
{
public:
    WvEv_PROFILE_Deleted(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_PROFILE_Deleted() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2ProfileDeletedEventHandler)) *ppv = (ICoreWebView2ProfileDeletedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2Profile* sender, IUnknown* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "Deleted", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

class WvEv_RECEIVER_DevToolsProtocolEventReceived : public ICoreWebView2DevToolsProtocolEventReceivedEventHandler
{
public:
    WvEv_RECEIVER_DevToolsProtocolEventReceived(WvEvtBind b) : m_ref(1), m_bind(b) {}
    virtual ~WvEv_RECEIVER_DevToolsProtocolEventReceived() {}
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ICoreWebView2DevToolsProtocolEventReceivedEventHandler)) *ppv = (ICoreWebView2DevToolsProtocolEventReceivedEventHandler*)this;
        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }
    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }
    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }
    STDMETHODIMP Invoke(ICoreWebView2* sender, ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args)
    {
        UNREF(sender);
        char json[WV_EVT_JSON_MAX]; json[0] = '\0';
        { int n = 0; Desc_ICoreWebView2DevToolsProtocolEventReceivedEventArgs(args, json, (int)sizeof(json), &n); }
        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "DevToolsProtocolEventReceived", json, (void*)args);
        return S_OK;
    }
private:
    LONG m_ref; WvEvtBind m_bind;
};

static BOOL Sub_ICoreWebView2Controller_AcceleratorKeyPressed(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Controller* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return FALSE;
    WvEv_CONTROLLER_AcceleratorKeyPressed* h = new WvEv_CONTROLLER_AcceleratorKeyPressed(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_AcceleratorKeyPressed(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Controller_AcceleratorKeyPressed(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Controller* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_AcceleratorKeyPressed(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Controller_GotFocus(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Controller* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return FALSE;
    WvEv_CONTROLLER_GotFocus* h = new WvEv_CONTROLLER_GotFocus(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_GotFocus(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Controller_GotFocus(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Controller* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_GotFocus(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Controller_LostFocus(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Controller* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return FALSE;
    WvEv_CONTROLLER_LostFocus* h = new WvEv_CONTROLLER_LostFocus(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_LostFocus(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Controller_LostFocus(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Controller* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_LostFocus(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Controller_MoveFocusRequested(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Controller* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return FALSE;
    WvEv_CONTROLLER_MoveFocusRequested* h = new WvEv_CONTROLLER_MoveFocusRequested(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_MoveFocusRequested(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Controller_MoveFocusRequested(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Controller* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_MoveFocusRequested(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Controller3_RasterizationScaleChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Controller3* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Controller3), (void**)&o))) return FALSE;
    WvEv_CONTROLLER_RasterizationScaleChanged* h = new WvEv_CONTROLLER_RasterizationScaleChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_RasterizationScaleChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Controller3_RasterizationScaleChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Controller3* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Controller3), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_RasterizationScaleChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Controller_ZoomFactorChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Controller* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return FALSE;
    WvEv_CONTROLLER_ZoomFactorChanged* h = new WvEv_CONTROLLER_ZoomFactorChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_ZoomFactorChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Controller_ZoomFactorChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Controller* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Controller), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_ZoomFactorChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_10_BasicAuthenticationRequested(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_10* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_10), (void**)&o))) return FALSE;
    WvEv_CORE_BasicAuthenticationRequested* h = new WvEv_CORE_BasicAuthenticationRequested(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_BasicAuthenticationRequested(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_10_BasicAuthenticationRequested(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_10* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_10), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_BasicAuthenticationRequested(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_5_ClientCertificateRequested(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_5* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_5), (void**)&o))) return FALSE;
    WvEv_CORE_ClientCertificateRequested* h = new WvEv_CORE_ClientCertificateRequested(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_ClientCertificateRequested(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_5_ClientCertificateRequested(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_5* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_5), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_ClientCertificateRequested(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_ContainsFullScreenElementChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_ContainsFullScreenElementChanged* h = new WvEv_CORE_ContainsFullScreenElementChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_ContainsFullScreenElementChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_ContainsFullScreenElementChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_ContainsFullScreenElementChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_ContentLoading(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_ContentLoading* h = new WvEv_CORE_ContentLoading(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_ContentLoading(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_ContentLoading(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_ContentLoading(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_11_ContextMenuRequested(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_11* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_11), (void**)&o))) return FALSE;
    WvEv_CORE_ContextMenuRequested* h = new WvEv_CORE_ContextMenuRequested(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_ContextMenuRequested(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_11_ContextMenuRequested(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_11* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_11), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_ContextMenuRequested(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_29_DedicatedWorkerCreated(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_29* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_29), (void**)&o))) return FALSE;
    WvEv_CORE_DedicatedWorkerCreated* h = new WvEv_CORE_DedicatedWorkerCreated(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_DedicatedWorkerCreated(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_29_DedicatedWorkerCreated(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_29* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_29), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_DedicatedWorkerCreated(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_DocumentTitleChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_DocumentTitleChanged* h = new WvEv_CORE_DocumentTitleChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_DocumentTitleChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_DocumentTitleChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_DocumentTitleChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_2_DOMContentLoaded(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_2), (void**)&o))) return FALSE;
    WvEv_CORE_DOMContentLoaded* h = new WvEv_CORE_DOMContentLoaded(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_DOMContentLoaded(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_2_DOMContentLoaded(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_DOMContentLoaded(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_4_DownloadStarting(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_4* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_4), (void**)&o))) return FALSE;
    WvEv_CORE_DownloadStarting* h = new WvEv_CORE_DownloadStarting(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_DownloadStarting(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_4_DownloadStarting(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_4* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_4), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_DownloadStarting(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_15_FaviconChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_15* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_15), (void**)&o))) return FALSE;
    WvEv_CORE_FaviconChanged* h = new WvEv_CORE_FaviconChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_FaviconChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_15_FaviconChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_15* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_15), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_FaviconChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_4_FrameCreated(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_4* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_4), (void**)&o))) return FALSE;
    WvEv_CORE_FrameCreated* h = new WvEv_CORE_FrameCreated(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_FrameCreated(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_4_FrameCreated(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_4* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_4), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_FrameCreated(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_FrameNavigationCompleted(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_FrameNavigationCompleted* h = new WvEv_CORE_FrameNavigationCompleted(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_FrameNavigationCompleted(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_FrameNavigationCompleted(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_FrameNavigationCompleted(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_FrameNavigationStarting(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_FrameNavigationStarting* h = new WvEv_CORE_FrameNavigationStarting(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_FrameNavigationStarting(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_FrameNavigationStarting(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_FrameNavigationStarting(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_HistoryChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_HistoryChanged* h = new WvEv_CORE_HistoryChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_HistoryChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_HistoryChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_HistoryChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_9_IsDefaultDownloadDialogOpenChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_9* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_9), (void**)&o))) return FALSE;
    WvEv_CORE_IsDefaultDownloadDialogOpenChanged* h = new WvEv_CORE_IsDefaultDownloadDialogOpenChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_IsDefaultDownloadDialogOpenChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_9_IsDefaultDownloadDialogOpenChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_9* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_9), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_IsDefaultDownloadDialogOpenChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_8_IsDocumentPlayingAudioChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_8* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_8), (void**)&o))) return FALSE;
    WvEv_CORE_IsDocumentPlayingAudioChanged* h = new WvEv_CORE_IsDocumentPlayingAudioChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_IsDocumentPlayingAudioChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_8_IsDocumentPlayingAudioChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_8* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_8), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_IsDocumentPlayingAudioChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_8_IsMutedChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_8* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_8), (void**)&o))) return FALSE;
    WvEv_CORE_IsMutedChanged* h = new WvEv_CORE_IsMutedChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_IsMutedChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_8_IsMutedChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_8* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_8), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_IsMutedChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_18_LaunchingExternalUriScheme(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_18* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_18), (void**)&o))) return FALSE;
    WvEv_CORE_LaunchingExternalUriScheme* h = new WvEv_CORE_LaunchingExternalUriScheme(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_LaunchingExternalUriScheme(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_18_LaunchingExternalUriScheme(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_18* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_18), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_LaunchingExternalUriScheme(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_NavigationCompleted(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_NavigationCompleted* h = new WvEv_CORE_NavigationCompleted(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_NavigationCompleted(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_NavigationCompleted(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_NavigationCompleted(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_NavigationStarting(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_NavigationStarting* h = new WvEv_CORE_NavigationStarting(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_NavigationStarting(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_NavigationStarting(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_NavigationStarting(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_NewWindowRequested(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_NewWindowRequested* h = new WvEv_CORE_NewWindowRequested(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_NewWindowRequested(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_NewWindowRequested(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_NewWindowRequested(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_24_NotificationReceived(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_24* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_24), (void**)&o))) return FALSE;
    WvEv_CORE_NotificationReceived* h = new WvEv_CORE_NotificationReceived(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_NotificationReceived(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_24_NotificationReceived(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_24* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_24), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_NotificationReceived(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_PermissionRequested(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_PermissionRequested* h = new WvEv_CORE_PermissionRequested(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_PermissionRequested(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_PermissionRequested(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_PermissionRequested(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_ProcessFailed(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_ProcessFailed* h = new WvEv_CORE_ProcessFailed(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_ProcessFailed(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_ProcessFailed(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_ProcessFailed(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_25_SaveAsUIShowing(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_25* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_25), (void**)&o))) return FALSE;
    WvEv_CORE_SaveAsUIShowing* h = new WvEv_CORE_SaveAsUIShowing(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_SaveAsUIShowing(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_25_SaveAsUIShowing(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_25* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_25), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_SaveAsUIShowing(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_26_SaveFileSecurityCheckStarting(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_26* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_26), (void**)&o))) return FALSE;
    WvEv_CORE_SaveFileSecurityCheckStarting* h = new WvEv_CORE_SaveFileSecurityCheckStarting(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_SaveFileSecurityCheckStarting(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_26_SaveFileSecurityCheckStarting(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_26* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_26), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_SaveFileSecurityCheckStarting(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_27_ScreenCaptureStarting(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_27* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_27), (void**)&o))) return FALSE;
    WvEv_CORE_ScreenCaptureStarting* h = new WvEv_CORE_ScreenCaptureStarting(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_ScreenCaptureStarting(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_27_ScreenCaptureStarting(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_27* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_27), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_ScreenCaptureStarting(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_ScriptDialogOpening(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_ScriptDialogOpening* h = new WvEv_CORE_ScriptDialogOpening(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_ScriptDialogOpening(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_ScriptDialogOpening(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_ScriptDialogOpening(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_14_ServerCertificateErrorDetected(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_14* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_14), (void**)&o))) return FALSE;
    WvEv_CORE_ServerCertificateErrorDetected* h = new WvEv_CORE_ServerCertificateErrorDetected(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_ServerCertificateErrorDetected(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_14_ServerCertificateErrorDetected(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_14* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_14), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_ServerCertificateErrorDetected(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_SourceChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_SourceChanged* h = new WvEv_CORE_SourceChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_SourceChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_SourceChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_SourceChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_12_StatusBarTextChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_12* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_12), (void**)&o))) return FALSE;
    WvEv_CORE_StatusBarTextChanged* h = new WvEv_CORE_StatusBarTextChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_StatusBarTextChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_12_StatusBarTextChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_12* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_12), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_StatusBarTextChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_WebMessageReceived(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_WebMessageReceived* h = new WvEv_CORE_WebMessageReceived(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_WebMessageReceived(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_WebMessageReceived(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_WebMessageReceived(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_WebResourceRequested(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_WebResourceRequested* h = new WvEv_CORE_WebResourceRequested(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_WebResourceRequested(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_WebResourceRequested(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_WebResourceRequested(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_2_WebResourceResponseReceived(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_2), (void**)&o))) return FALSE;
    WvEv_CORE_WebResourceResponseReceived* h = new WvEv_CORE_WebResourceResponseReceived(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_WebResourceResponseReceived(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_2_WebResourceResponseReceived(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2_2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2_2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_WebResourceResponseReceived(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2_WindowCloseRequested(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    WvEv_CORE_WindowCloseRequested* h = new WvEv_CORE_WindowCloseRequested(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_WindowCloseRequested(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2_WindowCloseRequested(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_WindowCloseRequested(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2DownloadOperation_BytesReceivedChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return FALSE;
    WvEv_DOWNLOAD_BytesReceivedChanged* h = new WvEv_DOWNLOAD_BytesReceivedChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_BytesReceivedChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2DownloadOperation_BytesReceivedChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_BytesReceivedChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2DownloadOperation_EstimatedEndTimeChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return FALSE;
    WvEv_DOWNLOAD_EstimatedEndTimeChanged* h = new WvEv_DOWNLOAD_EstimatedEndTimeChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_EstimatedEndTimeChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2DownloadOperation_EstimatedEndTimeChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_EstimatedEndTimeChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2DownloadOperation_StateChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return FALSE;
    WvEv_DOWNLOAD_StateChanged* h = new WvEv_DOWNLOAD_StateChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_StateChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2DownloadOperation_StateChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2DownloadOperation* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2DownloadOperation), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_StateChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Environment5_BrowserProcessExited(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Environment5* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Environment5), (void**)&o))) return FALSE;
    WvEv_ENV_BrowserProcessExited* h = new WvEv_ENV_BrowserProcessExited(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_BrowserProcessExited(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Environment5_BrowserProcessExited(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Environment5* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Environment5), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_BrowserProcessExited(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Environment_NewBrowserVersionAvailable(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Environment* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Environment), (void**)&o))) return FALSE;
    WvEv_ENV_NewBrowserVersionAvailable* h = new WvEv_ENV_NewBrowserVersionAvailable(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_NewBrowserVersionAvailable(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Environment_NewBrowserVersionAvailable(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Environment* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Environment), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_NewBrowserVersionAvailable(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Environment8_ProcessInfosChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Environment8* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Environment8), (void**)&o))) return FALSE;
    WvEv_ENV_ProcessInfosChanged* h = new WvEv_ENV_ProcessInfosChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_ProcessInfosChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Environment8_ProcessInfosChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Environment8* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Environment8), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_ProcessInfosChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Frame2_ContentLoading(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame2), (void**)&o))) return FALSE;
    WvEv_FRAME_ContentLoading* h = new WvEv_FRAME_ContentLoading(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_ContentLoading(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Frame2_ContentLoading(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_ContentLoading(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Frame8_DedicatedWorkerCreated(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame8* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame8), (void**)&o))) return FALSE;
    WvEv_FRAME_DedicatedWorkerCreated* h = new WvEv_FRAME_DedicatedWorkerCreated(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_DedicatedWorkerCreated(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Frame8_DedicatedWorkerCreated(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame8* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame8), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_DedicatedWorkerCreated(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Frame_Destroyed(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame), (void**)&o))) return FALSE;
    WvEv_FRAME_Destroyed* h = new WvEv_FRAME_Destroyed(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_Destroyed(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Frame_Destroyed(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_Destroyed(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Frame2_DOMContentLoaded(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame2), (void**)&o))) return FALSE;
    WvEv_FRAME_DOMContentLoaded* h = new WvEv_FRAME_DOMContentLoaded(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_DOMContentLoaded(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Frame2_DOMContentLoaded(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_DOMContentLoaded(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Frame7_FrameCreated(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame7* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame7), (void**)&o))) return FALSE;
    WvEv_FRAME_FrameCreated* h = new WvEv_FRAME_FrameCreated(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_FrameCreated(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Frame7_FrameCreated(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame7* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame7), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_FrameCreated(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Frame_NameChanged(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame), (void**)&o))) return FALSE;
    WvEv_FRAME_NameChanged* h = new WvEv_FRAME_NameChanged(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_NameChanged(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Frame_NameChanged(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_NameChanged(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Frame2_NavigationCompleted(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame2), (void**)&o))) return FALSE;
    WvEv_FRAME_NavigationCompleted* h = new WvEv_FRAME_NavigationCompleted(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_NavigationCompleted(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Frame2_NavigationCompleted(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_NavigationCompleted(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Frame2_NavigationStarting(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame2), (void**)&o))) return FALSE;
    WvEv_FRAME_NavigationStarting* h = new WvEv_FRAME_NavigationStarting(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_NavigationStarting(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Frame2_NavigationStarting(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_NavigationStarting(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Frame3_PermissionRequested(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame3* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame3), (void**)&o))) return FALSE;
    WvEv_FRAME_PermissionRequested* h = new WvEv_FRAME_PermissionRequested(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_PermissionRequested(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Frame3_PermissionRequested(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame3* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame3), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_PermissionRequested(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Frame6_ScreenCaptureStarting(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame6* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame6), (void**)&o))) return FALSE;
    WvEv_FRAME_ScreenCaptureStarting* h = new WvEv_FRAME_ScreenCaptureStarting(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_ScreenCaptureStarting(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Frame6_ScreenCaptureStarting(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame6* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame6), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_ScreenCaptureStarting(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Frame2_WebMessageReceived(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame2), (void**)&o))) return FALSE;
    WvEv_FRAME_WebMessageReceived* h = new WvEv_FRAME_WebMessageReceived(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_WebMessageReceived(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Frame2_WebMessageReceived(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Frame2* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Frame2), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_WebMessageReceived(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2Profile8_Deleted(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Profile8* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Profile8), (void**)&o))) return FALSE;
    WvEv_PROFILE_Deleted* h = new WvEv_PROFILE_Deleted(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_Deleted(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2Profile8_Deleted(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2Profile8* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2Profile8), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_Deleted(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

static BOOL Sub_ICoreWebView2DevToolsProtocolEventReceiver_DevToolsProtocolEventReceived(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2DevToolsProtocolEventReceiver* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2DevToolsProtocolEventReceiver), (void**)&o))) return FALSE;
    WvEv_RECEIVER_DevToolsProtocolEventReceived* h = new WvEv_RECEIVER_DevToolsProtocolEventReceived(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));
    const HRESULT hr = o->add_DevToolsProtocolEventReceived(h, &tk); h->Release(); o->Release();
    if (FAILED(hr) || pToken == NULL) return FALSE;
    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;
}
static BOOL Unsub_ICoreWebView2DevToolsProtocolEventReceiver_DevToolsProtocolEventReceived(IUnknown* pTarget, INT64 token)
{
    if (pTarget == NULL) return FALSE;
    ICoreWebView2DevToolsProtocolEventReceiver* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(ICoreWebView2DevToolsProtocolEventReceiver), (void**)&o))) return FALSE;
    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));
    const HRESULT hr = o->remove_DevToolsProtocolEventReceived(tk); o->Release();
    return SUCCEEDED(hr) ? TRUE : FALSE;
}

struct WvEvtItem { LPCSTR m_szName; BOOL (*m_pfnSub)(IUnknown*, WvEvtBind, INT64*); BOOL (*m_pfnUnsub)(IUnknown*, INT64); };

static const WvEvtItem g_evtCORE[] = {
    { "BasicAuthenticationRequested", Sub_ICoreWebView2_10_BasicAuthenticationRequested, Unsub_ICoreWebView2_10_BasicAuthenticationRequested },
    { "ClientCertificateRequested", Sub_ICoreWebView2_5_ClientCertificateRequested, Unsub_ICoreWebView2_5_ClientCertificateRequested },
    { "ContainsFullScreenElementChanged", Sub_ICoreWebView2_ContainsFullScreenElementChanged, Unsub_ICoreWebView2_ContainsFullScreenElementChanged },
    { "ContentLoading", Sub_ICoreWebView2_ContentLoading, Unsub_ICoreWebView2_ContentLoading },
    { "ContextMenuRequested", Sub_ICoreWebView2_11_ContextMenuRequested, Unsub_ICoreWebView2_11_ContextMenuRequested },
    { "DedicatedWorkerCreated", Sub_ICoreWebView2_29_DedicatedWorkerCreated, Unsub_ICoreWebView2_29_DedicatedWorkerCreated },
    { "DocumentTitleChanged", Sub_ICoreWebView2_DocumentTitleChanged, Unsub_ICoreWebView2_DocumentTitleChanged },
    { "DOMContentLoaded", Sub_ICoreWebView2_2_DOMContentLoaded, Unsub_ICoreWebView2_2_DOMContentLoaded },
    { "DownloadStarting", Sub_ICoreWebView2_4_DownloadStarting, Unsub_ICoreWebView2_4_DownloadStarting },
    { "FaviconChanged", Sub_ICoreWebView2_15_FaviconChanged, Unsub_ICoreWebView2_15_FaviconChanged },
    { "FrameCreated", Sub_ICoreWebView2_4_FrameCreated, Unsub_ICoreWebView2_4_FrameCreated },
    { "FrameNavigationCompleted", Sub_ICoreWebView2_FrameNavigationCompleted, Unsub_ICoreWebView2_FrameNavigationCompleted },
    { "FrameNavigationStarting", Sub_ICoreWebView2_FrameNavigationStarting, Unsub_ICoreWebView2_FrameNavigationStarting },
    { "HistoryChanged", Sub_ICoreWebView2_HistoryChanged, Unsub_ICoreWebView2_HistoryChanged },
    { "IsDefaultDownloadDialogOpenChanged", Sub_ICoreWebView2_9_IsDefaultDownloadDialogOpenChanged, Unsub_ICoreWebView2_9_IsDefaultDownloadDialogOpenChanged },
    { "IsDocumentPlayingAudioChanged", Sub_ICoreWebView2_8_IsDocumentPlayingAudioChanged, Unsub_ICoreWebView2_8_IsDocumentPlayingAudioChanged },
    { "IsMutedChanged", Sub_ICoreWebView2_8_IsMutedChanged, Unsub_ICoreWebView2_8_IsMutedChanged },
    { "LaunchingExternalUriScheme", Sub_ICoreWebView2_18_LaunchingExternalUriScheme, Unsub_ICoreWebView2_18_LaunchingExternalUriScheme },
    { "NavigationCompleted", Sub_ICoreWebView2_NavigationCompleted, Unsub_ICoreWebView2_NavigationCompleted },
    { "NavigationStarting", Sub_ICoreWebView2_NavigationStarting, Unsub_ICoreWebView2_NavigationStarting },
    { "NewWindowRequested", Sub_ICoreWebView2_NewWindowRequested, Unsub_ICoreWebView2_NewWindowRequested },
    { "NotificationReceived", Sub_ICoreWebView2_24_NotificationReceived, Unsub_ICoreWebView2_24_NotificationReceived },
    { "PermissionRequested", Sub_ICoreWebView2_PermissionRequested, Unsub_ICoreWebView2_PermissionRequested },
    { "ProcessFailed", Sub_ICoreWebView2_ProcessFailed, Unsub_ICoreWebView2_ProcessFailed },
    { "SaveAsUIShowing", Sub_ICoreWebView2_25_SaveAsUIShowing, Unsub_ICoreWebView2_25_SaveAsUIShowing },
    { "SaveFileSecurityCheckStarting", Sub_ICoreWebView2_26_SaveFileSecurityCheckStarting, Unsub_ICoreWebView2_26_SaveFileSecurityCheckStarting },
    { "ScreenCaptureStarting", Sub_ICoreWebView2_27_ScreenCaptureStarting, Unsub_ICoreWebView2_27_ScreenCaptureStarting },
    { "ScriptDialogOpening", Sub_ICoreWebView2_ScriptDialogOpening, Unsub_ICoreWebView2_ScriptDialogOpening },
    { "ServerCertificateErrorDetected", Sub_ICoreWebView2_14_ServerCertificateErrorDetected, Unsub_ICoreWebView2_14_ServerCertificateErrorDetected },
    { "SourceChanged", Sub_ICoreWebView2_SourceChanged, Unsub_ICoreWebView2_SourceChanged },
    { "StatusBarTextChanged", Sub_ICoreWebView2_12_StatusBarTextChanged, Unsub_ICoreWebView2_12_StatusBarTextChanged },
    { "WebMessageReceived", Sub_ICoreWebView2_WebMessageReceived, Unsub_ICoreWebView2_WebMessageReceived },
    { "WebResourceRequested", Sub_ICoreWebView2_WebResourceRequested, Unsub_ICoreWebView2_WebResourceRequested },
    { "WebResourceResponseReceived", Sub_ICoreWebView2_2_WebResourceResponseReceived, Unsub_ICoreWebView2_2_WebResourceResponseReceived },
    { "WindowCloseRequested", Sub_ICoreWebView2_WindowCloseRequested, Unsub_ICoreWebView2_WindowCloseRequested },
};

static const WvEvtItem g_evtCONTROLLER[] = {
    { "AcceleratorKeyPressed", Sub_ICoreWebView2Controller_AcceleratorKeyPressed, Unsub_ICoreWebView2Controller_AcceleratorKeyPressed },
    { "GotFocus", Sub_ICoreWebView2Controller_GotFocus, Unsub_ICoreWebView2Controller_GotFocus },
    { "LostFocus", Sub_ICoreWebView2Controller_LostFocus, Unsub_ICoreWebView2Controller_LostFocus },
    { "MoveFocusRequested", Sub_ICoreWebView2Controller_MoveFocusRequested, Unsub_ICoreWebView2Controller_MoveFocusRequested },
    { "RasterizationScaleChanged", Sub_ICoreWebView2Controller3_RasterizationScaleChanged, Unsub_ICoreWebView2Controller3_RasterizationScaleChanged },
    { "ZoomFactorChanged", Sub_ICoreWebView2Controller_ZoomFactorChanged, Unsub_ICoreWebView2Controller_ZoomFactorChanged },
};

static const WvEvtItem g_evtENV[] = {
    { "BrowserProcessExited", Sub_ICoreWebView2Environment5_BrowserProcessExited, Unsub_ICoreWebView2Environment5_BrowserProcessExited },
    { "NewBrowserVersionAvailable", Sub_ICoreWebView2Environment_NewBrowserVersionAvailable, Unsub_ICoreWebView2Environment_NewBrowserVersionAvailable },
    { "ProcessInfosChanged", Sub_ICoreWebView2Environment8_ProcessInfosChanged, Unsub_ICoreWebView2Environment8_ProcessInfosChanged },
};

static const WvEvtItem g_evtPROFILE[] = {
    { "Deleted", Sub_ICoreWebView2Profile8_Deleted, Unsub_ICoreWebView2Profile8_Deleted },
};

static const WvEvtItem g_evtFRAME[] = {
    { "ContentLoading", Sub_ICoreWebView2Frame2_ContentLoading, Unsub_ICoreWebView2Frame2_ContentLoading },
    { "DedicatedWorkerCreated", Sub_ICoreWebView2Frame8_DedicatedWorkerCreated, Unsub_ICoreWebView2Frame8_DedicatedWorkerCreated },
    { "Destroyed", Sub_ICoreWebView2Frame_Destroyed, Unsub_ICoreWebView2Frame_Destroyed },
    { "DOMContentLoaded", Sub_ICoreWebView2Frame2_DOMContentLoaded, Unsub_ICoreWebView2Frame2_DOMContentLoaded },
    { "FrameCreated", Sub_ICoreWebView2Frame7_FrameCreated, Unsub_ICoreWebView2Frame7_FrameCreated },
    { "NameChanged", Sub_ICoreWebView2Frame_NameChanged, Unsub_ICoreWebView2Frame_NameChanged },
    { "NavigationCompleted", Sub_ICoreWebView2Frame2_NavigationCompleted, Unsub_ICoreWebView2Frame2_NavigationCompleted },
    { "NavigationStarting", Sub_ICoreWebView2Frame2_NavigationStarting, Unsub_ICoreWebView2Frame2_NavigationStarting },
    { "PermissionRequested", Sub_ICoreWebView2Frame3_PermissionRequested, Unsub_ICoreWebView2Frame3_PermissionRequested },
    { "ScreenCaptureStarting", Sub_ICoreWebView2Frame6_ScreenCaptureStarting, Unsub_ICoreWebView2Frame6_ScreenCaptureStarting },
    { "WebMessageReceived", Sub_ICoreWebView2Frame2_WebMessageReceived, Unsub_ICoreWebView2Frame2_WebMessageReceived },
};

static const WvEvtItem g_evtDOWNLOAD[] = {
    { "BytesReceivedChanged", Sub_ICoreWebView2DownloadOperation_BytesReceivedChanged, Unsub_ICoreWebView2DownloadOperation_BytesReceivedChanged },
    { "EstimatedEndTimeChanged", Sub_ICoreWebView2DownloadOperation_EstimatedEndTimeChanged, Unsub_ICoreWebView2DownloadOperation_EstimatedEndTimeChanged },
    { "StateChanged", Sub_ICoreWebView2DownloadOperation_StateChanged, Unsub_ICoreWebView2DownloadOperation_StateChanged },
};

static const WvEvtItem g_evtRECEIVER[] = {
    { "DevToolsProtocolEventReceived", Sub_ICoreWebView2DevToolsProtocolEventReceiver_DevToolsProtocolEventReceived, Unsub_ICoreWebView2DevToolsProtocolEventReceiver_DevToolsProtocolEventReceived },
};

struct WvEvtGroup { LPCSTR m_szGroup; const WvEvtItem* m_pItems; INT m_nCount; };
static const WvEvtGroup g_evtGroups[] = {
    { "CORE", g_evtCORE, (INT)(sizeof(g_evtCORE) / sizeof(g_evtCORE[0])) },
    { "CONTROLLER", g_evtCONTROLLER, (INT)(sizeof(g_evtCONTROLLER) / sizeof(g_evtCONTROLLER[0])) },
    { "ENV", g_evtENV, (INT)(sizeof(g_evtENV) / sizeof(g_evtENV[0])) },
    { "PROFILE", g_evtPROFILE, (INT)(sizeof(g_evtPROFILE) / sizeof(g_evtPROFILE[0])) },
    { "FRAME", g_evtFRAME, (INT)(sizeof(g_evtFRAME) / sizeof(g_evtFRAME[0])) },
    { "DOWNLOAD", g_evtDOWNLOAD, (INT)(sizeof(g_evtDOWNLOAD) / sizeof(g_evtDOWNLOAD[0])) },
    { "RECEIVER", g_evtRECEIVER, (INT)(sizeof(g_evtRECEIVER) / sizeof(g_evtRECEIVER[0])) },
};

static INT WvEvtGroupIndexOf(LPCSTR group)
{
    if (group == NULL) return -1;
    for (INT i = 0; i < (INT)(sizeof(g_evtGroups) / sizeof(g_evtGroups[0])); i++)
        if (_stricmp(g_evtGroups[i].m_szGroup, group) == 0) return i;
    return -1;
}

BOOL WvEvtSubscribe(IUnknown* pTarget, LPCSTR group, LPCSTR evtName, void* pCtx, WvEventFn fn, INT64* pToken)
{
    const INT gi = WvEvtGroupIndexOf(group);
    if (gi < 0 || evtName == NULL || fn == NULL || pToken == NULL) return FALSE;
    for (INT i = 0; i < g_evtGroups[gi].m_nCount; i++)
        if (_stricmp(g_evtGroups[gi].m_pItems[i].m_szName, evtName) == 0)
            return g_evtGroups[gi].m_pItems[i].m_pfnSub(pTarget, WvEvtBind{ pCtx, fn }, pToken);
    return FALSE;
}

BOOL WvEvtUnsubscribe(IUnknown* pTarget, LPCSTR group, LPCSTR evtName, INT64 token)
{
    const INT gi = WvEvtGroupIndexOf(group);
    if (gi < 0 || evtName == NULL) return FALSE;
    for (INT i = 0; i < g_evtGroups[gi].m_nCount; i++)
        if (_stricmp(g_evtGroups[gi].m_pItems[i].m_szName, evtName) == 0)
            return g_evtGroups[gi].m_pItems[i].m_pfnUnsub(pTarget, token);
    return FALSE;
}

void WvEvtNames(LPCSTR group, char* buf, INT cap)
{
    if (buf == NULL || cap <= 0) return; buf[0] = '\0';
    const INT gi = WvEvtGroupIndexOf(group); if (gi < 0) return; INT n = 0;
    for (INT i = 0; i < g_evtGroups[gi].m_nCount; i++)
        n += snprintf(buf + n, (size_t)(cap - n - 1), "%s%s", n ? "\n" : "", g_evtGroups[gi].m_pItems[i].m_szName);
    if (n > cap - 1) n = cap - 1;
}

INT WvEvtGroupCount() { return (INT)(sizeof(g_evtGroups) / sizeof(g_evtGroups[0])); }
LPCSTR WvEvtGroupName(INT i) { return (i >= 0 && i < WvEvtGroupCount()) ? g_evtGroups[i].m_szGroup : NULL; }

static const struct { LPCSTR m_szEvt; LPCSTR m_szIface; } g_evtIface[] = {
    { "AcceleratorKeyPressed", "ICoreWebView2AcceleratorKeyPressedEventArgs" },
    { "GotFocus", "" },
    { "LostFocus", "" },
    { "MoveFocusRequested", "ICoreWebView2MoveFocusRequestedEventArgs" },
    { "RasterizationScaleChanged", "" },
    { "ZoomFactorChanged", "" },
    { "BasicAuthenticationRequested", "ICoreWebView2BasicAuthenticationRequestedEventArgs" },
    { "ClientCertificateRequested", "ICoreWebView2ClientCertificateRequestedEventArgs" },
    { "ContainsFullScreenElementChanged", "" },
    { "ContentLoading", "ICoreWebView2ContentLoadingEventArgs" },
    { "ContextMenuRequested", "ICoreWebView2ContextMenuRequestedEventArgs" },
    { "DedicatedWorkerCreated", "ICoreWebView2DedicatedWorkerCreatedEventArgs" },
    { "DocumentTitleChanged", "" },
    { "DOMContentLoaded", "ICoreWebView2DOMContentLoadedEventArgs" },
    { "DownloadStarting", "ICoreWebView2DownloadStartingEventArgs" },
    { "FaviconChanged", "" },
    { "FrameCreated", "ICoreWebView2FrameCreatedEventArgs" },
    { "FrameNavigationCompleted", "ICoreWebView2NavigationCompletedEventArgs" },
    { "FrameNavigationStarting", "ICoreWebView2NavigationStartingEventArgs" },
    { "HistoryChanged", "" },
    { "IsDefaultDownloadDialogOpenChanged", "" },
    { "IsDocumentPlayingAudioChanged", "" },
    { "IsMutedChanged", "" },
    { "LaunchingExternalUriScheme", "ICoreWebView2LaunchingExternalUriSchemeEventArgs" },
    { "NavigationCompleted", "ICoreWebView2NavigationCompletedEventArgs" },
    { "NavigationStarting", "ICoreWebView2NavigationStartingEventArgs" },
    { "NewWindowRequested", "ICoreWebView2NewWindowRequestedEventArgs" },
    { "NotificationReceived", "ICoreWebView2NotificationReceivedEventArgs" },
    { "PermissionRequested", "ICoreWebView2PermissionRequestedEventArgs" },
    { "ProcessFailed", "ICoreWebView2ProcessFailedEventArgs" },
    { "SaveAsUIShowing", "ICoreWebView2SaveAsUIShowingEventArgs" },
    { "SaveFileSecurityCheckStarting", "ICoreWebView2SaveFileSecurityCheckStartingEventArgs" },
    { "ScreenCaptureStarting", "ICoreWebView2ScreenCaptureStartingEventArgs" },
    { "ScriptDialogOpening", "ICoreWebView2ScriptDialogOpeningEventArgs" },
    { "ServerCertificateErrorDetected", "ICoreWebView2ServerCertificateErrorDetectedEventArgs" },
    { "SourceChanged", "ICoreWebView2SourceChangedEventArgs" },
    { "StatusBarTextChanged", "" },
    { "WebMessageReceived", "ICoreWebView2WebMessageReceivedEventArgs" },
    { "WebResourceRequested", "ICoreWebView2WebResourceRequestedEventArgs" },
    { "WebResourceResponseReceived", "ICoreWebView2WebResourceResponseReceivedEventArgs" },
    { "WindowCloseRequested", "" },
    { "BytesReceivedChanged", "" },
    { "EstimatedEndTimeChanged", "" },
    { "StateChanged", "" },
    { "BrowserProcessExited", "ICoreWebView2BrowserProcessExitedEventArgs" },
    { "NewBrowserVersionAvailable", "" },
    { "ProcessInfosChanged", "" },
    { "ContentLoading", "ICoreWebView2ContentLoadingEventArgs" },
    { "DedicatedWorkerCreated", "ICoreWebView2DedicatedWorkerCreatedEventArgs" },
    { "Destroyed", "" },
    { "DOMContentLoaded", "ICoreWebView2DOMContentLoadedEventArgs" },
    { "FrameCreated", "ICoreWebView2FrameCreatedEventArgs" },
    { "NameChanged", "" },
    { "NavigationCompleted", "ICoreWebView2NavigationCompletedEventArgs" },
    { "NavigationStarting", "ICoreWebView2NavigationStartingEventArgs" },
    { "PermissionRequested", "ICoreWebView2PermissionRequestedEventArgs2" },
    { "ScreenCaptureStarting", "ICoreWebView2ScreenCaptureStartingEventArgs" },
    { "WebMessageReceived", "ICoreWebView2WebMessageReceivedEventArgs" },
    { "Deleted", "" },
    { "DevToolsProtocolEventReceived", "ICoreWebView2DevToolsProtocolEventReceivedEventArgs" },
};
LPCSTR WvEvtArgsIface(LPCSTR evtName)
{
    if (evtName == NULL) return NULL;
    for (INT i = 0; i < (INT)(sizeof(g_evtIface) / sizeof(g_evtIface[0])); i++)
        if (_stricmp(g_evtIface[i].m_szEvt, evtName) == 0) return (g_evtIface[i].m_szIface[0] != '\0') ? g_evtIface[i].m_szIface : NULL;
    return NULL;
}

}  // namespace wv
