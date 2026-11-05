// 从 WebView2.h 生成「按名反射派发」的 C++：逐条 QueryInterface + 类型安全调用，不用 vtable 序号。
// 用法：node gen_reflect.mjs   →  reflect_gen.cpp + uncovered.txt + 控制台统计
import fs from 'node:fs';

const text = fs.readFileSync('third_party/webview2/include/WebView2.h', 'utf8');

const ifaceRe = /MIDL_INTERFACE\("([0-9a-fA-F\-]+)"\)\s*\n\s*(\w+)\s*:\s*public\s+(\w+)\s*\{([\s\S]*?)\n    \};/g;
const methRe = /virtual\s+(?:\/\*[^*]*\*\/\s*)?HRESULT\s+STDMETHODCALLTYPE\s+(\w+)\s*\(([\s\S]*?)\)\s*=\s*0/g;

// 参数串 → 规范类型名（剥掉参数名、统一指针写法）
function typeOf(arg) {
    let s = arg.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\s+/g, ' ').trim();
    s = s.replace(/\b(const|__RPC__deref_out_opt|__RPC__in|__RPC__out)\b/g, ' ').replace(/\s+/g, ' ').trim();
    s = s.replace(/(\w|\))\s*$/, (m, p1) => (/[A-Za-z0-9_]/.test(p1) ? '' : m));   // 去掉结尾的参数名
    // 上面会把类型最后一个标识符当名字删掉的情况：MIDL 参数总是 "类型 名字"，故按最后一个空格切
    return norm(s);
}
function norm(s) {
    s = s.replace(/\s+/g, ' ').replace(/\s*\*\s*/g, '*').replace(/\*+$/, '*').trim();
    if (s.endsWith('*')) s = s.replace(/\*+$/, '*');
    return s;
}
// 更稳的做法：按 "类型 名字" 切（名字是最后一个标识符，可能前面有 *）
function argType(arg) {
    const s = arg.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\s+/g, ' ').trim();
    const m = s.match(/^(.*?)([A-Za-z_]\w*)$/);
    if (!m) return norm(s);
    let t = m[1].trim();
    if (t === '') return norm(s);            // 只有一个标识符：那就是类型（如 void）
    return norm(t);
}

const ifaces = new Map();
for (const m of text.matchAll(ifaceRe)) {
    const [, , name, base, body] = m;
    const methods = [];
    for (const mm of body.matchAll(methRe)) {
        const args = mm[2].split(',').map(s => s.trim()).filter(s => s && s !== 'void');
        methods.push({ name: mm[1], args: args.map(a => ({ raw: a, type: argType(a) })) });
    }
    ifaces.set(name, { name, base, methods });
}

function chainOf(root) {
    const out = []; let cur = root;
    while (cur && ifaces.has(cur)) {
        out.push(cur);
        const nx = [...ifaces.values()].find(i => i.base === cur);
        cur = nx ? nx.name : null;
        if (out.length > 40) break;
    }
    return out;
}

const TARGETS = [
    ['CORE', chainOf('ICoreWebView2')],
    ['SETTINGS', chainOf('ICoreWebView2Settings')],
    ['CONTROLLER', chainOf('ICoreWebView2Controller')],
    ['ENV', chainOf('ICoreWebView2Environment')],
    ['OPTIONS', ['ICoreWebView2EnvironmentOptions']],
    ['COOKIEMGR', chainOf('ICoreWebView2CookieManager')],
    ['COOKIE', ['ICoreWebView2Cookie']],
    ['PROFILE', chainOf('ICoreWebView2Profile')],
    ['FRAME', chainOf('ICoreWebView2Frame')],
    ['DOWNLOAD', chainOf('ICoreWebView2DownloadOperation')],
    ['REQUEST', ['ICoreWebView2WebResourceRequest']],
    ['RESPONSE', chainOf('ICoreWebView2WebResourceResponse')],
    ['PROCESSINFO', chainOf('ICoreWebView2ProcessInfo')],
    ['RECEIVER', ['ICoreWebView2DevToolsProtocolEventReceiver']],
    ['CERT', ['ICoreWebView2ClientCertificate']],
];

