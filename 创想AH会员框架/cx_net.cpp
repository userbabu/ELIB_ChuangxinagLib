// 创想AH会员框架 · 文本 / JSON / 加解密 / HTTP / 接口层
// 非系统核心 DLL（bcrypt、winhttp、advapi32）一律运行期解析，静态编译不额外依赖。
#include "cx_core.h"

#include <winhttp.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// 这两个导入库只给动态 .fne 用：易语言自带的 VC6 链接器目录里没有 winhttp.lib/bcrypt.lib，
// /DEFAULTLIB 会直接 LNK1104。静态分支一律运行期 LoadLibrary 解析。
#ifndef __E_STATIC_LIB
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "bcrypt.lib")
#endif  // __E_STATIC_LIB

namespace cx {

// ==================== 文本 ====================

std::string Utf8FromWide(const std::wstring& w)
{
    if (w.empty()) return std::string();
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
    std::string out((size_t)(n > 0 ? n : 0), '\0');
    if (n <= 0) return std::string();
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &out[0], n, NULL, NULL);
    return out;
}

std::wstring WideFromUtf8(const std::string& s)
{
    if (s.empty()) return std::wstring();
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    if (n <= 0) return std::wstring();
    std::wstring out((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], n);
    return out;
}

std::string GbkToUtf8(const std::string& gbk)
{
    if (gbk.empty()) return std::string();
    const int nw = MultiByteToWideChar(CP_ACP, 0, gbk.c_str(), (int)gbk.size(), NULL, 0);
    if (nw <= 0) return gbk;
    std::wstring w((size_t)nw, L'\0');
    MultiByteToWideChar(CP_ACP, 0, gbk.c_str(), (int)gbk.size(), &w[0], nw);
    return Utf8FromWide(w);
}

std::string ToStr(int v)
{
    char buf[24];
    wsprintfA(buf, "%d", v);
    return std::string(buf);
}

std::string I64ToText(long long v)
{
    unsigned long long u = (v < 0) ? (0ULL - (unsigned long long)v) : (unsigned long long)v;
    char buf[24];
    int n = 0;
    do { buf[n++] = (char)('0' + (int)(u % 10ULL)); u /= 10ULL; } while (u != 0ULL);
    if (v < 0) buf[n++] = '-';
    buf[n] = 0;
    for (int a = 0, b = n - 1; a < b; ++a, --b) { const char t = buf[a]; buf[a] = buf[b]; buf[b] = t; }
    return std::string(buf);
}

long long ParseI64(const char* p)
{
    if (p == NULL) return 0;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
    bool neg = false;
    if (*p == '-') { neg = true; ++p; }
    else if (*p == '+') ++p;
    unsigned long long u = 0;
    for (; *p >= '0' && *p <= '9'; ++p) u = u * 10ULL + (unsigned long long)(*p - '0');
    return neg ? (long long)(0ULL - u) : (long long)u;
}

// 向零截断，逐级降到 32 位转换（double→int32 是内联 cvttsd2si），避开 __dtol3。
long long DoubleToI64(double d)
{
    if (d != d) return 0;
    if (!(d < 9223372036854775807.0)) return (long long)0x7fffffffffffffffui64;
    if (d < -9223372036854775808.0) return (long long)0x8000000000000000ui64;
    const bool neg = (d < 0.0);
    if (neg) d = -d;
    long long r;
    if (d >= 4294967296.0) {
        const int nHi = (int)(d / 4294967296.0);
        const double dRest = d - (double)nHi * 4294967296.0;
        const int nLo = (int)(dRest - 2147483648.0);
        r = ((long long)nHi << 32) + ((long long)nLo + 2147483648i64);
    } else {
        r = (long long)(int)d;
    }
    return neg ? -r : r;
}

// 长整数 -> 双精度：拆成高低 32 位，避开 VC6 没有的 __ltod3。
static double I64ToDouble(long long v)
{
    const int nHi = (int)(v >> 32);
    const unsigned int uLo = (unsigned int)(v & 0xffffffffi64);
    return (double)nHi * 4294967296.0 + (double)uLo;
}

// %.10g 的替身：整数走精确通道，小数最多 10 位有效数字并去掉尾零。
std::string NumToText(double d)
{
    if (d != d || d > 1e308 || d < -1e308) return "0";
    const bool neg = (d < 0.0);
    const double a = neg ? -d : d;
    const long long i = DoubleToI64(a);
    const double asD = I64ToDouble(i);
    if (asD == a) return neg ? "-" + I64ToText(i) : I64ToText(i);
    std::string s = I64ToText(i);
    double frac = a - asD;
    s += '.';
    for (int n = 0; n < 10; ++n) {
        frac *= 10.0;
        int dg = (int)frac;
        if (dg > 9) dg = 9;
        s += (char)('0' + dg);
        frac -= dg;
        if (frac <= 0.0) break;
    }
    while (!s.empty() && s[s.size() - 1] == '0') s.erase(s.size() - 1);
    if (!s.empty() && s[s.size() - 1] == '.') s.erase(s.size() - 1);
    return neg ? "-" + s : s;
}

std::string TrimStr(const std::string& s)
{
    size_t a = 0, b = s.size();
    while (a < b && (unsigned char)s[a] <= ' ') ++a;
    while (b > a && (unsigned char)s[b - 1] <= ' ') --b;
    return s.substr(a, b - a);
}

