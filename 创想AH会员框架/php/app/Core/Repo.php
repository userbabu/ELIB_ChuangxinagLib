<?php
/**
 * 数据访问：只负责「按旧库结构读写」，业务判断放 Service 层。
 * 旧库字段语义在此集中转换，别处不再出现列名。
 */

namespace CxSoft;

class Repo
{
    /* ----------------------------- 用户 ----------------------------- */

    public static function findUser($email, $pucType, $channelId)
    {
        return Db::one(
            'SELECT * FROM amazonhelper_registar_key WHERE email=? AND puc_type=? AND channel_id=? LIMIT 1',
            [mb_strtolower(trim((string) $email), 'UTF-8'), (string) $pucType, (int) $channelId]
        );
    }

    public static function userById($uid)
    {
        return Db::one('SELECT * FROM amazonhelper_registar_key WHERE Id=? LIMIT 1', [(int) $uid]);
    }

    public static function createUser(array $d)
    {
        return Db::insert('amazonhelper_registar_key', $d);
    }

    public static function saveUser($uid, array $d)
    {
        return Db::update('amazonhelper_registar_key', $d, ['Id' => (int) $uid]);
    }

    /** 同一台电脑被哪些账号绑定（机器码管理用） */
    public static function usersByMac($macKey, $limit = 50)
    {
        return Db::all(
            'SELECT Id,email,puc_type,channel_id,due_date,state FROM amazonhelper_registar_key WHERE mac_key=? ORDER BY Id DESC LIMIT ' . (int) $limit,
            [$macKey]
        );
    }

    /* --------------------------- 绑定表 --------------------------- */

    public static function findBind($uid, $pucType)
    {
        return Db::one('SELECT * FROM user_bind WHERE uid=? AND puc_type=? ORDER BY user_bind_id DESC LIMIT 1', [(int) $uid, (int) $pucType]);
    }

    public static function saveBind($uid, $pucType, $macKey, $limitChange)
    {
        $row = self::findBind($uid, $pucType);
        $now = date('Y-m-d H:i:s');
        if ($row) {
            Db::update('user_bind', ['mackey' => $macKey, 'bind_date' => $now], ['user_bind_id' => $row['user_bind_id']]);
            return (int) $row['user_bind_id'];
        }
        return Db::insert('user_bind', [
            'uid'          => (int) $uid,
            'puc_type'     => (int) $pucType,
            'mackey'       => $macKey,
            'limit_change' => (int) $limitChange,
            'change_count' => 0,
            'bind_date'    => $now,
        ]);
    }

    public static function bumpBindChange($uid, $pucType, $macKey)
    {
        $row = self::findBind($uid, $pucType);
        if (!$row) {
            self::saveBind($uid, $pucType, $macKey, 0);
            return;
        }
        Db::update('user_bind', [
            'mackey'       => $macKey,
            'change_count' => (int) $row['change_count'] + 1,
            'bind_date'    => date('Y-m-d H:i:s'),
        ], ['user_bind_id' => $row['user_bind_id']]);
    }

    /* --------------------------- 序列号 --------------------------- */

    public static function findKey($keyNum)
    {
        return Db::one('SELECT * FROM key_list WHERE key_num=? LIMIT 1', [trim((string) $keyNum)]);
    }

    /** 该账号历史上激活过的序列号（换机验证用） */
    public static function keysOfUser($keyNum, $email)
    {
        return Db::one(
            'SELECT * FROM key_list WHERE key_num=? AND email=? LIMIT 1',
            [trim((string) $keyNum), mb_strtolower(trim((string) $email), 'UTF-8')]
        );
    }

    public static function markKeyUsed($keyNum, $email, $macKey, $dueDate, $actType = 0)
    {
        Db::update('key_list', [
            'state'    => 1,
            'email'    => mb_strtolower(trim((string) $email), 'UTF-8'),
            'mac_key'  => $macKey,
            'act_date' => date('Y-m-d H:i:s'),
            'time'     => $dueDate,
            'act_type' => (int) $actType,
            'new'      => 1,
        ], ['key_num' => trim((string) $keyNum)]);
    }

    public static function keyDisabled($keyNum)
    {
        return Db::one('SELECT * FROM key_list_status WHERE key_num=? AND status=0 ORDER BY Id DESC LIMIT 1', [trim((string) $keyNum)]);
    }

    /* --------------------------- 渠道 --------------------------- */

    public static function channelByKey($key, $pucType = null)
    {
        $sql = 'SELECT * FROM channel WHERE `key`=? AND main=1 LIMIT 1';
        $args = [$key];
        if ($pucType !== null && $pucType !== '') {
            $sql = 'SELECT * FROM channel WHERE `key`=? AND puc_type=? LIMIT 1';
            $args = [$key, (string) $pucType];
        }
        return Db::one($sql, $args);
    }

    public static function channelById($channelId)
    {
        return Db::one('SELECT * FROM channel WHERE channel_id=? LIMIT 1', [(int) $channelId]);
    }

    /* --------------------------- 选配功能 --------------------------- */

    public static function functions($uid)
    {
        return Db::all('SELECT function_code,function_name,due_date,status,billing_type FROM user_function WHERE uid=?', [(int) $uid]);
    }

    public static function saveFunction($uid, $code, $name, $dueDate)
    {
        $row = Db::one('SELECT user_function_id FROM user_function WHERE uid=? AND function_code=? ORDER BY user_function_id DESC LIMIT 1', [(int) $uid, $code]);
        if ($row) {
            Db::update('user_function', ['due_date' => $dueDate, 'status' => 0, 'mod_date' => date('Y-m-d H:i:s')], ['user_function_id' => $row['user_function_id']]);
            return (int) $row['user_function_id'];
        }
        return Db::insert('user_function', [
            'uid'           => (int) $uid,
            'function_name' => $name,
            'function_code' => $code,
            'due_date'      => $dueDate,
            'status'        => 0,
            'billing_type'  => 1,
        ]);
    }

    /* --------------------------- 流水 --------------------------- */

    public static function addLoginRe(array $d)
    {
        return Db::insert('user_login_re', $d);
    }

    public static function addChangeRe(array $d)
    {
        return Db::insert('amazonhelper_change_re', $d);
    }

    public static function addEventRe(array $d)
    {
        return Db::insert('event_re', $d);
    }

    public static function addChangeRequest(array $d)
    {
        return Db::insert('cx_change_request', $d);
    }

    public static function finishChangeRequest($id, $result)
    {
        if ((int) $id > 0) {
            Db::update('cx_change_request', ['result' => (int) $result, 'done_date' => date('Y-m-d H:i:s')], ['Id' => (int) $id]);
        }
    }

    public static function editRe(array $d)
    {
        return Db::insert('amazonhelper_edit_re', $d);
    }

    /* --------------------------- 配置 --------------------------- */

    /** @var array|null cx_setting 全表内存缓存，一次请求只读一遍 */
    private static $settings;

    /** cx_setting 取配置，取不到用默认值 */
    public static function setting($key, $default = null)
    {
        if (self::$settings === null) {
            self::$settings = [];
            try {
                foreach (Db::all('SELECT k,v FROM cx_setting') as $row) {
                    self::$settings[$row['k']] = $row['v'];
                }
            } catch (\Throwable $e) {
                self::$settings = [];
            }
        }
        return array_key_exists($key, self::$settings) ? self::$settings[$key] : $default;
    }

    public static function settings(array $keys, array $defaults = [])
    {
        $out = [];
        foreach ($keys as $k) {
            $out[$k] = self::setting($k, isset($defaults[$k]) ? $defaults[$k] : null);
        }
        return $out;
    }
}