const GET_KIND = {
    'BOOL*': 'bool', 'int*': 'int', 'UINT*': 'int', 'UINT32*': 'int', 'DWORD*': 'int',
    'UINT64*': 'int64', 'double*': 'double', 'FLOAT*': 'double', 'DATE*': 'double',
    'LPWSTR*': 'text', 'PCWSTR*': 'text',
};
const SET_KIND = {
    'BOOL': 'bool', 'int': 'int', 'UINT': 'int', 'UINT32': 'int', 'DWORD': 'int',
    'UINT64': 'int64', 'double': 'double', 'LPCWSTR': 'text', 'PCWSTR': 'text',
};

const entries = [], uncovered = [];
for (const [t, list] of TARGETS) {
    for (const iname of list) {
        const it = ifaces.get(iname); if (!it) continue;
        for (const m of it.methods) {
            const mn = m.name, args = m.args;
            if (mn.startsWith('add_') || mn.startsWith('remove_')) continue;
            const a0 = args[0] ? args[0].type : '';
            if (mn.startsWith('get_')) {
                const k = GET_KIND[a0];
                if (!k) { uncovered.push([t, iname, mn, 'out=' + (a0 || '无')]); continue; }
                entries.push({ t, iface: iname, mn, prop: mn.slice(4), op: 'get', k, vt: a0.replace(/\*$/, '') });
            } else if (mn.startsWith('put_')) {
                const k = SET_KIND[a0];
                if (!k) { uncovered.push([t, iname, mn, 'in=' + (a0 || '无')]); continue; }
                entries.push({ t, iface: iname, mn, prop: mn.slice(4), op: 'set', k, vt: a0 });
            } else if (args.length === 0) {
                entries.push({ t, iface: iname, mn, prop: mn, op: 'call', k: 'void' });
            } else if (args.length === 1 && SET_KIND[a0]) {
                entries.push({ t, iface: iname, mn, prop: mn, op: 'call', k: SET_KIND[a0], vt: a0 });
            } else {
                uncovered.push([t, iname, mn, '参数=' + args.map(a => a.type).join(' , ')]);
            }
        }
    }
}

const dup = new Map();
for (const e of entries) {
    const key = `${e.t}/${e.op}/${e.prop.toLowerCase()}`;
    if (dup.has(key)) console.log('[warn] 同组同名', key, e.iface, 'vs', dup.get(key).iface);
    else dup.set(key, e);
}

const CT = { bool: 'BOOL', int: 'int', int64: 'UINT64', double: 'double', text: 'LPWSTR' };
const QI = e => `${e.iface}* o = nullptr; if (FAILED(pObj->QueryInterface(__uuidof(${e.iface}), (void**)&o))) return RF_ERR;`;
const TEXT_IN = `WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return RF_ERR; `;
const VAL = { bool: 'inInt ? TRUE : FALSE', int: '(int)inInt', int64: '(UINT64)inInt', double: 'inDouble', text: 'wz' };

function line(e) {
    const c = `if (!_stricmp(name, "${e.prop}"))`;
    if (e.op === 'get') {
        if (e.k === 'text')
            return `  ${c} { ${QI(e)} LPWSTR s = nullptr; HRESULT h = o->${e.mn}(&s); o->Release(); if (FAILED(h)) return RF_ERR; int r = ReflPutText(outText, textCap, s); if (s) CoTaskMemFree(s); return r ? RF_OK_TEXT : RF_ERR; }`;
        return `  ${c} { ${QI(e)} ${e.vt} v{}; HRESULT h = o->${e.mn}(&v); o->Release(); if (FAILED(h)) return RF_ERR; *outInt = (INT)v; *outDouble = ${e.k === 'double' ? '(double)v' : '0'}; return RF_OK; }`;
    }
    if (e.op === 'set')
        return `  ${c} { ${e.k === 'text' ? TEXT_IN : ''}${QI(e)} HRESULT h = o->${e.mn}(${VAL[e.k]}); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }`;
    if (e.k === 'void')
        return `  ${c} { ${QI(e)} HRESULT h = o->${e.mn}(); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }`;
    return `  ${c} { ${e.k === 'text' ? TEXT_IN : ''}${QI(e)} HRESULT h = o->${e.mn}((${e.vt})${VAL[e.k]}); o->Release(); return SUCCEEDED(h) ? RF_OK : RF_ERR; }`;
}

