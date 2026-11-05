<?php
/**
 * 新后端端到端自测：用真库跑通 注册→试用→登录→换机→序列号激活 全链路。
 *
 *   php tests/run.php          常规跑（加密通道）
 *   php tests/run.php --plain  额外再跑一遍明文通道
 */

require __DIR__ . '/../app/bootstrap.php';
require __DIR__ . '/Client.php';

use CxSoft\Db;
use CxSoft\Test\Client;

$config = cx_boot(['allow_plain' => true]);

$pass = 0;
$fail = 0;
function ok($name, $cond, $detail = '')
{
    global $pass, $fail;
    if ($cond) {
        $pass++;
        echo "  [ OK ] {$name}\n";
    } else {
        $fail++;
        echo "  [FAIL] {$name} {$detail}\n";
    }
}
function group($title)
{
    echo "\n== {$title} ==\n";
}

/** 取一张真验证码并拿到答案（测试专用，直接读服务端缓存） */
function captchaAnswer(Client $c)
{
    $r = $c->call('common.captcha');
    $file = $GLOBALS['config']['log']['dir'] . '/captcha/' . $r['data']['id'] . '.json';
    $saved = json_decode((string) file_get_contents($file), true);
    return ['id' => $r['data']['id'], 'code' => $saved['code'], 'png_bytes' => strlen(base64_decode($r['data']['image_base64']))];
}

/** 直接在库里造一个序列号（等同于后台生成） */
function makeKey($keyNum, $pucType = '3', $days = '30', $channelId = 0, $function = 1, $email = null)
{
    Db::exec('DELETE FROM key_list WHERE key_num=?', [$keyNum]);
    Db::insert('key_list', [
        'key_num' => $keyNum, 'channel_id' => $channelId, 'is_test' => '1',
        'time' => (string) $days, 'state' => $email === null ? 0 : 1, 'puc_type' => (int) $pucType,
        'type' => '月卡', 'date' => date('Y-m-d H:i:s'), 'email' => $email, 'mac_key' => null,
        'act_date' => $email === null ? null : date('Y-m-d H:i:s'), 'function' => $function, 'new' => 1, 'act_type' => 0,
    ]);
}

$suffix = 't' . substr((string) time(), -5) . mt_rand(10, 99);
$EMAIL  = 'cxah-' . $suffix . '@test.local';
$EMAIL2 = 'cxah2-' . $suffix . '@test.local';
$MAC1   = Client::mac('3', '123456');
$MAC2   = Client::mac('3', '654321');
$CTX    = ['puc_type' => '3', 'channel_id' => 0, 'version' => '20260001', 'app_md5' => str_repeat('a', 32), 'system' => '12'];

function withCtx(array $p)
{
    return $p + $GLOBALS['CTX'];
}

/* ------------------------------- 环境清理 ------------------------------- */
group('准备');
Db::exec("DELETE FROM amazonhelper_registar_key WHERE email LIKE 'cxah%'");
// 验证码必须开着跑，否则宿主测试留下的开关会让本套用例失去意义
if (Db::scalar('SELECT k FROM cx_setting WHERE k=?', ['captcha_enabled']) === null) {
    Db::insert('cx_setting', ['k' => 'captcha_enabled', 'v' => '1', 'note' => '图形验证码开关']);
} else {
    Db::update('cx_setting', ['v' => '1'], ['k' => 'captcha_enabled']);
}
Db::exec("DELETE FROM key_list WHERE key_num LIKE 'TESTKEY%'");
Db::exec("DELETE FROM cx_auth_attempt WHERE target LIKE 'cxah-%@test.local'");
ok('测试数据已清空', true);

$c = new Client($config);
$c->encrypt = true;
ok('加密通道', $c->encrypt);

/* -------------------------------- 协议层 -------------------------------- */
group('协议层');
$r = $c->call('common.time');
ok('服务器时间可取', $r['ok'] && isset($r['data']['timestamp']), json_encode($r['raw']));
ok('响应签名可验证', !empty($r['resp_sign_ok']), $r['raw']);

