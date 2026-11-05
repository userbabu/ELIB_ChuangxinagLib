<?php
/**
 * 机器码解析。
 *
 * 机器码由易语言客户端生成后传入，服务端只负责校验与解析，绝不生成。
 * 结构 = base64( 左校验码 - 硬盘段 - CPU序列号 - 长度/CRC段 - 产品类型 - 右校验码 [- 计算机名CRC32 - MACVER2] )
 *   V1: 6 段，第 4 段为 CPU段+硬盘段的 CRC32
 *   V2: 8 段，第 4 段为长度，末尾追加计算机名 CRC32 与固定标识 MACVER2
 */

namespace CxSoft;

class MachineCode
{
    const V2_SUFFIX = 'MACVER2';

    /** base64 -> 分段数组；解析失败返回 [] */
    public static function segments($macKey)
    {
        $raw = base64_decode(trim((string) $macKey), true);
        if ($raw === false || strpos($raw, '-') === false) {
            return [];
        }
        $parts = explode('-', $raw);
        return count($parts) < 5 ? [] : $parts;
    }

    /** 是否具备机器码的基本形态（与旧客户端判定口径一致：至少 5 个短横线） */
    public static function looksLike($macKey)
    {
        $raw = base64_decode(trim((string) $macKey), true);
        return $raw !== false && substr_count($raw, '-') >= 5;
    }

    /** 版本号：2=MACVER2 新版，1=旧版，0=无法识别 */
    public static function version($macKey)
    {
        $parts = self::segments($macKey);
        if (!$parts) {
            return 0;
        }
        return (end($parts) === self::V2_SUFFIX) ? 2 : 1;
    }

    /** 机器码里声明的产品类型 */
    public static function productType($macKey)
    {
        $parts = self::segments($macKey);
        return isset($parts[4]) ? $parts[4] : '';
    }

    /** CPU 序列号段，用于「同一台电脑」比对 */
    public static function cpuId($macKey)
    {
        $parts = self::segments($macKey);
        return isset($parts[2]) ? $parts[2] : '';
    }

    /** 计算机名 CRC32 段（V2 才有），用于同一台电脑换了系统盘后的弱匹配 */
    public static function hostCrc($macKey)
    {
        $parts = self::segments($macKey);
        if (count($parts) >= 7 && end($parts) === self::V2_SUFFIX) {
            return $parts[6];
        }
        return '';
    }

    /**
     * 校验机器码自洽性（各段长度/校验位是否互相吻合）。
     * 只校验结构，不校验硬件真伪——硬件信息由客户端采集，服务端无法复核。
     */
    public static function check($macKey, $expectPucType = null)
    {
        $parts = self::segments($macKey);
        if (!$parts) {
            return ['3001', '机器码段数不足'];
        }
        $seg2 = $parts[1];
        $cpu  = $parts[2];
        if (!ctype_digit($seg2) || $cpu === '' || !ctype_digit($parts[0])) {
            return ['3001', '机器码数字段异常'];
        }
        $len = strlen($seg2) + strlen($cpu);
        if ($parts[0] !== substr((string) abs($len), 0, 2)) {
            return ['3001', '机器码左校验位不符'];
        }
        $right = substr((string) abs($len * (int) $parts[0]), -2);
        $version = self::version($macKey);

        if ($version === 2) {
            if ($parts[3] !== (string) $len) {
                return ['3001', '机器码长度段不符'];
            }
            if ($parts[5] !== $right) {
                return ['3001', '机器码右校验位不符'];
            }
        } elseif ($version === 1) {
            $expectCrc = strtoupper(sprintf('%08X', crc32($seg2 . $cpu) & 0xFFFFFFFF));
            if ($parts[3] !== $expectCrc) {
                return ['3001', '机器码校验和不符'];
            }
            if ($parts[5] !== $right) {
                return ['3001', '机器码右校验位不符'];
            }
        } else {
            return ['3001', '机器码标识缺失'];
        }

        if ($expectPucType !== null && $expectPucType !== '' && $parts[4] !== (string) $expectPucType) {
            return ['3001', '机器码产品类型与本软件不一致'];
        }
        return ['0', ''];
    }

    /** 归一化：去空白、大写无关，仅用于比较 */
    public static function canonical($macKey)
    {
        return trim((string) $macKey);
    }
}
