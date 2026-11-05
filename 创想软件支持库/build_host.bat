@echo off
rem Build and run all offline hosts against out\cxsoft.fne. Pure ASCII, CRLF.
rem   host_const.exe  constant table vs the MSVC curl probe (see the http subdir generator)
rem   host_cover.exe  full sweep: every command called once (no crash + return type sane)
rem   host_uni.exe    semantic assertions: wide-text / regex / encoding / array-return
rem   host_test.exe   semantic assertions: network classes (does real HTTP)
rem   host_json.exe   semantic assertions: Json2 class members
rem A crashed host returns a NEGATIVE exit code, which "if errorlevel 1" would treat as
rem success, so every run goes through :chk (exact compare against 0).
rem Run from this directory:  cmd /c build_host.bat
setlocal
cd /d "%~dp0"

set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars32.bat
if not exist "%VCVARS%" (
    echo [ERROR] vcvars32.bat not found
    exit /b 1
)
call "%VCVARS%" >nul

if not exist out mkdir out
if not exist out\cxsoft.fne (
    echo [ERROR] out\cxsoft.fne missing - run build.bat first
    exit /b 1
)
copy /y third_party\lib\libcurl-impersonate.dll out\ >nul
if errorlevel 1 (
    echo [ERROR] copy libcurl-impersonate.dll failed
    exit /b 1
)

cl /nologo /c /O2 /MT /DWIN32 /DNDEBUG /D__GCC_ /D_CRT_SECURE_NO_WARNINGS ^
    /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W3 ^
    /Iinclude /Ithird_party/include host_test.cpp host_uni.cpp host_cover.cpp host_json.cpp host_const.cpp host_lock.cpp /Foout\
if errorlevel 1 (
    echo [ERROR] compile failed
    exit /b 1
)

link /nologo /OUT:out\host_const.exe /SUBSYSTEM:CONSOLE out\host_const.obj kernel32.lib user32.lib
if errorlevel 1 (
    echo [ERROR] link host_const failed
    exit /b 1
)

link /nologo /OUT:out\host_test.exe /SUBSYSTEM:CONSOLE out\host_test.obj kernel32.lib user32.lib
if errorlevel 1 (
    echo [ERROR] link host_test failed
    exit /b 1
)

link /nologo /STACK:32000000 /OUT:out\host_uni.exe /SUBSYSTEM:CONSOLE out\host_uni.obj kernel32.lib user32.lib ole32.lib oleaut32.lib
if errorlevel 1 (
    echo [ERROR] link host_uni failed
    exit /b 1
)

link /nologo /STACK:32000000 /OUT:out\host_cover.exe /SUBSYSTEM:CONSOLE out\host_cover.obj kernel32.lib user32.lib
if errorlevel 1 (
    echo [ERROR] link host_cover failed
    exit /b 1
)

echo ---- run constant-table host ----
if not exist out\consts_probe.txt (
    echo [ERROR] out\consts_probe.txt missing - run node gen_http_consts.mjs --verify first
    exit /b 1
)
out\host_const.exe out\cxsoft.fne out\consts_probe.txt
call :chk constants
if errorlevel 1 exit /b 1

echo ---- run full-coverage host ----
out\host_cover.exe out\cxsoft.fne
call :chk coverage
if errorlevel 1 exit /b 1

echo ---- run wide-text / regex / encoding host ----
out\host_uni.exe out\cxsoft.fne
call :chk wide-text
if errorlevel 1 exit /b 1

echo ---- run Json2 semantic host ----
cl /nologo /c /O2 /MT /DWIN32 /DNDEBUG /D__GCC_ /D_CRT_SECURE_NO_WARNINGS ^
    /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W3 ^
    /Iinclude host_json.cpp /Foout
if errorlevel 1 (
    echo [ERROR] compile host_json failed
    exit /b 1
)
link /nologo /STACK:32000000 /OUT:out\host_json.exe /SUBSYSTEM:CONSOLE out\host_json.obj kernel32.lib user32.lib
if errorlevel 1 (
    echo [ERROR] link host_json failed
    exit /b 1
)
out\host_json.exe out\cxsoft.fne
if errorlevel 1 (
    echo [ERROR] Json2 host failed
    exit /b 1
)

echo ---- run network host ----
out\host_test.exe out\cxsoft.fne
call :chk network
if errorlevel 1 exit /b 1


link /nologo /OUT:out\host_lock.exe /SUBSYSTEM:CONSOLE out\host_lock.obj kernel32.lib user32.lib ole32.lib oleaut32.lib
if errorlevel 1 (
    echo [ERROR] link host_lock failed
    exit /b 1
)

echo ---- run lock concurrency host ----
out\host_lock.exe out\cxsoft.fne
call :chk lock
if errorlevel 1 exit /b 1

echo [OK] all hosts passed
cl /nologo /O2 /MT /DWIN32 /D__GCC_ /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /Iinclude host_cancel.cpp /Foout\host_cancel.obj /Feout\host_cancel.exe /link ws2_32.lib kernel32.lib user32.lib psapi.lib
if errorlevel 1 exit /b 1
out\host_cancel.exe out\cxsoft.fne
call :chk cancellation-token
if errorlevel 1 exit /b 1
echo [OK] cancellation token and all hosts passed
endlocal
goto :eof

:chk
rem %1 = host label. Exact compare against 0: negative crash codes must not slip through.
if "%ERRORLEVEL%"=="0" exit /b 0
echo [ERROR] %~1 host failed with exit code %ERRORLEVEL%
exit /b 1
