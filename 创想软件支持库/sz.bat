@echo off
rem Measure MDATA_INF / ARG_INFO / CMD_INFO size with the shipping switches (/D__GCC_).
setlocal
cd /d "%~dp0"
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars32.bat
call "%VCVARS%" >nul
cl /nologo /c /Iinclude /DWIN32 /D__GCC_ /source-charset:utf-8 /execution-charset:gbk out\sz.c /Foout\sz_gcc.obj
if errorlevel 1 (echo [ERROR] compile with __GCC_ failed & exit /b 1)
cl /nologo /Fe:out\sz_gcc.exe out\sz_gcc.obj kernel32.lib user32.lib >nul
out\sz_gcc.exe
endlocal
