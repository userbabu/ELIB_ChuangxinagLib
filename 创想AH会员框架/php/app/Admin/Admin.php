<?php
/**
 * 后台框架：登录、风控、CSRF、布局与表格控件。
 *
 * 只服务本站管理页，与客户端 JSON 接口互不相干。
 * 页面保持朴素：不堆装饰，字段名直说，操作按钮就近放置。
 */

namespace CxSoft\Admin;

use CxSoft\Db;
use CxSoft\Log;

class Admin
{
    const PAGE_SIZE = 30;

    /** @var array */
    public static $config;

    public static function run(array $config)
    {
        self::$config = $config;
        session_name('CXAHAD');
        session_start();

        $action = isset($_GET['a']) ? preg_replace('/[^a-z_]/', '', (string) $_GET['a']) : 'home';

        header('Content-Type: text/html; charset=utf-8');
        if ($action === 'login' || $action === 'logout') {
            Pages::{$action}(self::contextFor(false));
            return;
        }
        if (!self::loggedIn()) {
            Pages::login(self::contextFor(false), '请先登录后台');
            return;
        }
        if ($_SERVER['REQUEST_METHOD'] === 'POST') {
            self::checkCsrf();
        }
        $method = 'page' . ucfirst($action);
        if (!method_exists('CxSoft\\Admin\\Pages', $method)) {
            self::halt('页面不存在');
        }
        try {
            Pages::{$method}(self::contextFor(true));
        } catch (\Throwable $e) {
            Log::error('后台异常 ' . $action . ' ' . get_class($e) . ': ' . $e->getMessage() . ' @' . $e->getFile() . ':' . $e->getLine());
            if (!empty($config['debug_plain']) && getenv('CXAH_DEV') === '1') {
                self::halt('后台异常：' . get_class($e) . ' ' . $e->getMessage() . ' @' . $e->getFile() . ':' . $e->getLine());
            }
            self::flash('error', '操作失败，请重试或联系开发查看日志。');
            header('Location: ?a=home');
        }
    }

    private static function contextFor($requireLogin)
    {
        return new Page(self::admin(), $requireLogin);
    }

    public static function admin()
    {
        return isset($_SESSION['cxah_admin']) ? $_SESSION['cxah_admin'] : null;
    }

    public static function loggedIn()
    {
        $a = self::admin();
        return is_array($a) && !empty($a['username']);
    }

    /* ------------------------------ 登录 ------------------------------ */

    /** 旧库口径：password = md5(明文密码 + 用户名) */
    public static function tryLogin($username, $password)
    {
        $username = trim((string) $username);
        $ip       = self::ip();
        if (self::isBlocked($ip, 'admin')) {
            return ['ok' => false, 'msg' => '登录失败次数过多，请 30 分钟后再试。'];
        }
        $row = Db::one('SELECT * FROM admin_login WHERE username=? LIMIT 1', [$username]);
        if (!$row || (string) $row['password'] !== md5($password . $username)) {
            self::countFail($ip, 'admin');
            return ['ok' => false, 'msg' => '账号或密码不正确。'];
        }
        if ((int) $row['state'] === 1) {
            return ['ok' => false, 'msg' => '该后台账号已停用。'];
        }
        self::clearFail($ip, 'admin');
        $_SESSION['cxah_admin'] = ['username' => $row['username'], 'uid' => (int) $row['admin_uid'], 'type' => (int) $row['UserType']];
        Db::update('admin_login', ['LoginDate' => date('Y-m-d H:i:s'), 'Login_num' => (int) $row['Login_num'] + 1], ['admin_uid' => (int) $row['admin_uid']]);
        Db::insert('admin_login_re', ['username' => $row['username'], 'date' => date('Y-m-d H:i:s'), 'ua' => self::ua(), 'ip' => $ip]);
        self::audit(1, '登录后台', '账号 ' . $row['username']);
        return ['ok' => true, 'msg' => ''];
    }

    public static function logout()
    {
        if (self::loggedIn()) {
            self::audit(2, '退出后台', '账号 ' . self::admin()['username']);
        }
        $_SESSION = [];
        session_destroy();
    }

    /* ------------------------------ 风控 ------------------------------ */

    private static function isBlocked($ip, $name)
    {
        $row = Db::one('SELECT * FROM admin_risk WHERE ip=? AND name=? LIMIT 1', [substr($ip, 0, 15), $name]);
        if (!$row) {
            return false;
        }
        return (int) $row['status'] === 1 && ($row['expire'] == 0 || (int) $row['expire'] > time());
    }

