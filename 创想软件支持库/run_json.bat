@echo off
rem Build and run the Json2 (zyJson port) semantic host. Pure ASCII, CRLF.
setlocal
cd /d "%~dp0"
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars32.bat
if not exist "%VCVARS%" ( echo [ERROR] vcvars32.bat not found & exit /b 1 )
call "%VCVARS%" >nul
if not exist out\cxsoft.fne ( echo [ERROR] run build_local.bat first & exit /b 1 )
cl /nologo /c /O2 /MT /DWIN32 /DNDEBUG /D__GCC_ /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W3 /Iinclude host_json.cpp /Foout\
if errorlevel 1 ( echo [ERROR] compile failed & exit /b 1 )
link /nologo /STACK:32000000 /OUT:out\host_json.exe /SUBSYSTEM:CONSOLE out\host_json.obj kernel32.lib user32.lib
if errorlevel 1 ( echo [ERROR] link failed & exit /b 1 )
out\host_json.exe out\cxsoft.fne
endlocal
