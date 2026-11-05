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
const srcX = src0.replace(/#include\s+"(http访问|线程锁)\/([a-z_0-9]+\.inc)"/g,
  (_m, dir, f) => fs.readFileSync(path.join(here, dir, f), 'utf8'));

const m = srcX.match(/const char\* g_commandFuncNames\[\] = \{([\s\S]*?)\};/);
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
    const re = /CX_CMD\("([^"]*)"|CX_CMD_AR\("([^"]*)"|CX_OBJ\("([^"]*)"|CX_OBJ_AR\("([^"]*)"|CX_LIFECYCLE\("[^"]*",\s*"([^"]*)"|\/\/ CX_APPEND_OWNER ([^\r\n]+)/g;
    const entries = [];
    let owner;
    for (const x of tableM[1].matchAll(re))
    {
        if (x[6]) { owner = x[6] === '-' ? undefined : x[6]; continue; }
        if (x[1] !== undefined || x[2] !== undefined) entries.push({ name: x[1] || x[2], member: false });
        else if (x[3] !== undefined || x[4] !== undefined) entries.push({ name: x[3] || x[4], member: true, owner });
        else for (const suffix of ['构造函数', '复制函数', '析构函数']) entries.push({ name: x[5] + suffix, member: true });
    }
    return entries;
}

// 类以构造函数登记；末尾追加成员通过 OWNER 标记归属，保留既有命令序号。
function splitBlocks(entries)
{
    const blocks = [];
    let stray = null;
    entries.forEach((e, i) => {
        if (!e.member) return;
        if (e.name.endsWith('构造函数')) { blocks.push({ indices: [i], head: e.name }); return; }
        if (!blocks.length) { stray = stray || e.name; return; }
        const block = e.owner ? blocks.find(b => b.head === e.owner + '构造函数') : blocks[blocks.length - 1];
        if (!block) throw new Error(`追加成员 ${e.name} 的类 ${e.owner} 未登记`);
        block.indices.push(i);
    });
    if (stray) { console.log(`[FAIL] 成员命令「${stray}」落在任何类块之前`); process.exit(1); }
    return blocks;
}

function renumberClassIndexes(text)
{
    const blocks = splitBlocks(parseCommands(text));
    const varNames = [...text.matchAll(/^INT (g_\w+CmdIndex)\[\] = \{/gm)].map((x) => x[1]);
    // 表可以比块少：末尾几张由 http访问/gen_http_table.mjs 生成，不在 lib.cpp 里
    if (varNames.length > blocks.length)
    {
        console.log(`[FAIL] 成员下标表 ${varNames.length} 张，比命令表里的 ${blocks.length} 个类块还多`);
        console.log('  块: ' + blocks.map((b) => b.head).join(' / '));
        process.exit(1);
    }

    let changed = 0;
    for (let k = 0; k < varNames.length; k++)
    {
        const idx = blocks[k].indices;
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
    'extern "C" INT WINAPI cxsoft_ProcessNotifyLib (INT nMsg, DWORD dwParam1, DWORD dwParam2)',
    '{',
    '    return cx::cxsoft_ProcessNotifyLib (nMsg, dwParam1, dwParam2);',
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

// 生成的段自带库信息收尾的 #endif，切点要退到它，否则重跑会重复一份 #endif。
// 重要：本脚本只在 lib.cpp 原文上改写「尾部段」，绝不把 .inc 的内容内联回去；
// HTTP访问 族的下标表由 http访问/gen_http_table.mjs 生成，这里只核对不修改。
const nsEnd = src0.indexOf('}  // namespace');
if (nsEnd < 0) { console.log('[FAIL] 找不到 namespace 收尾'); process.exit(1); }
let cut = src0.lastIndexOf('#endif  // !__E_STATIC_LIB', nsEnd);
if (cut < 0) cut = nsEnd;

// 1) 手写在 lib.cpp 里的那几张下标表：按命令表重算
const headNew = renumberClassIndexes(src0.slice(0, cut));
// 2) 生成在各族 cmdindex.inc 里的那几张：只核对，不匹配就报错让人去重跑生成器
{
    const blocksAll = splitBlocks(parseCommands(srcX));
    const genFiles = ['http访问/cxhttp_cmdindex.inc', '线程锁/lock_cmdindex.inc', '线程锁/join_cmdindex.inc', '线程锁/cancel_cmdindex.inc'];
    const vars = [];
    for (const rel of genFiles)
    {
        const incFile = path.join(here, ...rel.split('/'));
        if (!fs.existsSync(incFile)) continue;
        vars.push(...fs.readFileSync(incFile, 'utf8').matchAll(/^INT (g_\w+CmdIndex)\[\] = \{([\s\S]*?)\};/gm));
    }
    if (vars.length)
    {
        const firstIdx = (s) => { const m = s.match(/\d+/); return m ? Number(m[0]) : 1e9; };
        const genNames = new Set(vars.map((x) => x[1]));
        // lib.cpp 里的块序 = 手写表 + 生成表，按命令表逐块比对绝对下标（统一按首下标排序）
        const allTables = [
            ...[...headNew.matchAll(/^INT (g_\w+CmdIndex)\[\] = \{([\s\S]*?)\};/gm)].map((x) => ({ name: x[1], body: x[2], generated: genNames.has(x[1]) })),
            ...vars.map((x) => ({ name: x[1], body: x[2], generated: true })),
        ].sort((a, b) => firstIdx(a.body) - firstIdx(b.body));
        if (allTables.length !== blocksAll.length)
        {
            console.log(`[FAIL] 命令表里有 ${blocksAll.length} 个类，下标表只有 ${allTables.length} 张（少了 ${blocksAll.length - allTables.length} 张）`);
            console.log('  改过命令表就跑：node http访问/gen_http_table.mjs');
            process.exit(1);
        }
        let bad = 0;
        allTables.forEach((tb, k) => {
            const cur = [...tb.body.matchAll(/\d+/g)].map((x) => Number(x[0]));
            const want = blocksAll[k].indices;
            if (cur.join(',') !== want.join(','))
            {
                bad++;
                console.log(`[FAIL] ${tb.name}（${blocksAll[k].head}）下标是 ${cur[0]}..${cur[cur.length - 1]}/${cur.length}，命令表算出来是 ${want[0]}..${want[want.length - 1]}/${want.length}`);
            }
        });
        if (bad)
        {
            console.log('  生成表的部分请跑：node http访问/gen_http_table.mjs（并同步 cxhttp_api.json 的 _行基址）');
            process.exit(1);
        }
        console.log(`[OK] ${allTables.length} 张类成员下标表与命令表一致（其中 ${vars.length} 张由生成器维护）`);
    }
}
fs.writeFileSync(file, headNew + block);
console.log(`[OK] 已生成 ${names.length} 个转发：${path.basename(file)}（lib.cpp 主体未被内联改写）`);