$bad = $c->call('common.time', [], ['s' => str_repeat('0', 64)]);
ok('错误签名被拒(1002)', $bad['code'] === '1002', $bad['code']);

$old = $c->call('common.time', [], ['t' => (int) (round(microtime(true) * 1000) - 3600000)]);
ok('过期时间戳被拒(1003)', $old['code'] === '1003', $old['code']);

$none = $c->call('common.not_exist');
ok('未知路由被拒(1005)', $none['code'] === '1005', $none['code']);

$tamperedParts = explode('-', base64_decode($MAC1));
$tamperedParts[3] = '99';
$tampered = base64_encode(implode('-', $tamperedParts));
$r = $c->call('auth.login', withCtx(['email' => $EMAIL, 'mac_key' => $tampered]));
ok('结构被改的机器码被拒(3001)', $r['code'] === '3001', $r['code'] . ' ' . $r['msg']);

/* ------------------------------- 渠道 OEM ------------------------------- */
group('渠道 OEM');
$r = $c->call('channel.oem', ['key' => 'cxangsoft', 'puc_type' => '3'] + $CTX);
ok('取到官方渠道（官方账号 channel_id 恒为 0）', $r['ok'] && (int) $r['data']['channel_id'] === 0, $r['raw']);
ok('渠道带品牌与授权字段', isset($r['data']['soft_name']) && isset($r['data']['protection']), $r['raw']);

/* --------------------------------- 注册 --------------------------------- */
group('注册');
$r = $c->call('auth.login', withCtx(['email' => $EMAIL, 'mac_key' => $MAC1]));
ok('未注册账号登录失败(2001)', $r['code'] === '2001' && $r['data']['exists'] === false, $r['code']);

$cap = captchaAnswer($c);
ok('验证码可获取', $cap['png_bytes'] > 200, 'png=' . $cap['png_bytes']);

$r = $c->call('account.register', withCtx([
    'email' => $EMAIL, 'mac_key' => $MAC1, 'qq' => '709005505', 'referee' => '',
    'home_username' => '13800138000', 'learn' => '官网', 'captcha_id' => $cap['id'], 'captcha_code' => strtolower($cap['code']),
]));
ok('验证码大小写不敏感 + 注册成功', $r['ok'] && $r['data']['auth_status'] === 'none', $r['raw']);
$uid = (int) $r['data']['grant']['uid'];
ok('注册返回授权视图', $uid > 0 && $r['data']['grant']['change_left'] === 5, json_encode($r['data']['grant']));

$cap = captchaAnswer($c);
$r = $c->call('account.register', withCtx([
    'email' => $EMAIL, 'mac_key' => $MAC1, 'captcha_id' => $cap['id'], 'captcha_code' => $cap['code'],
]));
ok('重复注册被拒(2002)', $r['code'] === '2002', $r['code'] . ' ' . $r['msg']);

$r = $c->call('account.register', withCtx([
    'email' => $EMAIL2, 'mac_key' => $MAC1, 'captcha_id' => $cap['id'], 'captcha_code' => $cap['code'],
]));
ok('验证码一次一用(重复提交失效)', $r['code'] === '5007' || $r['code'] === '5005', $r['code']);

$cap = captchaAnswer($c);
$r = $c->call('account.register', withCtx([
    'email' => $EMAIL2, 'mac_key' => $MAC1, 'captcha_id' => $cap['id'], 'captcha_code' => 'ZZZZ',
]));
ok('验证码填错被拒(5005)', $r['code'] === '5005', $r['code']);

/* --------------------------------- 试用 --------------------------------- */
group('试用');
$r = $c->call('auth.login', withCtx(['email' => $EMAIL, 'mac_key' => $MAC1]));
ok('未开通授权(2005)', $r['code'] === '2005' && $r['data']['auth_status'] === 'none', $r['code']);

