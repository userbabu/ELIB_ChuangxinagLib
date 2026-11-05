@echo off
rem Build and run the full-coverage host that calls every command once. Pure ASCII.
setlocal
cd /d "%~dp0"
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars32.bat
if not exist "%VCVARS%" (
    echo [ERROR] vcvars32.bat not found
    exit /b 1
)
call "%VCVARS%" >nul
if not exist out\cxsoft.fne (
    echo [ERROR] out\cxsoft.fne missing - run build.bat first
    exit /b 1
)
cl /nologo /c /O2 /MT /DWIN32 /DNDEBUG /D__GCC_ /D_CRT_SECURE_NO_WARNINGS ^
    /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W3 ^
    /Iinclude host_cover.cpp /Foout\
if errorlevel 1 (
    echo [ERROR] compile failed
    exit /b 1
)
link /nologo /STACK:32000000 /OUT:out\host_cover.exe /SUBSYSTEM:CONSOLE out\host_cover.obj kernel32.lib user32.lib
if errorlevel 1 (
    echo [ERROR] link failed
    exit /b 1
)
out\host_cover.exe out\cxsoft.fne
endlocal
