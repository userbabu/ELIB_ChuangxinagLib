<?php
/**
 * 序列号：激活续费、换机验证。
 */

namespace CxSoft\Service;

use CxSoft\ApiException;
use CxSoft\Db;
use CxSoft\Repo;
use CxSoft\Throttle;

class KeyService
{
    /** 输入序列号开通/续费授权 */
    public static function activate(array $p, array $ctx)
    {
        $email  = AuthService::email($p);
        $mac    = AuthService::mac($p, $ctx);
        $keyNum = self::keyNum($p);
        $user   = AuthService::requireUser($email, $ctx);

        $bound = (string) $user['mac_key'];
        if ($bound !== '' && $bound !== $mac) {
            AuthService::changeProbe(['email' => $email, 'mac_key' => $mac], $ctx);
        }

        AccountService::requireCaptcha($p, 'act', $keyNum, $ctx['ip']);
        $key = self::requireKey($keyNum, $user, $ctx);

        // 同一账号重复提交同一个已用序列号：按幂等成功处理，不再二次加期
        if ((int) $key['state'] === 1) {
            if (mb_strtolower((string) $key['email'], 'UTF-8') === $email) {
                return ['grant' => Grant::build($user, $mac), 'auth_status' => Grant::status(Grant::build($user, $mac)), 'replay' => true];
            }
            throw new ApiException('5002', null, ['key_used_at' => (string) $key['act_date'], 'used_by' => Grant::hideEmail($key['email'])]);
        }

        $days = self::durationDays($key);
        $now  = time();
        $base = ($user['due_date'] !== null && $user['due_date'] !== '' && strtotime($user['due_date']) > $now)
            ? strtotime($user['due_date']) : $now;
        $due  = date('Y-m-d H:i:s', $base + $days * 86400);

        $actType = (int) $key['act_type'];
        $save = ['mac_key' => $mac, 'ip' => $ctx['ip'], 'version' => $ctx['version'], 'md5' => $ctx['md5']];
        if ($actType === 0 || $actType === 2) {
            $save['due_date']      = $due;
            $save['is_test']       = Grant::paidValue();
            $save['order_number']  = $keyNum;
            if ($key['function'] !== null) {
                $save['function'] = (int) $key['function'];
            }
        }
        if (self::mergeSoftLimit((string) $user['soft_limit'], (string) $key['soft_limit']) !== null) {
            $save['soft_limit'] = self::mergeSoftLimit((string) $user['soft_limit'], (string) $key['soft_limit']);
        }
        Db::update('amazonhelper_registar_key', $save, ['Id' => (int) $user['Id']]);

        // 选配功能型序列号：只登记功能到期时间
        if (($actType === 1 || $actType === 2) && !empty($key['soft_limit'])) {
            foreach (explode(',', $key['soft_limit']) as $code) {
                $code = trim($code);
                if ($code !== '') {
                    Repo::saveFunction((int) $user['Id'], $code, self::functionLabel($code), $actType === 1 ? $due : $due);
                }
            }
        }

        Repo::markKeyUsed($keyNum, $email, $mac, $due, $actType);
        Repo::addEventRe([
            'uid' => (int) $user['Id'], 'email' => $email, 'mac_key' => $mac, 'event' => 'activate',
            'puc_type' => (int) $ctx['puc_type'], 'channel_id' => (int) $ctx['channel_id'],
            'version' => $ctx['version'], 'ip' => $ctx['ip'], 'date' => date('Y-m-d H:i:s'),
        ]);
        Throttle::clear('act', $keyNum, $ctx['ip']);

        $fresh = Repo::userById($user['Id']);
        $grant = Grant::build($fresh, $mac);
        return ['grant' => $grant, 'auth_status' => Grant::status($grant), 'due_date' => $due];
    }

