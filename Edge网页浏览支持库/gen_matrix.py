# -*- coding: utf-8 -*-
"""汇总三层的对接面，产出 覆盖矩阵.md：命名命令 / 反射按名 / 事件按名订阅，以及 uncovered 的去向。"""
import io
import re

lib = io.open('lib.cpp', encoding='utf-8').read()
gen = io.open('reflect_gen.cpp', encoding='utf-8').read()
ev = io.open('reflect_events.cpp', encoding='utf-8').read()

# 1) 命名命令
tbl = lib[lib.index('CMD_INFO g_commands[] = {'):]
tbl = tbl[:tbl.index('\n};')]
cmds = []
for m in re.finditer(r'WV_(CMD|OBJ)\("([^"]*)",\s*"([^"]*)"', tbl):
    kind = '全局' if m.group(1) == 'CMD' else '成员'
    st = tbl[m.end():m.end() + 260]
    if m.group(1) == 'OBJ' and 'CT_IS_HIDED' in st:
        kind = '生命周期'
    cmds.append((m.group(2), m.group(3), kind))

# 2) 反射按名成员
refl = {}
for m in re.finditer(r'if \(!_stricmp\(name, "([^"]+)"\)\)', gen):
    refl.setdefault(m.group(1), 0)
    refl[m.group(1)] += 1
groups = re.findall(r'// ==== (\w+): 读 (\d+) / 写 (\d+) / 调 (\d+) ====', gen)

# 3) 事件
evt = re.findall(r'\{ "(\w+)", Sub_(\w+)_(\w+), Unsub_\2_\3 \}', ev)
evt_groups = {}
for name, decl, evt_name in evt:
    evt_groups.setdefault(name, []).append(decl)

described = re.findall(r'static void Desc_(I\w+)\(', ev)

# 4) uncovered 去向
named = set(c[1].lower() for c in cmds)
# 命名命令的英文名不必与 SDK 成员同名：这里补一张对照，避免矩阵把已接的说成待补。
ALIAS = {
    'AddScriptToExecuteOnDocumentCreated': 'AddInitScript',
    'RemoveScriptToExecuteOnDocumentCreated': 'RemoveInitScript',
    'PostWebMessageAsString': 'PostWebMessage',
    'PostWebMessageAsJson': 'PostWebMessageJson',
    'SetVirtualHostNameToFolderMapping': 'SetVirtualHostNameMapping',
    'AddWebResourceRequestedFilter': 'AddWebResourceRequestFilter',
    'GetCookies': 'GetCookieList',
    'AddOrUpdateCookie': 'SetCookieValue',
    'DeleteCookiesWithDomainAndPath': 'DeleteCookieAll',
    'ClearBrowsingData': 'ClearBrowsingDataInTime',
    'ClearBrowsingDataAll': 'ClearBrowsingDataAll',
    'GetBrowserExtensions': 'GetBrowserExtensions',
    'AddBrowserExtension': 'AddBrowserExtension',
    'GetProcessInfos': 'GetProcessInfos',
    'CallDevToolsProtocolMethod': 'CallDevToolsProtocol',
    'GetDevToolsProtocolEventReceiver': 'SubscribeDevToolsEvent',
    'TrySuspend': 'TrySuspendPage',
    'ShowSaveAsUI': 'ShowSaveAsUI',
    'CompareBrowserVersions': 'CompareWebViewVersions',
    'GetAvailableCoreWebView2BrowserVersionString': 'GetAvailableWebViewVersion',
    'CreateWebResourceResponse': 'AnswerWebResourceRequest',
    'CreateCoreWebView2Controller': 'UseWebViewEnvironment',
    'CreateCoreWebView2ControllerWithOptions': 'UseWebViewEnvironment',
    'CreateWebResourceRequest': 'AnswerWebResourceRequest',
    'MoveFocus': 'MoveFocusRequested',
    'get_Settings': '内部解析', 'get_CookieManager': '内部解析', 'get_Environment': '内部解析',
    'get_Profile': '内部解析', 'get_DownloadOperation': '事件快照', 'get_Request': '事件快照',
    'get_Response': '事件快照', 'get_Frame': '事件快照', 'get_Certificates': '事件快照',
    'get_MenuItems': '事件快照', 'get_ProcessInfo': '事件快照', 'get_WebResourceContext': '事件快照',
    'GetFrameByName': 'GetEventObjectSnapshot',
}
refl_l = set(k.lower() for k in refl)
rows = []
for line in io.open('uncovered.txt', encoding='utf-8'):
    line = line.rstrip('\n')
    if not line.strip():
        continue
    parts = line.split('\t')
    grp, iface, member = parts[0], parts[1], parts[2]
    alias = ALIAS.get(member, '')
    if member.lower() in named or alias.lower() in named:
        how = '命名命令'
    elif alias in ('内部解析', '事件快照'):
        how = alias
    elif member.lower() in refl_l:
        how = '命名命令'
    elif member.lower() in refl_l:
        how = '反射'
    elif member in [e[0] for e in evt]:
        how = '事件订阅'
    elif member.startswith('add_') and member[4:] in [e[0] for e in evt]:
        how = '事件订阅'
    else:
        how = '待补'
    rows.append((grp, iface, member, how))

