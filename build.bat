@echo off
REM ============================================================
REM  PyRecT3D v0.2 — 交互式构建（Windows / Win7 及以上）
REM
REM  这一版彻底不依赖 CMake 与 NMake：
REM    只用 MSVC 的 cl.exe / link.exe / lib.exe 直接编译源码，
REM    产物在 build\ 下，DLL 与 LIB 二选一。
REM
REM  为什么提示文字是英文：Windows 控制台代码页不稳定
REM  （GBK 与 UTF-8 混用会乱码），英文提示在任何代码页都能读。
REM  中文只出现在这些 REM 注释里，不影响运行。
REM ============================================================
setlocal enabledelayedexpansion
cd /d "%~dp0"
title PyRecT3D v0.2 build

set "SRC=cpp\src"
set "INC=cpp\include"
set "INCSRC=cpp\src"
set "GLAD=cpp\third_party\glad"
set "OBJ=build\obj"

REM ---------- defaults (press Enter to accept) ----------
set "LIBTYPE=DLL"
set "CFG=Release"
set "GL=0"
set "WITH_GUI=1"
set "WITH_VIDEO=1"
set "WITH_AUDIO=1"
set "WITH_PHYSICS=1"
set "PURGE=n"
set "BUILD_EXAMPLES=n"
set "CL=cl"
set "VSPATH="

echo ============================================================
echo   PyRecT3D v0.2 build   (no CMake, no NMake)
echo   compiler cl.exe   linker link.exe   archiver lib.exe
echo ============================================================
echo.

REM ============================================================
REM  STEP 1/6  artifact type
REM ============================================================
echo [1/6] Artifact type
echo        1 = DLL  (dynamic; caller uses only cpp\include + LoadLibraryA)
echo        2 = LIB  (static; caller adds /DPYRECT3D_STATIC=1 and links it)
set "ANS="
set /p "ANS=Choice [1]: "
if "%ANS%"=="2" set "LIBTYPE=LIB"
if "%LIBTYPE%"=="LIB" (echo         -^> static LIB) else (echo         -^> dynamic DLL)
echo.

REM ============================================================
REM  STEP 2/6  toolchain
REM ============================================================
echo [2/6] Toolchain
echo        Enter the folder containing cl.exe, or press Enter to
echo        auto-detect Visual Studio via vswhere, else use PATH.
echo        Example: C:\VS2022\VC\Tools\MSVC\14.29.30133\bin\Hostx64\x64
set "CLPATH="
set /p "CLPATH=cl.exe folder: "

