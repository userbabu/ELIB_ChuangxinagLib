// 站点名单与亚马逊网址识别。内置 21 个亚马逊站点，采集程序可用「置站点名单」整体覆盖。
// 识别口径与 EC 侧一致：pageTypes.type 1=搜索结果 2=店铺 3=详情页，colModType 字符串 List/Detail。
#include "bridge.h"

namespace cx {

struct SiteRow { const char* host; const char* platform; const char* display; const char* country; const char* currency; const char* language; };

static const char* AllowedCountry(const char* country)
{
    static const char* allowed[] = { "US", "UK", "CA", "JP", "DE", "FR", "IT", "ES", "MX", "IN", "AU", "AE", "SG", "SA", "TR", "PL", "SE", "NL", "BR", "EG", "BE" };
    for (INT i = 0; i < (INT)(sizeof(allowed) / sizeof(allowed[0])); ++i)
        if (StrEqI(country, allowed[i])) return allowed[i];
    return NULL;
}

// 表内中文是 GBK 字面量，出口一律过 Zh()
static const SiteRow g_builtin[] = {
    { "amazon.com",     "AmazonUs",      "美国站",        "US", "USD", "en" },
    { "amazon.co.uk",   "AmazonUk",      "英国站",        "UK", "GBP", "en" },
    { "amazon.de",      "AmazonDe",      "德国站",        "DE", "EUR", "de" },
    { "amazon.fr",      "AmazonFr",      "法国站",        "FR", "EUR", "fr" },
    { "amazon.it",      "AmazonIt",      "意大利站",      "IT", "EUR", "it" },
    { "amazon.es",      "AmazonEs",      "西班牙站",      "ES", "EUR", "es" },
    { "amazon.nl",      "AmazonNl",      "荷兰站",        "NL", "EUR", "nl" },
    { "amazon.se",      "AmazonSe",      "瑞典站",        "SE", "SEK", "sv" },
    { "amazon.pl",      "AmazonPl",      "波兰站",        "PL", "PLN", "pl" },
    { "amazon.tr",      "AmazonTr",      "土耳其站",      "TR", "TRY", "tr" },
    { "amazon.in",      "AmazonIn",      "印度站",        "IN", "INR", "en" },
    { "amazon.ca",      "AmazonCa",      "加拿大站",      "CA", "CAD", "en" },
    { "amazon.com.mx",  "AmazonMx",      "墨西哥站",      "MX", "MXN", "es" },
    { "amazon.com.br",  "AmazonBr",      "巴西站",        "BR", "BRL", "pt" },
    { "amazon.com.au",  "AmazonAu",      "澳大利亚站",    "AU", "AUD", "en" },
    { "amazon.sg",      "AmazonSg",      "新加坡站",      "SG", "SGD", "en" },
    { "amazon.ae",      "AmazonAe",      "阿联酋站",      "AE", "AED", "ar" },
    { "amazon.sa",      "AmazonSa",      "沙特站",        "SA", "SAR", "ar" },
    { "amazon.eg",      "AmazonEg",      "埃及站",        "EG", "EGP", "ar" },
    { "amazon.co.jp",   "AmazonJp",      "日本站",        "JP", "JPY", "ja" },
    { "amazon.com.be",  "AmazonBe",      "比利时站",      "BE", "EUR", "fr" },
};
#define BUILTIN_SITE_COUNT ((INT)(sizeof(g_builtin) / sizeof(g_builtin[0])))

static BrSite g_sites[64];
static INT    g_nSite = 0;
static INT    g_bLoaded = 0;

static void SiteLoadBuiltin()
{
    g_nSite = 0;
    for (INT i = 0; i < BUILTIN_SITE_COUNT && g_nSite < 64; ++i)
    {
        BrSite* p = &g_sites[g_nSite++];
        ZeroMemory(p, sizeof(*p));
        StrCpyN(p->host, sizeof(p->host), g_builtin[i].host);
        StrCpyN(p->platform, sizeof(p->platform), g_builtin[i].platform);
        StrCpyN(p->display, sizeof(p->display), g_builtin[i].display);
        StrCpyN(p->country, sizeof(p->country), g_builtin[i].country);
        StrCpyN(p->currency, sizeof(p->currency), g_builtin[i].currency);
        StrCpyN(p->language, sizeof(p->language), g_builtin[i].language);
        p->bUtf8 = 0;   // GBK 字面量
    }
}

static void SiteEnsureLoaded()
{
    BrInitOnce();
    EnterCriticalSection(&g_cs);
    if (!g_bLoaded) { SiteLoadBuiltin(); g_bLoaded = 1; }
    LeaveCriticalSection(&g_cs);
}

// 取出一行里以 | 或 , 分隔的字段
static INT SplitFields(char* pszLine, char* apOut[6])
{
    INT n = 0;
    char* p = pszLine;
    apOut[0] = p;
    while (*p != '\0' && n < 5)
    {
        // 竖线/逗号/分号都当分隔符，中文站点名里不会出现
        if (*p == '|' || *p == ',' || *p == ';')
        {
            *p = '\0';
            StrTrim(apOut[n]);
            ++n;
            apOut[n] = p + 1;
        }
        ++p;
    }
    StrTrim(apOut[n]);
    if (apOut[n][0] != '\0') ++n;
    return n;
}

void SiteSetFromText(const char* pszListUtf8)
{
    BrInitOnce();
    SiteEnsureLoaded();
    if (pszListUtf8 == NULL || pszListUtf8[0] == '\0')
    {
        EnterCriticalSection(&g_cs);
        SiteLoadBuiltin();
        LeaveCriticalSection(&g_cs);
        return;
    }

    BrSite tmp[64];
    INT n = 0;
    const INT cch = StrLen(pszListUtf8);
    INT i = 0;
    while (i < cch && n < 64)
    {
        // 逐行拆
        INT j = i;
        while (j < cch && pszListUtf8[j] != '\n') ++j;
        INT len = j - i;
        while (len > 0 && pszListUtf8[i + len - 1] == '\r') --len;
        if (len > 0)
        {
            char line[512];
            if (len > (INT)sizeof(line) - 1) len = (INT)sizeof(line) - 1;
            memcpy(line, pszListUtf8 + i, len);
            line[len] = '\0';
            if (line[0] != '#' && line[0] != '/' && StrLen(line) > 3)
            {
                char* ap[6] = { NULL, NULL, NULL, NULL, NULL, NULL };
                const INT f = SplitFields(line, ap);
                if (f >= 3 && ap[2] != NULL && ap[2][0] != '\0')
                {
                    BrSite* p = &tmp[n];
                    ZeroMemory(p, sizeof(*p));
                    // 顺序：平台名|站点名|域名|国家|货币|语言
                    StrCpyN(p->platform, sizeof(p->platform), ap[0]);
                    StrCpyN(p->display, sizeof(p->display), f >= 2 ? ap[1] : ap[0]);
                    StrCpyN(p->host, sizeof(p->host), ap[2]);
                    StrCpyN(p->country, sizeof(p->country), f >= 4 ? ap[3] : "");
                    StrCpyN(p->currency, sizeof(p->currency), f >= 5 ? ap[4] : "");
                    StrCpyN(p->language, sizeof(p->language), f >= 6 ? ap[5] : "");
                    p->bUtf8 = 1;
                    ++n;
                }
            }
        }
        i = j + 1;
    }

    EnterCriticalSection(&g_cs);
    if (n > 0)
    {
        memcpy(g_sites, tmp, sizeof(BrSite) * n);
        g_nSite = n;
    }
    else SiteLoadBuiltin();
    LeaveCriticalSection(&g_cs);
}

INT SiteCount()
{
    SiteEnsureLoaded();
    return g_nSite;
}

void SiteGet(INT nIndex, BrSite* pOut)
{
    SiteEnsureLoaded();
    EnterCriticalSection(&g_cs);
    if (nIndex >= 0 && nIndex < g_nSite) *pOut = g_sites[nIndex];
    else ZeroMemory(pOut, sizeof(*pOut));
    LeaveCriticalSection(&g_cs);
}

// 站点显示名出口统一成 UTF-8
static const char* SiteDisplay(const BrSite* pSite)
{
    return pSite->bUtf8 ? pSite->display : Zh(pSite->display);
}

// pageTypes 元素：1 搜索结果 2 店铺 3 详情页，与 EC 侧口径一致
static void AddPageType(Buf* b, INT nType, const char* pszTitle)
{
    BufAddCh(b, '{');
    JsonInt(b, "type", nType, FALSE);
    JsonStr(b, "title", pszTitle, TRUE);
    BufAddCh(b, '}');
}

void SiteJson(Buf* bOut)
{
    SiteEnsureLoaded();
    const INT n = SiteCount();
    BOOL bFirst = TRUE;
    for (INT i = 0; i < n; ++i)
    {
        BrSite s;
        SiteGet(i, &s);
        if (s.host[0] == '\0') continue;
        if (!bFirst) BufAddCh(bOut, ',');
        bFirst = FALSE;
        BufAddCh(bOut, '{');
        JsonStr(bOut, "platformName", s.platform, FALSE);
        JsonStr(bOut, "displayName", ToUtf8(SiteDisplay(&s)), TRUE);
        JsonStr(bOut, "host", s.host, TRUE);
        char szHome[128];
        CxSnprintf(szHome, sizeof(szHome) - 1, "https://www.%s/", s.host);
        szHome[sizeof(szHome) - 1] = '\0';
        JsonStr(bOut, "homeUrl", szHome, TRUE);
        JsonStr(bOut, "country", AllowedCountry(s.country) ? AllowedCountry(s.country) : "", TRUE);
        JsonStr(bOut, "currency", s.currency, TRUE);
        JsonStr(bOut, "language", s.language, TRUE);
        JsonKey(bOut, "pageTypes", TRUE);
        BufAddCh(bOut, '[');
        AddPageType(bOut, PT_LIST, Zh("列表页"));
        BufAddCh(bOut, ',');
        AddPageType(bOut, PT_SHOP, Zh("店铺页"));
        BufAddCh(bOut, ',');
        AddPageType(bOut, PT_DETAIL, Zh("详情页"));
        BufAddCh(bOut, ']');
        JsonBool(bOut, "canList", 1, TRUE);
        JsonBool(bOut, "canDetail", 1, TRUE);
        JsonBool(bOut, "auth", 1, TRUE);
        BufAddCh(bOut, '}');
    }
}

// ===================== 网址拆解与识别 =====================

static void HostOfUrl(const char* pszUrl, char* dst, INT cchDst)
{
    dst[0] = '\0';
    const char* p = StrFindI(pszUrl, "://");
    p = (p != NULL) ? p + 3 : pszUrl;
    INT i = 0;
    for (; p[i] != '\0' && i < cchDst - 1; ++i)
    {
        const char c = p[i];
        if (c == '/' || c == '?' || c == '#' || c == ':' || c == ' ' || c == '\t') break;
        dst[i] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
    }
    dst[i] = '\0';
    if (StrFindI(dst, "www.") == dst) StrCpyN(dst, cchDst, dst + 4);
}

static void PathOfUrl(const char* pszUrl, char* dst, INT cchDst)
{
    dst[0] = '\0';
    const char* p = StrFindI(pszUrl, "://");
    p = (p != NULL) ? p + 3 : pszUrl;
    while (*p != '\0' && *p != '/') ++p;
    INT i = 0;
    for (; p[i] != '\0' && i < cchDst - 1; ++i) dst[i] = p[i];
    dst[i] = '\0';
}

// 从 /dp/XXXX 这类路径里取 ASIN（10 位大写字母数字）
static BOOL AsinFromPath(const char* pszPath, char* dst)
{
    dst[0] = '\0';
    static const char* apMarks[] = { "/dp/", "/product/", "/gp/product/", "/gp/aw/d/", "/gp/aip/" };
    for (INT k = 0; k < 5; ++k)
    {
        const char* p = StrFindI(pszPath, apMarks[k]);
        if (p == NULL) continue;
        p += StrLen(apMarks[k]);
        INT n = 0;
        while (n < 10 && p[n] != '\0')
        {
            const char c = p[n];
            if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) break;
            dst[n] = c;
            ++n;
        }
        dst[n] = '\0';
        // 10 位才是 ASIN，短了说明撞上了 /dp/ 后面的别的路径
        if (n == 10) return TRUE;
    }
    return FALSE;
}

