@echo off
rem ============================================================================
rem MODS: build a portable, self-contained folder + zip of this fork.
rem
rem   package.cmd            build (if needed) and package into dist\
rem   package.cmd --nobuild  package whatever is already in build\bin
rem
rem The result runs from any folder with no installation: everything links
rem statically, so only Windows' own DLLs are needed.
rem
rem NOTE Aseprite's EULA section 2(b) forbids distributing compiled copies to
rem third parties, and 2(g) limits compiling it to your own personal purpose.
rem This packages the build for your own machines; what you do with it is your
rem call as the licensee.
rem ============================================================================
setlocal EnableDelayedExpansion

set "REPO=%~dp0"
set "REPO=%REPO:~0,-1%"
set "BIN=%REPO%\build\bin"
set "DIST=%REPO%\dist"

if /i "%~1"=="--nobuild" goto :nobuild
call "%REPO%\run.cmd" --build
if errorlevel 1 (
    echo [pkg] ERROR: build failed, not packaging.
    exit /b 1
)
:nobuild

if not exist "%BIN%\aseprite.exe" (
    echo [pkg] ERROR: %BIN%\aseprite.exe not found. Run run.cmd --build first.
    exit /b 1
)

rem ---- version, from the executable itself ------------------------------------
set "VER="
for /f "tokens=2" %%v in ('"%BIN%\aseprite.exe" --version') do set "VER=%%v"
if "!VER!"=="" set "VER=unknown"
set "NAME=Aseprite-mods-!VER!-win64"
set "STAGE=%DIST%\!NAME!"

echo [pkg] version  : !VER!
echo [pkg] staging  : !STAGE!

if exist "!STAGE!" rmdir /s /q "!STAGE!"
mkdir "!STAGE!" 2>nul

rem ---- payload ----------------------------------------------------------------
rem Only what the program needs at run time. Excluded on purpose:
rem   *.pdb          debug symbols, ~160 MB and useless to a user
rem   gen.exe        build-time code generator
rem   data\strings.git  empty leftover from ENABLE_I18N_STRINGS
rem   *.lua *.psd    scratch files from testing
copy /y "%BIN%\aseprite.exe" "!STAGE!\" >nul
if exist "%BIN%\icudtl.dat" copy /y "%BIN%\icudtl.dat" "!STAGE!\" >nul

robocopy "%BIN%\data" "!STAGE!\data" /e /njh /njs /ndl /nc /ns /np /xd "strings.git" >nul
if errorlevel 8 (
    echo [pkg] ERROR: copying data\ failed.
    exit /b 1
)

rem ---- the notes that ship with it --------------------------------------------
call :writereadme "!STAGE!\README.txt" "!VER!"

rem ---- zip --------------------------------------------------------------------
set "ZIP=%DIST%\!NAME!.zip"
if exist "!ZIP!" del /q "!ZIP!"
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "Compress-Archive -Path '!STAGE!' -DestinationPath '!ZIP!' -CompressionLevel Optimal" || (
    echo [pkg] ERROR: Compress-Archive failed.
    exit /b 1
)

rem ---- update manifest ---------------------------------------------------------
rem The sidecar the in-app updater reads before downloading: it refuses to
rem install a package whose sha256 does not match this file. Upload it next to
rem the zip, named "<zip name>.json". See docs/MODDING_NOTES.md 33.
set "MANIFEST=!ZIP!.json"
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$f = Get-Item '!ZIP!';" ^
  "$h = (Get-FileHash -Algorithm SHA256 $f.FullName).Hash.ToLower();" ^
  "$j = [ordered]@{ sha256 = $h; size = $f.Length; version = '!VER!' } | ConvertTo-Json;" ^
  "[System.IO.File]::WriteAllText('!MANIFEST!', $j)" || (
    echo [pkg] ERROR: could not write the update manifest.
    exit /b 1
)

