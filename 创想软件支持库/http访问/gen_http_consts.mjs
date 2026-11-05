// 从 third_party/include/curl/*.h 抽取 libcurl 常量，生成易语言支持库常量表片段。
// 用法：node http访问/gen_http_consts.mjs
// 产物：http访问/cxhttp_consts.inc —— 被 lib.cpp 的 g_constants[] 里 #include。
// 展开规则：对象式 #define 递归求值；函数式宏只在「展开后是 name = 整数表达式」时采用；
// 位宽常量（CURLOPT/CURLINFO 的 10000/20000/30000 基址）一律保留，因为它们是值的一部分。
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const libDir = path.resolve(here, '..');
const curlInc = path.join(libDir, 'third_party', 'include', 'curl');

const HEADERS = ['curlver.h', 'curl.h', 'easy.h', 'header.h', 'multi.h',
  'options.h', 'urlapi.h', 'websockets.h'];

// 常量名正则 -> 中文说明。命中第一条即用；不命中的名字直接丢弃（挡住 CURLof/CURLofx 这类噪声）。
const GROUPS = [
  [/^CURLOPT_/, 'libcurl 选项'],
  [/^CURLINFO_/, 'libcurl 信息码'],
  [/^CURLE_/, 'libcurl 错误码'],
  [/^CURLM_/, 'libcurl multi 错误码'],
  [/^CURSH_/, 'libcurl share 错误码'],
  [/^CURLUE_/, 'libcurl URL解析错误码'],
  [/^CURLAUTH_/, 'libcurl 认证方式'],
  [/^CURLPIPE_/, 'libcurl 管道复用'],
  [/^CURLPROXY_/, 'libcurl 代理类型'],
  [/^CURL_HTTP_VERSION/, 'libcurl HTTP版本'],
  [/^CURLSTATE_/, 'libcurl 引擎状态'],
  [/^CURLOT_/, 'libcurl TLS最低版本'],
  [/^CURLSSLVERSION_/, 'libcurl TLS版本'],
  [/^CURLSSLBACKEND_/, 'libcurl TLS后端'],
  [/^CURLSSLOPT_/, 'libcurl SSL选项'],
  [/^CURLVERSION/, 'libcurl 版本代数'],
  [/^CURLFTP(S|METHOD|SSL|MODE|_)?/, 'libcurl FTP'],
  [/^CURLGSSAPI_/, 'libcurl GSSAPI委托'],
  [/^CURLHEADER_/, 'libcurl 自定义头作用域'],
  [/^CURLIOE_/, 'libcurl 流回调动作'],
  [/^CURLIOCMD_/, 'libcurl 流回调命令'],
  [/^CURL_CHUNK_(BGN|END)FUNC_/, 'libcurl 分块回调'],
  [/^CURL_(WRITEFUNC|READFUNC|SEEKFUNC|DEBUGDATA|XFERINFODIR)_/, 'libcurl 回调返回值'],
  [/^CURL_WRITEFUNC_/, 'libcurl 写回调返回值'],
  [/^CURL_READFUNC_/, 'libcurl 读回调返回值'],
  [/^CURL_SEEKFUNC_/, 'libcurl 定位回调返回值'],
  [/^CURL_DEBUG_/, 'libcurl 调试数据类型'],
  [/^CURL_GLOBAL_/, 'libcurl 全局初始化'],
  [/^CURL_MAX_/, 'libcurl 上限'],
  [/^CURL_NETRC_/, 'libcurl netrc级别'],
  [/^CURLPAUSE/, 'libcurl 传输暂停'],
  [/^CURL_IPRESOLVE_/, 'libcurl IP解析策略'],
  [/^CURLMSG_/, 'libcurl multi消息'],
  [/^CURL_SOCKET_/, 'libcurl socket回调'],
  [/^CURL_SOCKOPT_/, 'libcurl socket选项'],
  [/^CURLSTS_/, 'libcurl 续传状态'],
  [/^CURL_TIMECOND_/, 'libcurl 时间条件'],
  [/^CURLUSELIMIT_/, 'libcurl 限流方式'],
  [/^CURL_UPGRADE_/, 'libcurl 协议升级'],
  [/^CURLWS_/, 'libcurl WebSocket帧'],
  [/^CURLU_/, 'libcurl URL句柄选项'],
  [/^CURLMETALINK_/, 'libcurl metalink'],
  [/^CURL_FORMADD_/, 'libcurl 旧表单返回码'],
  [/^CURL_VERSION_/, 'libcurl 特性位'],
  [/^CURL_TLS_(BACKEND|VERSIONS)_/, 'libcurl TLS后端查询'],
  [/^CURLOPT(?!_)$/, 'libcurl 选项基址'],
  [/^CURLINFO(?!_)$/, 'libcurl 信息码基址'],
];

