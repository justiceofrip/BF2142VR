@echo off
setlocal
powershell.exe -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0tools\Player.ps1" -Action Play
if errorlevel 1 exit /b 1
exit /b 0
