// 从 WebView2.h 生成 reflect_events.cpp：
//   1) 所有 add_/remove_ 事件对 → 「组名 + 事件名」按名订阅；
//   2) 每个事件参数对象 → UTF-8 JSON 快照（嵌套对象递归、列表按 Count + ValueAt 展开）；
//   3) 每个参数对象 → 按属性名读 / 写、取子对象、取列表成员、GetDeferral。
// JSON 追加工具在 wv_json.h（带容量钳位），不在此文件内联。
// 用法：node gen_events.mjs
import fs from 'node:fs';

const text = fs.readFileSync('third_party/webview2/include/WebView2.h', 'utf8');

const ifaceRe = /MIDL_INTERFACE\("([0-9a-fA-F\-]+)"\)\s*\n\s*(\w+)\s*:\s*public\s+(\w+)\s*\{([\s\S]*?)\n    \};/g;
const methRe = /virtual\s+(?:\/\*[^*]*\*\/\s*)?HRESULT\s+STDMETHODCALLTYPE\s+(\w+)\s*\(([\s\S]*?)\)\s*=\s*0/g;

function norm(s) {
    return s.replace(/\s+/g, ' ').replace(/\s*\*\s*/g, '*').replace(/\*+$/, '*').trim();
}
function argType(arg) {
    const s = arg.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\s+/g, ' ').trim();
    const m = s.match(/^(.*?)([A-Za-z_]\w*)$/);
    if (!m) return norm(s);
    const t = m[1].trim();
    return t === '' ? norm(s) : norm(t);
}

const ifaces = new Map();
for (const m of text.matchAll(ifaceRe)) {
    const methods = [];
    for (const mm of m[4].matchAll(methRe)) {
        const args = mm[2].split(',').map(x => x.trim()).filter(x => x && x !== 'void');
        methods.push({ name: mm[1], args: args.map(a => ({ raw: a, type: argType(a) })) });
    }
    ifaces.set(m[2], { name: m[2], iid: m[1], base: m[3], methods });
}

