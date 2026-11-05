<?php
/**
 * 登录 / 刷新授权 / 事件上报。
 *
 * 登录核心规则（新框架的主要改动）：
 *   机器码与账号绑定不一致时——
 *     · 换机次数已用完  -> 直接报错，提示联系客服；
 *     · 换机次数还有    -> 返回 need_change，让客户端弹出「序列号 + 验证码」，
 *                          用户输入本账号激活过的序列号，验证通过即自动换机并登录。
 *   换机会消耗一次配额，所以只有确实换了电脑才走这条路，不做无谓消耗。
 */

namespace CxSoft\Service;

use CxSoft\ApiException;
use CxSoft\Db;
use CxSoft\MachineCode;
use CxSoft\Repo;

class AuthService
{
    /**
     * @param array $p   email, mac_key, captcha?
     * @param array $ctx ip, ip_location, version, md5, system, puc_type, channel_id, ua
     */
    public static function login(array $p, array $ctx, $withLoginRecord = true)
    {
        $email = self::email($p);
        $mac   = self::mac($p, $ctx);
        $user  = self::requireUser($email, $ctx);

        $bound = (string) $user['mac_key'];
        if ($bound !== '' && $bound !== $mac) {
            self::rejectOtherMachine($user, $mac, $ctx);
        }

        $grant = Grant::build($user, $mac);

        if ($grant['state'] === 'no') {
            throw new ApiException('2003', $grant['state_reason'], ['grant' => $grant]);
        }

        if ($grant['due_date'] === '') {
            throw new ApiException('2005', null, ['grant' => $grant, 'auth_status' => 'none', 'can_change' => Grant::changeLeft($user) > 0]);
        }
        if ($grant['is_expired']) {
            throw new ApiException('2004', null, ['grant' => $grant, 'auth_status' => 'expired']);
        }

        // 只有真正拿到授权才算一次登录，未开通/已到期的尝试不进登录记录
        if ($withLoginRecord) {
            self::recordLogin($user, $mac, $ctx);
        }

        return ['grant' => $grant, 'auth_status' => Grant::status($grant)];
    }

    /** 只做授权校验，不写登录记录、不更新登录时间 */
    public static function refresh(array $p, array $ctx)
    {
        $email = self::email($p);
        $mac   = self::mac($p, $ctx);
        $user  = self::requireUser($email, $ctx);

        $bound = (string) $user['mac_key'];
        if ($bound !== '' && $bound !== $mac) {
            self::rejectOtherMachine($user, $mac, $ctx);
        }

        $grant = Grant::build($user, $mac);
        return ['grant' => $grant, 'auth_status' => Grant::status($grant)];
    }

    /** 换机前的探测：客户端据此决定要不要弹序列号框 */
    public static function changeProbe(array $p, array $ctx)
    {
        $email = self::email($p);
        $mac   = self::mac($p, $ctx);
        $user  = self::requireUser($email, $ctx);
        $left  = Grant::changeLeft($user);

        if ((string) $user['mac_key'] === $mac) {
            return ['need_change' => false, 'change_left' => $left];
        }
        if ($left <= 0) {
            throw new ApiException('4002', null, ['change_left' => 0]);
        }

        $id = Repo::addChangeRequest([
            'uid'        => (int) $user['Id'],
            'email'      => $user['email'],
            'puc_type'   => (int) $ctx['puc_type'],
            'channel_id' => (int) $ctx['channel_id'],
            'old_mac'    => (string) $user['mac_key'],
            'new_mac'    => $mac,
            'key_num'    => null,
            'result'     => 0,
            'ip'         => $ctx['ip'],
            'date'       => date('Y-m-d H:i:s'),
        ]);

        return [
            'need_change'   => true,
            'request_id'    => $id,
            'change_left'   => $left,
            'need_key'      => (int) Repo::setting('change_need_key', 1) === 1,
            'need_captcha'  => true,
        ];
    }

    /* ----------------------------- 内部实现 ----------------------------- */

    /** 机器码不符：次数不足直接报错，次数充足要求走序列号换机 */
    private static function rejectOtherMachine($user, $mac, $ctx)
    {
        $left = Grant::changeLeft($user);
        if ($left <= 0) {
            Repo::addChangeRequest([
                'uid'        => (int) $user['Id'],
                'email'      => $user['email'],
                'puc_type'   => (int) $ctx['puc_type'],
                'channel_id' => (int) $ctx['channel_id'],
                'old_mac'    => (string) $user['mac_key'],
                'new_mac'    => $mac,
                'result'     => 3,
                'ip'         => $ctx['ip'],
                'date'       => date('Y-m-d H:i:s'),
            ]);
            throw new ApiException('4002', null, [
                'need_change' => false,
                'change_left' => 0,
                'bind_mac'    => (string) $user['mac_key'],
            ]);
        }
        throw new ApiException('4001', null, [
            'need_change'  => true,
            'change_left'  => $left,
            'need_key'     => (int) Repo::setting('change_need_key', 1) === 1,
            'need_captcha' => true,
            'bind_mac'     => (string) $user['mac_key'],
        ]);
    }

