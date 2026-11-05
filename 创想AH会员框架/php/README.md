# 创想AH会员框架 · 全新 PHP 后端

新版授权服务的**唯一后端**：统一 JSON 协议、单一入口、旧库表结构直接复用。
与旧版 PHP 的关系：**并行部署**。旧客户端继续访问旧接口目录，新客户端（易语言「创想AH会员框架」支持库）访问本目录，两边读写同一批表，因此**线上数据无需迁移**。

---

## 1. 目录结构

```
php/
  app/
    bootstrap.php        类加载 + 时区 + 配置 + 日志 + 数据库
    config.php           出厂默认配置（线上覆盖用 config.local.php 或 cx_setting 表）
    Core/                协议、数据库、验证码、机器码解析、限流、错误码
    Service/             业务：授权、账号、序列号、换机、渠道、站点
    Admin/               后台页面
  public/                ← Web 根目录指向这里
    api.php              客户端唯一入口
    admin/index.php      管理后台入口
  sql/
    schema.sql           建表（旧表原样 + 4 张新表）
    seed.sql             初始配置、官方渠道、后台账号
    _旧结构原始备份.sql   线上导出的原始结构，留作比对
  tests/                 自测脚本（见第 6 节）
  runtime/               日志与验证码缓存，必须不可被 HTTP 访问
```

## 2. 部署要点

1. Web 根指向 `public/`，`app/` 与 `runtime/` 不可访问。
2. 先执行 `sql/schema.sql`，再执行 `sql/seed.sql`（可重复执行，不覆盖已改的配置值）。
3. `app/config.php` 的 `db` 与 `app_secret` 是必改项；建议新建 `app/config.local.php` 返回覆盖数组，避免升级代码时被冲掉。
4. `app_secret` 必须与支持库初始化时传入的产品密钥**完全一致**，否则全部请求返回 `1002`。
5. 生产环境走 HTTPS。明文调试通道需同时满足 `config.php` 的 `debug_plain=true` 与环境变量 `CXAH_DEV=1`，线上不要设这个环境变量。
6. 可调项都在 `cx_setting` 表（试用天数、换机默认次数、是否要求序列号换机、失败锁定阈值、验证码开关、旧序列号兜底天数、`is_test` 列的试用取值），后台「接口配置」页可直接改，保存即生效。

`is_test` 列按线上注释口径处理：**2=试用版，1=正式版**。如实际数据相反，只需把 `trial_is_test_value` / `paid_is_test_value` 两项对调，无需改代码。

## 3. 协议

一次请求 = 一个 JSON 信封；一次响应 = 一个 JSON 信封。没有 Cookie、没有会话、没有第二种格式。

**请求**（`POST /api.php`，`Content-Type: application/json`）

```json
{ "r": "auth.login", "v": 1, "t": 1792331963000, "d": "<密文>", "s": "<签名>" }
```

- `r` 路由名；`v` 协议版本，固定 1；`t` 客户端**毫秒**时间戳。
- `d` = `base64( 16字节随机IV + AES-256-CBC(业务JSON, PKCS7填充) )`，业务 JSON 是 UTF-8。
- `s` = `hex( HMAC-SHA256( "r|t|d", 签名密钥 ) )`。

**密钥派生**（客户端与服务端各自独立计算，`app_secret` 为共享密钥）

```
encKey(req)  = SHA256( app_secret | "ENC-REQ"  | r | t )   // 请求加密，32 字节原始值
sigKey(req)  = SHA256( app_secret | "SIG-REQ"  | r | t )
encKey(resp) = SHA256( app_secret | "ENC-RESP" | r | t )   // 响应加密，沿用同一个 t
sigKey(resp) = SHA256( app_secret | "SIG-RESP" | r | t )
```

**响应**

```json
{ "ok": true, "code": "0", "msg": "", "t": 1792331963400, "v": 1, "d": "<密文>", "s": "<签名>" }
```

密文解开后是 `{"data":{...},"code":"0","msg":"..."}`。`code`/`msg` 在信封外层也有一份，便于不解密就能判断成败；`msg` 已经是**给用户看的完整中文文案**，客户端直接显示，不要再拼技术细节。

**明文调试格式**（仅 `CXAH_DEV=1`）：`{"r":"...","v":1,"t":毫秒,"data":{...}}`，响应里直接给 `data`。

## 4. 路由一览

业务参数都放在加密后的业务 JSON 里；下表 `公共字段` = `puc_type`(产品类型,必填) `channel_id` `version`(程序版本) `app_md5` `system`(Windows版本代号) `mac_key`(机器码,base64串)。

