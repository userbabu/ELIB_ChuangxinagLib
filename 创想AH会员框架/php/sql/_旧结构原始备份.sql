# Host: 47.119.166.45:59997  (Version 5.7.40-log)
# Date: 2026-10-18 21:21:44
# Generator: MySQL-Front 6.1  (Build 1.15)


#
# Structure for table "admin_log"
#

CREATE TABLE `admin_log` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `log_title` varchar(255) DEFAULT NULL COMMENT '标题',
  `log_text` varchar(500) DEFAULT NULL COMMENT '文本',
  `date` datetime DEFAULT NULL COMMENT '记录日期',
  `username` varchar(255) DEFAULT NULL COMMENT '操作用户',
  `type` int(1) DEFAULT NULL COMMENT '1=登录 2=退出 3=修改用户信息 4=删除用户 5=查询序列号 6=提取序列号',
  `ip` varchar(60) DEFAULT '' COMMENT '20',
  `ua` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB AUTO_INCREMENT=3874 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='操作日志';

#
# Structure for table "admin_login"
#

CREATE TABLE `admin_login` (
  `admin_uid` int(11) NOT NULL AUTO_INCREMENT,
  `username` varchar(255) DEFAULT NULL,
  `password` varchar(255) DEFAULT NULL,
  `UserType` int(1) DEFAULT NULL,
  `Permission` varchar(255) DEFAULT NULL,
  `LoginDate` datetime DEFAULT NULL,
  `state` int(1) DEFAULT '0',
  `Login_num` int(11) DEFAULT '0',
  PRIMARY KEY (`admin_uid`)
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=gbk ROW_FORMAT=DYNAMIC COMMENT='后台网站管理用户表';

#
# Structure for table "admin_login_re"
#

CREATE TABLE `admin_login_re` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `username` varchar(255) DEFAULT NULL,
  `date` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `ua` varchar(255) DEFAULT NULL,
  `ip` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB AUTO_INCREMENT=294 DEFAULT CHARSET=utf8mb4;

#
# Structure for table "admin_risk"
#

CREATE TABLE `admin_risk` (
  `admin_risk_id` int(11) NOT NULL AUTO_INCREMENT,
  `ip` varchar(15) DEFAULT NULL COMMENT 'IP地址',
  `ua` text COMMENT '浏览器UA',
  `date` datetime DEFAULT NULL COMMENT '记录时间',
  `status` int(1) DEFAULT '0' COMMENT '0= 允许操作 1=限制操作',
  `last_date` datetime DEFAULT NULL COMMENT '最后尝试登录时间',
  `count` int(11) DEFAULT '0' COMMENT '操作次数',
  `name` varchar(20) DEFAULT NULL,
  `expire` bigint(14) DEFAULT NULL COMMENT '限制过期时间 0=不允许过期',
  PRIMARY KEY (`admin_risk_id`),
  UNIQUE KEY `ip` (`ip`,`name`),
  KEY `ip_2` (`ip`)
) ENGINE=InnoDB AUTO_INCREMENT=79 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

#
# Structure for table "amazonhelper_change_re"
#

CREATE TABLE `amazonhelper_change_re` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `email` varchar(255) DEFAULT NULL,
  `puc_type` int(11) DEFAULT '1' COMMENT '产品类型',
  `mac_key1` varchar(255) DEFAULT NULL COMMENT '原机器码',
  `mac_key2` varchar(255) DEFAULT NULL COMMENT '更换后的机器码',
  `cd_key` varchar(255) DEFAULT NULL COMMENT '序列号',
  `change_limit` int(11) DEFAULT NULL COMMENT '可用总计换机数',
  `change_time` int(11) DEFAULT NULL COMMENT '已使用换机数',
  `date` datetime DEFAULT NULL,
  `ip` varchar(20) DEFAULT NULL,
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB AUTO_INCREMENT=18528 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='换机记录表';

#
# Structure for table "amazonhelper_edit_re"
#

CREATE TABLE `amazonhelper_edit_re` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `uid` int(11) NOT NULL DEFAULT '0' COMMENT '会员ID(amazonhelper_registar_key.Id)',
  `email` varchar(255) DEFAULT NULL COMMENT '修改后的登录邮箱',
  `op` varchar(20) NOT NULL DEFAULT 'edit' COMMENT 'edit=资料修改 email=换绑邮箱',
  `diff` text COMMENT '变更明细JSON {字段:{name,old,new}}',
  `snapshot` text COMMENT '修改后快照JSON {fields:{...}, keys:[受影响序列号]}',
  `operator` varchar(60) DEFAULT NULL COMMENT '后台操作账号名',
  `operator_uid` int(11) NOT NULL DEFAULT '0' COMMENT '后台操作账号UID',
  `ip` varchar(45) DEFAULT NULL COMMENT '操作者IP(留 IPv6 长度)',
  `date` datetime DEFAULT NULL COMMENT '操作时间',
  PRIMARY KEY (`Id`),
  KEY `uid` (`uid`)
) ENGINE=MyISAM AUTO_INCREMENT=6 DEFAULT CHARSET=utf8;

