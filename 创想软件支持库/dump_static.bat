@echo off
rem Dump static lib member names + undefined symbols for review. Pure ASCII.
setlocal
cd /d "%~dp0"
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars32.bat
if not exist "%VCVARS%" (
    echo [ERROR] vcvars32.bat not found
    exit /b 1
)
call "%VCVARS%" >nul
dumpbin /LINKERMEMBER:1 out\static\cxsoft_static.lib > out\static_members.txt
dumpbin /SYMBOLS out\static\*.obj > out\static_symbols.txt
echo [OK] wrote out\static_members.txt and out\static_symbols.txt
endlocal
