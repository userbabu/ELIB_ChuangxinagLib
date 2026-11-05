<?php
/**
 * 创想AH会员框架 · 客户端唯一入口
 *
 * 所有授权请求都 POST 到本文件，路由写在信封的 r 字段里。
 * Web 根目录请指向本 public 目录，app/ 与 runtime/ 必须不可被 HTTP 访问。
 */

require __DIR__ . '/../app/bootstrap.php';

$config = cx_boot();
\CxSoft\Api::run($config);
