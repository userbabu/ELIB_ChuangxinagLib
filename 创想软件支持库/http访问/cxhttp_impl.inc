// 由 http访问/gen_http_table.mjs 从 cxhttp_api.json 生成，勿手改。
void CurlEasyConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    pRetData->m_int = (INT)HNew(0);
}

void CurlEasyCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    HCopyObj(pArgInf, 0);
}

void CurlEasyDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    if (pArgInf[0].m_pCompoundData != NULL)
    {
        HFreeObj((CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData));
        *(PINT)pArgInf[0].m_pCompoundData = 0;
    }
}

void CurlEasy_Create(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Create, 0);
}

void CurlEasy_Destroy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Destroy, 0);
}

void CurlEasy_GetHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetHandle, 0);
}

void CurlEasy_SetHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetHandle, 0);
}

void CurlEasy_DupHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_DupHandle, 0);
}

void CurlEasy_SetoptInt(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetoptInt, 0);
}

void CurlEasy_SetoptLong(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetoptLong, 0);
}

void CurlEasy_SetoptText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetoptText, 0);
}

void CurlEasy_SetoptBin(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetoptBin, 0);
}

void CurlEasy_Perform(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Perform, 0);
}

void CurlEasy_GetinfoInt(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoInt, 0);
}

void CurlEasy_GetinfoDouble(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoDouble, 0);
}

void CurlEasy_GetinfoText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoText, 0);
}

void CurlEasy_GetinfoTextAry(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoTextAry, 0);
}

void CurlEasy_GetinfoCert(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoCert, 0);
}

void CurlEasy_Reset(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Reset, 0);
}

void CurlEasy_Escape(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Escape, 0);
}

void CurlEasy_Unescape(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Unescape, 0);
}

void CurlHttpConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    pRetData->m_int = (INT)HNew(1);
}

void CurlHttpCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    HCopyObj(pArgInf, 1);
}

void CurlHttpDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    if (pArgInf[0].m_pCompoundData != NULL)
    {
        HFreeObj((CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData));
        *(PINT)pArgInf[0].m_pCompoundData = 0;
    }
}

void CurlHttp_Create(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Create, 1);
}

void CurlHttp_Destroy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Destroy, 1);
}

void CurlHttp_GetHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetHandle, 1);
}

void CurlHttp_SetHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetHandle, 1);
}

void CurlHttp_DupHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_DupHandle, 1);
}

void CurlHttp_SetoptInt(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetoptInt, 1);
}

void CurlHttp_SetoptLong(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetoptLong, 1);
}

void CurlHttp_SetoptText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetoptText, 1);
}

void CurlHttp_SetoptBin(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetoptBin, 1);
}

void CurlHttp_Perform(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Perform, 1);
}

void CurlHttp_GetinfoInt(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoInt, 1);
}

void CurlHttp_GetinfoDouble(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoDouble, 1);
}

void CurlHttp_GetinfoText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoText, 1);
}

void CurlHttp_GetinfoTextAry(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoTextAry, 1);
}

void CurlHttp_GetinfoCert(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoCert, 1);
}

void CurlHttp_Reset(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Reset, 1);
}

void CurlHttp_Escape(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Escape, 1);
}

void CurlHttp_Unescape(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Unescape, 1);
}

void CurlHttp_SetXferinfo(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetXferinfo, 1);
}

void CurlHttp_SetXferinfoAddr(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetXferinfoAddr, 1);
}

void CurlHttp_SetUrl(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetUrl, 1);
}

void CurlHttp_SetProxy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetProxy, 1);
}

void CurlHttp_Impersonate(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_Impersonate, 1);
}

void CurlHttp_SetUserPwd(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetUserPwd, 1);
}

void CurlHttp_SetCookieFile(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetCookieFile, 1);
}

void CurlHttp_SetCookieList(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetCookieList, 1);
}

void CurlHttp_SetCookieText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetCookieText, 1);
}

void CurlHttp_SetTimeoutMs(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetTimeoutMs, 1);
}

void CurlHttp_SetTimeout(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetTimeout, 1);
}

void CurlHttp_SetPost(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetPost, 1);
}

void CurlHttp_SetPostBin(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetPostBin, 1);
}

void CurlHttp_SetPostText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetPostText, 1);
}

void CurlHttp_SetFollow(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetFollow, 1);
}

void CurlHttp_SetKeepAlive(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetKeepAlive, 1);
}

void CurlHttp_SetEncoding(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetEncoding, 1);
}

void CurlHttp_SetHeaders(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetHeaders, 1);
}

void CurlHttp_SetBufSize(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetBufSize, 1);
}

void CurlHttp_SetReferer(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetReferer, 1);
}

void CurlHttp_SetHttpVersion(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetHttpVersion, 1);
}

void CurlHttp_SetUserAgent(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetUserAgent, 1);
}

void CurlHttp_SetCaInfo(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetCaInfo, 1);
}