#
# Structure for table "amazonhelper_registar_key"
#

CREATE TABLE `amazonhelper_registar_key` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `qq` varchar(13) NOT NULL DEFAULT '0',
  `email` varchar(255) DEFAULT NULL,
  `mac_key` varchar(255) DEFAULT NULL,
  `is_test` varchar(2) NOT NULL DEFAULT '2' COMMENT '默认为试用版',
  `system` varchar(1) NOT NULL DEFAULT '0',
  `ip` varchar(255) DEFAULT NULL,
  `ip_location` varchar(255) DEFAULT NULL,
  `version` varchar(10) NOT NULL DEFAULT '0',
  `md5` varchar(32) NOT NULL DEFAULT '0',
  `puc_type` varchar(1) NOT NULL DEFAULT '1' COMMENT '1=助手2=生成器 3=采集器 4=关键词助手',
  `referee` varchar(30) NOT NULL DEFAULT '0' COMMENT '改为关联账号',
  `reg_date` timestamp NULL DEFAULT CURRENT_TIMESTAMP,
  `due_date` datetime DEFAULT NULL,
  `login_date` datetime DEFAULT NULL,
  `state` varchar(3) NOT NULL DEFAULT 'yes',
  `state_reason` varchar(255) DEFAULT NULL,
  `remarks` varchar(255) DEFAULT NULL COMMENT '备注',
  `order_amount` float(10,2) DEFAULT '0.00' COMMENT '出售价格',
  `order_number` varchar(50) DEFAULT '0' COMMENT '改为 序列号',
  `mod_time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '修改时间',
  `soft_limit` varchar(255) DEFAULT NULL COMMENT '软件操作权限',
  `login_num` int(5) DEFAULT '0',
  `function` int(1) DEFAULT '1' COMMENT '0=旧企业版 0.9=旗舰版 1=企业版 2=高级 3=标准',
  `change_time` int(11) DEFAULT '0' COMMENT '已换机次数',
  `limit_change` int(11) DEFAULT '5' COMMENT '限制换机次数',
  `change_mackey` int(1) DEFAULT '0' COMMENT '是否已更新过机器码到新版本',
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
  KEY `puc_type` (`puc_type`)
) ENGINE=InnoDB AUTO_INCREMENT=41643 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='创想亚马逊软件注册用户';

#
# Structure for table "channel"
#

CREATE TABLE `channel` (
  `channel_id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(50) DEFAULT NULL,
  `main` int(11) DEFAULT NULL COMMENT '某个代理可以有多个渠道,但必须有一个是1  0=不是主要 1=为主要渠道 注册用户使用此渠道 打开软件也使用此渠道',
  `offical` int(1) DEFAULT NULL COMMENT '0=官方 1=代理  务必设置',
  `puc_type` varchar(1) DEFAULT NULL,
  `setting` text COMMENT '程序设置(会覆盖程序配置)',
  `phone` varchar(13) DEFAULT NULL COMMENT '渠道联系手机',
  `wechat` varchar(20) DEFAULT NULL COMMENT '微信',
  `qq` varchar(13) DEFAULT NULL,
  `qq2` varchar(13) DEFAULT NULL,
  `discount` float(3,2) DEFAULT NULL COMMENT '折扣',
  `status` int(1) DEFAULT '1' COMMENT '0=禁止  1=可用',
  `count` int(11) DEFAULT '0' COMMENT '序列号生成数量',
  `reg_count` int(10) DEFAULT '1' COMMENT '注册用户数',
  `key` varchar(15) DEFAULT NULL COMMENT '代理的唯一KEY, 可以有多个,但必须保证有一个main=1的值 用于软件载入默认的OEM信息',
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
  `soft_home_url` varchar(255) DEFAULT NULL COMMENT '软件首页左下角广告URL',
  `soft_home_url2` varchar(255) DEFAULT NULL COMMENT '行业动态广告URL',
  `able_reg` int(1) DEFAULT '0' COMMENT '0=可注册  1=不可注册',
  `protection` int(1) DEFAULT '0' COMMENT '0=不保护激活 1=保护激活  保护激活表示此类型的用户只能激活此渠道的序列号',
  PRIMARY KEY (`channel_id`)
) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='激活码渠道';

