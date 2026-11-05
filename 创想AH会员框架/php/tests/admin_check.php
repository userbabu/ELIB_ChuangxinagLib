<?php
/**
 * 后台冒烟测试：登录并逐页抓取，命中 PHP 报错关键字即判失败。
 *
 *   php -S 127.0.0.1:8317 -t public     （另开一个窗口）
 *   php tests/admin_check.php [admin地址] [账号] [密码]
 */

$base = isset($argv[1]) ? rtrim($argv[1], '/') : 'http://127.0.0.1:8317/admin/index.php';
$user = isset($argv[2]) ? $argv[2] : 'admin';
$pass = isset($argv[3]) ? $argv[3] : 'admin888';

$jar = tempnam(sys_get_temp_dir(), 'cxah');
$fail = 0;

function fetch($url, $post = null)
{
    global $jar;
    $ch = curl_init($url);
    curl_setopt_array($ch, [
        CURLOPT_RETURNTRANSFER => true,
        CURLOPT_COOKIEJAR      => $jar,
        CURLOPT_COOKIEFILE     => $jar,
        CURLOPT_TIMEOUT        => 20,
        CURLOPT_FOLLOWLOCATION => true,
    ]);
    if ($post !== null) {
        curl_setopt($ch, CURLOPT_POST, true);
        curl_setopt($ch, CURLOPT_POSTFIELDS, http_build_query($post));
    }
    $body = curl_exec($ch);
    if ($body === false) {
        echo "无法访问 {$url}：" . curl_error($ch) . "\n先用 php -S 127.0.0.1:8317 -t public 起服务\n";
        exit(1);
    }
    $code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);
    return [$code, (string) $body];
}

function check($name, $cond, $extra = '')
{
    global $fail;
    echo ($cond ? '  [ OK ] ' : '  [FAIL] ') . $name . ($cond ? '' : '  ' . $extra) . "\n";
    if (!$cond) {
        $fail++;
    }
}

function noPhpError($body)
{
    foreach (['Fatal error', 'Parse error', 'Undefined variable', 'Undefined index', 'Warning:', 'Notice:', 'Deprecated:'] as $needle) {
        if (strpos($body, $needle) !== false) {
            return false;
        }
    }
    return true;
}

echo "== 后台冒烟 {$base} ==\n";

list($code, $body) = fetch($base);
check('未登录时给登录页', $code === 200 && strpos($body, '管理后台登录') !== false, 'HTTP ' . $code);
check('登录页无 PHP 报错', noPhpError($body));

if (!preg_match('/name="csrf" value="([0-9a-f]+)"/', $body, $m)) {
    check('取到 CSRF 令牌', false, $body);
    exit(1);
}
list($code, $body) = fetch($base . '?a=login', ['csrf' => $m[1], 'username' => $user, 'password' => $pass]);
check('登录成功进入概览', $code === 200 && strpos($body, '当前概况') !== false, 'HTTP ' . $code . ' ' . substr(strip_tags($body), 0, 120));

$pages = [
    'home'     => '概览',
    'users'    => '用户查询',
    'keys'     => '生成序列号',
    'machine'  => '机器码查询',
    'logins'   => '登录记录',
    'changes'  => '换机申请',
    'settings' => '接口配置',
];
foreach ($pages as $action => $needle) {
    list($code, $body) = fetch($base . '?a=' . $action);
    check("页面 {$action}", $code === 200 && strpos($body, $needle) !== false && noPhpError($body), 'HTTP ' . $code . ' ' . substr(strip_tags($body), 0, 140));
}

// 带筛选条件的查询
list($code, $body) = fetch($base . '?a=users&keyword=test.local&state=yes&expired=0&puc_type=3');
check('用户筛选', $code === 200 && noPhpError($body), 'HTTP ' . $code);
list($code, $body) = fetch($base . '?a=keys&state=0&puc_type=3&channel_id=0');
check('序列号筛选', $code === 200 && noPhpError($body), 'HTTP ' . $code);

// 取一个真实用户详情页
list($code, $body) = fetch($base . '?a=users');
$uid = 0;
if (preg_match('/\?a=user&id=(\d+)/', $body, $m2)) {
    $uid = (int) $m2[1];
}
if ($uid > 0) {
    list($code, $body) = fetch($base . '?a=user&id=' . $uid);
    check('用户详情', $code === 200 && strpos($body, '资料修改') !== false && noPhpError($body), 'HTTP ' . $code);

    // 保存一次到期时间（走完整 POST 链路）
    if (preg_match('/name="csrf" value="([0-9a-f]+)"/', $body, $m3)) {
        $due = date('Y-m-d H:i:s', strtotime('+15 days'));
        list($code, $body) = fetch($base . '?a=user', [
            'csrf' => $m3[1], 'op' => 'save', 'uid' => $uid, 'due_date' => $due, 'state' => 'yes',
            'state_reason' => '', 'function' => '1', 'limit_change' => '5', 'change_time' => '0',
            'soft_limit' => '', 'remarks' => '冒烟测试', 'referee' => '',
        ]);
        check('保存用户资料', $code === 200 && strpos($body, '已保存') !== false && noPhpError($body), 'HTTP ' . $code . ' ' . substr(strip_tags($body), 0, 160));
    }
}

// 后台生成序列号 -> 列表可见
list($code, $body) = fetch($base . '?a=keys');
preg_match('/name="csrf" value="([0-9a-f]+)"/', $body, $m4);
list($code, $body) = fetch($base . '?a=keys', [
    'csrf' => $m4[1], 'op' => 'make', 'puc_type' => '3', 'function' => '1', 'is_test' => '1',
    'days' => '30', 'count' => '3', 'prefix' => 'SMOKE', 'channel_id' => '0', 'remarks' => '冒烟生成',
]);
check('后台生成序列号', $code === 200 && strpos($body, '已生成 3 个序列号') !== false && noPhpError($body), 'HTTP ' . $code . ' ' . substr(strip_tags($body), 0, 160));
list($code, $body) = fetch($base . '?a=keys&keyword=SMOKE&state=0');
check('新序列号可查询', $code === 200 && preg_match('/SMOKE-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}/', $body) === 1, 'HTTP ' . $code);

$machine = base64_encode('25-126418432-000506EA12345678-19-3-75-ABCD1234-MACVER2');
list($code, $body) = fetch($base . '?a=machine&mac=' . urlencode($machine));
check('机器码解析页', $code === 200 && strpos($body, '机器码解析') !== false && noPhpError($body), 'HTTP ' . $code);

list($code, $body) = fetch($base . '?a=logout');
check('退出登录', $code === 200 && strpos($body, '已退出') !== false, 'HTTP ' . $code);

list($code, $body) = fetch($base . '?a=users');
check('退出后再访问回到登录页', strpos($body, '管理后台登录') !== false || strpos($body, '请先登录后台') !== false, substr(strip_tags($body), 0, 120));

@unlink($jar);
echo $fail === 0 ? "\n后台冒烟全部通过\n" : "\n失败 {$fail} 项\n";
exit($fail === 0 ? 0 : 1);
