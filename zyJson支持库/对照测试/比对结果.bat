@echo off
rem Compare the two zyJson differential runs.
rem   res\A = baseline (zyJson2.51 module)
rem   res\B = new      (cxsoft 3.4.0 Json2 class)
rem ASCII only + CRLF, keep it that way (cmd chokes on UTF-8 blocks here).
setlocal
set "ROOT=%~dp0"
set "A=%ROOT%res\A"
set "B=%ROOT%res\B"

if not exist "%A%\zydiff_text.txt" goto :missA
if not exist "%B%\zydiff_text.txt" goto :missB

echo ===== text diff (fc /N, A = module baseline, B = new lib) =====
fc /N "%A%\zydiff_text.txt" "%B%\zydiff_text.txt" > "%ROOT%diff_text.txt"
if errorlevel 1 (echo [DIFF] text results differ, see diff_text.txt) else (echo [OK] text results identical)
type "%ROOT%diff_text.txt"

echo ===== wide bin diff (fc /B) =====
fc /B "%A%\zydiff_wide.bin" "%B%\zydiff_wide.bin" > "%ROOT%diff_wide.txt"
if errorlevel 1 (echo [DIFF] wide byte results differ, see diff_wide.txt) else (echo [OK] wide byte results identical)
type "%ROOT%diff_wide.txt"

echo ===== line counts (sanity: both runs must reach the 99 marker) =====
for %%F in ("%A%\zydiff_text.txt" "%B%\zydiff_text.txt") do (
  for /f %%N in ('find /c /v "" ^< %%F') do echo %%~nxF lines=%%N
)
find "99." "%A%\zydiff_text.txt" >nul || echo [WARN] A did not reach the finish marker
find "99." "%B%\zydiff_text.txt" >nul || echo [WARN] B did not reach the finish marker
exit /b 0

:missA
echo [ERROR] missing %A%\zydiff_text.txt  -- run the baseline project first and drop its two output files there.
exit /b 1

:missB
echo [ERROR] missing %B%\zydiff_text.txt  -- run the new-lib project first and drop its two output files there.
exit /b 1