#
# Structure for table "cx_bug_re"
#

CREATE TABLE `cx_bug_re` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `uid` int(11) DEFAULT NULL,
  `email` varchar(255) DEFAULT NULL,
  `code` varchar(11) DEFAULT NULL COMMENT '错误代码',
  `type` int(11) DEFAULT NULL COMMENT '0=主动提交 1=程序错误报告',
  `content_title` varchar(3000) DEFAULT NULL COMMENT '错误标题',
  `content_data` text,
  `date` datetime DEFAULT NULL,
  `md5` varchar(32) DEFAULT NULL COMMENT '提交时程序的MD5',
  `run_data` text COMMENT '运行环境信息',
  `puc_type` int(2) DEFAULT NULL,
  `puc_ver` varchar(255) DEFAULT NULL COMMENT '产品版本',
  `ip` varchar(16) DEFAULT NULL,
  `ip_location` varchar(50) DEFAULT NULL,
  `flag` int(11) DEFAULT NULL COMMENT '星标',
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB AUTO_INCREMENT=369578 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='客户端错误日志记录';

#
# Structure for table "item_count"
#

CREATE TABLE `item_count` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `item` varchar(255) DEFAULT NULL,
  `value` int(11) DEFAULT '0',
  PRIMARY KEY (`Id`)
) ENGINE=MyISAM AUTO_INCREMENT=6 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

#
# Structure for table "key_list"
#

CREATE TABLE `key_list` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `key_num` varchar(255) NOT NULL DEFAULT '',
  `channel_id` int(11) DEFAULT '0',
  `is_test` varchar(1) NOT NULL DEFAULT '',
  `time` varchar(255) DEFAULT NULL,
  `state` int(1) DEFAULT NULL,
  `puc_type` int(2) DEFAULT NULL,
  `type` varchar(255) DEFAULT NULL COMMENT '序列号类型',
  `date` datetime DEFAULT NULL,
  `email` varchar(255) DEFAULT NULL,
  `mac_key` varchar(500) DEFAULT NULL,
  `act_date` datetime DEFAULT NULL,
  `remarks` varchar(500) DEFAULT NULL,
  `isexp` int(1) DEFAULT '0' COMMENT '是否已导出 0=未导出 1=已导出',
  `function` int(1) DEFAULT '1' COMMENT '0=旗舰版 1=企业 2=高级版 3=标准版',
  `soft_limit` text COMMENT '软件增配功能',
  `new` int(1) DEFAULT '0' COMMENT '0=旧版本预存序列号 1=新版本生成序列号',
  `act_type` int(11) DEFAULT '0' COMMENT '0=授权时长版本激活  1=选配功能激活  2=授权+选配功能激活 时长一致',
  PRIMARY KEY (`Id`),
  UNIQUE KEY `key_num` (`key_num`),
  KEY `act_type` (`act_type`),
  KEY `channel_id` (`channel_id`),
  KEY `date` (`date`),
  KEY `email` (`email`),
  KEY `puc_type` (`puc_type`)
) ENGINE=InnoDB AUTO_INCREMENT=46988 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='序列号表';

#
# Structure for table "key_list_recovery"
#

