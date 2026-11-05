<?php
/**
 * 后台页面：概览 / 用户 / 序列号 / 机器码 / 登录记录 / 换机记录 / 接口配置。
 *
 * 全部读写都落在旧库表上，不引入新表语义（新增表只用于新后端自身的流水与配置）。
 */

namespace CxSoft\Admin;

use CxSoft\Db;
use CxSoft\MachineCode;
use CxSoft\Repo;
use CxSoft\Service\Grant;

class Pages
{
    const PRODUCTS = [
        '1' => '亚马逊助手',
        '2' => 'UPC生成器',
        '3' => 'ASIN采集软件',
        '4' => '关键词助手',
    ];

    const FUNCTIONS = [
        '0'   => '企业版(旧)',
        '1'   => '企业版',
        '2'   => '高级版',
        '3'   => '标准版',
    ];

    /* ------------------------------ 登录 ------------------------------ */

    public static function login(Page $ctx, $notice = '')
    {
        if ($_SERVER['REQUEST_METHOD'] === 'POST') {
            $r = Admin::tryLogin(Admin::post('username'), (string) (isset($_POST['password']) ? $_POST['password'] : ''));
            if ($r['ok']) {
                header('Location: ?a=home');
                return;
            }
            $notice = $r['msg'];
        }
        $err = $notice === '' ? '' : '<div class="msg error">' . Admin::h($notice) . '</div>';
        $csrf = Admin::csrfField();
        $body = <<<HTML
<div class="card" style="max-width:420px;margin:48px auto">
  <h2>管理后台登录</h2>
  {$err}
  <form method="post" action="?a=login">
    {$csrf}
    <p><label>账号<br><input name="username" style="width:100%" autocomplete="off"></label></p>
    <p><label>密码<br><input name="password" type="password" style="width:100%"></label></p>
    <p><input type="submit" value="登录"></p>
  </form>
  <p><small>初始账号见 sql/seed.sql，登录后请立刻修改密码。连续失败 5 次将临时锁定本机 IP。</small></p>
</div>
HTML;
        Admin::echoAdmin(Admin::layout('登录后台', $body, false));
    }

    public static function logout(Page $ctx)
    {
        Admin::logout();
        Admin::echoAdmin(Admin::layout('已退出', '<p>已退出登录。<a href="?a=login">重新登录</a></p>', false));
    }

    /* ------------------------------ 概览 ------------------------------ */

    public static function pageHome(Page $ctx)
    {
        $today = date('Y-m-d');
        $stats = [
            '账号总数'      => (int) Db::scalar('SELECT COUNT(*) FROM amazonhelper_registar_key'),
            '今日注册'      => (int) Db::scalar('SELECT COUNT(*) FROM amazonhelper_registar_key WHERE DATE(reg_date)=?', [$today]),
            '今日登录'      => (int) Db::scalar('SELECT COUNT(*) FROM user_login_re WHERE DATE(date)=?', [$today]),
            '今日换机'      => (int) Db::scalar('SELECT COUNT(*) FROM amazonhelper_change_re WHERE DATE(date)=?', [$today]),
            '换机待验证'    => (int) Db::scalar('SELECT COUNT(*) FROM cx_change_request WHERE result=0'),
            '序列号总数'    => (int) Db::scalar('SELECT COUNT(*) FROM key_list'),
            '未使用序列号'  => (int) Db::scalar('SELECT COUNT(*) FROM key_list WHERE state=0'),
            '7天内到期'     => (int) Db::scalar('SELECT COUNT(*) FROM amazonhelper_registar_key WHERE due_date BETWEEN ? AND ?', [$ctx->now(), date('Y-m-d H:i:s', strtotime('+7 days'))]),
        ];
        $dl = '';
        foreach ($stats as $k => $v) {
            $dl .= '<dt>' . Admin::h($k) . '</dt><dd>' . $v . '</dd>';
        }

        $recent = Db::all('SELECT email,puc_type,reg_date,due_date,state FROM amazonhelper_registar_key ORDER BY Id DESC LIMIT 10');
        $rows = [];
        foreach ($recent as $u) {
            $rows[] = [
                '<a class="act" href="?a=users&keyword=' . Admin::h(rawurlencode((string) $u['email'])) . '">' . Admin::h($u['email']) . '</a>',
                self::product($u['puc_type']),
                Admin::h($u['reg_date']),
                Admin::h($u['due_date']),
                self::stateTag($u['state']),
            ];
        }

        $changes = Db::all('SELECT email,puc_type,key_num,result,date FROM cx_change_request ORDER BY Id DESC LIMIT 10');
        $crows = [];
        foreach ($changes as $c) {
            $crows[] = [
                Admin::h($c['email']),
                self::product($c['puc_type']),
                '<code>' . Admin::h($c['key_num']) . '</code>',
                self::changeResult((int) $c['result']),
                Admin::h($c['date']),
            ];
        }

        Admin::echoAdmin(Admin::layout('概览', '<div class="grid"><div class="card kv" style="flex:1"><h2>当前概况（' . date('Y-m-d H:i') . '）</h2><dl>' . $dl . '</dl></div>'
            . '<div class="card" style="flex:2"><h2>最新注册</h2>' . Admin::table(['邮箱', '产品', '注册时间', '到期时间', '状态'], $rows) . '</div></div>'
            . '<div class="card"><h2>最近换机申请</h2>' . Admin::table(['邮箱', '产品', '验证序列号', '结果', '申请时间'], $crows) . '</div>'));
    }

    /* ----------------------------- 用户管理 ----------------------------- */