    /**
     * 换机：用本账号激活过的序列号验证身份，验证通过即换机并登录。
     * 每次成功消耗一次换机配额。
     */
    public static function changeConfirm(array $p, array $ctx)
    {
        $email  = AuthService::email($p);
        $mac    = AuthService::mac($p, $ctx);
        $user   = AuthService::requireUser($email, $ctx);
        $reqId  = isset($p['request_id']) ? (int) $p['request_id'] : 0;

        if ($user['state'] === 'no') {
            throw new ApiException('2003', (string) $user['state_reason']);
        }

        $left = Grant::changeLeft($user);
        if ($left <= 0) {
            Repo::addChangeRequest([
                'uid' => (int) $user['Id'], 'email' => $email, 'puc_type' => (int) $ctx['puc_type'],
                'channel_id' => (int) $ctx['channel_id'], 'old_mac' => (string) $user['mac_key'],
                'new_mac' => $mac, 'result' => 3, 'ip' => $ctx['ip'], 'date' => date('Y-m-d H:i:s'),
            ]);
            throw new ApiException('4002', null, ['change_left' => 0]);
        }

        $needKey = (int) Repo::setting('change_need_key', 1) === 1;
        $usedKey = null;

        if ($needKey) {
            $keyNum = self::keyNum($p);
            AccountService::requireCaptcha($p, 'change', $email, $ctx['ip']);

            $key = Repo::keysOfUser($keyNum, $email);
            if (!$key) {
                $locked = Throttle::fail('change', $email, $ctx['ip'],
                    (int) Repo::setting('change_max_fail', 5), (int) Repo::setting('change_lock_minutes', 30));
                self::logFailedChangeRequest($user, $mac, $keyNum, $ctx, $locked ? 5 : 2, $reqId);
                throw new ApiException('4003', null, ['change_left' => $left, 'locked' => $locked]);
            }
            $usedKey = (string) $key['key_num'];
        } else {
            AccountService::requireCaptcha($p, 'change', $email, $ctx['ip']);
        }

        $oldMac = (string) $user['mac_key'];
        Db::update('amazonhelper_registar_key', [
            'mac_key'     => $mac,
            'change_time' => (int) $user['change_time'] + 1,
            'mod_time'    => date('Y-m-d H:i:s'),
        ], ['Id' => (int) $user['Id']]);
        Repo::bumpBindChange((int) $user['Id'], (int) $ctx['puc_type'], $mac);

        Repo::addChangeRe([
            'email'        => $email,
            'puc_type'     => (int) $ctx['puc_type'],
            'mac_key1'     => $oldMac,
            'mac_key2'     => $mac,
            'cd_key'       => $usedKey,
            'change_limit' => (int) $user['limit_change'],
            'change_time'  => (int) $user['change_time'] + 1,
            'date'         => date('Y-m-d H:i:s'),
            'ip'           => $ctx['ip'],
        ]);
        Repo::finishChangeRequest($reqId, 1);
        Throttle::clear('change', $email, $ctx['ip']);

        // 换机成功后直接登录，客户端不必再走一次登录请求
        $fresh  = Repo::userById($user['Id']);
        $result = AuthService::login(['email' => $email, 'mac_key' => $mac], $ctx);
        return [
            'grant'        => $result['grant'],
            'auth_status'  => $result['auth_status'],
            'changed'      => true,
            'old_mac'      => $oldMac,
            'change_left'  => Grant::changeLeft($fresh),
        ];
    }

    /* ------------------------------ 内部 ------------------------------ */

    private static function keyNum(array $p)
    {
        $keyNum = trim((string) (isset($p['key_num']) ? $p['key_num'] : ''));
        $keyNum = str_replace(["\r", "\n", "\t", ' '], '', $keyNum);
        if ($keyNum === '' || strlen($keyNum) > 100) {
            throw new ApiException('1001', '请输入序列号');
        }
        return $keyNum;
    }

    private static function requireKey($keyNum, $user, $ctx)
    {
        $key = Repo::findKey($keyNum);
        if (!$key) {
            throw new ApiException('5001', null);
        }
        if ((int) $key['state'] === 2) {
            throw new ApiException('5003', null);
        }
        $ban = Repo::keyDisabled($keyNum);
        if ($ban) {
            throw new ApiException('5003', $ban['status_reson'] ? $ban['status_reson'] : null);
        }
        if ($key['puc_type'] !== null && (string) $key['puc_type'] !== '' && (string) $key['puc_type'] !== (string) $ctx['puc_type']) {
            throw new ApiException('5004', '该序列号不适用于本软件，请核对后重新输入。');
        }
        $channel = Repo::channelById((int) $user['channel_id']);
        if ($channel && (int) $channel['protection'] === 1 && (int) $key['channel_id'] !== (int) $user['channel_id']) {
            throw new ApiException('5004', '该序列号不属于当前软件渠道，请联系卖家确认。');
        }
        return $key;
    }

    /** 序列号时长：新版生成的 time 列存纯天数；旧数据按配置兜底 */
    private static function durationDays($key)
    {
        $time = trim((string) $key['time']);
        if ($time !== '' && ctype_digit($time)) {
            return max(1, (int) $time);
        }
        if ($time !== '' && is_numeric($time)) {
            return max(1, (int) round((float) $time));
        }
        $fallback = (int) Repo::setting('key_default_days', 365);
        \CxSoft\Log::info('序列号时长非数字，按默认天数处理 key=' . $key['key_num'] . ' time=' . $time . ' days=' . $fallback);
        return max(1, $fallback);
    }

    /** 合并增配权限，返回 null 表示无需改动 */
    private static function mergeSoftLimit($current, $add)
    {
        if ($add === '') {
            return null;
        }
        $list = array_filter(array_map('trim', explode(',', (string) $current)));
        $changed = false;
        foreach (array_filter(array_map('trim', explode(',', $add))) as $item) {
            if (!in_array($item, $list, true)) {
                $list[]    = $item;
                $changed = true;
            }
        }
        return $changed ? implode(',', $list) : null;
    }

    private static function functionLabel($code)
    {
        $map = json_decode((string) Repo::setting('function_names', '{}'), true);
        return (is_array($map) && isset($map[$code])) ? $map[$code] : $code;
    }

    private static function logFailedChangeRequest($user, $mac, $keyNum, $ctx, $result, $reqId)
    {
        if ($reqId > 0) {
            Repo::finishChangeRequest($reqId, $result);
            return;
        }
        Repo::addChangeRequest([
            'uid' => (int) $user['Id'], 'email' => $user['email'], 'puc_type' => (int) $ctx['puc_type'],
            'channel_id' => (int) $ctx['channel_id'], 'old_mac' => (string) $user['mac_key'],
            'new_mac' => $mac, 'key_num' => mb_substr((string) $keyNum, 0, 100),
            'result' => $result, 'ip' => $ctx['ip'], 'date' => date('Y-m-d H:i:s'),
        ]);
    }
}
