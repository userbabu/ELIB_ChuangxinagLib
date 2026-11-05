#!/usr/bin/env node
// 从 lib.cpp 的 g_commandFuncNames 生成「静态编译转发」段：
// 易语言静态编译按这些名字去 static_lib\edgewv_static.lib 找符号，
// 而实现都在 namespace wv 里，所以每个命令需要一份 extern "C" 全局转发。
// 可视化组件的 ITF 接口函数同理要有全局符号，故一并转发并 dllexport。
// 用法：node gen_static_trampolines.mjs [lib.cpp 路径]   （改完命令表后重跑）
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const file = path.resolve(process.argv[2] || path.join(path.dirname(fileURLToPath(import.meta.url)), 'lib.cpp'));
const src = fs.readFileSync(file, 'utf8');

const m = src.match(/const char\* g_commandFuncNames\[\] = \{([\s\S]*?)\};/);
if (!m) { console.log('[FAIL] 找不到 g_commandFuncNames'); process.exit(1); }
const names = [...m[1].matchAll(/"([A-Za-z0-9_]+)"/g)].map((x) => x[1]);
if (!names.length) { console.log('[FAIL] 名字表为空'); process.exit(1); }

const dup = names.filter((n, i) => names.indexOf(n) !== i);
if (dup.length) { console.log(`[FAIL] 名字表有重复项：${[...new Set(dup)].join(', ')}（静态编译会符号冲突）`); process.exit(1); }

// 组件接口函数：lib.cpp 里以 wv_* 定义，接口表按地址引用。
const itfFuncs = [
    ['wv_CreateUnit', 'HUNIT WINAPI', '(LPBYTE pAllData, INT nAllDataSize, DWORD dwStyle, HWND hParentWnd, UINT uID, HMENU hMenu, INT x, INT y, INT cx, INT cy, DWORD dwWinFormID, DWORD dwUnitID, HWND hDesignWnd, BOOL blInDesignMode)', 'wv::pvi_CreateUnit(pAllData, nAllDataSize, dwStyle, hParentWnd, uID, hMenu, x, y, cx, cy, dwWinFormID, dwUnitID, hDesignWnd, blInDesignMode)'],
    ['wv_PropertyUpdateUI', 'BOOL WINAPI', '(HUNIT hUnit, INT nPropertyIndex)', 'wv::pvi_PropertyUpdateUI(hUnit, nPropertyIndex)'],
    ['wv_NotifyPropertyChanged', 'BOOL WINAPI', '(HUNIT hUnit, INT nPropertyIndex, PUNIT_PROPERTY_VALUE pPropertyValue, LPTSTR* ppszTipText)', 'wv::pvi_NotifyPropertyChanged(hUnit, nPropertyIndex, pPropertyValue, ppszTipText)'],
    ['wv_GetPropertyData', 'BOOL WINAPI', '(HUNIT hUnit, INT nPropertyIndex, PUNIT_PROPERTY_VALUE pPropertyValue)', 'wv::pvi_GetPropertyData(hUnit, nPropertyIndex, pPropertyValue)'],
    ['wv_GetAllPropertyData', 'HGLOBAL WINAPI', '(HUNIT hUnit)', 'wv::pvi_GetAllPropertyData(hUnit)'],
];

const MARK = '// ==== 以下由 gen_static_trampolines.mjs 生成，勿手改 ====';
const block = [
    '}  // namespace wv',
    '',
    MARK,
    '// 静态编译时易语言按名字到 edgewv_static.lib 里找命令实现，',
    '// 而实现在 namespace wv 内，故每个命令生成一份 extern "C" 全局转发。',
    '#define WV_FWD(fn) extern "C" void fn (PMDATA_INF pRetData, INT nArgCount, PMDATA_INF pArgInf) { wv::fn (pRetData, nArgCount, pArgInf); }',
    '',
    ...names.map((n) => `WV_FWD(${n})`),
    '',
    '#undef WV_FWD',
    '',
    'extern "C" INT WINAPI edgewv_ProcessNotifyLib (INT nMsg, DWORD dwParam1, DWORD dwParam2)',
    '{',
    '    return wv::edgewv_ProcessNotifyLib (nMsg, dwParam1, dwParam2);',
    '}',
    '',
    '// 组件接口：静态编译同样按名字挑这些符号，所以要有全局名 + dllexport。',
    // ITF 包装不能被静态编译排除：EPL 静态链接要按名字挑它们
    ...itfFuncs.map(([n, ret, args, call]) => `extern "C" __declspec(dllexport) ${ret} ${n} ${args} { return ${call}; }`),
    '// 运行时按「短名_GetInterface_<组件数据类型英文名>」找符号，两个名字都要有。',
    'extern "C" __declspec(dllexport) PFN_INTERFACE WINAPI edgewv_GetInterface_WebView (INT nInterfaceNO)',
    '{',
    '    return wv::pvi_GetInterface_WebView (nInterfaceNO);',
    '}',
    'extern "C" __declspec(dllexport) PFN_INTERFACE WINAPI edgewv_GetInterface_WebViewBox (INT nInterfaceNO)',
    '{',
    '    return wv::pvi_GetInterface_WebView (nInterfaceNO);',
    '}',
    
    '',
    '// 导出名必须固定为 GetNewInf 且不带修饰，所以靠 .def 导出而不是 __declspec。',
    '#ifndef __E_STATIC_LIB',
    'extern "C" PLIB_INFO WINAPI GetNewInf()',
    '{',
    '    return &wv::g_libraryInfo;',
    '}',
    '',
    'BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID)',
    '{',
    '    if (dwReason == DLL_PROCESS_ATTACH)',
    '    {',
    '        wv::g_hModule = hModule;',
    '        DisableThreadLibraryCalls(hModule);',
    '    }',
    '    else if (dwReason == DLL_PROCESS_DETACH)',
    '    {',
    '        wv::EnvCacheReleaseAll();',
    '    }',
    '    return TRUE;',
    '}',
    '#endif  // !__E_STATIC_LIB',
    '',
].join('\n');

const cut = src.indexOf('}  // namespace wv');
if (cut < 0) { console.log('[FAIL] 找不到 namespace 收尾'); process.exit(1); }
fs.writeFileSync(file, src.slice(0, cut) + block);
console.log(`[OK] 已生成 ${names.length} 个命令转发 + ${itfFuncs.length + 1} 个组件接口转发：${path.basename(file)}`);
