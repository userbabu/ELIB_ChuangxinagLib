// HTTP访问 族核心实现：原 zyCurl 模块 curl_easy / curl_http 两类的同名门面。
// 由 lib.cpp 在 namespace cx 内 #include，复用本库已加载的 libcurl-impersonate。
// 与 lib.cpp 里 2.0 的 浏览器会话/请求/响应 完全隔离：自己持有 easy 句柄与响应缓冲。
// 参数名/顺序/类型/可空/数组/参考/返回类型逐字照原源码（见 cxhttp_api.json）。
//
// 有意修掉原模块两处隐患（只在原实现出错时行为不同）：
//  1) 响应缓冲改为对象私有，克隆对象不会读到别人的正文；
//  2) 句柄为空时统一报运行时错误，不再直接调 curl 造成崩溃。
#ifndef CXHTTP_CORE_INC
#define CXHTTP_CORE_INC

// uni_core.cpp 提供：易语言内存池分配
void* UniMalloc(INT nSize);   // 注意是 INT，不是 SIZE_T

#define CURL_STATICLIB
#include <curl/curl.h>

// 运行期再解析的入口（不进 CurlApi，免得动 lib.cpp 的加载器）
typedef void* (*fn_duphandle_t)(void*);
typedef char* (*fn_escape_t)(void*, const char*, int);
typedef char* (*fn_unescape_t)(void*, const char*, int, int*);
typedef void (*fn_free_t)(void*);
#define H_PROC(n) ((void*)GetProcAddress(g_curl.hModule, n))

struct CxHttpObj
{
    CX_CURL h;        // curl_easy 句柄
    CxBuf body;       // 响应正文
    CxBuf head;       // 响应头
    void* pSlist;     // 置请求头 建的 curl_slist 链
    CxBuf post;       // 置POST数据 的副本：curl 要求实参在 执行 期间存活
    void* pFn;        // 进度回调
    void* pUser;      // 进度回调用户数据
    int nHttp;        // 1 = curl_http（自动挂收集回调）
};

static size_t CXH_COLLECT_CB(char* ptr, size_t size, size_t n, void* user)
{
    const size_t bytes = size * n;
    if (user != NULL && bytes > 0) BufAppend((CxBuf*)user, ptr, (int)bytes);
    return bytes;
}

static CxHttpObj* HNew(int nHttp)
{
    CxHttpObj* p = (CxHttpObj*)LocalAlloc(LPTR, sizeof(CxHttpObj));
    if (p == NULL) return NULL;
    p->nHttp = nHttp;
    BufInit(&p->body, 4096);
    BufInit(&p->head, 1024);
    BufInit(&p->post, 1024);
    return p;
}

static void HFreeObj(CxHttpObj* p)
{
    if (p == NULL) return;
    if (p->h != NULL && g_curl.EasyCleanup != NULL) g_curl.EasyCleanup(p->h);
    if (p->pSlist != NULL && g_curl.SlistFreeAll != NULL) g_curl.SlistFreeAll((CxSlist*)p->pSlist);
    BufFree(&p->body);
    BufFree(&p->head);
    BufFree(&p->post);
    LocalFree(p);
}

// 复制函数：只复制配置与句柄，响应缓冲各自新建（原模块共享静态缓冲是隐患）
static void HCopyObj(PMDATA_INF pArgInf, int nHttp)
{
    CxHttpObj* pSrc = (CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData);
    CxHttpObj* pDst = HNew(nHttp);
    if (pDst == NULL) return;
    if (pSrc != NULL && pSrc->h != NULL)
    {
        fn_duphandle_t f = (fn_duphandle_t)H_PROC("curl_easy_duphandle");
        pDst->h = f == NULL ? pSrc->h : f(pSrc->h);
    }
    *(PINT)pArgInf[1].m_pCompoundData = (INT)pDst;
}

static CxHttpObj* HSelf(PMDATA_INF pArgInf)
{
    CxHttpObj* p = (CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData);
    if (p == NULL) ReportError("HTTP访问对象未初始化：请先调用 创建()");
    return p;
}