std::string LowerStr(const std::string& in)
{
    std::string s = in;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] >= 'A' && s[i] <= 'Z') s[i] = (char)(s[i] + 32);
    }
    return s;
}

bool StartsWith(const std::string& s, const char* head)
{
    const size_t n = strlen(head);
    return s.size() >= n && memcmp(s.data(), head, n) == 0;
}

static const char kB64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string Base64Encode(const unsigned char* p, size_t n)
{
    std::string out;
    out.reserve((n + 2) / 3 * 4);
    size_t i = 0;
    while (i + 2 < n) {
        unsigned v = ((unsigned)p[i] << 16) | ((unsigned)p[i + 1] << 8) | p[i + 2];
        out += kB64[(v >> 18) & 63]; out += kB64[(v >> 12) & 63];
        out += kB64[(v >> 6) & 63];  out += kB64[v & 63];
        i += 3;
    }
    if (i < n) {
        unsigned v = (unsigned)p[i] << 16;
        if (i + 1 < n) v |= (unsigned)p[i + 1] << 8;
        out += kB64[(v >> 18) & 63];
        out += kB64[(v >> 12) & 63];
        out += (i + 1 < n) ? kB64[(v >> 6) & 63] : '=';
        out += '=';
    }
    return out;
}

std::string Base64EncodeStr(const std::string& s)
{
    return Base64Encode((const unsigned char*)s.data(), s.size());
}

std::vector<unsigned char> Base64Decode(const std::string& text)
{
    std::vector<unsigned char> out;
    int buf = 0, bits = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '=' || c == '\r' || c == '\n' || c == ' ') continue;
        const char* pos = strchr(kB64, c);
        if (pos == NULL) continue;
        buf = (buf << 6) | (int)(pos - kB64);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back((unsigned char)((buf >> bits) & 0xFF));
        }
    }
    return out;
}

void LogLine(const std::string& text)
{
    if (!S().debug) return;
    char buf[64];
    wsprintfA(buf, "[%s] ", NowBeijingText().c_str());
    OutputDebugStringA((std::string(buf) + text + "\r\n").c_str());
}

// ==================== JSON ====================

const JVal* JVal::find(const std::string& key) const
{
    if (type != TObj) return NULL;
    for (size_t i = 0; i < obj.size(); ++i) {
        if (obj[i].first == key) return &obj[i].second;
    }
    return NULL;
}

std::string JVal::asStr(const char* def) const
{
    if (type == TStr) return str;
    if (type == TNum) return numText.empty() ? NumToText(num) : numText;
    if (type == TBool) return bval ? "1" : "0";
    return def;
}

long long JVal::asInt64(long long def) const
{
    if (type == TNum) return numText.empty() ? DoubleToI64(num) : ParseI64(numText.c_str());
    if (type == TStr) return ParseI64(str.c_str());
    if (type == TBool) return bval ? 1 : 0;
    return def;
}

int JVal::asInt(int def) const
{
    if (type == TNum) return (int)num;
    if (type == TStr) return atoi(str.c_str());
    if (type == TBool) return bval ? 1 : 0;
    return def;
}

bool JVal::asBool(bool def) const
{
    if (type == TBool) return bval;
    if (type == TNum) return num != 0;
    if (type == TStr) return str == "1" || str == "true";
    return def;
}

namespace {

struct JsonReader {
    const std::string& s;
    size_t i;
    bool bad;
    JsonReader(const std::string& text) : s(text), i(0), bad(false) {}

    void skip()
    {
        while (i < s.size() && (unsigned char)s[i] <= ' ') ++i;
    }
    bool parseVal(JVal& v)
    {
        skip();
        if (i >= s.size()) { bad = true; return false; }
        const char c = s[i];
        if (c == '{') return parseObj(v);
        if (c == '[') return parseArr(v);
        if (c == '"') { v.type = JVal::TStr; v.str = parseStr(); return !bad; }
        if (c == 't' || c == 'f') {
            v.type = JVal::TBool;
            v.bval = s.compare(i, 4, "true") == 0;
            i += v.bval ? 4 : 5;
            return true;
        }
        if (c == 'n') { v.type = JVal::TNull; i += 4; return true; }
        v.type = JVal::TNum;
        size_t start = i;
        while (i < s.size() && (isdigit((unsigned char)s[i]) || s[i] == '-' || s[i] == '+' || s[i] == '.' || s[i] == 'e' || s[i] == 'E')) ++i;
        v.numText = s.substr(start, i - start);
        v.num = atof(v.numText.c_str());
        return i > start;
    }
    bool parseObj(JVal& v)
    {
        v.type = JVal::TObj;
        ++i;  // {
        skip();
        if (i < s.size() && s[i] == '}') { ++i; return true; }
        for (;;) {
            skip();
            if (i >= s.size() || s[i] != '"') { bad = true; return false; }
            std::string key = parseStr();
            skip();
            if (i >= s.size() || s[i] != ':') { bad = true; return false; }
            ++i;
            JVal item;
            if (!parseVal(item)) return false;
            v.obj.push_back(std::make_pair(key, item));
            skip();
            if (i < s.size() && s[i] == ',') { ++i; continue; }
            if (i < s.size() && s[i] == '}') { ++i; return true; }
            bad = true;
            return false;
        }
    }
    bool parseArr(JVal& v)
    {
        v.type = JVal::TArr;
        ++i;  // [
        skip();
        if (i < s.size() && s[i] == ']') { ++i; return true; }
        for (;;) {
            JVal item;
            if (!parseVal(item)) return false;
            v.arr.push_back(item);
            skip();
            if (i < s.size() && s[i] == ',') { ++i; continue; }
            if (i < s.size() && s[i] == ']') { ++i; return true; }
            bad = true;
            return false;
        }
    }
    std::string parseStr()
    {
        std::string out;
        if (i >= s.size() || s[i] != '"') { bad = true; return out; }
        ++i;
        while (i < s.size()) {
            const char c = s[i];
            if (c == '"') { ++i; return out; }
            if (c != '\\') { out += c; ++i; continue; }
            if (i + 1 >= s.size()) break;
            const char e = s[i + 1];
            i += 2;
            switch (e) {
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case '/': out += '/'; break;
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case 'u': {
                if (i + 4 > s.size()) { bad = true; return out; }
                unsigned code = (unsigned)strtoul(s.substr(i, 4).c_str(), NULL, 16);
                i += 4;
                if (code < 0x80) out += (char)code;
                else if (code < 0x800) {
                    out += (char)(0xC0 | (code >> 6));
                    out += (char)(0x80 | (code & 0x3F));
                } else {
                    out += (char)(0xE0 | (code >> 12));
                    out += (char)(0x80 | ((code >> 6) & 0x3F));
                    out += (char)(0x80 | (code & 0x3F));
                }
                break;
            }
            default: bad = true; return out;
            }
        }
        bad = true;
        return out;
    }
};

}  // namespace

