<?php
/**
 * 渠道（OEM/代理）：客户端启动时取品牌信息，注册/激活时校验渠道可用性。
 */

namespace CxSoft\Service;

use CxSoft\ApiException;
use CxSoft\Repo;

class ChannelService
{
    /** 校验渠道可用，返回渠道行；channel_id=0 视为官方 */
    public static function requireUsable($channelId)
    {
        $channelId = (int) $channelId;
        if ($channelId === 0) {
            $row = Repo::channelByKey('cxangsoft');
            if ($row) {
                return $row;
            }
            return ['channel_id' => 0, 'status' => 1, 'able_reg' => 0, 'protection' => 0, 'name' => '官方'];
        }
        $row = Repo::channelById($channelId);
        if (!$row) {
            throw new ApiException('7001', null);
        }
        if ((int) $row['status'] !== 1) {
            throw new ApiException('7001', '该渠道已停止服务，请联系客服。');
        }
        return $row;
    }

    /**
     * 客户端载入 OEM：按 代理KEY(+产品类型) 取 main=1 的渠道。
     * 只输出软件需要的字段，密码类字段一律不外发。
     */
    public static function oem(array $p, array $ctx)
    {
        $key = trim((string) (isset($p['key']) ? $p['key'] : 'cxangsoft'));
        if ($key === '') {
            $key = 'cxangsoft';
        }
        $pucType = isset($p['puc_type']) && $p['puc_type'] !== '' ? (string) $p['puc_type'] : null;

        $row = Repo::channelByKey($key, $pucType) ?: Repo::channelByKey($key);
        if (!$row) {
            $row = Repo::channelByKey('cxangsoft');
        }
        if (!$row) {
            throw new ApiException('7001', '未找到该渠道的软件配置，请联系客服。');
        }

        $setting = json_decode((string) $row['setting'], true);
        if (!is_array($setting)) {
            $setting = new \stdClass();
        }

        return [
            // 官方渠道在用户表里历来记 channel_id=0，这里保持一致，避免新注册的账号落到渠道自增 ID 上
            'channel_id'    => ((int) $row['offical'] === 0) ? 0 : (int) $row['channel_id'],
            'key'           => (string) $row['key'],
            'official'      => (int) $row['offical'] === 0,
            'brand_name'    => (string) $row['brand_name'],
            'soft_name'     => (string) $row['soft_name'],
            'puc_type'      => (string) $row['puc_type'],
            'theme_color'   => (string) $row['theme_color'],
            'qq'            => (string) $row['qq'],
            'qq2'           => (string) $row['qq2'],
            'wechat'        => (string) $row['wechat'],
            'phone'         => (string) $row['phone'],
            'home'          => (string) $row['home'],
            'shop_home'     => (string) $row['shop_home'],
            'help_home'     => (string) $row['help_home'],
            'download_home' => (string) $row['download_home'],
            'change_home'   => (string) $row['change_home'],
            'buy_text'      => (string) $row['buy_text'],
            'ad_url'        => (string) $row['soft_home_url'],
            'ad_url2'       => (string) $row['soft_home_url2'],
            'server_url'    => (string) $row['server_url'],
            'server_url2'   => (string) $row['server_url2'],
            'able_reg'      => (int) $row['able_reg'],
            'protection'    => (int) $row['protection'],
            'setting'       => $setting,
        ];
    }
}
