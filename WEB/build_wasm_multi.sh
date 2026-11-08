#!/usr/bin/env bash
# ============================================================
#  PyRecT3D — WASM 多包模式构建
#  目标：抖音 / 小红书等「主包体积受限 + 资源可分片」的平台
#  产物：
#    multi-package/pyr3d_main.wasm        主包（引擎核心）
#    multi-package/chunks/assets_N.pak    资源分片
#    multi-package/manifest.json          分包清单
# ============================================================
set -e
cd "$(dirname "$0")/.."

if ! command -v emcc >/dev/null 2>&1; then
  echo "[ERROR] 未找到 emcc。请先 source emsdk_env.sh"
  exit 1
fi

OUT="WEB/multi-package"
CHUNKS="$OUT/chunks"
mkdir -p "$CHUNKS"

SRC=$(find cpp/src -name '*.cpp' \
      ! -name 'win7_input.cpp' \
      ! -name 'x11_input.cpp' \
      ! -name 'binding.cpp')

echo "=================================================="
echo " 构建 WASM 多包（主包 + 资源分包）"
echo "=================================================="

# ---- 主包：引擎核心（不含资源）----
emcc $SRC \
  -Icpp/include -Icpp/src \
  -std=c++17 \
  -O3 -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=createPyRecT3D \
  -s ENVIRONMENT=web \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  -s TOTAL_MEMORY=64MB \
  -DPYRECT3D_PLATFORM_WEB=1 \
  -DPYRECT3D_WITH_FFMPEG=0 \
  -o "$OUT/pyr3d_main.js"

# ---- 资源分包：每个分片独立可下载，支持断点续传标志 ----
shopt -s nullglob
for dir in assets/*/; do
  name=$(basename "$dir")
  echo "  打包资源分片: $name"
  emcc --preload-file "$dir"@/assets/$name \
    -s WASM=1 -O3 \
    -o "$CHUNKS/assets_${name}.js" \
    -s SINGLE_FILE=1
done
shopt -u nullglob

# ---- 生成 manifest.json ----
python3 - <<'PY'
import json, os, hashlib, pathlib

out = pathlib.Path("WEB/multi-package")
chunks = out / "chunks"

def sha(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()

files = []
for name in ("pyr3d_main.wasm", "pyr3d_main.js"):
    p = out / name
    if p.exists():
        files.append({"path": name, "role": "main", "size": p.stat().st_size, "sha256": sha(p)})

for p in sorted(chunks.glob("*")):
    if p.is_file():
        files.append({"path": f"chunks/{p.name}", "role": "asset",
                      "size": p.stat().st_size, "sha256": sha(p)})

manifest = {
    "name": "PyRecT3D",
    "version": "0.2.0",
    "mode": "multi-package",
    "backend": "GLES3/WebGL2",
    "entry": "pyr3d_main.js",
    "mainPackageLimit": "4MB",
    "files": files,
    "notes": "主包只含引擎核心；资源分片按需下载，支持断点续传（按 sha256 校验）"
}
(out / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
print(f"manifest.json 已生成：{len(files)} 个文件")
PY

echo
echo "产物："
ls -lh "$OUT"
echo
echo "多包模式部署：把整个 multi-package/ 上传到平台，按 manifest.json 按需拉取分片。"
