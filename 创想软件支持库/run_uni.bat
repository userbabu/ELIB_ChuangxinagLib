@echo off
rem Recompile and run only the wide-text/regex/encoding host. Pure ASCII.
setlocal
cd /d "%~dp0"
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars32.bat
call "%VCVARS%" >nul
cl /nologo /c /O2 /MT /DWIN32 /DNDEBUG /D__GCC_ /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W3 /Iinclude host_uni.cpp /Foout\
if errorlevel 1 (echo [ERROR] compile failed & exit /b 1)
link /nologo /OUT:out\host_uni.exe /SUBSYSTEM:CONSOLE out\host_uni.obj kernel32.lib user32.lib ole32.lib oleaut32.lib
if errorlevel 1 (echo [ERROR] link failed & exit /b 1)
out\host_uni.exe out\cxsoft.fne
endlocal
