#!/usr/bin/env node
// 从 lib.cpp 的 g_commandFuncNames 生成「静态编译转发」段：
// 易语言静态编译按这些名字去 static_lib\cxsoft_static.lib 找符号，
// 而实现都在 namespace cx 里，所以每个命令需要一份 extern "C" 全局转发。
// 同时按命令表重算各类的成员命令下标表 —— 表内删/插一条命令会挪动后面所有序号，
// 类的 m_pnCmdsIndex 是绝对下标，手改漏一张就是「类里找不到命令」（离线宿主会先崩在那里）。
// 用法：node gen_static_trampolines.mjs [lib.cpp 路径]   （改完命令表后重跑）
import fs from 'node:fs';
import path from 'node:path';

const here = decodeURIComponent(new URL('.', import.meta.url).pathname).replace(/^\/([A-Za-z]:)/, '$1');
const file = path.resolve(process.argv[2] || path.join(here, 'lib.cpp'));
const src0 = fs.readFileSync(file, 'utf8');

const m = src0.match(/const char\* g_commandFuncNames\[\] = \{([\s\S]*?)\};/);
if (!m) { console.log('[FAIL] 找不到 g_commandFuncNames'); process.exit(1); }
const names = [...m[1].matchAll(/"([A-Za-z0-9_]+)"/g)].map((x) => x[1]);
if (!names.length) { console.log('[FAIL] 名字表为空'); process.exit(1); }

const dup = names.filter((n, i) => names.indexOf(n) !== i);
if (dup.length) { console.log(`[FAIL] 名字表有重复项：${[...new Set(dup)].join(', ')}（静态编译会符号冲突）`); process.exit(1); }

