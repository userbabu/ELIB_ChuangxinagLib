<?php
/**
 * 测试用 API 客户端：与易语言支持库走同一套信封算法（明文/加密两种）。
 */

namespace CxSoft\Test;

use CxSoft\Crypto;

class Client
{
    /** @var array */
    public $config;
    /** @var Crypto */
    private $crypto;
    /** @var bool 是否使用加密信封 */
    public $encrypt = true;

    public function __construct(array $config)
    {
        $this->config = $config;
        $this->crypto = new Crypto($config['app_secret']);
    }

    /**
     * 调用接口，返回 ['ok','code','msg','data','raw']
     * @param array $extra 覆盖信封字段，用于构造异常请求
     */
    public function call($route, array $params = [], array $extra = [])
    {
        $ts  = isset($extra['t']) ? (int) $extra['t'] : (int) round(microtime(true) * 1000);
        if ($this->encrypt) {
            $json = json_encode($params, JSON_UNESCAPED_UNICODE);
            $d    = $this->crypto->encrypt($json, $route, $ts, 'req');
            $env  = ['r' => $route, 'v' => 1, 't' => $ts, 'd' => $d];
            $env['s'] = isset($extra['s']) ? $extra['s'] : $this->crypto->sign($route, $ts, $d, 'req');
        } else {
            $env = ['r' => $route, 'v' => 1, 't' => $ts, 'data' => (object) $params];
        }
        foreach ($extra as $k => $v) {
            if ($k !== 't') {
                $env[$k] = $v;
            }
        }

        ob_start();
        \CxSoft\Api::run($this->config, json_encode($env, JSON_UNESCAPED_UNICODE));
        $out = ob_get_clean();

        $body = json_decode($out, true);
        if (!is_array($body)) {
            return ['ok' => false, 'code' => 'RAW', 'msg' => $out, 'data' => [], 'raw' => $out];
        }
        if (isset($body['d'])) {
            $plain = $this->crypto->decrypt($body['d'], $route, $ts, 'resp');
            $inner = json_decode((string) $plain, true);
            $body['data'] = isset($inner['data']) ? $inner['data'] : [];
            if (isset($body['s']) && is_string($plain)) {
                $expect = $this->crypto->sign($route, $ts, $body['d'], 'resp');
                $body['resp_sign_ok'] = hash_equals($expect, (string) $body['s']);
            }
        }
        $body['raw'] = $out;
        return $body;
    }

    /** 造一个结构合法的 V2 机器码 */
    public static function mac($pucType = '3', $seed = null)
    {
        $seed = $seed === null ? (string) random_int(100000, 999999) : (string) $seed;
        $cpu  = '000506EA' . strtoupper(substr(md5($seed), 0, 8));
        $seg2 = (string) abs((int) $seed * 1024 - strlen($cpu));
        $len  = strlen($seg2) + strlen($cpu);
        $s1   = substr((string) abs($len), 0, 2);
        $s4   = (string) $len;
        $s6   = substr((string) abs($len * (int) $s1), -2);
        $s7   = strtoupper(sprintf('%08X', crc32('PC-' . $seed) & 0xFFFFFFFF));
        return base64_encode(implode('-', [$s1, $seg2, $cpu, $s4, (string) $pucType, $s6, $s7, 'MACVER2']));
    }
}
