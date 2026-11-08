# PyRecT3D 文档说明

本目录面向使用者和 AI，目标是：**只读这里的文档，就能使用全库，不需要读源码。**

## 文件构成

| 文件 | 说明 |
|------|------|
| `API.md` | 公共 API 完整参考 —— **第一部分（第 0–9 章）**：阅读路线 / 双 API / 总索引 / export / math / core / message / explicit / implicit / renderer |
| `API-part2.md` | **第二部分（第 10–17 章）**：GUI / 双碰撞箱 / 视频动态材质 / 音频与 IO / 硬约束 / 构建分发 / WASM / Android |
| `API-part3.md` | **第三部分（第 18–22 章）**：Python 绑定 / 返回值约定 / v0.1→v0.2 变更 / 给 AI 的调用模板 / 图形后端与 glad 加载层 |
| `API-full.md` | 三片拼接后的**单文件完整版**（CI 自动生成并入库） |
| `API.docx` | Word 版（pandoc 从 `API-full.md` 生成，CI 自动生成并入库） |
| `make_api.py` | 拼接脚本：`API.md` + `API-part2.md` + `API-part3.md` → `API-full.md` |
| `build_docs.sh` | 一键生成 `API-full.md` + `API.docx`（需 pandoc） |
| `api.md` | v0.1 旧文档，已改为**重定向页**，只指向 `API.md` / `API.docx` |

> **为什么拆三片？** GitHub 单次提交的文本体积有限，50+ KB 中文正文一轮推不上去。
> 对外阅读一律用拼接后的 `API-full.md` / `API.docx`，拆分只是仓库内部的维护方式。

## 怎么改文档

只改 `API.md` / `API-part2.md` / `API-part3.md` 三片之一，**不要手工改 `API-full.md` 和 `API.docx`** —— 它们是生成物，下一次 CI 会覆写。

提交后 `.github/workflows/publish-docs.yml` 会自动：

1. 跑 `build_docs.sh` 重新拼接 `API-full.md`、用 pandoc 生成 `API.docx`；
2. 仅在 `API-full.md` 内容真的变了时才提交这两个产物（pandoc 产物带内部时间戳，不看它，避免空提交）；
3. 把 `wiki/*.md` 同步到 GitHub Wiki 仓库。

> 不需要配任何 PAT：工作流用的是仓库自带的 `GITHUB_TOKEN`，对本仓库和 `<repo>.wiki.git` 都有写权限。
> 也可以在 Actions 页手动 `Run workflow` 强制重生成一次。

## 本地自行生成

```bash
cd docs && bash build_docs.sh       # 需 pandoc：apt install pandoc / choco install pandoc
# 产物：docs/API-full.md / docs/API.docx（含目录，二级标题）
```

或手动：

```bash
python3 docs/make_api.py && pandoc docs/API-full.md -o docs/API.docx --toc --toc-depth=2
```

> 本地与 CI 的 pandoc 版本不同，生成的 docx 字节不会完全一致（内容相同）。仓库里的 docx 以 CI 产物为准。

## 文档目标

- `API.md` 三片合计覆盖**全部公共 API 签名 + 语义 + 硬约束 + 构建分发 + 平台对接**。
- 包含「给 AI 的调用模板」（第 21 章）：把该章喂给 AI，即可产出可运行的调用代码。
- 涵盖「常见陷阱自查清单」：线程、向量钳位、多窗口挂载、UI/播放器绑定、帧边界、资源释放。

## 在线 Wiki

<https://github.com/wanghaoyang-wilson/CppRenderEngine_OpenGl/wiki>

仓库内 `wiki/` 是 GitHub Wiki 的源文件目录（9 个内容页 + `_Sidebar.md` + `_Footer.md`），由上面那个 worklow 自动同步。
也保留了手工发布的脚本：

```bash
./wiki/publish_wiki.sh <username> <repo>
```

---

PyRecT3D v0.2 · GPL-3.0