// 挂响应收集回调（curl_http）
static void HBind(CxHttpObj* p)
{
    BufReset(&p->body);
    BufReset(&p->head);
    g_curl.EasySetopt(p->h, CURLOPT_WRITEFUNCTION, (CXH_COLLECT_CB));
    g_curl.EasySetopt(p->h, CURLOPT_WRITEDATA, &p->body);
    g_curl.EasySetopt(p->h, CURLOPT_HEADERFUNCTION, (CXH_COLLECT_CB));
    g_curl.EasySetopt(p->h, CURLOPT_HEADERDATA, &p->head);
}

// 取可用句柄；失败已报错并返回 NULL
static CX_CURL HOk(CxHttpObj* p)
{
    if (p == NULL) return NULL;
    if (p->h == NULL)
    {
        ReportError("curl 句柄无效：请先调用 创建()，或用 置句柄() 绑定句柄");
        return NULL;
    }
    return p->h;
}

// 指针型选项：文本/字节集两种实参都接
// 选项号合法性：0 是 CURLOPT_ERRORBUFFER（curl 会往里写），越界值会让 curl 按错宽度取参
#define H_OPT_OK(n)  ((n) > 0 && (n) < 100000)

static int HSetopt(CX_CURL h, int nOpt, const MDATA_INF& v)
{
    fn_easy_setopt_t so = g_curl.EasySetopt;
    // 选项号 0（CURLOPT_ERRORBUFFER）等一律拒收：原模块在这里会把实参当缓冲区让 curl 写，
    // 传 0 就是往空指针写错误文本直接崩；支持库不该给调用方这个自毁入口。
    if (nOpt <= 0 || nOpt >= 100000) return CURLE_UNKNOWN_OPTION;
    switch (nOpt)
    {
    case CURLOPT_URL: case CURLOPT_PROXY: case CURLOPT_USERPWD: case CURLOPT_PROXYUSERPWD:
    case CURLOPT_USERNAME: case CURLOPT_PASSWORD: case CURLOPT_COOKIE: case CURLOPT_COOKIEFILE:
    case CURLOPT_COOKIEJAR: case CURLOPT_REFERER: case CURLOPT_USERAGENT: case CURLOPT_CAINFO:
    case CURLOPT_ACCEPT_ENCODING: case CURLOPT_CUSTOMREQUEST: case CURLOPT_POSTFIELDS:
    case CURLOPT_RANGE: case CURLOPT_SSLCERT: case CURLOPT_SSLKEY: case CURLOPT_INTERFACE:
    case CURLOPT_KRBLEVEL: case CURLOPT_DNS_SERVERS:
        return so(h, nOpt, v.m_dtDataType == SDT_BIN ? (const void*)v.m_pBin : (const void*)v.m_pText);
    case CURLOPT_HTTPHEADER: case CURLOPT_RESOLVE: case CURLOPT_HTTP200ALIASES:
        return so(h, nOpt, (const void*)(curl_slist*)v.m_int);
    default:
        if (v.m_dtDataType == SDT_INT64) return so(h, nOpt, (curl_off_t)v.m_int64);
        if (v.m_dtDataType == SDT_TEXT) return so(h, nOpt, v.m_pText);
        if (v.m_dtDataType == SDT_BIN) return so(h, nOpt, v.m_pBin);
        return so(h, nOpt, v.m_int);
    }
}

// 取信息：0=长整数 1=双精度 2=文本。值直接落进 pOut 的 union，故不再回传错误码——
// 调用方若再写 pOut->m_int，会把刚分配的字符串指针覆盖成错误码（同一块 union）。
static void HGetinfo(CX_CURL h, int nInfo, PMDATA_INF pOut, int nMode)
{
    // CURLINFO_* 都带类型基址（0x100000 起），越界值会让 curl 按错宽度写栈
    if (nInfo < 0x100000 || nInfo > 0x700000)
    {
        // 非法信息码：按声明的返回类型给同型零值，不能改类型（否则调用方拿到的数据类型和声明不符）
        if (nMode == 1) { pOut->m_double = 0; }
        else if (nMode == 2) { SetRetText(pOut, ""); }
        else { pOut->m_int = 0; }
        return;
    }
    switch (nMode)
    {
    case 0: { long v = 0; g_curl.EasyGetinfo(h, nInfo, &v); pOut->m_int = (int)v; break; }
    case 1: { double v = 0; g_curl.EasyGetinfo(h, nInfo, &v); pOut->m_double = v; break; }
    default: { char* v = NULL; g_curl.EasyGetinfo(h, nInfo, &v); SetRetText(pOut, v == NULL ? "" : v); break; }
    }
}

