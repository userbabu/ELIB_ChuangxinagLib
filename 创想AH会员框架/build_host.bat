@echo off
rem Offline host for cxah.fne: verify the library tables and drive every command
rem against the local PHP backend. Keep this file pure ASCII, CRLF.
rem Run:  cmd /c build_host.bat [server] [secret] [email] [keyOwn] [keyNew] [emailLock] [keyFeat]
setlocal enabledelayedexpansion
cd /d "%~dp0"

set NAME=cxah
set VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat
if not exist "%VCVARS%" (
    echo [ERROR] vcvars32.bat not found
    exit /b 1
)
call "%VCVARS%" >nul

if not exist out mkdir out
if not exist out\%NAME%.fne (
    echo [ERROR] out\%NAME%.fne missing - run build.bat first
    exit /b 1
)

cl /nologo /c /O2 /MT /DWIN32 /DNDEBUG /D__GCC_ /D_CRT_SECURE_NO_WARNINGS ^
    /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W3 ^
    /Iinclude host_test.cpp host_ui.cpp host_class.cpp /Foout\
if errorlevel 1 (
    echo [ERROR] compile host failed
    exit /b 1
)

link /nologo /OUT:out\host_test.exe /SUBSYSTEM:CONSOLE out\host_test.obj kernel32.lib user32.lib
if errorlevel 1 (
    echo [ERROR] link host_test failed
    exit /b 1
)

link /nologo /OUT:out\host_ui.exe /SUBSYSTEM:CONSOLE out\host_ui.obj kernel32.lib user32.lib gdi32.lib
if errorlevel 1 (
    echo [ERROR] link host_ui failed
    exit /b 1
)

link /nologo /OUT:out\host_class.exe /SUBSYSTEM:CONSOLE out\host_class.obj kernel32.lib user32.lib
if errorlevel 1 (
    echo [ERROR] link host_class failed
    exit /b 1
)

if "%~1"=="" (
    echo [OK] host built. run: out\host_test.exe out\%NAME%.fne http://127.0.0.1:8317/api.php ^<secret^> ^<email^> ^<keyOwn^> ^<keyNew^> ^<emailLock^> ^<keyFeat^>
    goto finish
)

out\host_test.exe out\%NAME%.fne "%~1" "%~2" "%~3" "%~4" "%~5" "%~6" "%~7"
call :chk command
if errorlevel 1 exit /b 1

out\host_ui.exe out\%NAME%.fne "%~1" "%~2" "%~3" "%~4" "%~5"
call :chk ui
if errorlevel 1 exit /b 1

echo [OK] hosts passed

:finish
endlocal
goto :eof

:chk
rem %1 = host label. Exact compare against 0 so a negative crash code cannot slip through.
if "%ERRORLEVEL%"=="0" exit /b 0
echo [ERROR] %~1 host failed with exit code %ERRORLEVEL%
exit /b 1