| 路由 | 用途 | 主要参数 | 成功返回 data |
| --- | --- | --- | --- |
| `common.time` | 校准本机时间 | — | `timestamp,datetime,date,utc_offset` |
| `common.captcha` | 取图形验证码 | — | `id,image_base64,expire_in,enabled` |
| `channel.oem` | 载入渠道/品牌信息 | `key`(代理KEY) | `channel_id,brand_name,soft_name,qq,qq2,wechat,home,shop_home,...` |
| `account.register` | 注册账号并绑定机器码 | 公共字段 + `email,qq,referee,home_username,learn,channel,remarks,captcha_id,captcha_code` | `grant,auth_status:"none"` |
| `account.trial` | 开通免费试用 | 公共字段 + `email` | `grant,auth_status,due_date` |
| `auth.login` | 登录（写登录记录） | 公共字段 + `email` | `grant,auth_status` |
| `auth.refresh` | 只刷新授权，不记登录 | 公共字段 + `email` | `grant,auth_status` |
| `auth.event` | 上报退出/心跳 | 公共字段 + `email,event(quit/logout/heartbeat)` | `event` |
| `key.activate` | 输入序列号开通/续费 | 公共字段 + `email,key_num,captcha_id,captcha_code` | `grant,auth_status,due_date` |
| `change.request` | 换机探测（要不要验证、还剩几次） | 公共字段 + `email` | `need_change,request_id,change_left,need_key,need_captcha` |
| `change.confirm` | 换机验证并直接登录 | 公共字段 + `email,key_num,captcha_id,captcha_code,request_id` | `grant,auth_status,changed,old_mac,change_left` |

`grant`（授权视图）字段：`uid,email,qq,bind_mac,mac_key,state,state_reason,due_date,reg_date,last_login,login_num,is_trial,is_expired,authorized,function,function_name,soft_limit,order_number,puc_type,channel_id,home_username,remarks,limit_change,change_time,change_left,server_time,functions[]`。
`auth_status`：`ok / trial / expired / frozen / none`。字段只增不改。

## 5. 换机流程（本次主要改动）

```
客户端                    服务端
  │ auth.login(email,新机器码)
  ├───────────────────────▶ 机器码与绑定不符
  │                         ├ 剩余次数<=0 → code=4002 直接报错，界面提示联系客服
  │                         └ 剩余次数>0  → code=4001 + need_change/change_left
  │ common.captcha
  ├───────────────────────▶ 返回 id + base64 图片
  │ 界面提示「请输入本账号激活过的序列号」
  │ change.confirm(email,新机器码,key_num,captcha)
  ├───────────────────────▶ 校验序列号确属该账号(key_list.email 命中)
  │                          mac_key=新机, change_time+1, 写 amazonhelper_change_re,
  │                          完成 cx_change_request, 直接登录
  ◀───────────────────────  code=0 + grant（无需再调一次登录）
```

要点：
- 只有真的换了电脑才消耗配额；`change_left = limit_change - change_time`，服务端每次登录都回传，界面可提前显示「还剩 N 次」。
- 验证用的是**本账号激活过的序列号**（`key_list.email` 必须等于登录邮箱），别人的序列号一律 `4003`。
- 同一目标（邮箱/序列号）+ 同一 IP 连续失败到达阈值即锁定，锁定期内即使填对也返回 `4004`，防猜测。
- 不需要序列号也能换机：后台把 `change_need_key` 置 0，此时仅校验验证码。

## 6. 自测

```bat
:: 1. 建库（本机 docker MySQL 已验证 MySQL 9 与线上 5.7 语句兼容）
docker exec -i mysql-server mysql -uroot -proot cxang_key_platform < php\sql\schema.sql
docker exec -i mysql-server mysql -uroot -proot cxang_key_platform < php\sql\seed.sql

:: 2. 全链路自测（注册→试用→登录→换机两个分支→激活→冻结→事件→防爆破）
php tests\run.php          :: 52 项断言，用例开头会自行把验证码开关打开
php tests\run.php --plain  :: 额外验证明文通道

:: 3. HTTP 真实链路（另开窗口起服务）
set CXAH_DEV=1 && php -S 127.0.0.1:8317 -t php\public
php tests\http_check.php       :: 加密信封走 HTTP（8 项）
php tests\admin_check.php      :: 后台逐页冒烟 + 生成序列号 + 改资料（18 项）

:: 4. 支持库离线宿主（在 libs\创想AH会员框架 目录下执行：命令宿主 64 项、界面宿主 34 项）
::    两个宿主各自单独 host_seed，别共用邮箱与序列号，否则第二轮起全是「账号已存在」的假失败
php tests\host_seed.php 12345        :: 造换机/激活要用的序列号，并临时免验证码
cmd /c build_host.bat http://127.0.0.1:8317/api.php <app_secret> <邮箱> <已用序列号> <新序列号> <次数用完邮箱>
php tests\host_unseed.php            :: 恢复验证码开关并清理宿主测试数据
```

后台初始账号 `admin / admin888`（`seed.sql` 写入，口令为旧库口径 `md5(密码+用户名)`），上线请立即修改。

## 7. 已定规则（后续别改）

- 错误码只在 `app/Core/Code.php` 末尾追加，旧编号含义不变。
- `grant` 与响应字段只增不改；客户端按「有就用、没有就忽略」处理。
- 到期顺延：`max(现在, 现到期) + 序列号天数`；试用只有一次，判据是「有过到期时间或激活过序列号」。
- 登录记录只在真正拿到授权时写 `user_login_re`；退出、注册、试用、激活、换机都写 `event_re`，换机另写 `amazonhelper_change_re` + `cx_change_request`。
- 机器码一律由易语言客户端生成后传入，服务端只做结构与校验位解析（`app/Core/MachineCode.php`），绝不生成。
