// 从原模块 ecode\常量.e.txt 机械抽取「模块自有常量」（HTTP状态_/HTTP配置_/INTERNET_/WINHTTP_ 等非 curl 项），
// 生成支持库常量表片段 cxmod_consts.inc。
// 用法：node http访问/gen_mod_consts.mjs [常量.e.txt 路径]
// 口径：值一律按 32 位有符号整数落库（2147483648 记作 -2147483648），文本常量走 CT_TEXT。
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const SRC = process.argv[2] || path.join(HERE,
  '../../../创想亚马逊采集软件问题分析/源码txt/创想HTTP访问类/ecode/常量.e.txt');
const OUT = path.join(HERE, 'cxmod_consts.inc');

const raw = fs.readFileSync(SRC, 'utf8').replace(/^﻿/, '');
const lines = raw.split(/\r?\n/);

// 中文前缀 -> ASCII 英文前缀（英文名只用于库内部标识，中文名才是用户敲的）
const PREFIX = [['HTTP状态_', 'HTTP_STATE_'], ['HTTP配置_', 'HTTP_CONFIG_']];
const WORD = {
  '建立会话': 'SESSION', '建立连接': 'CONNECT', '建立请求': 'REQUEST', '正在发送请求': 'SENDING',
  '等待响应': 'WAIT_RESP', '正在请求头信息': 'HEADINFO', '获得数长度': 'TOTAL_LEN', '开始获取数据': 'DATA_BEGIN',
  '获得已接收数据长度': 'RECV_LEN', '获取已接收数据': 'RECV_DATA', '处理完成': 'DONE',
  '错误_HTTP接口不可用': 'ERR_API', '错误_接收不到数据': 'ERR_NO_DATA', '错误_等待响应超时': 'ERR_TIMEOUT',
  '错误_发送请求失败': 'ERR_SEND', '错误_建立请求失败': 'ERR_REQUEST', '错误_建立连接失败': 'ERR_CONNECT',
  '错误_建立会话失败': 'ERR_SESSION', '错误_发送请求失败重试次数': 'ERR_RETRY_TIMES',
  '默认浏览器UA': 'DEFAULT_UA', '默认每次接收数据长度': 'DEFAULT_CHUNK',
};
function enOf(name) {
  for (const [cn, en] of PREFIX) if (name.startsWith(cn)) {
    const tail = name.slice(cn.length);
    return en + (WORD[tail] || tail.replace(/[^A-Za-z0-9_]/g, '_'));
  }
  return name;                       // INTERNET_* / WINHTTP_* 本身就是 ASCII
}

const rows = [];
const seen = new Set();
// 8.22 那张英文表的取值集合，用来核对中文别名指到的选项号在本库版本里还存在
const curl822 = new Set();
{
  const en = fs.readFileSync(path.join(HERE, 'cxhttp_consts.inc'), 'utf8');
  for (const m of en.matchAll(/CT_NUM, NULL, (-?\d+) \},/g)) curl822.add(m[1]);
}
let nAlias = 0, nAliasUnknown = [];
let nSkipDeco = 0, nDup = 0, nText = 0, nCurlSkip = 0;
function 读文件(行集, 只取中文别名) {
for (const l of 行集) {
  const t = l.trim();
  if (!t.startsWith('.常量')) continue;
  const rest = t.replace(/^\.常量\s+/, '');
  // 名称, "值", 公开/作用域, 说明 —— 值里可能有逗号与引号，手工切第一段
  const ci = rest.indexOf(',');
  const name = (ci < 0 ? rest : rest.slice(0, ci)).trim();
  if (!name) continue;
  if (!/^[A-Za-z_\u4e00-\u9fa5][A-Za-z0-9_\u4e00-\u9fa5]*$/.test(name)) { nSkipDeco++; continue; }
  if (ci < 0) { nSkipDeco++; continue; }               // 只有名字的分隔行
  let tail = rest.slice(ci + 1).trim();
  // 取值：先剥最外层 ASCII 引号，再剥里面的中文引号
  let val = '', num = false;
  const q = tail.match(/^"([^"]*)"(.*)$/);
  if (q) { val = q[1]; tail = q[2].replace(/^,/, '').trim(); }
  else {
    const q2 = tail.match(/^[“"]([^”"]*)[”](.*)$/);
    if (!q2) { nSkipDeco++; continue; }
    val = q2[1]; tail = q2[2].replace(/^,/, '').trim();
  }
  val = val.replace(/^[“”]+|^[”]/, '').replace(/[“”]+$/, '').trim();
  if (/^-?\d+$/.test(val)) num = true;
  let explain = '';
  if (tail) {
    // 作用域（公开/全局/项目/静态）不是说明，跳掉
    explain = tail.split(',').map((x) => x.trim())
      .filter((x, i) => x && !(i === 0 && /^(公开|全局|项目|静态)$/.test(x)))
      .join(', ');
  }
  let dt, nVal = 0, txt = '';
  if (num) {
    dt = 'CT_NUM';
    nVal = (parseInt(val.trim(), 10) | 0);            // 32 位有符号截断
    if (!Number.isFinite(parseInt(val.trim(), 10))) { nSkipDeco++; continue; }
  } else { dt = 'CT_TEXT'; txt = val; nText++; }
  if (seen.has(name)) { nDup++; continue; }
  // 原模块把整套 curl 常量也抄进了自己的常量文件，值来自老版头文件；
  // 库里那 814 条是从本库自带的 curl-impersonate 8.22 头机械展开并经编译器核对的，
  // 两边对不上时以 8.22 为准，所以这里一律不再生成 CURL* 前缀的项。
  if (/^CURL/.test(name)) {
    // 英文项以 8.22 那张表为准；只有「中文别名的 curl 常量」才收进来（用户代码就写这些名字）
    if (!只取中文别名 || !/[\u4e00-\u9fa5]/.test(name)) { nCurlSkip++; continue; }
    if (num && !curl822.has(String(nVal))) nAliasUnknown.push(name + '=' + nVal);
    nAlias++;
  }
  seen.add(name);
  const esc = (s) => (s || '').replace(/\\/g, '\\\\').replace(/"/g, '\\"').replace(/[\r\n]+/g, ' ');
  rows.push(`    { _T("${esc(name)}"), _T("${enOf(name)}"), _T("${esc(explain || name)}"), _CMD_OS(__OS_WIN), ${dt}, ${num ? 'NULL' : `_T("${esc(txt)}")`}, ${nVal} },`);
}
}
读文件(lines, false);
if (process.argv[3]) 读文件(fs.readFileSync(process.argv[3], 'utf8').replace(/^\uFEFF/, '').split(/\r?\n/), true);
fs.writeFileSync(OUT, `// 由 http访问/gen_mod_consts.mjs 从原模块 ecode\\常量.e.txt 生成，勿手改。
// 原模块自有常量（HTTP状态_/HTTP配置_/INTERNET_/WINHTTP_）；值为 32 位有符号整数，文本常量用 CT_TEXT。
${rows.join('\r\n')}\r\n`, 'utf8');
console.log('[OK] 常量 %d 条（文本 %d，数值 %d），丢弃装饰行/无值 %d，重名 %d，跳过英文 curl 项 %d，其中中文别名 %d 条',
  rows.length, nText, rows.length - nText, nSkipDeco, nDup, nCurlSkip, nAlias);
if (nAliasUnknown.length) console.log('[注意] %d 条中文别名的选项号不在 8.22 表里：' + nAliasUnknown.slice(0, 12).join(', '), nAliasUnknown.length);
