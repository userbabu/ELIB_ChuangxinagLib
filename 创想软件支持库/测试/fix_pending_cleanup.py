# -*- coding: utf-8 -*-
"""发布成功后顺手清掉之前暂存的 .new，免得目录里留着让人误以为没发上去。"""
import io

BS = chr(92).encode()
RN = BS + b'r' + BS + b'n'

files = [
    ('创想软件支持库' + chr(92) + 'build.bat'),
    ('创想软件支持库' + chr(92) + 'build_static.bat'),
    ('Edge网页浏览支持库' + chr(92) + 'build.bat'),
    ('Edge网页浏览支持库' + chr(92) + 'build_static.bat'),
]

for f in files:
    d = io.open(f, 'rb').read()
    if b'del /q %~2.new' in d:
        print('already', f)
        continue
    key = b':pub_ok'
    if key not in d:
        print('no :pub in', f)
        continue
    i = d.index(key)
    j = d.index(RN, i)
    d = d[:j] + RN + b'del /q %~2.new >nul 2>&1' + d[j:]
    io.open(f, 'wb').write(d)
    print('cleanup added', f)
