<?php
/**
 * 统一 JSON 协议的加解密与签名。
 *
 * 请求信封: {"r":路由,"v":1,"t":毫秒,"d":加密后的业务JSON,"s":签名}
 * 响应信封: {"ok":true,"code":"0","msg":"","t":毫秒,"v":1,"d":加密后的数据,"s":签名}
 *
 * d = base64( 16字节IV + AES-256-CBC(明文, PKCS7) )
 * s = hex( HMAC-SHA256( "r|t|d", 派生签名密钥 ) )
 * 派生密钥 = SHA256( app_secret | 方向 | 路由 | 时间戳 )，服务端与客户端各自独立算得。
 */

namespace CxSoft;

class Crypto
{
    /** @var string */
    private $secret;

    public function __construct($appSecret)
    {
        $this->secret = (string) $appSecret;
    }

    public function deriveKey($purpose, $route, $timestamp)
    {
        return hash('sha256', $this->secret . '|' . $purpose . '|' . $route . '|' . $timestamp, true);
    }

    private function encKey($purpose, $route, $ts)
    {
        return $this->deriveKey($purpose === 'req' ? 'ENC-REQ' : 'ENC-RESP', $route, $ts);
    }

    private function sigKey($purpose, $route, $ts)
    {
        return $this->deriveKey($purpose === 'req' ? 'SIG-REQ' : 'SIG-RESP', $route, $ts);
    }

    /** 明文 JSON 字符串 -> 信封里的 d 字段 */
    public function encrypt($plainJson, $route, $ts, $purpose = 'req')
    {
        $iv = random_bytes(16);
        $cipher = openssl_encrypt($plainJson, 'aes-256-cbc', $this->encKey($purpose, $route, $ts), OPENSSL_RAW_DATA, $iv);
        if ($cipher === false) {
            throw new \RuntimeException('AES 加密失败');
        }
        return base64_encode($iv . $cipher);
    }

    /** 信封里的 d 字段 -> 明文 JSON 字符串，失败返回 null */
    public function decrypt($encoded, $route, $ts, $purpose = 'req')
    {
        $raw = base64_decode((string) $encoded, true);
        if ($raw === false || strlen($raw) <= 16) {
            return null;
        }
        $iv = substr($raw, 0, 16);
        $plain = openssl_decrypt(substr($raw, 16), 'aes-256-cbc', $this->encKey($purpose, $route, $ts), OPENSSL_RAW_DATA, $iv);
        return $plain === false ? null : $plain;
    }

    public function sign($route, $ts, $encoded, $purpose = 'req')
    {
        return hash_hmac('sha256', $route . '|' . $ts . '|' . $encoded, $this->sigKey($purpose, $route, $ts));
    }

    public function verify($route, $ts, $encoded, $signature)
    {
        return is_string($signature) && hash_equals($this->sign($route, $ts, $encoded, 'req'), strtolower($signature));
    }
}
