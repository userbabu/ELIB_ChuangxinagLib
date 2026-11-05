// 逐条核对：cxhttp_api.json 里的参数名/顺序/类型/可空/数组/参考/返回类型，
// 是否与原模块 e.txt 源码逐字一致。差异一律打印出来，供修正用。
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const SRC = 'E:/dev/a我的程序项目/ai研发/创想亚马逊采集软件问题分析/源码txt/创想HTTP访问类/ecode';
const spec = JSON.parse(fs.readFileSync(path.join(HERE, 'cxhttp_api.json'), 'utf8'));

const TY = {
  '整数型': 'int', '长整数型': 'long', '文本型': 'text', '字节集': 'bin', '字节型': 'bin',
  '逻辑型': 'bool', '双精度小数型': 'double', '单精度小数型': 'double', '子程序指针': 'subptr',
  '通用型': 'common', 'curl_easy': 'easy_ref', 'CURL_消息': 'msg_ref', 'CURL_证书信息': 'cert_ref',
};
function tok(型, 修饰) {
  let base = TY[型];
  if (!base) { base = '??' + 型; }
  const ary = /数组/.test(修饰), ref = /参考/.test(修饰), opt = /可空/.test(修饰);
  if (base === 'cert_ref') return 'cert_ref';
  if (base === 'msg_ref') return 'msg_ref';
  if (base === 'easy_ref') return 'easy_ref';
  if (ary && ref) return base + '[]ref';
  if (ary) return base + '[]' + (opt ? '?' : '');
  if (ref) return base + '_ref';
  return base + (opt ? '?' : '');
}
function parseFile(p) {
  const raw = fs.readFileSync(p, 'utf8').replace(/^﻿/, '');
  const out = { asm: null, subs: [] };
  let cur = null;
  for (const l of raw.split(/\r?\n/)) {
    const t = l.trim();
    if (t.startsWith('.程序集') && !t.startsWith('.程序集变量')) {
      const parts = t.replace(/^\.程序集\s+/, '').split(',').map((x) => x.trim());
      out.asm = { 名: parts[0] };
      continue;
    }
    if (t.startsWith('.子程序')) {
      const rest = t.replace(/^\.子程序\s+/, '');
      const m = /^([^,]+),\s*([^,]*),\s*([^,]*),?\s*(.*)$/.exec(rest + ',,,,');
      cur = { 名: m[1].trim(), 返: m[2].trim(), 标志: m[3].trim(), 参: [] };
      out.subs.push(cur);
      continue;
    }
    if (t.startsWith('.参数') && cur) {
      const rest = t.replace(/^\.参数\s+/, '');
      const m = /^([^,]+),\s*([^,]*),\s*([^,]*),?\s*(.*)$/.exec(rest + ',,,,');
      cur.参.push([m[1].trim(), tok(m[2].trim(), m[3].trim())]);
      continue;
    }
  }
  return out;
}
const RET = { '': 'null', '整数型': 'int', '长整数型': 'long', '文本型': 'text', '字节集': 'bin', '逻辑型': 'bool', '双精度小数型': 'double', 'curl_version_info_data': 'common' };

const files = {
  'curl_easy': SRC + '/模块/zyCurl/代码/curl_easy.class.e.txt',
  'curl_http': SRC + '/模块/zyCurl/代码/curl_http.class.e.txt',
  'curl_smtp': SRC + '/模块/zyCurl/代码/curl_smtp.class.e.txt',
  'curl_multi': SRC + '/模块/zyCurl/代码/curl_multi.class.e.txt',
  'URL参数类': SRC + '/代码/cURL类/URL参数类.class.e.txt',
  'HTTP访问_Curl访问类': SRC + '/代码/cURL类/HTTP访问_Curl访问类.class.e.txt',
  'HTTP访问_WinHTTP访问类': SRC + '/代码/WinHTTP类/HTTP访问_WinHTTP访问类.class.e.txt',
  'HTTP访问_WinInet访问类': SRC + '/代码/WinInet类/HTTP访问_WinInet访问类.class.e.txt',
};
const globals = [SRC + '/代码/通用公开函数.static.e.txt', SRC + '/代码/WinHTTP类/WinHTTP专用.static.e.txt',
  SRC + '/代码/cURL类/CURL专用.static.e.txt', SRC + '/代码/程序集1.static.e.txt'];

let bad = 0, checked = 0;
function cmpOne(池条目, 源子程序, 归属) {
  checked++;
  const 源参 = 源子程序.参.map((x) => x.join(':'));
  const 我参 = 池条目['参'].map((x) => x.join(':'));
  const 源返 = RET[源子程序.返] || ('??' + 源子程序.返);
  if (源参.join('|') !== 我参.join('|')) {
    bad++;
    console.log(`[参数不符] ${归属} · ${池条目['名']}`);
    console.log(`   源码: ${源参.join(' , ')}`);
    console.log(`   库内: ${我参.join(' , ')}`);
  }
  if (源返 !== 池条目['返']) {
    bad++;
    console.log(`[返回不符] ${归属} · ${池条目['名']} 源码=${源返} 库内=${池条目['返']}`);
  }
}
for (const cls of spec['类']) {
  const f = files[cls['名']];
  if (!f) { console.log(`[缺源码] ${cls['名']}`); continue; }
  const src = parseFile(f);
  for (const cmd of cls['命令']) {
    const s = src.subs.find((x) => x.名 === cmd['名'] && x.标志 === '公开');
    if (!s) { bad++; console.log(`[源码里没有公开子程序] ${cls['名']} · ${cmd['名']}`); continue; }
    cmpOne(cmd, s, cls['名']);
  }
  // 反查：源码里有、库里漏了的公开子程序
  for (const s of src.subs.filter((x) => x.标志 === '公开')) {
    if (s.名.startsWith('_')) continue;
    if (!cls['命令'].find((c) => c['名'] === s.名)) { bad++; console.log(`[库里漏了] ${cls['名']} · ${s.名}`); }
  }
}
const 池全局 = [];
for (const f of globals) for (const s of parseFile(f).subs.filter((x) => x.标志 === '公开')) 池全局.push(s);
for (const g of spec['全局命令'] || []) {
  const s = 池全局.find((x) => x.名 === g['名']);
  if (!s) { bad++; console.log(`[源码里找不到全局命令] ${g['名']}`); continue; }
  cmpOne(g, s, '全局');
}
console.log(`\n核对 ${checked} 条（类成员 + 全局），不一致 ${bad} 处`);
console.log(`源码里公开全局子程序共 ${池全局.length} 条，库里已登记 ${(spec['全局命令'] || []).length} 条`);
const 未迁 = 池全局.filter((s) => !(spec['全局命令'] || []).find((g) => g.名 === s.名)).map((s) => s.名);
console.log('未迁的全局命令：' + (未迁.join(' / ') || '（无）'));
