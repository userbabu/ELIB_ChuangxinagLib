-- ============================================================================
-- 创想AH会员框架 · 初始化数据（可重复执行）
-- 执行顺序：先 schema.sql 再本文件。
-- ============================================================================

SET NAMES utf8mb4;

-- 新后端可调项：全部放表里，改配置不用改代码
INSERT INTO `cx_setting` (`k`, `v`, `note`) VALUES
  ('trial_days',            '7',  '开通试用天数，0=关闭试用'),
  ('change_default_limit',  '5',  '新注册账号默认可换机次数'),
  ('change_need_key',       '1',  '换机是否必须提供本账号激活过的序列号 1=需要 0=不需要'),
  ('change_max_fail',       '5',  '换机/激活连续失败次数上限'),
  ('change_lock_minutes',   '30', '超过上限后锁定时长（分钟）'),
  ('captcha_enabled',       '1',  '注册/激活/换机是否需要图形验证码'),
  ('register_max_ip_day',   '20', '同一IP每天注册上限，0=不限制'),
  ('key_default_days',      '365','旧序列号时长列不是数字时的兜底天数'),
  ('trial_is_test_value',   '2',  'is_test 列中代表「试用」的值'),
  ('paid_is_test_value',    '1',  'is_test 列中代表「正式」的值'),
  ('function_names',        '{"catalog":"类目采集","zombie":"僵尸Listing采集","v3":"V3采集模式"}', '选配功能码对应名称')
ON DUPLICATE KEY UPDATE `note` = VALUES(`note`);

-- 官方渠道（各产品线一条，key=cxangsoft，main=1 供软件载入默认 OEM）
INSERT INTO `channel`
  (`channel_id`, `name`, `main`, `offical`, `puc_type`, `setting`, `qq`, `qq2`, `wechat`,
   `status`, `count`, `reg_count`, `key`, `brand_name`, `home`, `shop_home`, `help_home`,
   `download_home`, `change_home`, `soft_name`, `theme_color`, `buy_text`, `able_reg`, `protection`)
VALUES
  (1, '创想官方-亚马逊助手', 1, 0, '1', '{}', '709005505', '703965666', 'cxruanjian', 1, 0, 1, 'cxangsoft',
   '创想软件', 'http://www.cxsup.com/', 'http://shop.cxsup.com/', 'http://blog.cxsup.com/',
   'http://ac.cxsup.com/', 'http://www.cxsup.com/change.html', '超级亚马逊跨境软件', '262150',
   '', 0, 0),
  (2, '创想官方-UPC生成器', 1, 0, '2', '{}', '709005505', '703965666', 'cxruanjian', 1, 0, 1, 'cxangsoft',
   '创想软件', 'http://www.cxsup.com/', 'http://shop.cxsup.com/', 'http://blog.cxsup.com/',
   'http://ac.cxsup.com/', 'http://www.cxsup.com/change.html', '创想EAN UPC生成器', '262150',
   '', 0, 0),
  (3, '创想官方-ASIN采集软件', 1, 0, '3', '{}', '709005505', '703965666', 'cxruanjian', 1, 0, 1, 'cxangsoft',
   '创想软件', 'http://www.cxsup.com/', 'http://shop.cxsup.com/', 'http://blog.cxsup.com/',
   'http://ac.cxsup.com/', 'http://www.cxsup.com/change.html', '创想亚马逊ASIN采集软件', '262150',
   '', 0, 0),
  (4, '创想官方-关键词助手', 1, 0, '4', '{}', '709005505', '703965666', 'cxruanjian', 1, 0, 1, 'cxangsoft',
   '创想软件', 'http://www.cxsup.com/', 'http://shop.cxsup.com/', 'http://blog.cxsup.com/',
   'http://ac.cxsup.com/', 'http://www.cxsup.com/change.html', '创想云关键词助手', '262150',
   '', 0, 0)
ON DUPLICATE KEY UPDATE `soft_name` = VALUES(`soft_name`);

-- 后台账号：口令按旧库口径 md5(密码 + 用户名) 存放，初始密码 admin888，上线请立即修改。
-- MD5() 在 MySQL 9 已移除，这里直接写死摘要值：MD5('admin888admin')
INSERT INTO `admin_login` (`username`, `password`, `UserType`, `Permission`, `state`, `Login_num`)
SELECT 'admin', '149c1eac031c5de3b6aec8a771be456a', 1, 'all', 0, 0
FROM DUAL
WHERE NOT EXISTS (SELECT 1 FROM `admin_login` WHERE `username` = 'admin');