static void JDumpEsc(const std::string& raw, std::string& out)
{
    out += '"';
    out += JsonEscape(raw);
    out += '"';
}

std::string JVal::dump() const
{
    std::string out;
    switch (type) {
    case TNull: out = "null"; break;
    case TBool: out = bval ? "true" : "false"; break;
    case TNum: out = numText.empty() ? NumToText(num) : numText; break;
    case TStr: JDumpEsc(str, out); break;
    case TArr: {
        out += "[";
        for (size_t i = 0; i < arr.size(); ++i) {
            if (i) out += ",";
            out += arr[i].dump();
        }
        out += "]";
        break;
    }
    case TObj: {
        out += "{";
        for (size_t i = 0; i < obj.size(); ++i) {
            if (i) out += ",";
            JDumpEsc(obj[i].first, out);
            out += ":";
            out += obj[i].second.dump();
        }
        out += "}";
        break;
    }
    }
    return out;
}

bool JsonParse(const std::string& text, JVal& out)
{
    JsonReader r(text);
    const bool ok = r.parseVal(out);
    return ok && !r.bad;
}

std::string JsonEscape(const std::string& raw)
{
    std::string out;
    for (size_t i = 0; i < raw.size(); ++i) {
        const unsigned char c = (unsigned char)raw[i];
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20) {
            static const char kHex[] = "0123456789ABCDEF";
            out += "\\u00";
            out += kHex[(c >> 4) & 15];
            out += kHex[c & 15];
        }
        else out += (char)c;
    }
    return out;
}

void JBuild::str(const std::string& k, const std::string& v)
{
    m_items.push_back("\"" + JsonEscape(k) + "\":\"" + JsonEscape(v) + "\"");
}

void JBuild::num(const std::string& k, const std::string& v)
{
    raw(k, v);
}

void JBuild::raw(const std::string& k, const std::string& v)
{
    m_items.push_back("\"" + JsonEscape(k) + "\":" + v);
}

void JBuild::boolean(const std::string& k, bool v)
{
    raw(k, v ? "true" : "false");
}

std::string JBuild::done() const
{
    std::string out = "{";
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (i) out += ",";
        out += m_items[i];
    }
    return out + "}";
}

// ==================== bcrypt（运行期解析） ====================

