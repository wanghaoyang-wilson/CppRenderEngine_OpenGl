# PyRecT3D 文档说明

本目录面向使用者和 AI，目标是：**只读这里的文档，就能使用全库，不需要读源码。**

## 文件构成

| 文件 | 说明 |
|------|------|
| `API.md` | 公共 API 完整参考 —— **第一部分（第 0-9 章）**：阅读路线 / 双 API / 总索引 / export / math / core / message / explicit / implicit / renderer |
| `API-part2.md` | **第二部分（第 10-21 章）**：GUI / 碰撞 / 视频 / 音频 IO / 硬约束 / 构建分发 / WASM / Android / Python / 返回值约定 / v0.1→v0.2 变更 / 给 AI 的调用模板 |
| `API-full.md` | 两份拼接后的**单文件完整版**（由 `make_api.py` 生成，不入版仓库，需要时本地生成） |
| `API.docx` | Word 版（由 pandoc 从 `API-full.md` 生成，同样不入版仓库） |
| `make_api.py` | 拼接脚本：`API.md` + `API-part2.md` → `API-full.md` |
| `build_docs.sh` | 一键生成 `API-full.md` + `API.docx`（需装 pandoc） |
| `api.md` | v0.1 旧文档，已改为**重定向页**，只指向 `API.md` / `API.docx` |

> **为什么拆两个文件？** GitHub 接口单次提交的文本体积有限，50 KB 中文正文一轮推不上去。
> 对外阅读一律用拼接后的 `API-full.md` / `API.docx`，拆分只是仓库内部的维护方式。

## 生成 Word 文档

```bash
# 已装 pandoc（macOS: brew install pandoc；Ubuntu: apt install pandoc；Windows: choco install pandoc）
./docs/build_docs.sh
# 产物：docs/API-full.md / docs/API.docx（含目录，二级标题）
```

或手动：

```bash
python3 docs/make_api.py
pandoc docs/API-full.md -o docs/API.docx --toc --toc-depth=2
```

## 文档目标

- `docs/API.md` 覆盖**全部公共 API 签名 + 语义 + 硬约束 + 构建分发 + 平台对接**。
- 包含「给 AI 的调用模板」（第 21 章）：把该章喂给 AI，即可产出可运行的调用代码。
- 涵盖「常见陷阱自查清单」：线程、向量钳位、多窗口挂载、UI/播放器绑定、帧边界、资源释放。

## 在线 Wiki

仓库内 `wiki/` 是 GitHub Wiki 的源文件目录，另有 `wiki/publish_wiki.sh` 一键发布：

```bash
./wiki/publish_wiki.sh <username> <repo>
```

该脚本会把 `wiki/*.md` 拷入 `*.wiki.git`，并用拼接后的 API 全文覆盖 Wiki 的 `API-Reference` 页。
（首次使用前需在 GitHub 网页上手动创建第一个 Wiki 页面，否则 wiki 仓库不存在、无法 clone。）

---

PyRecT3D v0.2 · GPL-3.0