    private static function countFail($ip, $name)
    {
        $row = Db::one('SELECT * FROM admin_risk WHERE ip=? AND name=? LIMIT 1', [substr($ip, 0, 15), $name]);
        if ($row) {
            $count = (int) $row['count'] + 1;
            Db::update('admin_risk', [
                'count'     => $count,
                'status'    => $count >= 5 ? 1 : 0,
                'expire'    => $count >= 5 ? time() + 1800 : 0,
                'last_date' => date('Y-m-d H:i:s'),
            ], ['admin_risk_id' => (int) $row['admin_risk_id']]);
            return;
        }
        Db::insert('admin_risk', [
            'ip' => substr($ip, 0, 15), 'ua' => self::ua(), 'date' => date('Y-m-d H:i:s'),
            'status' => 0, 'last_date' => date('Y-m-d H:i:s'), 'count' => 1, 'name' => $name, 'expire' => 0,
        ]);
    }

    private static function clearFail($ip, $name)
    {
        Db::exec('DELETE FROM admin_risk WHERE ip=? AND name=?', [substr($ip, 0, 15), $name]);
    }

    /* ---------------------------- CSRF / 审计 ---------------------------- */

    public static function csrf()
    {
        if (empty($_SESSION['cxah_csrf'])) {
            $_SESSION['cxah_csrf'] = bin2hex(random_bytes(16));
        }
        return $_SESSION['cxah_csrf'];
    }

    private static function checkCsrf()
    {
        $token = isset($_POST['csrf']) ? (string) $_POST['csrf'] : '';
        if (!hash_equals((string) (isset($_SESSION['cxah_csrf']) ? $_SESSION['cxah_csrf'] : ''), $token)) {
            self::halt('页面已过期，请刷新后重新提交。');
        }
    }

    public static function audit($type, $title, $text)
    {
        $a = self::admin();
        try {
            Db::insert('admin_log', [
                'log_title' => mb_substr((string) $title, 0, 200),
                'log_text'  => mb_substr((string) $text, 0, 480),
                'date'      => date('Y-m-d H:i:s'),
                'username'  => $a ? $a['username'] : 'system',
                'type'      => (int) $type,
                'ip'        => self::ip(),
                'ua'        => self::ua(),
            ]);
        } catch (\Throwable $e) {
            Log::error('后台审计写库失败 ' . $e->getMessage());
        }
    }

    /* ------------------------------ 提示 ------------------------------ */

    public static function flash($kind, $text)
    {
        $_SESSION['cxah_flash'] = ['kind' => $kind, 'text' => $text];
    }

    public static function takeFlash()
    {
        if (empty($_SESSION['cxah_flash'])) {
            return null;
        }
        $f = $_SESSION['cxah_flash'];
        unset($_SESSION['cxah_flash']);
        return $f;
    }

    public static function halt($text)
    {
        echo self::layout('提示', '<p>' . self::h($text) . '</p><p><a href="javascript:history.back()">返回</a> <a href="?a=home">回首页</a></p>', false);
        exit;
    }

    /* ------------------------------ 工具 ------------------------------ */

    public static function h($text)
    {
        return htmlspecialchars((string) $text, ENT_QUOTES, 'UTF-8');
    }

    public static function ip()
    {
        foreach (['HTTP_X_FORWARDED_FOR', 'HTTP_X_REAL_IP', 'REMOTE_ADDR'] as $k) {
            if (!empty($_SERVER[$k])) {
                $ip = trim(explode(',', (string) $_SERVER[$k])[0]);
                if (filter_var($ip, FILTER_VALIDATE_IP)) {
                    return $ip;
                }
            }
        }
        return '0.0.0.0';
    }

    private static function ua()
    {
        return mb_substr((string) (isset($_SERVER['HTTP_USER_AGENT']) ? $_SERVER['HTTP_USER_AGENT'] : ''), 0, 250);
    }

    public static function post($key, $default = '')
    {
        return isset($_POST[$key]) ? trim((string) $_POST[$key]) : $default;
    }

    public static function get($key, $default = '')
    {
        return isset($_GET[$key]) ? trim((string) $_GET[$key]) : $default;
    }

    public static function pageNow()
    {
        $p = (int) self::get('p', '1');
        return $p < 1 ? 1 : $p;
    }

    /** 生成带当前筛选条件的翻页地址 */
    public static function pageUrl($p)
    {
        $query = $_GET;
        $query['p'] = (int) $p;
        return '?' . http_build_query($query);
    }

    /* ------------------------------ 布局 ------------------------------ */

