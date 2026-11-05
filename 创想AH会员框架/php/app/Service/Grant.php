<?php
/**
 * 授权视图：把旧库的用户行统一换算成客户端需要的授权数据。
 * 只在末尾加字段，已有字段含义永不改动。
 */

namespace CxSoft\Service;

use CxSoft\Db;
use CxSoft\Repo;

class Grant
{
    /** 版本代号 -> 名称（旧库 function 列） */
    public static $functionNames = [
        '0'   => '企业版',
        '0.9' => '旗舰版',
        '1'   => '企业版',
        '2'   => '高级版',
        '3'   => '标准版',
    ];

    /** 试用判定：旧库 is_test 列 2=试用版（列注释即此口径），1=正式版 */
    public static function isTrial($user)
    {
        return (string) $user['is_test'] === self::trialValue();
    }

    public static function trialValue()
    {
        $v = Repo::setting('trial_is_test_value', null);
        return $v === null ? '2' : (string) $v;
    }

    public static function paidValue()
    {
        $v = Repo::setting('paid_is_test_value', null);
        return $v === null ? '1' : (string) $v;
    }

    public static function changeLeft($user)
    {
        $limit  = (int) ($user['limit_change'] === null ? Repo::setting('change_default_limit', 5) : $user['limit_change']);
        $used   = (int) ($user['change_time'] === null ? 0 : $user['change_time']);
        return max(0, $limit - $used);
    }

    /**
     * 组装授权数据。
     * @param array $user   amazonhelper_registar_key 行
     * @param string $mac   本次请求使用的机器码（不传则用库里绑定的）
     */
    public static function build($user, $mac = null)
    {
        $due      = ($user['due_date'] === null || $user['due_date'] === '') ? '' : $user['due_date'];
        $state    = ($user['state'] === 'no') ? 'no' : 'yes';
        $trial    = self::isTrial($user);
        $left     = self::changeLeft($user);
        $function = (string) ($user['function'] === null ? '1' : $user['function']);
        $now      = date('Y-m-d H:i:s');

        $functions = [];
        $dueOf = [];   // 功能码 -> 到期时间，旧模块的「附加信息」按这个口径给
        foreach (Repo::functions($user['Id']) as $f) {
            $dueOf[(string) $f['function_code']] = (string) $f['due_date'];
            $functions[] = [
                'code'     => $f['function_code'],
                'name'     => $f['function_name'],
                'due_date' => $f['due_date'],
                'status'   => (int) $f['status'],
                'usable'   => ((int) $f['status'] === 0) && ($f['billing_type'] == 0 || (int) strtotime((string) $f['due_date']) > time()),
            ];
        }

        return [
            'uid'            => (int) $user['Id'],
            'email'          => $user['email'],
            'qq'             => (string) $user['qq'],
            'bind_mac'       => (string) $user['mac_key'],
            'mac_key'        => $mac === null ? (string) $user['mac_key'] : $mac,
            'state'          => $state,
            'state_reason'   => (string) $user['state_reason'],
            'due_date'       => $due,
            'reg_date'       => (string) $user['reg_date'],
            'last_login'     => (string) $user['login_date'],
            'login_num'      => (int) $user['login_num'],
            'is_trial'       => $trial,
            'is_expired'     => $due !== '' && strtotime($due) <= time(),
            'authorized'     => $state === 'yes' && $due !== '' && strtotime($due) > time(),
            'function'       => $function,
            'function_name'  => isset(self::$functionNames[$function]) ? self::$functionNames[$function] : '企业版',
            'soft_limit'     => (string) $user['soft_limit'],
            'order_number'   => (string) $user['order_number'],
            'puc_type'       => (string) $user['puc_type'],
            'channel_id'     => (int) $user['channel_id'],
            'home_username'  => (string) $user['home_username'],
            'remarks'        => (string) $user['remarks'],
            'limit_change'   => (int) $user['limit_change'],
            'change_time'    => (int) $user['change_time'],
            'change_left'    => $left,
            'server_time'    => $now,
            'functions'      => $functions,
            // 旧模块 全局_授权信息.附加信息 与 IP 字段，采集器直接读这些
            'attr'           => self::attr($user, $dueOf),
            'ip'             => isset($user['ip']) ? (string) $user['ip'] : '',
            'ip_location'    => isset($user['ip_location']) ? (string) $user['ip_location'] : '',
        ];
    }

    /**
     * 组装旧口径的附加信息：企业版最后激活时间、按功能到期时间、采集限速参数。
     * 限速参数取全局配置（后台「接口配置」里同名键），没配就是 0 = 不限速。
     */
    private static function attr($user, array $dueOf)
    {
        $last = Db::scalar(
            'SELECT act_date FROM key_list WHERE email=? AND state=1 ORDER BY act_date DESC LIMIT 1',
            [(string) $user['email']]
        );
        $num = function ($key) {
            $v = Repo::setting($key, 0);
            return (int) $v;
        };
        return [
            'cor_last_actdate'            => (string) $last,
            'ac_function_catalog_duedate' => isset($dueOf['catalog']) ? $dueOf['catalog'] : '',
            'ac_function_cc_duedate'      => isset($dueOf['cc']) ? $dueOf['cc'] : '',
            'ac_sp1'                      => $num('ac_sp1'),
            'ac_sp2'                      => $num('ac_sp2'),
            'ac_sp_max'                   => $num('ac_sp_max'),
            'ac_sp_v3_1'                  => $num('ac_sp_v3_1'),
            'ac_sp_v3_2'                  => $num('ac_sp_v3_2'),
            'ac_sp_v3_max'                => $num('ac_sp_v3_max'),
        ];
    }

    /** 授权状态：ok / trial / expired / frozen / none */
    public static function status($grant)
    {
        if ($grant['state'] === 'no') {
            return 'frozen';
        }
        if ($grant['due_date'] === '') {
            return 'none';
        }
        if ($grant['is_expired']) {
            return 'expired';
        }
        return $grant['is_trial'] ? 'trial' : 'ok';
    }

    /** 邮箱打码：a***@qq.com，用于提示「该序列号已被 xxx 用过」而不泄露完整邮箱 */
    public static function hideEmail($email)
    {
        $email = (string) $email;
        $at = strpos($email, '@');
        if ($at === false) {
            return mb_substr($email, 0, 1, 'UTF-8') . '***';
        }
        $name = mb_substr($email, 0, $at, 'UTF-8');
        $head = mb_substr($name, 0, 1, 'UTF-8');
        return $head . '***' . mb_substr($email, $at, null, 'UTF-8');
    }
}
