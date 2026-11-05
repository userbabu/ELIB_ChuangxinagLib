// 临时占位派发器：等 cxhttp_access_win.inc 的正式实现到位后，把 lib.cpp 里的 include 换回去。
// 这里只把 WinHTTP / WinInet 两条路线的操作码接下来并给安全空结果，用于先把表与 curl 路线验证跑通。
#ifndef CXHTTP_ACCESS_WIN_INC
#define CXHTTP_ACCESS_WIN_INC

static bool HAccessWinInvoke(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf,
    int nOp, int nKind, CxHttpObj** ppObj)
{
    UNREFERENCED_PARAMETER(nKind);
    UNREFERENCED_PARAMETER(ppObj);
    UNREFERENCED_PARAMETER(nArgCount);
    UNREFERENCED_PARAMETER(pArgInf);
    switch (nOp)
    {
    case HOP_AccessWinHttp_GetLastHeader:
    case HOP_AccessWinHttp_GetSubmitCookie:
    case HOP_AccessWinHttp_GetRetCookie:
    case HOP_AccessWinInet_GetSubmitCookie:
    case HOP_AccessWinInet_GetRetCookie:
        SetRetText(pRetData, "");
        return true;
    case HOP_AccessWinHttp_SetRecvChunk:
    case HOP_AccessWinHttp_SetCookie:
    case HOP_AccessWinInet_SetRecvChunk:
    case HOP_AccessWinInet_SetCookie:
        return true;
    case HOP_AccessWinHttp_AccessPageWinHttp:
    case HOP_AccessWinInet_AccessPageWinInet:
    case HOP_G_WebAccessWinHttp:
    case HOP_G_WebAccessWinInet:
        pRetData->m_pBin = CloneBinData(NULL, 0);
        return true;
    default: break;
    }
    return false;
}

#endif  // CXHTTP_ACCESS_WIN_INC
