#!/usr/bin/env bash
# ============================================================
#  PyRecT3D  Linux 一键构建脚本 (X11)
#  使用 CMake + make，源码优先编译，无预编译二进制
# ============================================================
set -e
cd "$(dirname "$0")"

echo "=========================================="
echo "  PyRecT3D Build (Linux / X11)"
echo "=========================================="

# 依赖检查
for cmd in cmake make g++; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "[ERROR] '$cmd' not found. Install build-essential + cmake."
    exit 1
  fi
done

# X11 开发头文件检查
if ! pkg-config --exists x11; then
  echo "[WARN] X11 dev headers not found. Install libx11-dev."
fi

mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
make -j"$(nproc)"

cd ..

echo
echo "=========================================="
echo "  BUILD OK. Outputs:"
echo "    core so : build/libPyRecT3D.so"
echo "    python  : py/package/ (if pybind11 present)"
echo "=========================================="
