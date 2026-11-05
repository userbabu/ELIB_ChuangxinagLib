@echo off
rem Build <SHORT>_static.lib so EPL can static-compile (link the library into the EXE).
rem EPL static compile uses the VC6 linker + VC6 CRT: /GS- and NO double<->int conversions,
rem otherwise you get LNK2001 on __security_check_cookie / __dtol3 / __report_rangecheckfailure.
rem Keep this file pure ASCII with CRLF line endings.
setlocal enabledelayedexpansion
cd /d "%~dp0"

set NAME=cxbrow

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

if not exist out\obj_static mkdir out\obj_static
if not exist out\static_lib mkdir out\static_lib

cl /nologo /c /O2 /MT /GS- /DWIN32 /DNDEBUG /D_LIB /D__E_STATIC_LIB /D__GCC_ /D_CRT_SECURE_NO_WARNINGS ^
    /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W3 ^
    /Iinclude lib.cpp bridge_util.cpp bridge_site.cpp bridge_ev.cpp bridge_srv.cpp /Foout\obj_static\

if errorlevel 1 (
    echo [ERROR] compile failed
    exit /b 1
)

set OBJS=out\obj_static\lib.obj out\obj_static\bridge_util.obj out\obj_static\bridge_site.obj out\obj_static\bridge_ev.obj out\obj_static\bridge_srv.obj

rem ---------- drop linker directives the VC6 linker rejects (LNK4044 makes EPL report a
rem static-link failure even though the EXE was produced) ----------
node strip_drectve.mjs %OBJS%
if errorlevel 1 (
    echo [ERROR] strip_drectve failed
    exit /b 1
)

lib /nologo /OUT:out\static_lib\cxbrow_static.lib %OBJS%
if errorlevel 1 (
    echo [ERROR] lib failed
    exit /b 1
)
echo [OK] out\static_lib\cxbrow_static.lib

rem ---------- sanity: every undefined symbol must exist in the VC6 libs EPL ships ----------
node check_static_undef.mjs out\static_lib\cxbrow_static.lib
if errorlevel 1 (
    echo [ERROR] static lib cannot be linked by the EPL VC6 linker
    exit /b 1
)

rem ---------- publish the static lib into every EPL root found ----------
set ROOTS=
if defined EPL_E_PATH call :try_root "%EPL_E_PATH%"
for /f "tokens=1,2*" %%a in ('reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\e.exe" /ve 2^>nul ^| findstr /i REG_SZ') do call :try_root "%%c"
call :try_root "D:\e5.5"
call :try_root "D:\e5.5NewExui"

if "!ROOTS!"=="" (
    echo [WARN] no EPL root detected; copy out\static_lib\%NAME%_static.lib by hand or set EPL_E_PATH
    goto finish
)
for %%r in (!ROOTS!) do (
    call :pub "%%~r" "out\static_lib\%NAME%_static.lib" "static_lib" "%NAME%_static.lib"
    if errorlevel 1 exit /b 1
)

:finish
endlocal
goto :eof

:try_root
rem %1 = candidate EPL dir or full path to e.exe. Must have e.exe and a static_lib subdir.
set C=%~1
if "%C%"=="" exit /b 0
for %%f in ("%C%") do if /i "%%~nxf"=="e.exe" set C=%%~dpi
if not exist "!C!\e.exe" exit /b 0
if not exist "!C!\static_lib" exit /b 0
if "!C:~-1!"=="\" set C=!C:~0,-1!
for %%r in (!ROOTS!) do if /i "%%~r"=="!C!" exit /b 0
set ROOTS=!ROOTS!;"!C!"
exit /b 0

:pub
rem %1 EPL root  %2 source file  %3 target subdir  %4 target file name
set DST=%~1\%~3\%~4
copy /y "%~2" "!DST!" >nul 2>nul
if errorlevel 1 copy /y "%~2" "!DST!" >nul 2>nul
if errorlevel 1 (
    copy /y "%~2" "!DST!.new" >nul
    echo [PENDING] !DST! is locked by the running IDE; staged as !DST!.new - re-run after closing it
    exit /b 0
)
fc /b "%~2" "!DST!" >nul
if errorlevel 1 (
    echo [ERROR] installed file differs from %~2
    exit /b 1
)
if exist "!DST!.new" del /q "!DST!.new"
echo [OK] installed !DST!
exit /b 0
