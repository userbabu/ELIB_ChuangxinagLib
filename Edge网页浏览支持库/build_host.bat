@echo off
rem Build + run the offline host that loads edgewv.fne, checks its tables and really runs a page.
rem Keep this file pure ASCII. Run from this directory: cmd /c build_host.bat
setlocal
cd /d "%~dp0"

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
if not exist out\edgewv.fne (
    echo [ERROR] out\edgewv.fne missing - run build.bat first
    exit /b 1
)
copy /y third_party\webview2\x86\WebView2Loader.dll out\ >nul
if errorlevel 1 (
    echo [ERROR] copy WebView2Loader.dll failed
    exit /b 1
)

cl /nologo /c /O2 /MT /DWIN32 /DNDEBUG /D__GCC_ /D_CRT_SECURE_NO_WARNINGS ^
    /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /wd4005 /W3 ^
    /Iinclude host_test.cpp /Foout\
if errorlevel 1 (
    echo [ERROR] compile failed
    exit /b 1
)

link /nologo /OUT:out\host_test.exe /SUBSYSTEM:CONSOLE /MACHINE:X86 ^
    out\host_test.obj user32.lib kernel32.lib gdi32.lib ws2_32.lib

if errorlevel 1 (
    echo [ERROR] link failed
    exit /b 1
)
echo [OK] out\host_test.exe

out\host_test.exe
set RC=%ERRORLEVEL%
if not "%RC%"=="0" echo [ERROR] host test failed rc=%RC%
exit /b %RC%
