@echo off
rem Local-only build: compile + link out\cxsoft.fne WITHOUT publishing to the EPL lib dir.
rem Pure ASCII, CRLF.  Run from this directory:  cmd /c build_local.bat
setlocal
cd /d "%~dp0"
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars32.bat
if not exist "%VCVARS%" ( echo [ERROR] vcvars32.bat not found & exit /b 1 )
call "%VCVARS%" >nul
if not exist out mkdir out

set SRCS=lib.cpp uni_core.cpp uni_text.cpp uni_wstr.cpp uni_regex.cpp uni_conv.cpp json_core.cpp json_parse.cpp json_cmds.cpp
set OBJS=out\lib.obj out\uni_core.obj out\uni_text.obj out\uni_wstr.obj out\uni_regex.obj out\uni_conv.obj out\json_core.obj out\json_parse.obj out\json_cmds.obj

cl /nologo /c /O2 /MT /DWIN32 /DNDEBUG /D__GCC_ /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W3 /Iinclude /Ithird_party/include %SRCS% /Foout\
if errorlevel 1 ( echo [ERROR] compile failed & exit /b 1 )

link /nologo /DLL /MACHINE:X86 /OUT:out\cxsoft.fne /DEF:lib.def %OBJS% user32.lib kernel32.lib
if errorlevel 1 ( echo [ERROR] link failed & exit /b 1 )

if exist out\cxsoft.lib del /q out\cxsoft.lib
if exist out\cxsoft.exp del /q out\cxsoft.exp
echo [OK] built out\cxsoft.fne (local only, not published)
endlocal
