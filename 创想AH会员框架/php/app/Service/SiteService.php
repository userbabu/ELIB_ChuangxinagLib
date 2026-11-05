<?php
/**
 * 站点基础服务：服务器时间、验证码下发。
 */

namespace CxSoft\Service;

use CxSoft\Captcha;
use CxSoft\Repo;

class SiteService
{
    /** 客户端用它校准本机时间，避免用户改系统时间绕过到期判断 */
    public static function time()
    {
        $now = time();
        return [
            'timestamp'    => (int) ($now * 1000),
            'datetime'     => date('Y-m-d H:i:s', $now),
            'date'         => date('Y-m-d', $now),
            'utc_offset'   => 8,
        ];
    }

    /** 图形验证码：返回 id + base64 PNG，客户端自己显示，不依赖 Cookie */
    public static function captcha(array $p)
    {
        $data = Captcha::issue();
        $data['enabled'] = (int) Repo::setting('captcha_enabled', 1) === 1;
        return $data;
    }
}
