<?php
/**
 * 失败计数与临时锁定：防止对序列号/换机接口暴力猜测。
 * 以 场景+目标+IP 为唯一键，超限后写入 lock_until。
 */

namespace CxSoft;

class Throttle
{
    /** 是否处于锁定中；返回 ['locked'=>bool,'retry_in'=>秒] */
    public static function state($scene, $target, $ip)
    {
        $row = Db::one(
            'SELECT fail_count, lock_until FROM cx_auth_attempt WHERE scene=? AND target=? AND ip=?',
            [$scene, self::key($target), $ip]
        );
        if (!$row) {
            return ['locked' => false, 'retry_in' => 0];
        }
        if ($row['lock_until'] !== null && strtotime($row['lock_until']) > time()) {
            return ['locked' => true, 'retry_in' => strtotime($row['lock_until']) - time()];
        }
        return ['locked' => false, 'retry_in' => 0];
    }

    /** 记录一次失败，返回是否已触发锁定 */
    public static function fail($scene, $target, $ip, $maxFail, $lockMinutes)
    {
        $key = self::key($target);
        $now = date('Y-m-d H:i:s');
        $row = Db::one('SELECT Id, fail_count FROM cx_auth_attempt WHERE scene=? AND target=? AND ip=?', [$scene, $key, $ip]);
        if ($row) {
            $count = (int) $row['fail_count'] + 1;
            $lock  = $count >= $maxFail ? date('Y-m-d H:i:s', time() + $lockMinutes * 60) : null;
            Db::update('cx_auth_attempt', [
                'fail_count' => $count,
                'lock_until' => $lock,
                'last_date'  => $now,
            ], ['Id' => $row['Id']]);
            return $lock !== null;
        }
        Db::insert('cx_auth_attempt', [
            'scene'      => $scene,
            'target'     => $key,
            'ip'         => $ip,
            'fail_count' => 1,
            'lock_until' => null,
            'last_date'  => $now,
        ]);
        return false;
    }

    /** 成功后清零 */
    public static function clear($scene, $target, $ip)
    {
        Db::exec('DELETE FROM cx_auth_attempt WHERE scene=? AND target=? AND ip=?', [$scene, self::key($target), $ip]);
    }

    private static function key($target)
    {
        return mb_strtolower(trim((string) $target), 'UTF-8');
    }
}
