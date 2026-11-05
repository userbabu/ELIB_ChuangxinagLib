// 新成员命令追加到总表末尾，三个类的原有命令序号保持不变。
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const here = path.dirname(fileURLToPath(import.meta.url));
const base = 613;
const args = [], cmds = [], funcs = [], names = [];
let indexes = fs.readFileSync(path.join(here, 'cxhttp_cmdindex.inc'), 'utf8');
const header = '// 由 gen_http_cancel_table.mjs 生成，勿手改。\n';
for (const [tag, title] of [['AccessCurl', 'HTTP访问_Curl访问类'], ['AccessWinHttp', 'HTTP访问_WinHTTP访问类'], ['AccessWinInet', 'HTTP访问_WinInet访问类']]) {
  cmds.push(`// CX_APPEND_OWNER ${title}`);
  const extra = [];
  for (const [suffix, cn, dt, flag, note] of [
    ['BindStop', '绑定停止变量', 'SDT_INT', 'AS_RECEIVE_VAR', '参考传入四字节对齐的整数；跨线程原子置1取消，变量需存活到解绑且所有请求返回。'],
    ['BindStopPointer', '绑定停止变量指针', 'SDT_INT', '0', '传入四字节对齐的整数变量地址；0解绑。变量需存活到解绑且所有请求返回。'],
    ['GetLastError', '取上次请求错误码', null, null, '0成功；1223调用方取消；其余为curl或Windows错误码。'],
  ]) {
    const fn = `${tag}_${suffix}`;
    const argName = `g_${fn}Args`;
    if (dt) args.push(`static ARG_INFO ${argName}[] = { { _T("停止变量${suffix === 'BindStopPointer' ? '指针' : ''}"), _T("${note}"), 0, 0, ${dt}, 0, ${flag} } };`);
    cmds.push(`    CX_OBJ("${cn}", "${fn}", "${note}", _CMD_OS(__OS_WIN), ${dt ? 'SDT_BOOL' : 'SDT_INT'}, ${dt ? 1 : 0}, ${dt ? argName : 'NULL'}),`);
    extra.push(base + funcs.length);
    funcs.push(`    ${fn},`);
    names.push(`    _T("${fn}"),`);
  }
  const array = `g_${tag}CmdIndex`;
  const re = new RegExp(`INT ${array}\\[\\] = \\{([^}]+)\\};`);
  const old = indexes.match(re);
  if (!old) throw new Error(`Missing ${array}`);
  const cells = [...old[1].matchAll(/\d+/g)].map(x => Number(x[0])).filter(x => x < base).concat(extra);
  indexes = indexes.replace(re, `INT ${array}[] = { ${cells.join(', ')} };`)
    .replace(new RegExp(`sizeof\\(${array}\\) / sizeof\\(INT\\) == \\d+`), `sizeof(${array}) / sizeof(INT) == ${cells.length}`);
}
cmds.push('// CX_APPEND_OWNER -');
args.push('static ARG_INFO g_HttpExchangeStopArgs[] = {\n    { _T("停止变量"), _T("参考传入四字节对齐且存活的整数变量。"), 0, 0, SDT_INT, 0, AS_RECEIVE_VAR },\n    { _T("值"), _T("1取消；请求全部返回后可置0复用。返回原值。"), 0, 0, SDT_INT, 0, 0 },\n};');
cmds.push('    CX_CMD("HTTP_原子置停止变量", "HttpExchangeStop", "原子设置停止变量，返回原值；请求运行期间不得将已置1的变量重新置0。", 6, SDT_INT, 2, g_HttpExchangeStopArgs),');
funcs.push('    G_HttpExchangeStop,');
names.push('    _T("G_HttpExchangeStop"),');
for (const [name, lines] of [['args', args], ['cmds', cmds], ['funcs', funcs], ['names', names]])
  fs.writeFileSync(path.join(here, `cxhttp_cancel_${name}.inc`), header + lines.join('\n') + '\n');
fs.writeFileSync(path.join(here, 'cxhttp_cmdindex.inc'), indexes);
console.log(`[OK] HTTP cancellation: ${funcs.length} appended commands, ${base}..${base + funcs.length - 1}`);

await import('../线程锁/gen_cancel_table.mjs');
