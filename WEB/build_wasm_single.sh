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
      ! -name 'binding.cpp' \
      ! -name 'bind_*.cpp')

echo "=================================================="
echo " 构建 WASM 单包（GLES3 / WebGL2）"
echo "=================================================="

# 导出符号表必须与 WEB/web_entry.cpp 的 extern "C" 一一对应。
# --no-entry：浏览器 / Node 没有 main() 概念，全部调用走导出函数。
em++ $SRC WEB/web_entry.cpp \
  -Icpp/include -Icpp/src \
  -std=c++17 \
  -O3 \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=createPyRecT3D \
  -s ENVIRONMENT=web,node \
  --no-entry \
  -s EXPORTED_FUNCTIONS='["_pyr3d_version","_pyr3d_smoke_run","_pyr3d_pass_count","_pyr3d_fail_count","_malloc","_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","UTF8ToString"]' \
  -s ASSERTIONS=0 \
  -s DISABLE_EXCEPTION_CATCHING=1 \
  -DPYRECT3D_PLATFORM_WEB=1 \
  -DPYRECT3D_WITH_FFMPEG=0 \
  -o "$OUT/pyr3d_single.js"

echo
echo "产物："
ls -lh "$OUT"/pyr3d_single.*
echo
echo "打开 $OUT/index.html 即可运行（需通过 http 服务器访问，不能 file:// 直开）"
