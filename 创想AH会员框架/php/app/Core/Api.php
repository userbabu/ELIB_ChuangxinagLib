<?php
/**
 * 统一 JSON 入口：解析信封 -> 校验签名/时间窗 -> 路由 -> 统一响应。
 *
 * 请求（加密模式）: {"r":"auth.login","v":1,"t":1700000000000,"d":"<加密业务JSON>","s":"<签名>"}
 * 请求（明文模式）: {"r":"auth.login","v":1,"t":1700000000000,"data":{...}}   仅本机联调用
 * 响应: {"ok":true|false,"code":"0","msg":"...","t":毫秒,"v":1,"data":{...}}
 *       加密模式下 data 换成 d(密文)+s(签名)。
 */

namespace CxSoft;

use CxSoft\Service\AccountService;
use CxSoft\Service\AuthService;
use CxSoft\Service\ChannelService;
use CxSoft\Service\KeyService;
use CxSoft\Service\SiteService;

class Api
{
    /** @var array */
    private static $cfg;
    /** @var Crypto */
    private static $crypto;

    public static function run(array $config, $raw = null)
    {
        self::$cfg    = $config;
        self::$crypto = new Crypto($config['app_secret']);

        header('Content-Type: application/json; charset=utf-8');
        header('Cache-Control: no-store');
        header('X-Powered-By: cxah-api');

        if ($raw === null) {
            $raw = file_get_contents('php://input');
        }
        $env = json_decode((string) $raw, true);
        if (!is_array($env) || !isset($env['r'])) {
            self::fail('1001', '请求格式不正确', '', true);
            return;
        }

        $route = (string) $env['r'];
        $ts    = isset($env['t']) ? (int) $env['t'] : 0;
        $plain = !isset($env['d']);

        try {
            $params = self::unwrap($env, $route, $ts, $plain);
            $ctx    = self::context($params);
            $data   = self::dispatch($route, $params, $ctx);
            self::ok($data, $route, $ts, $plain);
        } catch (ApiException $e) {
            self::fail($e->apiCode, $e->userMsg, $route, $plain, $e->data, $ts);
        } catch (\Throwable $e) {
            Log::error('接口异常 ' . $route . ' ' . get_class($e) . ': ' . $e->getMessage() . ' @' . $e->getFile() . ':' . $e->getLine());
            self::fail('9999', null, $route, $plain, [], $ts);
        }
    }

    /* ------------------------------ 信封 ------------------------------ */

    private static function unwrap(array $env, $route, $ts, $plain)
    {
        self::checkTime($ts);

        if ($plain) {
            if (empty(self::$cfg['allow_plain'])) {
                throw new ApiException('1001', '本接口只接受加密请求');
            }
            return isset($env['data']) && is_array($env['data']) ? $env['data'] : [];
        }

        $encoded = (string) (isset($env['d']) ? $env['d'] : '');
        if (!self::$crypto->verify($route, $ts, $encoded, isset($env['s']) ? $env['s'] : '')) {
            Log::info('签名校验失败 route=' . $route . ' ip=' . self::ip());
            throw new ApiException('1002', null);
        }
        $json = self::$crypto->decrypt($encoded, $route, $ts, 'req');
        if ($json === null) {
            throw new ApiException('1004', null);
        }
        $params = json_decode($json, true);
        if (!is_array($params)) {
            throw new ApiException('1001', null);
        }
        return $params;
    }

    private static function checkTime($ts)
    {
        $now  = (int) (micro_time() * 1000);
        $past = (int) self::$cfg['time_window']['past'];
        $fut  = (int) self::$cfg['time_window']['future'];
        if ($ts <= 0 || ($now - $ts) > $past || ($ts - $now) > $fut) {
            throw new ApiException('1003', null);
        }
    }

    private static function ok(array $data, $route, $ts, $plain)
    {
        self::send(self::envelope($data, $route, $ts, $plain, '0', '', true));
    }

    private static function fail($code, $msg, $route, $plain, $data = [], $ts = 0)
    {
        $msg = $msg !== null && $msg !== '' ? $msg : Code::msg($code);
        if ($ts <= 0) {
            $ts = (int) (micro_time() * 1000);
        }
        self::send(self::envelope($data, $route, $ts, $plain, $code, $msg, false));
    }

