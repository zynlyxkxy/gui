@echo off
rem ============================================================
rem  Build Debug. build\Debug\ is self-contained (Qt + platforms +
rem  WebEngine debug variants + ISTAR + libUI* + config).
rem
rem  The deploy shape (output\exe\) is Release-only: its install rules carry
rem  CONFIGURATIONS Release, so a Debug install cannot pollute output\exe\.
rem  gui has no downstream hand-off package, so nothing lands in
rem  output\interface\ either - the Debug run is driven straight from
rem  build\Debug\GUI_WM.exe by both IDEs.
rem
rem  Usage:
rem    build-debug.bat                  build everything (default)
rem    build-debug.bat -t <target>      build only that target (+ its deps)
rem
rem  Why -t exists: VSCode runs a build before every launch (preLaunchTask).
rem  Building ALL targets would relink the exe of the OTHER configuration that
rem  is still running, and that fails with
rem      LINK : fatal error LNK1168: cannot open <exe> for writing
rem  after which VSCode aborts the launch. So each launch configuration builds
rem  only the target it is about to start (see .vscode\tasks.json).
rem  The CMake target is "GUI"; only the product is called GUI_WM.exe.
rem
rem  ASCII-only on purpose - see the note in build-release.bat.
rem ============================================================
setlocal
set "HERE=%~dp0"
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "TARGET="

call :main %*
set "RC=%ERRORLEVEL%"

if defined TARGET (
    set "SUMMARY_NOTE=Debug build (single target: %TARGET%)"
    call "%HERE%private_scripts\_lib.bat" summary %RC% "%ROOT%\build\Debug\GUI_WM.exe"
    exit /b %RC%
)

set "SUMMARY_NOTE=Debug build"
call "%HERE%private_scripts\_lib.bat" summary %RC% ^
    "%ROOT%\build\Debug\GUI_WM.exe" ^
    "%ROOT%\build\Debug\mctrld.dll" ^
    "%ROOT%\build\Debug\QtWebEngineProcessd.exe" ^
    "%ROOT%\build\Debug\qtwebengine_locales" ^
    "%ROOT%\build\Debug\libUIRE_CHIPd.dll" ^
    "%ROOT%\build\Debug\libUIRE_Waferd.dll" ^
    "%ROOT%\build\Debug\libUIRmd.dll" ^
    "--" ^
    "%ROOT%\build\Debug\libUIRE_CHIP.dll" ^
    "%ROOT%\output\exe\GUI_WM.exe"
exit /b %RC%

:main
cd /d "%ROOT%" || exit /b 1
call "%HERE%private_scripts\_lib.bat" check_qtdir

if /i "%~1"=="-t"       set "TARGET=%~2"
if /i "%~1"=="--target" set "TARGET=%~2"

rem Every install rule is CONFIGURATIONS Release, so the Debug "deploy" target
rem writes nothing into output\ - run scripts\build-release.bat for that.
cmake --preset win64         || exit /b 1
if defined TARGET (
    cmake --build --preset debug --target %TARGET% || exit /b 1
    exit /b 0
)
cmake --build --preset debug || exit /b 1
exit /b 0
