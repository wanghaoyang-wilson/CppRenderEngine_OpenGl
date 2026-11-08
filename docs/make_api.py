#!/usr/bin/env python3
"""把 docs/API.md 三片拼成完整文档（docs/API-full.md）。

为什么分片：GitHub 单文件上传对体积敏感，58 KB 的 API.md 会被拒。
第 0–9 章 → API.md，10–17 章 → API-part2.md，18–22 章 → API-part3.md，
本脚本把它们拼成一份完整文档供阅读与生成 docx。
"""
from pathlib import Path

HERE = Path(__file__).resolve().parent
PARTS = ["API.md", "API-part2.md", "API-part3.md"]
OUT = HERE / "API-full.md"


def main() -> int:
    chunks = []
    for name in PARTS:
        p = HERE / name
        if not p.exists():
            print(f"[make_api] 缺文件: {name}")
            return 1
        chunks.append(p.read_text(encoding="utf-8").rstrip() + "\n")
    OUT.write_text("\n".join(chunks), encoding="utf-8")
    total = sum(len(c) for c in chunks)
    print(f"[make_api] 已生成 {OUT.name}（{len(PARTS)} 片，{total} 字符，{OUT.stat().st_size} 字节）")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