    public static function pageUsers(Page $ctx)
    {
        $where = '1=1';
        $args  = [];
        $kw    = $ctx->q('keyword');
        if ($kw !== '') {
            if (MachineCode::looksLike($kw)) {
                $where .= ' AND mac_key=?';
                $args[] = $kw;
            } else {
                $where .= ' AND email LIKE ?';
                $args[] = '%' . $kw . '%';
            }
        }
        if (($pt = $ctx->q('puc_type')) !== '') {
            $where .= ' AND puc_type=?';
            $args[] = $pt;
        }
        if (($st = $ctx->q('state')) !== '') {
            $where .= ' AND state=?';
            $args[] = $st;
        }
        if ($ctx->q('expired') === '1') {
            $where .= ' AND due_date IS NOT NULL AND due_date < NOW()';
        }
        if ($ctx->q('expired') === '0') {
            $where .= ' AND (due_date IS NULL OR due_date >= NOW())';
        }
        if (($ch = $ctx->q('channel_id')) !== '') {
            $where .= ' AND channel_id=?';
            $args[] = (int) $ch;
        }

        if ($_SERVER['REQUEST_METHOD'] === 'POST' && Admin::post('op') === 'save') {
            self::saveUser($ctx);
            return;
        }

        $total = (int) Db::scalar('SELECT COUNT(*) FROM amazonhelper_registar_key WHERE ' . $where, $args);
        $start = (Admin::pageNow() - 1) * Admin::PAGE_SIZE;
        $list  = Db::all(
            'SELECT * FROM amazonhelper_registar_key WHERE ' . $where . ' ORDER BY Id DESC LIMIT ' . $start . ',' . Admin::PAGE_SIZE,
            $args
        );

        $rows = [];
        foreach ($list as $u) {
            $left = Grant::changeLeft($u);
            $rows[] = [
                (int) $u['Id'],
                '<a class="act" href="?a=user&id=' . (int) $u['Id'] . '">' . Admin::h($u['email']) . '</a>',
                self::product($u['puc_type']),
                $u['channel_id'] ? (int) $u['channel_id'] : '官方',
                '<code>' . self::macBrief($u['mac_key']) . '</code>',
                Admin::h($u['due_date']),
                self::stateTag($u['state']),
                ((string) $u['is_test'] === Grant::trialValue() ? '<span class="tag">试用</span>' : '<span class="tag yes">正式</span>'),
                $left . ' / ' . (int) $u['limit_change'],
                '<a class="act" href="?a=user&id=' . (int) $u['Id'] . '">编辑</a>',
            ];
        }

        $filter = self::userFilter($ctx);
        Admin::echoAdmin(Admin::layout('用户管理', '<div class="card"><h2>用户查询</h2>' . $filter . '</div>'
            . '<div class="card"><h2>用户列表</h2>' . Admin::table(
                ['UID', '邮箱', '产品', '渠道', '绑定机器码', '到期时间', '状态', '版本', '剩余换机', '操作'],
                $rows
            ) . Admin::pager($total) . '</div>'));
    }

    private static function userFilter(Page $ctx)
    {
        $kw = Admin::h($ctx->q('keyword'));
        $sel = function ($name, $options, $current) {
            $html = '<select name="' . $name . '"><option value="">全部</option>';
            foreach ($options as $value => $label) {
                $on = ((string) $value === (string) $current) ? ' selected' : '';
                $html .= '<option value="' . Admin::h($value) . '"' . $on . '>' . Admin::h($label) . '</option>';
            }
            return $html . '</select>';
        };
        return '<form method="get">'
            . '<input name="a" value="users" type="hidden">'
            . '<p>邮箱或机器码 <input name="keyword" value="' . $kw . '" style="width:320px" placeholder="填机器码时按整串精确匹配">'
            . ' 产品 ' . $sel('puc_type', self::PRODUCTS, $ctx->q('puc_type'))
            . ' 状态 ' . $sel('state', ['yes' => '正常', 'no' => '已冻结'], $ctx->q('state'))
            . ' 授权 ' . $sel('expired', ['1' => '已到期', '0' => '未到期'], $ctx->q('expired'))
            . ' 渠道ID <input name="channel_id" style="width:80px" value="' . Admin::h($ctx->q('channel_id')) . '">'
            . ' <input type="submit" value="查询"></p></form>';
    }

