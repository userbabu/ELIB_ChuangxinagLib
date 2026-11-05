// 从 zyJson2.51 导出的易语言源码文本里抽取 Json2 类的公开方法面，落成 json2_api.json。
// 命令表（CMD_INFO / ARG_INFO / 实现名表）全部由这份描述生成，保证名称、参数名、
// 参数类型、可空/参考/数组 属性与原模块逐字一致（历史兼容的根据）。
// 用法：node gen_json2_table.mjs dump      → 打印/写 json2_api.json
//       node gen_json2_table.mjs emit      → 写 include/cx_json2_table.h 与 lib.cpp 追加段
import fs from 'node:fs';
import path from 'node:path';

const here = decodeURIComponent(new URL('.', import.meta.url).pathname).replace(/^\/([A-Za-z]:)/, '$1');
const SRC = path.join(here, '../zyJson支持库/源码txt/代码/Json2.class.e.txt');
const NAMES = path.join(here, 'json2_names.json');

const TYPE_MAP = {
  '文本型': 'SDT_TEXT',
  '整数型': 'SDT_INT',
  '长整数型': 'SDT_INT64',
  '双精度小数型': 'SDT_DOUBLE',
  '逻辑型': 'SDT_BOOL',
  '字节集': 'SDT_BIN',
  'Json2': 'MAKELONG(CX_TYPE_JSON2, 0)',
  '子程序指针': 'SDT_SUB_PTR',
};

function parse()
{
  const lines = fs.readFileSync(SRC, 'utf8').replace(/^/, '').split(/\r?\n/);
  const methods = [];
  let cur = null;
  for (const raw of lines)
  {
    const line = raw.trim();
    if (line.startsWith('.子程序 '))
    {
      const parts = line.slice('.子程序 '.length).split(/,\s*/);
      const [name, ret = '', flags = '', explain = ''] = parts;
      const isPublic = /公开/.test(flags) || parts.length >= 3 && /公开/.test(flags);
      cur = { name, ret: ret.trim(), public: flags.includes('公开'), explain: explain.trim().replace(/^,\s*/, ''), params: [], line: lines.indexOf(raw) + 1 };
      if (cur.ret === '') cur.ret = '无';
      methods.push(cur);
      continue;
    }
    if (line.startsWith('.参数 '))
    {
      if (!cur) continue;
      const body = line.slice('.参数 '.length);
      const parts = body.split(/,\s*/);
      const p = { name: parts[0].trim(), type: (parts[1] || '').trim(), attr: (parts[2] || '').trim(), explain: (parts[3] || '').trim() };
      // 属性列里可能同时写 参考 可空 数组
      p.byRef = /参考/.test(p.attr);
      p.mayEmpty = /可空/.test(p.attr);
      p.isArray = /数组/.test(p.attr);
      cur.params.push(p);
      continue;
    }
    if (line.startsWith('.局部变量') || line.startsWith('.如果') || line.startsWith('.')) continue;
  }
  return methods;
}

const mode = process.argv[2] || 'dump';
const all = parse();
const pub = all.filter((m) => m.public);
const api = pub.map((m, i) => ({
  idx: i + 1,
  cn: m.name,
  ret: m.ret,
  explain: m.explain,
  line: m.line,
  params: m.params.map((p) => ({ n: p.name, t: p.type, ref: p.byRef, empty: p.mayEmpty, ary: p.isArray, e: p.explain })),
}));

if (mode === 'dump')
{
  fs.writeFileSync('json2_api.json', JSON.stringify(api, null, 1), 'utf8');
  const unknown = [];
  for (const m of api)
  {
    if (!TYPE_MAP[m.ret]) unknown.push(`返回类型 ${m.ret} (${m.cn})`);
    for (const p of m.params) if (!TYPE_MAP[p.t]) unknown.push(`参数类型 ${p.t} (${m.cn}.${p.n})`);
  }
  console.log(`[OK] 公开方法 ${api.length} 条，参数总数 ${api.reduce((s, m) => s + m.params.length, 0)}`);
  if (unknown.length) console.log('[WARN] 未映射的类型：\n  ' + [...new Set(unknown)].join('\n  '));
  const names = fs.existsSync(NAMES) ? JSON.parse(fs.readFileSync(NAMES, 'utf8')) : {};
  const missing = api.filter((m) => !names[m.cn]).map((m) => m.cn);
  console.log(missing.length ? `[NEXT] json2_names.json 缺少 ${missing.length} 条：${missing.join(' / ')}` : '[OK] 英文名映射齐全');
}

