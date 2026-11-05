<?php
/**
 * 离线宿主测试的前置准备：造好换机/激活要用的序列号与一个次数用完的账号，
 * 并在测试期间关掉图形验证码（宿主无法看图输码，验证码单独用命令覆盖）。
 *
 *   php tests/host_seed.php <运行标记>
 */

require __DIR__ . '/../app/bootstrap.php';

$config = cx_boot();
$run = isset($argv[1]) ? preg_replace('/[^0-9a-zA-Z]/', '', $argv[1]) : (string) time();

use CxSoft\Db;

$email      = 'cxah-host-' . $run . '@test.local';
$emailLock  = 'cxah-hostlock-' . $run . '@test.local';
$keyOwn     = 'HOSTKEY-' . $run . '-OWN';
$keyNew     = 'HOSTKEY-' . $run . '-NEW';
$keyFeat   = 'HOSTKEY-' . $run . '-FEAT';

// 清掉上一轮同名数据
Db::exec('DELETE FROM amazonhelper_registar_key WHERE email IN (?,?)', [$email, $emailLock]);
Db::exec('DELETE FROM key_list WHERE key_num IN (?,?,?)', [$keyOwn, $keyNew, $keyFeat]);
Db::exec('DELETE FROM user_bind WHERE uid IN (SELECT Id FROM (SELECT Id FROM amazonhelper_registar_key WHERE email IN (?,?)) t)', [$email, $emailLock]);

// 已被该账号激活过的序列号（换机验证要用）
Db::insert('key_list', [
    'key_num' => $keyOwn, 'channel_id' => 0, 'is_test' => '1', 'time' => '30', 'state' => 1,
    'puc_type' => 3, 'type' => '月卡', 'date' => date('Y-m-d H:i:s'), 'email' => $email,
    'mac_key' => 'OLD-MAC', 'act_date' => date('Y-m-d H:i:s'), 'function' => 1, 'new' => 1, 'act_type' => 0,
]);
// 未使用的序列号（激活续费要用）
Db::insert('key_list', [
    'key_num' => $keyNew, 'channel_id' => 0, 'is_test' => '1', 'time' => '30', 'state' => 0,
    'puc_type' => 3, 'type' => '月卡', 'date' => date('Y-m-d H:i:s'), 'function' => 1, 'new' => 1, 'act_type' => 0,
]);
// 带选配功能的序列号（act_type=1 只登记功能到期时间，不动主授权）
Db::insert('key_list', [
    'key_num' => $keyFeat, 'channel_id' => 0, 'is_test' => '1', 'time' => '30', 'state' => 0,
    'puc_type' => 3, 'type' => '选配', 'date' => date('Y-m-d H:i:s'), 'function' => 1, 'new' => 1,
    'act_type' => 1, 'soft_limit' => 'asin_monitor,live_stats',
]);
// 换机次数已用完的账号
Db::insert('amazonhelper_registar_key', [
    'email' => $emailLock, 'mac_key' => 'OTHER-MAC', 'is_test' => '1', 'puc_type' => '3',
    'channel_id' => 0, 'state' => 'yes', 'due_date' => date('Y-m-d H:i:s', time() + 86400 * 30),
    'reg_date' => date('Y-m-d H:i:s'), 'limit_change' => 1, 'change_time' => 1, 'function' => 1,
]);

// 测试期间免验证码，并放开同 IP 注册限制
$featNames = '{"asin_monitor":"跟卖监控","live_stats":"实时销量统计"}';
foreach (['captcha_enabled' => '0', 'register_max_ip_day' => '1000', 'function_names' => $featNames] as $k => $v) {
    if (Db::scalar('SELECT k FROM cx_setting WHERE k=?', [$k]) === null) {
        Db::insert('cx_setting', ['k' => $k, 'v' => $v, 'note' => '宿主测试临时设置']);
    } else {
        Db::update('cx_setting', ['v' => $v], ['k' => $k]);
    }
}

echo json_encode([
    'run' => $run, 'email' => $email, 'emailLock' => $emailLock, 'keyOwn' => $keyOwn, 'keyNew' => $keyNew, 'keyFeat' => $keyFeat,
], JSON_UNESCAPED_SLASHES), PHP_EOL;