// 本头文件用 `enum X { ... } X;`，没有 typedef
const enumSet = new Set();
for (const m of text.matchAll(/\benum\s+(\w+)\s*\{/g)) enumSet.add(m[1]);
for (const m of text.matchAll(/typedef enum\s+(\w+)/g)) enumSet.add(m[1]);

const SCALAR_INT = new Set(['BOOL', 'int', 'UINT', 'UINT32', 'INT32', 'DWORD', 'UINT64', 'INT64',
    'size_t', 'LONG', 'USHORT', 'WPARAM', 'LPARAM', 'HANDLE', 'HBITMAP', 'HWND', 'COLORREF']);
const SCALAR_DBL = new Set(['double', 'FLOAT', 'DATE']);
const TEXT_OUT = new Set(['LPWSTR', 'PCWSTR', 'WCHAR', 'LPCWSTR']);

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

const GROUPS = [
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
const groupOf = new Map();
for (const [g, list] of GROUPS) for (const i of list) if (!groupOf.has(i)) groupOf.set(i, g);

// ---------- 事件收集 ----------
const events = [];
for (const it of ifaces.values()) {
    for (const a of it.methods.filter(m => m.name.startsWith('add_'))) {
        const evtName = a.name.slice(4);
        if (a.args.length !== 2) continue;
        const hIf = a.args[0].type.replace(/\*$/, '');
        if (!ifaces.has(hIf) || !/EventRegistrationToken\*?$/.test(a.args[1].type)) continue;
        const remover = it.methods.find(m => m.name === 'remove_' + evtName);
        if (!remover) continue;
        const inv = ifaces.get(hIf).methods.find(m => m.name === 'Invoke');
        if (!inv || inv.args.length !== 2) continue;
        const g = groupOf.get(it.name);
        if (!g) continue;
        const senderT = inv.args[0].type.replace(/\*$/, '');
        const argsT = inv.args[1].type.replace(/\*$/, '');
        events.push({ g, decl: it.name, evtName, add: a.name, remove: remover.name, hIf, senderT, argsT,
                      argsReal: argsT !== 'IUnknown' && ifaces.has(argsT) });
    }
}
const seenKey = new Set();
const evList = events.filter(e => {
    const k = `${e.g}/${e.evtName}`;
    if (seenKey.has(k)) return false;
    seenKey.add(k); return true;
}).sort((a, b) => a.g.localeCompare(b.g) || a.evtName.localeCompare(b.evtName));

// ---------- 要快照 / 派发的对象 ----------
const SCALAR_KIND = t => {
    const b = t.replace(/\*+$/, '');
    if (TEXT_OUT.has(b)) return 'text';
    if (SCALAR_INT.has(b) || enumSet.has(b)) return 'int';
    if (SCALAR_DBL.has(b)) return 'double';
    return null;
};
const OBJ_OUT = t => { const b = t.replace(/\*$|\*\*$/, ''); return ifaces.has(b) ? b : null; };

const EXTRA_DESCRIBE = [
    'ICoreWebView2ProcessInfoCollection', 'ICoreWebView2CookieList', 'ICoreWebView2ClientCertificateCollection',
    'ICoreWebView2ProcessExtendedInfoCollection', 'ICoreWebView2FrameInfoCollection', 'ICoreWebView2BrowserExtensionList',
    'ICoreWebView2Cookie', 'ICoreWebView2Profile', 'ICoreWebView2DownloadOperation',
    'ICoreWebView2Frame', 'ICoreWebView2WebResourceResponse', 'ICoreWebView2WebResourceRequest',
    'ICoreWebView2BrowserExtension', 'ICoreWebView2ClientCertificate', 'ICoreWebView2ProcessInfo',
];
const needDesc = new Set();
for (const e of evList) if (e.argsReal) needDesc.add(e.argsT);
for (const n of EXTRA_DESCRIBE) if (ifaces.has(n)) needDesc.add(n);

function hasReadable(name) {
    const it = ifaces.get(name); if (!it) return false;
    for (const m of it.methods) {
        if (!m.name.startsWith('get_')) continue;
        if (m.args.length === 1 && (SCALAR_KIND(m.args[0].type) || OBJ_OUT(m.args[0].type))) return true;
        if (m.args.length === 2 && /^(int|UINT|UINT32)\*?$/.test(m.args[0].type) && OBJ_OUT(m.args[1].type)) return true;
    }
    return false;
}
{
    let grow = true, guard = 0;
    while (grow && guard++ < 10) {
        grow = false;
        for (const name of [...needDesc]) {
            const it = ifaces.get(name); if (!it) continue;
            for (const m of it.methods) {
                if (!m.name.startsWith('get_') || m.args.length !== 1) continue;
                const o = OBJ_OUT(m.args[0].type);
                if (o && ifaces.has(o) && hasReadable(o) && !needDesc.has(o)) { needDesc.add(o); grow = true; }
            }
        }
    }
}

const descName = i => `Desc_${i}`;

// ---------- JSON 快照 ----------
function emitDesc(ifaceName) {
    const it = ifaces.get(ifaceName);
    const getters = it.methods.filter(m => m.name.startsWith('get_'));
    let s = `static void ${descName(ifaceName)}(${ifaceName}* p, char* buf, int cap, int* pn)\n{\n`;
    s += `    if (p == NULL || buf == NULL || cap <= 0 || *pn >= cap - 1) return;\n`;
    s += `    if (!JEnter()) return;\n`;
    s += `    J_Open(buf, cap, pn, '{');\n`;
    const done = new Set();
    for (const m of getters) {
        const prop = m.name.slice(4);
        if (done.has(prop) || m.args.length === 0) continue;
        done.add(prop);
        const a0 = m.args[0].type;
        const sk = SCALAR_KIND(a0);
        if (sk === 'text' && m.args.length === 1)
            s += `    { LPWSTR v = nullptr; if (p->${m.name}(&v) == S_OK) { J_StrW(buf, cap, pn, "${prop}", v); CoTaskMemFree(v); } }\n`;
        else if (sk === 'int' && m.args.length === 1) {
            const b = a0.replace(/\*+$/, '');
            s += `    { ${b} v{}; if (p->${m.name}(&v) == S_OK) J_Num(buf, cap, pn, "${prop}", (double)(long)v, FALSE); }\n`;
        } else if (sk === 'double' && m.args.length === 1) {
            const b = a0.replace(/\*+$/, '');
            s += `    { ${b} v{}; if (p->${m.name}(&v) == S_OK) J_Num(buf, cap, pn, "${prop}", (double)v, TRUE); }\n`;
        } else if (m.args.length === 1 && OBJ_OUT(a0)) {
            const oi = OBJ_OUT(a0);
            if (needDesc.has(oi) && oi !== ifaceName)
                s += `    { ${oi}* v = nullptr; if (p->${m.name}(&v) == S_OK) { J_Key(buf, cap, pn, "${prop}"); ${descName(oi)}(v, buf, cap, pn); v->Release(); } }\n`;
        }
    }
    const cntGetter = getters.find(g => /Count$/.test(g.name) && g.args.length === 1
        && /^(int|UINT|UINT32)\*?$/.test(g.args[0].type));
    if (cntGetter) {
        for (const m of it.methods) {
            if (m.args.length !== 2 || !/^(int|UINT|UINT32)\*?$/.test(m.args[0].type)) continue;
            const oi = OBJ_OUT(m.args[1].type);
            if (!oi || !needDesc.has(oi)) continue;
            const cntT = cntGetter.args[0].type.replace(/\*+$/, '') || 'int';
            s += `    { ${cntT} nRaw{}; if (p->${cntGetter.name}(&nRaw) == S_OK) { J_Key(buf, cap, pn, "Items"); J_Open(buf, cap, pn, '[');\n`;
            s += `        for (int i = 0; i < (int)nRaw && i < 32; i++) { ${oi}* v = nullptr; if (p->${m.name}(i, &v) == S_OK) { ${descName(oi)}(v, buf, cap, pn); v->Release(); } }\n`;
            s += `        J_Close(buf, cap, pn, ']'); } }\n`;
            break;
        }
    }
    s += `    J_Close(buf, cap, pn, '}');\n    JLeave();\n}\n\n`;
    return s;
}

// ---------- 属性派发 ----------
function emitProps(ifaceName) {
    const it = ifaces.get(ifaceName);
    const getters = it.methods.filter(m => m.name.startsWith('get_') && m.args.length === 1);
    const done = new Set();
    const names = [];

    let g = `static int GetP_${ifaceName}(void* pV, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)\n{\n`;
    g += `    ${ifaceName}* p = (${ifaceName}*)pV; UNREF(outInt); UNREF(pDbl);\n`;
    let c = `static int Child_${ifaceName}(void* pV, LPCSTR name, void** ppChild, LPCSTR* ppIface)\n{\n`;
    c += `    ${ifaceName}* p = (${ifaceName}*)pV; UNREF(ppChild); UNREF(ppIface);\n`;

    for (const m of getters) {
        const prop = m.name.slice(4);
        if (done.has(prop)) continue;
        done.add(prop);
        const a0 = m.args[0].type;
        const sk = SCALAR_KIND(a0);
        if (sk === 'text') {
            names.push(prop);
            g += `    if (!_stricmp(name, "${prop}")) { LPWSTR v = nullptr; if (FAILED(p->${m.name}(&v))) return 0; int r = JPutText(outText, cap, v); if (v) CoTaskMemFree(v); return r ? 2 : 0; }\n`;
        } else if (sk === 'double') {
            const b = a0.replace(/\*+$/, '');
            names.push(prop);
            g += `    if (!_stricmp(name, "${prop}")) { ${b} v{}; if (FAILED(p->${m.name}(&v))) return 0; if (pDbl) *pDbl = (double)v; if (outInt) *outInt = (INT)v; return 1; }\n`;
        } else if (sk === 'int') {
            const b = a0.replace(/\*+$/, '');
            names.push(prop);
            g += `    if (!_stricmp(name, "${prop}")) { ${b} v{}; if (FAILED(p->${m.name}(&v))) return 0; if (outInt) *outInt = (INT)(INT64)v; return 1; }\n`;
        } else {
            const oi = OBJ_OUT(a0);
            if (oi && needDesc.has(oi) && oi !== ifaceName) {
                names.push(prop);
                c += `    if (!_stricmp(name, "${prop}")) { ${oi}* v = nullptr; if (FAILED(p->${m.name}(&v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "${oi}"; return 1; }\n`;
            }
        }
    }
    g += `    return 0;\n}\n\n`;
    c += `    return 0;\n}\n\n`;

    const setters = it.methods.filter(m => m.name.startsWith('put_') && m.args.length === 1);
    let s2 = `static int SetP_${ifaceName}(void* pV, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)\n{\n`;
    s2 += `    ${ifaceName}* p = (${ifaceName}*)pV; UNREF(inInt); UNREF(inDouble); UNREF(inText);\n`;
    for (const m of setters) {
        const prop = m.name.slice(4);
        const a0 = m.args[0].type;
        const sk = SCALAR_KIND(a0);
        if (!sk || done.has('set:' + prop)) continue;
        done.add('set:' + prop);
        const pre = sk === 'text' ? `WCHAR wz[8192]; if (MultiByteToWideChar(CP_UTF8, 0, inText ? inText : "", -1, wz, 8192) == 0) return 0; ` : '';
        const val = sk === 'text' ? `(LPCWSTR)wz` : sk === 'double' ? `(${a0})inDouble`
                  : sk === 'bool' ? `(${a0})(inInt ? TRUE : FALSE)` : `(${a0})inInt`;
        s2 += `    if (!_stricmp(name, "${prop}")) { ${pre}return SUCCEEDED(p->${m.name}(${val})) ? 1 : 0; }\n`;
    }
    s2 += `    return 0;\n}\n\n`;

    const def = it.methods.find(m => m.name === 'GetDeferral' && m.args.length === 1);
    let d2 = `static int Deferral_${ifaceName}(void* pV, void** ppOut)\n{\n`;
    d2 += def
        ? `    ${ifaceName}* p = (${ifaceName}*)pV; UNREF(ppOut);\n    if (ppOut == NULL) return 0; ICoreWebView2Deferral* v = nullptr;\n`
        + `    if (FAILED(p->GetDeferral(&v)) || v == nullptr) return 0; *ppOut = (void*)v; return 1;\n}\n\n`
        : `    UNREF(pV); UNREF(ppOut); return 0;\n}\n\n`;

    const cntGetter = getters.find(x => /Count$/.test(x.name) && /^(int|UINT|UINT32)\*?$/.test(x.args[0].type));
    const atGetter = it.methods.find(m => m.args.length === 2 && /^(int|UINT|UINT32)\*?$/.test(m.args[0].type)
        && OBJ_OUT(m.args[1].type) && needDesc.has(OBJ_OUT(m.args[1].type)));
    let at, lc;
    if (cntGetter && atGetter) {
        const oi = OBJ_OUT(atGetter.args[1].type);
        const idxT = atGetter.args[0].type.replace(/\*+$/, '') || 'int';
        const cntT = cntGetter.args[0].type.replace(/\*+$/, '') || 'int';
        at = `static int ChildAt_${ifaceName}(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)\n{\n`
           + `    ${ifaceName}* p = (${ifaceName}*)pV; UNREF(ppChild); UNREF(ppIface);\n`
           + `    if (!_stricmp(name, "Items") || !_stricmp(name, "ValueAtIndex")) { ${oi}* v = nullptr; `
           + `if (FAILED(p->${atGetter.name}((${idxT})index, &v)) || v == nullptr) return 0; *ppChild = (void*)v; *ppIface = "${oi}"; return 1; }\n`
           + `    return 0;\n}\n\n`;
        lc = `static int ListCount_${ifaceName}(void* pV, LPCSTR name, INT* pCount)\n{\n`
           + `    ${ifaceName}* p = (${ifaceName}*)pV; UNREF(name);\n`
           + `    if (!pCount) return 0; ${cntT} n{}; if (FAILED(p->${cntGetter.name}(&n))) return 0; *pCount = (INT)n; return 1;\n}\n\n`;
        names.push('Items');
    } else {
        at = `static int ChildAt_${ifaceName}(void* pV, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)\n{ UNREF(pV); UNREF(name); UNREF(index); UNREF(ppChild); UNREF(ppIface); return 0; }\n\n`;
        lc = `static int ListCount_${ifaceName}(void* pV, LPCSTR name, INT* pCount)\n{ UNREF(pV); UNREF(name); UNREF(pCount); return 0; }\n\n`;
    }

    if (!names.length)
        return g + c + s2 + d2 + at + lc
             + `static void Names_${ifaceName}(char* buf, INT cap)\n{ UNREF(buf); UNREF(cap); if (buf != NULL && cap > 0) buf[0] = '\\0'; }\n\n`;

    let nm = `static void Names_${ifaceName}(char* buf, INT cap)\n{\n    static const char* items[] = {\n`;
    let col = 0;
    for (const n of names) { nm += `        "${n}",`; nm += ((++col % 6) === 0) ? '\n' : ' '; }
    nm += `\n    };\n    WvJoinNames(buf, cap, items, (INT)(sizeof(items) / sizeof(items[0])));\n}\n\n`;
    return g + c + s2 + d2 + at + lc + nm;
}

// ---------- 输出 ----------
let o = '';
o += `// 由 gen_events.mjs 从 WebView2.h 自动生成，勿手改。\n`;
o += `// 事件按名订阅 + 参数 JSON 快照 + 参数按名读写 / 子对象 / 列表 / 延后。\n\n`;
o += `#include "wv_pre.h"\n#include "reflect_events.h"\n#include "wv_json.h"\n#include <string.h>\n#include <stdio.h>\n\n`;
o += `namespace wv {\n\n`;
o += `static void WvJoinNames(char* buf, INT cap, const char** items, INT count)\n`;
o += `{ if (buf == NULL || cap <= 0) return; buf[0] = '\\0'; INT used = 0;\n`;
o += `  for (INT i = 0; i < count; i++) { INT n = (INT)lstrlenA(items[i]); if (used + n + 2 > cap) break;\n`;
o += `    lstrcpyA(buf + used, items[i]); used += n; buf[used++] = '\\n'; buf[used] = '\\0'; } }\n\n`;

const depsOf = n => {
    const it = ifaces.get(n); const out = [];
    if (!it) return out;
    for (const m of it.methods) {
        if (m.args.length === 1 && m.name.startsWith('get_')) { const x = OBJ_OUT(m.args[0].type); if (x) out.push(x); }
        else if (m.args.length === 2) { const x = OBJ_OUT(m.args[1].type); if (x) out.push(x); }
    }
    return out;
};
const depthOf = (n, seen) => {
    if (seen.has(n)) return 0;
    seen.add(n);
    let d = 0;
    for (const x of depsOf(n)) if (needDesc.has(x) && x !== n) d = Math.max(d, depthOf(x, seen) + 1);
    return Math.min(d, 6);
};
const ordered = [...needDesc].sort((a, b) => depthOf(a, new Set()) - depthOf(b, new Set()));

for (const n of ordered) o += emitDesc(n);

o += `struct WvDescItem { LPCSTR m_szIface; void (*m_pfn)(void*, char*, int, int*); };\n`;
let table = `static const WvDescItem g_descItems[] = {\n`;
for (const n of ordered) {
    o += `static void CDesc_${n}(void* p, char* buf, int cap, int* pn) { ${descName(n)}((${n}*)p, buf, cap, pn); }\n`;
    table += `    { "${n}", CDesc_${n} },\n`;
}
o += table + `};\n\n`;
o += `BOOL WvDescribeObject(LPCSTR ifaceName, void* pObj, char* buf, INT cap)\n{\n`;
o += `    if (ifaceName == NULL || pObj == NULL || buf == NULL || cap <= 0) return FALSE;\n    buf[0] = '\\0';\n`;
o += `    for (INT i = 0; i < (INT)(sizeof(g_descItems) / sizeof(g_descItems[0])); i++)\n`;
o += `        if (lstrcmpA(g_descItems[i].m_szIface, ifaceName) == 0) { INT n = 0; g_descItems[i].m_pfn(pObj, buf, cap, &n); return TRUE; }\n`;
o += `    return FALSE;\n}\n\n`;

for (const n of ordered) o += emitProps(n);

o += `struct WvPropItem { LPCSTR m_szIface;\n`;
o += `    int (*m_pfnGet)(void*, LPCSTR, INT*, DOUBLE*, LPSTR, INT);\n`;
o += `    int (*m_pfnChild)(void*, LPCSTR, void**, LPCSTR*);\n`;
o += `    int (*m_pfnChildAt)(void*, LPCSTR, INT, void**, LPCSTR*);\n`;
o += `    int (*m_pfnListCount)(void*, LPCSTR, INT*);\n`;
o += `    int (*m_pfnSet)(void*, LPCSTR, INT, DOUBLE, LPCSTR);\n`;
o += `    int (*m_pfnDeferral)(void*, void**);\n`;
o += `    void (*m_pfnNames)(char*, INT); };\n`;
o += `static const WvPropItem g_propItems[] = {\n`;
for (const n of ordered)
    o += `    { "${n}", GetP_${n}, Child_${n}, ChildAt_${n}, ListCount_${n}, SetP_${n}, Deferral_${n}, Names_${n} },\n`;
o += `};\n\n`;
o += `static const WvPropItem* WvFindProp(LPCSTR iface)\n{\n    if (iface == NULL) return NULL;\n`;
o += `    for (INT i = 0; i < (INT)(sizeof(g_propItems) / sizeof(g_propItems[0])); i++)\n`;
o += `        if (lstrcmpA(g_propItems[i].m_szIface, iface) == 0) return &g_propItems[i];\n    return NULL;\n}\n\n`;
o += `BOOL WvObjKnown(LPCSTR iface) { return WvFindProp(iface) != NULL ? TRUE : FALSE; }\n`;
o += `INT WvObjPropGet(LPCSTR iface, void* p, LPCSTR name, INT* outInt, DOUBLE* pDbl, LPSTR outText, INT cap)\n`;
o += `{ const WvPropItem* it = WvFindProp(iface); if (it == NULL || p == NULL || name == NULL) return 0; return it->m_pfnGet(p, name, outInt, pDbl, outText, cap); }\n`;
o += `INT WvObjPropChild(LPCSTR iface, void* p, LPCSTR name, void** ppChild, LPCSTR* ppIface)\n`;
o += `{ const WvPropItem* it = WvFindProp(iface); if (it == NULL || p == NULL || name == NULL) return 0; return it->m_pfnChild(p, name, ppChild, ppIface); }\n`;
o += `INT WvObjPropChildAt(LPCSTR iface, void* p, LPCSTR name, INT index, void** ppChild, LPCSTR* ppIface)\n`;
o += `{ const WvPropItem* it = WvFindProp(iface); if (it == NULL || p == NULL || name == NULL) return 0; return it->m_pfnChildAt(p, name, index, ppChild, ppIface); }\n`;
o += `INT WvObjListCount(LPCSTR iface, void* p, LPCSTR name, INT* pCount)\n`;
o += `{ const WvPropItem* it = WvFindProp(iface); if (it == NULL || p == NULL) return 0; return it->m_pfnListCount(p, name, pCount); }\n`;
o += `INT WvObjPropSet(LPCSTR iface, void* p, LPCSTR name, INT inInt, DOUBLE inDouble, LPCSTR inText)\n`;
o += `{ const WvPropItem* it = WvFindProp(iface); if (it == NULL || p == NULL || name == NULL) return 0; return it->m_pfnSet(p, name, inInt, inDouble, inText); }\n`;
o += `INT WvObjDeferral(LPCSTR iface, void* p, void** ppOut)\n`;
o += `{ const WvPropItem* it = WvFindProp(iface); if (it == NULL || p == NULL) return 0; return it->m_pfnDeferral(p, ppOut); }\n`;
o += `void WvObjPropNames(LPCSTR iface, char* buf, INT cap)\n`;
o += `{ if (buf != NULL && cap > 0) buf[0] = '\\0'; const WvPropItem* it = WvFindProp(iface); if (it == NULL) return; it->m_pfnNames(buf, cap); }\n\n`;

o += `struct WvEvtBind { void* m_pCtx; WvEventFn m_fn; };\n\n`;
const subFns = [];
for (const e of evList) {
    const cls = `WvEv_${e.g}_${e.evtName}`;
    o += `class ${cls} : public ${e.hIf}\n{\npublic:\n`;
    o += `    ${cls}(WvEvtBind b) : m_ref(1), m_bind(b) {}\n    virtual ~${cls}() {}\n`;
    o += `    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) { if (ppv == NULL) return E_POINTER;\n`;
    o += `        if (riid == __uuidof(IUnknown) || riid == __uuidof(${e.hIf})) *ppv = (${e.hIf}*)this;\n`;
    o += `        else { *ppv = NULL; return E_NOINTERFACE; } AddRef(); return S_OK; }\n`;
    o += `    STDMETHODIMP_(ULONG) AddRef() { return (ULONG)InterlockedIncrement(&m_ref); }\n`;
    o += `    STDMETHODIMP_(ULONG) Release() { const ULONG n = (ULONG)InterlockedDecrement(&m_ref); if (n == 0) delete this; return n; }\n`;
    o += `    STDMETHODIMP Invoke(${e.senderT}* sender, ${e.argsT}* args)\n    {\n        UNREF(sender);\n`;
    o += `        char json[WV_EVT_JSON_MAX]; json[0] = '\\0';\n`;
    if (e.argsReal) o += `        { int n = 0; ${descName(e.argsT)}(args, json, (int)sizeof(json), &n); }\n`;
    o += `        if (m_bind.m_fn != NULL) m_bind.m_fn(m_bind.m_pCtx, "${e.evtName}", json, (void*)args);\n        return S_OK;\n    }\n`;
    o += `private:\n    LONG m_ref; WvEvtBind m_bind;\n};\n\n`;
    subFns.push(`static BOOL Sub_${e.decl}_${e.evtName}(IUnknown* pTarget, WvEvtBind bind, INT64* pToken)\n{\n`
        + `    if (pTarget == NULL) return FALSE;\n`
        + `    ${e.decl}* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(${e.decl}), (void**)&o))) return FALSE;\n`
        + `    ${cls}* h = new ${cls}(bind); EventRegistrationToken tk; memset(&tk, 0, sizeof(tk));\n`
        + `    const HRESULT hr = o->${e.add}(h, &tk); h->Release(); o->Release();\n`
        + `    if (FAILED(hr) || pToken == NULL) return FALSE;\n`
        + `    memcpy(pToken, &tk, sizeof(*pToken) < sizeof(tk) ? sizeof(*pToken) : sizeof(tk)); return TRUE;\n}\n`
        + `static BOOL Unsub_${e.decl}_${e.evtName}(IUnknown* pTarget, INT64 token)\n{\n`
        + `    if (pTarget == NULL) return FALSE;\n`
        + `    ${e.decl}* o = nullptr; if (FAILED(pTarget->QueryInterface(__uuidof(${e.decl}), (void**)&o))) return FALSE;\n`
        + `    EventRegistrationToken tk; memset(&tk, 0, sizeof(tk)); memcpy(&tk, &token, sizeof(tk) < sizeof(token) ? sizeof(tk) : sizeof(token));\n`
        + `    const HRESULT hr = o->${e.remove}(tk); o->Release();\n    return SUCCEEDED(hr) ? TRUE : FALSE;\n}\n`);
}
o += subFns.join('\n') + '\n';

const byGroup = {};
for (const e of evList) (byGroup[e.g] ||= []).push(e);
o += `struct WvEvtItem { LPCSTR m_szName; BOOL (*m_pfnSub)(IUnknown*, WvEvtBind, INT64*); BOOL (*m_pfnUnsub)(IUnknown*, INT64); };\n\n`;
for (const [g] of GROUPS) {
    const list = byGroup[g]; if (!list || !list.length) continue;
    o += `static const WvEvtItem g_evt${g}[] = {\n`;
    for (const e of list) o += `    { "${e.evtName}", Sub_${e.decl}_${e.evtName}, Unsub_${e.decl}_${e.evtName} },\n`;
    o += `};\n\n`;
}
o += `struct WvEvtGroup { LPCSTR m_szGroup; const WvEvtItem* m_pItems; INT m_nCount; };\n`;
o += `static const WvEvtGroup g_evtGroups[] = {\n`;
for (const [g] of GROUPS) {
    const list = byGroup[g]; if (!list || !list.length) continue;
    o += `    { "${g}", g_evt${g}, (INT)(sizeof(g_evt${g}) / sizeof(g_evt${g}[0])) },\n`;
}
o += `};\n\n`;
o += `static INT WvEvtGroupIndexOf(LPCSTR group)\n{\n    if (group == NULL) return -1;\n`;
o += `    for (INT i = 0; i < (INT)(sizeof(g_evtGroups) / sizeof(g_evtGroups[0])); i++)\n`;
o += `        if (_stricmp(g_evtGroups[i].m_szGroup, group) == 0) return i;\n    return -1;\n}\n\n`;
o += `BOOL WvEvtSubscribe(IUnknown* pTarget, LPCSTR group, LPCSTR evtName, void* pCtx, WvEventFn fn, INT64* pToken)\n{\n`;
o += `    const INT gi = WvEvtGroupIndexOf(group);\n    if (gi < 0 || evtName == NULL || fn == NULL || pToken == NULL) return FALSE;\n`;
o += `    for (INT i = 0; i < g_evtGroups[gi].m_nCount; i++)\n        if (_stricmp(g_evtGroups[gi].m_pItems[i].m_szName, evtName) == 0)\n`;
o += `            return g_evtGroups[gi].m_pItems[i].m_pfnSub(pTarget, WvEvtBind{ pCtx, fn }, pToken);\n    return FALSE;\n}\n\n`;
o += `BOOL WvEvtUnsubscribe(IUnknown* pTarget, LPCSTR group, LPCSTR evtName, INT64 token)\n{\n`;
o += `    const INT gi = WvEvtGroupIndexOf(group);\n    if (gi < 0 || evtName == NULL) return FALSE;\n`;
o += `    for (INT i = 0; i < g_evtGroups[gi].m_nCount; i++)\n        if (_stricmp(g_evtGroups[gi].m_pItems[i].m_szName, evtName) == 0)\n`;
o += `            return g_evtGroups[gi].m_pItems[i].m_pfnUnsub(pTarget, token);\n    return FALSE;\n}\n\n`;
o += `void WvEvtNames(LPCSTR group, char* buf, INT cap)\n{\n    if (buf == NULL || cap <= 0) return; buf[0] = '\\0';\n`;
o += `    const INT gi = WvEvtGroupIndexOf(group); if (gi < 0) return; INT n = 0;\n`;
o += `    for (INT i = 0; i < g_evtGroups[gi].m_nCount; i++)\n`;
o += `        n += snprintf(buf + n, (size_t)(cap - n - 1), "%s%s", n ? "\\n" : "", g_evtGroups[gi].m_pItems[i].m_szName);\n`;
o += `    if (n > cap - 1) n = cap - 1;\n}\n\n`;
o += `INT WvEvtGroupCount() { return (INT)(sizeof(g_evtGroups) / sizeof(g_evtGroups[0])); }\n`;
o += `LPCSTR WvEvtGroupName(INT i) { return (i >= 0 && i < WvEvtGroupCount()) ? g_evtGroups[i].m_szGroup : NULL; }\n\n`;
o += `static const struct { LPCSTR m_szEvt; LPCSTR m_szIface; } g_evtIface[] = {\n`;
for (const e of evList) o += `    { "${e.evtName}", "${e.argsReal ? e.argsT : ''}" },\n`;
o += `};\n`;
o += `LPCSTR WvEvtArgsIface(LPCSTR evtName)\n{\n    if (evtName == NULL) return NULL;\n`;
o += `    for (INT i = 0; i < (INT)(sizeof(g_evtIface) / sizeof(g_evtIface[0])); i++)\n`;
o += `        if (_stricmp(g_evtIface[i].m_szEvt, evtName) == 0) return (g_evtIface[i].m_szIface[0] != '\\0') ? g_evtIface[i].m_szIface : NULL;\n`;
o += `    return NULL;\n}\n\n`;
o += `}  // namespace wv\n`;

fs.writeFileSync('reflect_events.cpp', o);
console.log('接口', ifaces.size, '| 事件', evList.length, '| 快照对象', needDesc.size, '| 字节', o.length);
for (const [g] of GROUPS) if (byGroup[g]) console.log('  ', g.padEnd(12), byGroup[g].length);
