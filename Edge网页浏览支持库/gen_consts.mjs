// 从 WebView2.h 生成「库常量表」：内核的全部枚举值 + 数值宏，落成 LIB_CONST_INFO 数组。
// 用法：node gen_consts.mjs   →  wv_consts.h（由 lib.cpp 在 #ifndef __E_STATIC_LIB 内 include）
import fs from 'node:fs';

const text = fs.readFileSync('third_party/webview2/include/WebView2.h', 'utf8');

const rows = [];
const seen = new Set();

// enum COREWEBVIEW2_X { A = 0, B = 1, ... }  COREWEBVIEW2_X;
const enumRe = /\benum\s+(COREWEBVIEW2_\w+)\s*\{([\s\S]*?)\}\s*\1\s*;/g;
const CN = {
    Color: '颜色', CookieSameSiteKind: 'Cookie 同站策略',
};

function push(name, value, explain, group) {
    if (!name || seen.has(name)) return;
    seen.add(name);
    rows.push({ name, value, explain, group });
}

for (const m of text.matchAll(enumRe)) {
    const enumName = m[1];
    const short = enumName.replace(/^COREWEBVIEW2_/, '');
    const body = m[2];
    for (const v of body.matchAll(/(\w+)\s*=\s*(-?0x[0-9a-fA-F]+|-?\d+)/g)) {
        const val = parseInt(v[2], 16) || parseInt(v[2], 10) || 0;
        push(v[1], val, `${shortCn(short)}：${v[1].replace(enumName + '_', '').replace(/_/g, ' ').toLowerCase()}（内核枚举 ${enumName}）。`, short);
    }
}

// 数值宏（COREWEBVIEW2_INVALID_COOKIE_TIME 之类）
for (const m of text.matchAll(/^#define\s+(COREWEBVIEW2_\w+)\s+(-?0x[0-9a-fA-F]+|-?\d+)\s*$/gm)) {
    const val = m[2].startsWith('0x') ? parseInt(m[2], 16) : parseInt(m[2], 10);
    push(m[1], val, `内核数值常量 ${m[1]} = ${m[2]}。`, '数值常量');
}

function shortCn(s) {
    const map = {
        PermissionState: '权限应答状态', PermissionKind: '权限种类',
        CapturePreviewImageFormat: '截图格式', PrintDialogKind: '打印界面种类',
        PrintStatus: '打印结果', SaveAsUIShowingKind: '另存为种类',
        SaveAsUIResult: '另存为结果', ServerCertificateErrorAction: '证书错误处置',
        CookieSameSiteKind: 'Cookie 同站策略', HostResourceAccessKind: '宿主资源访问级别',
        WebErrorStatus: '网页错误码', WebResourceContext: '资源类型',
        ProcessFailedKind: '进程失败种类', ProcessFailedReason: '进程失败原因',
        MouseEventKind: '鼠标事件种类', KeyEventKind: '按键事件种类',
        KeyEventType: '按键状态类型', PhysicalKeyStatus: '物理按键状态',
        TextureState: '纹理状态', Color: '颜色',
        NavigationKind: '跳转种类', NewWindowReplacementContext: '新窗口替换上下文',
        PreferredTrackingPreventionLevel: '跟踪防护级别',
        PreferredColorScheme: '配色方案', MemoryUsageTargetLevel: '内存占用级别',
        DownloadDialogCornerAlignment: '下载气泡角位', FrameKind: '帧种类',
        Cursor: '光标形状', ContextMenuKind: '菜单项种类', ContextMenuTarget: '菜单目标',
        BrowserProcessExitedKind: '浏览器进程退出种类',
        ClientCertificateKind: '客户端证书种类', Favicon: '图标',
        ContentDialogKind: '内容对话框种类',
    };
    return map[s] || s;
}

rows.forEach(r => { r.explain = r.explain.replace('shortCn(short)', shortCn(r.group)); });

let o = '';
o += '// 由 gen_consts.mjs 从 WebView2.h 自动生成，勿手改：内核枚举与数值常量，共 ' + rows.length + ' 条。\n';
o += '// 只在动态库产物里用（静态编译不需要常量表），须在 namespace wv 内 include。\n\n';
o += 'static LIB_CONST_INFO g_wvConsts[] = {\n';
for (const r of rows) {
    const explain = r.explain.replace(/\\/g, '\\\\').replace(/"/g, '\\"');
    o += `    { _T("${r.name}"), _T("${r.name}"), _T("${explain}"), _CONST_OS(__OS_WIN), CT_NUM, NULL, ${r.value} },\n`;
}
o += '};\n';
o += '#define WV_CONST_COUNT ((INT)(sizeof(g_wvConsts) / sizeof(g_wvConsts[0])))\n';

fs.writeFileSync('wv_consts.h', o);
console.log('常量', rows.length, '枚举组', new Set(rows.map(r => r.group)).size);