    public static function pageUser(Page $ctx)
    {
        // 保存时表单 POST 到 ?a=user（不带 id），因此优先取提交值
        $uid = (int) ($_SERVER['REQUEST_METHOD'] === 'POST' ? Admin::post('uid') : Admin::get('id'));
        $u   = Repo::userById($uid);
        if (!$u) {
            Admin::halt('用户不存在或已删除');
        }

        if ($_SERVER['REQUEST_METHOD'] === 'POST') {
            self::saveUser($ctx);
            return;
        }

        $binds   = Db::all('SELECT * FROM user_bind WHERE uid=? ORDER BY user_bind_id DESC', [$uid]);
        $bindRows = [];
        foreach ($binds as $b) {
            $bindRows[] = [
                self::product($b['puc_type']),
                '<code>' . Admin::h($b['mackey']) . '</code>',
                Admin::h($b['bind_date']),
                (int) $b['change_count'] . ' / ' . (int) $b['limit_change'],
            ];
        }

        $keys = Db::all('SELECT key_num,time,`function`,state,act_date FROM key_list WHERE email=? ORDER BY Id DESC LIMIT 20', [$u['email']]);
        $keyRows = [];
        foreach ($keys as $k) {
            $keyRows[] = [
                '<code>' . Admin::h($k['key_num']) . '</code>',
                Admin::h($k['time']),
                self::version($k['function']),
                self::keyState((int) $k['state']),
                Admin::h($k['act_date']),
            ];
        }

        $logins = Db::all('SELECT date,mackey,ip,ip_local,version,puctype FROM user_login_re WHERE uid=? ORDER BY user_login_re_id DESC LIMIT 20', [$uid]);
        $loginRows = [];
        foreach ($logins as $l) {
            $loginRows[] = [Admin::h($l['date']), '<code>' . self::macBrief($l['mackey']) . '</code>', Admin::h($l['ip']), Admin::h($l['ip_local']), Admin::h($l['version'])];
        }

        $edits = Db::all('SELECT op,diff,operator,date FROM amazonhelper_edit_re WHERE uid=? ORDER BY Id DESC LIMIT 20', [$uid]);
        $editRows = [];
        foreach ($edits as $e) {
            $editRows[] = [Admin::h($e['op']), '<code>' . Admin::h(mb_substr((string) $e['diff'], 0, 300)) . '</code>', Admin::h($e['operator']), Admin::h($e['date'])];
        }

        $csrf = Admin::csrfField();
        $left = Grant::changeLeft($u);
        $v = [];
        foreach (['due_date', 'state_reason', 'soft_limit', 'remarks', 'referee'] as $field) {
            $v[$field] = Admin::h($u[$field]);
        }
        $stateYes = (string) $u['state'] === 'no' ? '' : ' selected';
        $stateNo  = (string) $u['state'] === 'no' ? ' selected' : '';
        $version  = self::options(self::FUNCTIONS, $u['function']);

        $form = '<form method="post" action="?a=user">' . $csrf
            . '<input type="hidden" name="op" value="save"><input type="hidden" name="uid" value="' . $uid . '">'
            . '<p><label>到期时间 <input name="due_date" value="' . $v['due_date'] . '" style="width:200px" placeholder="2026-12-31 23:59:59"></label>'
            . ' <label>状态 <select name="state"><option value="yes"' . $stateYes . '>正常</option><option value="no"' . $stateNo . '>已冻结</option></select></label>'
            . ' <label>冻结说明 <input name="state_reason" value="' . $v['state_reason'] . '" style="width:260px"></label></p>'
            . '<p><label>版本 <select name="function">' . $version . '</select></label>'
            . ' <label>可换机总次数 <input name="limit_change" value="' . (int) $u['limit_change'] . '" style="width:80px"></label>'
            . ' <label>已用换机次数 <input name="change_time" value="' . (int) $u['change_time'] . '" style="width:80px"></label>'
            . ' <label>增配权限 <input name="soft_limit" value="' . $v['soft_limit'] . '" style="width:240px"></label></p>'
            . '<p><label>备注 <input name="remarks" value="' . $v['remarks'] . '" style="width:420px"></label>'
            . ' <label>关联账号 <input name="referee" value="' . $v['referee'] . '" style="width:220px"></label></p>'
            . '<p><input type="submit" value="保存修改"></p></form>';

        $head = '<div class="card"><h2>' . Admin::h($u['email']) . '　' . self::product($u['puc_type']) . '　' . self::stateTag($u['state']) . '</h2>'
            . '<div class="grid"><div class="kv"><dt>UID</dt><dd>' . $uid . '</dd>'
            . '<dt>注册时间</dt><dd>' . Admin::h($u['reg_date']) . '</dd>'
            . '<dt>最后登录</dt><dd>' . Admin::h($u['login_date']) . '（累计 ' . (int) $u['login_num'] . ' 次）</dd>'
            . '<dt>注册IP</dt><dd>' . Admin::h($u['ip']) . ' ' . Admin::h($u['ip_location']) . '</dd></div>'
            . '<div class="kv"><dt>绑定机器码</dt><dd><code>' . Admin::h($u['mac_key']) . '</code></dd>'
            . '<dt>换机配额</dt><dd>剩余 ' . $left . ' 次（总 ' . (int) $u['limit_change'] . ' / 已用 ' . (int) $u['change_time'] . '）</dd>'
            . '<dt>最后序列号</dt><dd>' . Admin::h($u['order_number']) . '</dd>'
            . '<dt>下线时间</dt><dd>' . Admin::h($u['quit_date']) . '</dd></div></div>'
            . '<h2>资料修改</h2>' . $form . '</div>';

        $unbind = '<form method="post" action="?a=user" style="display:inline">' . $csrf
            . '<input type="hidden" name="op" value="unbind"><input type="hidden" name="uid" value="' . $uid . '">'
            . '<input type="submit" value="解绑本机（下次登录需重新验证换机）" onclick="return confirm(\'确认解绑该账号的机器码？\')"></form>';

        Admin::echoAdmin(Admin::layout('用户详情', $head . $unbind
            . '<div class="card"><h2>机器码绑定</h2>' . Admin::table(['产品', '机器码', '绑定时间', '换机次数'], $bindRows) . '</div>'
            . '<div class="card"><h2>该账号序列号</h2>' . Admin::table(['序列号', '时长', '版本', '状态', '激活时间'], $keyRows) . '</div>'
            . '<div class="card"><h2>最近登录记录</h2>' . Admin::table(['时间', '机器码', 'IP', 'IP归属', '版本'], $loginRows) . '</div>'
            . '<div class="card"><h2>资料变更记录</h2>' . Admin::table(['类型', '变更明细', '操作人', '时间'], $editRows) . '</div>'));
    }

