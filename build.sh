#!/usr/bin/env bash
# ============================================================
#  PyRecT3D v0.2 — Linux / 类 Unix 一键构建
#  用法：
#    ./build.sh                        # 默认：shared + Release + 全模块
#    ./build.sh static                 # 构建静态库 libPyRecT3D.a
#    ./build.sh shared debug           # 动态库调试版
#    ./build.sh static release trim    # 裁剪模块（交互询问）
# ============================================================
set -e
cd "$(dirname "$0")"

LIBTYPE="${1:-shared}"
BUILD_TYPE="${2:-release}"
TRIM="${3:-}"

if [ "$LIBTYPE" = "lib" ] || [ "$LIBTYPE" = "static" ]; then
  LIBTYPE=static
else
  LIBTYPE=shared
fi

if [ "$BUILD_TYPE" = "debug" ]; then
  BUILD_TYPE=Debug
else
  BUILD_TYPE=Release
fi

WITH_GUI=ON; WITH_VIDEO=ON; WITH_AUDIO=ON; WITH_PHYSICS=ON; WITH_RAYTRACER=ON

if [ "$TRIM" = "trim" ]; then
  echo "[模块裁剪] 直接回车表示保留"
  read -r -p "  保留 GUI        (y/n): " a; [ "$a" = "n" ] && WITH_GUI=OFF
  read -r -p "  保留 视频       (y/n): " b; [ "$b" = "n" ] && WITH_VIDEO=OFF
  read -r -p "  保留 音频       (y/n): " c; [ "$c" = "n" ] && WITH_AUDIO=OFF
  read -r -p "  保留 物理/碰撞  (y/n): " d; [ "$d" = "n" ] && WITH_PHYSICS=OFF
  read -r -p "  保留 软光追     (y/n): " e; [ "$e" = "n" ] && WITH_RAYTRACER=OFF
fi

echo "=============================================="
echo " PyRecT3D  lib=$LIBTYPE  config=$BUILD_TYPE"
echo "=============================================="

mkdir -p build && cd build
cmake .. \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DPYRECT3D_LIB_TYPE="$LIBTYPE" \
  -DPYRECT3D_WITH_GUI="$WITH_GUI" \
  -DPYRECT3D_WITH_VIDEO="$WITH_VIDEO" \
  -DPYRECT3D_WITH_AUDIO="$WITH_AUDIO" \
  -DPYRECT3D_WITH_PHYSICS="$WITH_PHYSICS" \
  -DPYRECT3D_WITH_RAYTRACER="$WITH_RAYTRACER" \
  -DPYRECT3D_BUILD_PYTHON=ON

cmake --build . -j"$(nproc)"

echo
echo "构建完成："
ls -lh libPyRecT3D.* 2>/dev/null || true
echo "冒烟测试： ./smoke_v2"
