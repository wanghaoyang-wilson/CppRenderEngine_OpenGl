#!/usr/bin/env bash
# 生成 docs/API.docx：先拼三片，再用 pandoc 出带目录的 Word
set -euo pipefail
cd "$(dirname "$0")"

python3 make_api.py

if ! command -v pandoc >/dev/null 2>&1; then
  echo "[build_docs] 未找到 pandoc，请先安装（apt install pandoc / brew install pandoc）" >&2
  exit 1
fi

pandoc API-full.md -o API.docx --toc --toc-depth=2
echo "[build_docs] 已生成 API.docx（$(stat -c%s API.docx 2>/dev/null || stat -f%z API.docx) 字节）"
