@echo off
REM DeepCoreSandbox.cmd : double-clickable entry point.
REM
REM Exists so the launcher can be started from Explorer without fighting PowerShell's
REM execution policy, and so the window stays open long enough to read.
REM
REM This runs the SANDBOX -- this project's own systems on a generated cavern. It is not
REM the 1999 game, which requires your own installed copy (see scripts\run.ps1).

setlocal
cd /d "%~dp0.."
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0DeepCoreSandbox.ps1" %*
if errorlevel 1 pause
endlocal
