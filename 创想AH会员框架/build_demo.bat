@echo off
rem Build the shell demo exe that loads cxah.fne like a host program does. Pure ASCII.
setlocal
cd /d "%~dp0"
set VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat
if not exist "%VCVARS%" (
    echo [ERROR] vcvars32.bat not found
    exit /b 1
)
call "%VCVARS%" >nul

if not exist out mkdir out
cl /nologo /c /O2 /MT /DWIN32 /DNDEBUG /D__GCC_ /D_CRT_SECURE_NO_WARNINGS ^
    /source-charset:utf-8 /execution-charset:gbk /wd4828 /W3 ^
    /Iinclude cx_demo.cpp /Foout\
if errorlevel 1 (
    echo [ERROR] compile failed
    exit /b 1
)
link /nologo /OUT:out\cx_demo.exe /SUBSYSTEM:CONSOLE out\cx_demo.obj kernel32.lib user32.lib
if errorlevel 1 (
    echo [ERROR] link failed
    exit /b 1
)
copy /y out\cxah.fne out\ >nul 2>&1
echo [OK] out\cx_demo.exe   (run: out\cx_demo.exe [api] [secret] [puc_type] [oem_key])
endlocal
