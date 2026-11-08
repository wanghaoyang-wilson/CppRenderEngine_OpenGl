# cpp/ 目录分类规范

PyRecT3D 的 C++ 代码量已经超过单层目录能承载的规模，从 v0.2 起按「职责分层」固定下来。
新增代码前先看这一页，按表放；不要往 `core/` 里随手塞新东西。

## 目录树

```
cpp/
├── include/PyRecT3D/          公共契约头（唯一对外暴露的头文件目录）
│   ├── export.h               导出宏 / 版本号 / 模块开关
│   ├── core.h  math.h         Math、Mat4、Vec3、Time、Log
│   ├── renderer.h             Renderer、RenderCommand、DirtyRegion
│   ├── message.h              Message、MessageQueue、EventPriority
│   ├── explicit.h             ExplicitApp、WindowInstance、WindowDesc
│   ├── implicit.h             quick:: 隐式 API
│   ├── collision.h            CollisionWorld、双碰撞箱、8 类事件
│   ├── video.h                MediaManager、VideoPlayer、动态材质
│   ├── gui.h                  UIInstance、按钮/文本/输入框/容器
│   └── io.h  audio.h          OBJ 加载 / 音频
│
├── src/                       实现（按职责分层，自上而下依赖）
│   ├── foundation/            最底层，不依赖任何其它实现目录
│   │   ├── math/              math.cpp（Mat4/Vec3/矩阵运算）
│   │   └── io/                obj_loader.cpp（OBJ 解析）
│   ├── core/                  引擎内核：生命周期、事件、消息、应用注册
│   │   ├── engine.cpp         Engine 单例
│   │   ├── event_loop.cpp     主循环、六级优先级泵
│   │   ├── message_queue.cpp  主/子队列
│   │   ├── app_registry.h     内部注册表（不对外）
│   │   ├── explicit_app.cpp   ExplicitApp / WindowInstance
│   │   ├── implicit_api.cpp   quick::
│   │   └── platform/          win7_input.cpp / x11_input.cpp / platform_private.h
│   ├── graphics/              图形：渲染、GL 后端、合成
│   │   ├── renderer.cpp       双路径（GL / 软件光栅化）
│   │   ├── gl_backend.h/.cpp  glad 之上的封装，GLSL 330 / ES300 同源
│   │   └── compositor.cpp     指令合并、2D/3D 局部更新
│   ├── ui/                    2D 界面（gui.cpp）
│   ├── physics/               碰撞与射线（collision_world.cpp）
│   ├── media/                 FFmpeg 解码 + 动态材质（video.cpp）
│   ├── audio/                 音频（audio.cpp）
│   └── bindings/python/       pybind11 绑定，一个 bind_*.cpp 对一个头文件
│
└── third_party/
    └── glad/                  glad.h + glad.c（自研轻量 GL 加载器）
```

## 依赖方向（单向，不许反向）

```
bindings/python ─┐
ui ──────────────┼─→ graphics ─→ core ─→ foundation
media ───────────┤
audio ───────────┤
physics ─────────┘
```

- `foundation/` 只能依赖标准库和 `include/PyRecT3D/` 的公共头。
- `core/` 可以依赖 `foundation/`，**不能**依赖 `graphics/`、`ui/`、`media/`。
- `graphics/` 可以用 `core/` 与 `foundation/`，不许反向被 core 调用具体渲染实现
  （core 只通过 `renderer.h` 的接口持有）。
- `bindings/python/` 是最后一级，可以依赖上面全部。

违反方向的调用会在编译期被 `PYRECT3D_WITH_*` 裁剪打脸——被裁掉的模块在 core 里只剩空实现。

## 内部头文件的落位

- 只被 `src/` 内部用的头（例如 `core/app_registry.h`、`graphics/gl_backend.h`、
  `core/platform/platform_private.h`）留在**对应实现目录**，不进 `include/PyRecT3D/`。
- 头文件与它的实现同目录。`include/PyRecT3D/` 里只允许放「外部调用者需要 include」的头。
- 内部头用**相对 `cpp/src` 根的路径** include（构建脚本把 `cpp/src` 与 `cpp/third_party/glad`
  加进 include 路径），例如 `#include "core/app_registry.h"`；同目录文件用裸名
  `#include "gl_backend.h"`。

## 新增一个模块时的动作清单

1. 加公共头 `cpp/include/PyRecT3D/<模块>.h`，内容只有契约，不含实现。
2. 加实现目录 `cpp/src/<模块>/`，源文件 + 需要的内部头。
3. 在 `cpp/include/PyRecT3D/export.h` 加 `PYRECT3D_WITH_<模块>` 默认开关。
4. 在 `build.bat` 与 `CMakeLists.txt` **两处**同时登记源文件（两套构建互不依赖，
   任何一处漏了就有一边编不过）。若维护 `build.sh`，同步第三处。
5. 若要暴露给 Python，加 `cpp/src/bindings/python/bind_<模块>.cpp`，
   并在 `binding_common.h` 声明、`binding.cpp` 里按序调用。
6. 在 `docs/API.md` 对应章节补一段，`wiki/` 视需要加一页。

## 不要做的事

- 不要在 `cpp/src` 根目录放 `.cpp`。
- 不要把 `third_party/` 里的代码改造成引擎逻辑（glad 只做加载，不做渲染决策）。
- 不要为了省事把内部头放进 `include/PyRecT3D/`——那会把内部结构变成对外承诺。