namespace {

typedef void* BCRYPT_ALG_HANDLE;
typedef void* BCRYPT_KEY_HANDLE;
typedef void* BCRYPT_HASH_HANDLE;
typedef NTSTATUS(WINAPI *PFN_BCryptOpenAlgorithmProvider)(BCRYPT_ALG_HANDLE*, LPCWSTR, LPWSTR, ULONG);
typedef NTSTATUS(WINAPI *PFN_BCryptCloseAlgorithmProvider)(BCRYPT_ALG_HANDLE, ULONG);
typedef NTSTATUS(WINAPI *PFN_BCryptSetProperty)(HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG);
typedef NTSTATUS(WINAPI *PFN_BCryptGetProperty)(HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG*, ULONG);
typedef NTSTATUS(WINAPI *PFN_BCryptGenerateSymmetricKey)(BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE*, PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
typedef NTSTATUS(WINAPI *PFN_BCryptDestroyKey)(BCRYPT_KEY_HANDLE);
typedef NTSTATUS(WINAPI *PFN_BCryptEncrypt)(BCRYPT_KEY_HANDLE, PUCHAR, ULONG, PUCHAR, PUCHAR, ULONG, PUCHAR, ULONG, ULONG*, ULONG);
typedef NTSTATUS(WINAPI *PFN_BCryptDecrypt)(BCRYPT_KEY_HANDLE, PUCHAR, ULONG, PUCHAR, PUCHAR, ULONG, PUCHAR, ULONG, ULONG*, ULONG);
typedef NTSTATUS(WINAPI *PFN_BCryptCreateHash)(BCRYPT_ALG_HANDLE, BCRYPT_HASH_HANDLE*, PUCHAR, ULONG, PUCHAR, ULONG, ULONG);
typedef NTSTATUS(WINAPI *PFN_BCryptHashData)(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
typedef NTSTATUS(WINAPI *PFN_BCryptFinishHash)(BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG);
typedef NTSTATUS(WINAPI *PFN_BCryptDestroyHash)(BCRYPT_HASH_HANDLE);
typedef NTSTATUS(WINAPI *PFN_BCryptGenRandom)(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, ULONG);

struct BcryptApi {
    HMODULE mod;
    PFN_BCryptOpenAlgorithmProvider OpenAlg;
    PFN_BCryptCloseAlgorithmProvider CloseAlg;
    PFN_BCryptSetProperty SetProperty;
    PFN_BCryptGetProperty GetProperty;
    PFN_BCryptGenerateSymmetricKey GenKey;
    PFN_BCryptDestroyKey DestroyKey;
    PFN_BCryptEncrypt Encrypt;
    PFN_BCryptDecrypt Decrypt;
    PFN_BCryptCreateHash CreateHash;
    PFN_BCryptHashData HashData;
    PFN_BCryptFinishHash FinishHash;
    PFN_BCryptDestroyHash DestroyHash;
    PFN_BCryptGenRandom GenRandom;
    bool ready;
    BcryptApi() : mod(NULL), OpenAlg(NULL), CloseAlg(NULL), SetProperty(NULL), GetProperty(NULL),
                  GenKey(NULL), DestroyKey(NULL), Encrypt(NULL), Decrypt(NULL), CreateHash(NULL),
                  HashData(NULL), FinishHash(NULL), DestroyHash(NULL), GenRandom(NULL), ready(false) {}
    void load()
    {
        if (mod != NULL) return;
        mod = LoadLibraryA("bcrypt.dll");
        if (mod == NULL) return;
        OpenAlg = (PFN_BCryptOpenAlgorithmProvider)GetProcAddress(mod, "BCryptOpenAlgorithmProvider");
        CloseAlg = (PFN_BCryptCloseAlgorithmProvider)GetProcAddress(mod, "BCryptCloseAlgorithmProvider");
        SetProperty = (PFN_BCryptSetProperty)GetProcAddress(mod, "BCryptSetProperty");
        GetProperty = (PFN_BCryptGetProperty)GetProcAddress(mod, "BCryptGetProperty");
        GenKey = (PFN_BCryptGenerateSymmetricKey)GetProcAddress(mod, "BCryptGenerateSymmetricKey");
        DestroyKey = (PFN_BCryptDestroyKey)GetProcAddress(mod, "BCryptDestroyKey");
        Encrypt = (PFN_BCryptEncrypt)GetProcAddress(mod, "BCryptEncrypt");
        Decrypt = (PFN_BCryptDecrypt)GetProcAddress(mod, "BCryptDecrypt");
        CreateHash = (PFN_BCryptCreateHash)GetProcAddress(mod, "BCryptCreateHash");
        HashData = (PFN_BCryptHashData)GetProcAddress(mod, "BCryptHashData");
        FinishHash = (PFN_BCryptFinishHash)GetProcAddress(mod, "BCryptFinishHash");
        DestroyHash = (PFN_BCryptDestroyHash)GetProcAddress(mod, "BCryptDestroyHash");
        GenRandom = (PFN_BCryptGenRandom)GetProcAddress(mod, "BCryptGenRandom");
        ready = OpenAlg && SetProperty && GenKey && Encrypt && Decrypt && CreateHash && HashData && FinishHash && DestroyKey && DestroyHash && CloseAlg && GenRandom;
    }
};

BcryptApi g_bc;

}  // namespace

bool Sha256Raw(const unsigned char* p, size_t n, unsigned char out[32])
{
    g_bc.load();
    if (!g_bc.ready) return false;
    BCRYPT_ALG_HANDLE hAlg = NULL;
    if (g_bc.OpenAlg(&hAlg, L"SHA256", NULL, 0) != 0) return false;
    BCRYPT_HASH_HANDLE hHash = NULL;
    bool ok = false;
    if (g_bc.CreateHash(hAlg, &hHash, NULL, 0, NULL, 0, 0) == 0) {
        if (g_bc.HashData(hHash, (PUCHAR)p, (ULONG)n, 0) == 0 &&
            g_bc.FinishHash(hHash, out, 32, 0) == 0) {
            ok = true;
        }
        g_bc.DestroyHash(hHash);
    }
    g_bc.CloseAlg(hAlg, 0);
    return ok;
}

std::string Sha256Hex(const std::string& text)
{
    unsigned char digest[32];
    if (!Sha256Raw((const unsigned char*)text.data(), text.size(), digest)) return std::string();
    static const char* hex = "0123456789abcdef";
    std::string out;
    for (int i = 0; i < 32; ++i) {
        out += hex[digest[i] >> 4];
        out += hex[digest[i] & 15];
    }
    return out;
}

std::string HmacSha256Hex(const std::string& keyRaw, const std::string& data)
{
    g_bc.load();
    if (!g_bc.ready) return std::string();
    BCRYPT_ALG_HANDLE hAlg = NULL;
    if (g_bc.OpenAlg(&hAlg, L"SHA256", NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0) return std::string();
    BCRYPT_HASH_HANDLE hHash = NULL;
    std::string out;
    unsigned char digest[32];
    if (g_bc.CreateHash(hAlg, &hHash, NULL, 0, (PUCHAR)keyRaw.data(), (ULONG)keyRaw.size(), 0) == 0) {
        if (g_bc.HashData(hHash, (PUCHAR)data.data(), (ULONG)data.size(), 0) == 0 &&
            g_bc.FinishHash(hHash, digest, 32, 0) == 0) {
            static const char* hex = "0123456789abcdef";
            for (int i = 0; i < 32; ++i) {
                out += hex[digest[i] >> 4];
                out += hex[digest[i] & 15];
            }
        }
        g_bc.DestroyHash(hHash);
    }
    g_bc.CloseAlg(hAlg, 0);
    return out;
}

namespace {

// AES-256-CBC：CBC 链模式 + 手工 PKCS7，IV 放在数据前 16 字节
bool AesKey(const unsigned char* key32, BCRYPT_ALG_HANDLE* hAlgOut, BCRYPT_KEY_HANDLE* hKeyOut)
{
    if (!g_bc.ready) return false;
    BCRYPT_ALG_HANDLE hAlg = NULL;
    if (g_bc.OpenAlg(&hAlg, L"AES", NULL, 0) != 0) return false;
    const wchar_t* chain = BCRYPT_CHAIN_MODE_CBC;
    if (g_bc.SetProperty((HANDLE)hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)chain, sizeof(wchar_t) * 3, 0) != 0) {
        g_bc.CloseAlg(hAlg, 0);
        return false;
    }
    BCRYPT_KEY_HANDLE hKey = NULL;
    if (g_bc.GenKey(hAlg, &hKey, NULL, 0, (PUCHAR)key32, 32, 0) != 0) {
        g_bc.CloseAlg(hAlg, 0);
        return false;
    }
    *hAlgOut = hAlg;
    *hKeyOut = hKey;
    return true;
}

}  // namespace

bool RandomBytes(unsigned char* buf, size_t n)
{
    g_bc.load();
    if (!g_bc.ready || g_bc.GenRandom == NULL) return false;
    return g_bc.GenRandom(NULL, buf, (ULONG)n, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
}

std::string Aes256CbcEncrypt(const unsigned char* key32, const std::string& plain, std::string* ivOut)
{
    g_bc.load();
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    if (!AesKey(key32, &hAlg, &hKey)) return std::string();

    unsigned char iv[16];
    if (ivOut == NULL) {
        if (!RandomBytes(iv, 16)) { g_bc.DestroyKey(hKey); g_bc.CloseAlg(hAlg, 0); return std::string(); }
    } else {
        if (ivOut->size() != 16) { g_bc.DestroyKey(hKey); g_bc.CloseAlg(hAlg, 0); return std::string(); }
        memcpy(iv, ivOut->data(), 16);
    }

    std::string data = plain;
    const size_t pad = 16 - (data.size() % 16);
    data.append(pad, (char)(unsigned char)pad);

    std::string out((size_t)data.size() + 16, '\0');
    ULONG done = 0;
    // BCrypt 会就地改写 IV，必须用副本加密；回传给后端的仍是原始 IV
    unsigned char ivWork[16];
    memcpy(ivWork, iv, 16);
    const NTSTATUS st = g_bc.Encrypt(hKey, (PUCHAR)data.data(), (ULONG)data.size(), NULL, ivWork, 16, (PUCHAR)&out[0], (ULONG)out.size(), &done, 0);
    g_bc.DestroyKey(hKey);
    g_bc.CloseAlg(hAlg, 0);
    if (st != 0) return std::string();
    out.resize((size_t)done);
    // 后端格式是 base64(IV + 密文)，BCrypt 不会自带 IV，这里显式拼接
    return std::string((const char*)iv, 16) + out;
}

bool Aes256CbcDecrypt(const unsigned char* key32, const unsigned char* iv, const unsigned char* cipher, size_t cipherLen, std::string* plainOut)
{
    g_bc.load();
    if (cipherLen == 0 || (cipherLen % 16) != 0) return false;
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    if (!AesKey(key32, &hAlg, &hKey)) return false;

    std::string out(cipherLen + 16, '\0');
    ULONG done = 0;
    unsigned char ivCopy[16];
    memcpy(ivCopy, iv, 16);
    const NTSTATUS st = g_bc.Decrypt(hKey, (PUCHAR)cipher, (ULONG)cipherLen, NULL, ivCopy, 16, (PUCHAR)&out[0], (ULONG)out.size(), &done, 0);
    g_bc.DestroyKey(hKey);
    g_bc.CloseAlg(hAlg, 0);
    if (st != 0 || done == 0) return false;

    out.resize((size_t)done);
    const unsigned char pad = (unsigned char)out[out.size() - 1];
    if (pad == 0 || pad > 16 || out.size() < pad) return false;
    for (unsigned char i = 1; i <= pad; ++i) {
        if ((unsigned char)out[out.size() - i] != pad) return false;
    }
    out.resize(out.size() - pad);
    *plainOut = out;
    return true;
}

// ==================== WinHTTP（运行期解析） ====================

namespace {

typedef HINTERNET(WINAPI *PFN_Open)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
typedef HINTERNET(WINAPI *PFN_Connect)(HINTERNET, LPCWSTR, INTERNET_PORT, BOOL);
typedef HINTERNET(WINAPI *PFN_OpenRequest)(HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR*, LPCWSTR*, DWORD);
typedef BOOL(WINAPI *PFN_SetOption)(HINTERNET, DWORD, LPVOID, DWORD);
typedef BOOL(WINAPI *PFN_QueryOption)(HINTERNET, DWORD, LPVOID, LPDWORD);
typedef BOOL(WINAPI *PFN_QueryHeaders)(HINTERNET, DWORD, LPCWSTR, LPVOID, LPDWORD, LPDWORD);
typedef BOOL(WINAPI *PFN_SendRequest)(HINTERNET, LPCWSTR, DWORD, LPCVOID, DWORD, DWORD, DWORD_PTR);
typedef BOOL(WINAPI *PFN_ReceiveResponse)(HINTERNET, LPVOID);
typedef BOOL(WINAPI *PFN_ReadData)(HINTERNET, LPVOID, DWORD, LPDWORD);
typedef BOOL(WINAPI *PFN_CloseHandle)(HINTERNET);
typedef BOOL(WINAPI *PFN_QueryDataAvailable)(HINTERNET, LPDWORD);

struct WinHttpApi {
    HMODULE mod;
    PFN_Open Open;
    PFN_Connect Connect;
    PFN_OpenRequest OpenRequest;
    PFN_SetOption SetOption;
    PFN_QueryOption QueryOption;
    PFN_QueryHeaders QueryHeaders;
    PFN_SendRequest SendRequest;
    PFN_ReceiveResponse ReceiveResponse;
    PFN_ReadData ReadData;
    PFN_CloseHandle CloseHandle;
    PFN_QueryDataAvailable QueryAvailable;
    bool ready;
    WinHttpApi() : mod(NULL), Open(NULL), Connect(NULL), OpenRequest(NULL), SetOption(NULL), QueryOption(NULL), QueryHeaders(NULL),
                   SendRequest(NULL), ReceiveResponse(NULL), ReadData(NULL), CloseHandle(NULL),
                   QueryAvailable(NULL), ready(false) {}
    void load()
    {
        if (mod != NULL) return;
        mod = LoadLibraryA("winhttp.dll");
        if (mod == NULL) return;
        Open = (PFN_Open)GetProcAddress(mod, "WinHttpOpen");
        Connect = (PFN_Connect)GetProcAddress(mod, "WinHttpConnect");
        OpenRequest = (PFN_OpenRequest)GetProcAddress(mod, "WinHttpOpenRequest");
        SetOption = (PFN_SetOption)GetProcAddress(mod, "WinHttpSetOption");
        QueryOption = (PFN_QueryOption)GetProcAddress(mod, "WinHttpQueryOption");
        QueryHeaders = (PFN_QueryHeaders)GetProcAddress(mod, "WinHttpQueryHeaders");
        SendRequest = (PFN_SendRequest)GetProcAddress(mod, "WinHttpSendRequest");
        ReceiveResponse = (PFN_ReceiveResponse)GetProcAddress(mod, "WinHttpReceiveResponse");
        ReadData = (PFN_ReadData)GetProcAddress(mod, "WinHttpReadData");
        CloseHandle = (PFN_CloseHandle)GetProcAddress(mod, "WinHttpCloseHandle");
        QueryAvailable = (PFN_QueryDataAvailable)GetProcAddress(mod, "WinHttpQueryDataAvailable");
        ready = Open && Connect && OpenRequest && SetOption && QueryHeaders && SendRequest && ReceiveResponse && ReadData && CloseHandle;
    }
};

WinHttpApi g_wh;

bool SplitUrl(const std::string& url, std::string* scheme, std::string* host, std::string* path, INTERNET_PORT* port)
{
    const std::string head = "https://";
    bool https = true;
    size_t p = 0;
    if (StartsWith(url, "https://")) p = head.size();
    else if (StartsWith(url, "http://")) { https = false; p = 7; }
    else return false;
    *scheme = https ? "https" : "http";
    size_t slash = url.find('/', p);
    std::string hostpart = (slash == std::string::npos) ? url.substr(p) : url.substr(p, slash - p);
    *path = (slash == std::string::npos) ? "/" : url.substr(slash);
    size_t colon = hostpart.find(':');
    if (colon != std::string::npos) {
        *host = hostpart.substr(0, colon);
        *port = (INTERNET_PORT)atoi(hostpart.substr(colon + 1).c_str());
    } else {
        *host = hostpart;
        *port = https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    }
    return !host->empty();
}

}  // namespace

HttpResult HttpPostJson(const std::string& url, const std::string& body, int timeoutMs)
{
    HttpResult res;
    g_wh.load();
    if (!g_wh.ready) {
        res.error = "winhttp unavailable";
        return res;
    }

    std::string scheme, host, path;
    INTERNET_PORT port = 0;
    if (!SplitUrl(url, &scheme, &host, &path, &port)) {
        res.error = "bad url: " + url;
        return res;
    }

    HINTERNET hSession = g_wh.Open(L"cxah-client", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession == NULL) { res.error = "WinHttpOpen failed"; return res; }

    DWORD resolve = (DWORD)timeoutMs, connect = (DWORD)timeoutMs, send = (DWORD)timeoutMs, recv = (DWORD)timeoutMs;
    {
        g_wh.SetOption(hSession, WINHTTP_OPTION_RESOLVE_TIMEOUT, &resolve, sizeof(resolve));
        g_wh.SetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &connect, sizeof(connect));
        g_wh.SetOption(hSession, WINHTTP_OPTION_SEND_TIMEOUT, &send, sizeof(send));
        g_wh.SetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &recv, sizeof(recv));
    }

    HINTERNET hConnect = g_wh.Connect(hSession, WideFromUtf8(host).c_str(), port, 0);
    if (hConnect == NULL) { g_wh.CloseHandle(hSession); res.error = "WinHttpConnect failed"; return res; }

    const BOOL secure = (scheme == "https") ? TRUE : FALSE;
    HINTERNET hRequest = g_wh.OpenRequest(hConnect, L"POST", WideFromUtf8(path).c_str(), NULL, NULL, NULL,
                                          secure ? WINHTTP_FLAG_SECURE : 0);
    if (hRequest == NULL) {
        g_wh.CloseHandle(hConnect); g_wh.CloseHandle(hSession);
        res.error = "WinHttpOpenRequest failed";
        return res;
    }

    if (secure) {
        DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
        g_wh.SetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
    }

    const wchar_t* headers = L"Content-Type: application/json; charset=utf-8\r\nAccept: application/json\r\n";
    BOOL sent = g_wh.SendRequest(hRequest, headers, (DWORD)-1, (LPVOID)body.data(),
                                (DWORD)body.size(), (DWORD)body.size(), 0);
    if (g_wh.SetOption) g_wh.SetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &recv, sizeof(recv));
    if (sent && g_wh.ReceiveResponse(hRequest, NULL)) {
        DWORD status = 0, size = sizeof(status);
        g_wh.QueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &size, NULL);
        res.status = (int)status;
        for (;;) {
            char buf[8192];
            DWORD read = 0;
            if (!g_wh.ReadData(hRequest, buf, sizeof(buf), &read) || read == 0) break;
            res.body.append(buf, read);
        }
        res.ok = (res.status >= 200 && res.status < 300);
    } else {
        res.error = "request failed";
    }

    g_wh.CloseHandle(hRequest);
    g_wh.CloseHandle(hConnect);
    g_wh.CloseHandle(hSession);
    return res;
}