// 名称黑名单：废弃项与内部宏
const SKIP_NAME = /OBSOLETE|_DEPRECATED|CURL_DEPRECATED|CURL_EXTERN|CURL_CALLING|CURL_INLINE|CURL_INCLUDES|CURL_NO_CURL|CURL_STATICLIB|CURL_ISOCPP|CURL_ASCII|CURL_BERKELEY|CURL_PUBKEY|CURL_ALIGN|CURL_FORMAT|CURL_OFF_T|CURL_TYPEOF|CURL_PARAM|CURL_UNUSED|CURL_SETOPT_|CURL_UNSET_|CURL_BEGIN_|CURL_END_|CURL_COMBINE|CURL_CONST|CURL_BAD|CURL_OS\b|SIZEOF_|CURL_AT_LEAST|CURL_IS_|CURL_STRICTER|CURL_INLINE|__|CURL_GNUTLS|CURL_USE_|CURL_HEADER_ENV|CURLOT_V|HEADERH_/;

function groupOf(name) {
  for (const [re, g] of GROUPS) if (re.test(name)) return g;
  return null;
}

function stripComments(s) {
  return s.replace(/\/\*[\s\S]*?\*\//g, ' ').replace(/\/\/[^\n]*/g, ' ');
}

// ---------------- 宏收集 ----------------
const objMacros = new Map();    // NAME -> 表达式文本
const fnMacros = new Map();     // NAME -> {params, body}

function collectMacros(text) {
  // 先把跨行续接（反斜杠）合并；注释必须在求值前剥掉，否则 `#define X (1<<0) /* 说明 */` 取不到值
  const joined = stripComments(text).replace(/\\[ \t]*\r?\n/g, ' ');
  for (const line of joined.split('\n')) {
    // 只有名字后紧跟 '(' 才是函数式宏；`#define CURLAUTH_ANY (A|B)` 的括号属于值。
    const m = /^\s*#\s*define\s+([A-Za-z_]\w*)(\(([^)]*)\))?\s*([\s\S]*)$/.exec(line);
    if (!m) continue;
    const name = m[1];
    const body = (m[4] || '').trim();
    if (m[2] !== undefined) {
      fnMacros.set(name, { params: (m[3] || '').split(',').map(x => x.trim()).filter(Boolean), body });
    } else if (body && !objMacros.has(name)) {
      objMacros.set(name, body);
    }
  }
}

function splitArgs(text) {
  const res = [];
  let depth = 0, cur = '', str = null;
  for (const ch of text) {
    if (str) { cur += ch; if (ch === str) str = null; continue; }
    if (ch === "'" || ch === '"') { str = ch; cur += ch; continue; }
    if (ch === '(') depth++;
    if (ch === ')') depth--;
    if (ch === ',' && depth === 0) { res.push(cur.trim()); cur = ''; continue; }
    cur += ch;
  }
  if (cur.trim()) res.push(cur.trim());
  return res;
}

function expandFn(name, argText) {
  const def = fnMacros.get(name);
  if (!def) return null;
  const args = splitArgs(argText);
  let body = def.body;
  def.params.forEach((p, i) => {
    body = body.replace(new RegExp('\\b' + p + '\\b', 'g'), '(' + (args[i] ?? '') + ')');
  });
  // 属性宏 CURL_DEPRECATED(v, msg) 直接抹掉
  body = body.replace(/\bCURL_DEPRECATED\s*\([^()]*(?:\([^()]*\)[^()]*)*\)/g, ' ');
  // 形参代入后名字会带括号：`(CURLOPT_URL) = ((10000) + (2))` 先还原成裸名
  body = body.replace(/^\(\s*([A-Za-z_]\w*)\s*\)/, '$1');
  return body.trim();
}

// 类型转换整体去掉：((unsigned long)0)、(curl_off_t)2 之类
function stripCasts(expr) {
  return expr.replace(/\(\s*(?:(?:unsigned|signed)\s+)?(?:char|short|int|long|float|double|void|[A-Za-z_]\w*_t)(?:\s*\*)?\s*\)/g, ' ');
}

// ---------------- 表达式求值 ----------------
const TOKEN = /[A-Za-z_]\w*|0[xX][0-9a-fA-F]+[uUlL]*|\d+[uUlL]*|'(?:\\.|[^'\\])'|"(?:\\.|[^"\\])*"|\(|\)|<<|>>|[-+*/%~&|^!<>=,?:.;]|[\s{},]+/g;

