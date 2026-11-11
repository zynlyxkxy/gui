@echo off
rem ============================================================
rem  Build Release and install the deploy shape into output\exe\
rem  (the folder the top level copies wholesale into exe\apps\gui\).
rem  Touches no git, no other project - usable standalone.
rem
rem  gui needs Qt WebEngine: the platform base library ships a Qt without it,
rem  so istar_deploy_webengine() adds 19 items from the Qt installation.
rem  build\Release\ is self-contained (Qt + platforms + WebEngine + ISTAR
rem  + libUI* + config), so both IDEs can start GUI_WM.exe directly.
rem
rem  Usage:
rem    build-release.bat                build everything + install (default)
rem    build-release.bat -t <target>    build only that target (+ its deps),
rem                                     no install
rem
rem  -t is for VSCode launch configurations: building ALL targets would relink
rem  the exe of the other configuration that is still running -> LNK1168 ->
rem  the launch is aborted. See the same note in build-debug.bat.
rem  The CMake target is "GUI"; only the product is called GUI_WM.exe.
rem
rem  NOTE: .bat files must stay ASCII-only. cmd parses them with the
rem  console code page (GBK/936 here), so UTF-8 Chinese comments can
rem  swallow the line ending and corrupt the next line - depending on
rem  byte alignment. Chinese explanations live in the CMakeLists files.
rem ============================================================
setlocal
set "HERE=%~dp0"
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "TARGET="

call :main %*
set "RC=%ERRORLEVEL%"

if defined TARGET (
    set "SUMMARY_NOTE=Release build (single target: %TARGET%, no install)"
    call "%HERE%private_scripts\_lib.bat" summary %RC% "%ROOT%\build\Release\GUI_WM.exe"
    exit /b %RC%
)

set "SUMMARY_NOTE=Release build"
call "%HERE%private_scripts\_lib.bat" summary %RC% ^
    "%ROOT%\build\Release\GUI_WM.exe" ^
    "%ROOT%\build\Release\mctrl.dll" ^
    "%ROOT%\build\Release\QtWebEngineProcess.exe" ^
    "%ROOT%\build\Release\qtwebengine_locales" ^
    "%ROOT%\build\Release\libUIRE_CHIP.dll" ^
    "%ROOT%\build\Release\libUIRE_Wafer.dll" ^
    "%ROOT%\build\Release\libUIRm.dll" ^
    "%ROOT%\output\exe\GUI_WM.exe" ^
    "%ROOT%\output\exe\mctrl.dll" ^
    "%ROOT%\output\exe\qt.conf" ^
    "%ROOT%\output\exe\qtwebengine_locales" ^
    "--" ^
    "%ROOT%\build\Release\libUIRE_CHIPd.dll" ^
    "%ROOT%\build\Release\libUIRmd.dll"
exit /b %RC%

:main
cd /d "%ROOT%" || exit /b 1
call "%HERE%private_scripts\_lib.bat" check_qtdir

if /i "%~1"=="-t"       set "TARGET=%~2"
if /i "%~1"=="--target" set "TARGET=%~2"

rem output\ is produced by the "deploy" target that is part of the default
rem build (see cmake\istar_install.cmake), so no separate cmake --install here.
cmake --preset win64           || exit /b 1
if defined TARGET (
    cmake --build --preset release --target %TARGET% || exit /b 1
    exit /b 0
)
cmake --build --preset release || exit /b 1
exit /b 0
