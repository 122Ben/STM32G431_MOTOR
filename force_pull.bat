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
echo   FORCE PULL  (cloud  -^>  this computer)
echo   Repo    : %cd%
echo   Branch  : !BRANCH!
echo   Remote  : origin
echo.
echo   This DISCARDS every local commit and uncommitted change on this
echo   branch, and DELETES every untracked file/folder in this repo
echo   (git reset --hard + git clean -fd), then makes this folder match
echo   origin/!BRANCH! exactly. Anything only on this machine is LOST.
echo ================================================================
set /p CONFIRM="Continue? (Y/N): "
if /i not "!CONFIRM!"=="Y" (
    echo Cancelled, nothing was done.
    pause
    exit /b 0
)

echo.
echo Fetching origin/!BRANCH!...
git fetch origin !BRANCH!
if errorlevel 1 (
    echo.
    echo [ERROR] Fetch failed - check network / credentials / remote URL.
    pause
    exit /b 1
)

git reset --hard origin/!BRANCH!
git clean -fd

echo.
echo [OK] This folder now matches origin/!BRANCH!.
pause
