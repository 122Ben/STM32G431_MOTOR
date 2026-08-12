@echo off
setlocal enabledelayedexpansion
rem Always operate on the folder this .bat file lives in, so it works no
rem matter which drive/path the repo is cloned to on a given machine.
cd /d "%~dp0"

where git >nul 2>nul
if errorlevel 1 (
    echo [ERROR] git was not found on PATH. Install Git for Windows first.
    pause
    exit /b 1
)

git rev-parse --is-inside-work-tree >nul 2>nul
if errorlevel 1 (
    echo [ERROR] "%cd%" is not a git repository.
    pause
    exit /b 1
)

for /f "delims=" %%b in ('git rev-parse --abbrev-ref HEAD') do set BRANCH=%%b

echo ================================================================
echo   FORCE PUSH  (this computer  -^>  cloud)
echo   Repo    : %cd%
echo   Branch  : !BRANCH!
echo   Remote  : origin
echo.
echo   This commits everything currently in this folder and OVERWRITES
echo   the remote branch history with it (git push --force). Anything
echo   on the remote that is not on this machine will be LOST.
echo ================================================================
set /p CONFIRM="Continue? (Y/N): "
if /i not "!CONFIRM!"=="Y" (
    echo Cancelled, nothing was done.
    pause
    exit /b 0
)

git add -A
git commit -m "sync: force upload from %COMPUTERNAME% on %date% %time%"
if errorlevel 1 (
    echo [INFO] Nothing new to commit ^(working tree already matched last commit^), continuing to push.
)

echo.
echo Pushing !BRANCH! to origin (force)...
git push origin !BRANCH! --force

if errorlevel 1 (
    echo.
    echo [ERROR] Push failed - see the message above ^(check network / credentials / remote URL^).
) else (
    echo.
    echo [OK] origin/!BRANCH! now matches this computer.
)

pause
