// HTTP访问 族：curl_multi / URL参数类 / curl_smtp 独有命令的实现体。
// 由 lib.cpp 在 cxhttp_core.inc 之后 #include；只认自己的操作码，其余返回假交回核心派发。
// 参数名/顺序/类型/可空/返回类型逐字照原源码（见 cxhttp_api.json）。
#ifndef CXHTTP_EXTRA_INC
#define CXHTTP_EXTRA_INC

// ============================================================ 文本/字节小工具
// GBK 安全的「删全部空」：去掉半角空格、制表、回车、换行与全角空格(A1A1)。
// 双字节必须成对识别，否则「尾字节 A1 + 首字节 A1」会被误删。
static void HXNoSpace(CxBuf* pOut, LPCSTR sz)
{
    for (int i = 0; sz[i] != '\0'; )
    {
        const BYTE b = (BYTE)sz[i];
        if (b == ' ' || b == '\t' || b == '\r' || b == '\n') { i++; continue; }
        if (b >= 0x81 && b <= 0xFE && sz[i + 1] != '\0')
        {
            if (b == 0xA1 && (BYTE)sz[i + 1] == 0xA1) { i += 2; continue; }
            BufAppend(pOut, sz + i, 2);
            i += 2;
            continue;
        }
        BufAppend(pOut, sz + i, 1);
        i++;
    }
}

static void HXCopy(CxBuf* pOut, LPCSTR sz) { BufAppend(pOut, sz, lstrlenA(sz)); }

// 整串替换（原模块的 子文本替换(源, 被替, 新, , , 真)）
static void HXReplaceAll(CxBuf* pOut, LPCSTR szSrc, LPCSTR szFrom, LPCSTR szTo)
{
    const int nFrom = lstrlenA(szFrom);
    if (nFrom <= 0) { HXCopy(pOut, szSrc); return; }
    const int nTo = lstrlenA(szTo);
    LPCSTR p = szSrc;
    for (;;)
    {
        LPCSTR pHit = strstr(p, szFrom);
        if (pHit == NULL) { HXCopy(pOut, p); break; }
        BufAppend(pOut, p, (int)(pHit - p));
        if (nTo > 0) BufAppend(pOut, szTo, nTo);
        p = pHit + nFrom;
    }
}

static void HXLower(CxBuf* pOut, LPCSTR sz)
{
    for (int i = 0; sz[i] != '\0'; i++)
    {
        const char c = (char)tolower((BYTE)sz[i]);
        BufAppend(pOut, &c, 1);
    }
}

// 文本_取右边：取最后一个 标记 之后的内容；找不到返回空
static void HXRightOfLast(CxBuf* pOut, LPCSTR szSrc, LPCSTR szMark)
{
    const int nMark = lstrlenA(szMark);
    if (nMark <= 0) return;
    LPCSTR pLast = NULL;
    for (LPCSTR p = szSrc; ; )
    {
        LPCSTR pHit = strstr(p, szMark);
        if (pHit == NULL) break;
        pLast = pHit;
        p = pHit + nMark;
    }
    if (pLast != NULL) HXCopy(pOut, pLast + nMark);
}

// 文本_取出中间文本：第一个 左标记 之后到第一个 右标记
static BOOL HXBetween(CxBuf* pOut, LPCSTR szSrc, LPCSTR szLeft, LPCSTR szRight)
{
    LPCSTR pL = strstr(szSrc, szLeft);
    if (pL == NULL) return FALSE;
    pL += lstrlenA(szLeft);
    LPCSTR pR = strstr(pL, szRight);
    if (pR == NULL) return FALSE;
    BufAppend(pOut, pL, (int)(pR - pL));
    return TRUE;
}

