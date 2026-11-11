@echo off
rem ============================================================
rem  Remove this project's build tree and deploy shape (clean state).
rem  ASCII-only on purpose: see the note in build-release.bat.
rem ============================================================
setlocal
set "HERE=%~dp0"
for %%I in ("%~dp0..") do set "ROOT=%%~fI"

call :main %*
set "RC=%ERRORLEVEL%"
set "SUMMARY_NOTE=build\ and output\ removed"

call "%HERE%private_scripts\_lib.bat" summary %RC%
exit /b %RC%

:main
cd /d "%ROOT%" || exit /b 1
if exist "%ROOT%\build"  rmdir /s /q "%ROOT%\build"  || exit /b 1
if exist "%ROOT%\output" rmdir /s /q "%ROOT%\output" || exit /b 1
exit /b 0
