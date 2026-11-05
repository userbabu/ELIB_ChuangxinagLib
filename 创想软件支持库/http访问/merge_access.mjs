// 把 parse_e_sigs.mjs 抽出来的「三个访问类 + 公开全局命令」并进了 cxhttp_api.json。
// 只补 JSON，不动 lib.cpp；表与桩由 gen_http_table.mjs 统一生成。
// 用法：node http访问/merge_access.mjs
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const spec = JSON.parse(fs.readFileSync(path.join(HERE, 'cxhttp_api.json'), 'utf8'));
const acc = JSON.parse(fs.readFileSync(path.join(HERE, 'out_all.json'), 'utf8'));
const pub = JSON.parse(fs.readFileSync(path.join(HERE, 'out_pub.json'), 'utf8'));

const KIND = { AccessCurl: 5, AccessWinHttp: 6, AccessWinInet: 7 };
const TYPEMAC = { AccessCurl: 'CX_TYPE_ACC_CURL', AccessWinHttp: 'CX_TYPE_ACC_WINHTTP', AccessWinInet: 'CX_TYPE_ACC_WININET' };
const SEQ = { AccessCurl: 17, AccessWinHttp: 18, AccessWinInet: 19 };
const DESC = {
  AccessCurl: '原模块同名类：libcurl-impersonate 路线的一次性网页访问。',
  AccessWinHttp: '原模块同名类：WinHTTP 路线的一次性网页访问。',
  AccessWinInet: '原模块同名类：WinInet 路线的一次性网页访问。',
};

// 类：按 英文 从 out_all.json 里取
for (const c of acc['类']) {
  if (!KIND[c['英文']]) continue;
  const exist = spec['类'].find((x) => x['名'] === c['名']);
  if (exist) continue;
  spec['类'].push({
    名: c['名'], 英文: c['英文'], 说明: DESC[c['英文']],
    对象: KIND[c['英文']], 类型宏: TYPEMAC[c['英文']], 序号: SEQ[c['英文']],
    命令: c['命令'],
  });
}

// 全局命令：白名单（英文 -> 实现归属），不在此列的一律不迁
const GLOBALS = [
  // 三条访问命令：curl 路线在 cxhttp_access_curl.inc，两条 Win 路线在 cxhttp_access_win.inc
  ['WebAccessCurl', '网页_访问_Curl'], ['WebAccessWinHttp', '网页_访问_WinHTTP'], ['WebAccessWinInet', '网页_访问_WinInet'],
  // 其余是工具命令，实现在 cxhttp_util.inc
  ['WebCookieMerge', '网页_Cookie合并更新_内部'], ['WebCookieFromHead', '网页_提取返回头COOKIE'],
  ['WebPickHeader', '网页_提取头'], ['WebBuildHeaders', '网页_构建协议头'], ['WebSystemProxy', '网页_取系统代理EX'],
  ['JudgeHeadValid', '判断响应头是否有效'], ['GetHeadCode', '取响应头代码'], ['WebStatusCode', '网页_取返回协议代码'],
  ['WebBuildForm', '网页_构建提交表单数据'], ['WebContentType', '网页_取常见ContentType'],
  ['AnalyseHeadCookies', '分析返回头COOKIES'], ['GetMiddleText', '取中间文本'], ['CookieMerge', 'COOKIE合并更新'],
  ['WebCookieInner', '网页_取Cookie_内部'], ['WebCookieOne', '网页_取单条Cookie'],
  ['TextCountOccur', '文本_取出现次数'], ['CompressAmazonPage', '压缩亚马逊页面数据'],
];
const pool = [...pub['全局'], ...acc['全局']];
spec['全局命令'] = spec['全局命令'] || [];
const have = new Set(spec['全局命令'].map((g) => g['英文']));
for (const [en, cn] of GLOBALS) {
  if (have.has(en)) continue;
  const it = pool.find((g) => g['英文'] === en || (g['名'] === cn && g['英文'] === en));
  if (!it) { console.log('[FAIL] 签名池里找不到全局命令 ' + cn + ' / ' + en); process.exit(1); }
  spec['全局命令'].push({ 名: it['名'], 英文: it['英文'], 说明: '', 类别: 6, 返: it['返'], 参: it['参'] });
  have.add(en);
}
fs.writeFileSync(path.join(HERE, 'cxhttp_api.json'), JSON.stringify(spec, null, 2), 'utf8');
console.log('[OK] 类 %d 个 / 全局命令 %d 条', spec['类'].length, spec['全局命令'].length);
for (const c of spec['类']) console.log('   类 %s (%s) kind=%s 序号=%s 成员 %d', c['名'], c['英文'], c['对象'], c['序号'], c['命令'].length);
