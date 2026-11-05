<?php
/**
 * 账号：注册、开通试用。
 */

namespace CxSoft\Service;

use CxSoft\ApiException;
use CxSoft\Captcha;
use CxSoft\Code;
use CxSoft\Db;
use CxSoft\MachineCode;
use CxSoft\Repo;
use CxSoft\Throttle;

class AccountService
{
    /** 注册：创建账号并绑定机器码，不含授权 */
    public static function register(array $p, array $ctx)
    {
        $email = AuthService::email($p);
        $mac   = AuthService::mac($p, $ctx);
        self::requireCaptcha($p, 'reg', $email);

        $channel = ChannelService::requireUsable($ctx['channel_id']);
        if ((int) $channel['able_reg'] === 1) {
            throw new ApiException('7002', null);
        }
        self::checkIpRegisterLimit($ctx['ip']);

        if (Repo::findUser($email, $ctx['puc_type'], $ctx['channel_id'])) {
            throw new ApiException('2002', null, ['exists' => true, 'can_login' => true]);
        }

        $limit = (int) Repo::setting('change_default_limit', 5);
        $uid = Repo::createUser([
            'qq'            => self::scalar($p, 'qq', 13, '0'),
            'email'         => $email,
            'mac_key'       => $mac,
            'is_test'       => Grant::trialValue(),
            'system'        => $ctx['system'],
            'ip'            => $ctx['ip'],
            'ip_location'   => $ctx['ip_location'],
            'version'       => $ctx['version'],
            'md5'           => $ctx['md5'],
            'puc_type'      => (string) $ctx['puc_type'],
            'referee'       => self::scalar($p, 'referee', 30, '0'),
            'reg_date'      => date('Y-m-d H:i:s'),
            'state'         => 'yes',
            'remarks'       => self::scalar($p, 'remarks', 255, null),
            'order_number'  => '0',
            'soft_limit'    => '',
            'login_num'     => 0,
            'function'      => 1,
            'change_time'   => 0,
            'limit_change'  => $limit,
            'auto_reg'      => 0,
            'channel_id'    => (int) $ctx['channel_id'],
            'channel'       => self::scalar($p, 'channel', 255, null),
            'learn'         => self::scalar($p, 'learn', 255, null),
            'home_username' => self::scalar($p, 'home_username', 50, ''),
        ]);

        Repo::saveBind($uid, (int) $ctx['puc_type'], $mac, $limit);
        Repo::addEventRe([
            'uid' => $uid, 'email' => $email, 'mac_key' => $mac, 'event' => 'register',
            'puc_type' => (int) $ctx['puc_type'], 'channel_id' => (int) $ctx['channel_id'],
            'version' => $ctx['version'], 'ip' => $ctx['ip'], 'date' => date('Y-m-d H:i:s'),
        ]);

        $user = Repo::userById($uid);
        return ['grant' => Grant::build($user, $mac), 'auth_status' => 'none'];
    }

    /** 开通试用：一个账号只给一次，到期时间直接写入主表 */
    public static function trial(array $p, array $ctx)
    {
        $email = AuthService::email($p);
        $mac   = AuthService::mac($p, $ctx);
        $user  = AuthService::requireUser($email, $ctx);

        $bound = (string) $user['mac_key'];
        if ($bound !== '' && $bound !== $mac) {
            // 试用同样受机器码约束，走统一的换机判定
            AuthService::changeProbe(['email' => $email, 'mac_key' => $mac], $ctx);
        }

        $used = ($user['due_date'] !== null && $user['due_date'] !== '')
            || ((string) $user['order_number'] !== '0' && (string) $user['order_number'] !== '');
        if ($used) {
            throw new ApiException('6001', null, ['grant' => Grant::build($user, $mac)]);
        }

        $days = (int) Repo::setting('trial_days', 7);
        if ($days <= 0) {
            throw new ApiException('1006', '当前未开放免费试用，请联系客服开通正式授权。');
        }
        $due = date('Y-m-d H:i:s', strtotime('+' . $days . ' days'));

        Db::update('amazonhelper_registar_key', [
            'due_date' => $due,
            'is_test'  => Grant::trialValue(),
            'mac_key'  => $mac,
            'ip'       => $ctx['ip'],
            'version'  => $ctx['version'],
            'md5'      => $ctx['md5'],
        ], ['Id' => (int) $user['Id']]);

        Repo::addEventRe([
            'uid' => (int) $user['Id'], 'email' => $email, 'mac_key' => $mac, 'event' => 'trial',
            'puc_type' => (int) $ctx['puc_type'], 'channel_id' => (int) $ctx['channel_id'],
            'version' => $ctx['version'], 'ip' => $ctx['ip'], 'date' => date('Y-m-d H:i:s'),
        ]);
        Throttle::clear('reg', $email, $ctx['ip']);

        $user = Repo::userById($user['Id']);
        return ['grant' => Grant::build($user, $mac), 'auth_status' => Grant::status(Grant::build($user, $mac)), 'due_date' => $due];
    }

    /* ------------------------------ 辅助 ------------------------------ */

    /** 验证码一次一用；错误与失效都记入失败计数 */
    public static function requireCaptcha(array $p, $scene, $target, $ip = null)
    {
        if ((int) Repo::setting('captcha_enabled', 1) !== 1) {
            return;
        }
        $ip = $ip === null ? (isset($p['ip']) ? $p['ip'] : '') : $ip;
        $state = Throttle::state($scene, $target, $ip);
        if ($state['locked']) {
            throw new ApiException('4004', null, ['retry_in' => $state['retry_in']]);
        }

        $id  = isset($p['captcha_id']) ? (string) $p['captcha_id'] : '';
        $ans = isset($p['captcha_code']) ? (string) $p['captcha_code'] : '';
        $result = Captcha::consume($id, $ans);
        if ($result !== '0') {
            Throttle::fail($scene, $target, $ip,
                (int) Repo::setting('change_max_fail', 5), (int) Repo::setting('change_lock_minutes', 30));
            throw new ApiException($result, null);
        }
    }

    private static function checkIpRegisterLimit($ip)
    {
        $max = (int) Repo::setting('register_max_ip_day', 20);
        if ($max <= 0) {
            return;
        }
        $n = (int) Db::scalar(
            'SELECT COUNT(*) FROM amazonhelper_registar_key WHERE ip=? AND reg_date>=?',
            [$ip, date('Y-m-d 00:00:00')]
        );
        if ($n >= $max) {
            throw new ApiException('1006', '本机今天注册次数过多，请明天再试或联系客服。');
        }
    }

    private static function scalar(array $p, $key, $maxLen, $default)
    {
        if (!isset($p[$key])) {
            return $default;
        }
        $v = trim((string) $p[$key]);
        if ($v === '') {
            return $default;
        }
        if (mb_strlen($v, 'UTF-8') > $maxLen) {
            $v = mb_substr($v, 0, $maxLen, 'UTF-8');
        }
        return $v;
    }
}
