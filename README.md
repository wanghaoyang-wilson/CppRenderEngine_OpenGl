# PyRecT3D

**轻量 C++ 2D/3D 渲染引擎**，底层 OpenGL 3.3 Core ／ Android + Web 走 OpenGL ES 3.0（WebGL2）。
既是可独立链接的 C++ 库（DLL / LIB），也通过 pybind11 提供 Python 扩展。

> 开发代号：PyRecT3D（Python + ReCursion + 3D）　版本 **0.2.0**　许可证 **GPL-3.0**

---

## 产品由两部分组成

| 部分 | 头文件 | 适用场景 |
|------|--------|----------|
| **显式 API（Explicit API）** | `<PyRecT3D/explicit.h>` | **多窗口实例**，或**需要详细配置**（自选渲染后端、按需裁剪模块、手动挂载子消息队列、手动绑定 UI 与播放器） |
| **隐式 API（Implicit API）** | `<PyRecT3D/implicit.h>` | **最快的方式入门**：单窗口自动弱绑定，一行启动，Scratch 式原型 |

> 一句话：小游戏/教学用隐式，多窗口/深度定制用显式。两者共用同一内核。

---

## 功能总览

| 模块 | 能力 |
|------|------|
| **双消息队列** | 主消息队列（全局单例）+ 子消息队列（每窗口一个）；六级事件优先级：**窗口输入 > 物理 > 碰撞 > 音效 > 媒体 > 普通** |
| **双 API** | 显式（多窗口/详细配置）+ 隐式（快速入门），弱绑定规则明确 |
| **渲染** | 2D+3D 统一、`Cube`/`ScreenCube`、**渲染指令合并**、**脏区域局部更新**（3D 视口不重绘只更新 UI）、**合成层 3D 特效**、软光追、帧导出 |
| **双碰撞箱** | 物理碰撞箱（射线计算）+ 模型碰撞箱（内部小型物理空间、肢体级精细碰撞） |
| **碰撞事件** | 进入 / 退出 / 停留 / 按下 / 松开 / 拖动 / **拖出** / **区域外松开**（9 种 GUI 事件同理） |
| **消息驱动** | 输入、场景变换、碰撞、射线结果、媒体事件全部入队，主队列统一调度 |
| **克隆 / 召唤** | `clone`（复制场上现成状态，Scratch 思维）+ `summon`（按模板新建，Unity/UE 思维）双内置 |
| **视频** | 全局 `MediaManager` 单例 + 多播放器实例，FFmpeg 子线程解码，**解码帧直接包装成动态材质**贴到任意物体 |
| **GUI** | 按钮 / 文本 / 输入框 / 容器 / 滚动容器，9 种组件事件，支持贴图 |
| **模型 IO** | OBJ + MTL 加载 |
| **跳平台** | Windows（Win7+）/ Linux（X11）/ Android（NDK + GLES3）/ Web（WASM + WebGL2） |

### 明确边界
- 不内置任何游戏逻辑，只提供渲染 / 输入 / 消息 / 碰撞 / 媒体能力
- 仅软件光追，不含硬件光追（Vulkan 后端留作未来可选扩展）
- GUI 保持极简：不含表格、树、标签页、Dock、动画、主题
- **不支持 Apple 平台**（不引入 Xcode / Objective-C 工具链）

---

## 快速开始

### 隐式 API（最快）

```cpp
#include <PyRecT3D/implicit.h>
using namespace pyrect3d;

int main() {
    quick::runSetup(1280, 720, "Quick Demo", [] {
        EntityHandle a = quick::cube(0, 0, -5);
        EntityHandle c = quick::clone(a);      // 克隆场上状态
        quick::move(c, -2, 0, -5);

        Button* btn = quick::button(20, 20, 140, 40, "复位");
        btn->onClick = [a] { quick::move(a, 0, 0, -5); };

        quick::video("assets/demo.mp4", true);
    });
}
```

### 显式 API（多窗口 / 详细配置）

```cpp
#include <PyRecT3D/explicit.h>
using namespace pyrect3d;

int main() {
    ExplicitApp app;

    AppConfig cfg;
    cfg.backend = Backend::Auto;              // 桌面→GL33，Android/Web→GLES3
    cfg.enableGUILocalUpdate = true;
    app.configure(cfg);

    WindowDesc da; da.title = "A"; da.width = 800; da.height = 600;
    WindowDesc db; db.title = "B"; db.width = 640; db.height = 480;
    WindowInstance* wa = app.createWindow(da);
    WindowInstance* wb = app.createWindow(db);

    app.mountQueues();                        // 多窗口必须挂载子队列

    UIInstance* uiA = wa->createUI();
    uiA->bindWindow(wa);                      // 多窗口必须显式绑定
    uiA->addButton(Rect{ 20, 20, 160, 44 }, "退出")->onClick = [] { App::quit(); };

    return app.run();
}
```

---

## 构建

### Windows（Win7+，交互式）

```bat
build.bat
```

交互项（回车即取默认值）：
```
[1/6] 产物类型：1 = DLL（调用侧只用 cpp\include + LoadLibraryA）  2 = LIB（静态）
[2/6] CL 编译路径：回车 = 自动用 vswhere 找 VS，找不到再用 PATH 里的 cl.exe
[3/6] 配置：Release（/O2 /GL + LTCG）/ Debug（/Od /Zi）
[4/6] GL 后端：0 = 只有软件光栅化（最小体积）/ 1 = glad + GL33/GLES3
[5/6] 模块裁剪：GUI / 视频 / 音频 / 物理（不编就是不编，对应 API 返回 false）
       + 是否额外编 examples
[6/6] 确认清单，并单独询问「是否把被裁掉的源码从磁盘上删掉」（默认 n）
```

