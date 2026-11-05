// 加密/HTTP 单文件探针：直接链接 cx_net.cpp + cx_api.cpp，逐步打印定位崩溃点。
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "cx_core.h"

using namespace cx;

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    S().server = "http://127.0.0.1:8317/api.php";
    S().secret = "CXAH-DEV-SECRET-CHANGE-ME";
    S().pucType = "3";
    S().timeoutMs = 15000;

    printf("1 sha256hex = %s\n", Sha256Hex("abc").c_str());

    unsigned char key[32];
    std::string src = S().secret + "|ENC-REQ|common.time|123";
    if (!Sha256Raw((const unsigned char*)src.data(), src.size(), key)) { printf("1b Sha256Raw FAIL\n"); return 1; }
    printf("1b sha256raw ok %02X%02X\n", key[0], key[1]);

    std::string hmac = HmacSha256Hex(std::string((char*)key, 32), "common.time|123|xx");
    printf("2 hmac = %s\n", hmac.c_str());

    std::string cipher = Aes256CbcEncrypt(key, "{\"a\":1}", NULL);
    printf("3 cipher bytes = %d\n", (int)cipher.size());
    if (cipher.empty()) return 3;

    std::string d = Base64Encode((const unsigned char*)cipher.data(), cipher.size());
    printf("4 b64 len = %d\n", (int)d.size());

    std::vector<unsigned char> back = Base64Decode(d);
    printf("5 decoded = %d\n", (int)back.size());
    std::string plain;
    if (!Aes256CbcDecrypt(key, &back[0], &back[16], back.size() - 16, &plain)) { printf("6 decrypt FAIL\n"); return 6; }
    printf("6 roundtrip = %s\n", plain.c_str());

    std::string ts;
    std::string env = BuildEnvelope("common.time", "{}", &ts);
    printf("7 envelope len = %d ts=%s\n", (int)env.size(), ts.c_str());
    if (env.empty()) return 7;

    HttpResult http = HttpPostJson(S().server, env, 15000);
    printf("8 http ok=%d status=%d len=%d err=%s\n", http.ok ? 1 : 0, http.status, (int)http.body.size(), http.error.c_str());

    ApiResponse res;
    bool parsed = ParseEnvelope("common.time", ts, http.body, res);
    printf("9 parsed=%d ok=%d code=%s msg=%s\n", parsed ? 1 : 0, res.ok ? 1 : 0, res.code.c_str(), res.msg.c_str());
    if (!res.data.isNull()) printf("9b timestamp=%s\n", res.data.find("timestamp") ? res.data.find("timestamp")->asStr().c_str() : "?");

    S().macKey = "MzUtMTIzNDU2LUNQVVUtOTktMy03NS1hYmMtTUFDVkVSMg==";
    JBuild probe;
    probe.str("puc_type", S().pucType);
    probe.str("channel_id", "0");
    probe.str("email", "probe-reg@test.local");
    probe.str("mac_key", S().macKey);
    probe.str("learn", GbkToUtf8("官网"));
    printf("11 body json = %s\n", probe.done().c_str());
    const ApiResponse reg = ApiCall("account.register", probe.done());
    printf("11b register code=%s msg=%s\n", reg.code.c_str(), reg.msg.c_str());
    printf("11c raw=%.200s\n", reg.raw.c_str());

    const ApiResponse direct = ApiCall("common.time", "{}");
    printf("10 direct api ok=%d code=%s\n", direct.ok ? 1 : 0, direct.code.c_str());
    return 0;
}
