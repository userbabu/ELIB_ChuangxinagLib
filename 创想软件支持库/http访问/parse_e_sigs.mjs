// 从易语言导出的 e.txt 里机械抽取子程序签名，生成 cxhttp_api.json 的片段。
// 用法：node http访问/parse_e_sigs.mjs <文件.e.txt ...>   （只打印，不改任何源码）
// 输出：{ "类": [ {名,英文,说明,命令:[{名,返,参,impl}]} ], "全局": [ {名,英文,说明,类别,返,参} ] }
import fs from 'node:fs';

const TY = {
  '整数型': 'int', '长整数型': 'long', '文本型': 'text', '字节集': 'bin', '字节型': 'bin',
  '逻辑型': 'bool', '双精度小数型': 'double', '单精度小数型': 'double', '子程序指针': 'subptr',
  '通用型': 'common', '符号表': 'int', '无效值': 'null',
};
// 英文短名：优先保留 ASCII 部分，中文按对照表补
const EN = {
  '网页_访问_Curl': 'WebAccessCurl', '网页_访问_WinHTTP': 'WebAccessWinHttp', '网页_访问_WinInet': 'WebAccessWinInet',
  'HTTP访问_Curl访问类': 'AccessCurl', 'HTTP访问_WinHTTP访问类': 'AccessWinHttp', 'HTTP访问_WinInet访问类': 'AccessWinInet',
  '网页_Cookie合并更新_内部': 'WebCookieMerge', '网页_提取返回头COOKIE': 'WebCookieFromHead',
  '网页_提取头': 'WebPickHeader', '网页_构建协议头': 'WebBuildHeaders', '网页_取系统代理EX': 'WebSystemProxy',
  '判断响应头是否有效': 'JudgeHeadValid', '取响应头代码': 'GetHeadCode', '网页_取返回协议代码': 'WebStatusCode',
  '网页_构建提交表单数据': 'WebBuildForm', '网页_取常见ContentType': 'WebContentType',
  '分析返回头COOKIES': 'AnalyseHeadCookies', '取中间文本': 'GetMiddleText', 'COOKIE合并更新': 'CookieMerge',
  '网页_取Cookie_内部': 'WebCookieInner', '网页_取单条Cookie': 'WebCookieOne',
  '取CURL版本': 'GetCurlVersion', 'CURL_预载入': 'CurlPreload', 'CURL_关闭CURL': 'CurlShutdown',
  'CURL_GetXferinfoAddress': 'CurlXferinfoAddr', '文本_取出现次数': 'TextCountOccur',
  '压缩亚马逊页面数据': 'CompressAmazonPage', '设置每次接收长度': 'SetRecvChunk', '设置COOKIE': 'SetCookie',
  '设置DNS服务器': 'SetDns', '获取当前提交的COOKIE': 'GetSubmitCookie', '获取当前返回的COOKIE': 'GetRetCookie',
  '设置CURL句柄': 'SetCurlHandle', '设置共享的DNS缓存': 'SetShareDns', 'SetOption': 'SetOption',
  'SetOption_int': 'SetOptionInt', '模仿': 'Impersonate', '访问网页_cURL': 'AccessPageCurl',
  '访问网页_WinHTTP': 'AccessPageWinHttp', '访问网页_WinInet': 'AccessPageWinInet', '取上次提交请求头': 'GetLastHeader',
  '通用公开函数': 'WebPublic',
};
const slug = (s) => s.replace(/[^A-Za-z0-9_]/g, '');
const enOf = (n) => EN[n] || (slug(n) ? slug(n) : null);

function stripComment(line) {
  return line.replace(/["“].*?["”]/g, '""').replace(/'.*$/, '').trim();
}
function parseFile(p) {
  const raw = fs.readFileSync(p, 'utf8').replace(/^﻿/, '');
  const lines = raw.split(/\r?\n/);
  const out = { asm: null, subs: [] };
  let cur = null;
  for (let l of lines) {
    const t = l.trim();
    if (t.startsWith('.程序集') && !t.startsWith('.程序集变量')) {
      const parts = t.replace(/^\.程序集\s+/, '').split(',').map((x) => x.trim());
      out.asm = { 名: parts[0], 说明: parts[3] || '' };
      continue;
    }
    if (t.startsWith('.子程序')) {
      const rest = t.replace(/^\.子程序\s+/, '');
      // 名称后面是逗号分隔；说明里也可能有逗号，所以只切前三段
      const m = /^([^,]+),\s*([^,]*),\s*([^,]*),\s*(.*)$/.exec(rest + ',,,,');
      cur = { 名: m[1].trim(), 返: m[2].trim(), 标志: m[3].trim(), 说明: (m[4] || '').trim(), 参: [] };
      out.subs.push(cur);
      continue;
    }
    if (t.startsWith('.参数') && cur) {
      const rest = t.replace(/^\.参数\s+/, '');
      const m = /^([^,]+),\s*([^,]*),\s*([^,]*),\s*(.*)$/.exec(rest + ',,,,');
      cur.参.push({ 名: m[1].trim(), 型: m[2].trim(), 修饰: m[3].trim(), 说明: (m[4] || '').trim() });
      continue;
    }
    if (/^\.(局部变量|返回|如果|判断|计次|循环|数据|结束|注释|子程序变量)/.test(t)) continue;
  }
  return out;
}
function toTok(p) {
  const mod = p.修饰 || '';
  let base = TY[p.型];
  if (!base) { console.error('  [_warn] 未识别类型「' + p.型 + '」于参数 ' + p.名); base = 'int'; }
  if (base === 'null') base = 'int';
  const ary = /数组/.test(mod), ref = /参考/.test(mod), opt = /可空/.test(mod);
  if (ary && ref) return base + '[]ref';
  if (ary) return base + '[]' + (opt ? '?' : '');
  if (ref) return base + '_ref' + (opt ? '?' : '');
  return base + (opt ? '?' : '');
}
function retTok(s) {
  if (!s) return 'null';
  return TY[s] || (console.error('  [warn] 未识别返回类型「' + s + '）'), 'int');
}

const files = process.argv.slice(2);
const res = { 类: [], 全局: [] };
for (const f of files) {
  const r = parseFile(f);
  const name = r.asm ? r.asm.名 : null;
  const isClass = /class\.e\.txt$/.test(f.replace(/\\/g, '/'));
  const pub = r.subs.filter((s) => s.标志 === '公开');
  const items = pub.map((s) => ({
    名: s.名, 返: retTok(s.返),
    参: s.参.map((p) => [p.名, toTok(p)]),
    impl: null,
  }));
  const en = name ? enOf(name) : null;
  if (!en) { console.error('[FAIL] 缺英文短名：' + (name || f)); continue; }
  for (const it of items) {
    it.impl = enOf(it.名) || (slug(it.名) || ('M' + (items.indexOf(it) + 1)));
  }
  if (isClass) res.类.push({ 名: name, 英文: en, 说明: (r.asm.说明 || '').replace(/[",]/g, ''), 命令: items, _源: f });
  else for (const it of items) res.全局.push({ 名: it.名, 英文: enOf(it.名) || slug(it.名), 说明: '', 返: it.返, 参: it.参, _源: f });
}
console.log(JSON.stringify(res, null, 2));
