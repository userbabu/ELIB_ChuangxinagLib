@echo off
rem Build + run the pure-C++ engine smoke test (no EPL, no .fne).
rem Requires the library objects to exist: run build_local.bat first.
rem Pure ASCII, CRLF.  Run from this directory:  cmd /c run_engine_test.bat
setlocal
cd /d "%~dp0"
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars32.bat
if not exist "%VCVARS%" ( echo [ERROR] vcvars32.bat not found & exit /b 1 )
call "%VCVARS%" >nul
if not exist out\json_core.obj ( echo [ERROR] run build_local.bat first & exit /b 1 )

cl /nologo /c /O2 /MT /DWIN32 /DNDEBUG /D__GCC_ /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:gbk /wd4828 /wd4838 /W3 /Iinclude test_json_engine.cpp /Foout\
if errorlevel 1 ( echo [ERROR] compile test failed & exit /b 1 )

link /nologo /OUT:out\test_json_engine.exe /SUBSYSTEM:CONSOLE out\test_json_engine.obj out\json_core.obj out\json_parse.obj out\json_cmds.obj out\lib.obj out\uni_core.obj out\uni_text.obj out\uni_wstr.obj out\uni_regex.obj out\uni_conv.obj user32.lib kernel32.lib ole32.lib oleaut32.lib
if errorlevel 1 ( echo [ERROR] link test failed & exit /b 1 )

out\test_json_engine.exe
set RC=%ERRORLEVEL%
if not "%RC%"=="0" echo [ERROR] engine test failed rc=%RC%
endlocal & exit /b %RC%
