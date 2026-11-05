@echo off
rem Build cxah_static.lib so EPL can static-compile (link the library into the EXE).
rem Same sources as the .fne, compiled with /D__E_STATIC_LIB so only the command
rem implementations are emitted. Then install it into <EPL>\static_lib.
rem EPL static compile uses the VC6 linker + VC6 CRT: keep /GS- and never convert
rem double <-> __int64. Keep this file pure ASCII with CRLF line endings.
setlocal
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

if not exist out\static mkdir out\static

rem /Zc:threadSafeInit- : VC6 CRT has no _Init_thread_header/footer/epoch.
rem /D_MSVC_STL_USE_ABORT_AS_DOOM_FUNCTION : the STL precondition 'doom function' is _invoke_watson,
rem which VC6 CRT does not have; with this switch it calls abort() instead (VC6 LIBC has it).
cl /nologo /c /O2 /MT /GS- /Zc:threadSafeInit- /D_MSVC_STL_USE_ABORT_AS_DOOM_FUNCTION /DWIN32 /DNDEBUG /D_LIB /D__E_STATIC_LIB /D__GCC_ /D_CRT_SECURE_NO_WARNINGS ^
    /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W3 ^
    /Iinclude /Foout\static\ lib.cpp cx_net.cpp cx_api.cpp cx_ui.cpp cx_event.cpp cx_static_shim.cpp

if errorlevel 1 (
    echo [ERROR] compile failed
    exit /b 1
)

rem ---------- drop the directives the VC6 linker rejects ----------
node strip_drectve.mjs out\static\lib.obj out\static\cx_net.obj out\static\cx_api.obj out\static\cx_ui.obj out\static\cx_event.obj out\static\cx_static_shim.obj
if errorlevel 1 (
    echo [ERROR] strip_drectve failed
    exit /b 1
)

lib /nologo /OUT:out\static\%NAME%_static.lib out\static\lib.obj out\static\cx_net.obj ^
    out\static\cx_api.obj out\static\cx_ui.obj out\static\cx_event.obj out\static\cx_static_shim.obj
if errorlevel 1 (
    echo [ERROR] lib failed
    exit /b 1
)
echo [OK] out\static\%NAME%_static.lib

rem ---------- sanity: every symbol must be resolvable by the VC6 libs EPL ships ----------
rem Not a banned-name list: this indexes D:\e5.5\linker\VC6linker\Lib + MFC\Lib and diffs
rem our archive against it, so a new UCRT/STL helper cannot slip through silently.
node check_static_undef.mjs out\static\%NAME%_static.lib
if errorlevel 1 (
    echo [ERROR] static lib cannot be linked by the EPL VC6 linker
    exit /b 1
)

rem ---------- install into <EPL>\static_lib ----------
set EEPATH=%EPL_E_PATH%
if not defined EEPATH for /f "tokens=1,2*" %%a in ('reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\e.exe" /ve 2^>nul ^| findstr /i REG_SZ') do set EEPATH=%%c
if defined EEPATH if exist "%EEPATH%" for %%i in ("%EEPATH%") do set EROOT=%%~dpi
if not defined EROOT if exist "D:\e5.5\e.exe" set EROOT=D:\e5.5\
if not defined EROOT if exist "D:\e5.5NewExui\e.exe" set EROOT=D:\e5.5NewExui\

if not defined EROOT (
    echo [ERROR] EPL dir not detected; copy out\static\%NAME%_static.lib by hand or set EPL_E_PATH
    exit /b 1
)
if not exist "%EROOT%static_lib" (
    echo [ERROR] %EROOT%static_lib missing; copy out\static\%NAME%_static.lib by hand
    exit /b 1
)

call :pub out\static\%NAME%_static.lib "%EROOT%static_lib\%NAME%_static.lib"
if errorlevel 1 exit /b 1
echo [OK] installed %EROOT%static_lib\%NAME%_static.lib

endlocal
goto :eof

:pub
rem %1 = source (quoted)  %2 = destination (quoted)
rem The EPL IDE holds the loaded library open, so retry, then stage a .new copy and say so loudly.
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
