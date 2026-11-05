<?php
/**
 * 创想AH会员框架 · 新后端配置
 *
 * 本文件是「出厂默认值」，线上要改的东西一律放 cx_setting 表或 config.local.php，
 * 这样发布新版代码不会覆盖现场配置。
 */

$config = [
    'db' => [
        'host'    => '127.0.0.1',
        'port'    => 3306,
        'name'    => 'cxang_key_platform',
        'user'    => 'root',
        'pass'    => 'root',
        'charset' => 'utf8mb4',
    ],

    // 协议密钥：与易语言支持库初始化时传入的产品密钥保持一致
    'app_secret' => 'CXAH-DEV-SECRET-CHANGE-ME',

    // 明文调试通道：本机联调时置 true（请求直接带 data、不签名）。线上必须 false。
    'allow_plain' => false,

    // 开发便利开关：置 true 后，只有同时设了环境变量 CXAH_DEV=1 才临时打开明文通道。
    // 线上部署不设该环境变量即可，正式通道始终只认加密信封。
    'debug_plain' => true,

    // 请求时间戳允许的偏差（毫秒）
    'time_window' => ['past' => 180000, 'future' => 60000],

    'trial' => [
        'days'        => 7,      // 试用天数
        'once_per'    => 'email', // 试用唯一性判定：email=按账号 email+mac=按账号加机器码
    ],

    'change' => [
        'default_limit' => 5,     // 注册时写入的默认可换机次数
        'need_key'      => true,  // 换机是否必须提供该账号激活过的序列号
        'max_fail'      => 5,     // 同一目标连续失败次数上限
        'lock_minutes'  => 30,    // 超限后锁定时长
    ],

    'captcha' => [
        'enabled'      => true,
        'ttl_seconds'  => 300,
        'width'        => 130,
        'height'       => 44,
        'length'       => 4,
    ],

    // 注册时是否要求邮箱验证码之外的风控（同IP注册数上限）
    'register' => [
        'max_per_ip_per_day' => 20,
    ],

    'log' => [
        'dir'       => __DIR__ . '/../runtime',
        'to_db'     => true,      // 异常写入 server_debug_log
        'sql_error' => true,      // 数据库异常写 log 表
    ],
];

$local = __DIR__ . '/config.local.php';
if (is_file($local)) {
    $override = require $local;
    if (is_array($override)) {
        $config = array_replace_recursive($config, $override);
    }
}

return $config;