function tokenize(src) {
  const out = [];
  TOKEN.lastIndex = 0;
  let m;
  while ((m = TOKEN.exec(src))) {
    const t = m[0];
    if (/^\s/.test(t)) continue;
    out.push(t);
  }
  return out;
}

const BIN_LEVELS = [['*', '/', '%'], ['+', '-'], ['<<', '>>'], ['<', '<=', '>', '>='], ['==', '!='], ['&'], ['^'], ['|']];

function evaluate(expr, expanding) {
  const toks = tokenize(stripCasts(expr));
  let i = 0;
  const peek = () => toks[i];
  const eat = () => toks[i++];

  function primary() {
    const t = eat();
    if (t === undefined) throw err('eof');
    if (t === '(') { const v = top(); expect(')'); return v; }
    if (t === '-' || t === '+' || t === '~' || t === '!') {
      const v = primary();
      return t === '-' ? -v : t === '~' ? ~v : t === '!' ? (v ? 0 : 1) : v;
    }
    if (t[0] === "'") return t.charCodeAt(1) | 0;
    if (t[0] === '"') throw err('string');
    if (/^\d/.test(t)) {
      const s = t.replace(/[uUlL]+$/, '');
      return (s.startsWith('0x') || s.startsWith('0X')) ? parseInt(s, 16) | 0 : parseInt(s, 10) | 0;
    }
    if (/^[A-Za-z_]/.test(t)) {
      if (peek() === '(') {
        // 收集参数文本
        let depth = 0, argText = '';
        eat(); depth = 1;
        while (i < toks.length) {
          const c = eat();
          if (c === '(') { depth++; }
          if (c === ')') { depth--; if (!depth) break; }
          argText += c + ' ';
        }
        return callInto(t, argText);
      }
      return resolveName(t);
    }
    throw err('token ' + t);
  }
  function resolveName(t) {
    const d = objMacros.get(t);
    if (d === undefined) throw err('unknown ' + t);
    if (expanding.has(t)) throw err('cycle ' + t);
    expanding.add(t);
    try { return evaluate(d, expanding); } finally { expanding.delete(t); }
  }
  function callInto(macName, argText) {
    // 函数式宏：只有「名字 = 表达式」形态才取值，例如 CURL_OFF_T_C / CURLOPT_CAST
    const eq = splitTopAssign(argText);
    void eq;
    const body = expandFn(macName, argText);
    if (body === null) throw err('unknown fn ' + macName);
    const m = /^([A-Za-z_]\w*)\s*=\s*([\s\S]+)$/.exec(body);
    if (!m) {
      // 无名展开：整体是表达式
      return evaluate(body, expanding);
    }
    throw err('named-macro ' + m[1]);   // enum 路径处理，不走这里
  }
  function expect(t) { if (eat() !== t) throw err('expect ' + t); }
  function binLevel(l) {
    if (l >= BIN_LEVELS.length) return primary();
    let v = binLevel(l + 1);
    for (;;) {
      const t = peek();
      if (!t || !BIN_LEVELS[l].includes(t)) return v;
      eat();
      const r = binLevel(l + 1);
      switch (t) {
        case '*': v = Math.imul(v, r); break;
        case '/': v = Math.trunc(v / r); break;
        case '%': v = v - Math.trunc(v / r) * r; break;
        case '+': v = (v + r) | 0; break;
        case '-': v = (v - r) | 0; break;
        case '<<': v = (v << r) | 0; break;
        case '>>': v = v >> r; break;
        case '<': v = v < r ? 1 : 0; break;
        case '<=': v = v <= r ? 1 : 0; break;
        case '>': v = v > r ? 1 : 0; break;
        case '>=': v = v >= r ? 1 : 0; break;
        case '==': v = v === r ? 1 : 0; break;
        case '!=': v = v !== r ? 1 : 0; break;
        case '&': v = v & r; break;
        case '^': v = v ^ r; break;
        case '|': v = v | r; break;
      }
    }
  }
  function top() { return binLevel(0); }
  const v = top();
  if (i < toks.length) throw err('trailing ' + toks[i]);
  return v | 0;
}
function err(msg) { return new Error(msg); }

