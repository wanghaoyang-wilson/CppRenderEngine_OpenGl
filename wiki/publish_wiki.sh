#!/usr/bin/env bash
# ==========================================================
#  把 wiki/*.md 发布到 GitHub Wiki
#  用法：./wiki/publish_wiki.sh <github-username> <repo-name>
#  依赖：有该仓库写权限的 git 凭证（SSH key 或 https token）
# ==========================================================
set -e
USER_NAME="${1:?用法: ./wiki/publish_wiki.sh <username> <repo>}"
REPO_NAME="${2:?用法: ./wiki/publish_wiki.sh <username> <repo>}"

WORK="$(mktemp -d)"
echo "→ clone wiki 仓库"
git clone "https://github.com/${USER_NAME}/${REPO_NAME}.wiki.git" "$WORK" 2>/dev/null \
  || { echo "Wiki 尚未初始化：请先在 GitHub 网页上创建第一个 Wiki 页面（如 Home），再重试。"; exit 1; }

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cp -v "$ROOT"/wiki/*.md "$WORK"/

# Wiki 上放 API 文档全文（拆的两部分先拼接）
python3 "$ROOT/docs/make_api.py" 2>/dev/null || true
if [ -f "$ROOT/docs/API-full.md" ]; then
  cp -v "$ROOT/docs/API-full.md" "$WORK/API-Reference.md"
else
  echo "→ 未找到 API-full.md，直接拼接 API.md + API-part2.md"
  cat "$ROOT/docs/API.md" <(echo) "$ROOT/docs/API-part2.md" > "$WORK/API-Reference.md"
fi

cd "$WORK"
git add -A
git commit -m "docs: PyRecT3D v0.2 wiki (API 参考 / 双 API / 事件模型 / 构建 / WASM / Android)"
git push origin master || git push origin main
echo "✓ Wiki 已发布：https://github.com/${USER_NAME}/${REPO_NAME}/wiki"
