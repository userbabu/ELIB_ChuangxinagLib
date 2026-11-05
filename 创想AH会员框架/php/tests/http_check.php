<?php
/**
 * 走真实 HTTP 的联调脚本：证明 加密信封 + 签名 + AES 往返 在 Web 服务器上同样可用。
 *
 *   1) 起服务： php -S 127.0.0.1:8317 -t public
 *   2) 跑本脚本： php tests/http_check.php [接口地址]
 *
 * 同时打印一条等价 curl 命令，方便用命令行手工复现（明文通道）。
 */

require __DIR__ . '/../app/bootstrap.php';
require __DIR__ . '/Client.php';

use CxSoft\Crypto;
use CxSoft\Test\Client;

$config = cx_boot();
$base   = isset($argv[1]) ? rtrim($argv[1], '/') : 'http://127.0.0.1:8317/api.php';
$crypto = new Crypto($config['app_secret']);

$fail = 0;
function check($name, $cond, $extra = '')
{
    global $fail;
    echo ($cond ? '  [ OK ] ' : '  [FAIL] ') . $name . ($cond ? '' : '  ' . $extra) . "\n";
    if (!$cond) {
        $fail++;
    }
}

/** 真实 HTTP 调用（加密信封） */
function post($route, array $params)
{
    global $crypto, $base;
    $ts  = (int) round(microtime(true) * 1000);
    $d   = $crypto->encrypt(json_encode($params, JSON_UNESCAPED_UNICODE), $route, $ts, 'req');
    $env = ['r' => $route, 'v' => 1, 't' => $ts, 'd' => $d, 's' => $crypto->sign($route, $ts, $d, 'req')];

    $ch = curl_init($base);
    curl_setopt_array($ch, [
        CURLOPT_POST           => true,
        CURLOPT_POSTFIELDS     => json_encode($env, JSON_UNESCAPED_UNICODE),
        CURLOPT_HTTPHEADER     => ['Content-Type: application/json'],
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_TIMEOUT        => 15,
    ]);
    $body = curl_exec($ch);
    if ($body === false) {
        echo "无法连接 {$base}：" . curl_error($ch) . "\n请先用 php -S 127.0.0.1:8317 -t public 起服务\n";
        exit(1);
    }
    $code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);

    $json = json_decode((string) $body, true);
    if (!is_array($json)) {
        check('HTTP 返回 JSON (状态 ' . $code . ')', false, substr((string) $body, 0, 200));
        exit(1);
    }
    if (isset($json['d'])) {
        $plain = $crypto->decrypt($json['d'], $route, $ts, 'resp');
        $inner = json_decode((string) $plain, true);
        $json['data'] = is_array($inner) && isset($inner['data']) ? $inner['data'] : [];
        $json['resp_sign_ok'] = isset($inner['data']) && hash_equals(
            $crypto->sign($route, $ts, $json['d'], 'resp'), (string) $json['s']
        );
    }
    return $json;
}

$CTX = ['puc_type' => '3', 'channel_id' => 0, 'version' => '20260001', 'app_md5' => str_repeat('b', 32), 'system' => '12'];
$email = 'cxah-http-' . substr((string) time(), -6) . '@test.local';
$mac   = Client::mac('3', (string) (100000 + random_int(1, 899999)));

echo "== HTTP 联调 {$base} ==\n";

$r = post('common.time', []);
check('时间接口', $r['ok'] === true && isset($r['data']['timestamp']), json_encode($r));
check('HTTP 响应签名有效', !empty($r['resp_sign_ok']), $r['raw'] ?? '');

$r = post('channel.oem', ['key' => 'cxangsoft'] + $CTX);
check('渠道接口（官方 channel_id 恒为 0）', $r['ok'] && (int) $r['data']['channel_id'] === 0 && $r['data']['soft_name'] !== '', json_encode($r['code']));

$cap = post('common.captcha', []);
$file = $config['log']['dir'] . '/captcha/' . $cap['data']['id'] . '.json';
$code = json_decode((string) file_get_contents($file), true);
$r = post('account.register', [
    'email' => $email, 'mac_key' => $mac, 'qq' => '709005505',
    'captcha_id' => $cap['data']['id'], 'captcha_code' => $code['code'],
] + $CTX);
check('注册接口', $r['ok'] === true, json_encode($r['code'] ?? '') . ' ' . ($r['msg'] ?? ''));

$r = post('account.trial', ['email' => $email, 'mac_key' => $mac] + $CTX);
check('试用接口', $r['ok'] === true && $r['data']['auth_status'] === 'trial', json_encode($r));

$r = post('auth.login', ['email' => $email, 'mac_key' => $mac] + $CTX);
check('登录接口', $r['ok'] === true && $r['data']['grant']['email'] === $email, json_encode($r['code']));
check('授权字段完整', isset($r['data']['grant']['due_date'], $r['data']['grant']['change_left']), '');

$other = Client::mac('3', (string) (100000 + random_int(1, 899999)));
$r = post('auth.login', ['email' => $email, 'mac_key' => $other] + $CTX);
check('换机器码走换机分支', $r['ok'] === false && $r['code'] === '4001' && $r['data']['need_change'] === true, json_encode($r['code']));

echo "\n明文通道 curl 复现示例（需本机设 CXAH_DEV=1 且配置 debug_plain=true）：\n";
echo '  curl -s -X POST ' . $base . " -d '{\"r\":\"common.time\",\"v\":1,\"t\":<毫秒时间戳>,\"data\":{}}'\n";
echo "易语言侧联调可参考 php/README.md 的信封字段说明。\n";

echo $fail === 0 ? "\nHTTP 联调全部通过\n" : "\n失败 {$fail} 项\n";
exit($fail === 0 ? 0 : 1);