    private static function saveUser(Page $ctx)
    {
        $uid = (int) Admin::post('uid');
        $old = Repo::userById($uid);
        if (!$old) {
            Admin::halt('用户不存在');
        }
        $op = Admin::post('op');

        if ($op === 'unbind') {
            Db::update('amazonhelper_registar_key', ['mac_key' => '', 'change_mackey' => 0], ['Id' => $uid]);
            Admin::audit(3, '解绑机器码', 'UID ' . $uid . ' 邮箱 ' . $old['email']);
            Admin::flash('ok', '已解绑机器码，该账号下次登录需要重新绑定。');
            header('Location: ?a=user&id=' . $uid);
            return;
        }

        $due = Admin::post('due_date');
        if ($due !== '' && strtotime($due) === false) {
            Admin::flash('error', '到期时间格式不正确，应为 2026-01-31 23:59:59');
            header('Location: ?a=user&id=' . $uid);
            return;
        }

        $new = [
            'due_date'     => $due === '' ? null : date('Y-m-d H:i:s', strtotime($due)),
            'state'        => Admin::post('state') === 'no' ? 'no' : 'yes',
            'state_reason' => mb_substr(Admin::post('state_reason'), 0, 200),
            'function'     => (int) Admin::post('function', '1'),
            'limit_change' => max(0, (int) Admin::post('limit_change', '5')),
            'change_time'  => max(0, (int) Admin::post('change_time', '0')),
            'soft_limit'   => mb_substr(Admin::post('soft_limit'), 0, 250),
            'remarks'      => mb_substr(Admin::post('remarks'), 0, 250),
            'referee'      => mb_substr(Admin::post('referee'), 0, 30),
            'mod_time'     => date('Y-m-d H:i:s'),
        ];

        $labels = [
            'due_date' => '到期时间', 'state' => '状态', 'state_reason' => '冻结说明', 'function' => '版本',
            'limit_change' => '可换机总次数', 'change_time' => '已用换机次数', 'soft_limit' => '增配权限',
            'remarks' => '备注', 'referee' => '关联账号',
        ];
        $diff = [];
        foreach ($new as $k => $v) {
            $before = $old[$k] === null ? '' : (string) $old[$k];
            $after  = $v === null ? '' : (string) $v;
            if ($before !== $after) {
                $diff[$k] = ['name' => $labels[$k], 'old' => $before, 'new' => $after];
            }
        }
        if (!$diff) {
            Admin::flash('ok', '没有需要保存的改动');
            header('Location: ?a=user&id=' . $uid);
            return;
        }

        Db::update('amazonhelper_registar_key', $new, ['Id' => $uid]);
        Repo::editRe([
            'uid'          => $uid,
            'email'        => $old['email'],
            'op'           => 'edit',
            'diff'         => json_encode($diff, JSON_UNESCAPED_UNICODE),
            'snapshot'     => json_encode(['fields' => $new, 'keys' => array_keys($diff)], JSON_UNESCAPED_UNICODE),
            'operator'     => $ctx->name(),
            'operator_uid' => $ctx->uid(),
            'ip'           => $ctx->ip(),
            'date'         => date('Y-m-d H:i:s'),
        ]);
        Db::insert('user_manager_re', ['uid' => $uid, 'admin_uid' => $ctx->uid(), 'text' => '修改 ' . implode('、', array_column($diff, 'name')), 'date' => date('Y-m-d H:i:s')]);
        Admin::audit(3, '修改用户资料', 'UID ' . $uid . ' 邮箱 ' . $old['email'] . ' 变更 ' . implode('；', array_map(function ($d) {
            return $d['name'] . ':' . $d['old'] . '→' . $d['new'];
        }, $diff)));

        Admin::flash('ok', '已保存 ' . count($diff) . ' 项改动');
        header('Location: ?a=user&id=' . $uid);
    }

    /* ---------------------------- 序列号管理 ---------------------------- */