// 编码_URL编码（逐字翻译原模块 通用模块.编码_URL编码）：
// 需要时先 GBK->UTF-8，再逐字节百分号编码（十六进制大写）；
// bKeepAlnum 为真时保留 - . 0-9 A-Z a-z（'/' 仍编码），为假时全部编码。
static void HXUrlEncode(CxBuf* pOut, LPCSTR szText, BOOL bKeepAlnum, BOOL bToUtf8)
{
    if (szText == NULL || szText[0] == '\0') return;
    static const char* const kHex = "0123456789ABCDEF";
    char* pSrc = NULL;
    if (bToUtf8)
    {
        const int nW = MultiByteToWideChar(936, 0, szText, -1, NULL, 0);
        if (nW > 1)
        {
            LPWSTR pW = (LPWSTR)LocalAlloc(LPTR, (SIZE_T)nW * sizeof(WCHAR));
            if (pW != NULL && MultiByteToWideChar(936, 0, szText, -1, pW, nW) > 0)
            {
                const int nU = WideCharToMultiByte(65001, 0, pW, -1, NULL, 0, NULL, NULL);
                if (nU > 1)
                {
                    pSrc = (char*)LocalAlloc(LPTR, nU);
                    if (pSrc != NULL) WideCharToMultiByte(65001, 0, pW, -1, pSrc, nU, NULL, NULL);
                }
            }
            if (pW != NULL) LocalFree(pW);
        }
    }
    if (pSrc == NULL)
    {
        const int n = lstrlenA(szText);
        pSrc = (char*)LocalAlloc(LPTR, n + 1);
        if (pSrc == NULL) return;
        memcpy(pSrc, szText, n);
    }
    for (LPCSTR p = pSrc; *p != '\0'; p++)
    {
        const BYTE b = (BYTE)*p;
        const BOOL bKeep = bKeepAlnum &&
            ((b > 44 && b < 58 && b != 47) || (b > 64 && b < 91) || (b > 96 && b < 123));
        if (bKeep) BufAppend(pOut, (LPCSTR)&b, 1);
        else
        {
            char szEsc[4];
            szEsc[0] = '%'; szEsc[1] = kHex[b >> 4]; szEsc[2] = kHex[b & 0xF]; szEsc[3] = '\0';
            BufAppend(pOut, szEsc, 3);
        }
    }
    LocalFree(pSrc);
}

static void HXBase64(CxBuf* pOut, const LPBYTE pData, int nLen)
{
    static const char* const kTab =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int nLine = 0;
    for (int i = 0; i < nLen; i += 3)
    {
        const DWORD v = ((DWORD)pData[i]) << 16 |
            ((i + 1 < nLen ? pData[i + 1] : 0) << 8) |
            (i + 2 < nLen ? pData[i + 2] : 0);
        char buf[5];
        buf[0] = kTab[(v >> 18) & 0x3F];
        buf[1] = kTab[(v >> 12) & 0x3F];
        buf[2] = i + 1 < nLen ? kTab[(v >> 6) & 0x3F] : '=';
        buf[3] = i + 2 < nLen ? kTab[v & 0x3F] : '=';
        BufAppend(pOut, buf, 4);
        nLine += 4;
        if (nLine >= 76) { BufAppend(pOut, "\r\n", 2); nLine = 0; }
    }
    if (nLine != 0) BufAppend(pOut, "\r\n", 2);
}

