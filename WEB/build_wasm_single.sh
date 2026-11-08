#!/usr/bin/env bash
# ============================================================
#  PyRecT3D — WASM 单包模式构建
#  目标：小游戏平台（不支持分包 / 分片传输）
#  产物：WEB/single-package/pyr3d_single.wasm + pyr3d_single.js
#  上层 API 与桌面端完全一致，底层固定 GLES3 → WebGL2
# ============================================================
set -e
cd "$(dirname "$0")/.."

if ! command -v emcc >/dev/null 2>&1; then
  echo "[ERROR] 未找到 emcc。请先安装并激活 emsdk："
  echo "  source /path/to/emsdk/emsdk_env.sh"
  exit 1
fi

OUT="WEB/single-package"
mkdir -p "$OUT"

SRC=$(find cpp/src -name '*.cpp' \
      ! -name 'win7_input.cpp' \
      ! -name 'x11_input.cpp' \
      ! -name 'binding.cpp')

echo "=================================================="
echo " 构建 WASM 单包（GLES3 / WebGL2）"
echo "=================================================="

emcc $SRC \
  -Icpp/include -Icpp/src \
  -std=c++17 \
  -O3 \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=createPyRecT3D \
  -s ENVIRONMENT=web \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  -s ASSERTIONS=0 \
  -s DISABLE_EXCEPTION_CATCHING=1 \
  -DPYRECT3D_PLATFORM_WEB=1 \
  -DPYRECT3D_WITH_FFMPEG=0 \
  --use-preload-plugins \
  -o "$OUT/pyr3d_single.js"

echo
echo "产物："
ls -lh "$OUT"/pyr3d_single.*
echo
echo "打开 $OUT/index.html 即可运行（需通过 http 服务器访问，不能 file:// 直开）"
