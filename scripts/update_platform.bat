@echo off
rem ============================================================
rem  update_platform.bat - align THIS project's platform mounts to a
rem  version. gui owns its two platform mounts; the top level only
rem  delegates here (see scripts\update_platform.bat).
rem
rem    update_platform.bat           align to the highest version tag
rem                                    on origin/master (= "latest")
rem    update_platform.bat 2.0.1     align to tag 2.0.1
rem    update_platform.bat latest    same as no argument
rem    update_platform.bat show      show the current version, change nothing
rem    update_platform.bat -h        this help
rem
rem  Mounts this project owns (see ..\.gitmodules):
rem    basiclibrary_output   platform base library (ISTAR*.dll + a Qt copy)
rem    platform_output       platform module libraries (libUI*, mc/eh/rm/rp/ua)
rem  Nothing outside them is touched - the other levels own their mounts.
rem  gui owns no third-party submodule: that is scripts\update_thirdparty.bat.
rem
rem  Version policy: highest version tag reachable from origin/master
rem  (tags created on test/release branches are never picked).
rem
rem  NON-destructive: a plain "git checkout <tag>". If the worktree is
rem  dirty the checkout fails - we print the error and exit non-zero. We
rem  never discard a local change: fix it and re-run.
rem
rem  Wave 3 renamed this file (name only); the contract and the delegation
rem  wiring are unchanged.
rem
rem  ASCII-only on purpose - see the note in build-release.bat.
rem ============================================================
setlocal enabledelayedexpansion
set "HERE=%~dp0"
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "MOWN=basiclibrary_output platform_output"

rem  Quiet mode: the top level (scripts\update_platform.bat) drives us with
rem  _CHILD_QUIET=1 and owns the summary, so we print no report block of our
rem  own - only the per-mount lines and the [ERROR] lines. Run this script
rem  directly (no _CHILD_QUIET) and the full report is printed as before.
set "_QUIET="
if "%_CHILD_QUIET%"=="1" set "_QUIET=1"

call :main %*
set "RC=%ERRORLEVEL%"
if defined SUB_TAG (
    set "SUMMARY_NOTE=submodules aligned to !SUB_TAG!"
) else (
    set "SUMMARY_NOTE=submodule alignment"
)
if defined FAILED_SUB set "SUMMARY_NOTE=failed on !FAILED_SUB! - no change was discarded"

if not defined _QUIET call "%HERE%private_scripts\_lib.bat" summary %RC% ^
    "%ROOT%\basiclibrary_output\.git" ^
    "%ROOT%\platform_output\.git"
exit /b %RC%

:main
cd /d "%ROOT%" || exit /b 1
set "SUB_TAG="

set "VER=%~1"
if /i "%VER%"=="-h" goto :usage
if /i "%VER%"=="/?" goto :usage
if /i "%VER%"=="show" goto :show
if "%VER%"=="" set "VER=latest"

where git >nul 2>nul
if errorlevel 1 (
    echo [ERROR] no git found
    exit /b 1
)

rem  All-or-nothing: the first mount that fails stops the second one.
call :align basiclibrary_output || exit /b 1
call :align platform_output      || exit /b 1

rem  The "[OK] ... aligned" trailer is part of the report, so it is silent
rem  when the top level drives us (_CHILD_QUIET=1): the parent prints the
rem  verdict for this project itself.
if not defined _QUIET (
    echo.
    echo [OK] update_platform: platform mounts aligned
)
exit /b 0

rem ============================================================
rem  align <submodule path> - pin it to %VER% (or to the highest tag).
rem  Records the failing mount in FAILED_SUB so the RESULT block names it.
rem ============================================================
:align
set "SUB=%~1"
set "DIR=%ROOT%\%SUB%"
set "TAG="
if /i not "%VER%"=="latest" set "TAG=%VER%"

echo.
echo ---- %SUB% -^> %VER% ----

