@echo off
rem Build and publish this support library into the EPL directories in one go:
rem   dynamic  .fne  -> <EPL>\lib
rem   static   .lib  -> <EPL>\static_lib
rem Every step verifies the installed bytes match the freshly built ones.
rem Keep this file pure ASCII. Run from this directory: cmd /c publish.bat
setlocal
cd /d "%~dp0"

call build.bat
if errorlevel 1 (
    echo [ERROR] build or publish failed
    exit /b 1
)

echo [DONE] published to the EPL directories. Restart the IDE to load the new library.
endlocal