// 把缓冲按分隔符切开发进 文本型 参考数组。
// 元素与数据块一律走易语言内存池（UniAryWrite 内部 UniMalloc / NRS_FREE_ARY），
// 用 LocalAlloc 会让易语言释放时跨堆崩。
static void HFillTextAry(const MDATA_INF& arg, CxBuf* pSrc, char chSep)
{
    UniAryList list;
    int nStart = 0;
    for (int i = 0; i <= pSrc->nLen; i++)
    {
        if (i != pSrc->nLen && pSrc->pData[i] != chSep) continue;
        const int nLen = i - nStart;
        char* p = (char*)UniMalloc(nLen + 1);
        if (p != NULL)
        {
            if (nLen > 0) memcpy(p, pSrc->pData + nStart, nLen);
            p[nLen] = '\0';
            list.Add(p);
        }
        nStart = i + 1;
    }
    UniAryWrite(arg, &list);
}

// 文本型数组 -> curl_slist，旧链一并释放
static curl_slist* HBuildSlist(CxHttpObj* p, const MDATA_INF& ary)
{
    if (p->pSlist != NULL && g_curl.SlistFreeAll != NULL)
    {
        g_curl.SlistFreeAll((CxSlist*)p->pSlist);
        p->pSlist = NULL;
    }
    curl_slist* pList = NULL;
    // 数组块 = [维数][个数][元素指针...]，个数在块头第二个 INT
    const int nRaw = ary.m_pAryData == NULL ? 0 : *(INT*)((LPBYTE)ary.m_pAryData + sizeof(INT));
    const int n = (nRaw < 0 || nRaw > 4096) ? 0 : nRaw;      // 块头被写坏时不越界
    if (g_curl.SlistAppend != NULL)
    {
        for (int i = 0; i < n; i++)
        {
            LPCSTR sz = ((LPCSTR*)ary.m_pAryData)[i];
            // 元素可能是调用方给的野指针：先验可读再取串，否则 lstrlenA 直接崩在库里
            if (sz == NULL || IsBadReadPtr(sz, 1) || sz[0] == '\0') continue;
            curl_slist* pNext = (curl_slist*)g_curl.SlistAppend((CxSlist*)pList, sz);
            if (pNext == NULL) break;
            pList = pNext;
        }
    }
    p->pSlist = pList;
    return pList;
}

// 可空实参：没传就按 0 / 空串处理
static int HIntAt(PMDATA_INF pArgInf, INT nArgCount, int i)
{
    return (i < nArgCount) ? pArgInf[i].m_int : 0;
}
static LPCSTR HTextAt(PMDATA_INF pArgInf, INT nArgCount, int i)
{
    if (i >= nArgCount || pArgInf[i].m_pText == NULL) return "";
    return pArgInf[i].m_pText;
}

