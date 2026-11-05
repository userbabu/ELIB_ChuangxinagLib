@echo off
rem Compile and run the crypto/http probe (pure ASCII).
setlocal
cd /d "%~dp0"
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >/dev/null
cl /nologo /c /O2 /MT /DWIN32 /DNDEBUG /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:gbk /W3 /Iinclude cx_probe.cpp cx_net.cpp cx_api.cpp /Foout\
if errorlevel 1 (echo [ERROR] compile & exit /b 1)
link /nologo /OUT:out\cx_probe.exe /SUBSYSTEM:CONSOLE out\cx_probe.obj out\cx_net.obj out\cx_api.obj kernel32.lib user32.lib
if errorlevel 1 (echo [ERROR] link & exit /b 1)
out\cx_probe.exe
endlocal