if not "%CLPATH%"=="" (
  set "PATH=%CLPATH%;%PATH%"
  echo         -^> prepended %CLPATH%
  goto STEP3
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" for /f "delims=" %%i in ('"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath') do set "VSPATH=%%i"
if defined VSPATH if exist "!VSPATH!\VC\Auxiliary\Build\vcvarsall.bat" (
  echo         -^> found Visual Studio: !VSPATH!
  call "!VSPATH!\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul
  goto STEP3
)
echo         -^> no Visual Studio found, assuming cl.exe is on PATH

:STEP3
REM ============================================================
REM  STEP 3/6  configuration
REM ============================================================
echo.
echo [3/6] Configuration
echo        1 = Release (O2 + LTCG, smallest artifact)
echo        2 = Debug   (debug symbols, easier to trace)
set "ANS="
set /p "ANS=Choice [1]: "
if "%ANS%"=="2" (set "CFG=Debug") else (set "CFG=Release")
echo         -^> %CFG%
echo.

REM ============================================================
REM  STEP 4/6  graphics backend
REM ============================================================
echo [4/6] Graphics backend
echo        0 = software rasterizer only (no GL symbol touched, smallest)
echo        1 = OpenGL 3.3 Core / GLES 3.0 through glad (needs a context)
set "ANS="
set /p "ANS=GL backend 0/1 [0]: "
if "%ANS%"=="1" (set "GL=1") else (set "GL=0")
if "%GL%"=="1" (echo         -^> GL on, glad.c is compiled as well) else (echo         -^> GL off, software fallback only)
echo.

REM ============================================================
REM  STEP 5/6  module trim
REM ============================================================
echo [5/6] Module trim
echo        Trimmed modules are not compiled at all; their APIs then
echo        return false / nullptr at runtime. Press Enter to keep all.
set "ANS="
set /p "ANS=Keep GUI module? (y/n) [y]: "
if "%ANS%"=="n" set "WITH_GUI=0"
set "ANS="
set /p "ANS=Keep video module (FFmpeg dynamic material)? (y/n) [y]: "
if "%ANS%"=="n" set "WITH_VIDEO=0"
set "ANS="
set /p "ANS=Keep audio module? (y/n) [y]: "
if "%ANS%"=="n" set "WITH_AUDIO=0"
set "ANS="
set /p "ANS=Keep physics/collision module? (y/n) [y]: "
if "%ANS%"=="n" set "WITH_PHYSICS=0"
echo         -^> GUI=%WITH_GUI% VIDEO=%WITH_VIDEO% AUDIO=%WITH_AUDIO% PHYSICS=%WITH_PHYSICS%
echo.
echo        Also build the three C++ examples into build\ ? (y/n) [n]
set "ANS="
set /p "ANS=Examples: "
if "%ANS%"=="y" set "BUILD_EXAMPLES=y"
echo.

REM ============================================================
REM  STEP 6/6  confirm + optional purge
REM ============================================================
echo [6/6] Summary
echo        artifact : %LIBTYPE%
echo        config   : %CFG%
echo        GL       : %GL%
echo        modules  : GUI=%WITH_GUI% VIDEO=%WITH_VIDEO% AUDIO=%WITH_AUDIO% PHYSICS=%WITH_PHYSICS%
echo        examples : %BUILD_EXAMPLES%
echo.
echo        Delete trimmed source files from disk too?
echo        Default NO: they are only skipped at compile time.
echo        Answering y is irreversible without git restore.
set "ANS="
set /p "ANS=Purge trimmed sources? (y/n) [n]: "
if "%ANS%"=="y" set "PURGE=y"
echo.

REM ---------- clean output ----------
if exist build rmdir /s /q build
mkdir "%OBJ%" 2>nul

REM ---------- source list (keep in sync with CMakeLists.txt) ----------
set "FILES="
set "FILES=%FILES% %SRC%\foundation\math\math.cpp"
set "FILES=%FILES% %SRC%\foundation\io\obj_loader.cpp"
set "FILES=%FILES% %SRC%\core\engine.cpp"
set "FILES=%FILES% %SRC%\core\event_loop.cpp"
set "FILES=%FILES% %SRC%\core\message_queue.cpp"
set "FILES=%FILES% %SRC%\core\explicit_app.cpp"
set "FILES=%FILES% %SRC%\core\implicit_api.cpp"
set "FILES=%FILES% %SRC%\graphics\renderer.cpp"
set "FILES=%FILES% %SRC%\graphics\compositor.cpp"
set "FILES=%FILES% %SRC%\graphics\gl_backend.cpp"

if "%WITH_GUI%"=="1"     set "FILES=%FILES% %SRC%\ui\gui.cpp"
if "%WITH_VIDEO%"=="1"   set "FILES=%FILES% %SRC%\media\video.cpp"
if "%WITH_AUDIO%"=="1"   set "FILES=%FILES% %SRC%\audio\audio.cpp"
if "%WITH_PHYSICS%"=="1" set "FILES=%FILES% %SRC%\physics\collision_world.cpp"
if exist "%SRC%\core\platform\win7_input.cpp" set "FILES=%FILES% %SRC%\core\platform\win7_input.cpp"

REM ---------- compile flags ----------
set "DEFS=/DPYRECT3D_WITH_GL=%GL% /DPYRECT3D_WITH_GUI=%WITH_GUI% /DPYRECT3D_WITH_VIDEO=%WITH_VIDEO% /DPYRECT3D_WITH_AUDIO=%WITH_AUDIO% /DPYRECT3D_WITH_PHYSICS=%WITH_PHYSICS% /D_WIN32_WINNT=0x0601"
set "DEFS=%DEFS% /I"%INC%" /I"%INCSRC%" /I"%GLAD%""

if "%LIBTYPE%"=="DLL" (set "LIBDEF=/DPYRECT3D_BUILD_DLL=1") else (set "LIBDEF=/DPYRECT3D_STATIC=1")

if "%CFG%"=="Debug" (set "OPT=/Od /Zi /MDd /W3") else (set "OPT=/O2 /GL /MD /W3")

echo Compiling library sources ...
"%CL%" /nologo /c /std:c++17 /EHsc %OPT% %DEFS% %LIBDEF% /Fo"%OBJ%\\" %FILES%
if errorlevel 1 goto FAILED

if "%GL%"=="1" (
  echo Compiling glad.c ...
  "%CL%" /nologo /c /TC /O2 /MD %DEFS% /Fo"%OBJ%\glad.obj" "%GLAD%\glad.c"
  if errorlevel 1 goto FAILED
)

REM ---------- link / archive ----------
echo Linking ...
if "%LIBTYPE%"=="DLL" (
  set "LIBS=user32.lib gdi32.lib winmm.lib ole32.lib"
  if "%GL%"=="1" set "LIBS=%LIBS% opengl32.lib"
  if "%CFG%"=="Release" (
    link /nologo /DLL /LTCG /OUT:"build\PyRecT3D.dll" /IMPLIB:"build\PyRecT3D.lib" "%OBJ%\*.obj" !LIBS!
  ) else (
    link /nologo /DLL /DEBUG /OUT:"build\PyRecT3D.dll" /IMPLIB:"build\PyRecT3D.lib" "%OBJ%\*.obj" !LIBS!
  )
) else (
  lib /nologo /OUT:"build\PyRecT3D.lib" "%OBJ%\*.obj"
)
if errorlevel 1 goto FAILED

REM ---------- optional examples ----------
if "%BUILD_EXAMPLES%"=="y" (
  echo Building examples ...
  "%CL%" /nologo /std:c++17 /EHsc %OPT% /I"%INC%" examples_cpp\basic_cube.cpp /Fe"build\basic_cube.exe" /link /LIBPATH:"build" PyRecT3D.lib /SUBSYSTEM:CONSOLE
  "%CL%" /nologo /std:c++17 /EHsc %OPT% /I"%INC%" examples_cpp\explicit_demo.cpp /Fe"build\explicit_demo.exe" /link /LIBPATH:"build" PyRecT3D.lib /SUBSYSTEM:CONSOLE
  "%CL%" /nologo /std:c++17 /EHsc %OPT% /I"%INC%" examples_cpp\implicit_demo.cpp /Fe"build\implicit_demo.exe" /link /LIBPATH:"build" PyRecT3D.lib /SUBSYSTEM:CONSOLE
)

REM ---------- optional purge ----------
if "%PURGE%"=="y" (
  echo Purging trimmed sources ...
  if "%WITH_GUI%"=="0"     del /q "%SRC%\ui\gui.cpp"
  if "%WITH_VIDEO%"=="0"   del /q "%SRC%\media\video.cpp"
  if "%WITH_AUDIO%"=="0"   del /q "%SRC%\audio\audio.cpp"
  if "%WITH_PHYSICS%"=="0" del /q "%SRC%\physics\collision_world.cpp"
  echo         -^> trimmed sources deleted; use git restore to bring them back
)

echo.
echo ============================================================
echo   DONE
if "%LIBTYPE%"=="DLL" (echo   build\PyRecT3D.dll  +  build\PyRecT3D.lib ^(import lib^)) else (echo   build\PyRecT3D.lib)
echo.
echo   Caller side, DLL mode:
echo     include only cpp\include, then LoadLibraryA("PyRecT3D.dll")
echo     and resolve PYRECT3D_CALL entries with GetProcAddress.
echo   Caller side, LIB mode:
echo     add /DPYRECT3D_STATIC=1 to YOUR project, link build\PyRecT3D.lib
echo   GL=%GL%  GUI=%WITH_GUI% VIDEO=%WITH_VIDEO% AUDIO=%WITH_AUDIO% PHYSICS=%WITH_PHYSICS%
echo ============================================================
goto END

:FAILED
echo.
echo BUILD FAILED — read the cl.exe / link.exe message above.
exit /b 1

:END
endlocal
