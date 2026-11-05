// 引擎级冒烟测试：直接调 cx:: 的解析/序列化/路径函数，逐条比对 zyJson2.51 的行为口径。
// 编译运行见 run_engine_test.bat（不经过易语言，纯 C++）。
#include <cx_json.h>
#include <stdio.h>

using namespace cx;

static int g_fail = 0;
static int g_pass = 0;

static WCHAR* W(LPCSTR szA)   // 字面量已被 /execution-charset:gbk 落成 GBK，按 ACP 转宽
{
    INT n = (INT)lstrlenA(szA);
    INT nWide = MultiByteToWideChar(CP_ACP, 0, szA, n, NULL, 0);
    WCHAR* p = (WCHAR*)JvMalloc((SIZE_T)(nWide + 1) * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, szA, n, p, nWide);
    p[nWide] = 0;
    return p;
}

static void PrintW(const WCHAR* p)
{
    if (p == NULL) { printf("(null)"); return; }
    INT n = JvWideLen(p);
    for (INT i = 0; i < n; i++)
    {
        WCHAR c = p[i];
        if (c == 13) printf("\\r");
        else if (c == 10) printf("\\n");
        else if (c == 9) printf("\\t");
        else if (c < 32 || c > 126) printf("\\u%04X", (unsigned)c);
        else printf("%c", (char)c);
    }
}

static void Check(const char* szName, const WCHAR* pGot, const char* szWant)
{
    CHAR szA[4096];
    INT n = (pGot == NULL) ? 0 : WideCharToMultiByte(CP_ACP, 0, pGot, JvWideLen(pGot), szA, 4000, NULL, NULL);
    if (n < 0) n = 0;
    szA[n] = '\0';
    if (pGot != NULL && lstrcmpA(szA, szWant) == 0) { g_pass++; return; }
    g_fail++;
    printf("[FAIL] %s\n       want: %s\n       got : ", szName, szWant);
    PrintW(pGot);
    printf("\n");
}

static void CheckW(const char* szName, const WCHAR* pGot, const WCHAR* pWant)
{
    if (pGot != NULL && pWant != NULL && JvWideCmp(pGot, pWant) == 0) { g_pass++; return; }
    g_fail++;
    printf("[FAIL] %s\n", szName);
    printf("       want: "); PrintW(pWant); printf("\n");
    printf("       got : "); PrintW(pGot); printf("\n");
}

static void CheckI(const char* szName, INT nGot, INT nWant)
{
    if (nGot == nWant) { g_pass++; return; }
    g_fail++;
    printf("[FAIL] %s want=%d got=%d\n", szName, nWant, nGot);
}

static WCHAR* ToString(CxJsonNode* n, INT bFmt = 0, INT bTrans = 1)
{
    UniWBuf buf;
    JvToString(n, &buf, bFmt, bTrans);
    WCHAR* p = JvWideDup(buf.p, buf.nLen);
    return p;
}

static CxJsonNode* Parse(const char* szJson, INT* pErr = NULL, INT bTransU = 1)
{
    WCHAR* pSrc = W(szJson);
    INT nLen = JvWideLen(pSrc);
    CxJsonNode* root = JvNewObject();          // 与 置代码 一致：先建空对象再解析
    INT errPos = 0;
    INT err = JvParse(root, pSrc, nLen, bTransU, &errPos);
    if (pErr != NULL) *pErr = err;
    JvFree(pSrc);
    if (err != CXJSON_OK && pErr == NULL)
    {
        printf("[FAIL] 解析失败: %s → err=%d pos=%d\n", szJson, err, errPos);
        g_fail++;
    }
    return root;
}

// 类层 置文本(path, text) 的等价实现：SelectW(创建) + SetString
static void SetPathText(CxJsonNode* root, const char* szPath, const char* szText)
{
    WCHAR* pPath = W(szPath);
    CxJsonNode* jv = JvSelect(root, pPath, 1);
    WCHAR* pVal = W(szText);
    JvSetString(jv, pVal, -1);
    JvFree(pPath);
    JvFree(pVal);
}