    public static function pageKeys(Page $ctx)
    {
        if ($_SERVER['REQUEST_METHOD'] === 'POST') {
            $op = Admin::post('op');
            if ($op === 'make') {
                self::makeKeys($ctx);
                return;
            }
            if ($op === 'void') {
                $keyNum = Admin::post('key_num');
                Db::update('key_list', ['state' => 2, 'remarks' => mb_substr(Admin::post('remarks'), 0, 200)], ['key_num' => $keyNum]);
                Admin::audit(6, '作废序列号', $keyNum);
                Admin::flash('ok', '序列号已作废');
                header('Location: ?a=keys');
                return;
            }
        }

        $where = '1=1';
        $args  = [];
        if (($kw = $ctx->q('keyword')) !== '') {
            $where .= ' AND (key_num LIKE ? OR email LIKE ?)';
            $args[] = '%' . $kw . '%';
            $args[] = '%' . $kw . '%';
        }
        if (($pt = $ctx->q('puc_type')) !== '') {
            $where .= ' AND puc_type=?';
            $args[] = (int) $pt;
        }
        if (($st = $ctx->q('state')) !== '') {
            $where .= ' AND state=?';
            $args[] = (int) $st;
        }
        if (($ch = $ctx->q('channel_id')) !== '') {
            $where .= ' AND channel_id=?';
            $args[] = (int) $ch;
        }

        $total = (int) Db::scalar('SELECT COUNT(*) FROM key_list WHERE ' . $where, $args);
        $start = (Admin::pageNow() - 1) * Admin::PAGE_SIZE;
        $list  = Db::all('SELECT * FROM key_list WHERE ' . $where . ' ORDER BY Id DESC LIMIT ' . $start . ',' . Admin::PAGE_SIZE, $args);

        $rows = [];
        foreach ($list as $k) {
            $act = (string) $k['state'] === '0'
                ? '<form method="post" style="display:inline">' . Admin::csrfField() . '<input type="hidden" name="op" value="void"><input type="hidden" name="key_num" value="' . Admin::h($k['key_num']) . '"><a href="#" onclick="this.parentNode.submit()">作废</a></form>'
                : '';
            $rows[] = [
                '<code>' . Admin::h($k['key_num']) . '</code>',
                self::product($k['puc_type']),
                $k['channel_id'] ? (int) $k['channel_id'] : '官方',
                Admin::h($k['time']),
                self::version($k['function']),
                self::keyState((int) $k['state']),
                Admin::h($k['email']),
                Admin::h($k['act_date']),
                $act,
            ];
        }

        $csrf = Admin::csrfField();
        $make = '<div class="card"><h2>生成序列号</h2><form method="post">' . $csrf
            . '<input type="hidden" name="op" value="make">'
            . '<p>产品 ' . self::select('puc_type', self::PRODUCTS, '3')
            . ' 版本 ' . self::select('function', self::FUNCTIONS, '1')
            . ' 类型 ' . self::select('is_test', ['1' => '正式序列号', '2' => '试用序列号'], '1')
            . ' 时长(天) <input name="days" value="365" style="width:90px">'
            . ' 数量 <input name="count" value="10" style="width:80px"></p>'
            . '<p>前缀 <input name="prefix" value="CXAH" style="width:120px"> 渠道ID <input name="channel_id" value="0" style="width:90px">'
            . ' 备注 <input name="remarks" style="width:260px"> <input type="submit" value="生成"></p>'
            . '<p><small>序列号形如 前缀-XXXX-XXXX-XXXX，字符去掉易混淆的 0/O/1/I；生成后可用上方列表导出文本发给买家。</small></p>'
            . '</form></div>';

        $filter = '<form method="get"><input name="a" value="keys" type="hidden">'
            . '<p>序列号或激活邮箱 <input name="keyword" value="' . Admin::h($ctx->q('keyword')) . '" style="width:280px">'
            . ' 产品 ' . self::select('puc_type', self::PRODUCTS, $ctx->q('puc_type'), true)
            . ' 状态 ' . self::select('state', ['0' => '未使用', '1' => '已激活', '2' => '已作废'], $ctx->q('state'), true)
            . ' 渠道ID <input name="channel_id" value="' . Admin::h($ctx->q('channel_id')) . '" style="width:80px">'
            . ' <input type="submit" value="查询"> '
            . '<a class="act" href="?a=keys' . '&' . http_build_query(array_merge($_GET, ['export' => '1', 'p' => 1])) . '">导出本页</a></p></form>';

        if ($ctx->q('export') === '1') {
            header('Content-Type: text/plain; charset=utf-8');
            header('Content-Disposition: attachment; filename="serials-' . date('Ymd-His') . '.txt"');
            foreach ($list as $k) {
                echo $k['key_num'], "\r\n";
            }
            return;
        }

        Admin::echoAdmin(Admin::layout('序列号管理', $make . '<div class="card"><h2>序列号列表</h2>' . $filter
            . Admin::table(['序列号', '产品', '渠道', '时长', '版本', '状态', '激活账号', '激活时间', '操作'], $rows) . Admin::pager($total) . '</div>'));
    }

    private static function makeKeys(Page $ctx)
    {
        $count  = min(2000, max(1, (int) Admin::post('count', '10')));
        $prefix = preg_replace('/[^A-Za-z0-9]/', '', Admin::post('prefix', 'CXAH'));
        $prefix = $prefix === '' ? 'CXAH' : strtoupper($prefix);
        $days   = max(1, (int) Admin::post('days', '365'));
        $puc    = (string) Admin::post('puc_type', '3');
        $func   = (int) Admin::post('function', '1');
        $isTest = Admin::post('is_test', '1') === '2' ? '2' : '1';
        $chan   = (int) Admin::post('channel_id', '0');
        $remark = mb_substr(Admin::post('remarks'), 0, 200);

        $made = [];
        for ($i = 0; $i < $count; $i++) {
            $keyNum = self::generateKey($prefix);
            $tries  = 0;
            while (Repo::findKey($keyNum) && $tries++ < 5) {
                $keyNum = self::generateKey($prefix);
            }
            Db::insert('key_list', [
                'key_num'    => $keyNum,
                'channel_id' => $chan,
                'is_test'    => $isTest,
                'time'       => (string) $days,
                'state'      => 0,
                'puc_type'   => (int) $puc,
                'type'       => $days >= 365 ? '年卡' : ($days >= 90 ? '季卡' : '月卡'),
                'date'       => date('Y-m-d H:i:s'),
                'function'   => $func,
                'new'        => 1,
                'act_type'   => 0,
                'remarks'    => $remark,
            ]);
            $made[] = $keyNum;
        }

        Db::update('channel', ['count' => (int) Db::scalar('SELECT `count` FROM channel WHERE channel_id=?', [$chan]) + count($made)], ['channel_id' => $chan]);
        Admin::audit(6, '生成序列号', '产品' . $puc . ' 时长' . $days . '天 共' . count($made) . '个');
        Admin::flash('ok', '已生成 ' . count($made) . ' 个序列号，可用「导出本页」取文本');
        $_SESSION['cxah_last_keys'] = $made;
        header('Location: ?a=keys&state=0&keyword=');
    }