static BOOL HXReadFileA(LPCSTR szPath, CxBuf* pOut)
{
    const HANDLE hFile = CreateFileA(szPath, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;
    BYTE bTmp[8192];
    DWORD nRead = 0;
    while (ReadFile(hFile, bTmp, sizeof(bTmp), &nRead, NULL) && nRead > 0)
        BufAppend(pOut, (LPCSTR)bTmp, (int)nRead);
    CloseHandle(hFile);
    return TRUE;
}

static LPCSTR HXBaseName(LPCSTR szPath)
{
    LPCSTR p = szPath + lstrlenA(szPath);
    while (p > szPath) { --p; if (*p == '\\' || *p == '/') return p + 1; }
    return szPath;
}

// 从 “酷宝贝<kyozy@xx.com>” 取纯地址；没有尖括号就原样取
static void HXMailAddr(CxBuf* pOut, LPCSTR szFrom)
{
    LPCSTR pLt = strstr(szFrom, "<");
    LPCSTR pGt = pLt == NULL ? NULL : strstr(pLt, ">");
    if (pLt != NULL && pGt != NULL && pGt > pLt + 1)
    {
        BufAppend(pOut, pLt + 1, (int)(pGt - pLt - 1));
        return;
    }
    HXCopy(pOut, szFrom);
}

// 文本型数组实参的成员数（块 = [维数][个数][元素指针...]），块头被写坏时按 0 处理
static int HXAryCount(const MDATA_INF& ary)
{
    if (ary.m_pAryData == NULL) return 0;
    const int nRaw = *(INT*)((LPBYTE)ary.m_pAryData + sizeof(INT));
    return (nRaw < 0 || nRaw > 4096) ? 0 : nRaw;
}

// ============================================================ URL参数类
// 内部网址读写：一律 LocalAlloc，随对象一起释放
static void HPUSet(CxHttpObj* p, LPCSTR szUrl)
{
    CxBuf tmp;
    BufInit(&tmp, lstrlenA(szUrl) + 1);
    HXNoSpace(&tmp, szUrl);
    if (p->szUrl != NULL) LocalFree(p->szUrl);
    p->szUrl = (char*)LocalAlloc(LPTR, tmp.nLen + 1);
    if (p->szUrl != NULL && tmp.nLen > 0) memcpy(p->szUrl, tmp.pData, tmp.nLen);
    BufFree(&tmp);
}

// 判断参数是否存在：把 ? 之后的参数表转小写，按 & 切块，返回第一块以「参数名=」开头的块
static BOOL HPUFindSeg(CxBuf* pOut, LPCSTR szUrl, LPCSTR szLowerName)
{
    CxBuf body, lower;
    BufInit(&body, lstrlenA(szUrl) + 1);
    LPCSTR pQ = strchr(szUrl, '?');
    HXCopy(&body, pQ == NULL ? szUrl : pQ + 1);
    BufInit(&lower, body.nLen + 1);
    HXLower(&lower, body.pData);

    BOOL bFound = FALSE;
    int nStart = 0;
    CxBuf key;
    BufInit(&key, lstrlenA(szLowerName) + 2);
    BufAppend(&key, szLowerName, lstrlenA(szLowerName));
    BufAppend(&key, "=", 1);
    for (int i = 0; i <= lower.nLen && !bFound; i++)
    {
        if (i != lower.nLen && lower.pData[i] != '&') continue;
        const int nLen = i - nStart;
        if (nLen >= key.nLen && strncmp(lower.pData + nStart, key.pData, key.nLen) == 0)
        {
            BufAppend(pOut, lower.pData + nStart, nLen);   // 原命令返回的是小写块
            bFound = TRUE;
        }
        nStart = i + 1;
    }
    BufFree(&key); BufFree(&lower); BufFree(&body);
    return bFound;
}

// 添加参数（原模块 添加参数）：按末尾字符决定用 ? 还是 &
static void HPUAdd(CxHttpObj* p, LPCSTR szName, LPCSTR szValue, BOOL bEnc, BOOL bUtf8, BOOL bAlnum)
{
    CxBuf name, val;
    BufInit(&name, lstrlenA(szName) + 1);
    HXReplaceAll(&name, szName, "&", "");
    BufInit(&val, lstrlenA(szValue) * 3 + 1);
    if (bEnc) HXUrlEncode(&val, szValue, bAlnum, bUtf8);
    else HXReplaceAll(&val, szValue, "&", "");

    CxBuf line;
    BufInit(&line, name.nLen + val.nLen + 2);
    BufAppend(&line, name.pData, name.nLen);
    BufAppend(&line, "=", 1);
    BufAppend(&line, val.pData, val.nLen);

    LPCSTR szCur = p->szUrl == NULL ? "" : p->szUrl;
    const int nCur = lstrlenA(szCur);
    CxBuf next;
    BufInit(&next, nCur + line.nLen + 2);
    BufAppend(&next, szCur, nCur);
    if (nCur == 0) { /* 空网址：直接 name=value */ }
    else if (szCur[nCur - 1] == '?') { /* 直接接上 */ }
    else if (szCur[nCur - 1] == '/') BufAppend(&next, "?", 1);
    else if (szCur[nCur - 1] == '&') { /* 直接接上 */ }
    else BufAppend(&next, "&", 1);
    BufAppend(&next, line.pData, line.nLen);

    if (p->szUrl != NULL) LocalFree(p->szUrl);
    p->szUrl = (char*)LocalAlloc(LPTR, next.nLen + 1);
    if (p->szUrl != NULL && next.nLen > 0) memcpy(p->szUrl, next.pData, next.nLen);
    BufFree(&next); BufFree(&line); BufFree(&name); BufFree(&val);
}

// 修改参数值（原模块 修改参数值）
static BOOL HPUModify(CxHttpObj* p, LPCSTR szName, LPCSTR szValue, BOOL bEnc, BOOL bUtf8, BOOL bAlnum)
{
    CxBuf name, key, seg, oldBlock, newBlock, val;
    BufInit(&name, lstrlenA(szName) + 1);
    HXReplaceAll(&name, szName, "&", "");
    if (bEnc) { BufInit(&val, lstrlenA(szValue) * 3 + 1); HXUrlEncode(&val, szValue, bAlnum, bUtf8); }
    else { BufInit(&val, lstrlenA(szValue) + 1); HXReplaceAll(&val, szValue, "&", ""); }

    LPCSTR szUrl = p->szUrl == NULL ? "" : p->szUrl;
    BufInit(&seg, name.nLen + 2);
    HXLower(&seg, name.pData);
    CxBuf found;
    BufInit(&found, 32);
    const BOOL bExist = HPUFindSeg(&found, szUrl, seg.pData);
    BufFree(&found);

    BufInit(&key, name.nLen + 2);
    BufAppend(&key, name.pData, name.nLen);
    BufAppend(&key, "=", 1);
    BufInit(&oldBlock, 64);
    if (!HXBetween(&oldBlock, szUrl, key.pData, "&"))
    {
        BufReset(&oldBlock);
        HXRightOfLast(&oldBlock, szUrl, key.pData);
    }
    BufInit(&newBlock, key.nLen + val.nLen + 1);
    BufAppend(&newBlock, key.pData, key.nLen);
    BufAppend(&newBlock, val.pData, val.nLen);
    BufAppend(&oldBlock, "", 0);

    // 原模块把「参数名=」再拼回块首，整块替换
    CxBuf oldFull;
    BufInit(&oldFull, key.nLen + oldBlock.nLen + 1);
    BufAppend(&oldFull, key.pData, key.nLen);
    BufAppend(&oldFull, oldBlock.pData, oldBlock.nLen);

    if (bExist)
    {
        CxBuf out;
        BufInit(&out, lstrlenA(szUrl) + newBlock.nLen + oldFull.nLen + 1);
        HXReplaceAll(&out, szUrl, oldFull.pData, newBlock.pData);
        if (p->szUrl != NULL) LocalFree(p->szUrl);
        p->szUrl = (char*)LocalAlloc(LPTR, out.nLen + 1);
        if (p->szUrl != NULL && out.nLen > 0) memcpy(p->szUrl, out.pData, out.nLen);
        BufFree(&out);
    }
    BufFree(&name); BufFree(&key); BufFree(&seg); BufFree(&oldBlock); BufFree(&newBlock);
    BufFree(&val); BufFree(&oldFull);
    return bExist;
}

// 修改参数名（原模块 修改参数名）
static BOOL HPURename(CxHttpObj* p, LPCSTR szName, LPCSTR szNewName)
{
    CxBuf name;
    BufInit(&name, lstrlenA(szName) + 1);
    HXReplaceAll(&name, szName, "&", "");
    LPCSTR szUrl = p->szUrl == NULL ? "" : p->szUrl;
    if (name.nLen == 0 || strstr(szUrl, name.pData) == NULL)
    {
        BufFree(&name);
        return FALSE;
    }
    CxBuf key, block, oldFull, newFull;
    BufInit(&key, name.nLen + 2);
    BufAppend(&key, name.pData, name.nLen);
    BufAppend(&key, "=", 1);
    BufInit(&block, 64);
    if (!HXBetween(&block, szUrl, key.pData, "&"))
    {
        BufReset(&block);
        HXRightOfLast(&block, szUrl, key.pData);
    }
    BufInit(&oldFull, key.nLen + block.nLen + 1);
    BufAppend(&oldFull, key.pData, key.nLen);
    BufAppend(&oldFull, block.pData, block.nLen);
    BufInit(&newFull, oldFull.nLen + lstrlenA(szNewName) + 1);
    HXReplaceAll(&newFull, oldFull.pData, name.pData, szNewName);

    CxBuf out;
    BufInit(&out, lstrlenA(szUrl) + oldFull.nLen + newFull.nLen + 1);
    HXReplaceAll(&out, szUrl, oldFull.pData, newFull.pData);
    if (p->szUrl != NULL) LocalFree(p->szUrl);
    p->szUrl = (char*)LocalAlloc(LPTR, out.nLen + 1);
    if (p->szUrl != NULL && out.nLen > 0) memcpy(p->szUrl, out.pData, out.nLen);
    BufFree(&out); BufFree(&name); BufFree(&key); BufFree(&block); BufFree(&oldFull); BufFree(&newFull);
    return TRUE;
}

// 删除参数（原模块 删除参数）
static BOOL HPUDelete(CxHttpObj* p, LPCSTR szName)
{
    CxBuf name, seg, found;
    BufInit(&name, lstrlenA(szName) + 1);
    HXReplaceAll(&name, szName, "&", "");
    LPCSTR szUrl = p->szUrl == NULL ? "" : p->szUrl;
    BufInit(&seg, name.nLen + 1);
    HXLower(&seg, name.pData);
    BufInit(&found, 32);
    const BOOL bExist = HPUFindSeg(&found, szUrl, seg.pData);
    BufFree(&found);
    if (!bExist) { BufFree(&name); BufFree(&seg); return FALSE; }

    CxBuf key, block;
    BufInit(&key, name.nLen + 2);
    BufAppend(&key, name.pData, name.nLen);
    BufAppend(&key, "=", 1);
    BufInit(&block, 64);
    BOOL bMid = HXBetween(&block, szUrl, key.pData, "&");

    // 中间取到值 → 删「name=值&」；取不到（在结尾）→ 删「name=值」
    CxBuf delA, delB;
    BufInit(&delA, key.nLen + block.nLen + 2);
    BufAppend(&delA, key.pData, key.nLen);
    BufAppend(&delA, block.pData, block.nLen);
    if (bMid) BufAppend(&delA, "&", 1);
    BufInit(&delB, delA.nLen + 1);
    BufAppend(&delB, "&", 1);
    BufAppend(&delB, delA.pData, delA.nLen);

    CxBuf out, tmp;
    BufInit(&out, lstrlenA(szUrl) + 1);
    HXReplaceAll(&out, szUrl, delB.pData, "");
    BufInit(&tmp, out.nLen + 1);
    HXReplaceAll(&tmp, out.pData, delA.pData, "");
    if (p->szUrl != NULL) LocalFree(p->szUrl);
    p->szUrl = (char*)LocalAlloc(LPTR, tmp.nLen + 1);
    if (p->szUrl != NULL && tmp.nLen > 0) memcpy(p->szUrl, tmp.pData, tmp.nLen);
    BufFree(&tmp); BufFree(&out); BufFree(&delA); BufFree(&delB);
    BufFree(&key); BufFree(&block); BufFree(&name); BufFree(&seg);
    return TRUE;
}

// ============================================================ 派发
static bool HExtra(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf, int nOp, int nKind, CxHttpObj** ppObj)
{
    CxHttpObj* p = *ppObj;
    switch (nOp)
    {
    // ---------------- curl_multi ----------------
    case HOP_CurlMulti_Create:
    {
        if (p == NULL)
        {
            p = HNew(CXH_KIND_MULTI);
            if (p == NULL) { ReportError("创建 curl_multi 对象失败：内存不足"); return TRUE; }
            *(PINT)pArgInf[0].m_pCompoundData = (INT)p;
            *ppObj = p;
        }
        fn_multi_init_t f = (fn_multi_init_t)H_PROC("curl_multi_init");
        if (f == NULL) { ReportError("curl_multi_init 入口不存在"); return TRUE; }
        if (p->h == NULL) p->h = (CX_CURL)f();
        if (p->h == NULL) { ReportError("curl_multi_init 失败"); return TRUE; }
        pRetData->m_bool = BL_TRUE;
        return TRUE;
    }
    case HOP_CurlMulti_Destroy:
        if (p != NULL) { HFreeObj(p); *(PINT)pArgInf[0].m_pCompoundData = 0; }
        return TRUE;
    case HOP_CurlMulti_GetHandle:
        pRetData->m_int = p == NULL ? 0 : (INT)p->h;
        return TRUE;
    case HOP_CurlMulti_SetHandle:
    {
        if (p == NULL)
        {
            p = HNew(CXH_KIND_MULTI);
            if (p == NULL) return TRUE;
            *(PINT)pArgInf[0].m_pCompoundData = (INT)p;
            *ppObj = p;
        }
        const INT nNew = pArgInf[1].m_int;
        // 外部句柄先验可读：否则后面每条命令都会在 curl 里解引用崩溃
        if (nNew != 0 && IsBadReadPtr((LPCVOID)nNew, 16))
        {
            ReportError("置句柄：传入的 CURLM 句柄不可读，已拒收");
            return TRUE;
        }
        p->h = (CX_CURL)nNew;
        return TRUE;
    }
    case HOP_CurlMulti_AddHandle: case HOP_CurlMulti_RemoveHandle:
    {
        if (p == NULL || p->h == NULL) { ReportError("curl_multi 句柄无效：请先调用 创建()"); return TRUE; }
        fn_multi_add_t f = (fn_multi_add_t)H_PROC(nOp == HOP_CurlMulti_AddHandle
            ? "curl_multi_add_handle" : "curl_multi_remove_handle");
        if (f == NULL) { ReportError("curl_multi_add_handle 入口不存在"); return TRUE; }
        CX_CURL hEasy = NULL;
        if (pArgInf[1].m_pCompoundData != NULL)
        {
            CxHttpObj* pEasy = (CxHttpObj*)(*(PINT)pArgInf[1].m_pCompoundData);
            if (pEasy != NULL) hEasy = pEasy->h;
        }
        if (hEasy == NULL) { ReportError("参数_curl 还没有可用的 easy 句柄"); return TRUE; }
        pRetData->m_int = f(p->h, hEasy);
        return TRUE;
    }
    case HOP_CurlMulti_Perform:
    {
        if (p == NULL || p->h == NULL) { ReportError("curl_multi 句柄无效：请先调用 创建()"); return TRUE; }
        fn_multi_perform_t f = (fn_multi_perform_t)H_PROC("curl_multi_perform");
        if (f == NULL) { ReportError("curl_multi_perform 入口不存在"); return TRUE; }
        int nStill = 0;
        pRetData->m_int = f(p->h, &nStill);
        HSetRefInt(&pArgInf[1], nStill);                    // 参数_仍在运行数（参考）
        return TRUE;
    }
    case HOP_CurlMulti_Wait:
    {
        if (p == NULL || p->h == NULL) { ReportError("curl_multi 句柄无效：请先调用 创建()"); return TRUE; }
        fn_multi_wait_t f = (fn_multi_wait_t)H_PROC("curl_multi_wait");
        if (f == NULL) { ReportError("curl_multi_wait 入口不存在"); return TRUE; }
        int nFds = 0;
        pRetData->m_int = f(p->h, NULL, 0, pArgInf[1].m_int, &nFds);
        HSetRefInt(&pArgInf[2], nFds);                      // 参数_文件描述符数（参考）
        return TRUE;
    }
    case HOP_CurlMulti_InfoRead:
    {
        if (p == NULL || p->h == NULL) { ReportError("curl_multi 句柄无效：请先调用 创建()"); return TRUE; }
        fn_multi_info_read_t f = (fn_multi_info_read_t)H_PROC("curl_multi_info_read");
        if (f == NULL) { ReportError("curl_multi_info_read 入口不存在"); return TRUE; }
        int nLeft = 0;
        CURLMsg* pMsg = (CURLMsg*)f(p->h, &nLeft);
        HSetRefInt(&pArgInf[1], nLeft);                     // 参数_剩余消息数（参考）
        pRetData->m_bool = pMsg == NULL ? BL_FALSE : BL_TRUE;
        if (pMsg != NULL && pArgInf[2].m_pCompoundData != NULL)
        {
            // CURL_消息 = { 整数型 msg, 整数型 easy_handle, 整数型 result }
            LPINT pField = (LPINT)pArgInf[2].m_pCompoundData;
            pField[0] = (int)pMsg->msg;
            pField[1] = (INT)(INT_PTR)pMsg->easy_handle;
            pField[2] = (int)pMsg->data.result;
        }
        return TRUE;
    }
    case HOP_CurlMulti_Setopt:
    {
        if (p == NULL || p->h == NULL) { ReportError("curl_multi 句柄无效：请先调用 创建()"); return TRUE; }
        fn_multi_setopt_t f = (fn_multi_setopt_t)H_PROC("curl_multi_setopt");
        if (f == NULL) { ReportError("curl_multi_setopt 入口不存在"); return TRUE; }
        const int nOpt = pArgInf[1].m_int;
        if (nOpt <= 0 || nOpt >= 100000) { pRetData->m_int = 1; return TRUE; }   // CURLM_UNKNOWN_OPTION
        pRetData->m_int = f(p->h, nOpt, (long)pArgInf[2].m_int);
        return TRUE;
    }

    // ---------------- URL参数类 ----------------
    case HOP_UrlParam_InitUrl:
    {
        if (p == NULL)
        {
            p = HNew(CXH_KIND_URLPARAM);
            if (p == NULL) { ReportError("创建 URL参数类 对象失败：内存不足"); return TRUE; }
            *(PINT)pArgInf[0].m_pCompoundData = (INT)p;
            *ppObj = p;
        }
        HPUSet(p, HTextAt(pArgInf, nArgCount, 1));
        return TRUE;
    }
    case HOP_UrlParam_GetUrl:
        SetRetText(pRetData, p == NULL || p->szUrl == NULL ? "" : p->szUrl);
        return TRUE;
    case HOP_UrlParam_FindParam:
    {
        CxBuf low, seg;
        BufInit(&low, lstrlenA(HTextAt(pArgInf, nArgCount, 2)) + 1);
        HXLower(&low, HTextAt(pArgInf, nArgCount, 2));
        BufInit(&seg, 32);
        HPUFindSeg(&seg, HTextAt(pArgInf, nArgCount, 1), low.pData);
        SetRetText(pRetData, seg.nLen == 0 ? "" : seg.pData);
        BufFree(&seg); BufFree(&low);
        return TRUE;
    }
    case HOP_UrlParam_AddParam: case HOP_UrlParam_ModifyValue: case HOP_UrlParam_AddOrModify:
    {
        if (p == NULL)
        {
            p = HNew(CXH_KIND_URLPARAM);
            if (p == NULL) { ReportError("创建 URL参数类 对象失败：内存不足"); return TRUE; }
            *(PINT)pArgInf[0].m_pCompoundData = (INT)p;
            *ppObj = p;
        }
        // 可空逻辑型实参：没传按原模块默认值（编码/自动UTF8/不编码数字 默认真，空值跳过默认假）
        const BOOL bEnc  = nArgCount > 3 ? (pArgInf[3].m_bool == BL_TRUE) : TRUE;
        const BOOL bUtf8 = nArgCount > 4 ? (pArgInf[4].m_bool == BL_TRUE) : TRUE;
        const BOOL bAlnum = nArgCount > 5 ? (pArgInf[5].m_bool == BL_TRUE) : TRUE;
        const BOOL bSkipEmpty = nArgCount > 6 ? (pArgInf[6].m_bool == BL_TRUE) : FALSE;
        LPCSTR szName = HTextAt(pArgInf, nArgCount, 1);
        LPCSTR szValue = HTextAt(pArgInf, nArgCount, 2);

        // 0 = 添加参数，1 = 修改参数值，2 = 添加修改参数（按存在与否转调前两者）
        int nMode = nOp == HOP_UrlParam_AddParam ? 0 : (nOp == HOP_UrlParam_ModifyValue ? 1 : 2);
        if (nMode == 2)
        {
            // 添加修改参数：存在则改，不存在则加（原模块就是转调这两条）
            CxBuf low, found;
            BufInit(&low, lstrlenA(szName) + 1);
            HXLower(&low, szName);
            BufInit(&found, 32);
            HPUFindSeg(&found, p->szUrl == NULL ? "" : p->szUrl, low.pData);
            nMode = found.nLen > 0 ? 1 : 0;
            BufFree(&found); BufFree(&low);
        }
        if (szValue[0] == '\0' && bSkipEmpty)
        {
            if (nMode == 1) pRetData->m_bool = BL_FALSE;
            return TRUE;
        }
        HPUSet(p, p->szUrl == NULL ? "" : p->szUrl);       // 三条命令都先 删全部空(内部网址)
        if (nMode == 0) HPUAdd(p, szName, szValue, bEnc, bUtf8, bAlnum);
        else pRetData->m_bool = HPUModify(p, szName, szValue, bEnc, bUtf8, bAlnum) ? BL_TRUE : BL_FALSE;
        if (nMode == 2) pRetData->m_int = 0;               // 添加修改参数 无返回值
        return TRUE;
    }
    case HOP_UrlParam_ModifyName:
    {
        if (p == NULL) { pRetData->m_bool = BL_FALSE; return TRUE; }
        HPUSet(p, p->szUrl == NULL ? "" : p->szUrl);
        pRetData->m_bool = HPURename(p, HTextAt(pArgInf, nArgCount, 1), HTextAt(pArgInf, nArgCount, 2))
            ? BL_TRUE : BL_FALSE;
        return TRUE;
    }
    case HOP_UrlParam_RemoveParam:
    {
        if (p == NULL) { pRetData->m_bool = BL_FALSE; return TRUE; }
        HPUSet(p, p->szUrl == NULL ? "" : p->szUrl);
        pRetData->m_bool = HPUDelete(p, HTextAt(pArgInf, nArgCount, 1)) ? BL_TRUE : BL_FALSE;
        return TRUE;
    }

    // ---------------- curl_smtp 独有 ----------------
    case HOP_CurlSmtp_SetServer:
    {
        if (p == NULL || p->h == NULL) { ReportError("curl_smtp 句柄无效：请先调用 创建()"); return TRUE; }
        fn_easy_setopt_t so = g_curl.EasySetopt;
        CxBuf url;
        BufInit(&url, 128);
        LPCSTR szHost = HTextAt(pArgInf, nArgCount, 1);
        if (strstr(szHost, "://") != NULL) HXCopy(&url, szHost);
        else { BufAppend(&url, "smtp://", 7); HXCopy(&url, szHost); }
        const int nPort = HIntAt(pArgInf, nArgCount, 2);
        if (nPort > 0)
        {
            char szP[16];
            wsprintfA(szP, ":%d", nPort);
            BufAppend(&url, szP, lstrlenA(szP));
        }
        int r = so(p->h, CURLOPT_URL, url.pData);
        BufFree(&url);
        r |= so(p->h, CURLOPT_USERNAME, HTextAt(pArgInf, nArgCount, 3));
        r |= so(p->h, CURLOPT_PASSWORD, HTextAt(pArgInf, nArgCount, 4));
        pRetData->m_int = r;
        return TRUE;
    }
    case HOP_CurlSmtp_SetMail:
    {
        if (p == NULL || p->h == NULL) { ReportError("curl_smtp 句柄无效：请先调用 创建()"); return TRUE; }
        fn_easy_setopt_t so = g_curl.EasySetopt;
        LPCSTR szSubject = HTextAt(pArgInf, nArgCount, 1);
        LPCSTR szBody = HTextAt(pArgInf, nArgCount, 2);
        LPCSTR szFrom = HTextAt(pArgInf, nArgCount, 3);
        LPCSTR szCharset = HTextAt(pArgInf, nArgCount, 6);
        if (szCharset[0] == '\0') szCharset = "gb2312";

        // 收件人列表 -> 「To: a, b」与信封收件人链
        if (p->pRcptList != NULL && g_curl.SlistFreeAll != NULL)
        {
            g_curl.SlistFreeAll((CxSlist*)p->pRcptList);
            p->pRcptList = NULL;
        }
        curl_slist* pRcpt = NULL;
        CxBuf toList, addr;
        BufInit(&toList, 64);
        BufInit(&addr, 64);
        const MDATA_INF& aryRcpt = pArgInf[4];
        const int nR = HXAryCount(aryRcpt);
        for (int i = 0; i < nR; i++)
        {
            LPCSTR sz = ((LPCSTR*)aryRcpt.m_pAryData)[i];
            if (sz == NULL || IsBadReadPtr(sz, 1) || sz[0] == '\0') continue;
            BufReset(&addr);
            HXMailAddr(&addr, sz);
            if (toList.nLen > 0) BufAppend(&toList, ", ", 2);
            BufAppend(&toList, addr.pData, addr.nLen);
            if (g_curl.SlistAppend != NULL)
            {
                curl_slist* pNext = (curl_slist*)g_curl.SlistAppend((CxSlist*)pRcpt, addr.pData);
                if (pNext != NULL) pRcpt = pNext;
            }
        }
        p->pRcptList = pRcpt;
        const int nA = HXAryCount(pArgInf[5]);

        BufReset(&p->mail);
        static const char* const kBoundary = "_===CxSoftMailBoundary===_";
        CxBuf head;
        BufInit(&head, 256);
        BufAppend(&head, "From: ", 6);
        HXCopy(&head, szFrom);
        BufAppend(&head, "\r\nTo: ", 6);
        BufAppend(&head, toList.pData, toList.nLen);
        BufAppend(&head, "\r\nSubject: ", 11);
        HXCopy(&head, szSubject);
        BufAppend(&head, "\r\nMIME-Version: 1.0\r\n", 21);
        BufAppend(&p->mail, head.pData, head.nLen);
        BufFree(&head);

        char szTmp[512];
        if (nA > 0)
        {
            wsprintfA(szTmp, "Content-Type: multipart/mixed; boundary=\"%s\"\r\n\r\n--%s\r\n"
                "Content-Type: text/html; charset=\"%s\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n",
                kBoundary, kBoundary, szCharset);
            BufAppend(&p->mail, szTmp, lstrlenA(szTmp));
            HXCopy(&p->mail, szBody);
            BufAppend(&p->mail, "\r\n", 2);
            for (int i = 0; i < nA; i++)
            {
                LPCSTR szPath = ((LPCSTR*)pArgInf[5].m_pAryData)[i];
                if (szPath == NULL || IsBadReadPtr(szPath, 1) || szPath[0] == '\0') continue;
                CxBuf data, b64;
                BufInit(&data, 1024);
                if (!HXReadFileA(szPath, &data)) { BufFree(&data); continue; }
                BufInit(&b64, data.nLen / 3 * 4 + 16);
                HXBase64(&b64, (LPBYTE)data.pData, data.nLen);
                LPCSTR szName = HXBaseName(szPath);
                wsprintfA(szTmp, "\r\n--%s\r\nContent-Type: application/octet-stream; name=\"%s\"\r\n"
                    "Content-Transfer-Encoding: base64\r\nContent-Disposition: attachment; filename=\"%s\"\r\n\r\n",
                    kBoundary, szName, szName);
                BufAppend(&p->mail, szTmp, lstrlenA(szTmp));
                BufAppend(&p->mail, b64.pData, b64.nLen);
                BufFree(&b64); BufFree(&data);
            }
            wsprintfA(szTmp, "\r\n--%s--\r\n", kBoundary);
            BufAppend(&p->mail, szTmp, lstrlenA(szTmp));
        }
        else
        {
            wsprintfA(szTmp, "Content-Type: text/html; charset=\"%s\"\r\n"
                "Content-Transfer-Encoding: 8bit\r\n\r\n", szCharset);
            BufAppend(&p->mail, szTmp, lstrlenA(szTmp));
            HXCopy(&p->mail, szBody);
        }

        CxBuf envFrom;
        BufInit(&envFrom, 64);
        HXMailAddr(&envFrom, szFrom);
        int r = so(p->h, CURLOPT_MAIL_FROM, envFrom.pData);
        BufFree(&envFrom);
        r |= so(p->h, CURLOPT_MAIL_RCPT, pRcpt);
        r |= so(p->h, CURLOPT_POSTFIELDS, p->mail.pData);
        r |= so(p->h, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)p->mail.nLen);
        BufFree(&toList); BufFree(&addr);
        pRetData->m_int = r;
        return TRUE;
    }
    default: break;
    }
    UNREFERENCED_PARAMETER(nKind);
    return FALSE;   // 不是本文件的命令，交回核心派发/
}

#endif  // CXHTTP_EXTRA_INC