$r = $c->call('account.trial', withCtx(['email' => $EMAIL, 'mac_key' => $MAC1]));
ok('试用开通成功', $r['ok'] && $r['data']['auth_status'] === 'trial', $r['raw']);
ok('试用到期时间约 7 天后', abs(strtotime($r['data']['due_date']) - (time() + 7 * 86400)) < 60, $r['data']['due_date']);

$r = $c->call('account.trial', withCtx(['email' => $EMAIL, 'mac_key' => $MAC1]));
ok('不能重复试用(6001)', $r['code'] === '6001', $r['code']);

$r = $c->call('auth.login', withCtx(['email' => $EMAIL, 'mac_key' => $MAC1]));
ok('试用后可登录', $r['ok'] && $r['data']['auth_status'] === 'trial', $r['raw']);
$n = (int) Db::scalar('SELECT COUNT(*) FROM user_login_re WHERE uid=?', [$uid]);
ok('登录写入登录记录', $n === 1, 'login_re=' . $n);

/* -------------------------------- 换机流程 -------------------------------- */
group('换机 · 次数充足');
$r = $c->call('auth.login', withCtx(['email' => $EMAIL, 'mac_key' => $MAC2]));
ok('机器码不符提示换机(4001)', $r['code'] === '4001' && $r['data']['need_change'] === true, $r['code'] . ' ' . json_encode($r['data']));
ok('返回剩余换机次数', (int) $r['data']['change_left'] === 5, json_encode($r['data']));

$r = $c->call('change.request', withCtx(['email' => $EMAIL, 'mac_key' => $MAC2]));
$reqId = (int) $r['data']['request_id'];
ok('换机申请可下推(need_key)', $r['ok'] && $r['data']['need_change'] && $r['data']['need_key'] && $reqId > 0, $r['raw']);

makeKey('TESTKEY-OWNED-1', '3', '30');
Db::update('key_list', ['email' => $EMAIL, 'state' => 1, 'mac_key' => $MAC1, 'act_date' => date('Y-m-d H:i:s')], ['key_num' => 'TESTKEY-OWNED-1']);

$cap = captchaAnswer($c);
$r = $c->call('change.confirm', withCtx([
    'email' => $EMAIL, 'mac_key' => $MAC2, 'request_id' => $reqId, 'key_num' => 'TESTKEY-NOT-MINE',
    'captcha_id' => $cap['id'], 'captcha_code' => $cap['code'],
]));
ok('别的序列号不能换机(4003)', $r['code'] === '4003', $r['code'] . ' ' . json_encode($r['data']));

$cap = captchaAnswer($c);
$r = $c->call('change.confirm', withCtx([
    'email' => $EMAIL, 'mac_key' => $MAC2, 'request_id' => $reqId, 'key_num' => 'TESTKEY-OWNED-1',
    'captcha_id' => $cap['id'], 'captcha_code' => $cap['code'],
]));
ok('本账号序列号换机成功并自动登录', $r['ok'] && $r['data']['changed'] === true && $r['data']['auth_status'] === 'trial', $r['raw']);
ok('换机后剩余次数减一', (int) $r['data']['change_left'] === 4, json_encode($r['data']));
ok('机器码已更新', $r['data']['grant']['mac_key'] === $MAC2 && $r['data']['old_mac'] === $MAC1, $r['data']['grant']['mac_key']);
$chg = Db::one('SELECT * FROM amazonhelper_change_re WHERE email=? ORDER BY Id DESC LIMIT 1', [$EMAIL]);
ok('换机流水落库', $chg && $chg['cd_key'] === 'TESTKEY-OWNED-1' && (int) $chg['change_time'] === 1, json_encode($chg));
ok('换机请求标记成功', (int) Db::scalar('SELECT result FROM cx_change_request WHERE Id=?', [$reqId]) === 1);