// ============================================================ 单入口派发
void CxHttpInvoke(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf, int nOp)
{
    if (!LoadCurlApi()) { ReportError(g_curl.LoadError); return; }

    CxHttpObj* p = (CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData);
    const int nHttp = (nOp >= HOP_CurlHttp_Create);

    // ---- 生命周期与句柄：不要求句柄已存在 ----
    switch (nOp)
    {
    case HOP_CurlEasy_Create: case HOP_CurlHttp_Create:
    {
        if (p == NULL)
        {
            p = HNew(nHttp);
            if (p == NULL) { ReportError("创建 HTTP 对象失败：内存不足"); return; }
            *(PINT)pArgInf[0].m_pCompoundData = (INT)p;
        }
        if (p->h == NULL) p->h = g_curl.EasyInit();
        if (p->h == NULL) { ReportError("curl_easy_init 失败"); return; }
        if (p->nHttp) HBind(p);
        pRetData->m_bool = BL_TRUE;
        return;
    }
    case HOP_CurlEasy_Destroy: case HOP_CurlHttp_Destroy:
        if (p != NULL) { HFreeObj(p); *(PINT)pArgInf[0].m_pCompoundData = 0; }
        return;
    case HOP_CurlEasy_GetHandle: case HOP_CurlHttp_GetHandle:
        pRetData->m_int = p == NULL ? 0 : (INT)p->h;
        return;
    case HOP_CurlEasy_SetHandle: case HOP_CurlHttp_SetHandle:
    {
        if (p == NULL)
        {
            p = HNew(nHttp);
            if (p == NULL) return;
            *(PINT)pArgInf[0].m_pCompoundData = (INT)p;
        }
        // 句柄是外部交进来的裸指针：不可读就拒收，否则后面每条命令都会在 curl 里解引用崩溃
        const INT nNew = pArgInf[1].m_int;
        if (nNew != 0 && IsBadReadPtr((LPCVOID)nNew, 8))
        {
            ReportError("置句柄：传入的句柄不可读，已拒收");
            return;
        }
        p->h = (CX_CURL)nNew;
        if (p->nHttp) HBind(p);
        return;
    }
    case HOP_CurlEasy_DupHandle: case HOP_CurlHttp_DupHandle:
    {
        CX_CURL h = HOk(p);
        if (h == NULL) return;
        fn_duphandle_t f = (fn_duphandle_t)H_PROC("curl_easy_duphandle");
        pRetData->m_int = f == NULL ? 0 : (INT)f(h);
        return;
    }
    case HOP_CurlEasy_Reset: case HOP_CurlHttp_Reset:
    {
        CX_CURL h = HOk(p);
        if (h == NULL) return;
        g_curl.EasyReset(h);
        p->pSlist = NULL;
        if (p->nHttp) HBind(p);
        return;
    }
    default: break;
    }

    CX_CURL h = HOk(p);
    if (h == NULL) return;
    fn_easy_setopt_t so = g_curl.EasySetopt;

    switch (nOp)
    {
    // ---- 通用 setopt / getinfo / perform / URL 编解码 ----
    case HOP_CurlEasy_SetoptInt: case HOP_CurlHttp_SetoptInt:
        pRetData->m_int = HSetopt(h, pArgInf[1].m_int, pArgInf[2]); return;
    case HOP_CurlEasy_SetoptLong: case HOP_CurlHttp_SetoptLong:
        if (!H_OPT_OK(pArgInf[1].m_int)) { pRetData->m_int = CURLE_UNKNOWN_OPTION; return; }
        // curl_off_t 型选项按值直传（cdecl 变参里 8 字节原样入栈）
        pRetData->m_int = so(h, pArgInf[1].m_int, (curl_off_t)pArgInf[2].m_int64); return;
    case HOP_CurlEasy_SetoptText: case HOP_CurlHttp_SetoptText:
        if (!H_OPT_OK(pArgInf[1].m_int)) { pRetData->m_int = CURLE_UNKNOWN_OPTION; return; }
        pRetData->m_int = so(h, pArgInf[1].m_int, pArgInf[2].m_pText); return;
    case HOP_CurlEasy_SetoptBin: case HOP_CurlHttp_SetoptBin:
        if (!H_OPT_OK(pArgInf[1].m_int)) { pRetData->m_int = CURLE_UNKNOWN_OPTION; return; }
        pRetData->m_int = so(h, pArgInf[1].m_int, pArgInf[2].m_pBin); return;
    case HOP_CurlEasy_Perform: case HOP_CurlHttp_Perform:
        BufReset(&p->body); BufReset(&p->head);
        pRetData->m_int = g_curl.EasyPerform(h); return;
    case HOP_CurlEasy_GetinfoInt: case HOP_CurlHttp_GetinfoInt:
        HGetinfo(h, pArgInf[1].m_int, pRetData, 0); return;
    case HOP_CurlEasy_GetinfoDouble: case HOP_CurlHttp_GetinfoDouble:
        HGetinfo(h, pArgInf[1].m_int, pRetData, 1); return;
    case HOP_CurlEasy_GetinfoText: case HOP_CurlHttp_GetinfoText:
        HGetinfo(h, pArgInf[1].m_int, pRetData, 2); return;
    case HOP_CurlEasy_GetinfoTextAry: case HOP_CurlHttp_GetinfoTextAry:
    {
        curl_slist* pList = NULL;
        const int r = g_curl.EasyGetinfo(h, pArgInf[1].m_int, &pList);
        BufReset(&p->head);
        for (curl_slist* q = pList; q != NULL; q = q->next)
        {
            if (q->data != NULL) BufAppend(&p->head, q->data, (int)lstrlenA(q->data));
            BufAppend(&p->head, "\n", 1);
        }
        HFillTextAry(pArgInf[2], &p->head, '\n');
        pRetData->m_int = r;
        return;
    }
    case HOP_CurlEasy_GetinfoCert: case HOP_CurlHttp_GetinfoCert:
    {
        so(h, CURLOPT_CERTINFO, 1L);
        struct curl_certinfo* ci = NULL;
        const int r = g_curl.EasyGetinfo(h, pArgInf[1].m_int, &ci);
        if (ci != NULL && pArgInf[2].m_pCompoundData != NULL)
        {
            // CURL_证书信息 = { 整数型 数量, 整数型 链表数组 }：数据块是裸整数
            LPINT pField = (LPINT)pArgInf[2].m_pCompoundData;
            pField[0] = ci->num_of_certs;
            pField[1] = (INT)(INT_PTR)ci->certinfo;
        }
        pRetData->m_int = r;
        return;
    }
    case HOP_CurlEasy_Escape: case HOP_CurlHttp_Escape:
    {
        fn_escape_t f = (fn_escape_t)H_PROC("curl_easy_escape");
        char* pOut = f == NULL ? NULL : f(h, pArgInf[1].m_pText, 0);
        SetRetText(pRetData, pOut);
        fn_free_t fr = (fn_free_t)H_PROC("curl_free");
        if (pOut != NULL && fr != NULL) fr(pOut);
        return;
    }
    case HOP_CurlEasy_Unescape: case HOP_CurlHttp_Unescape:
    {
        fn_unescape_t f = (fn_unescape_t)H_PROC("curl_easy_unescape");
        int nLen = 0;
        char* pOut = f == NULL ? NULL : f(h, pArgInf[1].m_pText, 0, &nLen);
        SetRetText(pRetData, pOut);
        fn_free_t fr = (fn_free_t)H_PROC("curl_free");
        if (pOut != NULL && fr != NULL) fr(pOut);
        return;
    }

    // ---- curl_http 便捷设置 ----
    case HOP_CurlHttp_SetXferinfo: case HOP_CurlHttp_SetXferinfoAddr:
    {
        void* pFn = nOp == HOP_CurlHttp_SetXferinfo ? (void*)pArgInf[1].m_int : (void*)pArgInf[1].m_int;
        p->pFn = pFn;
        p->pUser = HIntAt(pArgInf, nArgCount, 2) ? (void*)pArgInf[2].m_int : NULL;
        so(h, CURLOPT_XFERINFOFUNCTION, pFn);
        so(h, CURLOPT_XFERINFODATA, p->pUser);
        so(h, CURLOPT_NOPROGRESS, 0L);
        pRetData->m_int = CURLE_OK;
        return;
    }
    case HOP_CurlHttp_SetUrl:
        pRetData->m_int = so(h, CURLOPT_URL, pArgInf[1].m_pText); return;
    case HOP_CurlHttp_SetUserPwd:
        so(h, CURLOPT_USERNAME, pArgInf[1].m_pText);
        so(h, CURLOPT_PASSWORD, pArgInf[2].m_pText);
        pRetData->m_int = so(h, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY); return;
    case HOP_CurlHttp_SetCookieFile:
        pRetData->m_int = so(h, CURLOPT_COOKIEFILE, pArgInf[1].m_pText); return;
    case HOP_CurlHttp_SetCookieList:
        pRetData->m_int = so(h, CURLOPT_COOKIEJAR, pArgInf[1].m_pText); return;
    case HOP_CurlHttp_SetCookieText:
        pRetData->m_int = so(h, CURLOPT_COOKIE, pArgInf[1].m_pText); return;
    case HOP_CurlHttp_SetPost:
        pRetData->m_int = so(h, CURLOPT_POST, pArgInf[1].m_bool ? 1L : 0L); return;
    case HOP_CurlHttp_SetPostBin:
    {
        // 字节集实参：块头是 [保留][长度]，正文在头之后；拷进对象自有缓冲再交给 curl
        LPBYTE pBin = (LPBYTE)pArgInf[1].m_pBin;
        const INT n = pBin == NULL ? 0 : *(INT*)(pBin + sizeof(INT));
        BufReset(&p->post);
        if (n > 0) BufAppend(&p->post, (LPCSTR)(pBin + sizeof(INT) * 2), n);
        so(h, CURLOPT_POSTFIELDSIZE, (long)p->post.nLen);
        pRetData->m_int = so(h, CURLOPT_POSTFIELDS, p->post.pData);
        return;
    }
    case HOP_CurlHttp_SetPostText:
    {
        BufReset(&p->post);
        if (pArgInf[1].m_pText != NULL) BufAppend(&p->post, pArgInf[1].m_pText, (int)lstrlenA(pArgInf[1].m_pText));
        so(h, CURLOPT_POSTFIELDSIZE, (long)p->post.nLen);
        pRetData->m_int = so(h, CURLOPT_POSTFIELDS, p->post.pData);
        return;
    }
    case HOP_CurlHttp_SetReferer:
        pRetData->m_int = so(h, CURLOPT_REFERER, pArgInf[1].m_pText); return;
    case HOP_CurlHttp_SetBufSize:
        pRetData->m_int = so(h, CURLOPT_BUFFERSIZE, (long)pArgInf[1].m_int); return;
    case HOP_CurlHttp_SetUserAgent:
        pRetData->m_int = so(h, CURLOPT_USERAGENT, pArgInf[1].m_pText); return;
    case HOP_CurlHttp_SetCaInfo:
        pRetData->m_int = so(h, CURLOPT_CAINFO, pArgInf[1].m_pText); return;
    case HOP_CurlHttp_SetHttpVersion:
        pRetData->m_int = so(h, CURLOPT_HTTP_VERSION, (long)pArgInf[1].m_int); return;
    case HOP_CurlHttp_SetProxy:
    {
        // 代理串形如 类型://服务器:端口；用户名非空时把 user:pass@ 插在 :// 之后
        char szBuf[1024], szOut[1280];
        lstrcpynA(szBuf, HTextAt(pArgInf, nArgCount, 1), sizeof(szBuf));
        LPCSTR szUser = HTextAt(pArgInf, nArgCount, 2);
        if (szUser[0] == '\0')
        {
            pRetData->m_int = so(h, CURLOPT_PROXY, szBuf);
            return;
        }
        wsprintfA(szOut, "%s:%s@", szUser, HTextAt(pArgInf, nArgCount, 3));
        LPCSTR sch = strstr(szBuf, "://");
        if (sch == NULL) lstrcatA(szOut, szBuf);
        else
        {
            char szHead[1024];
            const int nHead = (int)(sch - szBuf) + 3;
            memcpy(szHead, szBuf, nHead);
            szHead[nHead] = '\0';
            lstrcatA(szOut, szHead);
            lstrcatA(szOut, sch + 3);
        }
        pRetData->m_int = so(h, CURLOPT_PROXY, szOut);
        return;
    }
    case HOP_CurlHttp_Impersonate:
        pRetData->m_int = g_curl.Impersonate(h, pArgInf[1].m_pText,
                                             (nArgCount > 2 && pArgInf[2].m_bool) ? 1 : 0);
        return;
    case HOP_CurlHttp_SetTimeoutMs:
    {
        int r = CURLE_OK;
        if (HIntAt(pArgInf, nArgCount, 1) > 0) r = so(h, CURLOPT_CONNECTTIMEOUT_MS, (long)pArgInf[1].m_int);
        if (HIntAt(pArgInf, nArgCount, 2) > 0) r = so(h, CURLOPT_TIMEOUT_MS, (long)pArgInf[2].m_int);
        pRetData->m_int = r; return;
    }
    case HOP_CurlHttp_SetTimeout:
    {
        int r = CURLE_OK;
        if (HIntAt(pArgInf, nArgCount, 1) > 0) r = so(h, CURLOPT_CONNECTTIMEOUT, (long)pArgInf[1].m_int);
        if (HIntAt(pArgInf, nArgCount, 2) > 0) r = so(h, CURLOPT_TIMEOUT, (long)pArgInf[2].m_int);
        pRetData->m_int = r; return;
    }
    case HOP_CurlHttp_SetFollow:
    {
        int r = so(h, CURLOPT_FOLLOWLOCATION, pArgInf[1].m_bool ? 1L : 0L);
        const int nMax = HIntAt(pArgInf, nArgCount, 2);
        r = so(h, CURLOPT_MAXREDIRS, nMax > 0 ? (long)nMax : 5L);
        pRetData->m_int = r; return;
    }
    case HOP_CurlHttp_SetKeepAlive:
    {
        int r = so(h, CURLOPT_TCP_KEEPALIVE, pArgInf[1].m_bool ? 1L : 0L);
        const int nIdle = HIntAt(pArgInf, nArgCount, 2);
        if (nIdle > 0) r = so(h, CURLOPT_TCP_KEEPIDLE, (long)nIdle);
        pRetData->m_int = r; return;
    }
    case HOP_CurlHttp_SetEncoding:
    {
        LPCSTR sz = HTextAt(pArgInf, nArgCount, 1);
        pRetData->m_int = so(h, CURLOPT_ACCEPT_ENCODING, sz[0] == '\0' ? NULL : sz);
        return;
    }
    case HOP_CurlHttp_SetVerify:
    {
        int r = so(h, CURLOPT_SSL_VERIFYPEER, pArgInf[1].m_bool ? 1L : 0L);
        r = so(h, CURLOPT_SSL_VERIFYHOST, (nArgCount > 2 && pArgInf[2].m_bool) ? 2L : 0L);
        pRetData->m_int = r; return;
    }
    case HOP_CurlHttp_SetHeaders:
        pRetData->m_int = so(h, CURLOPT_HTTPHEADER, HBuildSlist(p, pArgInf[1])); return;

    // ---- curl_http 取结果 ----
    case HOP_CurlHttp_RespBody:  pRetData->m_pBin = CloneBinData(p->body.pData, p->body.nLen); return;
    case HOP_CurlHttp_RespHead:  SetRetText(pRetData, p->head.pData == NULL ? "" : p->head.pData); return;
    case HOP_CurlHttp_RespHeadBin: pRetData->m_pBin = CloneBinData(p->head.pData, p->head.nLen); return;
    case HOP_CurlHttp_RespCode:  HGetinfo(h, CURLINFO_RESPONSE_CODE, pRetData, 0); return;
    case HOP_CurlHttp_RespTime:  HGetinfo(h, CURLINFO_TOTAL_TIME, pRetData, 1); return;
    case HOP_CurlHttp_RespUrl:   HGetinfo(h, CURLINFO_EFFECTIVE_URL, pRetData, 2); return;
    case HOP_CurlHttp_RespRedirectUrl: HGetinfo(h, CURLINFO_REDIRECT_URL, pRetData, 2); return;
    case HOP_CurlHttp_RespRemoteIp:    HGetinfo(h, CURLINFO_PRIMARY_IP, pRetData, 2); return;
    case HOP_CurlHttp_RespLocalIp:     HGetinfo(h, CURLINFO_LOCAL_IP, pRetData, 2); return;
    case HOP_CurlHttp_RespRemotePort:  HGetinfo(h, CURLINFO_PRIMARY_PORT, pRetData, 0); return;
    case HOP_CurlHttp_RespLocalPort:   HGetinfo(h, CURLINFO_LOCAL_PORT, pRetData, 0); return;
    case HOP_CurlHttp_RespCookies:
    {
        char* pCookie = NULL;
        const int r = g_curl.EasyGetinfo(h, CURLINFO_COOKIELIST, &pCookie);
        BufReset(&p->head);
        if (pCookie != NULL) BufAppend(&p->head, pCookie, (int)lstrlenA(pCookie));
        HFillTextAry(pArgInf[1], &p->head, '\n');
        pRetData->m_int = r;
        return;
    }
    default:
        ReportError("HTTP访问：未知的成员命令");
        return;
    }
}

#endif  // CXHTTP_CORE_INC
