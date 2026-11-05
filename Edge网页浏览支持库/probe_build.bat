@echo off
rem Build the throwaway probe that dumps any .fne's component registration.
setlocal
cd /d "%~dp0"

set VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat
if not exist "%VCVARS%" (
    echo [ERROR] vcvars32.bat not found
    exit /b 1
)
call "%VCVARS%" >nul
if errorlevel 1 (
    echo [ERROR] vcvars failed
    exit /b 1
)

if not exist out\probe mkdir out\probe

cl /nologo /O2 /MT /DWIN32 /DNDEBUG /D__GCC_ /D_CRT_SECURE_NO_WARNINGS ^
    /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W1 ^
    /Iinclude /Foout\probe\ probe_lib.cpp /Feout\probe\probe_lib.exe
if errorlevel 1 (
    echo [ERROR] compile failed
    exit /b 1
)
echo [OK] probe built
