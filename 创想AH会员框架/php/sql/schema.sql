-- ============================================================================
-- 创想AH会员框架 · 数据库结构（全新 PHP 后端使用）
--
-- 设计原则：表结构完全沿用线上旧库 cxang_key_platform（MySQL 5.7.40），
--           旧版易语言客户端与新版 PHP 后端共用同一份数据，无需迁移。
--           新流程需要的少量额外数据放在新增的 cx_* 表里，不动旧表任何列语义。
--
-- 兼容目标：MySQL 5.7（线上）与 MySQL 8/9（本机 docker 测试）都能直接执行。
-- 字符集：线上旧表为 utf8(3 字节)，本文件保持原样；新表用 utf8mb4。
-- ============================================================================

SET NAMES utf8mb4;

-- ---------------------------------------------------------------------------
-- 1. 用户主表：一个邮箱 + 一个产品类型 + 一个渠道 = 一个账号，绑定一个机器码
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `amazonhelper_registar_key` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `qq` varchar(13) NOT NULL DEFAULT '0',
  `email` varchar(255) DEFAULT NULL,
  `mac_key` varchar(255) DEFAULT NULL COMMENT '绑定的机器码',
  `is_test` varchar(2) NOT NULL DEFAULT '2' COMMENT '默认试用版：2=试用 1=正式',
  `system` varchar(1) NOT NULL DEFAULT '0',
  `ip` varchar(255) DEFAULT NULL,
  `ip_location` varchar(255) DEFAULT NULL,
  `version` varchar(10) NOT NULL DEFAULT '0',
  `md5` varchar(32) NOT NULL DEFAULT '0',
  `puc_type` varchar(1) NOT NULL DEFAULT '1' COMMENT '1=助手 2=生成器 3=采集器 4=关键词助手',
  `referee` varchar(30) NOT NULL DEFAULT '0' COMMENT '关联账号',
  `reg_date` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `due_date` datetime DEFAULT NULL,
  `login_date` datetime DEFAULT NULL,
  `state` varchar(3) NOT NULL DEFAULT 'yes',
  `state_reason` varchar(255) DEFAULT NULL,
  `remarks` varchar(255) DEFAULT NULL COMMENT '备注',
  `order_amount` float(10,2) DEFAULT '0.00' COMMENT '出售价格',
  `order_number` varchar(50) DEFAULT '0' COMMENT '最后一次激活的序列号',
  `mod_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '修改时间',
  `soft_limit` varchar(255) DEFAULT NULL COMMENT '软件操作权限',
  `login_num` int(5) DEFAULT '0',
  `function` int(1) DEFAULT '1' COMMENT '0=旧企业版 0.9=旗舰版 1=企业版 2=高级 3=标准',
  `change_time` int(11) DEFAULT '0' COMMENT '已换机次数',
  `limit_change` int(11) DEFAULT '5' COMMENT '允许换机总次数',
  `change_mackey` int(1) DEFAULT '0' COMMENT '是否已更新机器码到新版本',
  `auto_reg` int(1) DEFAULT '0' COMMENT '是否为自动注册',
  `quit_date` datetime DEFAULT NULL COMMENT '下线时间',
  `channel_id` int(11) DEFAULT '0' COMMENT '0=官方版本',
  `channel` varchar(255) DEFAULT NULL COMMENT '购买渠道',
  `learn` varchar(255) DEFAULT NULL COMMENT '获取途径',
  `home_username` varchar(50) DEFAULT '' COMMENT '官网注册手机号',
  PRIMARY KEY (`Id`),
  UNIQUE KEY `email_2` (`email`,`puc_type`,`channel_id`),
  KEY `channel_id` (`channel_id`),
  KEY `email` (`email`),
  KEY `ip` (`ip`),
  KEY `mac_key` (`mac_key`),
  KEY `puc_type` (`puc_type`),
  KEY `due_date` (`due_date`),
  KEY `state` (`state`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='创想亚马逊软件注册用户';

-- ---------------------------------------------------------------------------
-- 2. 序列号表
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `key_list` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `key_num` varchar(255) NOT NULL DEFAULT '' COMMENT '序列号',
  `channel_id` int(11) DEFAULT '0',
  `is_test` varchar(1) NOT NULL DEFAULT '' COMMENT '2=试用 1=正式',
  `time` varchar(255) DEFAULT NULL COMMENT '授权时长描述',
  `state` int(1) DEFAULT NULL COMMENT '0=未激活 1=已激活 2=作废',
  `puc_type` int(2) DEFAULT NULL,
  `type` varchar(255) DEFAULT NULL COMMENT '序列号类型',
  `date` datetime DEFAULT NULL COMMENT '生成时间',
  `email` varchar(255) DEFAULT NULL COMMENT '激活账号',
  `mac_key` varchar(500) DEFAULT NULL COMMENT '激活时机器码',
  `act_date` datetime DEFAULT NULL COMMENT '激活时间',
  `remarks` varchar(500) DEFAULT NULL,
  `isexp` int(1) DEFAULT '0' COMMENT '0=未导出 1=已导出',
  `function` int(1) DEFAULT '1' COMMENT '0=旗舰版 1=企业 2=高级 3=标准',
  `soft_limit` text COMMENT '软件增配功能',
  `new` int(1) DEFAULT '0' COMMENT '0=旧版本预存序列号 1=新版本生成序列号',
  `act_type` int(11) DEFAULT '0' COMMENT '0=授权时长 1=选配功能 2=授权+选配',
  PRIMARY KEY (`Id`),
  UNIQUE KEY `key_num` (`key_num`),
  KEY `act_type` (`act_type`),
  KEY `channel_id` (`channel_id`),
  KEY `date` (`date`),
  KEY `email` (`email`),
  KEY `puc_type` (`puc_type`),
  KEY `state` (`state`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='序列号表';

-- 序列号回收站（作废/退回的序列号快照，结构与 key_list 一致）
CREATE TABLE IF NOT EXISTS `key_list_recovery` LIKE `key_list`;

-- 禁用序列号名单
CREATE TABLE IF NOT EXISTS `key_list_status` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `key_num` varchar(255) DEFAULT NULL COMMENT '序列号',
  `status` int(1) DEFAULT '1' COMMENT '1=可用 0=不可用',
  `status_reson` text COMMENT '对用户说明的原因',
  `inside_reson` text COMMENT '内部记录的原因',
  `date` datetime DEFAULT NULL COMMENT '状态设置日期',
  PRIMARY KEY (`Id`),
  KEY `key_num` (`key_num`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='保存禁用的序列号';

-- ---------------------------------------------------------------------------
-- 3. 机器码绑定表（一个账号在一个产品下绑定的机器码与换机配额）
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `user_bind` (
  `user_bind_id` int(11) NOT NULL AUTO_INCREMENT,
  `uid` int(11) DEFAULT NULL,
  `puc_type` int(11) DEFAULT '0' COMMENT '产品代码',
  `mackey` text COMMENT '机器码',
  `limit_change` int(11) DEFAULT '5' COMMENT '限制换绑次数',
  `change_count` int(11) DEFAULT '0' COMMENT '已换机次数',
  `bind_date` datetime DEFAULT NULL COMMENT '绑定时间',
  PRIMARY KEY (`user_bind_id`),
  KEY `uid` (`uid`),
  KEY `puc_type` (`puc_type`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='机器码绑定';

-- 换机流水
CREATE TABLE IF NOT EXISTS `amazonhelper_change_re` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `email` varchar(255) DEFAULT NULL,
  `puc_type` int(11) DEFAULT '1' COMMENT '产品类型',
  `mac_key1` varchar(255) DEFAULT NULL COMMENT '原机器码',
  `mac_key2` varchar(255) DEFAULT NULL COMMENT '更换后的机器码',
  `cd_key` varchar(255) DEFAULT NULL COMMENT '验证用的序列号',
  `change_limit` int(11) DEFAULT NULL COMMENT '可用总计换机数',
  `change_time` int(11) DEFAULT NULL COMMENT '已使用换机数',
  `date` datetime DEFAULT NULL,
  `ip` varchar(20) DEFAULT NULL,
  PRIMARY KEY (`Id`),
  KEY `email` (`email`),
  KEY `date` (`date`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='换机记录表';

-- 登录记录
CREATE TABLE IF NOT EXISTS `user_login_re` (
  `user_login_re_id` int(11) NOT NULL AUTO_INCREMENT,
  `uid` int(11) DEFAULT NULL COMMENT '用户UID',
  `date` timestamp NULL DEFAULT CURRENT_TIMESTAMP COMMENT '时间',
  `mackey` varchar(300) DEFAULT NULL COMMENT '机器码',
  `ip` varchar(20) DEFAULT NULL COMMENT 'IP地址',
  `ip_local` varchar(100) DEFAULT NULL,
  `puctype` int(11) DEFAULT NULL COMMENT '产品代码',
  `channel_id` int(11) DEFAULT '0' COMMENT '代理类型',
  `version` varchar(10) DEFAULT '' COMMENT '程序版本',
  `app_hash` varchar(32) DEFAULT '' COMMENT '程序HASH',
  `platform` int(11) DEFAULT '-1' COMMENT '平台代码',
  PRIMARY KEY (`user_login_re_id`),
  KEY `uid` (`uid`),
  KEY `ip` (`ip`),
  KEY `puctype` (`puctype`),
  KEY `date` (`date`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='用户登录记录';

-- 客户端退出/事件流水
CREATE TABLE IF NOT EXISTS `event_re` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `uid` int(11) DEFAULT NULL,
  `email` varchar(255) DEFAULT NULL,
  `mac_key` varchar(255) DEFAULT NULL,
  `event` varchar(30) DEFAULT NULL COMMENT 'quit=关闭软件 login=登录 change=换机 act=激活',
  `puc_type` int(2) DEFAULT NULL,
  `channel_id` int(11) DEFAULT '0',
  `version` varchar(10) DEFAULT NULL,
  `ip` varchar(20) DEFAULT NULL,
  `date` datetime DEFAULT NULL,
  PRIMARY KEY (`Id`),
  KEY `uid` (`uid`),
  KEY `email` (`email`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='客户端事件记录';

-- ---------------------------------------------------------------------------
-- 4. 选配功能（按功能码单独计算到期时间）
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `user_function` (
  `user_function_id` int(11) NOT NULL AUTO_INCREMENT,
  `uid` int(11) DEFAULT NULL,
  `function_name` varchar(50) DEFAULT NULL,
  `function_code` varchar(20) DEFAULT NULL,
  `due_date` datetime DEFAULT NULL COMMENT '到期时间',
  `status` int(11) DEFAULT '0' COMMENT '0=可用',
  `billing_type` int(1) DEFAULT '1' COMMENT '0=不限时间 1=限制时间',
  `add_date` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `mod_date` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '修改时间',
  PRIMARY KEY (`user_function_id`),
  KEY `due_date` (`due_date`),
  KEY `function_code` (`function_code`),
  KEY `status` (`status`),
  KEY `uid` (`uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='选配功能';