// ==================== 状态 ====================

State& S()
{
    static State g_state;
    return g_state;
}

void Grant::load(const JVal& v)
{
    rawJson = v.dump();
    uid          = v.find("uid") ? v.find("uid")->asStr() : "";
    email        = v.find("email") ? v.find("email")->asStr() : "";
    qq           = v.find("qq") ? v.find("qq")->asStr() : "";
    bindMac      = v.find("bind_mac") ? v.find("bind_mac")->asStr() : "";
    macKey       = v.find("mac_key") ? v.find("mac_key")->asStr() : "";
    state        = v.find("state") ? v.find("state")->asStr() : "yes";
    stateReason  = v.find("state_reason") ? v.find("state_reason")->asStr() : "";
    dueDate      = v.find("due_date") ? v.find("due_date")->asStr() : "";
    regDate      = v.find("reg_date") ? v.find("reg_date")->asStr() : "";
    lastLogin    = v.find("last_login") ? v.find("last_login")->asStr() : "";
    loginNum     = v.find("login_num") ? v.find("login_num")->asInt(0) : 0;
    function     = v.find("function") ? v.find("function")->asStr() : "1";
    functionName = v.find("function_name") ? v.find("function_name")->asStr() : "";
    softLimit    = v.find("soft_limit") ? v.find("soft_limit")->asStr() : "";
    orderNumber  = v.find("order_number") ? v.find("order_number")->asStr() : "";
    pucType      = v.find("puc_type") ? v.find("puc_type")->asStr() : "";
    channelId    = v.find("channel_id") ? v.find("channel_id")->asStr() : "0";
    homeUsername = v.find("home_username") ? v.find("home_username")->asStr() : "";
    remarks      = v.find("remarks") ? v.find("remarks")->asStr() : "";
    serverTime   = v.find("server_time") ? v.find("server_time")->asStr() : "";
    isTrial      = v.find("is_trial") ? v.find("is_trial")->asBool(false) : false;
    isExpired    = v.find("is_expired") ? v.find("is_expired")->asBool(false) : false;
    authorized   = v.find("authorized") ? v.find("authorized")->asBool(false) : false;
    limitChange  = v.find("limit_change") ? v.find("limit_change")->asInt(0) : 0;
    changeTime   = v.find("change_time") ? v.find("change_time")->asInt(0) : 0;
    changeLeft   = v.find("change_left") ? v.find("change_left")->asInt(0) : 0;

    const JVal* at = v.find("attr");
    attrJson   = at == NULL ? std::string() : at->dump();
    ip         = v.find("ip") ? v.find("ip")->asStr() : "";
    ipLocation = v.find("ip_location") ? v.find("ip_location")->asStr() : "";

    features.clear();
    const JVal* fs = v.find("functions");
    if (fs != NULL && fs->type == JVal::TArr) {
        for (size_t i = 0; i < fs->arr.size(); ++i) {
            const JVal& f = fs->arr[i];
            Feature fe;
            fe.code    = f.find("code") ? f.find("code")->asStr() : "";
            fe.name    = f.find("name") ? f.find("name")->asStr() : "";
            fe.dueDate = f.find("due_date") ? f.find("due_date")->asStr() : "";
            fe.usable  = f.find("usable") ? f.find("usable")->asBool(false) : false;
            if (fe.name.empty()) fe.name = fe.code;
            features.push_back(fe);
        }
    }
}

