# ============================================================
#  PyRecT3D  Windows 一键构建脚本 (Win7 兼容)
#  使用 CMake + NMake，源码优先编译，无预编译二进制
# ============================================================
@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo ==========================================
echo   PyRecT3D Build (Windows / Win7+)
echo ==========================================

REM ---- 检查 cmake ----
where cmake >nul 2>nul
if errorlevel 1 (
  echo [ERROR] cmake not found. Install CMake and add to PATH.
  exit /b 1
)

REM ---- 检查 python ----
where python >nul 2>nul
if errorlevel 1 (
  echo [WARN] python not found, Python binding will be skipped.
  set PYTHON_FOUND=0
) else (
  set PYTHON_FOUND=1
)

REM ---- 配置 VS 环境（如需 MSVC）----
REM 若已配置则跳过；此处留给用户手动 call vcvars64.bat

REM ---- 生成 ----
if not exist build mkdir build
pushd build

cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
  echo [ERROR] cmake configure failed.
  popd
  exit /b 1
)

nmake
if errorlevel 1 (
  echo [ERROR] nmake build failed.
  popd
  exit /b 1
)

popd

echo.
echo ==========================================
echo   BUILD OK. Outputs:
echo     core dll : build/PyRecT3D.dll (+ .lib)
echo     python   : py/package/ (if pybind11 present)
echo ==========================================
endlocal
