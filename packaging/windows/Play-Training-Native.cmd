@echo off
setlocal
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Play-Training.ps1" -GameDirectory "%~1" -ThreadMode single -CampaignMode native
if not errorlevel 1 exit /b 0
pause
exit /b 1