if not exist "%DIR%\.git" (
    echo [1/4] %SUB% is not initialized - initializing it first ...
    git submodule update --init "%SUB%" || (
        echo [ERROR] failed to init submodule %SUB%
        set "FAILED_SUB=%SUB%"
        exit /b 1
    )
)

pushd "%DIR%" || exit /b 1

echo [2/4] fetch tags ...
git fetch --tags origin || (
    echo [ERROR] %SUB%: git fetch failed - network or credentials?
    set "FAILED_SUB=%SUB%"
    popd
    exit /b 1
)

if defined TAG goto :align_have_tag
rem  --merged origin/master keeps tags made on a test/release branch out
set "TAG="
for /f %%t in ('git tag --sort=-v:refname --merged origin/master 2^>nul') do (
    set "TAG=%%t"
    goto :align_latest
)
:align_latest
if not defined TAG (
    echo [ERROR] %SUB%: no version tag reachable from origin/master
    set "FAILED_SUB=%SUB%"
    popd
    exit /b 1
)
echo       latest tag on master = !TAG!
:align_have_tag

echo [3/4] clear skip-worktree flags ...
rem  A flagged file is invisible to the checkout, which would then
rem  overwrite it without a word. Clear the flags so a local change makes
rem  the checkout fail instead of being silently lost.
for /f "tokens=1,*" %%a in ('git ls-files -v 2^>nul') do (
    if "%%a"=="S" git update-index --no-skip-worktree "%%b" >nul 2>nul
)

git rev-parse --verify "refs/tags/!TAG!" >nul 2>nul || (
    echo [ERROR] %SUB%: tag !TAG! does not exist in this submodule
    set "FAILED_SUB=%SUB%"
    popd
    exit /b 1
)

echo [4/4] checkout !TAG! ...
git checkout "!TAG!" || (
    echo [ERROR] %SUB%: git checkout !TAG! failed - dirty worktree? no change was discarded
    set "FAILED_SUB=%SUB%"
    popd
    exit /b 1
)

for /f %%t in ('git describe --tags --abbrev^=0 2^>nul') do echo [OK] %SUB% : %%t
set "SUB_TAG=!TAG!"
popd
exit /b 0

:show
rem  Quiet mode: the banner and the hint block are report furniture, so the
rem  top level suppresses them (_CHILD_QUIET=1) and prints its own one-line
rem  header instead. The per-mount lines below are the data and always print.
if not defined _QUIET (
    echo.
    echo ============================================================
    echo  gui platform mounts - current versions
    echo ============================================================
)
call :show_one "basiclibrary_output"
call :show_one "platform_output"
if not defined _QUIET (
    echo ============================================================
    echo  To just look:        update_platform.bat show
    echo  To chase the newest: update_platform.bat          ^(default, = latest^)
    echo  To pin a version:    update_platform.bat 2.0.1
    echo ============================================================
)
exit /b 0

:show_one
set "M=%~1"
set "T=no tag"
if exist "%ROOT%\%M%\.git" (
    for /f %%t in ('git -C "%ROOT%\%M%" describe --tags --abbrev^=0 2^>nul') do set "T=%%t"
) else (
    set "T=not a repo"
)
echo   %M%  :  %T%
exit /b 0

:usage
echo.
echo Usage: update_platform.bat [^<tag^>^|latest^|show] [-h]
echo.
echo   no argument : align to the highest tag on origin/master ^(= latest^)
echo   2.0.1       : align to tag 2.0.1
echo   latest      : same as no argument
echo   show        : only show the current version, change nothing
echo.
echo   Owns ^(this level^): %MOWN%
echo   Other levels own theirs - this script does not run git on those.
echo.
echo   Non-destructive: a plain "git checkout" - a dirty worktree makes
echo   the run fail rather than being silently discarded.
echo.
echo Examples:
echo   update_platform.bat
echo   update_platform.bat 2.0.1
echo   update_platform.bat show
echo.
exit /b 0
