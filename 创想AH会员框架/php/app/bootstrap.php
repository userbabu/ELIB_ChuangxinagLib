<?php
/**
 * 新后端启动引导：类加载、时区、配置、日志、数据库。
 * 用法：在入口文件里 require 本文件后调用 cx_boot()。
 */

if (!defined('CXAH_ROOT')) {
    define('CXAH_ROOT', dirname(__DIR__));
}

spl_autoload_register(function ($class) {
    if (strpos($class, 'CxSoft\\') !== 0) {
        return;
    }
    $rel = str_replace('\\', '/', substr($class, strlen('CxSoft\\')));
    // CxSoft\Service\Grant -> app/Service/Grant.php
    // CxSoft\Repo          -> app/Core/Repo.php
    foreach (['/' . $rel . '.php', '/Core/' . $rel . '.php'] as $suffix) {
        $path = CXAH_ROOT . '/app' . $suffix;
        if (is_file($path)) {
            require $path;
            return;
        }
    }
});

/**
 * @param array $override 测试用配置覆盖
 * @return array 生效配置
 */
function cx_boot(array $override = [])
{
    date_default_timezone_set('Asia/Shanghai');
    error_reporting(E_ALL);
    ini_set('display_errors', '0');
    ini_set('log_errors', '1');

    // 本机联调：把报错直接显示到页面，便于冒烟测试抓取
    $dev = getenv('CXAH_DEV') === '1';
    if ($dev) {
        ini_set('display_errors', '1');
    }

    $config = require CXAH_ROOT . '/app/config.php';
    if ($override) {
        $config = array_replace_recursive($config, $override);
    }

    // 只有显式打开调试开关时才允许明文请求，线上默认为加密通道
    if (!empty($config['debug_plain']) && getenv('CXAH_DEV') === '1') {
        $config['allow_plain'] = true;
    }

    \CxSoft\Db::init($config['db']);
    \CxSoft\Log::init($config['log']);
    \CxSoft\Captcha::init(array_merge($config['captcha'], ['dir' => $config['log']['dir']]));

    set_exception_handler(function (\Throwable $e) {
        \CxSoft\Log::error('未捕获异常 ' . get_class($e) . ': ' . $e->getMessage() . ' @' . $e->getFile() . ':' . $e->getLine());
    });

    return $config;
}
