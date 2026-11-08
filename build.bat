@echo off
REM ============================================================
REM  PyRecT3D v0.2 — Windows 交互式一键构建（Win7 兼容）
REM  产物形态二选一：
REM    1) DLL —— 动态库，调用侧只用轻量头文件 + LoadLibrary
REM    2) LIB —— 静态库，直接链接进工程
REM  交互项：CL 编译器路径 / 构建配置 / 模块裁剪 / FFmpeg 开关
REM ============================================================
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo ==================================================
echo    PyRecT3D v0.2 Build  (Windows / Win7+)
echo ==================================================
echo.

REM ---------- 1) 选择产物类型 ----------
echo [1/5] 请选择产物类型：
echo   1 = DLL  (动态库，LoadLibrary 加载)
echo   2 = LIB  (静态库，直接链接)
set /p LIBTYPE=请输入 1 或 2：
if "%LIBTYPE%"=="2" (
  set PYRECT3D_LIB_TYPE=static
  echo   -^> 将构建静态库 LIB
) else (
  set PYRECT3D_LIB_TYPE=shared
  echo   -^> 将构建动态库 DLL
)
echo.

REM ---------- 2) CL 编译器路径 ----------
echo [2/5] 请输入 CL 编译器路径（直接回车 = 使用 PATH 中的 cl.exe）
echo        示例：C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.29.30133\bin\Hostx64\x64
set /p CLPATH=CL 路径：
if "%CLPATH%"=="" (
  echo   -^> 使用 PATH 中的 cl.exe
) else (
  set "PATH=%CLPATH%;%PATH%"
  echo   -^> 已前置 CL 路径：%CLPATH%
)
echo.

REM ---------- 3) 构建配置 ----------
echo [3/5] 请选择构建配置：
echo   1 = Release （O2 + 全程序优化，体积最小）
echo   2 = Debug   （带调试符号，方便排错）
set /p CFG=请输入 1 或 2：
if "%CFG%"=="2" (
  set CMAKE_BUILD_TYPE=Debug
) else (
  set CMAKE_BUILD_TYPE=Release
)
echo.

REM ---------- 4) 模块裁剪 ----------
echo [4/5] 是否裁剪模块？（可显著减小最终库体积）
echo   注意：被裁掉的模块不参与编译，相关 API 调用返回 false / nullptr
set /p TRIM=是否裁剪 (y/n):
set PYRECT3D_WITH_GUI=ON
set PYRECT3D_WITH_VIDEO=ON
set PYRECT3D_WITH_AUDIO=ON
set PYRECT3D_WITH_PHYSICS=ON
set PYRECT3D_WITH_RAYTRACER=ON
if /i "%TRIM%"=="y" (
  set /p KGUI=  保留 GUI 模块        (y/n):
  set /p KVIDEO=  保留 视频模块       (y/n):
  set /p KAUDIO=  保留 音频模块       (y/n):
  set /p KPHYS=  保留 物理/碰撞模块  (y/n):
  set /p KRT=  保留 软光追模块    (y/n):
  if /i not "!KGUI!"=="y"   set PYRECT3D_WITH_GUI=OFF
  if /i not "!KVIDEO!"=="y" set PYRECT3D_WITH_VIDEO=OFF
  if /i not "!KAUDIO!"=="y" set PYRECT3D_WITH_AUDIO=OFF
  if /i not "!KPHYS!"=="y"  set PYRECT3D_WITH_PHYSICS=OFF
  if /i not "!KRT!"=="y"    set PYRECT3D_WITH_RAYTRACER=OFF
)
echo.

REM ---------- 5) FFmpeg ----------
echo [5/5] 是否启用外部 FFmpeg 解码？（未启用时使用内置占位帧）
set /p FFMPEG=启用 FFmpeg (y/n):
if /i "%FFMPEG%"=="y" (
  set PYRECT3D_WITH_FFMPEG=ON
) else (
  set PYRECT3D_WITH_FFMPEG=OFF
)
echo.

REM ---------- 依赖检查 ----------
where cmake >nul 2>nul
if errorlevel 1 (
  echo [ERROR] 未找到 cmake，请安装 CMake 并加入 PATH。
  exit /b 1
)

REM ---------- 生成 ----------
if not exist build mkdir build
pushd build

cmake .. -G "NMake Makefiles" ^
  -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% ^
  -DPYRECT3D_LIB_TYPE=%PYRECT3D_LIB_TYPE% ^
  -DPYRECT3D_WITH_GUI=%PYRECT3D_WITH_GUI% ^
  -DPYRECT3D_WITH_VIDEO=%PYRECT3D_WITH_VIDEO% ^
  -DPYRECT3D_WITH_AUDIO=%PYRECT3D_WITH_AUDIO% ^
  -DPYRECT3D_WITH_PHYSICS=%PYRECT3D_WITH_PHYSICS% ^
  -DPYRECT3D_WITH_RAYTRACER=%PYRECT3D_WITH_RAYTRACER% ^
  -DPYRECT3D_WITH_FFMPEG=%PYRECT3D_WITH_FFMPEG% ^
  -DPYRECT3D_BUILD_EXAMPLES=ON ^
  -DPYRECT3D_BUILD_TESTS=ON

if errorlevel 1 (
  echo [ERROR] cmake configure 失败
  popd
  exit /b 1
)

nmake
if errorlevel 1 (
  echo [ERROR] nmake 构建失败
  popd
  exit /b 1
)

popd
echo.
echo ==================================================
echo    BUILD OK
echo      lib type : %PYRECT3D_LIB_TYPE%
echo      config   : %CMAKE_BUILD_TYPE%
echo      output   : build\PyRecT3D.dll / build\PyRecT3D.lib
echo      test     : build\smoke_v2.exe
echo ==================================================
endlocal