    public static function layout($title, $body, $withMenu = true)
    {
        $menu = [
            'home'    => '概览',
            'users'   => '用户管理',
            'keys'    => '序列号管理',
            'machine' => '机器码查询',
            'logins'  => '登录记录',
            'changes' => '换机记录',
            'settings'=> '接口配置',
        ];
        $a = self::admin();
        $who = $a ? Admin::h($a['username']) . ' <a href="?a=logout" style="color:#c9d3e4;margin-left:12px">退出</a>' : '未登录';
        $nav = '';
        if ($withMenu) {
            foreach ($menu as $action => $label) {
                $current = (isset($_GET['a']) ? $_GET['a'] : 'home') === $action;
                $nav .= '<a' . ($current ? ' class="on"' : '') . ' href="?a=' . $action . '">' . self::h($label) . '</a>';
            }
        }
        $flash = self::takeFlash();
        $notice = $flash ? '<div class="msg ' . self::h($flash['kind']) . '">' . self::h($flash['text']) . '</div>' : '';

        return <<<HTML
<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{$title} - 创想AH会员管理</title>
<style>
*{box-sizing:border-box}
body{margin:0;padding:0;background:#f5f6f7;color:#222;font:14px/1.6 "Microsoft YaHei",Arial,sans-serif}
header{background:#2b3a55;color:#fff;padding:12px 24px;display:flex;justify-content:space-between;align-items:center}
header b{font-size:16px}
header span{color:#c9d3e4;font-size:13px}
nav{background:#fff;border-bottom:1px solid #e3e5e8;padding:0 24px}
nav a{display:inline-block;padding:11px 16px;color:#3b4a63;text-decoration:none;border-bottom:2px solid transparent}
nav a.on{color:#111;border-bottom-color:#2b3a55;font-weight:bold}
main{padding:20px 24px 48px}
.card{background:#fff;border:1px solid #e3e5e8;padding:16px 18px;margin-bottom:18px}
.card h2{margin:0 0 12px;font-size:15px;color:#2b3a55}
table{width:100%;border-collapse:collapse}
th,td{border-bottom:1px solid #eceef0;padding:8px 10px;text-align:left;vertical-align:top;word-break:break-all}
th{background:#fafbfc;color:#555;font-weight:normal;white-space:nowrap}
td.num{text-align:right;font-family:Consolas,monospace}
input,select,textarea{border:1px solid #cfd4da;padding:6px 8px;font-size:14px;background:#fff;max-width:100%}
input[type=submit],button{background:#2b3a55;color:#fff;border:0;padding:8px 16px;cursor:pointer}
input[type=submit]:hover,button:hover{background:#3b4f73}
a.act{color:#2b3a55;margin-right:10px}
.msg{padding:10px 14px;margin-bottom:16px;border:1px solid #cfe0f1;background:#eff6fc}
.msg.error{border-color:#f0c9c9;background:#fdf0f0}
.grid{display:flex;gap:24px;flex-wrap:wrap}
.kv{min-width:210px}
.kv dt{color:#777;font-size:13px}
.kv dd{margin:2px 0 12px;font-size:15px}
.pager{margin-top:12px;color:#666}
.pager a{margin-right:8px}
.tag{border:1px solid #cfd4da;padding:1px 8px;font-size:12px;color:#444;white-space:nowrap}
.tag.no{border-color:#e0b4b4;color:#a33}
.tag.yes{border-color:#b6d7b6;color:#266}
small{color:#888}
</style>
</head>
<body>
<header>
  <b>创想AH会员管理后台</b>
  <span>{$who}</span>
</header>
<nav>{$nav}</nav>
<main>
{$notice}
{$body}
</main>
</body>
</html>
HTML;
    }

    /** 表格渲染：$rows 为二维数组，值可是 HTML */
    public static function table(array $headers, array $rows)
    {
        if (!$rows) {
            return '<p><small>暂无数据</small></p>';
        }
        $head = '';
        foreach ($headers as $t) {
            $head .= '<th>' . self::h($t) . '</th>';
        }
        $body = '';
        foreach ($rows as $row) {
            $cells = '';
            foreach ($row as $cell) {
                $cells .= '<td>' . (is_array($cell) ? self::h(implode(' ', $cell)) : (string) $cell) . '</td>';
            }
            $body .= '<tr>' . $cells . '</tr>';
        }
        return '<table><thead><tr>' . $head . '</tr></thead><tbody>' . $body . '</tbody></table>';
    }

    public static function pager($total)
    {
        $pages = (int) ceil($total / self::PAGE_SIZE);
        $now   = self::pageNow();
        if ($pages <= 1) {
            return '<div class="pager">共 ' . (int) $total . ' 条</div>';
        }
        $html = '<div class="pager">共 ' . (int) $total . ' 条，第 ' . $now . '/' . $pages . ' 页 ';
        if ($now > 1) {
            $html .= '<a href="' . self::h(self::pageUrl($now - 1)) . '">上一页</a>';
        }
        if ($now < $pages) {
            $html .= '<a href="' . self::h(self::pageUrl($now + 1)) . '">下一页</a>';
        }
        return $html . '</div>';
    }

    public static function csrfField()
    {
        return '<input type="hidden" name="csrf" value="' . self::h(self::csrf()) . '">';
    }

    /** 输出一页（统一在这里 echo，避免各处漏掉布局） */
    public static function echoAdmin($html)
    {
        echo $html;
    }
}
