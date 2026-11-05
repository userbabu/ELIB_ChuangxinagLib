// HTTP访问 族：原模块「HTTP访问_Curl访问类」与全局命令「网页_访问_Curl」的 C++ 实现。
// 复用本库已加载的 curl-impersonate（g_curl / LoadCurlApi），与 cxhttp_core.inc 的 2.0 引擎互不打扰。
// 参数名/顺序/类型/可空/返回类型逐字照原源码；语义以 http访问\规格_访问类.md 第二节为准。
// 修掉的坑（行为只在原实现出错时不同）：句柄每次访问后必定回收；空缓冲不再解引用。
#ifndef CXHTTP_ACCESS_CURL_INC
#define CXHTTP_ACCESS_CURL_INC

// 访问类里「设置CURL句柄」存下来的句柄快照：原模块用 CURL(整数型) 成员做 ≤0 守卫，这里放 hSession。
#define HACC_SNAP(p)  ((p)->hSession)

// 把文本拷进缓冲
static void HXCopyTo(CxBuf* pOut, LPCSTR sz)
{
    if (sz != NULL && pOut != NULL) BufAppend(pOut, sz, lstrlenA(sz));
}

// 字节集实参的数据与长度（块 = [保留=1][长度][数据]，数据在 8 字节头之后）
static LPCSTR HBinData(const MDATA_INF& a)
{
    if (a.m_dtDataType != SDT_BIN || a.m_pBin == NULL) return NULL;
    return (LPCSTR)((LPBYTE)a.m_pBin + sizeof(INT) * 2);
}
static int HBinLen(const MDATA_INF& a)
{
    if (a.m_dtDataType != SDT_BIN || a.m_pBin == NULL) return 0;
    const int n = *(INT*)((LPBYTE)a.m_pBin + sizeof(INT));
    return (n < 0 || n > 0x08000000) ? 0 : n;        // 块头被写坏时不越界
}