void CurlHttp_SetVerify(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetVerify, 1);
}

void CurlHttp_RespBody(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_RespBody, 1);
}

void CurlHttp_RespHead(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_RespHead, 1);
}

void CurlHttp_RespHeadBin(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_RespHeadBin, 1);
}

void CurlHttp_RespCode(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_RespCode, 1);
}

void CurlHttp_RespTime(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_RespTime, 1);
}

void CurlHttp_RespUrl(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_RespUrl, 1);
}

void CurlHttp_RespRedirectUrl(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_RespRedirectUrl, 1);
}

void CurlHttp_RespRemoteIp(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_RespRemoteIp, 1);
}

void CurlHttp_RespRemotePort(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_RespRemotePort, 1);
}

void CurlHttp_RespLocalIp(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_RespLocalIp, 1);
}

void CurlHttp_RespLocalPort(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_RespLocalPort, 1);
}

void CurlHttp_RespCookies(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_RespCookies, 1);
}

void CurlSmtpConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    pRetData->m_int = (INT)HNew(2);
}

void CurlSmtpCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    HCopyObj(pArgInf, 2);
}

void CurlSmtpDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    if (pArgInf[0].m_pCompoundData != NULL)
    {
        HFreeObj((CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData));
        *(PINT)pArgInf[0].m_pCompoundData = 0;
    }
}

void CurlSmtp_Create(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Create, 2);
}

void CurlSmtp_Destroy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Destroy, 2);
}

void CurlSmtp_GetHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetHandle, 2);
}

void CurlSmtp_SetHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetHandle, 2);
}

void CurlSmtp_DupHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_DupHandle, 2);
}

void CurlSmtp_SetoptInt(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetoptInt, 2);
}

void CurlSmtp_SetoptLong(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetoptLong, 2);
}

void CurlSmtp_SetoptText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetoptText, 2);
}

void CurlSmtp_SetoptBin(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_SetoptBin, 2);
}

void CurlSmtp_Perform(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Perform, 2);
}

void CurlSmtp_GetinfoInt(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoInt, 2);
}

void CurlSmtp_GetinfoDouble(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoDouble, 2);
}

void CurlSmtp_GetinfoText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoText, 2);
}

void CurlSmtp_GetinfoTextAry(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoTextAry, 2);
}

void CurlSmtp_GetinfoCert(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_GetinfoCert, 2);
}

void CurlSmtp_Reset(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Reset, 2);
}

void CurlSmtp_Escape(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Escape, 2);
}

void CurlSmtp_Unescape(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlEasy_Unescape, 2);
}

void CurlSmtp_SetXferinfo(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetXferinfo, 2);
}

void CurlSmtp_SetXferinfoAddr(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetXferinfoAddr, 2);
}

void CurlSmtp_SetServer(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlSmtp_SetServer, 2);
}

void CurlSmtp_SetCaInfo(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetCaInfo, 2);
}

void CurlSmtp_SetVerify(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlHttp_SetVerify, 2);
}

void CurlSmtp_SetMail(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlSmtp_SetMail, 2);
}

void CurlMultiConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    pRetData->m_int = (INT)HNew(3);
}

void CurlMultiCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    HCopyObj(pArgInf, 3);
}

void CurlMultiDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    if (pArgInf[0].m_pCompoundData != NULL)
    {
        HFreeObj((CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData));
        *(PINT)pArgInf[0].m_pCompoundData = 0;
    }
}

void CurlMulti_Create(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlMulti_Create, 3);
}

void CurlMulti_Destroy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlMulti_Destroy, 3);
}

void CurlMulti_GetHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlMulti_GetHandle, 3);
}

void CurlMulti_SetHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlMulti_SetHandle, 3);
}

void CurlMulti_AddHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlMulti_AddHandle, 3);
}

void CurlMulti_RemoveHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlMulti_RemoveHandle, 3);
}

void CurlMulti_Perform(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlMulti_Perform, 3);
}

void CurlMulti_Wait(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlMulti_Wait, 3);
}

void CurlMulti_InfoRead(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlMulti_InfoRead, 3);
}

void CurlMulti_Setopt(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_CurlMulti_Setopt, 3);
}

void UrlParamConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    pRetData->m_int = (INT)HNew(4);
}

void UrlParamCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    HCopyObj(pArgInf, 4);
}

void UrlParamDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    if (pArgInf[0].m_pCompoundData != NULL)
    {
        HFreeObj((CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData));
        *(PINT)pArgInf[0].m_pCompoundData = 0;
    }
}

void UrlParam_InitUrl(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_UrlParam_InitUrl, 4);
}

void UrlParam_AddOrModify(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_UrlParam_AddOrModify, 4);
}

void UrlParam_GetUrl(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_UrlParam_GetUrl, 4);
}

void UrlParam_ModifyValue(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_UrlParam_ModifyValue, 4);
}

void UrlParam_ModifyName(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_UrlParam_ModifyName, 4);
}