    private static function generateKey($prefix)
    {
        $pool = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
        $part = function () use ($pool) {
            $s = '';
            for ($i = 0; $i < 4; $i++) {
                $s .= $pool[random_int(0, strlen($pool) - 1)];
            }
            return $s;
        };
        return $prefix . '-' . $part() . '-' . $part() . '-' . $part();
    }

    /* ---------------------------- 机器码查询 ---------------------------- */

    public static function pageMachine(Page $ctx)
    {
        $kw  = $ctx->q('mac');
        $out = '';
        if ($kw !== '') {
            $parts = MachineCode::segments($kw);
            list($code, $msg) = MachineCode::check($kw);
            $info = $parts ? [
                '机器码版本' => MachineCode::version($kw) === 2 ? 'MACVER2（新版）' : '旧版',
                '结构校验'   => $code === '0' ? '通过' : '不通过：' . $msg,
                '产品类型'   => self::product(MachineCode::productType($kw)),
                'CPU 序列号' => MachineCode::cpuId($kw),
                '计算机名CRC' => MachineCode::hostCrc($kw),
                '硬盘段'     => isset($parts[1]) ? $parts[1] : '',
            ] : ['解析结果' => '不是合法的机器码（base64 段数不足）'];

            $dl = '';
            foreach ($info as $k => $v) {
                $dl .= '<dt>' . Admin::h($k) . '</dt><dd>' . Admin::h($v) . '</dd>';
            }

            $bound = Repo::usersByMac($kw);
            $rows = [];
            foreach ($bound as $u) {
                $rows[] = ['<a class="act" href="?a=user&id=' . (int) $u['Id'] . '">' . Admin::h($u['email']) . '</a>',
                           self::product($u['puc_type']), $u['channel_id'] ? (int) $u['channel_id'] : '官方', Admin::h($u['due_date']), self::stateTag($u['state'])];
            }

            $same = [];
            $cpu  = MachineCode::cpuId($kw);
            if ($cpu !== '') {
                $same = Db::all(
                    'SELECT Id,email,puc_type,due_date,mac_key FROM amazonhelper_registar_key WHERE mac_key<>? AND mac_key LIKE ? LIMIT 100',
                    [$kw, '%' . $cpu . '%']
                );
            }
            $sameRows = [];
            foreach ($same as $u) {
                $sameRows[] = [Admin::h($u['email']), self::product($u['puc_type']), Admin::h($u['due_date']), '<code>' . self::macBrief($u['mac_key']) . '</code>'];
            }

            $out = '<div class="card"><h2>机器码解析</h2><dl class="kv">' . $dl . '</dl></div>'
                . '<div class="card"><h2>绑定该机器码的账号</h2>' . Admin::table(['邮箱', '产品', '渠道', '到期时间', '状态'], $rows) . '</div>'
                . '<div class="card"><h2>同一台电脑上的其他账号（按 CPU 段匹配）</h2>'
                . '<p><small>重装系统或换硬盘后机器码会变，这里用于确认是否同一台物理电脑。</small></p>'
                . Admin::table(['邮箱', '产品', '到期时间', '机器码'], $sameRows) . '</div>';
        }

        $form = '<form method="get"><input name="a" value="machine" type="hidden">'
            . '<p>粘贴客户端显示的机器码 <input name="mac" value="' . Admin::h($kw) . '" style="width:640px" placeholder="base64 串">'
            . ' <input type="submit" value="查询"></p></form>';

        $recent = Db::all('SELECT email,puc_type,mac_key1,mac_key2,cd_key,change_time,date FROM amazonhelper_change_re ORDER BY Id DESC LIMIT 20');
        $rrows = [];
        foreach ($recent as $r) {
            $rrows[] = [Admin::h($r['email']), self::product($r['puc_type']), '<code>' . self::macBrief($r['mac_key2']) . '</code>', '<code>' . Admin::h($r['cd_key']) . '</code>', (int) $r['change_time'], Admin::h($r['date'])];
        }

        Admin::echoAdmin(Admin::layout('机器码查询', '<div class="card"><h2>机器码查询</h2>' . $form . '</div>' . $out
            . '<div class="card"><h2>最近换机（新→旧）</h2>' . Admin::table(['邮箱', '产品', '新机器码', '验证序列号', '累计换机', '时间'], $rrows) . '</div>'));
    }

    /* ---------------------------- 登录记录 ---------------------------- */