// 命令表 → 条目序列：CX_CMD=全局命令，CX_OBJ=类成员命令，CX_LIFECYCLE=构造/复制/析构三条成员命令。
function parseCommands(text)
{
    const tableM = text.match(/CMD_INFO g_commands\[\] = \{([\s\S]*?)\n\};/);
    if (!tableM) { console.log('[FAIL] 找不到 g_commands'); process.exit(1); }
    // 两种命名都认：CX_CMD/CX_OBJ/CX_LIFECYCLE（创想软件支持库）与 CMD/OBJ/LIFE（本库）。
    const re = /CX_CMD\("([^"]*)"|(?<![A-Z_])CMD\("([^"]*)"|CX_OBJ\("([^"]*)"|(?<![A-Z_])OBJ\("([^"]*)"|CX_LIFECYCLE\("[^"]*",\s*"([^"]*)"|(?<![A-Z_])LIFE\("([^"]*)"/g;
    const entries = [];
    for (const x of tableM[1].matchAll(re))
    {
        if (x[1] !== undefined || x[2] !== undefined) entries.push({ name: x[1] !== undefined ? x[1] : x[2], member: false });
        else if (x[3] !== undefined || x[4] !== undefined) entries.push({ name: x[3] !== undefined ? x[3] : x[4], member: true });
        else if (x[5] !== undefined) { for (const suffix of ['构造函数', '复制函数', '析构函数']) entries.push({ name: x[5] + suffix, member: true }); }
        else entries.push({ name: x[6], member: true });
    }
    return entries;
}

// 成员命令以「…构造函数」起块，块内连续；块序即类序。
function splitBlocks(entries)
{
    const blocks = [];
    let stray = null;
    entries.forEach((e, i) => {
        if (!e.member) return;
        if (e.name.endsWith('构造函数')) { blocks.push({ start: i, end: i + 1, head: e.name }); return; }
        if (!blocks.length) { stray = stray || e.name; return; }
        blocks[blocks.length - 1].end = i + 1;
    });
    if (stray) { console.log(`[FAIL] 成员命令「${stray}」落在任何类块之前`); process.exit(1); }
    return blocks;
}

function renumberClassIndexes(text)
{
    const blocks = splitBlocks(parseCommands(text));
    const varNames = [...text.matchAll(/^INT (g_\w+CmdIndex)\[\] = \{/gm)].map((x) => x[1]);
    if (varNames.length !== blocks.length)
    {
        console.log(`[FAIL] 类块 ${blocks.length} 个，成员下标表 ${varNames.length} 张，对不上`);
        console.log('  块: ' + blocks.map((b) => b.head).join(' / '));
        process.exit(1);
    }

    let changed = 0;
    for (let k = 0; k < varNames.length; k++)
    {
        const idx = [];
        for (let i = blocks[k].start; i < blocks[k].end; i++) idx.push(i);
        const decl = new RegExp(`INT ${varNames[k]}\\[\\] = \\{([\\s\\S]*?)\\};`);
        const cur = text.match(decl);
        if (!cur) { console.log(`[FAIL] 找不到 ${varNames[k]}`); process.exit(1); }
        const oldIdx = [...cur[1].matchAll(/\d+/g)].map((x) => Number(x[0]));
        if (oldIdx.join(',') === idx.join(',')) continue;
        text = text.replace(decl, `INT ${varNames[k]}[] = { ${idx.join(', ')} };`);
        const asrt = new RegExp(`static_assert\\(sizeof\\(${varNames[k]}\\) / sizeof\\(INT\\) == \\d+,`);
        if (asrt.test(text)) text = text.replace(asrt, `static_assert(sizeof(${varNames[k]}) / sizeof(INT) == ${idx.length},`);
        console.log(`[OK] ${varNames[k]}（${blocks[k].head}）: ${oldIdx[0]}..${oldIdx[oldIdx.length - 1]}/${oldIdx.length} -> ${idx[0]}..${idx[idx.length - 1]}/${idx.length}`);
        changed++;
    }
    console.log(changed ? `[OK] 已按命令表重算 ${changed} 张类成员下标表` : '[OK] 类成员下标表与命令表一致');
    return text;
}

const notifyM = src0.match(/#define\s+LIB_NOTIFY_FUNC_NAME\s+"([A-Za-z0-9_]+)"/);
if (!notifyM) { console.error('[FAIL] 找不到 LIB_NOTIFY_FUNC_NAME，无法生成库通知转发'); process.exit(1); }
const notifyName = notifyM[1];

const MARK = '// ==== 以下由 gen_static_trampolines.mjs 生成，勿手改 ====';
const block = [
    '#endif  // !__E_STATIC_LIB',
    '',
    '}  // namespace cx',
    '',
    MARK,
    '// 静态编译时易语言按名字到 cxsoft_static.lib 里找命令实现，',
    '// 而实现在 namespace cx 内，故每个命令生成一份 extern "C" 全局转发。',
    '#define CX_FWD(fn) extern "C" void fn (PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf) { cx::fn (pRetData, nArgCount, pArgInf); }',
    '',
    ...names.map((n) => `CX_FWD(${n})`),
    '',
    '#undef CX_FWD',
    '',
    `extern "C" INT WINAPI ${notifyName} (INT nMsg, DWORD dwParam1, DWORD dwParam2)`,
    '{',
    `    return cx::${notifyName} (nMsg, dwParam1, dwParam2);`,
    '}',
    '',
    '#ifndef __E_STATIC_LIB',
    '',
    '// 导出名必须固定为 GetNewInf 且不带修饰，所以靠 .def 导出而不是 __declspec。',
    'extern "C" PLIB_INFO WINAPI GetNewInf()',
    '{',
    '    return &cx::g_libraryInfo;',
    '}',
    '',
    'BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID)',
    '{',
    '    if (dwReason == DLL_PROCESS_ATTACH) DisableThreadLibraryCalls(hModule);',
    '    return TRUE;',
    '}',
    '',
    '#endif  // !__E_STATIC_LIB',
    '',
].join('\n');

const nsEnd = src0.indexOf('}  // namespace');
if (nsEnd < 0) { console.log('[FAIL] 找不到 namespace 收尾'); process.exit(1); }
// 生成的段自带库信息收尾的 #endif，切点要退到它，否则重跑会重复一份 #endif。
let cut = src0.lastIndexOf('#endif  // !__E_STATIC_LIB', nsEnd);
if (cut < 0) cut = nsEnd;
const head = renumberClassIndexes(src0.slice(0, cut));
fs.writeFileSync(file, head + block);
console.log(`[OK] 已生成 ${names.length} 个转发：${path.basename(file)}`);
