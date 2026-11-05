@echo off
rem Compile out\consts_probe.c so MSVC resolves every curl constant, then dump name=value.
rem Called by: node <lib>\http\gen_http_consts.mjs --verify  (keep this file pure ASCII)
setlocal
cd /d "%~dp0.."

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

cl /nologo /O2 /MT /DWIN32 /DNDEBUG /D_CRT_SECURE_NO_WARNINGS /W3 ^
    /Ithird_party\include out\consts_probe.c /Foout\ /Feout\consts_probe.exe

if errorlevel 1 (
    echo [ERROR] probe compile failed
    exit /b 1
)

out\consts_probe.exe > out\consts_probe.txt
if errorlevel 1 (
    echo [ERROR] probe run failed
    exit /b 1
)
echo [OK] probe wrote out\consts_probe.txt
endlocal
