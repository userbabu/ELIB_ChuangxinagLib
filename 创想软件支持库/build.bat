@echo off
rem Build ChuangXiang support library (.fne): 32-bit, static CRT, GBK output.
rem Then install it into the EPL lib directory automatically.
rem Run from this directory:  cmd /c build.bat
setlocal enabledelayedexpansion
cd /d "%~dp0"

set NAME=cxsoft

set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars32.bat
if not exist "%VCVARS%" (
    echo [ERROR] vcvars32.bat not found
    exit /b 1
)
call "%VCVARS%" >nul
if errorlevel 1 (
    echo [ERROR] vcvars32.bat failed
    exit /b 1
)

if not exist out mkdir out

cl /nologo /c /O2 /MT /DWIN32 /DNDEBUG /D__GCC_ /D_CRT_SECURE_NO_WARNINGS ^
    /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W3 ^
    /Iinclude /Ithird_party/include lib.cpp uni_core.cpp uni_text.cpp uni_wstr.cpp uni_regex.cpp uni_conv.cpp json_core.cpp json_parse.cpp json_cmds.cpp /Foout\

if errorlevel 1 (
    echo [ERROR] compile failed
    exit /b 1
)

link /nologo /DLL /MACHINE:X86 /OUT:out\%NAME%.fne /DEF:lib.def ^
    out\lib.obj out\uni_core.obj out\uni_text.obj out\uni_wstr.obj out\uni_regex.obj out\uni_conv.obj out\json_core.obj out\json_parse.obj out\json_cmds.obj user32.lib kernel32.lib

if errorlevel 1 (
    echo [ERROR] link failed
    exit /b 1
)

if exist out\%NAME%.lib del /q out\%NAME%.lib
if exist out\%NAME%.exp del /q out\%NAME%.exp
echo [OK] built out\%NAME%.fne

rem ---------- copy the runtime dependency next to the library ----------
if exist third_party\lib\libcurl-impersonate.dll copy /y third_party\lib\libcurl-impersonate.dll out\ >nul

rem ---------- static library, required by EPL static compile ----------
rem (comment removed: batch files must stay pure ASCII)
call build_static.bat

rem ---------- install into EPL lib dir (auto) ----------
set EEPATH=%EPL_E_PATH%
if not defined EEPATH for /f "tokens=1,2*" %%a in ('reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\e.exe" /ve 2^>nul ^| findstr /i REG_SZ') do set EEPATH=%%c
if defined EEPATH if exist "%EEPATH%" for %%i in ("%EEPATH%") do set EROOT=%%~dpi
if not defined EROOT if exist "D:\e5.5\e.exe" set EROOT=D:\e5.5\
if not defined EROOT if exist "D:\e5.5NewExui\e.exe" set EROOT=D:\e5.5NewExui\

if not defined EROOT (
    echo [WARN] EPL lib dir not detected; copy out\%NAME%.fne by hand or set EPL_E_PATH
    goto finish
)
if not exist "%EROOT%lib" (
    echo [WARN] %EROOT%lib missing; copy out\%NAME%.fne by hand
    goto finish
)

call :pub "out\%NAME%.fne" "%EROOT%lib\%NAME%.fne"
if errorlevel 1 exit /b 1
if exist third_party\lib\libcurl-impersonate.dll (
    call :pub third_party\lib\libcurl-impersonate.dll "%EROOT%lib\libcurl-impersonate.dll"
    if errorlevel 1 echo [WARN] libcurl-impersonate.dll not published; net commands will report load failure
)
echo [OK] installed  %EROOT%lib\%NAME%.fne
echo [NEXT] tick the library once in Tools / Support library config; replacing the files needs no IDE restart (see README)

:finish
endlocal
goto :eof

:pub
rem %1 = source (quoted)  %2 = destination (quoted)
rem EPL IDE running holds the .fne open, so retry, then stage a .new copy and say so loudly.
set PB_N=0
:pub_try
set /a PB_N+=1
copy /y %1 %2 >nul 2>&1
if not errorlevel 1 goto pub_ok
if %PB_N% GEQ 3 goto pub_stage
echo [WARN] %~2 busy (EPL IDE running?), retry %PB_N%/2 ...
ping -n 3 127.0.0.1 >nul
goto pub_try
:pub_stage
copy /y %1 %~2.new >nul 2>&1
echo [PENDING] cannot publish %~2 - it is locked.
echo [PENDING] staged as %~2.new ; close the IDE and re-run to publish.
exit /b 1
:pub_ok
rem Publishing is not done until the installed bytes match what we just built.
fc /b %1 %2 >nul 2>&1
if errorlevel 1 (
    echo [ERROR] verify failed: %~2 differs from %~1
    exit /b 1
)
exit /b 0