    private static function envelope(array $data, $route, $ts, $plain, $code, $msg, $ok)
    {
        $body = [
            'ok'   => $ok,
            'code' => (string) $code,
            'msg'  => (string) $msg,
            't'    => (int) (micro_time() * 1000),
            'v'    => 1,
        ];
        if ($plain) {
            $body['data'] = (object) $data;
            return $body;
        }
        $json  = json_encode(['data' => (object) $data, 'code' => (string) $code, 'msg' => (string) $msg], JSON_UNESCAPED_UNICODE);
        $enc   = self::$crypto->encrypt($json, $route, $ts, 'resp');
        $body['d'] = $enc;
        $body['s'] = self::$crypto->sign($route, $ts, $enc, 'resp');
        return $body;
    }

    private static function send(array $body)
    {
        echo json_encode($body, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    }

    /* ------------------------------ 路由 ------------------------------ */

    private static function dispatch($route, array $p, array $ctx)
    {
        // 授权类路由必须带产品类型；基础路由（时间/验证码/渠道）不需要
        if (in_array($route, ['account.register', 'account.trial', 'auth.login', 'auth.refresh', 'auth.event', 'change.request', 'change.confirm', 'key.activate'], true)) {
            self::requireProduct($ctx);
        }

        switch ($route) {
            case 'common.time':
                return SiteService::time();
            case 'common.captcha':
                return SiteService::captcha($p);
            case 'channel.oem':
                return ChannelService::oem($p, $ctx);
            case 'account.register':
                return AccountService::register($p, $ctx);
            case 'account.trial':
                return AccountService::trial($p, $ctx);
            case 'auth.login':
                return AuthService::login($p, $ctx);
            case 'auth.refresh':
                return AuthService::refresh($p, $ctx);
            case 'auth.event':
                return AuthService::event($p, $ctx);
            case 'change.request':
                return AuthService::changeProbe($p, $ctx);
            case 'change.confirm':
                return KeyService::changeConfirm($p, $ctx);
            case 'key.activate':
                return KeyService::activate($p, $ctx);
            default:
                Log::info('未知路由 ' . $route . ' ip=' . self::ip());
                throw new ApiException('1005', null);
        }
    }

    /** 公共上下文：设备与来源信息，全部走同一套字段名 */
    private static function context(array $p)
    {
        return [
            'ip'          => self::ip(),
            'ip_location' => isset($p['ip_location']) ? mb_substr((string) $p['ip_location'], 0, 100) : '',
            'ua'          => isset($_SERVER['HTTP_USER_AGENT']) ? mb_substr((string) $_SERVER['HTTP_USER_AGENT'], 0, 255) : '',
            'puc_type'    => isset($p['puc_type']) ? (string) $p['puc_type'] : '',
            'channel_id'  => isset($p['channel_id']) ? (int) $p['channel_id'] : 0,
            'version'     => isset($p['version']) ? mb_substr((string) $p['version'], 0, 10) : '',
            'md5'         => isset($p['app_md5']) ? mb_substr((string) $p['app_md5'], 0, 32) : '',
            'system'      => isset($p['system']) ? mb_substr((string) $p['system'], 0, 4) : '0',
        ];
    }

    /** 授权类路由必须带产品类型与机器码来源信息 */
    private static function requireProduct(array $ctx)
    {
        if ($ctx['puc_type'] === '') {
            throw new ApiException('1001', '缺少产品类型');
        }
    }

    private static function ip()
    {
        $keys = ['HTTP_X_FORWARDED_FOR', 'HTTP_X_REAL_IP', 'REMOTE_ADDR'];
        foreach ($keys as $k) {
            if (!empty($_SERVER[$k])) {
                $value = explode(',', (string) $_SERVER[$k])[0];
                $ip    = trim($value);
                if (filter_var($ip, FILTER_VALIDATE_IP)) {
                    return $ip;
                }
            }
        }
        return '0.0.0.0';
    }
}

/** 毫秒时间戳 */
function micro_time()
{
    return microtime(true);
}
