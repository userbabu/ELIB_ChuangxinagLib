@echo off
rem Offline host for cxbrow.fne: verify the tables, then play the browser extension
rem against the local /cxa/ service and the EPL receiver subroutine.
rem Keep this file pure ASCII, CRLF. Run:  cmd /c build_host.bat
setlocal enabledelayedexpansion
cd /d "%~dp0"

set NAME=cxbrow
set VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat
if not exist "%VCVARS%" (
    echo [ERROR] vcvars32.bat not found
    exit /b 1
)
call "%VCVARS%" >nul

if not exist out mkdir out
if not exist out\lib mkdir out\lib
if not exist out\obj mkdir out\obj
if not exist out\lib\%NAME%.fne (
    echo [ERROR] out\lib\%NAME%.fne missing - run build.bat first
    exit /b 1
)

cl /nologo /c /O2 /MT /DWIN32 /DNDEBUG /D__GCC_ /D_CRT_SECURE_NO_WARNINGS ^
    /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W3 ^
    /Iinclude host_bridge.cpp /Foout\obj\
if errorlevel 1 (
    echo [ERROR] compile host failed
    exit /b 1
)

link /nologo /OUT:out\host_bridge.exe /SUBSYSTEM:CONSOLE out\obj\host_bridge.obj ^
    kernel32.lib user32.lib ws2_32.lib
if errorlevel 1 (
    echo [ERROR] link host failed
    exit /b 1
)

out\host_bridge.exe out\lib\%NAME%.fne
set RC=%ERRORLEVEL%
if not "%RC%"=="0" (
    echo [ERROR] host_bridge failed with exit code %RC%
    exit /b 1
)
echo [OK] host_bridge passed
endlocal