static void MatchHost(const char* pszHost, BrSite* pOut, INT* pnIndex)
{
    SiteEnsureLoaded();
    const INT n = SiteCount();
    for (INT i = 0; i < n; ++i)
    {
        BrSite s;
        SiteGet(i, &s);
        if (s.host[0] == '\0') continue;
        if (StrEqI(pszHost, s.host))
        {
            *pOut = s; *pnIndex = i; return;
        }
        // 只认主域名和它的子域，别把 evil.com 认成 amazon.com
        const INT nHost = StrLen(pszHost), nSite = StrLen(s.host);
        if (nHost > nSite + 1 && pszHost[nHost - nSite - 1] == '.' &&
            StrNCmpI(pszHost + nHost - nSite, s.host, nSite))
        {
            *pOut = s; *pnIndex = i; return;
        }
    }
    ZeroMemory(pOut, sizeof(*pOut));
    *pnIndex = -1;
}

void MatchUrl(const char* pszUrl, BrMatch* pOut)
{
    ZeroMemory(pOut, sizeof(*pOut));
    if (pszUrl == NULL || pszUrl[0] == '\0') return;

    char host[128], path[1024];
    HostOfUrl(pszUrl, host, sizeof(host));
    PathOfUrl(pszUrl, path, sizeof(path));
    StrCpyN(pOut->host, sizeof(pOut->host), host);

    BrSite site;
    INT nIdx = -1;
    MatchHost(host, &site, &nIdx);
    if (nIdx < 0) return;   // 不在站点名单里 = 认不出

    pOut->bOk = TRUE;
    StrCpyN(pOut->platform, sizeof(pOut->platform), site.platform);
    StrCpyN(pOut->display, sizeof(pOut->display), SiteDisplay(&site));
    const char* country = AllowedCountry(site.country);
    if (country) StrCpyN(pOut->country, sizeof(pOut->country), country);
    StrCpyN(pOut->currency, sizeof(pOut->currency), site.currency);
    StrCpyN(pOut->language, sizeof(pOut->language), site.language);

    // 1 详情页：路径里有 ASIN
    if (AsinFromPath(path, pOut->asin))
    {
        pOut->bIdentifiable = TRUE;
        pOut->nPageType = PT_DETAIL;
        pOut->nColMode = CM_DETAIL;
        return;
    }
    // 2 列表页：搜索结果 / 分类浏览
    const char* p = path;
    while (*p == '/') ++p;
    if (StrNCmpI(p, "s?", 2) || StrEqI(p, "s") || StrNCmpI(p, "s/", 2) ||
        StrNCmpI(p, "b?", 2) || StrNCmpI(p, "b/", 2) || StrNCmpI(p, "bp/", 3) ||
        StrNCmpI(p, "n/", 2) || StrHasI(path, "/s?k=") || StrHasI(path, "keywords=") ||
        StrHasI(path, "/search?"))
    {
        pOut->bIdentifiable = TRUE;
        pOut->nPageType = PT_LIST;
        pOut->nColMode = CM_LIST;
        return;
    }
    // 3 店铺页按列表处理
    if (StrHasI(path, "/stores/") || StrNCmpI(p, "sp/", 3) || StrHasI(path, "/shop/"))
    {
        pOut->bIdentifiable = TRUE;
        pOut->nPageType = PT_SHOP;
        pOut->nColMode = CM_LIST;
        return;
    }
    // 首页与其余页面不算可采
}

}  // namespace cx