`build.bat` 内部只用 `cl.exe` / `link.exe` / `lib.exe`，不调用 CMake、不调用 NMake。
`CMakeLists.txt` 仍然保留，但只是给 IDE 或包管理用的可选路径。

**调用侧怎么接**

- DLL：include 只加 `cpp\include`，`LoadLibraryA("PyRecT3D.dll")` + `GetProcAddress`。
- LIB：自己的工程加 `/DPYRECT3D_STATIC=1`（否则 `export.h` 会当成 dllimport 去找导入库），
  再链接 `build\PyRecT3D.lib`。

### Linux

```bash
./build.sh                      # 共享库 + Release + 全模块 + GL 关
./build.sh --static             # 静态库 libPyRecT3D.a
./build.sh --gl=on --tests      # 开 GL 并编译运行冒烟测试
./build.sh --trim=video,audio   # 裁剪视频与音频模块
```

`build.sh` 同样不依赖 CMake，直接调 g++。

### 产物

| 类型 | Windows | Linux |
|------|---------|-------|
| DLL / SO | `PyRecT3D.dll` + `.lib` | `libPyRecT3D.so` |
| LIB / A | `PyRecT3D.lib` | `libPyRecT3D.a` |
| Python | `py/package/pyrect3d_binding*.pyd` | `.so` |

---

## 跳平台分发

| 平台 | 目录 | 说明 |
|------|------|------|
| Windows / Linux | 源码 + `build.bat` / `build.sh` | 用户本地编译（DLL 或 LIB 二选一） |
| **Web (WASM)** | `WEB/` | **预编译交付**，两种模式：`single-package/`（小游戏平台，不分包）、`multi-package/`（抖音/小红书，主包 + 资源分片 + manifest 断点续传） |
| **Android** | `Android/apk/` | NDK + Gradle，`GLES3` 后端与 Web 共用；当前为占位契约 |

> WASM 端与桌面端**对外 API 完全一致**，平台差异只在 `WEB/bridge/pyrect3d_web.js` 消化。

---

## 目录结构

```
PyRecT3D/
├── build.bat               # Windows 交互式构建（只用 cl.exe/link.exe/lib.exe，不依赖 CMake/NMake）
├── build.sh                # Linux / GLES 构建（直接用 g++，不依赖 CMake）
├── CMakeLists.txt          # 可选：给需要 IDE / 包管理的场景用
├── LICENSE                 # GPL-3.0
├── cpp/
│   ├── LAYOUT.md           # 目录分类规范：新代码该放哪、依赖方向
│   ├── include/PyRecT3D/   # 对外契约头文件（仅声明，调用侧只需要这一层）
│   │   ├── export.h  math.h  core.h
│   │   ├── message.h       # 主/子消息队列、事件优先级
│   │   ├── explicit.h      # 显式 API
│   │   ├── implicit.h      # 隐式 API
│   │   └── renderer.h  gui.h  collision.h  video.h  audio.h  io.h
│   ├── src/                # 按职责分层，依赖单向
│   │   ├── foundation/     # 最底层：math/ 与 io/
│   │   ├── core/           # 引擎 / 消息队列 / 显式 / 隐式 / platform/
│   │   ├── graphics/       # renderer + gl_backend + compositor
│   │   ├── ui/             # 2D 界面
│   │   ├── physics/        # 双碰撞箱
│   │   ├── media/          # FFmpeg 播放器
│   │   ├── audio/
│   │   └── bindings/python/  # pybind11，一个 bind_*.cpp 对一个头文件
│   └── third_party/glad/   # 轻量 GL 加载器（自研，17 KB）
├── examples_cpp/           # basic_cube / explicit_demo / implicit_demo
├── tests/smoke_v2.cpp      # 无头冒烟测试
├── docs/                   # API.md + API.docx（权威接口文档）
├── WEB/                    # WASM 单包 / 多包 + 桥接层
├── Android/apk/            # APK 占位
└── wiki/                   # GitHub Wiki 页源文件
```

---

## 文档

| 文件 | 说明 |
|------|------|
| [`docs/API.md`](docs/API.md) | 全量公共 API 参考（含签名、约束、示例、AI 调用模板） |
| [`docs/API.docx`](docs/API.docx) | 同一内容的 Word 版本，便于分发 |
| `docs/build_docs.sh` | 三片拼接 + pandoc 生成 docx |

> **给 AI 用**：只需把 `docs/API.md` 整份喂给它，即可在不读源码的前提下产出正确调用代码。

---

## 平台与图形约束

| 项 | 要求 |
|----|------|
| Windows | 最低 **Win7**（经典 Win32 消息机制，不用 Win10+ 专属 API） |
| Linux | **X11**（不默认 Wayland） |
| 桌面图形 | OpenGL **3.3 Core** |
| Android / Web | OpenGL **ES 3.0**（WebGL2） |
| Apple | **不支持** |

---

## License

**GNU General Public License v3.0（GPL-3.0）** — 见 [`LICENSE`](LICENSE)。