    public static function pageLogins(Page $ctx)
    {
        $where = '1=1';
        $args  = [];
        if (($uid = $ctx->q('uid')) !== '') {
            $where .= ' AND uid=?';
            $args[] = (int) $uid;
        }
        if (($ip = $ctx->q('ip')) !== '') {
            $where .= ' AND ip=?';
            $args[] = $ip;
        }
        if (($pt = $ctx->q('puctype')) !== '') {
            $where .= ' AND puctype=?';
            $args[] = (int) $pt;
        }
        if (($d = $ctx->q('date')) !== '' && strtotime($d) !== false) {
            $where .= ' AND DATE(date)=?';
            $args[] = date('Y-m-d', strtotime($d));
        }

        $total = (int) Db::scalar('SELECT COUNT(*) FROM user_login_re WHERE ' . $where, $args);
        $start = (Admin::pageNow() - 1) * Admin::PAGE_SIZE;
        $list  = Db::all('SELECT * FROM user_login_re WHERE ' . $where . ' ORDER BY user_login_re_id DESC LIMIT ' . $start . ',' . Admin::PAGE_SIZE, $args);

        $rows = [];
        foreach ($list as $l) {
            $rows[] = [
                (int) $l['user_login_re_id'],
                '<a class="act" href="?a=user&id=' . (int) $l['uid'] . '">' . (int) $l['uid'] . '</a>',
                Admin::h($l['date']),
                self::product($l['puctype']),
                Admin::h($l['ip']),
                Admin::h($l['ip_local']),
                Admin::h($l['version']),
                '<code>' . self::macBrief($l['mackey']) . '</code>',
            ];
        }

        $form = '<form method="get"><input name="a" value="logins" type="hidden">'
            . '<p>UID <input name="uid" value="' . Admin::h($ctx->q('uid')) . '" style="width:100px">'
            . ' IP <input name="ip" value="' . Admin::h($ctx->q('ip')) . '" style="width:150px">'
            . ' 产品 ' . self::select('puctype', self::PRODUCTS, $ctx->q('puctype'), true)
            . ' 日期 <input name="date" value="' . Admin::h($ctx->q('date', date('Y-m-d'))) . '" style="width:150px">'
            . ' <input type="submit" value="查询"></p></form>';

        Admin::echoAdmin(Admin::layout('登录记录', '<div class="card"><h2>登录记录</h2>' . $form
            . Admin::table(['ID', 'UID', '时间', '产品', 'IP', 'IP归属', '版本', '机器码'], $rows) . Admin::pager($total) . '</div>'));
    }

    /* ---------------------------- 换机记录 ---------------------------- */

    public static function pageChanges(Page $ctx)
    {
        if ($_SERVER['REQUEST_METHOD'] === 'POST' && Admin::post('op') === 'grant') {
            $uid  = (int) Admin::post('uid');
            $plus = max(0, (int) Admin::post('plus'));
            $u    = Repo::userById($uid);
            if ($u) {
                Db::update('amazonhelper_registar_key', ['limit_change' => (int) $u['limit_change'] + $plus], ['Id' => $uid]);
                Admin::audit(3, '增加换机次数', 'UID ' . $uid . ' 邮箱 ' . $u['email'] . ' +' . $plus . ' 次');
                Admin::flash('ok', '已为该账号增加 ' . $plus . ' 次换机配额');
            }
            header('Location: ?a=changes');
            return;
        }

        $where = '1=1';
        $args  = [];
        if (($kw = $ctx->q('keyword')) !== '') {
            $where .= ' AND (r.email LIKE ? OR r.key_num LIKE ?)';
            $args[] = '%' . $kw . '%';
            $args[] = '%' . $kw . '%';
        }
        if (($res = $ctx->q('result')) !== '') {
            $where .= ' AND r.result=?';
            $args[] = (int) $res;
        }

        $total = (int) Db::scalar('SELECT COUNT(*) FROM cx_change_request r WHERE ' . $where, $args);
        $start = (Admin::pageNow() - 1) * Admin::PAGE_SIZE;
        $list  = Db::all('SELECT r.*, u.limit_change, u.change_time FROM cx_change_request r LEFT JOIN amazonhelper_registar_key u ON u.Id=r.uid WHERE ' . $where . ' ORDER BY r.Id DESC LIMIT ' . $start . ',' . Admin::PAGE_SIZE, $args);

        $rows = [];
        foreach ($list as $r) {
            $rows[] = [
                (int) $r['Id'],
                '<a class="act" href="?a=user&id=' . (int) $r['uid'] . '">' . Admin::h($r['email']) . '</a>',
                self::product($r['puc_type']),
                '<code>' . self::macBrief($r['old_mac']) . '</code>',
                '<code>' . self::macBrief($r['new_mac']) . '</code>',
                '<code>' . Admin::h($r['key_num']) . '</code>',
                self::changeResult((int) $r['result']),
                (int) $r['change_time'] . ' / ' . (int) $r['limit_change'],
                Admin::h($r['date']) . '<br><small>' . Admin::h($r['done_date']) . '</small>',
                Admin::h($r['ip']),
            ];
        }

        $form = '<form method="get"><input name="a" value="changes" type="hidden">'
            . '<p>邮箱或序列号 <input name="keyword" value="' . Admin::h($ctx->q('keyword')) . '" style="width:260px">'
            . ' 结果 ' . self::select('result', ['0' => '待验证', '1' => '换机成功', '2' => '序列号不符', '3' => '次数不足', '4' => '账号异常', '5' => '被锁定'], $ctx->q('result'), true)
            . ' <input type="submit" value="查询"></p></form>';

        $grant = '<div class="card"><h2>为指定账号追加换机次数</h2><form method="post">' . Admin::csrfField()
            . '<input type="hidden" name="op" value="grant">'
            . '<p>用户UID <input name="uid" style="width:120px"> 追加次数 <input name="plus" value="1" style="width:80px"> <input type="submit" value="追加"></p>'
            . '<p><small>换机次数用完的客户，可在此直接加配额；加完让客户重新登录再按提示输入序列号即可自助换机。</small></p></form></div>';

        Admin::echoAdmin(Admin::layout('换机记录', '<div class="card"><h2>换机申请</h2>' . $form . Admin::table(
            ['ID', '邮箱', '产品', '原机器码', '新机器码', '验证序列号', '结果', '已用/上限', '申请/完成时间', 'IP'], $rows
        ) . Admin::pager($total) . '</div>' . $grant));
    }

    /* ---------------------------- 接口配置 ---------------------------- */