// ============================================================ emit
// 把解析出的签名落成 C++ 表：命令名 / 英文名 / 参数名 / 类型 / 可空·参考·数组 逐字照抄原模块，
// 历史兼容性由这份生成物保证。改完原模块或英文名后重跑：node gen_json2_table.mjs emit
if (mode === 'emit')
{
  if (!fs.existsSync(NAMES)) { console.log('[FAIL] 缺少 json2_names.json'); process.exit(1); }
  const names = JSON.parse(fs.readFileSync(NAMES, 'utf8'));
  const miss = api.filter((m) => !names[m.cn]);
  if (miss.length) { console.log('[FAIL] 未命名：' + miss.map((m) => m.cn).join(' / ')); process.exit(1); }
  const egList = api.map((m) => names[m.cn]);
  const dup = egList.filter((v, i) => egList.indexOf(v) !== i);
  if (dup.length) { console.log('[FAIL] 英文名重复：' + [...new Set(dup)].join(', ')); process.exit(1); }

  const BS = String.fromCharCode(92);
  const esc = (s) => (s || '').split(BS).join(BS + BS).split('"').join('\\"').replace(/\r/g, '');
  const retType = (t) => (t === '无' ? '_SDT_NULL' : TYPE_MAP[t]);
  // 只读数组入参：原版标的是「数组传址」，但实现只读不回写，标传址只会逼调用方先声明一个变量。
  const RO_ARY = new Set(['置文本数组', '置文本数组W', '置文本数组UTF8', '置数值数组UTF8', '置整数数组UTF8', '置逻辑数组UTF8']);
  const argFlag = (p, cn) =>
  {
    const EMP = 'AS_DEFAULT_VALUE_IS_EMPTY', REF = 'AS_RECEIVE_VAR', VARA = 'AS_RECEIVE_VAR_ARRAY', ARY = 'AS_RECEIVE_ARRAY_DATA';
    if (p.ary) return (!p.ref || RO_ARY.has(cn)) ? ARY : (p.empty ? EMP + ' | ' + ARY : VARA);
    if (p.ref) return p.empty ? REF + ' | ' + EMP : REF;
    return p.empty ? EMP : '0';
  };
  const vArgs = (m) => 'j2' + names[m.cn] + 'Args';

  let argsTxt = '', cmdsTxt = '', funcsTxt = '', namesTxt = '', stubs = '', ops = '', decls = '';
  api.forEach((m, i) => {
    const eg = names[m.cn], v = 'J2' + eg;
    ops += `#define J2OP_${eg} ${i + 1}\n`;
    decls += `JSON_CMD(${v});\n`;
    stubs += `JSON_CMD(${v}) { J2Invoke(J2OP_${eg}, pRetData, nArgCount, pArgInf); }\n`;
    if (m.params.length)
    {
      argsTxt += `static ARG_INFO g_${vArgs(m)}[] = {\n`;
      for (const p of m.params)
        argsTxt += `    { _T("${esc(p.n)}"), _T("${esc(p.e)}"), 0, 0, ${TYPE_MAP[p.t]}, 0, ${argFlag(p, m.cn)} },\n`;
      argsTxt += `};\n`;
    }
    cmdsTxt += `    CX_OBJ("${esc(m.cn)}", "${eg}", "${esc(m.explain)}", _CMD_OS(__OS_WIN), ${retType(m.ret)}, ${m.params.length}, ${m.params.length ? 'g_' + vArgs(m) : 'NULL'}),\n`;
    funcsTxt += `    ${v},\n`;
    namesTxt += `    "${v}",\n`;
  });

  const total = api.length + 3;                       // 成员命令 + 构造/复制/析构
  const idxStart = 280;                              // 现有命令占 0..279（3.21.0 校准：写错会让 Json2 成员命令整体错一位，实跑现场崩溃）
  const idxList = Array.from({ length: total }, (_, i) => idxStart + i);

  const consts = [
    ['JV类型_未定义', '-1', 'JVTYPE_UNDEFINED'], ['JV类型_空', '0', 'JVTYPE_NULL'],
    ['JV类型_双精度型', '1', 'JVTYPE_DOUBLE'], ['JV类型_长整数型', '2', 'JVTYPE_LONG'],
    ['JV类型_逻辑型', '3', 'JVTYPE_BOOL'], ['JV类型_文本型', '4', 'JVTYPE_STRING'],
    ['JV类型_数组', '5', 'JVTYPE_ARRAY'], ['JV类型_对象', '6', 'JVTYPE_OBJECT'],
    ['JSON_OK', '0', 'JSON_OK'], ['JSON_ERROR', '-1', 'JSON_ERROR'],
    ['JSON_INVALIDARG', '-2', 'JSON_INVALIDARG'], ['JSON_MEMORY', '-3', 'JSON_MEMORY'],
    ['JSON_INVALIDNAME', '-4', 'JSON_INVALIDNAME'], ['JSON_OBJECTPARENT', '-5', 'JSON_OBJECTPARENT'],
    ['JSON_INVALIDVALUE', '-6', 'JSON_INVALIDVALUE'],
  ];
  let constsTxt = 'static LIB_CONST_INFO g_constants[] = {\n';
  for (const c of consts)
    constsTxt += `    { _T("${c[0]}"), _T("${c[2]}"), _T("${c[0]}"), _CMD_OS(__OS_WIN), CT_NUM, NULL, ${c[1]} },\n`;
  // 这两段由别的生成器写进 g_constants 内部；重写整段时必须原样带上，否则会被擦掉（3.21.0 踩过：curl 常量族清零）。
  constsTxt += '// ==== HTTP访问 libcurl 常量（http访问/gen_http_consts.mjs 生成，勿手改）====\n' +
    '#include "http访问/cxhttp_consts.inc"\n' +
    '// ==== 原模块自有常量（HTTP状态_/HTTP配置_/INTERNET_/WINHTTP_/CP_/SECURITY_ 等，gen_mod_consts.mjs 生成）====\n' +
    '#include "http访问/cxmod_consts.inc"\n';
  constsTxt += '};\n';

  fs.writeFileSync(path.join(here, 'include/cx_json2_table.h'),
    '// 由 gen_json2_table.mjs 从 zyJson2.51 原模块生成，勿手改。\n' +
    '// Json2 成员命令的操作码与实现名；顺序即命令表顺序（只能往末尾追加）。\n' +
    '#ifndef CX_JSON2_TABLE_H\n#define CX_JSON2_TABLE_H\n\n' +
    'JSON_CMD(J2ObjConstruct); JSON_CMD(J2ObjCopy); JSON_CMD(J2ObjDestruct);\n\n' + ops + '\n' + decls +
    '\n#endif  // CX_JSON2_TABLE_H\n', 'utf8');

  fs.writeFileSync(path.join(here, 'include/cx_json2_stubs.inc'),
    '// 由 gen_json2_table.mjs 生成，勿手改：每条命令转发到 J2Invoke(操作码)。\n' +
    'JSON_CMD(J2ObjConstruct) { J2Invoke(0, pRetData, nArgCount, pArgInf); }\n' +
    'JSON_CMD(J2ObjCopy) { J2Invoke(-1, pRetData, nArgCount, pArgInf); }\n' +
    'JSON_CMD(J2ObjDestruct) { J2Invoke(-2, pRetData, nArgCount, pArgInf); }\n\n' + stubs, 'utf8');

  const LIB = path.join(here, 'lib.cpp');
  let lib = fs.readFileSync(LIB, 'utf8');
  const M = (tag) => '// ==== JSON2 ' + tag + '（gen_json2_table.mjs 生成，勿手改）====';
  const ME = (tag) => '// ==== JSON2 ' + tag + ' 结束 ====';
  function inject(anchor, tag, body, opts)
  {
    if (lib.includes(M(tag)))
    {
      const re = new RegExp(M(tag).replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + '[\\s\\S]*?' +
        ME(tag).replace(/[.*+?^${}()|[\]\\]/g, '\\$&'));
      lib = lib.replace(re, M(tag) + '\n' + body + ME(tag));
      return;
    }
    if (!lib.includes(anchor)) { console.log('[FAIL] 找不到锚点：' + tag); process.exit(1); }
    const block = M(tag) + '\n' + body + ME(tag) + '\n';
    lib = (opts && opts.after) ? lib.replace(anchor, anchor + block) : lib.replace(anchor, block + anchor);
  }

  // 往已有数组的结尾追加条目（绝不插到中间：命令序号是历史契约）。
  function injectTail(openAnchor, tag, body)
  {
    if (lib.includes(M(tag)))
    {
      const re = new RegExp(M(tag).replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + '[\\s\\S]*?' +
        ME(tag).replace(/[.*+?^${}()|[\]\\]/g, '\\$&'));
      lib = lib.replace(re, M(tag) + '\n' + body + ME(tag));
      return;
    }
    const at = lib.indexOf(openAnchor);
    if (at < 0) { console.log('[FAIL] 找不到数组开头：' + tag); process.exit(1); }
    const close = lib.indexOf('\n};', at);
    if (close < 0) { console.log('[FAIL] 找不到数组结尾：' + tag); process.exit(1); }
    const head = lib.slice(0, close).replace(/\s+$/, '');
    const comma = head.endsWith(',') ? '' : ',';
    lib = head + comma + '\n' + M(tag) + '\n' + body + ME(tag) + lib.slice(close);
  }

  inject('#include <cx_uni.h>', '头文件', '#include <cx_json.h>\n');
  inject('// ============================================================ 命令表', '参数表',
    'static ARG_INFO g_j2ObjCopyArgs[] = {\n    { _T("源对象"), _T("要复制的 Json2 对象。"), 0, 0, MAKELONG(CX_TYPE_JSON2, 0), 0, AS_RECEIVE_VAR },\n};\n' + argsTxt);
  injectTail('CMD_INFO g_commands[] = {', '成员命令',
    '    CX_LIFECYCLE("Json2", "Json2", "Json2", g_j2ObjCopyArgs, "创建/复制/销毁一个 Json2 值。"),\n' + cmdsTxt);
  injectTail('PFN_EXECUTE_CMD g_commandFuncs[] = {', '成员命令实现表',
    '    J2ObjConstruct,\n    J2ObjCopy,\n    J2ObjDestruct,\n' + funcsTxt);
  injectTail('const char* g_commandFuncNames[] = {', '成员命令实现名表',
    '    "J2ObjConstruct",\n    "J2ObjCopy",\n    "J2ObjDestruct",\n' + namesTxt);
  inject('#define CX_TYPE(', '类成员下标表与常量表',
    constsTxt + '\n// Json2：成员命令下标（构造/复制/析构 + 112 条公开方法，追加在表尾）\n' +
    `INT g_json2CmdIndex[] = { ${idxList.join(', ')} };\n` +
    `static_assert(sizeof(g_json2CmdIndex) / sizeof(INT) == ${total}, "Json2 成员命令条数变了");\n\n`);
  injectTail('LIB_DATA_TYPE_INFO g_dataTypes[] = {', 'Json2 类登记',
    '    CX_TYPE("Json2", "JsonValue",\n' +
    '        "zyJson 同源实现：JSON 解析与读写；属性按插入顺序输出；W 后缀用 UTF-16 字节集，文本型按 GBK。",\n' +
    '        g_json2CmdIndex),\n');
  lib = lib.replace('    NULL, NULL,\n    0, NULL,\n    const_cast<TCHAR*>(_T(CX_CURL_DLL_NAME',
    '    NULL, NULL,\n    sizeof(g_constants) / sizeof(g_constants[0]), g_constants,\n    const_cast<TCHAR*>(_T(CX_CURL_DLL_NAME');
  fs.writeFileSync(LIB, lib, 'utf8');
  console.log('[OK] 已生成 cx_json2_table.h / cx_json2_stubs.inc 并更新 lib.cpp 表段');
  console.log(`     成员命令 ${api.length} 条 + 生命周期 3 条 = ${total}，命令下标 ${idxStart}..${idxStart + total - 1}`);
}