int main()
{
    INT err = 0;

    // 1) 回写一致：解析→到文本 与原输入逐字符相同
    {
        static const char* cases[] = {
            "{\"a\":1,\"b\":[1,2,{\"c\":\"x\"}],\"d\":{\"e\":true}}",
            "{\"z\":1,\"a\":2}",
            "[]", "{}", "[1,2,3]",
            "{\"n\":null,\"f\":false,\"t\":true}",
            "{\"a\":[]}", "{\"a\":{}}",            "{\"a\":1.50}", "{\"a\":-0}", "{\"a\":+5}", "{\"a\":007}", "{\"a\":1e5}", "{\"a\":1E+5}",
            "[\"a\",\"b\"]",
        };
        for (INT i = 0; i < (INT)(sizeof(cases) / sizeof(cases[0])); i++)
        {
            CxJsonNode* n = Parse(cases[i]);
            WCHAR* p = ToString(n);
            Check(cases[i], p, cases[i]);
            JvFree(p);
            JvRelease(n);
        }
    }

    // 2) 输出侧转义：/ 恒被转义，控制字符集合内才转
    {
        CxJsonNode* n = Parse("{\"a\":\"x/y\\\"z\\\\w\"}");
        WCHAR* p = ToString(n);
        Check("escape slash", p, "{\"a\":\"x\\/y\\\"z\\\\w\"}");
        JvFree(p); JvRelease(n);
    }
    {
        CxJsonNode* n = Parse("{\"a\":\"p\\nq\"}");
        WCHAR* p = ToString(n);
        Check("slash-n becomes crlf", p, "{\"a\":\"p\\r\\nq\"}");
        JvFree(p); JvRelease(n);
    }
    {
        // 未解码的 \u 原样保留，输出时反斜杠再被转义
        INT errPos = 0;
        WCHAR* src = W("{\"a\":\"\\u4e2d\"}");
        CxJsonNode* n = JvNewObject();
        JvParse(n, src, JvWideLen(src), 0, &errPos);
        WCHAR* p = ToString(n);
        Check("u escape kept", p, "{\"a\":\"\\\\u4e2d\"}");
        JvFree(p); JvRelease(n); JvFree(src);
    }
    {
        INT errPos = 0;
        WCHAR* src = W("{\"a\":\"\\u4e2d\"}");
        CxJsonNode* n = JvNewObject();
        JvParse(n, src, JvWideLen(src), 1, &errPos);
        WCHAR* p = ToString(n);
        Check("u decoded", p, "{\"a\":\"中\"}");
        JvFree(p); JvRelease(n); JvFree(src);
    }

    // 3) 宽松语法
    {
        CxJsonNode* n = Parse("{\"a\":1,}");
        WCHAR* p = ToString(n);
        Check("trailing comma object", p, "{\"a\":1}");
        JvFree(p); JvRelease(n);
    }
    {
        CxJsonNode* n = Parse("[1,2,]");
        WCHAR* p = ToString(n);
        Check("trailing comma array", p, "[1,2]");
        JvFree(p); JvRelease(n);
    }
    {
        CxJsonNode* n = Parse("{a:'b'}");
        WCHAR* p = ToString(n);
        Check("unquoted key + single quote", p, "{\"a\":\"b\"}");
        JvFree(p); JvRelease(n);
    }

    // 4) 键序与覆盖：新键追加、同名原位替换且键名以首次为准
    {
        CxJsonNode* n = JvNewObject();
        WCHAR* ka = W("a"), *kB = W("b");
        JvChildSet(n, ka, JvNewLong(1, NULL));
        JvChildSet(n, kB, JvNewLong(2, NULL));
        JvChildSet(n, ka, JvNewLong(3, NULL));
        WCHAR* p = ToString(n);
        Check("overwrite keeps position", p, "{\"a\":3,\"b\":2}");
        JvFree(p); JvFree(ka); JvFree(kB); JvRelease(n);
    }

    // 5) 根标量与类型打标
    {
        INT errPos = 0;
        WCHAR* s1 = W("123");
        CxJsonNode* n = JvNewObject();
        CheckI("root scalar ok", JvParse(n, s1, 3, 1, &errPos), CXJSON_OK);
        CheckI("root int is LONG", JvTypeOf(n), CXJV_LONG);
        JvFree(s1); JvRelease(n);
    }
    {
        CxJsonNode* n = Parse("{\"a\":123}");
        CxJsonNode* c = JvChildReadOnly(n, W("a"));
        CheckI("child int stored as DOUBLE", JvTypeOf(c), CXJV_DOUBLE);
        CheckI("child value", (INT)JvGetLong(c), 123);
        JvRelease(n);
    }

    // 6) 路径自动创建 / 越界补齐
    {
        CxJsonNode* n = JvNewObject();
        SetPathText(n, "a.b[0].c", "x");
        WCHAR* p = ToString(n);
        Check("path auto create", p, "{\"a\":{\"b\":[{\"c\":\"x\"}]}}");
        JvFree(p); JvRelease(n);
    }
    {
        CxJsonNode* n = JvNewObject();
        WCHAR* k = W("miss");
        CxJsonNode* got = JvChildOrCreate(n, k);
        JvFree(k);
        WCHAR* p = ToString(n);
        Check("read creates null member", p, "{\"miss\":null}");
        JvFree(p); JvRelease(n); UNREFERENCED_PARAMETER(got);
    }
    {
        CxJsonNode* n = Parse("[1]");
        CxJsonNode* got = JvAtOrCreate(n, 3);
        CheckI("out of range pads", JvCount(n), 4);
        CheckI("padded is null", JvTypeOf(got), CXJV_NULL);
        WCHAR* p = ToString(n);
        Check("padded text", p, "[1,null,null,null]");
        JvFree(p); JvRelease(n);
    }

    // 7) 空路径 = 当前节点；是否存在空路径 = 假
    {
        CxJsonNode* n = Parse("{\"a\":1}");
        CheckI("empty path selects self", JvSelect(n, NULL, 1) == n, 1);
        CheckI("query empty path false", JvQuery(n, L"") ? 1 : 0, 0);
        WCHAR* k = W("a");
        CheckI("query exists", JvQuery(n, k) ? 1 : 0, 1);
        WCHAR* k2 = W("b");
        CheckI("query missing", JvQuery(n, k2) ? 1 : 0, 0);
        CheckI("query does not create", JvCount(n), 1);
        JvFree(k); JvFree(k2); JvRelease(n);
    }

    // 8) 错误码
    {
        INT got = 0; Parse(" null", &got);        CheckI("lead space null invalid", got, CXJSON_INVALIDVALUE);
        got = 0; Parse("{\"a\":}", &got);         CheckI("missing value invalid", got, CXJSON_INVALIDVALUE);
        got = 0; Parse("{\"a\"}", &got);          CheckI("missing colon invalid", got, CXJSON_INVALIDVALUE);
        got = 0; Parse("{\"a\":1", &got);         CheckI("unclosed object parent", got, CXJSON_OBJECTPARENT);
        got = 0; Parse("[1", &got);               CheckI("unclosed array parent", got, CXJSON_OBJECTPARENT);
        got = 0; Parse("{\"a\":NaN}", &got);      CheckI("NaN invalid", got, CXJSON_INVALIDVALUE);
        got = 0; Parse("{\"a\":1.2.3}", &got);    CheckI("two dots invalid", got, CXJSON_INVALIDVALUE);
    }

    // 9) 格式化输出：CRLF + Tab 缩进，数组永远紧凑
    {
        CxJsonNode* n = Parse("{\"name\":\"te\\\"st/行\",\"arr\":[1,2],\"obj\":{\"k\":\"v\"}}");
        WCHAR* p = ToString(n, 1, 1);
        static const WCHAR want[] = {
            '{',13,10,9,'"','n','a','m','e','"',':','"','t','e','\\','"','s','t','\\','/',0x884C,'"',',',13,10,
            9,'"','a','r','r','"',':','[','1',',','2',']',',',13,10,
            9,'"','o','b','j','"',':','{',13,10,9,9,'"','k','"',':','"','v','"',13,10,9,'}',13,10,'}',0
        };
        CheckW("auto format", p, want);
        JvFree(p); JvRelease(n);
    }

    // 10) 数字 → 文本（程序化 setter 的 raw 来源）
    {
        WCHAR sz[64];
        JvDoubleToText(1.0, sz, 64);   Check("double 1", sz, "1");
        JvDoubleToText(1.5, sz, 64);   Check("double 1.5", sz, "1.5");
        JvDoubleToText(1.0 / 3.0, sz, 64); Check("double 1/3", sz, "0.333333333333333");
        JvDoubleToText(1.25e20, sz, 64);   Check("double 1.25e20", sz, "1.25e+20");
        JvDoubleToText(0.0001, sz, 64);    Check("double 0.0001", sz, "0.0001");
        JvDoubleToText(0.00001, sz, 64);   Check("double 1e-5", sz, "1e-05");
        JvDoubleToText(-0.0, sz, 64);      Check("double -0", sz, "-0");
        JvDoubleToText(9007199254740993.0, sz, 64); Check("double 2^53+1", sz, "9.00719925474099e+15");
        __int64 l = 1234567890123456780i64;
        JvInt64ToText(l, sz, 64);      Check("i64", (const WCHAR*)sz, "1234567890123456780");
    }

    // 11) 强转矩阵抽样
    {
        CxJsonNode* n = Parse("{\"s\":\"78\",\"t\":\"abc\",\"d\":1.75,\"l\":-5,\"b\":true,\"z\":null,\"o\":{},\"a\":[]}");
        WCHAR* ks = W("s"), *kt = W("t"), *kd = W("d"), *kl = W("l"), *kb = W("b"), *kz = W("z"), *ko = W("o");
        CheckI("string to int", JvGetInt(JvChildReadOnly(n, ks)), 78);
        CheckI("string to double", (INT)JvGetDouble(JvChildReadOnly(n, ks)), 78);
        CheckI("text abc to int", JvGetInt(JvChildReadOnly(n, kt)), 0);
        CheckI("double to int trunc", JvGetInt(JvChildReadOnly(n, kd)), 1);
        CheckI("long to double", (INT)JvGetDouble(JvChildReadOnly(n, kl)), -5);
        CheckI("bool to long", (INT)JvGetLong(JvChildReadOnly(n, kb)), 1);
        CheckI("null to bool", JvGetBool(JvChildReadOnly(n, kz)) ? 1 : 0, 0);
        CheckI("object type", JvTypeOf(JvChildReadOnly(n, ko)), CXJV_OBJECT);
        WCHAR* ex = JvGetStringEx(JvChildReadOnly(n, kz));
        CheckI("null stringex is null literal", (ex != NULL && ex[0] == 'n') ? 1 : 0, 1);
        JvFree(ex);
        JvFree(ks); JvFree(kt); JvFree(kd); JvFree(kl); JvFree(kb); JvFree(kz); JvFree(ko);
        JvRelease(n);
    }

    // 12) 合并：同名递归、缺失键追加、新键排在末尾
    {
        CxJsonNode* dst = Parse("{\"a\":1,\"b\":{\"c\":2}}");
        WCHAR* text = W("{\"b\":{\"c\":9,\"d\":8},\"e\":7}");
        INT errPos = 0;
        CxJsonNode* desc = JvNewObject();
        JvParse(desc, text, JvWideLen(text), 1, &errPos);
        JvRetain(desc);
        JvCombineProc(dst, desc, 1);
        JvRelease(desc);
        WCHAR* p = ToString(dst);
        Check("combine recursive", p, "{\"a\":1,\"b\":{\"c\":9,\"d\":8},\"e\":7}");
        JvFree(p); JvRelease(dst); JvFree(text);
    }

    // 13) 排序对象：按属性名码元升/降序
    {
        CxJsonNode* n = Parse("{\"b\":1,\"A\":2,\"c\":3}");
        JvSortObject(n, 1);
        WCHAR* p = ToString(n);
        Check("sort object asc", p, "{\"A\":2,\"b\":1,\"c\":3}");
        JvFree(p);
        JvSortObject(n, 0);
        p = ToString(n);
        Check("sort object desc", p, "{\"c\":3,\"b\":1,\"A\":2}");
        JvFree(p); JvRelease(n);
    }

    // 14) 深拷贝不带父、数字重新格式化
    {
        CxJsonNode* n = Parse("{\"a\":{\"b\":[1,2]}}");
        CxJsonNode* c = JvClone(n);
        WCHAR* p = ToString(c);
        Check("clone text", p, "{\"a\":{\"b\":[1,2]}}");
        JvFree(p);
        CheckI("clone has no parent", JvGetParent(c) == NULL ? 1 : 0, 1);
        JvRelease(c);
        CxJsonNode* num = Parse("{\"x\":1.50}");
        CxJsonNode* nc = JvClone(num);
        p = ToString(nc);
        Check("clone re-formats number raw", p, "{\"x\":1.5}");
        JvFree(p);
        JvRelease(nc); JvRelease(num); JvRelease(n);
    }

    // 15) 多成员对象：桶扩容后查找不能死循环（曾把 JvObjFind 挂住）
    {
        CxJsonNode* n = JvNewObject();
        for (int i = 0; i < 40; i++)
        {
            WCHAR szNum[8], name[16];
            JvIntToText(i, szNum, 8);
            name[0] = 'k';
            INT j = 0;
            while (szNum[j]) { name[1 + j] = szNum[j]; j++; }
            name[1 + j] = 0;
            JvChildSet(n, name, JvNewLong(i, NULL));
        }
        CheckI("扩容后成员数", JvCount(n), 40);
        WCHAR q[16];
        lstrcpyW(q, L"k37");
        CheckI("扩容后查得到", JvTypeOf(JvChildReadOnly(n, q)), CXJV_LONG);
        lstrcpyW(q, L"k999");
        CheckI("扩容后查不到为空", (JvChildReadOnly(n, q) == NULL) ? 1 : 0, 1);
        JvRelease(n);
    }

    printf("%s  pass=%d fail=%d\n", g_fail == 0 ? "[OK] engine smoke test" : "[ERROR] engine smoke test", g_pass, g_fail);
    UNREFERENCED_PARAMETER(err);
    return g_fail == 0 ? 0 : 1;
}
