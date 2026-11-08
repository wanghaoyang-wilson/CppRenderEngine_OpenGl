# PyRecT3D — WEB / WASM 平台

本目录是 PyRecT3D 的 **WebAssembly（WASM）+ WebGL2** 构建与对接目录。
上层 API 与桌面端 **完全一致**（同一套头文件、同一套调用），底层从桌面 OpenGL 3.3
切换到 OpenGL ES 3.0 语义，经 Emscripten 映射成 WebGL2。

## 两种打包模式（按目标平台选）

| 模式 | 目录 | 适用平台 | 说明 |
|------|------|----------|------|
| **单包模式** | `single-package/` | 小游戏平台（不支持分包/分片传输） | 全部产物打进一个 `.wasm`，`index.html` 直接加载 |
| **多包模式** | `multi-package/` | 抖音、小红书等内容平台 | 主包 + 资源分包 + 分片清单，按需下载、可断点续传 |

> 单包模式优先级最高：目标平台不支持分包时，必须用单包。
> 多包模式用于「主包体积受限、资源需要分片上传」的平台。

## 目录结构

```
WEB/
├── README.md                     # 本文档
├── build_wasm_single.sh          # 单包模式构建
├── build_wasm_multi.sh           # 多包模式构建
├── bridge/
│   ├── pyrect3d_web.js           # JS 侧对接层：初始化 / 输入转发 / 生命周期
│   └── pyrect3d_api.json         # 对外 JS API 与 C++ API 的映射表
├── single-package/
│   ├── index.html                # 可直接打开的演示页
│   └── pyr3d_single.wasm         # 构建产物（占位，构建后覆盖）
└── multi-package/
    ├── index.html
    ├── manifest.json             # 分包清单（主包 / 资源包 / 分片）
    └── chunks/                   # 分片目录（占位）
```

## 构建（需要 Emscripten）

```bash
# 一次性准备
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest && source ./emsdk_env.sh

# 单包模式
cd PyRecT3D
./WEB/build_wasm_single.sh

# 多包模式
./WEB/build_wasm_multi.sh
```

## 对接约定（很重要）

1. **API 完全一致**：Web 端调用与桌面端同名同参，不引入平台专属函数。
   平台差异只在 `bridge/pyrect3d_web.js` 内部消化。
2. **输入事件**：浏览器 DOM 事件（`mousemove` / `mousedown` / `keydown` / `touchstart`）
   统一转成引擎 `InputState` 与消息队列事件，上层不感知设备类型。
3. **摇杆斜向加速**：虚拟摇杆与 WASD 一样，进入引擎前做向量钳位（模长 ≤ 1），
   避免斜向速度 1.414 倍的经典 bug；可通过 `PYRECT3D_CLAMP_INPUT` 关闭。
4. **生命周期**：页面隐藏（`visibilitychange`）时触发 `AppPause`，
   主动调用 `release_gpu_resources()` 释放 GPU 资源；恢复时重建。
5. **渲染后端**：浏览器只有 WebGL2（≈ ES 3.0），后端固定 `Backend::GLES3`。

## 已知限制

- WebGL2 不支持部分桌面 GL 纹理内部格式，压缩纹理需按浏览器能力协商。
- 浏览器沙盒内不能直接读本地文件，资源需通过 `FETCH` 或打包进虚拟文件系统。
- 单包模式下 `.wasm` 建议 < 4 MB（小游戏平台常见限制）。
