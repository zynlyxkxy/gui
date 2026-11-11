@echo off
rem ============================================================
rem  Sync this project's inputs (gui has no out-edges - it is the end of
rem  the chain, nothing consumes gui's artifacts):
rem    A5  ReData  -> gui\MainProject\MCRecipeModule\ReData        (mirror)
rem    A7  mctrl protocol -> gui\MainProject\RPCModule\include
rem    A8  McData  -> gui\MainProject\MCRecipeModule\McData        (mirror)
rem
rem  The engine lives at the top level (1927project\scripts\private_scripts\_sync_ops.bat) so
rem  all cross-project writes follow one table; this is only a forward.
rem  Identical content is skipped WITHOUT touching the target mtime, so a
rem  rebuild is not needlessly triggered.
rem
rem  The old build.bat "-D" step copied output\deploy into 1927project\exe.
rem  That is assembly, not an inter-project sync edge, and it now belongs to
rem  the top-level deploy entry (deploy.bat / deploy_ops.bat assemble).
rem
rem  Extra arguments are passed through, e.g.:
rem      sync.bat --dry-run      report only, write nothing
rem
rem  ASCII-only on purpose: see the note in build-release.bat.
rem ============================================================
setlocal
set "HERE=%~dp0"
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "TOP=%ROOT%\..\engineering\scripts"

if not exist "%TOP%\private_scripts\_sync_ops.bat" (
    echo [SKIP] top-level sync engine not found: %TOP%\private_scripts\_sync_ops.bat
    echo        Nothing was synced. Build the top level, or run private_scripts\_sync_ops.bat there.
    exit /b 0
)

call "%TOP%\private_scripts\_sync_ops.bat" proj gui %*
set "RC=%ERRORLEVEL%"
call "%HERE%private_scripts\_lib.bat" summary %RC% ^
    "%ROOT%\MainProject\RPCModule\include\mctrl_controller.h" ^
    "%ROOT%\MainProject\MCRecipeModule\ReData\S_Recipe.h" ^
    "%ROOT%\MainProject\MCRecipeModule\McData\S_MachineConstant.h"
exit /b %RC%
