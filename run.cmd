@echo off
rem ============================================================================
rem MODS: one-click configure + build + launch for this private fork.
rem See docs/MODDING_NOTES.md section 7 for what the flags mean.
rem
rem Usage:
rem   run.cmd                 configure (if needed) + build + launch
rem   run.cmd <file>          ... and open <file> on start
rem   run.cmd --build         build only, don't launch
rem   run.cmd --reconfigure   force a fresh cmake configure, then build + launch
rem   run.cmd --clean         delete build\ and start over
rem ============================================================================
setlocal EnableDelayedExpansion

set "REPO=%~dp0"
set "REPO=%REPO:~0,-1%"
set "BUILD=%REPO%\build"
set "SKIA=C:/Users/Cherry/deps/skia"
set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

rem ---- parse args -------------------------------------------------------------
set "LAUNCH=1"
set "RECONFIGURE=0"
set "OPENFILE="

:parse
if "%~1"=="" goto parsed
if /i "%~1"=="--build"       ( set "LAUNCH=0"      & shift & goto parse )
if /i "%~1"=="--reconfigure" ( set "RECONFIGURE=1" & shift & goto parse )
if /i "%~1"=="--clean" (
    echo [run] removing "%BUILD%"
    if exist "%BUILD%" rmdir /s /q "%BUILD%"
    set "RECONFIGURE=1"
    shift & goto parse
)
set "OPENFILE=%~1"
shift
goto parse
:parsed

rem ---- sanity checks ----------------------------------------------------------
rem NOTE: echo !VCVARS! must use delayed expansion -- the path contains "(x86)",
rem and with %VCVARS% the closing paren would terminate this if-block early.
if not exist "%VCVARS%" (
    echo [run] ERROR: vcvars64.bat not found at:
    echo        "!VCVARS!"
    echo        Install "Visual Studio 2022 Build Tools" with the C++ workload,
    echo        or edit the VCVARS line in this script.
    exit /b 1
)
if not exist "%SKIA%/out/Release-x64/skia.lib" (
    echo [run] ERROR: Skia not found at %SKIA%
    echo        Expected %SKIA%/out/Release-x64/skia.lib
    echo        See docs/MODDING_NOTES.md section 7.2 for the download URL.
    exit /b 1
)

rem ---- MSVC environment ------------------------------------------------------
where cl.exe >nul 2>nul
if errorlevel 1 (
    echo [run] loading MSVC environment...
    call "%VCVARS%" >nul
    if errorlevel 1 (
        echo [run] ERROR: vcvars64.bat failed.
        exit /b 1
    )
)

cd /d "%REPO%" || exit /b 1

rem ---- configure -------------------------------------------------------------
rem A missing build.ninja means we have never configured. Note that data/ files
rem are picked up by a configure-time file(GLOB_RECURSE), so adding a new file
rem under data/ requires --reconfigure before it lands in build\bin\data.
if not exist "%BUILD%\build.ninja" set "RECONFIGURE=1"

if "%RECONFIGURE%"=="1" (
    echo [run] configuring...
    cmake -B "%BUILD%" -G Ninja ^
      -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
      -DLAF_BACKEND=skia ^
      -DSKIA_DIR=%SKIA% ^
      -DSKIA_LIBRARY_DIR=%SKIA%/out/Release-x64 ^
      -DSKIA_LIBRARY=%SKIA%/out/Release-x64/skia.lib ^
      -DENABLE_PSD=ON ^
      -DENABLE_MODS=ON ^
      -DENABLE_I18N_STRINGS=ON
    if errorlevel 1 (
        echo [run] ERROR: cmake configure failed.
        exit /b 1
    )
)

rem ---- stop a running instance ------------------------------------------------
rem The linker cannot overwrite bin\aseprite.exe while it is running, and fails
rem with "LNK1104: cannot open file". Only our own build is killed, never an
rem installed Aseprite: the image name is matched against this build's path.
tasklist /fi "imagename eq aseprite.exe" 2>nul | find /i "aseprite.exe" >nul
if not errorlevel 1 (
    echo [run] a running aseprite.exe would block the linker, closing it...
    taskkill /f /im aseprite.exe >nul 2>nul
    rem Give the OS a moment to release the file handle.
    ping -n 2 127.0.0.1 >nul
)

rem ---- build -----------------------------------------------------------------
echo [run] building...
ninja -C "%BUILD%" aseprite
set "BUILDRC=%ERRORLEVEL%"
if not "%BUILDRC%"=="0" (
    echo [run] ERROR: build failed ^(ninja exit code %BUILDRC%^).
    exit /b %BUILDRC%
)

if not exist "%BUILD%\bin\aseprite.exe" (
    echo [run] ERROR: aseprite.exe was not produced.
    exit /b 1
)

echo [run] build OK -^> %BUILD%\bin\aseprite.exe

rem ---- launch ----------------------------------------------------------------
if "%LAUNCH%"=="0" (
    echo [run] --build given, not launching.
    exit /b 0
)

echo [run] launching...
cd /d "%BUILD%\bin"
if defined OPENFILE (
    start "" "aseprite.exe" "%OPENFILE%"
) else (
    start "" "aseprite.exe"
)
exit /b 0
