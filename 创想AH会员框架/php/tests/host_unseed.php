<?php
/**
 * 宿主测试收尾：恢复验证码开关并清掉测试数据。
 *   php tests/host_unseed.php
 */
require __DIR__ . '/../app/bootstrap.php';
cx_boot();
use CxSoft\Db;

Db::update('cx_setting', ['v' => '1'], ['k' => 'captcha_enabled']);
Db::exec("DELETE FROM amazonhelper_registar_key WHERE email LIKE 'cxah-host%'");
Db::exec("DELETE FROM key_list WHERE key_num LIKE 'HOSTKEY-%'");
Db::exec("DELETE FROM cx_auth_attempt WHERE target LIKE 'cxah-host%'");
echo "已恢复验证码开关并清理宿主测试数据", PHP_EOL;
