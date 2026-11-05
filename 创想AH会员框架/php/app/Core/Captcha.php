<?php
/**
 * 图形验证码：不再依赖 Cookie，改为「验证码ID + 图片」两段式。
 * 客户端先取 captcha_id 与 base64 图片，展示给用户，提交业务请求时带回落。
 * 答案放在服务端本地缓存文件里，取用即失效（一次一用）。
 */

namespace CxSoft;

class Captcha
{
    /** @var array */
    private static $cfg;

    public static function init(array $cfg)
    {
        self::$cfg = $cfg;
        if (!is_dir(self::dir())) {
            @mkdir(self::dir(), 0777, true);
        }
    }

    private static function dir()
    {
        return self::$cfg['dir'] . '/captcha';
    }

    /** 生成验证码，返回 ['id'=>..,'image_base64'=>..] */
    public static function issue()
    {
        $len  = max(3, (int) self::$cfg['length']);
        $pool = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
        $code = '';
        for ($i = 0; $i < $len; $i++) {
            $code .= $pool[random_int(0, strlen($pool) - 1)];
        }

        $id = bin2hex(random_bytes(16));
        self::store($id, $code);
        self::gc();

        return ['id' => $id, 'image_base64' => base64_encode(self::draw($code)), 'expire_in' => (int) self::$cfg['ttl_seconds']];
    }

    /**
     * 校验并消费验证码。
     * @return string '0'=通过 '5007'=不存在 '5006'=已失效 '5005'=答案错误
     */
    public static function consume($id, $answer)
    {
        $id = (string) $id;
        if ($id === '' || !preg_match('/^[0-9a-f]{32}$/', $id)) {
            return '5007';
        }
        $file = self::dir() . '/' . $id . '.json';
        if (!is_file($file)) {
            return '5007';
        }
        $saved = json_decode((string) file_get_contents($file), true);
        @unlink($file);

        if (!is_array($saved) || (int) $saved['exp'] < time()) {
            return '5006';
        }
        if (strcasecmp(trim((string) $answer), $saved['code']) !== 0) {
            return '5005';
        }
        return '0';
    }

    private static function store($id, $code)
    {
        file_put_contents(
            self::dir() . '/' . $id . '.json',
            json_encode(['code' => $code, 'exp' => time() + (int) self::$cfg['ttl_seconds']]),
            LOCK_EX
        );
    }

    /** 清掉过期文件，避免目录无限增长 */
    private static function gc()
    {
        foreach (glob(self::dir() . '/*.json') ?: [] as $f) {
            if (filemtime($f) < time() - 3600) {
                @unlink($f);
            }
        }
    }

    private static function draw($code)
    {
        $w = (int) self::$cfg['width'];
        $h = (int) self::$cfg['height'];
        $img = imagecreatetruecolor($w, $h);
        $bg  = imagecolorallocate($img, 245, 246, 248);
        imagefilledrectangle($img, 0, 0, $w, $h, $bg);

        $step = intval($w / (strlen($code) + 1));
        for ($i = 0; $i < strlen($code); $i++) {
            $color = imagecolorallocate($img, random_int(20, 110), random_int(20, 110), random_int(60, 160));
            // 内置位图字体放大绘制，避免依赖 TTF 字体文件
            imagestring($img, 5, $step * ($i + 1) - 6, random_int(6, $h - 18), $code[$i], $color);
        }
        for ($i = 0; $i < 3; $i++) {
            $line = imagecolorallocate($img, random_int(150, 220), random_int(150, 220), random_int(150, 220));
            imageline($img, random_int(0, $w), random_int(0, $h), random_int(0, $w), random_int(0, $h), $line);
        }
        for ($i = 0; $i < 60; $i++) {
            imagesetpixel($img, random_int(0, $w), random_int(0, $h), imagecolorallocate($img, random_int(120, 230), random_int(120, 230), random_int(120, 230)));
        }

        ob_start();
        imagepng($img);
        imagedestroy($img);
        return (string) ob_get_clean();
    }
}