const byGroup = {};
for (const e of entries) (byGroup[e.t] ||= []).push(e);

let o = `// 由 gen_reflect.mjs 从 WebView2.h 自动生成，勿手改。\n`;
o += `// 每条显式 QueryInterface(声明接口) → 调用 → Release；签名由编译器校验，不靠 vtable 序号。\n`;
o += `// 不能干净映射成 整数/逻辑/小数/文本 的成员见 uncovered.txt，由专用命令负责。\n\n`;
o += `#include "reflect.h"\n#include <string.h>\n\nnamespace wv {\n\n`;
o += `static int ReflPutText(LPSTR outText, int textCap, LPCWSTR s)\n{\n    if (outText == NULL || textCap <= 0) return 0;\n    outText[0] = '\\0';\n    if (s == NULL || s[0] == L'\\0') return 1;\n    return WideCharToMultiByte(CP_UTF8, 0, s, -1, outText, textCap, NULL, NULL) != 0;\n}\n\n`;

for (const [t] of TARGETS) {
    const list = byGroup[t] || [];
    if (!list.length) {
        // 这组一个可映射成员都没有（只有事件 / 列表访问器），仍要给出空实现，
        // 否则 reflect.h 声明了、lib.cpp 调用了，链接期就是未解析外部符号。
        o += `int ReflGet_${t}(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)
{ UNREF(pObj); UNREF(name); UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap); return RF_MISS; }
`;
        o += `int ReflSet_${t}(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{ UNREF(pObj); UNREF(name); UNREF(inInt); UNREF(inDouble); UNREF(inText); return RF_MISS; }
`;
        o += `int ReflCall_${t}(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)
{ UNREF(pObj); UNREF(name); UNREF(inInt); UNREF(inDouble); UNREF(inText); return RF_MISS; }
`;
        o += `void ReflNames_${t}(char* buf, INT cap)
{ if (buf != NULL && cap > 0) buf[0] = '\0'; UNREF(cap); }

`;
        continue;
    }
    const n = k => list.filter(e => e.op === k).length;
    o += `// ==== ${t}: 读 ${n('get')} / 写 ${n('set')} / 调 ${n('call')} ====\n`;
    o += `int ReflGet_${t}(IUnknown* pObj, LPCSTR name, INT* outInt, DOUBLE* outDouble, LPSTR outText, INT textCap)\n{\n`;
    o += list.filter(e => e.op === 'get').map(line).join('\n');
    o += `\n  UNREF(outInt); UNREF(outDouble); UNREF(outText); UNREF(textCap);\n  return RF_MISS;\n}\n\n`;
    o += `int ReflSet_${t}(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)\n{\n`;
    o += list.filter(e => e.op === 'set').map(line).join('\n');
    o += `\n  UNREF(inInt); UNREF(inDouble); UNREF(inText);\n  return RF_MISS;\n}\n\n`;
    o += `int ReflCall_${t}(IUnknown* pObj, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)\n{\n`;
    o += list.filter(e => e.op === 'call').map(line).join('\n');
    o += `\n  UNREF(inInt); UNREF(inDouble); UNREF(inText);\n  return RF_MISS;\n}\n\n`;
    o += `void ReflNames_${t}(char* buf, INT cap)\n{\n  static const char* items[] = {\n`;
    let col = 0;
    for (const e of list) {
        o += `    "${e.prop}",`;
        o += (++col % 6 === 0) ? '\n' : ' ';
    }
    o += `\n  };\n  ReflJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));\n}\n\n`;
}
o += `}  // namespace wv\n`;

o = o.split(String.fromCharCode(0)).join(String.fromCharCode(92,48));
fs.writeFileSync('reflect_gen.cpp', o);
fs.writeFileSync('uncovered.txt', uncovered.map(u => u.join('\t')).join('\n') + '\n');
console.log('解析接口数:', ifaces.size);
for (const [t] of TARGETS) console.log('  ', t.padEnd(10), '反射条目', (byGroup[t] || []).length);
console.log('反射条目合计:', entries.length, '| 未覆盖(走专用命令):', uncovered.length, '| reflect_gen.cpp 字节', o.length);
