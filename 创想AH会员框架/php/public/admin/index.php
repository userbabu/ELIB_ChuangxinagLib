<?php
/**
 * 创想AH会员框架 · 管理后台入口
 *
 * 访问地址： /admin/  （需与客户端接口同一台服务器，读同一份配置与库）
 */

require __DIR__ . '/../../app/bootstrap.php';

$config = cx_boot();
\CxSoft\Admin\Admin::run($config);
