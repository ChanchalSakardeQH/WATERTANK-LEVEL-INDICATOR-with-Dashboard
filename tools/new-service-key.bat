@echo off
rem woodyouloveit Water Tanks Monitor - create the manufacturer service login for the firmware.
rem Copyright (C) 2026 Chanchal Sakarde - woodyouloveit.com - GPL-3.0-or-later
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0new-service-key.ps1" %*
if "%~1"=="" pause
