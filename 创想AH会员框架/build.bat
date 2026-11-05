@echo off
rem Build the ChuangXiang AH member support library (.fne): 32-bit, static CRT, GBK output.
rem Installs the .fne into <EPL>\lib and then delegates to build_static.bat, which builds
rem and installs <EPL>\static_lib\cxah_static.lib. Every install is byte-verified.
rem Keep this file pure ASCII with CRLF line endings. Run:  cmd /c build.bat
setlocal enabledelayedexpansion
cd /d "%~dp0"

set NAME=cxah

set VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat
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
    /Iinclude lib.cpp cx_net.cpp cx_api.cpp cx_ui.cpp cx_event.cpp /Foout\

if errorlevel 1 (
    echo [ERROR] compile failed
    exit /b 1
)

link /nologo /DLL /MACHINE:X86 /OUT:out\%NAME%.fne /DEF:lib.def ^
    out\lib.obj out\cx_net.obj out\cx_api.obj out\cx_ui.obj out\cx_event.obj ^
    user32.lib kernel32.lib gdi32.lib

if errorlevel 1 (
    echo [ERROR] link failed
    exit /b 1
)

if exist out\%NAME%.lib del /q out\%NAME%.lib
if exist out\%NAME%.exp del /q out\%NAME%.exp
echo [OK] built out\%NAME%.fne

rem ---------- locate the EPL root ----------
set EEPATH=%EPL_E_PATH%
if not defined EEPATH for /f "tokens=1,2*" %%a in ('reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\e.exe" /ve 2^>nul ^| findstr /i REG_SZ') do set EEPATH=%%c
if defined EEPATH if exist "%EEPATH%" for %%i in ("%EEPATH%") do set EROOT=%%~dpi
if not defined EROOT if exist "D:\e5.5\e.exe" set EROOT=D:\e5.5\
if not defined EROOT if exist "D:\e5.5NewExui\e.exe" set EROOT=D:\e5.5NewExui\

if not defined EROOT (
    echo [ERROR] EPL dir not detected; copy out\%NAME%.fne by hand or set EPL_E_PATH
    exit /b 1
)
if not exist "%EROOT%lib" (
    echo [ERROR] %EROOT%lib missing; copy out\%NAME%.fne by hand
    exit /b 1
)

call :pub "out\%NAME%.fne" "%EROOT%lib\%NAME%.fne"
if errorlevel 1 exit /b 1
echo [OK] installed %EROOT%lib\%NAME%.fne
echo [NEXT] tick the library once in Tools / Support library config; replacing files needs no IDE restart

rem ---------- static library, required by EPL static compile ----------
call build_static.bat
if errorlevel 1 (
    echo [ERROR] static build or publish failed
    exit /b 1
)

echo [DONE] %NAME%.fne and %NAME%_static.lib published to %EROOT%
endlocal
goto :eof

:pub
rem %1 = source (quoted)  %2 = destination (quoted)
rem The EPL IDE holds the loaded .fne open, so retry, then stage a .new copy and say so loudly.
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