    private static function recordLogin($user, $mac, $ctx)
    {
        Db::update('amazonhelper_registar_key', [
            'login_date' => date('Y-m-d H:i:s'),
            'login_num'  => (int) $user['login_num'] + 1,
            'ip'         => $ctx['ip'],
            'ip_location'=> $ctx['ip_location'],
            'version'    => $ctx['version'],
            'md5'        => $ctx['md5'],
            'system'     => $ctx['system'],
            'mac_key'    => $mac,
        ], ['Id' => (int) $user['Id']]);

        Repo::addLoginRe([
            'uid'        => (int) $user['Id'],
            'date'       => date('Y-m-d H:i:s'),
            'mackey'     => $mac,
            'ip'         => $ctx['ip'],
            'ip_local'   => $ctx['ip_location'],
            'puctype'    => (int) $ctx['puc_type'],
            'channel_id' => (int) $ctx['channel_id'],
            'version'    => $ctx['version'],
            'app_hash'   => $ctx['md5'],
            'platform'   => (int) $ctx['system'],
        ]);

        Repo::saveBind((int) $user['Id'], (int) $ctx['puc_type'], $mac, (int) $user['limit_change']);
        Repo::addEventRe([
            'uid'        => (int) $user['Id'],
            'email'      => $user['email'],
            'mac_key'    => $mac,
            'event'      => 'login',
            'puc_type'   => (int) $ctx['puc_type'],
            'channel_id' => (int) $ctx['channel_id'],
            'version'    => $ctx['version'],
            'ip'         => $ctx['ip'],
            'date'       => date('Y-m-d H:i:s'),
        ]);
    }

    /** 客户端事件：quit=关闭软件 */
    public static function event(array $p, array $ctx)
    {
        $event = isset($p['event']) ? strtolower(trim((string) $p['event'])) : '';
        if (!in_array($event, ['quit', 'logout', 'heartbeat'], true)) {
            throw new ApiException('1001', '不支持的事件类型');
        }
        $email = isset($p['email']) ? mb_strtolower(trim((string) $p['email']), 'UTF-8') : '';
        $user  = $email === '' ? null : Repo::findUser($email, $ctx['puc_type'], $ctx['channel_id']);

        if ($user && $event === 'quit') {
            Db::update('amazonhelper_registar_key', ['quit_date' => date('Y-m-d H:i:s')], ['Id' => (int) $user['Id']]);
        }
        Repo::addEventRe([
            'uid'        => $user ? (int) $user['Id'] : null,
            'email'      => $email,
            'mac_key'    => isset($p['mac_key']) ? (string) $p['mac_key'] : '',
            'event'      => $event,
            'puc_type'   => (int) $ctx['puc_type'],
            'channel_id' => (int) $ctx['channel_id'],
            'version'    => $ctx['version'],
            'ip'         => $ctx['ip'],
            'date'       => date('Y-m-d H:i:s'),
        ]);
        return ['event' => $event];
    }

    /* ------------------------------- 取值 ------------------------------- */

    public static function email(array $p)
    {
        $email = mb_strtolower(trim((string) (isset($p['email']) ? $p['email'] : '')), 'UTF-8');
        if ($email === '' || !filter_var($email, FILTER_VALIDATE_EMAIL)) {
            throw new ApiException('1001', '邮箱格式不正确');
        }
        if (strlen($email) > 200) {
            throw new ApiException('1001', '邮箱过长');
        }
        return $email;
    }

    public static function mac(array $p, array $ctx)
    {
        $mac = trim((string) (isset($p['mac_key']) ? $p['mac_key'] : ''));
        if ($mac === '') {
            throw new ApiException('1001', '缺少机器码');
        }
        if (strlen($mac) > 255) {
            throw new ApiException('3001', null);
        }
        list($code, $msg) = MachineCode::check($mac, $ctx['puc_type']);
        if ($code !== '0') {
            throw new ApiException('3001', $msg);
        }
        return $mac;
    }

    public static function requireUser($email, $ctx)
    {
        $user = Repo::findUser($email, $ctx['puc_type'], $ctx['channel_id']);
        if (!$user) {
            throw new ApiException('2001', null, ['exists' => false]);
        }
        return $user;
    }
}