echo.
echo [pkg] folder   : !STAGE!
echo [pkg] zip      : !ZIP!
for %%f in ("!ZIP!") do echo [pkg] size     : %%~zf bytes
echo [pkg] manifest : !MANIFEST!
exit /b 0

rem ============================================================================
:writereadme
> "%~1" echo Aseprite %~2 -- private build
>>"%~1" echo ============================================
>>"%~1" echo.
>>"%~1" echo A portable build of Aseprite compiled from a modified source tree.
>>"%~1" echo Unzip anywhere and run aseprite.exe. No installation, no runtime to
>>"%~1" echo install: everything is linked statically.
>>"%~1" echo.
>>"%~1" echo Settings live in %%APPDATA%%\Aseprite, the same place the official
>>"%~1" echo build uses, so the two share preferences and installed extensions.
>>"%~1" echo.
>>"%~1" echo What this build adds
>>"%~1" echo --------------------
>>"%~1" echo * Per-layer thumbnails in the timeline, with a header button to
>>"%~1" echo   toggle them. Ctrl+click a thumbnail to select that layer's
>>"%~1" echo   content as a selection (+Shift add, +Alt subtract).
>>"%~1" echo * Three gradient ramps above the palette. Click an end swatch to
>>"%~1" echo   store the foreground (left button) or background (right button)
>>"%~1" echo   color; pick from the bands the same way. View ^> Palette Bars.
>>"%~1" echo * Opacity for the pencil and the other plain paint tools, in the
>>"%~1" echo   tool options bar.
>>"%~1" echo * .psd support: open and save. Layers, groups, blend modes, opacity,
>>"%~1" echo   visibility and layer masks all survive the round trip, and both
>>"%~1" echo   RLE- and ZIP-compressed files open. Several import bugs are fixed:
>>"%~1" echo   non-Latin layer names no longer crash on open and non-square
>>"%~1" echo   files open at the right size. Saving writes the current frame.
>>"%~1" echo * The pixel font used by the official release, so Chinese and other
>>"%~1" echo   scripts render at the same size and baseline as Latin text.
>>"%~1" echo * The 23 languages the official release ships, plus three written
>>"%~1" echo   in invented scripts: Sarkaz, Seaborn and Far North Runes. Those
>>"%~1" echo   ship a real translation and get their script from a font, so the
>>"%~1" echo   words are ordinary English or Norwegian, just unreadable.
>>"%~1" echo   Edit ^> Preferences ^> General ^> Language.
>>"%~1" echo * Scripts can dock a panel in the main window with app.panel{}.
>>"%~1" echo * Update checks and in-app updating go to our own server, not to
>>"%~1" echo   aseprite.org. A download is installed only if it matches the
>>"%~1" echo   checksum the server published.
>>"%~1" echo.
>>"%~1" echo Known limitations
>>"%~1" echo -----------------
>>"%~1" echo * PSD: no adjustment layers, no layer effects, no CMYK or Lab color
>>"%~1" echo   and no 32-bit channels. Files using those report an error on open
>>"%~1" echo   rather than opening wrong. Saving writes one frame, since PSD has
>>"%~1" echo   no equivalent of Aseprite's frames.
>>"%~1" echo * Palette ramp colors are written when Aseprite exits normally; a
>>"%~1" echo   forced kill loses them.
>>"%~1" echo.
>>"%~1" echo License
>>"%~1" echo -------
>>"%~1" echo Aseprite is licensed by Igara Studio S.A. under its EULA, not under
>>"%~1" echo an open-source license. Section 2(b) says copies may not be
>>"%~1" echo distributed to third parties, and 2(g) allows compiling the source
>>"%~1" echo only for your own personal purpose. If you want Aseprite, buy it:
>>"%~1" echo   https://www.aseprite.org/
>>"%~1" echo.
>>"%~1" echo Upstream source: https://github.com/aseprite/aseprite
exit /b 0