    public static function pageSettings(Page $ctx)
    {
        if ($_SERVER['REQUEST_METHOD'] === 'POST') {
            $saved = 0;
            foreach ((array) (isset($_POST['setting']) ? $_POST['setting'] : []) as $k => $v) {
                $v = trim((string) $v);
                $row = Db::one('SELECT k FROM cx_setting WHERE k=?', [$k]);
                if ($row) {
                    Db::update('cx_setting', ['v' => $v, 'mod_date' => date('Y-m-d H:i:s')], ['k' => $k]);
                } else {
                    Db::insert('cx_setting', ['k' => $k, 'v' => $v, 'note' => '后台新增', 'mod_date' => date('Y-m-d H:i:s')]);
                }
                $saved++;
            }
            Admin::audit(3, '修改接口配置', '共 ' . $saved . ' 项');
            Admin::flash('ok', '配置已保存，立即生效（无需重启）');
            header('Location: ?a=settings');
            return;
        }

        $rows = Db::all('SELECT k,v,note FROM cx_setting ORDER BY k');
        $body = '';
        foreach ($rows as $s) {
            $body .= '<tr><td><code>' . Admin::h($s['k']) . '</code><br><small>' . Admin::h($s['note']) . '</small></td>'
                . '<td><input name="setting[' . Admin::h($s['k']) . ']" value="' . Admin::h($s['v']) . '" style="width:100%"></td></tr>';
        }
        $body .= '<tr><td><code>__new_key__</code><br><small>填左侧不存在的配置名即可新增，键名留空则忽略本行</small></td>'
            . '<td><input name="setting[__new_key__]" value="" style="width:100%"></td></tr>';

        $channels = Db::all('SELECT channel_id,name,`key`,offical,puc_type,status,able_reg,protection,reg_count FROM channel ORDER BY channel_id');
        $crows = [];
        foreach ($channels as $c) {
            $crows[] = [(int) $c['channel_id'], Admin::h($c['name']), '<code>' . Admin::h($c['key']) . '</code>',
                        (int) $c['offical'] === 0 ? '官方' : '代理', self::product($c['puc_type']),
                        (int) $c['status'] === 1 ? '可用' : '已停用', (int) $c['able_reg'] === 1 ? '禁止注册' : '允许注册',
                        (int) $c['protection'] === 1 ? '仅本渠道序列号' : '不限制', (int) $c['reg_count']];
        }

        Admin::echoAdmin(Admin::layout('接口配置', '<div class="card"><h2>接口配置</h2><form method="post">' . Admin::csrfField()
            . '<table><thead><tr><th>配置项</th><th>值</th></tr></thead><tbody>' . $body . '</tbody></table>'
            . '<p><input type="submit" value="保存配置"></p></form>'
            . '<p><small>配置项含义见 sql/seed.sql 注释。协议密钥与数据库连接在 app/config.php，改了要同步改支持库初始化参数。</small></p></div>'
            . '<div class="card"><h2>渠道一览（只读）</h2>' . Admin::table(['渠道ID', '名称', '代理KEY', '性质', '产品', '状态', '注册', '序列号保护', '注册数'], $crows) . '</div>'));
    }

    /* ------------------------------ 小部件 ------------------------------ */

    public static function product($puc)
    {
        $puc = (string) $puc;
        return isset(self::PRODUCTS[$puc]) ? self::PRODUCTS[$puc] : ('产品' . $puc);
    }

    private static function version($f)
    {
        $f = (string) $f;
        return isset(self::FUNCTIONS[$f]) ? self::FUNCTIONS[$f] : '版本' . $f;
    }

    private static function stateTag($state)
    {
        $no = (string) $state === 'no';
        return '<span class="tag' . ($no ? ' no' : ' yes') . '">' . ($no ? '已冻结' : '正常') . '</span>';
    }

    private static function keyState($state)
    {
        $map = [0 => ['未使用', ''], 1 => ['已激活', 'yes'], 2 => ['已作废', 'no']];
        $s = isset($map[$state]) ? $map[$state] : ['未知', ''];
        return '<span class="tag ' . $s[1] . '">' . $s[0] . '</span>';
    }

    private static function changeResult($r)
    {
        $map = [0 => ['待验证', ''], 1 => ['换机成功', 'yes'], 2 => ['序列号不符', 'no'], 3 => ['次数不足', 'no'], 4 => ['账号异常', 'no'], 5 => ['被锁定', 'no']];
        $s = isset($map[$r]) ? $map[$r] : ['未知', ''];
        return '<span class="tag ' . $s[1] . '">' . $s[0] . '</span>';
    }

    /** 机器码太长，列表里只展示头尾，点击看全串 */
    private static function macBrief($mac)
    {
        $mac = (string) $mac;
        if ($mac === '') {
            return '<small>未绑定</small>';
        }
        if (strlen($mac) <= 24) {
            return Admin::h($mac);
        }
        return '<span title="' . Admin::h($mac) . '">' . Admin::h(substr($mac, 0, 12)) . '…' . Admin::h(substr($mac, -8)) . '</span>';
    }

    private static function options(array $map, $current)
    {
        $html = '';
        foreach ($map as $value => $label) {
            $on = ((string) $value === (string) $current) ? ' selected' : '';
            $html .= '<option value="' . Admin::h($value) . '"' . $on . '>' . Admin::h($label) . '</option>';
        }
        return $html;
    }

    private static function select($name, array $map, $current, $withAll = false)
    {
        $html = '<select name="' . Admin::h($name) . '">';
        if ($withAll) {
            $html .= '<option value="">全部</option>';
        }
        return $html . self::options($map, $current) . '</select>';
    }
}
