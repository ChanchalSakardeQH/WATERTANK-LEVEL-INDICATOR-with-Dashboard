@echo off
rem woodyouloveit Water Tanks Monitor - build the Android app on Windows.
rem Double-click for a menu, or run with options, e.g.:  build-android.bat -Task Debug -Install -Launch
rem Copyright (C) 2026 Chanchal Sakarde - woodyouloveit.com - GPL-3.0-or-later
setlocal
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\build-android.ps1" %*
set "EXITCODE=%ERRORLEVEL%"
if "%~1"=="" (
    echo.
    pause
)
exit /b %EXITCODE%