function splitTopAssign(entry) {
  let depth = 0;
  for (let k = 0; k < entry.length; k++) {
    const ch = entry[k];
    if (ch === '(') depth++;
    else if (ch === ')') depth--;
    else if (ch === '=' && depth === 0 && entry[k + 1] !== '=' && entry[k - 1] !== '=' && entry[k - 1] !== '!' && entry[k - 1] !== '<' && entry[k - 1] !== '>') {
      const nm = /^([A-Za-z_]\w*)\s*$/.exec(entry.slice(0, k));
      if (!nm) return null;
      return [nm[1], entry.slice(k + 1).trim()];
    }
  }
  return null;
}

// ---------------- enum ----------------
const consts = new Map();
const order = [];

function add(name, value) {
  if (!/^[A-Za-z_]\w*$/.test(name)) return;
  if (SKIP_NAME.test(name)) return;
  if (!groupOf(name)) return;
  if (consts.has(name)) return;
  consts.set(name, value);
  order.push(name);
}

function parseEnums(text) {
  const re = /\benum\s+(?:[A-Za-z_]\w*\s+)?\{([\s\S]*?)\}\s*(?:[A-Za-z_]\w*\s*)?;/g;
  let m;
  while ((m = re.exec(text))) {
    // 条目上的属性宏 `CURL_DEPRECATED(7.56.0, "")` 要整体抹掉，否则整条识别不出名字、隐式自增全错位
    const body = m[1].replace(/\bCURL_DEPRECATED\s*\([^()]*(?:\([^()]*\)[^()]*)*\)/g, ' ');
    let prev = -1;
    for (let entry of splitArgs(body)) {
      entry = stripComments(entry).trim();
      if (!entry) continue;
      const callForm = /^([A-Za-z_]\w*)\s*\(([\s\S]*)\)\s*$/.exec(entry);
      let name = null, expr = null;
      if (callForm) {
        const body = expandFn(callForm[1], callForm[2]);
        if (body === null) continue;
        const nm = /^([A-Za-z_]\w*)/.exec(body);
        if (!nm) continue;
        name = nm[1];
        const eq = splitTopAssign(body);
        if (eq) expr = eq[1];
      } else {
        const eq = splitTopAssign(entry);
        if (eq) { name = eq[0]; expr = eq[1]; }
        else {
          const only = /^([A-Za-z_]\w*)$/.exec(entry);
          if (only) name = only[1];
        }
      }
      if (!name) continue;
      let v;
      if (expr === null) v = prev + 1;
      else { try { v = evaluate(expr, new Set()); } catch { continue; } }
      add(name, v);
      prev = v;
    }
  }
}

