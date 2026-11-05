@echo off
rem Publish the built library into the EPL install dir (auto-detect D:\e5.5 and friends).
rem Usage:
rem   install_to_epl.bat                                  default: .\out and .\out\static
rem   install_to_epl.bat "stage\lib" "stage\static_lib"   from a release staging dir
rem Relative args resolve against the caller's current directory.
rem Idempotent: identical bytes are skipped; every install is verified byte-for-byte afterwards.
rem Pure ASCII, CRLF.
setlocal enabledelayedexpansion
set CALLER=%CD%
cd /d "%~dp0"
set NAME=cxsoft

set SRCLIB=%~dp0out
set SRCSTATIC=%~dp0out\static
if not "%~1"=="" call :resolve SRCLIB "%~1"
if not "%~2"=="" call :resolve SRCSTATIC "%~2"

set EEPATH=%EPL_E_PATH%
if not defined EEPATH for /f "tokens=1,2*" %%a in ('reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\e.exe" /ve 2^>nul ^| findstr /i REG_SZ') do set EEPATH=%%c
if defined EEPATH if exist "%EEPATH%" for %%i in ("%EEPATH%") do set EROOT=%%~dpi
if not defined EROOT if exist "D:\e5.5\e.exe" set EROOT=D:\e5.5\
if not defined EROOT if exist "D:\e5.5NewExui\e.exe" set EROOT=D:\e5.5NewExui\

if not defined EROOT (
    echo [ERROR] EPL root not detected. Set EPL_E_PATH to the full e.exe path.
    exit /b 1
)
if not exist "%EROOT%lib\" (
    echo [ERROR] %EROOT%lib missing
    exit /b 1
)
echo [EPL] %EROOT%

call :pub "%SRCLIB%\%NAME%.fne" "%EROOT%lib\%NAME%.fne"
if errorlevel 1 exit /b 1
if exist "%SRCLIB%\libcurl-impersonate.dll" (
    call :pub "%SRCLIB%\libcurl-impersonate.dll" "%EROOT%lib\libcurl-impersonate.dll"
    if errorlevel 1 echo [WARN] libcurl dll not published - net commands will report load failure
)
if exist "%SRCSTATIC%\%NAME%_static.lib" (
    call :pub "%SRCSTATIC%\%NAME%_static.lib" "%EROOT%static_lib\%NAME%_static.lib"
    if errorlevel 1 exit /b 1
)
echo [NEXT] no IDE restart needed for a library that is already ticked; a locked .fne means close the IDE and re-run.
endlocal
goto :eof

:resolve
rem %1 = out var name  %2 = path (may be relative to the caller)
set RS_P=%~2
if "!RS_P:~1,1!"==":" (
    set %1=!RS_P!
) else (
    set %1=%CALLER%\!RS_P!
)
exit /b 0

:pub
rem %1 = source (quoted)  %2 = destination (quoted)
if not exist %1 (
    echo [ERROR] source missing: %~1
    exit /b 1
)
fc /b %1 %2 >nul 2>&1
if not errorlevel 1 (
    echo [SKIP] same bytes  %~2
    exit /b 0
)
rem The EPL IDE holds lib\*.fne open, so retry a couple of times, then stage a .new copy loudly.
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
echo [PENDING] staged as %~2.new ; close the IDE and re-run this script.
exit /b 1
:pub_ok
fc /b %1 %2 >nul 2>&1
if errorlevel 1 (
    echo [ERROR] verify failed: %~2 differs from %~1
    exit /b 1
)
echo [OK] installed  %~2
exit /b 0