void UrlParam_RemoveParam(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_UrlParam_RemoveParam, 4);
}

void UrlParam_AddParam(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_UrlParam_AddParam, 4);
}

void UrlParam_FindParam(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_UrlParam_FindParam, 4);
}

void AccessCurlConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    pRetData->m_int = (INT)HNew(5);
}

void AccessCurlCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    HCopyObj(pArgInf, 5);
}

void AccessCurlDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    if (pArgInf[0].m_pCompoundData != NULL)
    {
        HFreeObj((CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData));
        *(PINT)pArgInf[0].m_pCompoundData = 0;
    }
}

void AccessCurl_SetRecvChunk(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessCurl_SetRecvChunk, 5);
}

void AccessCurl_SetCookie(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessCurl_SetCookie, 5);
}

void AccessCurl_SetDns(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessCurl_SetDns, 5);
}

void AccessCurl_GetSubmitCookie(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessCurl_GetSubmitCookie, 5);
}

void AccessCurl_GetRetCookie(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessCurl_GetRetCookie, 5);
}

void AccessCurl_SetCurlHandle(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessCurl_SetCurlHandle, 5);
}

void AccessCurl_SetShareDns(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessCurl_SetShareDns, 5);
}

void AccessCurl_SetOption(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessCurl_SetOption, 5);
}

void AccessCurl_SetOptionInt(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessCurl_SetOptionInt, 5);
}

void AccessCurl_Impersonate(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessCurl_Impersonate, 5);
}

void AccessCurl_AccessPageCurl(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessCurl_AccessPageCurl, 5);
}

void AccessWinHttpConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    pRetData->m_int = (INT)HNew(6);
}

void AccessWinHttpCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    HCopyObj(pArgInf, 6);
}

void AccessWinHttpDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    if (pArgInf[0].m_pCompoundData != NULL)
    {
        HFreeObj((CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData));
        *(PINT)pArgInf[0].m_pCompoundData = 0;
    }
}

void AccessWinHttp_GetLastHeader(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessWinHttp_GetLastHeader, 6);
}

void AccessWinHttp_SetRecvChunk(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessWinHttp_SetRecvChunk, 6);
}

void AccessWinHttp_SetCookie(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessWinHttp_SetCookie, 6);
}

void AccessWinHttp_GetSubmitCookie(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessWinHttp_GetSubmitCookie, 6);
}

void AccessWinHttp_GetRetCookie(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessWinHttp_GetRetCookie, 6);
}

void AccessWinHttp_AccessPageWinHttp(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessWinHttp_AccessPageWinHttp, 6);
}

void AccessWinInetConstruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    pRetData->m_int = (INT)HNew(7);
}

void AccessWinInetCopy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    HCopyObj(pArgInf, 7);
}

void AccessWinInetDestruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    if (pArgInf[0].m_pCompoundData != NULL)
    {
        HFreeObj((CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData));
        *(PINT)pArgInf[0].m_pCompoundData = 0;
    }
}

void AccessWinInet_SetRecvChunk(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessWinInet_SetRecvChunk, 7);
}

void AccessWinInet_SetCookie(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessWinInet_SetCookie, 7);
}

void AccessWinInet_GetSubmitCookie(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessWinInet_GetSubmitCookie, 7);
}

void AccessWinInet_GetRetCookie(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessWinInet_GetRetCookie, 7);
}

void AccessWinInet_AccessPageWinInet(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_AccessWinInet_AccessPageWinInet, 7);
}

void G_WebAccessCurl(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_WebAccessCurl);
}

void G_WebAccessWinHttp(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_WebAccessWinHttp);
}

void G_WebAccessWinInet(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_WebAccessWinInet);
}

void G_WebCookieMerge(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_WebCookieMerge);
}

void G_WebCookieFromHead(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_WebCookieFromHead);
}

void G_WebPickHeader(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_WebPickHeader);
}

void G_WebBuildHeaders(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_WebBuildHeaders);
}

void G_WebSystemProxy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_WebSystemProxy);
}

void G_JudgeHeadValid(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_JudgeHeadValid);
}

void G_GetHeadCode(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_GetHeadCode);
}

void G_WebStatusCode(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_WebStatusCode);
}

void G_WebBuildForm(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_WebBuildForm);
}

void G_WebContentType(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_WebContentType);
}

void G_AnalyseHeadCookies(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_AnalyseHeadCookies);
}

void G_GetMiddleText(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_GetMiddleText);
}

void G_CookieMerge(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_CookieMerge);
}

void G_WebCookieInner(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_WebCookieInner);
}

void G_WebCookieOne(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_WebCookieOne);
}

void G_TextCountOccur(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_TextCountOccur);
}

void G_CompressAmazonPage(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_CompressAmazonPage);
}

void G_CurlPreload(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_CurlPreload);
}

void G_CurlShutdown(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_G_CurlShutdown);
}