std::string Grant::featuresText() const
{
    std::string out;
    for (size_t i = 0; i < features.size(); ++i) {
        if (i) out += "\r\n";
        out += features[i].name + "|" + features[i].dueDate + (features[i].usable ? "|1" : "|0");
    }
    return out;
}

std::string Grant::featureNames() const
{
    std::string out;
    for (size_t i = 0; i < features.size(); ++i) {
        if (!features[i].usable) continue;
        if (!out.empty()) out += "、";
        out += features[i].name;
    }
    return out;
}

std::string Grant::attrField(const std::string& key) const
{
    if (attrJson.empty() || key.empty()) return std::string();
    JVal j;
    if (!JsonParse(attrJson, j)) return std::string();
    const JVal* one = j.find(key);
    return one == NULL ? std::string() : one->asStr();
}

std::string Grant::field(const std::string& name) const
{
    if (name == "uid") return uid;
    if (name == "email") return email;
    if (name == "qq") return qq;
    if (name == "bind_mac") return bindMac;
    if (name == "mac_key") return macKey;
    if (name == "state") return state;
    if (name == "state_reason") return stateReason;
    if (name == "due_date") return dueDate;
    if (name == "reg_date") return regDate;
    if (name == "last_login") return lastLogin;
    if (name == "login_num") return ToStr(loginNum);
    if (name == "function") return function;
    if (name == "function_name") return functionName;
    if (name == "soft_limit") return softLimit;
    if (name == "functions") return featuresText();
    if (name == "function_names") return featureNames();
    if (name == "order_number") return orderNumber;
    if (name == "puc_type") return pucType;
    if (name == "channel_id") return channelId;
    if (name == "home_username") return homeUsername;
    if (name == "remarks") return remarks;
    if (name == "server_time") return serverTime;
    if (name == "limit_change") return ToStr(limitChange);
    if (name == "change_time") return ToStr(changeTime);
    if (name == "change_left") return ToStr(changeLeft);
    if (name == "is_trial") return isTrial ? "1" : "0";
    if (name == "is_expired") return isExpired ? "1" : "0";
    if (name == "authorized") return authorized ? "1" : "0";
    if (name == "attr") return attrJson;
    if (name == "ip") return ip;
    if (name == "ip_location") return ipLocation;
    if (name == "json") return rawJson;
    return std::string();
}