// ------------------------------------------------------------ 一次访问的全部过程
// p：访问类对象（Cookie 状态在这里）；出参写进 pOutHead/pOutErr/pOutBody。
static void HAccessCurlRun(CxHttpObj* p,
    LPCSTR szUrl, LPCSTR szMethod, INT nCbFn, const void* pPost, int nPostLen,
    int nTimeout, LPCSTR szExtraHead, BOOL bNoRedirect,
    LPCSTR szUser, LPCSTR szPass, LPCSTR szProxy, LPCSTR szProxyUser, LPCSTR szProxyPass,
    BOOL bGzip, BOOL bRetryNoProxy, LPCSTR szTsl, INT nCbUser, int nRetry,
    CxBuf* pOutHead, CxBuf* pOutErr, CxBuf* pOutBody)
{
    if (HStopRequested(p)) { HXCopyTo(pOutErr, "Request cancelled (1223)"); return; }
    fn_easy_setopt_t so = g_curl.EasySetopt;

    // 默认值规整（原模块就地改写实参，口径逐条照抄）
    if (nTimeout <= 0) nTimeout = 60;
    LPCSTR szVerb = (szMethod == NULL || szMethod[0] == '\0') ? "GET" : szMethod;
    CxBuf m;
    BufInit(&m, 16);
    for (int i = 0; szVerb[i] != '\0' && m.pData != NULL; i++)
    {
        char c = szVerb[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        BufAppend(&m, &c, 1);
    }
    if (m.pData == NULL || m.nLen == 0) { BufReset(&m); BufAppend(&m, "GET", 3); }

    // 每次访问都新建 easy 句柄（原模块：CURL类.创建()，结束时 销毁()）
    CX_CURL h = g_curl.EasyInit();
    if (h == NULL)
    {
        p->nLastError = CURLE_OUT_OF_MEMORY;
        BufFree(&m);
        if (pOutErr->pData != NULL) BufReset(pOutErr);
        return;
    }
    p->h = h;                     // HBind 要用对象字段挂回调数据
    HBind(p);                     // 响应正文/头收进 p->body / p->head
    so(h, CURLOPT_URL, szUrl);    // 网址按 UTF-8 交出去（原模块就是 UTF8）
    if (nCbFn != 0)
    {
        so(h, CURLOPT_XFERINFOFUNCTION, (void*)nCbFn);
        so(h, CURLOPT_XFERINFODATA, (void*)nCbUser);
        so(h, CURLOPT_NOPROGRESS, 0L);
    }
    if (pPost != NULL && nPostLen > 0 && lstrcmpA(m.pData, "POST") == 0)
    {
        BufReset(&p->post);
        BufAppend(&p->post, (LPCSTR)pPost, nPostLen);
        so(h, CURLOPT_POSTFIELDS, p->post.pData);
        so(h, CURLOPT_POSTFIELDSIZE, nPostLen);
    }
    // 系统默认(IE)COOKIE：只在「开了默认取用 + 没自定义 + 当前没Cookie」时取
    if ((p->nFlags & CXH_F_SYS_COOKIE) && p->szCookie != NULL && p->szCookie[0] == '\0'
        && !(p->nFlags & CXH_F_CUSTOM_COOKIE))
    {
        CxBuf host, ck;
        BufInit(&host, 64);
        BufInit(&ck, 256);
        UxHostOf(&host, szUrl);
        UxCookieFromUrl(&ck, host.pData, NULL, strstr(szUrl, "https://") != NULL);
        HSetStr(&p->szCookie, ck.nLen == 0 ? "" : ck.pData);
        BufFree(&ck); BufFree(&host);
    }
    // 指纹模拟：留空按 chrome107；都不带默认头（后面自己拼请求头）
    fn_easy_setopt_t pf = so;
    {
        void* pImp = (void*)GetProcAddress(g_curl.hModule, "curl_easy_impersonate");
        LPCSTR szTarget = (szTsl == NULL || szTsl[0] == '\0') ? "chrome107" : szTsl;
        if (pImp != NULL) ((int (*)(void*, const char*, BOOL))pImp)(h, szTarget, FALSE);
    }
    pf(h, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    // 协议头：交给 网页_构建协议头 补全，再切行建 curl_slist
    CxBuf head;
    BufInit(&head, 512);
    UxBuildHeaders(&head, szExtraHead, szUrl, HGetStr(p->szCookie),
        lstrcmpA(m.pData, "POST") == 0, bGzip, nPostLen);
    // 逐行 append 成 slist
    if (p->pSlist != NULL && g_curl.SlistFreeAll != NULL)
    {
        g_curl.SlistFreeAll((CxSlist*)p->pSlist);
        p->pSlist = NULL;
    }
    curl_slist* pList = NULL;
    if (head.pData != NULL && g_curl.SlistAppend != NULL)
    {
        int nStart = 0;
        for (int i = 0; i <= head.nLen; i++)
        {
            if (i != head.nLen && head.pData[i] != '\n') continue;
            int nLen = i - nStart;
            while (nLen > 0 && (head.pData[nStart + nLen - 1] == '\r')) nLen--;
            if (nLen > 0)
            {
                CxBuf one;
                BufInit(&one, nLen + 1);
                BufAppend(&one, head.pData + nStart, nLen);
                if (one.pData != NULL)
                {
                    curl_slist* pNext = (curl_slist*)g_curl.SlistAppend((CxSlist*)pList, one.pData);
                    if (pNext != NULL) pList = pNext;
                }
                BufFree(&one);
            }
            nStart = i + 1;
        }
    }
    if (pList != NULL) { p->pSlist = pList; pf(h, CURLOPT_HTTPHEADER, pList); }
    BufFree(&head);
    pf(h, CURLOPT_BUFFERSIZE, p->nChunk > 0 ? p->nChunk : 512);
    // 代理：'*' 取系统代理；一律加 http:// 前缀（原模块就是这样）
    CxBuf proxy;
    BufInit(&proxy, 128);
    if (szProxy != NULL && szProxy[0] == '*' && szProxy[1] == '\0') UxSystemProxy(&proxy);
    else if (szProxy != NULL) HXCopyTo(&proxy, szProxy);
    CxBuf proxyUrl;
    BufInit(&proxyUrl, proxy.nLen + 8);
    if (proxy.pData != NULL && proxy.nLen > 0)
    {
        BufAppend(&proxyUrl, "http://", 7);
        BufAppend(&proxyUrl, proxy.pData, proxy.nLen);
        pf(h, CURLOPT_PROXY, proxyUrl.pData);
        if (szProxyUser != NULL && szProxyUser[0] != '\0')
        {
            CxBuf pu;
            BufInit(&pu, lstrlenA(szProxyUser) + lstrlenA(szProxyPass == NULL ? "" : szProxyPass) + 2);
            BufAppend(&pu, szProxyUser, lstrlenA(szProxyUser));
            BufAppend(&pu, ":", 1);
            BufAppend(&pu, szProxyPass == NULL ? "" : szProxyPass, lstrlenA(szProxyPass == NULL ? "" : szProxyPass));
            pf(h, CURLOPT_PROXYUSERPWD, pu.pData);
            BufFree(&pu);
        }
    }
    else pf(h, CURLOPT_PROXY, "");
    BufFree(&proxyUrl); BufFree(&proxy);
    // 站点登录
    if (szUser != NULL && szUser[0] != '\0')
    {
        CxBuf up;
        LPCSTR szP = szPass == NULL ? "" : szPass;
        BufInit(&up, lstrlenA(szUser) + lstrlenA(szP) + 2);
        BufAppend(&up, szUser, lstrlenA(szUser));
        BufAppend(&up, ":", 1);
        BufAppend(&up, szP, lstrlenA(szP));
        pf(h, CURLOPT_USERPWD, up.pData);
        BufFree(&up);
    }
    else pf(h, CURLOPT_USERPWD, "");
    pf(h, CURLOPT_SSL_VERIFYPEER, 0L);
    pf(h, CURLOPT_SSL_VERIFYHOST, 0L);
    pf(h, CURLOPT_FOLLOWLOCATION, bNoRedirect ? 0L : 1L);
    pf(h, CURLOPT_TCP_KEEPALIVE, 1L);
    pf(h, CURLOPT_TCP_KEEPIDLE, (long)nTimeout);
    pf(h, CURLOPT_TIMEOUT, (long)nTimeout);
    pf(h, CURLOPT_CONNECTTIMEOUT, (long)nTimeout);
    // 解压交给 curl 自己做：原模块用外部 gzip.dll，库里没有那套依赖，
    // 用 CURLOPT_ACCEPT_ENCODING 效果一致（返回正文都是解压后的），也不再有 DLL 缺失的坑。
    if (bGzip) pf(h, CURLOPT_ACCEPT_ENCODING, "gzip, deflate, br");
    else pf(h, CURLOPT_ACCEPT_ENCODING, NULL);
    pf(h, CURLOPT_NOSIGNAL, 1L);
    if (lstrcmpA(m.pData, "GET") != 0) pf(h, CURLOPT_CUSTOMREQUEST, m.pData);

    const int nRet = HCancelableCurlPerform(p);
    if (!HStopRequested(p)) p->nLastError = nRet;
    if (nRet != 0)
    {
        UxCurlErrorText(pOutErr, nRet);
        // 代理连不上(7)/SSL无法连接(35)/超时(28) 且开了自动处理：最多 3 次，去掉代理重发
        if (!HStopRequested(p) && bRetryNoProxy && nRetry < 3 && (nRet == 7 || nRet == 35 || nRet == 28)
            && !(nRetry == 0 && (szProxy == NULL || szProxy[0] == '\0')))
        {
            CxBuf tmpHead, tmpErr;
            BufInit(&tmpHead, 512);
            BufInit(&tmpErr, 128);
            g_curl.EasyCleanup(h);
            if (p->pSlist) g_curl.SlistFreeAll((CxSlist*)p->pSlist);
            p->pSlist = NULL; p->h = NULL;
            HAccessCurlRun(p, szUrl, szMethod, nCbFn, pPost, nPostLen, nTimeout, szExtraHead,
                bNoRedirect, szUser, szPass, "", "", "", bGzip, TRUE, szTsl, nCbUser, nRetry + 1,
                &tmpHead, &tmpErr, pOutBody);
            BufReset(pOutHead);
            BufAppend(pOutHead, tmpHead.pData, tmpHead.nLen);
            BufReset(pOutErr);
            BufAppend(pOutErr, tmpErr.pData, tmpErr.nLen);
            BufFree(&tmpHead); BufFree(&tmpErr);
            BufFree(&m);
            p->h = NULL;
            p->pSlist = NULL;
            return;
        }
    }
    // 响应头 + Cookie 合并（无论成败都做，和原模块一致）
    BufReset(pOutHead);
    BufAppend(pOutHead, p->head.pData, p->head.nLen);
    HSetStr(&p->szLastCookie, p->szCookie);
    CxBuf retCk;
    BufInit(&retCk, 128);
    UxCookieFromHead(&retCk, p->head.pData == NULL ? "" : p->head.pData);
    if (p->szCookie != NULL && p->szCookie[0] != '\0' && (p->nFlags & CXH_F_AUTO_COOKIE))
    {
        CxBuf merged;
        BufInit(&merged, retCk.nLen + lstrlenA(p->szCookie) + 2);
        UxCookieMerge(&merged, p->szCookie, retCk.nLen == 0 ? "" : retCk.pData);
        BufReset(&retCk);
        BufAppend(&retCk, merged.pData, merged.nLen);
        BufFree(&merged);
    }
    HSetStr(&p->szRetCookie, retCk.nLen == 0 ? "" : retCk.pData);
    BufFree(&retCk);

    BufReset(pOutBody);
    BufAppend(pOutBody, p->body.pData, p->body.nLen);
    g_curl.EasyCleanup(h);
    p->h = NULL;
    if (p->pSlist) g_curl.SlistFreeAll((CxSlist*)p->pSlist);
    p->pSlist = NULL;
    BufFree(&m);
}

// ------------------------------------------------------------ 公共入口：把一个对象跑成字节集
static void HAccessCurlInvokeSelf(PMDATA_INF pRetData, CxHttpObj* p,
    LPCSTR szUrl, LPCSTR szMethod, INT nCbFn, const void* pPost, int nPostLen, int nTimeout,
    LPCSTR szExtraHead, LPCSTR szSubmitCookie, BOOL bNoRedirect, LPCSTR szUser, LPCSTR szPass,
    LPCSTR szProxy, LPCSTR szProxyUser, LPCSTR szProxyPass, BOOL bGzip,
    BOOL bRetryNoProxy, LPCSTR szTsl, INT nCbUser, int nRetry,
    BOOL bWriteHeadRef, PMDATA_INF pHeadRef, BOOL bWriteErrRef, PMDATA_INF pErrRef)
{
    if (p == NULL || !LoadCurlApi())
    {
        pRetData->m_pBin = CloneBinData(NULL, 0);
        return;
    }
    p->nLastError = 0;
    BufReset(&p->body); BufReset(&p->head);
    if (szSubmitCookie != NULL) HSetStr(&p->szCookie, szSubmitCookie);
    CxBuf outHead, outErr, body;
    BufInit(&outHead, 512);
    BufInit(&outErr, 128);
    BufInit(&body, 4096);
    HAccessCurlRun(p, szUrl, szMethod, nCbFn, pPost, nPostLen, nTimeout, szExtraHead, bNoRedirect,
        szUser, szPass, szProxy, szProxyUser, szProxyPass, bGzip, bRetryNoProxy, szTsl,
        nCbUser, nRetry, &outHead, &outErr, &body);
    if (HStopRequested(p))
    {
        BufReset(&body); BufReset(&outHead); BufReset(&outErr);
        HXCopyTo(&outErr, "Request cancelled (1223)");
    }
    if (bWriteHeadRef && pHeadRef != NULL) HSetRefText(pHeadRef, outHead.nLen == 0 ? "" : outHead.pData);
    if (bWriteErrRef && pErrRef != NULL) HSetRefText(pErrRef, outErr.nLen == 0 ? "" : outErr.pData);
    // 魔术字节判定 gzip：curl 已解过的正文不会再带 1F 8B 08
    if (body.nLen > 3 && (BYTE)body.pData[0] == 31 && (BYTE)body.pData[1] == 139)
    {
        int nOut = 0;
        LPBYTE pRaw = UxGzipInflate(body.pData, body.nLen, &nOut);
        if (pRaw != NULL && nOut > 0)
        {
            pRetData->m_pBin = CloneBinData((LPCSTR)pRaw, nOut);
            LocalFree(pRaw);
            BufFree(&body); BufFree(&outHead); BufFree(&outErr);
            return;
        }
    }
    pRetData->m_pBin = CloneBinData(body.pData, body.nLen);
    BufFree(&body); BufFree(&outHead); BufFree(&outErr);
}

// ------------------------------------------------------------ 成员命令派发
static bool HAccessCurlInvoke(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf,
    int nOp, int nKind, CxHttpObj** ppObj)
{
    if (nKind != CXH_KIND_ACC_CURL && nOp != HOP_G_WebAccessCurl) return false;
    CxHttpObj* p = *ppObj;
    if (nOp != HOP_G_WebAccessCurl && p == NULL)
    {
        p = HNew(CXH_KIND_ACC_CURL);
        if (p == NULL) { ReportError("创建 HTTP访问_Curl访问类 对象失败：内存不足"); return true; }
        *(PINT)pArgInf[0].m_pCompoundData = (INT)p;
        *ppObj = p;
    }

    switch (nOp)
    {
    case HOP_AccessCurl_SetRecvChunk:
    {
        int n = pArgInf[1].m_int;
        if (n <= 0) n = 512;                       // #HTTP配置_默认每次接收数据长度
        p->nChunk = n;
        return true;
    }
    case HOP_AccessCurl_SetCookie:
    {
        // 优先级：用默认COOKIE 为真时忽略 COOKIE 参数；否则视为自定义（空串也算自定义）
        const BOOL bSysDefault = nArgCount > 2 ? (pArgInf[2].m_bool == BL_TRUE) : FALSE;
        const BOOL bAutoUpdate = nArgCount > 3 ? (pArgInf[3].m_bool == BL_TRUE) : TRUE;
        p->nFlags = (p->nFlags & ~CXH_F_AUTO_COOKIE) | (bAutoUpdate ? CXH_F_AUTO_COOKIE : 0);
        p->nFlags = (p->nFlags & ~(CXH_F_SYS_COOKIE | CXH_F_CUSTOM_COOKIE))
            | (bSysDefault ? CXH_F_SYS_COOKIE : CXH_F_CUSTOM_COOKIE);
        if (!bSysDefault) HSetStr(&p->szCookie, HTextAt(pArgInf, nArgCount, 1));
        return true;
    }
    case HOP_AccessCurl_SetDns:
        // 原模块只赋值，没有下游消费者（CURLOPT_DNS_SERVERS 一直没设上）；行为保持一致
        return true;
    case HOP_AccessCurl_GetSubmitCookie:
        SetRetText(pRetData, HGetStr(p->szLastCookie));
        return true;
    case HOP_AccessCurl_GetRetCookie:
        SetRetText(pRetData, HGetStr(p->szRetCookie));
        return true;
    case HOP_AccessCurl_SetCurlHandle:
        HACC_SNAP(p) = (void*)(INT_PTR)pArgInf[1].m_int;
        return true;
    case HOP_AccessCurl_SetShareDns:
        return true;                                        // 原模块里是空实现
    case HOP_AccessCurl_SetOption:
        pRetData->m_int = 0;                                // 原模块主体被注释，恒返回 0
        return true;
    case HOP_AccessCurl_SetOptionInt:
    {
        if (p->h == NULL) { pRetData->m_int = 0; return true; }
        pRetData->m_int = HSetopt(p->h, pArgInf[1].m_int, pArgInf[2]);
        return true;
    }
    case HOP_AccessCurl_Impersonate:
    {
        void* pImp = (void*)GetProcAddress(g_curl.hModule, "curl_easy_impersonate");
        if (pImp == NULL || p->h == NULL) { pRetData->m_int = CURLE_UNKNOWN_OPTION; return true; }
        const BOOL bDefault = nArgCount > 2 ? (pArgInf[2].m_bool == BL_TRUE) : FALSE;
        pRetData->m_int = ((int (*)(void*, const char*, BOOL))pImp)(p->h, HTextAt(pArgInf, nArgCount, 1), bDefault);
        return true;
    }
    case HOP_AccessCurl_AccessPageCurl:
    {
        // (完整网址, 访问方式, 回调子程序, 提交信息, 超时, 附加协议头, 返回头, 禁止重定向,
        //  网站登录用户名, 网站登录密码, 代理地址, 代理用户名, 代理密码, 是否采用Gzip, 失败日志,
        //  代理连接失败自动处理, 模拟浏览器TSL, 回调用户参数, 回调次数)
        const void* pPost = nArgCount > 4 ? (const void*)HBinData(pArgInf[4]) : NULL;
        const int nPostLen = nArgCount > 4 ? HBinLen(pArgInf[4]) : 0;
        HAccessCurlInvokeSelf(pRetData, p,
            HTextAt(pArgInf, nArgCount, 1), HTextAt(pArgInf, nArgCount, 2),
            nArgCount > 3 ? pArgInf[3].m_int : 0,
            pPost, nPostLen,
            nArgCount > 4 ? pArgInf[5].m_int : 0,
            HTextAt(pArgInf, nArgCount, 6), NULL,
            nArgCount > 8 ? (pArgInf[8].m_bool == BL_TRUE) : FALSE,
            HTextAt(pArgInf, nArgCount, 9), HTextAt(pArgInf, nArgCount, 10),
            HTextAt(pArgInf, nArgCount, 11), HTextAt(pArgInf, nArgCount, 12), HTextAt(pArgInf, nArgCount, 13),
            nArgCount > 14 ? (pArgInf[14].m_bool == BL_TRUE) : TRUE,
            nArgCount > 16 ? (pArgInf[16].m_bool == BL_TRUE) : FALSE,
            HTextAt(pArgInf, nArgCount, 17),
            nArgCount > 18 ? pArgInf[18].m_int : 0,
            nArgCount > 19 ? pArgInf[19].m_int : 0,
            true, &pArgInf[7], true, &pArgInf[15]);
        return true;
    }
    case HOP_G_WebAccessCurl:
    {
        // 全局版：(完整网址, 访问方式, 回调子程序, 提交信息, 提交COOKIE, 返回COOKIE, 超时,
        //  附加协议头, 返回头, 禁止重定向, 网站登录用户名, 网站登录密码, 代理地址, 代理用户名,
        //  代理密码, 是否采用Gzip, 返回错误日志, 代理连接失败自动重试, 模拟浏览器TSL, 取消令牌)
        CxHttpObj* pObj = HNew(CXH_KIND_ACC_CURL);
        if (pObj == NULL) { pRetData->m_pBin = CloneBinData(NULL, 0); return true; }
        if (!HOptionalToken(pObj, nArgCount, pArgInf, 19))
        {
            HFreeObj(pObj);
            pRetData->m_pBin = CloneBinData(NULL, 0);
            return true;
        }
        const void* pPost = nArgCount > 3 ? (const void*)HBinData(pArgInf[3]) : NULL;
        const int nPostLen = nArgCount > 3 ? HBinLen(pArgInf[3]) : 0;
        HAccessCurlInvokeSelf(pRetData, pObj,
            HTextAt(pArgInf, nArgCount, 0), HTextAt(pArgInf, nArgCount, 1),
            nArgCount > 2 ? pArgInf[2].m_int : 0,
            pPost, nPostLen,
            nArgCount > 6 ? pArgInf[6].m_int : 0,
            HTextAt(pArgInf, nArgCount, 7), HTextAt(pArgInf, nArgCount, 4),
            nArgCount > 9 ? (pArgInf[9].m_bool == BL_TRUE) : FALSE,
            HTextAt(pArgInf, nArgCount, 10), HTextAt(pArgInf, nArgCount, 11),
            HTextAt(pArgInf, nArgCount, 12), HTextAt(pArgInf, nArgCount, 13), HTextAt(pArgInf, nArgCount, 14),
            nArgCount > 15 ? (pArgInf[15].m_bool == BL_TRUE) : TRUE,
            nArgCount > 17 ? (pArgInf[17].m_bool == BL_TRUE) : FALSE,
            HTextAt(pArgInf, nArgCount, 18), 0, 0,
            true, &pArgInf[8], true, &pArgInf[16]);
        // 原模块这里 返回COOKIE 没标 参考，写不回去（源码 bug）；库按同样口径保留，
        // 想拿COOKIE 请用 返回头 里的 Set-Cookie 或访问类对象的 获取当前返回的COOKIE()。
        HFreeObj(pObj);
        return true;
    }
    default: break;
    }
    return false;
}

#endif  // CXHTTP_ACCESS_CURL_INC
