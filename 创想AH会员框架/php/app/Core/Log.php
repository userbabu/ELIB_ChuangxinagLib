<?php
/**
 * 服务端日志：文件 + 数据库双写，写库失败不影响主流程。
 */

namespace CxSoft;

class Log
{
    /** @var array */
    private static $cfg;

    public static function init(array $cfg)
    {
        self::$cfg = $cfg;
        if (!is_dir($cfg['dir'])) {
            @mkdir($cfg['dir'], 0777, true);
        }
    }

    public static function info($text, $uid = null, $submit = null)
    {
        self::write('INFO', $text, $uid, $submit);
    }

    public static function error($text, $uid = null, $submit = null)
    {
        self::write('ERROR', $text, $uid, $submit);
    }

    private static function file($line)
    {
        @file_put_contents(
            self::$cfg['dir'] . '/api-' . date('Y-m-d') . '.log',
            '[' . date('Y-m-d H:i:s') . '] ' . $line . PHP_EOL,
            FILE_APPEND | LOCK_EX
        );
    }

    private static function write($level, $text, $uid, $submit)
    {
        $text = mb_strcut((string) $text, 0, 1800, 'UTF-8');
        self::file($level . ' ' . $text);

        if (empty(self::$cfg['to_db'])) {
            return;
        }
        try {
            if ($level === 'ERROR') {
                Db::exec(
                    'INSERT INTO server_debug_log (username, puc_type, value, date, sumbit) VALUES (?,?,?,?,?)',
                    ['cxah', 0, mb_substr($text, 0, 900, 'UTF-8'), date('Y-m-d H:i:s'), self::trim($submit)]
                );
            } elseif (!empty(self::$cfg['sql_error'])) {
                Db::exec(
                    'INSERT INTO log (text, add_date, uid, sumbit, file) VALUES (?,?,?,?,?)',
                    [mb_substr($text, 0, 480, 'UTF-8'), date('Y-m-d H:i:s'), $uid, self::trim($submit), 'cxah/api.php']
                );
            }
        } catch (\Throwable $e) {
            self::file('LOG-DB-FAIL ' . $e->getMessage());
        }
    }

    private static function trim($submit)
    {
        if (!is_string($submit)) {
            return null;
        }
        return mb_strcut($submit, 0, 4000, 'UTF-8');
    }
}
