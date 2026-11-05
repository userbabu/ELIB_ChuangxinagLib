@echo off
rem Build cxsoft_static.lib so EPL can static-compile (link the library into the EXE).
rem Same sources as the .fne, compiled with /D__E_STATIC_LIB so only the command
rem implementations are emitted. Keep this file pure ASCII.
setlocal
cd /d "%~dp0"

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

if not exist out\static mkdir out\static

cl /nologo /c /O2 /MT /GS- /DWIN32 /DNDEBUG /D_LIB /D__E_STATIC_LIB /D__GCC_ /D_CRT_SECURE_NO_WARNINGS ^
    /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W3 ^
    /Iinclude /Ithird_party/include /Foout\static\ lib.cpp uni_core.cpp uni_text.cpp uni_wstr.cpp uni_regex.cpp uni_conv.cpp json_core.cpp json_parse.cpp json_cmds.cpp

if errorlevel 1 (
    echo [ERROR] compile failed
    exit /b 1
)

rem ---------- drop the directives the VC6 linker (used by EPL static compile) rejects ----------
node strip_drectve.mjs out\static\lib.obj out\static\uni_core.obj out\static\uni_text.obj out\static\uni_wstr.obj out\static\uni_regex.obj out\static\uni_conv.obj out\static\json_core.obj out\static\json_parse.obj out\static\json_cmds.obj
if errorlevel 1 (
    echo [ERROR] strip_drectve failed
    exit /b 1
)

lib /nologo /OUT:out\static\cxsoft_static.lib out\static\lib.obj out\static\uni_core.obj out\static\uni_text.obj out\static\uni_wstr.obj out\static\uni_regex.obj out\static\uni_conv.obj out\static\json_core.obj out\static\json_parse.obj out\static\json_cmds.obj
if errorlevel 1 (
    echo [ERROR] lib failed
    exit /b 1
)
echo [OK] out\static\cxsoft_static.lib

rem ---------- sanity: no modern-CRT helpers (EPL static-links with VC6, which lacks them) ----------
rem Verified absent from D:\e5.5\linker\VC6linker\Lib + MFC\Lib; _stricmp/_wcsstr/_wcstod/_chkstk
rem _fltused/malloc/free ARE there, so do not ban those.
dumpbin /SYMBOLS out\static\cxsoft_static.lib | findstr /c:"UNDEF" > out\static\undef.txt
if errorlevel 1 (
    echo [ERROR] dumpbin /SYMBOLS failed
    exit /b 1
)
findstr /c:"__dtol" /c:"__ltod" /c:"__dtoui" /c:"__ultod" /c:"___stdio_common_v" /c:"___acrt_iob_func" /c:"___security_cookie" /c:"__security_check_cookie" /c:"___report_rangecheckfailure" /c:"__except_handler4_common" /c:"___CxxFrameHandler" out\static\undef.txt > out\static\bad_undef.txt
if not errorlevel 1 (
    echo [ERROR] static lib references symbols the VC6 linker cannot resolve:
    type out\static\bad_undef.txt
    echo [ERROR] double-to-int64 casts pull in __dtol3; UCRT stdio pulls in ___stdio_common_v*;
    echo [ERROR] missing /GS- pulls in ___security_cookie. Fix the source, do not patch the lib.
    exit /b 1
)
echo [OK] undef check passed

rem ---------- install into <EPL>\static_lib ----------
set EEPATH=%EPL_E_PATH%
if not defined EEPATH for /f "tokens=1,2*" %%a in ('reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\e.exe" /ve 2^>nul ^| findstr /i REG_SZ') do set EEPATH=%%c
if defined EEPATH if exist "%EEPATH%" for %%i in ("%EEPATH%") do set EROOT=%%~dpi
if not defined EROOT if exist "D:\e5.5\e.exe" set EROOT=D:\e5.5\
if not defined EROOT if exist "D:\e5.5NewExui\e.exe" set EROOT=D:\e5.5NewExui\

if not defined EROOT (
    echo [WARN] EPL dir not detected; copy out\static\cxsoft_static.lib by hand or set EPL_E_PATH
    goto finish
)
if not exist "%EROOT%static_lib" (
    echo [WARN] %EROOT%static_lib missing; copy out\static\cxsoft_static.lib by hand
    goto finish
)

call :pub out\static\cxsoft_static.lib "%EROOT%static_lib\cxsoft_static.lib"
if errorlevel 1 exit /b 1
echo [OK] installed %EROOT%static_lib\cxsoft_static.lib

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
