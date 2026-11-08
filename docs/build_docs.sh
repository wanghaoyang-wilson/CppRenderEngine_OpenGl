#!/usr/bin/env bash
# ==========================================================
#  PyRecT3D 文档构建：md 全集 + docx
#  依赖：python3、pandoc
#  用法：./docs/build_docs.sh
# ==========================================================
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

echo "[1/2] 拼接 API.md + API-part2.md → API-full.md"
python3 "$ROOT/docs/make_api.py"

echo "[2/2] 生成 API.docx"
if ! command -v pandoc >/dev/null 2>&1; then
  echo "✗ 未找到 pandoc，请先安装："
  echo "   Ubuntu : sudo apt install pandoc"
  echo "   macOS  : brew install pandoc"
  echo "   Windows: choco install pandoc"
  exit 1
fi

pandoc "$ROOT/docs/API-full.md" -o "$ROOT/docs/API.docx" \
  --toc --toc-depth=2

echo "✓ 完成：docs/API-full.md 与 docs/API.docx"