function parseObjectMacros() {
  for (const [name, body] of objMacros) {
    if (!/^CURL/.test(name)) continue;
    if (/[{};]/.test(body)) continue;
    if (/^CURL_DEPRECATED|^CURL_EXTERN/.test(body)) continue;
    if (/"|'[^']*'\s*\)/.test(body)) continue;
    let v;
    try { v = evaluate(body, new Set()); } catch { continue; }
    add(name, v);
  }
}

for (const h of HEADERS) {
  const p = path.join(curlInc, h);
  if (!fs.existsSync(p)) { console.log('[WARN] 缺少头文件', h); continue; }
  const raw = fs.readFileSync(p, 'utf8');
  collectMacros(raw);
}
for (const h of HEADERS) {
  const p = path.join(curlInc, h);
  if (!fs.existsSync(p)) continue;
  parseEnums(stripComments(fs.readFileSync(p, 'utf8')));
}
parseObjectMacros();

// ---------------- 输出 ----------------
const lines = [
  '// 由 http访问/gen_http_consts.mjs 从 third_party\\include\\curl\\*.h 生成，勿手改。',
  '// libcurl 常量 -> 易语言支持库常量；值为 32 位有符号整数（0xFFFFFFFF 记作 -1）。',
];
for (const name of order) {
  lines.push(`    { _T("${name}"), _T("${name}"), _T("${groupOf(name)}"), _CMD_OS(__OS_WIN), CT_NUM, NULL, ${consts.get(name)} },`);
}
const outPath = path.join(here, 'cxhttp_consts.inc');
fs.writeFileSync(outPath, lines.join('\r\n') + '\r\n', 'utf8');

const byGroup = {};
for (const n of order) byGroup[groupOf(n)] = (byGroup[groupOf(n)] || 0) + 1;
console.log('[OK] 常量条数 =', order.length, '->', path.relative(libDir, outPath));
console.log('     分组：', Object.entries(byGroup).sort((a, b) => b[1] - a[1]).map(([k, v]) => `${k}=${v}`).join('  '));

// 自检一：下面每个期望值都是当场从 third_party/include/curl/*.h 读出来的，不是记忆值。
// 覆盖每种定义形态：CURLOPT 宏调用 / CURLINFO 直接式 / 隐式自增 / 对象式 #define / 位移 / 按位或。
const expect = {
  CURLOPT_URL: 10000 + 2,
  CURLOPT_POSTFIELDS: 10000 + 15,
  CURLOPT_HTTPHEADER: 10000 + 23,
  CURLOPT_PIPEWAIT: 237,
  CURLOPT_RESOLVE: 10000 + 203,
  CURLOPT_HEADERFUNCTION: 20000 + 79,
  CURLINFO_STRING: 0x100000, CURLINFO_OFF_T: 0x600000,
  CURLINFO_RESPONSE_CODE: 0x200000 + 2,
  CURLINFO_CONTENT_TYPE: 0x100000 + 18,
  CURLINFO_PRIMARY_PORT: 0x200000 + 40,
  CURLE_OK: 0, CURLE_COULDNT_CONNECT: 7, CURLE_PROXY: 97,
  CURLAUTH_NONE: 0, CURLAUTH_BASIC: 1, CURLAUTH_NTLM: 8, CURLAUTH_DIGEST_IE: 16,
  CURLSSLOPT_NATIVE_CA: 16,
  CURLPAUSE_ALL: 5,
  CURLWS_TEXT: 1, CURLWS_PING: 16,
  CURLPROXY_HTTP: 0, CURL_HTTP_VERSION_2TLS: 4, CURL_VERSION_IPV6: 1,
  CURLPIPE_NOTHING: 0, CURLPIPE_MULTIPLEX: 2,
  CURLUE_OK: 0, CURL_MAX_WRITE_SIZE: 16384,
};
let bad = 0;
for (const [k, v] of Object.entries(expect)) {
  const got = consts.get(k);
  if (got === undefined) { console.log('  [MISS]', k); bad++; continue; }
  if (got !== (v | 0)) { console.log('  [WRONG]', k, '得到', got, '期望', v | 0); bad++; }
}
console.log(bad ? `[ERROR] 权威值自检失败 ${bad} 项` : `[OK] 权威值自检通过：${Object.keys(expect).length} 项吻合`);

// 自检二（--verify）：把全部常量丢给 MSVC 求值，逐条比对，编译器说了算。
const probeDir = path.join(libDir, 'out');
if (!fs.existsSync(probeDir)) fs.mkdirSync(probeDir);
const probe = [
  '/* Generated by gen_http_consts.mjs: let the compiler resolve every curl constant. */',
  '#include <stdio.h>',
  '#include <curl/curl.h>',
  'static void P(const char* n, long long v) { printf("%s=%lld\\n", n, v); }',
  'int main(void) {',
];
for (const name of order) probe.push(`    P("${name}", (long long)(int)((${name}) + 0));`);
probe.push('    return 0;', '}', '');
fs.writeFileSync(path.join(probeDir, 'consts_probe.c'), probe.join('\r\n') + '\r\n', 'utf8');

if (process.argv.includes('--verify')) {
  const r = spawnSync('cmd', ['/c', path.join(here, 'verify_consts.bat')], { stdio: 'inherit' });
  if (r.status !== 0) { console.log('[ERROR] 编译探针失败'); process.exitCode = 1; }
  else {
    const txt = fs.readFileSync(path.join(probeDir, 'consts_probe.txt'), 'utf8').split(/\r?\n/);
    const real = new Map();
    for (const line of txt) {
      const m = /^([A-Za-z_]\w*)=(-?\d+)$/.exec(line.trim());
      if (m) real.set(m[1], parseInt(m[2], 10) | 0);
    }
    let diff = 0, missing = 0;
    for (const name of order) {
      if (!real.has(name)) { if (missing < 20) console.log('  [PROBE-MISS]', name); missing++; continue; }
      if (real.get(name) !== consts.get(name)) {
        if (diff < 30) console.log('  [DIFF]', name, '解析得', consts.get(name), '编译器给', real.get(name));
        diff++;
      }
    }
    console.log(diff || missing
      ? `[ERROR] 探针比对：差异 ${diff} 条，未取到 ${missing} 条（共 ${order.length} 条）`
      : `[OK] 探针比对通过：${order.length} 条常量与编译器取值全部一致`);
    process.exitCode = (diff || missing) ? 1 : 0;
  }
}

