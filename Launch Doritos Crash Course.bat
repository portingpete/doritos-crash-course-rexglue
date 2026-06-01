@echo off
setlocal

cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\launch.ps1"

if errorlevel 1 (
    echo.
    echo Launch failed. Review the message above, then press any key to close.
    pause >nul
    exit /b 1
)

exit /b 0