hit = sum(1 for r in rows if r[3] != '待补')
todo = [r for r in rows if r[3] == '待补']

out = []
out.append('# Edge网页浏览支持库 2.0.0 对接覆盖矩阵\n')
out.append('数据取自产物源码，由 `python3 gen_matrix.py` 生成。\n')
out.append('## 三层出口\n')
out.append('| 层 | 数量 | 说明 |')
out.append('| --- | --- | --- |')
out.append('| 命名命令 | %d 条 | 全局 %d + 组件成员 %d + 环境成员 %d + 生命周期 %d |' % (
    len(cmds), sum(1 for c in cmds if c[2] == '全局'),
    sum(1 for c in cmds if c[2] == '成员'), 0, sum(1 for c in cmds if c[2] == '生命周期')))
out.append('| 反射按名成员 | %d 条 | %s |' % (
    len(refl), ' · '.join('%s %s/%s/%s' % (g, r, w, c) for g, r, w, c in groups)))
out.append('| 可订阅事件 | %d 个 | %s |' % (
    len(evt), ' · '.join('%s %d' % (g, len(v)) for g, v in sorted(evt_groups.items()))))
out.append('| 事件参数快照对象 | %d 个 | 嵌套对象与列表（Count + ValueAt，最多 32 项）一并展开 |' % len(described))
out.append('')
out.append('## 组件事件（易语言侧 %d 个）\n' % (lib.count('WV_EVENT(') - lib.count('#define')))
ev_names = re.findall(r'WV_EVENT\("([^"]+)"', lib)
out.append(' · '.join(ev_names))
out.append('')
out.append('## 命名命令清单\n')
cur = None
for cn, eg, kind in cmds:
    if kind != cur:
        cur = kind
        out.append('\n### %s\n' % kind)
    out.append('- `%s` / %s' % (cn, eg))
out.append('')
out.append('## uncovered.txt 的去向（反射映射不干净、需要专用出口的成员）\n')
out.append('共 %d 条，其中已被命名命令或事件通道接走 %d 条，仍待补 %d 条。\n' % (len(rows), hit, len(todo)))
if todo:
    out.append('| 组 | 声明接口 | 成员 | 为什么反射接不走 |')
    out.append('| --- | --- | --- | --- |')
    for g, i, m2, _ in todo[:200]:
        raw = [l.split('\t') for l in io.open('uncovered.txt', encoding='utf-8') if l.startswith(g + '\t' + i + '\t' + m2 + '\t')]
        why = raw[0][3] if raw and len(raw[0]) > 3 else ''
        out.append('| %s | %s | %s | %s |' % (g, i, m2, why))
    out.append('')
    out.append('> 这些成员的共同点：参数是回调 / 多参 / 结构体 / 枚举位掩码，反射层的「单个简单参数」形状接不住。'
               '需要时要么补一条命名命令，要么走 `调用内核方法` 之外的专用出口，不做静默丢弃。\n')

io.open('覆盖矩阵.md', 'w', encoding='utf-8').write('\n'.join(out) + '\n')
print('覆盖矩阵.md: 命令 %d / 反射 %d / 事件 %d / 快照对象 %d / uncovered %d（待补 %d）'
      % (len(cmds), len(refl), len(evt), len(described), len(rows), len(todo)))
