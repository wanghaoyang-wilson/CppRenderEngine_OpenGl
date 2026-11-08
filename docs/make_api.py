#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
把 docs/API.md（第 0-9 章）与 docs/API-part2.md（第 10-21 章）
拼接成单份完整文档，并输出到 docs/API-full.md。

为什么拆成两份提交？GitHub API 单次提交文件内容有体积限制，
50KB 中文正文一轮传不上去，所以分两段维护；对外阅读一直用合成版。

用法：
python docs/make_api.py
然后：
pandoc docs/API-full.md -o docs/API.docx --toc --toc-depth=2
"""
import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
PART1 = HERE / "API.md"
PART2 = HERE / "API-part2.md"
OUT = HERE / "API-full.md"


def main():
    for p in (PART1, PART2):
        if not p.exists():
            sys.exit(f"缺少文件：{p}")

    text = PART1.read_text(encoding="utf-8").rstrip()
    # 去掉第二部分重复的标题区，只保留正文
    body = PART2.read_text(encoding="utf-8")
    marker = "---\n\n## 10."
    idx = body.find(marker)
    if idx >= 0:
        body = body[idx:]
    else:
        idx = body.find("## 10.")
        body = body[idx:] if idx >= 0 else body

    OUT.write_text(text + "\n\n" + body, encoding="utf-8")
    lines = OUT.read_text(encoding="utf-8").count("\n")
    print(f"✓ 已生成 {OUT}（{lines} 行）")
    print("  下一步：pandoc docs/API-full.md -o docs/API.docx --toc --toc-depth=2")


if __name__ == "__main__":
    main()
