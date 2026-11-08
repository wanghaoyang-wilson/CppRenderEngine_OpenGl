# WASM Packaging（Web 分发）

引擎在 Web 端跑在 **WebGL2** 上（≈ OpenGL ES 3.0），对外 API 与桌面端**完全一致**。
WASM 由**引擎方预编译交付**，使用方不需要自己装 Emscripten。

## 两种打包模式

| 模式 | 目录 | 适用平台 | 特征 |
|------|------|----------|------|
| **单包** | `WEB/single-package/` | 小游戏平台（不支持分包传输） | 全部打成一个 `.wasm`，`index.html` 直接加载；建议 < 4 MB |
| **多包** | `WEB/multi-package/` | 抖音、小红书等有体积/分包限制的平台 | 主包（核心 JS+wasm）+ 资源分片；`manifest.json` 含每片 `sha256`，支持按需加载 / 断点续传 |

## 构建

```bash
./WEB/build_wasm_single.sh      # 单包
./WEB/build_wasm_multi.sh       # 多包（主包 + chunks/ + 自动生成 manifest.json）
```

需要 Emscripten（仅引擎方需要）：

```bash
emcc --version          # 未装则需 emsdk
```

## 单包结构

```
WEB/single-package/
├── index.html          # 加载入口（含加载进度与错误提示）
├── pyr3d_single.js     # emcc 产出的 JS 壳
├── pyr3d_single.wasm   # 引擎本体
└── assets/             # 贴图 / 模型 / 音频（可选内嵌）
```

## 多包结构

```
WEB/multi-package/
├── index.html          # 分片加载器（进度 / 失败重试 / sha256 校验）
├── manifest.json       # 清单：文件列表 + 大小 + sha256
├── pyr3d_main.js       # 主包 JS
├── pyr3d_main.wasm     # 主包 wasm（核心 + 必需模块）
└── chunks/             # 资源分片（模型 / 贴图 / 音视频）
```

`manifest.json` 由 `build_wasm_multi.sh` 自动生成（含 `sha256`），不要手改。

## JS 桥接 API（`WEB/bridge/pyrect3d_web.js`）

与 C++ API 同名同义：

```js
const app = await PyRecT3DWeb.init({
  wasmPath: "pyr3d_single.js",
  width: 1280, height: 720,
  clampInput: true,        // 摇杆向量钳位（修真机上斜向加速）
  canvas: "#game",
});

const a = app.cube(0, 0, -5);
const c = app.clone(a);          // 克隆（与 C++ Scene::cloneEntity 同一语义）
app.move(c, -2, 0, -5);
app.video("assets/demo.mp4", true);

app.start();                     // 启动 requestAnimationFrame 循环
```

桥接层已经替你处理：

| 平台差异 | 桥接层处理 |
|----------|------------|
| WebGL2 上下文创建 | `canvas.getContext("webgl2")` + 版本探测 |
| 键盘 / 鼠标 | DOM 事件 → 引擎 `InputState` |
| 虚拟摇杆 | 触摸 → 向量 + **模长钳位 ≤ 1** |
| 页面隐藏 | `visibilitychange` → 释放 GPU 资源（幂等） |
| 高分屏 | `devicePixelRatio` → 渲染缩放 |

## 已知限制

- WebGL2 不支持部分桌面 GL 压缩纹理格式，需按浏览器能力协商。
- 浏览器沙盒不能直接读本地文件，资源走 `fetch` 或打包进虚拟文件系统（MEMFS）。
- 单包 `.wasm` 建议 < 4 MB；超出请改多包。
- 音频需等待首次用户手势后才能播放（浏览器策略）。

---

返回 [[Home]] · 构建见 [[Build]] · 上安卓见 [[Android]]
