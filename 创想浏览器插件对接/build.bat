@echo off
rem Build a native EPL support library (.fne): 32-bit, static CRT, GBK output.
rem Then install it into the EPL lib directory automatically.
rem Keep this file pure ASCII: Chinese characters here break cmd.exe parsing.
rem Run from this directory:  cmd /c build.bat
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

if not exist out mkdir out
if not exist out\lib mkdir out\lib
if not exist out\obj mkdir out\obj

cl /nologo /c /O2 /MT /GS- /DWIN32 /DNDEBUG /D__GCC_ /D_CRT_SECURE_NO_WARNINGS ^
    /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W3 ^
    /Iinclude lib.cpp bridge_util.cpp bridge_site.cpp bridge_ev.cpp bridge_srv.cpp /Foout\obj\

if errorlevel 1 (
    echo [ERROR] compile failed
    exit /b 1
)

link /nologo /DLL /MACHINE:X86 /OUT:out\lib\%NAME%.fne /DEF:lib.def ^
    out\obj\lib.obj out\obj\bridge_util.obj out\obj\bridge_site.obj out\obj\bridge_ev.obj out\obj\bridge_srv.obj ^
    user32.lib kernel32.lib

if errorlevel 1 (
    echo [ERROR] link failed
    exit /b 1
)

if exist out\lib\%NAME%.lib del /q out\lib\%NAME%.lib
if exist out\lib\%NAME%.exp del /q out\lib\%NAME%.exp
echo [OK] built out\lib\%NAME%.fne

rem ---------- static library (needed by EPL static compile) ----------
call build_static.bat

rem ---------- publish into every EPL root found on this machine ----------
rem Two IDEs may be installed (D:\e5.5 and D:\e5.5NewExui); installing into only one of
rem them leaves the other loading a stale .fne, so collect every root and loop.
set ROOTS=
if defined EPL_E_PATH call :try_root "%EPL_E_PATH%"
for /f "tokens=1,2*" %%a in ('reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\e.exe" /ve 2^>nul ^| findstr /i REG_SZ') do call :try_root "%%c"
call :try_root "D:\e5.5"
call :try_root "D:\e5.5NewExui"

if "!ROOTS!"=="" (
    echo [WARN] no EPL root detected; set EPL_E_PATH to e.exe or copy out\lib\%NAME%.fne by hand
    goto finish
)
for %%r in (!ROOTS!) do (
    call :pub "%%~r" "out\lib\%NAME%.fne" "lib" "%NAME%.fne"
    if errorlevel 1 exit /b 1
)
echo [NEXT] tick the library once in Tools / Support library config; replacing the files needs no IDE restart

:finish
endlocal
goto :eof

:try_root
rem %1 = candidate EPL dir or full path to e.exe. Must have e.exe and a lib subdir.
set C=%~1
if "%C%"=="" exit /b 0
for %%f in ("%C%") do if /i "%%~nxf"=="e.exe" set C=%%~dpi
if not exist "!C!\e.exe" exit /b 0
if not exist "!C!\lib" exit /b 0
if "!C:~-1!"=="\" set C=!C:~0,-1!
for %%r in (!ROOTS!) do if /i "%%~r"=="!C!" exit /b 0
set ROOTS=!ROOTS!;"!C!"
exit /b 0

:pub
rem %1 EPL root  %2 source file  %3 target subdir (lib / static_lib)  %4 target file name
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