-- ---------------------------------------------------------------------------
-- 5. 渠道 / 代理（OEM 信息与程序配置来源）
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `channel` (
  `channel_id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(50) DEFAULT NULL,
  `main` int(11) DEFAULT NULL COMMENT '0=次要 1=主要渠道（注册与载入都取此渠道）',
  `offical` int(1) DEFAULT NULL COMMENT '0=官方 1=代理',
  `puc_type` varchar(1) DEFAULT NULL,
  `setting` text COMMENT '程序设置(JSON, 会覆盖程序配置)',
  `phone` varchar(13) DEFAULT NULL COMMENT '渠道联系手机',
  `wechat` varchar(20) DEFAULT NULL,
  `qq` varchar(13) DEFAULT NULL,
  `qq2` varchar(13) DEFAULT NULL,
  `discount` float(3,2) DEFAULT NULL COMMENT '折扣',
  `status` int(1) DEFAULT '1' COMMENT '0=禁止 1=可用',
  `count` int(11) DEFAULT '0' COMMENT '序列号生成数量',
  `reg_count` int(10) DEFAULT '1' COMMENT '注册用户数',
  `key` varchar(15) DEFAULT NULL COMMENT '代理唯一KEY(main=1 的那条用于软件载入默认OEM)',
  `brand_name` varchar(20) DEFAULT NULL,
  `home` varchar(255) DEFAULT NULL,
  `server_url` varchar(255) DEFAULT NULL,
  `server_url2` varchar(255) DEFAULT NULL,
  `shop_home` varchar(255) DEFAULT NULL,
  `help_home` varchar(255) DEFAULT NULL,
  `download_home` varchar(255) DEFAULT NULL,
  `change_home` varchar(255) DEFAULT NULL,
  `soft_name` varchar(20) DEFAULT NULL,
  `theme_color` varchar(10) DEFAULT NULL,
  `buy_text` text,
  `soft_home_url` varchar(255) DEFAULT NULL,
  `soft_home_url2` varchar(255) DEFAULT NULL,
  `able_reg` int(1) DEFAULT '0' COMMENT '0=可注册 1=不可注册',
  `protection` int(1) DEFAULT '0' COMMENT '1=只允许激活本渠道序列号',
  PRIMARY KEY (`channel_id`),
  KEY `key` (`key`),
  KEY `main` (`main`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='激活码渠道';

-- ---------------------------------------------------------------------------
-- 6. 后台账号与审计
-- ---------------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `admin_login` (
  `admin_uid` int(11) NOT NULL AUTO_INCREMENT,
  `username` varchar(255) DEFAULT NULL,
  `password` varchar(255) DEFAULT NULL,
  `UserType` int(1) DEFAULT NULL,
  `Permission` varchar(255) DEFAULT NULL,
  `LoginDate` datetime DEFAULT NULL,
  `state` int(1) DEFAULT '0',
  `Login_num` int(11) DEFAULT '0',
  PRIMARY KEY (`admin_uid`),
  UNIQUE KEY `username` (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=gbk ROW_FORMAT=DYNAMIC COMMENT='后台网站管理用户表';

CREATE TABLE IF NOT EXISTS `admin_log` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `log_title` varchar(255) DEFAULT NULL COMMENT '标题',
  `log_text` varchar(500) DEFAULT NULL COMMENT '文本',
  `date` datetime DEFAULT NULL COMMENT '记录日期',
  `username` varchar(255) DEFAULT NULL COMMENT '操作用户',
  `type` int(1) DEFAULT NULL COMMENT '1=登录 2=退出 3=修改用户信息 4=删除用户 5=查询序列号 6=提取序列号',
  `ip` varchar(60) DEFAULT '',
  `ua` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`Id`),
  KEY `date` (`date`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='操作日志';

CREATE TABLE IF NOT EXISTS `admin_login_re` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `username` varchar(255) DEFAULT NULL,
  `date` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `ua` varchar(255) DEFAULT NULL,
  `ip` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `admin_risk` (
  `admin_risk_id` int(11) NOT NULL AUTO_INCREMENT,
  `ip` varchar(15) DEFAULT NULL COMMENT 'IP地址',
  `ua` text COMMENT '浏览器UA',
  `date` datetime DEFAULT NULL COMMENT '记录时间',
  `status` int(1) DEFAULT '0' COMMENT '0=允许操作 1=限制操作',
  `last_date` datetime DEFAULT NULL COMMENT '最后尝试时间',
  `count` int(11) DEFAULT '0' COMMENT '操作次数',
  `name` varchar(20) DEFAULT NULL,
  `expire` bigint(14) DEFAULT NULL COMMENT '限制过期时间戳 0=永不过期',
  PRIMARY KEY (`admin_risk_id`),
  UNIQUE KEY `ip` (`ip`,`name`),
  KEY `ip_2` (`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

-- 用户资料变更流水
CREATE TABLE IF NOT EXISTS `amazonhelper_edit_re` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `uid` int(11) NOT NULL DEFAULT '0' COMMENT '会员ID',
  `email` varchar(255) DEFAULT NULL COMMENT '修改后的登录邮箱',
  `op` varchar(20) NOT NULL DEFAULT 'edit' COMMENT 'edit=资料修改 email=换绑邮箱',
  `diff` text COMMENT '变更明细JSON {字段:{name,old,new}}',
  `snapshot` text COMMENT '修改后快照JSON',
  `operator` varchar(60) DEFAULT NULL COMMENT '后台操作账号名',
  `operator_uid` int(11) NOT NULL DEFAULT '0',
  `ip` varchar(45) DEFAULT NULL,
  `date` datetime DEFAULT NULL,
  PRIMARY KEY (`Id`),
  KEY `uid` (`uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='用户资料变更记录';

CREATE TABLE IF NOT EXISTS `user_manager_re` (
  `user_manager_re_id` int(11) NOT NULL AUTO_INCREMENT,
  `uid` int(11) DEFAULT NULL,
  `admin_uid` int(11) DEFAULT NULL,
  `text` text,
  `date` datetime DEFAULT NULL,
  PRIMARY KEY (`user_manager_re_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='用户管理记录';

-- 服务端日志
CREATE TABLE IF NOT EXISTS `log` (
  `log_id` int(11) NOT NULL AUTO_INCREMENT,
  `text` varchar(500) DEFAULT NULL,
  `add_date` datetime DEFAULT NULL,
  `uid` int(11) DEFAULT NULL,
  `sumbit` text,
  `file` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`log_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='服务端日志';

CREATE TABLE IF NOT EXISTS `server_debug_log` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `username` varchar(15) DEFAULT NULL,
  `puc_type` int(1) DEFAULT NULL,
  `value` varchar(1000) DEFAULT NULL,
  `date` datetime DEFAULT NULL,
  `sumbit` text,
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='服务端错误日志';

-- 客户端错误上报（旧客户端 bug.php 使用，结构保持原样）
CREATE TABLE IF NOT EXISTS `cx_bug_re` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `uid` int(11) DEFAULT NULL,
  `email` varchar(255) DEFAULT NULL,
  `code` varchar(11) DEFAULT NULL COMMENT '错误代码',
  `type` int(11) DEFAULT NULL COMMENT '0=主动提交 1=程序错误报告',
  `content_title` varchar(3000) DEFAULT NULL,
  `content_data` text,
  `date` datetime DEFAULT NULL,
  `md5` varchar(32) DEFAULT NULL,
  `run_data` text,
  `puc_type` int(2) DEFAULT NULL,
  `puc_ver` varchar(255) DEFAULT NULL COMMENT '产品版本',
  `ip` varchar(16) DEFAULT NULL,
  `ip_location` varchar(50) DEFAULT NULL,
  `flag` int(11) DEFAULT NULL COMMENT '星标',
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='客户端错误日志记录';

CREATE TABLE IF NOT EXISTS `item_count` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `item` varchar(255) DEFAULT NULL,
  `value` int(11) DEFAULT '0',
  PRIMARY KEY (`Id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

CREATE TABLE IF NOT EXISTS `user_auth` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='授权表';

-- ===========================================================================
-- 以下为全新后端新增表（旧客户端完全不接触）
-- ===========================================================================

-- 站点级配置：新后端的所有可调项都放这里，改配置不用改代码、不用重新发布
CREATE TABLE IF NOT EXISTS `cx_setting` (
  `k` varchar(64) NOT NULL,
  `v` varchar(1000) DEFAULT NULL,
  `note` varchar(255) DEFAULT NULL COMMENT '说明',
  `mod_date` datetime DEFAULT NULL,
  PRIMARY KEY (`k`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='新后端配置';

-- 授权接口防爆破计数：按 目标(邮箱/序列号)+IP 统计失败次数并临时锁定
CREATE TABLE IF NOT EXISTS `cx_auth_attempt` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `scene` varchar(20) NOT NULL COMMENT 'change=换机 act=激活 reg=注册',
  `target` varchar(255) NOT NULL COMMENT '被校验的目标（小写邮箱或序列号）',
  `ip` varchar(45) DEFAULT NULL,
  `fail_count` int(11) DEFAULT '0',
  `lock_until` datetime DEFAULT NULL COMMENT '锁定到期时间，NULL=未锁定',
  `last_date` datetime DEFAULT NULL,
  PRIMARY KEY (`Id`),
  UNIQUE KEY `scene` (`scene`,`target`,`ip`),
  KEY `lock_until` (`lock_until`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='授权接口失败计数与锁定';

-- 换机申请流水：记录「要求换机 → 输入序列号验证 → 成功/失败」全过程
CREATE TABLE IF NOT EXISTS `cx_change_request` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `uid` int(11) DEFAULT NULL,
  `email` varchar(255) DEFAULT NULL,
  `puc_type` int(2) DEFAULT NULL,
  `channel_id` int(11) DEFAULT '0',
  `old_mac` varchar(255) DEFAULT NULL,
  `new_mac` varchar(255) DEFAULT NULL,
  `key_num` varchar(255) DEFAULT NULL COMMENT '用户填写的序列号(失败时也留痕)',
  `result` int(1) DEFAULT '0' COMMENT '0=待验证 1=成功 2=序列号不符 3=次数不足 4=账号异常 5=被锁定',
  `ip` varchar(45) DEFAULT NULL,
  `date` datetime DEFAULT NULL,
  `done_date` datetime DEFAULT NULL,
  PRIMARY KEY (`Id`),
  KEY `email` (`email`),
  KEY `uid` (`uid`),
  KEY `date` (`date`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='换机请求流水';
