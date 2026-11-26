@echo off
setlocal
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\build_windows.ps1" %*
set "CODPORT_BUILD_EXIT=%errorlevel%"
echo.
pause
exit /b %CODPORT_BUILD_EXIT%
