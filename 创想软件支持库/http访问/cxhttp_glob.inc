// HTTP访问 族：全局命令（原模块「公开子程序」）的派发与薄封装。
// 三条 访问网页 类命令分别落在 cxhttp_access_curl.inc / cxhttp_access_win.inc，
// 其余工具命令直接调 cxhttp_util.h 声明的工具函数。
// 参数名/顺序/可空性由 gen_http_table.mjs 从 cxhttp_api.json 生成，本文件只负责收实参和回填出参。
#ifndef CXHTTP_GLOB_INC
#define CXHTTP_GLOB_INC

// 三个访问类的统一入口（按对象种类分派）
static bool HAccessInvoke(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf, int nOp, int nKind, CxHttpObj** ppObj)
{
    if (nKind == CXH_KIND_ACC_CURL || nOp == HOP_G_WebAccessCurl)
        return HAccessCurlInvoke(pRetData, nArgCount, pArgInf, nOp, nKind, ppObj);
    if (nKind == CXH_KIND_ACC_HTTP || nKind == CXH_KIND_ACC_INET
        || nOp == HOP_G_WebAccessWinHttp || nOp == HOP_G_WebAccessWinInet)
        return HAccessWinInvoke(pRetData, nArgCount, pArgInf, nOp, nKind, ppObj);
    return false;
}

// 取中间文本的多段版：左标记 →（可选 中间标记）→ 右标记，取最后一段到右标记之间
static BOOL HXBetweenFrom(CxBuf* pOut, LPCSTR szFrom, LPCSTR szLeft, LPCSTR szMid, LPCSTR szRight)
{
    if (szLeft == NULL || szLeft[0] == '\0' || szRight == NULL || szRight[0] == '\0') return FALSE;
    LPCSTR p = strstr(szFrom, szLeft);
    if (p == NULL) return FALSE;
    p += lstrlenA(szLeft);
    if (szMid != NULL && szMid[0] != '\0')
    {
        LPCSTR pM = strstr(p, szMid);
        if (pM == NULL) return FALSE;
        p = pM + lstrlenA(szMid);
    }
    LPCSTR pR = strstr(p, szRight);
    if (pR == NULL) return FALSE;
    BufAppend(pOut, p, (int)(pR - p));
    return TRUE;
}

// formData 元素里的四个字段都是「指向别的块」的指针，逐个验证可读再用
static BOOL HXFieldOk(LPCSTR p)
{
    if (p == NULL) return TRUE;                       // 空指针是合法的空文本/空字节集
    return IsBadReadPtr(p, 1) ? FALSE : TRUE;
}
static BOOL HXFieldsOk(LPBYTE pItem)
{
    for (int k = 0; k < 4; k++)
        if (!HXFieldOk(*(LPCSTR*)(pItem + k * sizeof(INT)))) return FALSE;
    return TRUE;
}

