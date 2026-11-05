<?php
/**
 * 后台页面上下文：当前管理员、IP 与常用取参。
 */

namespace CxSoft\Admin;

class Page
{
    /** @var array|null */
    public $admin;
    /** @var bool */
    public $authed;

    public function __construct($admin, $authed)
    {
        $this->admin  = $admin;
        $this->authed = $authed;
    }

    public function name()
    {
        return $this->admin ? $this->admin['username'] : '未登录';
    }

    public function uid()
    {
        return $this->admin ? (int) $this->admin['uid'] : 0;
    }

    public function ip()
    {
        return Admin::ip();
    }

    public function now()
    {
        return date('Y-m-d H:i:s');
    }

    public function q($key, $default = '')
    {
        return Admin::get($key, $default);
    }

    public function p($key, $default = '')
    {
        return Admin::post($key, $default);
    }
}