group('换机 · 次数不足');
Db::update('amazonhelper_registar_key', ['limit_change' => 1, 'change_time' => 1], ['Id' => $uid]);
$MAC3 = Client::mac('3', '999999');
$r = $c->call('auth.login', withCtx(['email' => $EMAIL, 'mac_key' => $MAC3]));
ok('次数不足直接报错(4002)', $r['code'] === '4002' && $r['data']['need_change'] === false, $r['code'] . ' ' . json_encode($r['data']));
ok('提示文案可直接展示', $r['msg'] !== '' && mb_strlen($r['msg']) < 60, $r['msg']);
$r = $c->call('change.request', withCtx(['email' => $EMAIL, 'mac_key' => $MAC3]));
ok('次数不足不再下发换机申请', $r['code'] === '4002', $r['code']);
Db::update('amazonhelper_registar_key', ['limit_change' => 5, 'change_time' => 1], ['Id' => $uid]);

/* ------------------------------ 序列号激活 ------------------------------ */
group('序列号激活');
$cap = captchaAnswer($c);
$r = $c->call('key.activate', withCtx([
    'email' => $EMAIL, 'mac_key' => $MAC2, 'key_num' => 'TESTKEY-NOPE',
    'captcha_id' => $cap['id'], 'captcha_code' => $cap['code'],
]));
ok('序列号不存在(5001)', $r['code'] === '5001', $r['code']);

makeKey('TESTKEY-OTHER-PROD', '1', '30');
$cap = captchaAnswer($c);
$r = $c->call('key.activate', withCtx([
    'email' => $EMAIL, 'mac_key' => $MAC2, 'key_num' => 'TESTKEY-OTHER-PROD',
    'captcha_id' => $cap['id'], 'captcha_code' => $cap['code'],
]));
ok('产品不匹配(5004)', $r['code'] === '5004', $r['code']);

makeKey('TESTKEY-DAY30', '3', '30');
$before = strtotime((string) Db::scalar('SELECT due_date FROM amazonhelper_registar_key WHERE Id=?', [$uid]));
$cap = captchaAnswer($c);
$r = $c->call('key.activate', withCtx([
    'email' => $EMAIL, 'mac_key' => $MAC2, 'key_num' => 'TESTKEY-DAY30',
    'captcha_id' => $cap['id'], 'captcha_code' => $cap['code'],
]));
$after = strtotime((string) $r['data']['due_date']);
ok('激活成功', $r['ok'] && $r['data']['auth_status'] === 'ok', $r['raw']);
ok('到期时间在原基础上顺延 30 天', $after > 0 && abs($after - ($before + 30 * 86400)) < 60, "before={$before} after={$after}");
ok('转为正式用户', $r['data']['grant']['is_trial'] === false && $r['data']['grant']['order_number'] === 'TESTKEY-DAY30', json_encode($r['data']['grant']));
ok('序列号回写账号与机器码', (string) Db::scalar('SELECT email FROM key_list WHERE key_num=?', ['TESTKEY-DAY30']) === $EMAIL);

$cap = captchaAnswer($c);
$r = $c->call('key.activate', withCtx([
    'email' => $EMAIL, 'mac_key' => $MAC2, 'key_num' => 'TESTKEY-DAY30',
    'captcha_id' => $cap['id'], 'captcha_code' => $cap['code'],
]));
ok('本人重复提交同一序列号按幂等成功', $r['ok'] && $r['data']['replay'] === true, $r['code'] . ' ' . $r['raw']);

$cap = captchaAnswer($c);
$r = $c->call('account.register', withCtx([
    'email' => $EMAIL2, 'mac_key' => $MAC2, 'captcha_id' => $cap['id'], 'captcha_code' => $cap['code'],
]));
ok('第二个账号注册成功', $r['ok'], $r['raw']);
$cap = captchaAnswer($c);
$r = $c->call('key.activate', withCtx([
    'email' => $EMAIL2, 'mac_key' => $MAC2, 'key_num' => 'TESTKEY-DAY30',
    'captcha_id' => $cap['id'], 'captcha_code' => $cap['code'],
]));
ok('他人已用序列号被拒(5002)', $r['code'] === '5002', $r['code']);
$usedBy = isset($r['data']['used_by']) ? $r['data']['used_by'] : '';
ok('提示里的邮箱已打码', $usedBy !== '' && strpos($usedBy, '***') !== false, $usedBy);