void CxHttpGlobalInvoke(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf, int nOp)
{
    if (!LoadCurlApi() && (nOp == HOP_G_WebAccessCurl || nOp == HOP_G_CompressAmazonPage))
    {
        pRetData->m_pBin = NULL;
        return;
    }
    switch (nOp)
    {
    // ---- 三条访问命令 ----
    case HOP_G_WebAccessCurl:
    case HOP_G_WebAccessWinHttp:
    case HOP_G_WebAccessWinInet:
    {
        static CxHttpObj* s_pNone = NULL;      // 全局命令没有对象自身，占位给派发器写
        HAccessInvoke(pRetData, nArgCount, pArgInf, nOp, 0, &s_pNone);
        return;
    }

    // ---- curl 全局生命周期（原模块 CURL专用 的两条公开命令）----
    case HOP_G_CurlPreload:
        SetRetText(pRetData, LoadCurlApi() ? "" : CX_CURL_DLL_NAME);
        return;
    case HOP_G_CurlShutdown:
        // 故意不做 curl_global_cleanup：库里可能有活的 easy/multi 句柄，清理会让它们下次调用时崩。
        // 全局状态随进程退出统一释放，与原模块「结束程序时销毁」的意图一致。
        SetRetText(pRetData, "");
        return;

    // ---- Cookie ----
    case HOP_G_WebCookieMerge:
    {
        CxBuf out;
        BufInit(&out, 128);
        UxCookieMerge(&out, HTextAt(pArgInf, nArgCount, 0), HTextAt(pArgInf, nArgCount, 1));
        SetRetText(pRetData, out.nLen == 0 ? "" : out.pData);
        BufFree(&out);
        return;
    }
    case HOP_G_WebCookieFromHead:
    case HOP_G_AnalyseHeadCookies:
    {
        CxBuf out;
        BufInit(&out, 128);
        UxCookieFromHead(&out, HTextAt(pArgInf, nArgCount, 0));
        SetRetText(pRetData, out.nLen == 0 ? "" : out.pData);
        BufFree(&out);
        return;
    }
    case HOP_G_CookieMerge:
    {
        // 与原模块的 网页_Cookie合并更新_内部 同一套规则（同名新值赢、剔 deleted、去重）；
        // 原两条命令在「是否清洗空格、空值处理」上不一致属源码缺陷，这里统一按内部版。
        CxBuf out;
        BufInit(&out, 128);
        UxCookieMerge(&out, HTextAt(pArgInf, nArgCount, 0), HTextAt(pArgInf, nArgCount, 1));
        SetRetText(pRetData, out.nLen == 0 ? "" : out.pData);
        BufFree(&out);
        return;
    }
    case HOP_G_WebCookieOne:
    {
        CxBuf out;
        BufInit(&out, 64);
        UxCookieOne(&out, HTextAt(pArgInf, nArgCount, 0), HTextAt(pArgInf, nArgCount, 1));
        SetRetText(pRetData, out.nLen == 0 ? "" : out.pData);
        BufFree(&out);
        return;
    }
    case HOP_G_WebCookieInner:
    {
        CxBuf out;
        BufInit(&out, 256);
        UxCookieFromUrl(&out, HTextAt(pArgInf, nArgCount, 0),
            nArgCount > 1 ? HTextAt(pArgInf, nArgCount, 1) : NULL,
            nArgCount > 2 ? (pArgInf[2].m_bool == BL_TRUE) : FALSE);
        SetRetText(pRetData, out.nLen == 0 ? "" : out.pData);
        BufFree(&out);
        return;
    }

    // ---- 协议头 / 状态行 ----
    case HOP_G_WebBuildHeaders:
    {
        CxBuf out;
        BufInit(&out, 512);
        UxBuildHeaders(&out, HTextAt(pArgInf, nArgCount, 0), HTextAt(pArgInf, nArgCount, 1),
            HTextAt(pArgInf, nArgCount, 2),
            nArgCount > 3 ? (pArgInf[3].m_bool == BL_TRUE) : FALSE,
            nArgCount > 4 ? (pArgInf[4].m_bool == BL_TRUE) : FALSE,
            HIntAt(pArgInf, nArgCount, 5));
        SetRetText(pRetData, out.nLen == 0 ? "" : out.pData);
        BufFree(&out);
        return;
    }
    case HOP_G_WebPickHeader:
    {
        CxBuf out;
        BufInit(&out, 64);
        UxPickHeader(&out, HTextAt(pArgInf, nArgCount, 0), HTextAt(pArgInf, nArgCount, 1));
        SetRetText(pRetData, out.nLen == 0 ? "" : out.pData);
        BufFree(&out);
        return;
    }
    case HOP_G_JudgeHeadValid:
        pRetData->m_bool = UxHeadIsOk(HTextAt(pArgInf, nArgCount, 0)) ? BL_TRUE : BL_FALSE;
        return;
    case HOP_G_GetHeadCode:
    {
        // 原模块声明 逻辑型 却返回文本（源码缺陷）。这里按声明给逻辑值：能解析出状态行给真。
        CxBuf code;
        BufInit(&code, 16);
        UxStatusCode(&code, HTextAt(pArgInf, nArgCount, 0));
        pRetData->m_bool = code.nLen > 0 ? BL_TRUE : BL_FALSE;
        BufFree(&code);
        return;
    }
    case HOP_G_WebStatusCode:
    {
        CxBuf code, phrase;
        BufInit(&code, 16);
        BufInit(&phrase, 32);
        UxStatusCode(&code, HTextAt(pArgInf, nArgCount, 0));
        UxHeadCodeText(&phrase, HTextAt(pArgInf, nArgCount, 0));
        SetRetText(pRetData, code.nLen == 0 ? "" : code.pData);
        if (nArgCount > 1) HSetRefText(&pArgInf[1], phrase.nLen == 0 ? "" : phrase.pData);
        BufFree(&code); BufFree(&phrase);
        return;
    }
    case HOP_G_WebSystemProxy:
    {
        CxBuf out;
        BufInit(&out, 64);
        UxSystemProxy(&out);
        SetRetText(pRetData, out.nLen == 0 ? "" : out.pData);
        BufFree(&out);
        return;
    }
    case HOP_G_WebContentType:
    {
        CxBuf out;
        BufInit(&out, 32);
        UxContentType(&out, HTextAt(pArgInf, nArgCount, 0));
        SetRetText(pRetData, out.nLen == 0 ? "" : out.pData);
        BufFree(&out);
        return;
    }

    // ---- 取中间文本（会就地推进 起始查找位置，原语义如此）----
    case HOP_G_GetMiddleText:
    {
        LPCSTR szSrc = HTextAt(pArgInf, nArgCount, 0);
        LPCSTR szLeft = HTextAt(pArgInf, nArgCount, 2);
        LPCSTR szMid = nArgCount > 3 ? HTextAt(pArgInf, nArgCount, 3) : "";
        LPCSTR szRight = HTextAt(pArgInf, nArgCount, 4);
        int nPos = HGetRefInt(pArgInf[1]);                 // 起始查找位置（参考，1 基）
        if (nPos < 1) nPos = 1;
        if (nPos - 1 > (int)lstrlenA(szSrc)) nPos = (int)lstrlenA(szSrc) + 1;
        CxBuf mid;
        BufInit(&mid, 64);
        BOOL bOk = HXBetweenFrom(&mid, szSrc + nPos - 1, szLeft, szMid, szRight);
        if (bOk)
        {
            LPCSTR pHit = strstr(szSrc + nPos - 1, szLeft);
            if (pHit != NULL) HSetRefInt(&pArgInf[1], (int)(pHit - szSrc) + 1 + lstrlenA(szLeft));
        }
        SetRetText(pRetData, bOk && mid.nLen > 0 ? mid.pData : "");
        BufFree(&mid);
        return;
    }

    // ---- 表单 / 文本工具 ----
    case HOP_G_WebBuildForm:
    {
        // formDatas 是 formData（自定义类型）数组：
        //   { 文本型 文件名, 字节集 提交数据, 文本型 文件格式, 文本型 表单名称 }
        CxBuf body, head, bnd;
        BufInit(&body, 1024);
        BufInit(&head, 128);
        BufInit(&bnd, 16);
        UxRandChars(&bnd, 10);                       // 10 位随机字符作边界尾巴
        CxBuf mark;
        BufInit(&mark, 64);
        BufAppend(&mark, "------WebKitFormBoundary", 24);
        BufAppend(&mark, bnd.pData, bnd.nLen);
        const int n = HXAryCount(pArgInf[0]);
        int nKept = 0;
        for (int i = 0; i < n; i++)
        {
            LPBYTE pItem = (LPBYTE)((LPCSTR*)pArgInf[0].m_pAryData)[i];
            if (pItem == NULL || IsBadReadPtr(pItem, 16)) continue;
            if (!HXFieldsOk(pItem)) continue;
            LPCSTR szFile = *(LPCSTR*)(pItem + 0);
            LPCSTR szData = *(LPCSTR*)(pItem + 4);     // 字节集块指针
            LPCSTR szMime = *(LPCSTR*)(pItem + 8);
            LPCSTR szName = *(LPCSTR*)(pItem + 12);
            if (szName == NULL || szName[0] == '\0') continue;      // 表单名称为空就跳过（原逻辑）
            nKept++;
            // 原模块把 文件格式 的自动推断放在拼好报文之后，等于没生效；这里先补再拼（修 bug）
            CxBuf mime;
            BufInit(&mime, 32);
            if ((szMime == NULL || szMime[0] == '\0') && szFile != NULL && szFile[0] != '\0')
                UxContentType(&mime, szFile);
            else BufAppend(&mime, szMime == NULL ? "" : szMime, szMime == NULL ? 0 : lstrlenA(szMime));

            CxBuf part;
            BufInit(&part, 128);
            BufAppend(&part, mark.pData, mark.nLen);
            BufAppend(&part, "\r\n", 2);
            BufAppend(&part, "Content-Disposition: form-data; name=\"", 36);
            BufAppend(&part, szName, lstrlenA(szName));
            BufAppend(&part, "\"", 1);
            if (szFile != NULL && szFile[0] != '\0')
            {
                // 原模块这里 filename 填的是 表单名称（bug），已改成真正的 文件名
                BufAppend(&part, "; filename=\"", 12);
                BufAppend(&part, szFile, lstrlenA(szFile));
                BufAppend(&part, "\"\r\nContent-Type: ", 17);
                BufAppend(&part, mime.pData, mime.nLen);
            }
            BufAppend(&part, "\r\n\r\n", 4);
            BufAppend(&body, part.pData, part.nLen);
            BufFree(&part); BufFree(&mime);

            int nData = szData == NULL ? 0 : *(INT*)((LPBYTE)szData + sizeof(INT));
            if (nData < 0 || nData > 0x08000000) nData = 0;
            if (nData > 0) BufAppend(&body, (LPCSTR)((LPBYTE)szData + sizeof(INT) * 2), nData);
            BufAppend(&body, "\r\n", 2);
        }
        if (nKept > 0)
        {
            BufAppend(&body, mark.pData, mark.nLen);
            BufAppend(&body, "--", 2);
        }
        BufAppend(&head, "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary", 58);
        BufAppend(&head, bnd.pData, bnd.nLen);
        pRetData->m_pBin = CloneBinData(body.pData, body.nLen);
        if (nArgCount > 1) HSetRefText(&pArgInf[1], head.nLen == 0 ? "" : head.pData);
        BufFree(&body); BufFree(&head); BufFree(&bnd); BufFree(&mark);
        return;
    }
    case HOP_G_TextCountOccur:
    {
        const BOOL bCase = nArgCount > 2 ? (pArgInf[2].m_bool == BL_TRUE) : TRUE;
        pRetData->m_int = UxCountOccur(HTextAt(pArgInf, nArgCount, 0),
            HTextAt(pArgInf, nArgCount, 1), bCase);
        return;
    }
    case HOP_G_CompressAmazonPage:
    {
        int n = 0;
        LPCSTR pRaw = nArgCount > 0 ? HGetValueBin(pArgInf[0], &n) : NULL;
        if (pRaw == NULL) { pRetData->m_pBin = CloneBinData(NULL, 0); return; }
        int nOut = 0;
        LPBYTE pOut = UxCompressAmazonPage(pRaw, n, &nOut);
        if (pOut == NULL || nOut <= 0) { pRetData->m_pBin = CloneBinData(pRaw, n); return; }
        pRetData->m_pBin = CloneBinData((LPCSTR)pOut, nOut);
        LocalFree(pOut);
        return;
    }
    default:
        ReportError("HTTP访问：未知的全局命令");
        return;
    }
}

#endif  // CXHTTP_GLOB_INC