void OemInfo::load(const JVal& v)
{
    channelId    = v.find("channel_id") ? v.find("channel_id")->asStr() : "0";
    key          = v.find("key") ? v.find("key")->asStr() : "";
    brandName    = v.find("brand_name") ? v.find("brand_name")->asStr() : "";
    softName     = v.find("soft_name") ? v.find("soft_name")->asStr() : "";
    pucType      = v.find("puc_type") ? v.find("puc_type")->asStr() : "";
    themeColor   = v.find("theme_color") ? v.find("theme_color")->asStr() : "";
    qq           = v.find("qq") ? v.find("qq")->asStr() : "";
    qq2          = v.find("qq2") ? v.find("qq2")->asStr() : "";
    wechat       = v.find("wechat") ? v.find("wechat")->asStr() : "";
    phone        = v.find("phone") ? v.find("phone")->asStr() : "";
    home         = v.find("home") ? v.find("home")->asStr() : "";
    shopHome     = v.find("shop_home") ? v.find("shop_home")->asStr() : "";
    helpHome     = v.find("help_home") ? v.find("help_home")->asStr() : "";
    downloadHome = v.find("download_home") ? v.find("download_home")->asStr() : "";
    changeHome   = v.find("change_home") ? v.find("change_home")->asStr() : "";
    buyText      = v.find("buy_text") ? v.find("buy_text")->asStr() : "";
    adUrl        = v.find("ad_url") ? v.find("ad_url")->asStr() : "";
    adUrl2       = v.find("ad_url2") ? v.find("ad_url2")->asStr() : "";
    serverUrl    = v.find("server_url") ? v.find("server_url")->asStr() : "";
    serverUrl2   = v.find("server_url2") ? v.find("server_url2")->asStr() : "";
    official     = v.find("official") ? v.find("official")->asBool(true) : true;
    ableReg      = v.find("able_reg") ? v.find("able_reg")->asInt(0) : 0;
    protection   = v.find("protection") ? v.find("protection")->asInt(0) : 0;
    const JVal* setting = v.find("setting");
    if (setting != NULL && setting->type == JVal::TObj) {
        settingRaw = setting->dump();   // 原样保留，宿主按名取值
    }
}

std::string OemInfo::field(const std::string& name) const
{
    if (name == "channel_id") return channelId;
    if (name == "key") return key;
    if (name == "brand_name") return brandName;
    if (name == "soft_name") return softName;
    if (name == "puc_type") return pucType;
    if (name == "theme_color") return themeColor;
    if (name == "qq") return qq;
    if (name == "qq2") return qq2;
    if (name == "wechat") return wechat;
    if (name == "phone") return phone;
    if (name == "home") return home;
    if (name == "shop_home") return shopHome;
    if (name == "help_home") return helpHome;
    if (name == "download_home") return downloadHome;
    if (name == "change_home") return changeHome;
    if (name == "buy_text") return buyText;
    if (name == "ad_url") return adUrl;
    if (name == "ad_url2") return adUrl2;
    if (name == "server_url") return serverUrl;
    if (name == "server_url2") return serverUrl2;
    if (name == "official") return official ? "1" : "0";
    if (name == "able_reg") return ToStr(ableReg);
    if (name == "protection") return ToStr(protection);
    if (name == "setting") return settingRaw;
    return std::string();
}

}  // namespace cx
