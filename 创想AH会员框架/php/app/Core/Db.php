<?php
/**
 * 极简 PDO 封装：只做取数/写数，不拼 SQL 之外的事情。
 * 所有查询都是预处理，参数一律绑定。
 */

namespace CxSoft;

class Db
{
    /** @var \PDO|null */
    private static $pdo;
    /** @var array */
    private static $cfg;

    public static function init(array $cfg)
    {
        self::$cfg = $cfg;
    }

    public static function pdo()
    {
        if (self::$pdo instanceof \PDO) {
            return self::$pdo;
        }
        $c = self::$cfg;
        $dsn = 'mysql:host=' . $c['host'] . ';port=' . $c['port'] . ';dbname=' . $c['name'] . ';charset=' . $c['charset'];
        self::$pdo = new \PDO($dsn, $c['user'], $c['pass'], [
            \PDO::ATTR_ERRMODE            => \PDO::ERRMODE_EXCEPTION,
            \PDO::ATTR_DEFAULT_FETCH_MODE => \PDO::FETCH_ASSOC,
            \PDO::ATTR_EMULATE_PREPARES   => false,
            \PDO::MYSQL_ATTR_INIT_COMMAND => "SET SESSION sql_mode='NO_ENGINE_SUBSTITUTION', time_zone='+08:00'",
        ]);
        return self::$pdo;
    }

    public static function query($sql, array $args = [])
    {
        $st = self::pdo()->prepare($sql);
        $st->execute($args);
        return $st;
    }

    public static function all($sql, array $args = [])
    {
        return self::query($sql, $args)->fetchAll();
    }

    /** 取一行，没有返回 null */
    public static function one($sql, array $args = [])
    {
        $row = self::query($sql, $args)->fetch();
        return $row === false ? null : $row;
    }

    /** 取单个标量，没有返回 null */
    public static function scalar($sql, array $args = [])
    {
        $value = self::query($sql, $args)->fetchColumn();
        return $value === false ? null : $value;
    }

    public static function exec($sql, array $args = [])
    {
        return self::query($sql, $args)->rowCount();
    }

    public static function insert($table, array $data)
    {
        $cols = array_keys($data);
        $sql = 'INSERT INTO `' . $table . '` (`' . implode('`,`', $cols) . '`) VALUES ('
            . implode(',', array_fill(0, count($cols), '?')) . ')';
        self::exec($sql, array_values($data));
        return (int) self::pdo()->lastInsertId();
    }

    /** 只更新给出的列；$where 为 [列 => 值] */
    public static function update($table, array $data, array $where)
    {
        $set = [];
        $args = [];
        foreach ($data as $k => $v) {
            $set[] = '`' . $k . '`=?';
            $args[] = $v;
        }
        $and = [];
        foreach ($where as $k => $v) {
            $and[] = '`' . $k . '`=?';
            $args[] = $v;
        }
        $sql = 'UPDATE `' . $table . '` SET ' . implode(',', $set) . ' WHERE ' . implode(' AND ', $and);
        return self::exec($sql, $args);
    }

    public static function begin()
    {
        self::pdo()->beginTransaction();
    }

    public static function commit()
    {
        if (self::pdo()->inTransaction()) {
            self::pdo()->commit();
        }
    }

    public static function rollback()
    {
        if (self::pdo()->inTransaction()) {
            self::pdo()->rollBack();
        }
    }
}
