import re, io, collections

src = io.open('third_party/webview2/include/WebView2.h', encoding='utf-8', errors='ignore').read()

# 只取 C++ 声明块：MIDL_INTERFACE("guid")\n  IName : public Base\n  { ... };
pat = re.compile(r'MIDL_INTERFACE\("([0-9a-f\-]+)"\)\s*\n\s*(\w+)\s*:\s*public\s+(\w+)\s*\{(.*?)\n    \};', re.S)
meth = re.compile(r'virtual\s+(?:/\*[^*]*\*/\s*)?HRESULT\s+STDMETHODCALLTYPE\s+(\w+)\s*\((.*?)\)\s*=\s*0', re.S)

rows = []
for guid, name, base, body in pat.findall(src):
    ms = [(a, re.sub(r'\s+', ' ', b).strip()) for a, b in meth.findall(body)]
    rows.append((name, base, guid, ms))

by_name = {r[0]: r for r in rows}
print('接口总数:', len(rows))

# 版本链：ICoreWebView2_N 继承 ICoreWebView2_{N-1}
def chain(root):
    order = [root]
    cur = root
    while True:
        nxt = None
        for r in rows:
            if r[1] == cur:
                nxt = r[0]
                break
        if not nxt:
            break
        order.append(nxt)
        cur = nxt
    return order

for root in ['ICoreWebView2', 'ICoreWebView2Environment', 'ICoreWebView2Controller', 'ICoreWebView2Settings']:
    c = chain(root)
    total = sum(len(by_name[n][3]) for n in c)
    print('\n#### 链 %s: %d 个接口, %d 个方法' % (root, len(c), total))
    for i, n in enumerate(c):
        ms = by_name[n][3]
        kinds = collections.Counter()
        names = []
        for a, _ in ms:
            k = '事件' if a.startswith('add_') else ('读属性' if a.startswith('get_') else ('写属性' if a.startswith('put_') else '方法'))
            kinds[k] += 1
            names.append(a)
        print('  [%s] %-46s 方法%-3d %s' % (i, n, len(ms), dict(kinds)))

# 全局导出函数
print('\n#### 全局导出')
for m in re.finditer(r'STDAPI\s+(\w+)\((.*?)\);', src, re.S):
    print('  ', m.group(1), '(', re.sub(r'\s+', ' ', m.group(2)).strip(), ')')

# 所有事件（add_X）汇总，用来对齐要抛给易语言的组件事件
evs = collections.OrderedDict()
for name, base, guid, ms in rows:
    for a, sig in ms:
        if a.startswith('add_'):
            evs.setdefault(a[4:], []).append(name)
print('\n#### 可注册事件 %d 个' % len(evs))
print('  ' + ', '.join(sorted(evs.keys())))

# 属性（get_）汇总
props = collections.OrderedDict()
for name, base, guid, ms in rows:
    for a, sig in ms:
        if a.startswith('get_') or a.startswith('put_'):
            props.setdefault(a[4:], []).append(name)
print('\n#### 属性 %d 个' % len(props))