CREATE TABLE `key_list_recovery` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `key_num` varchar(255) NOT NULL DEFAULT '',
  `channel_id` int(11) DEFAULT '0',
  `is_test` varchar(1) NOT NULL DEFAULT '',
  `time` varchar(255) DEFAULT NULL,
  `state` int(1) DEFAULT NULL,
  `puc_type` int(2) DEFAULT NULL,
  `type` varchar(255) DEFAULT NULL COMMENT '序列号类型',
  `date` datetime DEFAULT NULL,
  `email` varchar(255) DEFAULT NULL,
  `mac_key` varchar(500) DEFAULT NULL,
  `act_date` datetime DEFAULT NULL,
  `remarks` varchar(500) DEFAULT NULL,
  `isexp` int(1) DEFAULT '0' COMMENT '是否已导出 0=未导出 1=已导出',
  `function` int(1) DEFAULT '1' COMMENT '0=旗舰版 1=企业 2=高级版 3=标准版',
  `soft_limit` text COMMENT '软件增配功能',
  `new` int(1) DEFAULT '0' COMMENT '0=旧版本预存序列号 1=新版本生成序列号',
  `act_type` int(11) DEFAULT '0' COMMENT '0=授权时长版本激活  1=选配功能激活  2=授权+选配功能激活 时长一致',
  PRIMARY KEY (`Id`),
  UNIQUE KEY `key_num` (`key_num`),
  KEY `act_type` (`act_type`),
  KEY `channel_id` (`channel_id`),
  KEY `date` (`date`),
  KEY `email` (`email`),
  KEY `puc_type` (`puc_type`)
) ENGINE=InnoDB AUTO_INCREMENT=46971 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='序列号表';

#
# Structure for table "key_list_status"
#

CREATE TABLE `key_list_status` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `key_num` varchar(255) DEFAULT NULL COMMENT '序列号',
  `status` int(1) DEFAULT '1' COMMENT '1=可用 0=不可用',
  `status_reson` text COMMENT '状态原因',
  `inside_reson` text COMMENT '内部的状态原因',
  `date` datetime DEFAULT NULL COMMENT '状态设置日期',
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8mb4 COMMENT='保存禁用的序列号';

#
# Structure for table "log"
#

CREATE TABLE `log` (
  `log_id` int(11) NOT NULL AUTO_INCREMENT,
  `text` varchar(500) DEFAULT NULL,
  `add_date` datetime DEFAULT NULL,
  `uid` int(11) DEFAULT NULL,
  `sumbit` text,
  `file` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`log_id`)
) ENGINE=InnoDB AUTO_INCREMENT=33452 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='服务端日志';

#
# Structure for table "server_debug_log"
#

CREATE TABLE `server_debug_log` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  `username` varchar(15) DEFAULT NULL,
  `puc_type` int(1) DEFAULT NULL,
  `value` varchar(1000) DEFAULT NULL,
  `date` datetime DEFAULT NULL,
  `sumbit` text,
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB AUTO_INCREMENT=15553 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='服务端错误日志';

#
# Structure for table "user_auth"
#

CREATE TABLE `user_auth` (
  `Id` int(11) NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`Id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='授权表';

#
# Structure for table "user_bind"
#

CREATE TABLE `user_bind` (
  `user_bind_id` int(11) NOT NULL AUTO_INCREMENT,
  `uid` int(11) DEFAULT NULL,
  `puc_type` int(11) DEFAULT '0' COMMENT '产品代码',
  `mackey` text COMMENT '机器码',
  `limit_change` int(11) DEFAULT '5' COMMENT '限制换帮次数',
  `change_count` int(11) DEFAULT '0' COMMENT '换机次数',
  `bind_date` datetime DEFAULT NULL COMMENT '绑定时间',
  PRIMARY KEY (`user_bind_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;

#
# Structure for table "user_function"
#

CREATE TABLE `user_function` (
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
) ENGINE=InnoDB AUTO_INCREMENT=23944 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='选配功能';

#
# Structure for table "user_login_re"
#

CREATE TABLE `user_login_re` (
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
  KEY `puctype` (`puctype`)
) ENGINE=InnoDB AUTO_INCREMENT=912305 DEFAULT CHARSET=utf8mb4 COMMENT='用户登录记录';

#
# Structure for table "user_manager_re"
#

CREATE TABLE `user_manager_re` (
  `user_manager_re_id` int(11) NOT NULL AUTO_INCREMENT,
  `uid` int(11) DEFAULT NULL,
  `admin_uid` int(11) DEFAULT NULL,
  `text` text,
  `date` datetime DEFAULT NULL,
  PRIMARY KEY (`user_manager_re_id`)
) ENGINE=InnoDB AUTO_INCREMENT=169 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='用户管理记录';