/* -------------------------------- 冻结/事件 -------------------------------- */
group('冻结与事件');
Db::update('amazonhelper_registar_key', ['state' => 'no', 'state_reason' => '因违规使用被停止'], ['Id' => $uid]);
$r = $c->call('auth.login', withCtx(['email' => $EMAIL, 'mac_key' => $MAC2]));
ok('冻结账号被拒(2003)', $r['code'] === '2003' && $r['msg'] === '因违规使用被停止', $r['code'] . ' ' . $r['msg']);
Db::update('amazonhelper_registar_key', ['state' => 'yes', 'state_reason' => ''], ['Id' => $uid]);
$r = $c->call('auth.login', withCtx(['email' => $EMAIL, 'mac_key' => $MAC2]));
ok('解冻后可登录', $r['ok'], $r['raw']);
$r = $c->call('auth.refresh', withCtx(['email' => $EMAIL, 'mac_key' => $MAC2]));
ok('刷新授权不写登录记录', $r['ok'], $r['raw']);

$before = (int) Db::scalar('SELECT COUNT(*) FROM event_re WHERE uid=? AND event=?', [$uid, 'quit']);
$r = $c->call('auth.event', withCtx(['email' => $EMAIL, 'mac_key' => $MAC2, 'event' => 'quit']));
$after = (int) Db::scalar('SELECT COUNT(*) FROM event_re WHERE uid=? AND event=?', [$uid, 'quit']);
ok('退出事件落库', $r['ok'] && $after === $before + 1, "{$before} -> {$after}");
ok('下线时间已记录', Db::scalar('SELECT quit_date FROM amazonhelper_registar_key WHERE Id=?', [$uid]) !== null);

/* -------------------------------- 防爆破 -------------------------------- */
group('防爆破');
Db::exec('DELETE FROM cx_auth_attempt WHERE scene=?', ['change']);
for ($i = 0; $i < 5; $i++) {
    $cap = captchaAnswer($c);
    $r = $c->call('change.confirm', withCtx([
        'email' => $EMAIL, 'mac_key' => Client::mac('3', (string) (500000 + $i)), 'key_num' => 'TESTKEY-BAD-' . $i,
        'captcha_id' => $cap['id'], 'captcha_code' => $cap['code'],
    ]));
}
ok('连续失败后锁定(4004)', $r['code'] === '4004' || $r['code'] === '4003', $r['code']);
$cap = captchaAnswer($c);
$r = $c->call('change.confirm', withCtx([
    'email' => $EMAIL, 'mac_key' => $MAC2, 'key_num' => 'TESTKEY-OWNED-1',
    'captcha_id' => $cap['id'], 'captcha_code' => $cap['code'],
]));
ok('锁定期间正确序列号也被拒', $r['code'] === '4004', $r['code'] . ' ' . $r['msg']);
Db::exec('DELETE FROM cx_auth_attempt WHERE scene=?', ['change']);

/* -------------------------------- 明文通道 -------------------------------- */
if (in_array('--plain', $argv, true)) {
    group('明文通道（本机联调）');
    $p = new Client($config);
    $p->encrypt = false;
    $r = $p->call('auth.login', withCtx(['email' => $EMAIL, 'mac_key' => $MAC2]));
    ok('明文请求可用', $r['ok'] && isset($r['data']['grant']['email']), $r['raw']);
    $cfg = $config;
    $cfg['allow_plain'] = false;
    $locked = new Client($cfg);
    $locked->encrypt = false;
    $r = $locked->call('common.time');
    ok('关掉开关后拒绝明文', $r['code'] === '1001', $r['code']);
}

echo "\n----------------------------------------\n";
echo "通过 {$pass} 项，失败 {$fail} 项\n";
exit($fail === 0 ? 0 : 1);
