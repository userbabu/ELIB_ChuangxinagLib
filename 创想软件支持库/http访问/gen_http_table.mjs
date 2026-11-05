// 从 cxhttp_api.json 生成 HTTP访问 族的表与桩。
// 用法：node http访问/gen_http_table.mjs
// 产物（落在 http访问\，全部勿手改）：
//   cxhttp_ops.h     成员命令操作码枚举（CxHttpInvoke 用；同类命令可共享操作码）
//   cxhttp_args.inc  参数表 ARG_INFO 数组
//   cxhttp_cmds.inc  命令表行（每类先 CX_LIFECYCLE 再成员 CX_OBJ）
//   cxhttp_impl.inc  实现桩（构造/复制/析构 + 每条成员命令一个转调函数）
//   cxhttp_funcs.inc 实现表条目（与命令表同序）
//   cxhttp_names.inc 函数名表条目（静态编译按名挑符号）
//   cxhttp_typereg.inc 类成员下标表 + 数据类型登记行（lib.cpp 按序 include）
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const spec = JSON.parse(fs.readFileSync(path.join(here, 'cxhttp_api.json'), 'utf8'));
const HDR = '// 由 http访问/gen_http_table.mjs 从 cxhttp_api.json 生成，勿手改。';

const DT = {
  int: 'SDT_INT', long: 'SDT_INT64', text: 'SDT_TEXT', bin: 'SDT_BIN', double: 'SDT_DOUBLE',
  bool: 'SDT_BOOL', subptr: 'SDT_SUB_PTR', common: 'SDT_COMMON', null: '_SDT_NULL',
};
// 参数类型 token -> [SDT, 附加标志]
function paramTy(ty) {
  const opt = ty.endsWith('?');
  const base = opt ? ty.slice(0, -1) : ty;
  const EMPTY = 'AS_DEFAULT_VALUE_IS_EMPTY';   // 可空 = 只置 IS_EMPTY（与 AS_HAS_DEFAULT_VALUE 互斥）
  const map = {
    'text[]': ['SDT_TEXT', 'AS_RECEIVE_ARRAY_DATA'],
    'int[]': ['SDT_INT', 'AS_RECEIVE_ARRAY_DATA'],
    'text[]ref': ['SDT_TEXT', 'AS_RECEIVE_VAR_ARRAY'],
    'cert_ref': ['MAKELONG(CX_TYPE_CERT_INFO, 0)', 'AS_RECEIVE_VAR'],
    'msg_ref': ['MAKELONG(CX_TYPE_CURL_MSG, 0)', 'AS_RECEIVE_VAR'],
    'easy_ref': ['MAKELONG(CX_TYPE_CURL_EASY, 0)', 'AS_RECEIVE_VAR'],
    'cancel_ref': ['MAKELONG(CX_TYPE_CANCEL, 0)', 'AS_RECEIVE_VAR'],
    'formdata_ary': ['MAKELONG(CX_TYPE_FORM_DATA, 0)', 'AS_RECEIVE_ARRAY_DATA'],
    'int_ref': ['SDT_INT', 'AS_RECEIVE_VAR'],
    'long_ref': ['SDT_INT64', 'AS_RECEIVE_VAR'],
    'bool_ref': ['SDT_BOOL', 'AS_RECEIVE_VAR'],
    'double_ref': ['SDT_DOUBLE', 'AS_RECEIVE_VAR'],
    'text_ref': ['SDT_TEXT', 'AS_RECEIVE_VAR'],
    'bin_ref': ['SDT_BIN', 'AS_RECEIVE_VAR'],
  };
  if (base === 'common') return ['_SDT_ALL', '0'];   // 通用型：SDK 只给了 _SDT_ALL 这个「任何类型」码
  // 参考 + 可空（原模块的 返回头 / 失败日志 这类出参就是两个标记同时写）
  if (opt && /_ref$/.test(base) && map[base]) return [map[base][0], 'AS_RECEIVE_VAR | AS_DEFAULT_VALUE_IS_EMPTY'];
  if (map[base]) {
    const [dt, fl] = map[base];
    // 数组型允许「可空」（原模块 置邮件内容 的 附件列表 就是可空数组）；参考型不允许
    if (opt && fl.indexOf('AS_RECEIVE_VAR') === 0) throw new Error('参考型参数不允许可空：' + ty);
    return [dt, opt ? fl + ' | AS_DEFAULT_VALUE_IS_EMPTY' : fl];
  }
  const dt = DT[base] || 'SDT_INT';
  if (base === 'double' || base === 'bool') return [dt, opt ? EMPTY : '0'];
  if (base === 'text' || base === 'bin' || base === 'subptr') return [dt, opt ? EMPTY : '0'];
  return [dt, opt ? EMPTY : '0'];
}
const esc = (s) => (s || '').replace(/\\/g, '\\\\').replace(/"/g, '\\"');

const opSet = [];                 // 去重后的操作码
const args = [], cmds = [], impls = [], funcs = [], names = [], typereg = [], cmdidx = [];
// 本族的命令表行在 g_commands 里的起始下标（表只往末尾追加，所以这个数只会变大不会变小）
const BASE = spec['_行基址'];
let row = 0;                      // 本族内的相对行号（英文命令名编号用它，保持稳定）

for (const cls of spec['类']) {
  const tag = cls['英文'];
  const kind = cls['对象'];
  const base = row;
  args.push(`static ARG_INFO g_argCopy${tag}[] = {
    { _T("对象"), _T("要复制配置的源对象：把它的配置复制到本对象。"), 0, 0, MAKELONG(${cls['类型宏']}, 0), 0, 0 },
};`);

  cmds.push(`    CX_LIFECYCLE("${cls['名']}", "${cls['名']}", "${tag}", g_argCopy${tag}, "${esc(cls['说明'])}"),`);
  impls.push(
`void ${tag}Construct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pArgInf);
    pRetData->m_int = (INT)HNew(${kind});
}

void ${tag}Copy(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    HCopyObj(pArgInf, ${kind});
}

void ${tag}Destruct(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)
{
    UNREFERENCED_PARAMETER(nArgCount); UNREFERENCED_PARAMETER(pRetData);
    if (pArgInf[0].m_pCompoundData != NULL)
    {
        HFreeObj((CxHttpObj*)(*(PINT)pArgInf[0].m_pCompoundData));
        *(PINT)pArgInf[0].m_pCompoundData = 0;
    }
}`);
  funcs.push(`    ${tag}Construct, ${tag}Copy, ${tag}Destruct,`);
  names.push(`    _T("${tag}Construct"), _T("${tag}Copy"), _T("${tag}Destruct"),`);
  row += 3;

  for (const c of cls['命令']) {
    const op = c['op'] || `${tag}_${c.impl}`;
    if (!opSet.includes(op)) opSet.push(op);
    const an = `g_h${tag}${c.impl}Args`;
    if (c['参'].length) {
      args.push(`static ARG_INFO ${an}[] = {`);
      for (const p of c['参']) {
        const [nm, ty, ex] = p;
        const [dt, fl] = paramTy(ty);
        args.push(`    { _T("${esc(nm)}"), _T("${esc(ex || '')}"), 0, 0, ${dt}, 0, ${fl} },`);
      }
      args.push('};');
    }
    const ret = c['返'] === 'null' ? '_SDT_NULL' : (DT[c['返']] || 'SDT_INT');
    cmds.push(`    CX_OBJ("${esc(c['名'])}", "${tag}${String(row + 1).padStart(3, '0')}", "${esc(c['释'] || '')}", _CMD_OS(__OS_WIN), ${ret}, ${c['参'].length}, ${c['参'].length ? an : 'NULL'}),`);
    const fn = `${tag}_${c.impl}`;
    impls.push(`void ${fn}(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)\n{\n    CxHttpInvoke(pRetData, nArgCount, pArgInf, HOP_${op}, ${kind});\n}`);
    funcs.push(`    ${fn},`);
    names.push(`    _T("${fn}"),`);
    row += 1;
  }

  // m_pnCmdsIndex 要含 构造/复制/析构 三行（与 Json2 等既有类一致）
  const nAll = row - base;
  const idx = `g_${tag}CmdIndex`;
  const cells = Array.from({ length: nAll }, (_, i) => BASE + base + i).join(', ');
  cmdidx.push(`// ${cls['名']}：生命周期 3 + 成员 ${nAll - 3}，行区间 ${BASE + base}..${BASE + row - 1}
INT ${idx}[] = { ${cells} };
static_assert(sizeof(${idx}) / sizeof(INT) == ${nAll}, "${cls['名']} 成员命令条数变了");
static_assert(${cls['类型宏']} == ${cls['序号']}, "${cls['名']} 数据类型序号与登记顺序不一致");`);
  typereg.push(`    CX_TYPE("${cls['名']}", "${tag}", "${esc(cls['说明'])}。原模块同名类，参数逐字一致。", ${idx}),`);
  console.log(`     ${cls['名']}: 成员命令 ${cls['命令'].length} + 生命周期 3，绝对行 ${BASE + base}..${BASE + row - 1}，数据类型序号 ${cls['序号']}`);
}

// ==== 全局命令（原模块里的「公开子程序」）====
// 表行用 CX_CMD（有类别、不是成员命令），实现桩一律转 CxHttpGlobalInvoke，
// 由 cxhttp_glob.inc 按操作码分到 util / curl 路线 / Win 路线。
for (const g of (spec['全局命令'] || [])) {
  const en = g['英文'];
  const op = `G_${en}`;
  if (!opSet.includes(op)) opSet.push(op);
  const an = `g_g${en}Args`;
  if (g['参'].length) {
    args.push(`static ARG_INFO ${an}[] = {`);
    for (const p of g['参']) {
      const [nm, ty, ex] = p;
      const [dt, fl] = paramTy(ty);
      args.push(`    { _T("${esc(nm)}"), _T("${esc(ex || '')}"), 0, 0, ${dt}, 0, ${fl} },`);
    }
    args.push('};');
  }
  const ret = g['返'] === 'null' ? '_SDT_NULL' : (DT[g['返']] || 'SDT_INT');
  cmds.push(`    CX_CMD("${esc(g['名'])}", "${en}", "${esc(g['说明'] || g['名'])}", ${g['类别'] || 6}, ${ret}, ${g['参'].length}, ${g['参'].length ? an : 'NULL'}),`);
  impls.push(`void G_${en}(PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf)\n{\n    CxHttpGlobalInvoke(pRetData, nArgCount, pArgInf, HOP_${op});\n}`);
  funcs.push(`    G_${en},`);
  names.push(`    _T("G_${en}"),`);
  row += 1;
}

const W = (f, s) => fs.writeFileSync(path.join(here, f), s, 'utf8');
W('cxhttp_ops.h', `${HDR}\n// 成员命令操作码；共享操作码只出现一次（对象种类由 CxHttpInvoke 的 nKind 决定）\nenum\n{\n${opSet.map((o) => `    HOP_${o},`).join('\n')}\n    HOP_COUNT\n};\n`);
W('cxhttp_args.inc', `${HDR}\n${args.join('\r\n')}\r\n`);
W('cxhttp_cmds.inc', `${HDR}\n${cmds.join('\r\n')}\r\n`);
W('cxhttp_impl.inc', `${HDR}\n${impls.join('\r\n\r\n')}\n`);
W('cxhttp_funcs.inc', `${HDR}\n${funcs.join('\r\n')}\r\n`);
W('cxhttp_names.inc', `${HDR}\n${names.join('\r\n')}\r\n`);
W('cxhttp_typereg.inc', `${HDR}\n${typereg.join('\r\n')}\r\n`);
W('cxhttp_cmdindex.inc', `${HDR}
${cmdidx.join('\r\n')}
// 行基址校验：_行基址 必须等于本族之前的命令表行数；本族末尾必须正好是 g_commands 末尾。
// （数组元素不是常量表达式，起始行只能靠这条总长断言间接锁定。）
static_assert(sizeof(g_commands) / sizeof(CMD_INFO) >= ${BASE + row}, "HTTP访问 族的 _行基址 超过了 g_commands 长度");
`);
console.log('[OK] 共 %d 行命令（成员 %d + 生命周期 %d）/ 操作码 %d 个 / 参数表 %d 个 / 实现函数 %d 个',
  row, row - spec['类'].length * 3, spec['类'].length * 3, opSet.length,
  args.filter((l) => l.startsWith('static ARG_INFO')).length, impls.length);
await import('./gen_http_cancel_table.mjs');
