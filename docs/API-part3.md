## 18. Python 绑定（pybind11，v0.2 全量）

### 18.1 构建与安装

```bash
# 需要 pybind11（pip install pybind11），并用同一解释器构建
cmake -S . -B build -DPYRECT3D_BUILD_PYTHON=ON \
      -DPython3_EXECUTABLE=$(which python3) \
      -Dpybind11_DIR=$(python -c "import pybind11;print(pybind11.get_cmake_dir())")
cmake --build build -j4

# 产物落在 py/package/：
#   pyrect3d_binding.cpython-312-x86_64-linux-gnu.so
#   libPyRecT3D.so              （共享库构建时自动拷入，模块以 $ORIGIN 找它）
python -c "import sys;sys.path.insert(0,'py/package');import pyrect3d_binding as p;print(p.__version__)"
```

> 模块名是 `pyrect3d_binding`（不是 pyrect3d）：`import pyrect3d_binding as p`。
> 静态构建（`-DPYRECT3D_LIB_TYPE=static`）不会产生 `libPyRecT3D.so`，模块自带全部代码，分发最省事。

### 18.2 绑定源文件划分（一个头文件一份）

| 文件 | 导出内容 |
|------|----------|
| `cpp/src/binding/binding.cpp` | 模块入口 `PYBIND11_MODULE(pyrect3d_binding)`，只调用各 `bind_*` |
| `cpp/src/binding/binding_common.h` | 公共 include + 统一命名空间 + 各注册函数声明 |
| `bind_math.cpp` | `Vec2 / Vec3 / Vec4 / Mat4 / Ray / RaycastHit / AABB` |
| `bind_core.cpp` | `Key / InputState / Entity / Scene / Engine` |
| `bind_message.cpp` | `EventPriority / EventType / MessagePayload / Message / MessageQueue / MainMessageQueue / App` |
| `bind_explicit.cpp` | `Backend / Module / AppConfig / WindowDesc / UIInstance / WindowInstance / ExplicitApp` + `quick` 子模块 |
| `bind_renderer.cpp` | `RenderMode / Renderer / CommandType / RenderCommand / DirtyRegion / Compositor / Cube / ScreenCube / SoftRayTracer` |
| `bind_gui.cpp` | `Rect / GuiEvent / Widget / Button / Text / InputBox / Container / ScrollContainer / Gui` |
| `bind_collision.cpp` | `CollisionEventKind / Limb / PhysicsCollider / ModelCollider / HitResult / CollisionWorld` |
| `bind_media.cpp` | `VideoTexture / VideoDesc / VideoPlayer / MediaManager / Audio / Material / ObjModel / ObjLoader` |

### 18.3 命名约定

- C++ 的 camelCase 成员在 Python 侧统一变成 snake_case：`addPhysicsBox → add_physics_box`、`entityCount → entity_count`、`setClearColor → set_clear_color`。
- 字段同规则：`payload.entityId → payload.entity_id`，`desc.decodeAudio → desc.decode_audio`。
- 枚举值保留原拼写：`EventPriority.WindowInput`、`EventType.CollisionEnter`。
- 生命周期：窗口 / UI / 播放器 / 队列由引擎或调用方持有，Python 侧只拿句柄（`nodelete` holder），**不会**被 GC 回收，也不会被 Python 误删。

### 18.4 最小可用脚本（显式 API + 视频 + 碰撞）

```python
import sys, time
sys.path.insert(0, 'py/package')
import pyrect3d_binding as p

app = p.ExplicitApp()
cfg = p.AppConfig()
cfg.width, cfg.height, cfg.title = 1280, 720, "PyRecT3D"
app.configure(cfg)

win = app.create_window(p.WindowDesc())
ui  = win.create_ui()                 # 多窗口时必须 bind_window(win)
btn = ui.add_button(p.Rect(20, 20, 120, 40), "Play")
btn.on_click = lambda: print("clicked")

# 全局播放器池：整个程序只有一个 MediaManager
vp = app.create_video_player(p.VideoDesc(), win)
tex = vp.texture()                    # 每帧变化的动态材质句柄

coll = p.CollisionWorld()
phys = p.PhysicsCollider(); phys.box = p.AABB(p.Vec3(-1,-1,-1), p.Vec3(1,1,1))
coll.add_physics_box(phys)

def on_enter(msg):
    print("enter", msg.payload.entity_id)
coll.set_event_handler(p.CollisionEventKind.Enter, on_enter)

def frame():
    vp.upload_pending_frame()         # 主线程：把子线程解好的帧上传 GPU
    win.render_scene()
    ui.draw_all()
    win.present()

win.render_scene(); ui.draw_all(); win.present()
```

### 18.5 隐式 API（`quick` 子模块）

```python
import pyrect3d_binding as p

c = p.quick.cube(0, 0, -5)                 # EntityHandle
p.quick.move(c, 1, 0, 0)
dup = p.quick.clone(c)                     # 复制场上现成状态（不是召唤）
hit = p.quick.raycast(0, 0, 0, 0, 0, -1)   # 返回 HitResult（双碰撞箱）
btn = p.quick.button(10, 10, 120, 40, "OK")
p.quick.run(1280, 720, "PyRecT3D")         # 阻塞运行
```

> `quick` 只绑定"当前唯一窗口 / 唯一场景"；需要多窗口就改用 `ExplicitApp`。

---

## 19. 返回值与错误约定

| 场景 | 约定 |
|------|------|
| `bool` 返回 | `false` 表示失败 / 模块被裁剪 |
| 指针返回（`createUI` / `createVideoPlayer`） | `nullptr` 表示失败或模块被裁剪 |
| `Scene::getEntity` | 不存在返回 `nullptr` |
| id 类返回 | `-1` 表示失败 |
| `EventType` 事件携带失败 | `payload.code < 0` |
| `HitResult.hit` | `false` 表示未命中 |

---

## 20. v0.1 → v0.2 变更摘要

### 新增
- `message.h`：主消息队列 + 子消息队列 + 六级事件优先级 + `App::run` 统一启动。
- `explicit.h` / `implicit.h`：**显式 API 与隐式 API 双轨**，弱绑定规则明确。
- `video.h`：全局 `MediaManager` + 多播放器实例 + 子线程解码 + 动态视频材质 + 播放器事件按子队列路由。
- `collision.h`：**双碰撞箱**（物理箱 / 模型箱含肢体）+ 8 种碰撞事件 + 射线结果入队。
- `renderer.h`：`RenderCommand` 合批、`DirtyRegion` 局部更新、`Compositor` 2D/3D 分层合成、`applyEffect3D`。
- `gui.h`：`GuiEvent` 从 2 种扩到 9 种（进入/退出/按下/松开/拖动/拖出/区域外松开）。
- `Scene::cloneEntity` / `Scene::summonEntity`（克隆与召唤并存）。
- 构建：`PYRECT3D_LIB_TYPE`（DLL/LIB）、模块裁剪开关、交互式 `build.bat`。
- 分发：`WEB/`（单包 + 多包）、`Android/apk/`（占位契约）。

### 修复
- `Renderer::drawEntity` 每帧清屏导致多实体只显示最后一个 → 改为 `beginFrame/endFrame` 帧边界。
- `MessageQueue::dispatch` 无 handler 时误吞消息 → 改为保留给 `poll()`。
- `AABB` 射线起点在盒内时命中距离取错 → 统一 `t = (tNear > 0) ? tNear : tFar`。
- CMake 引用不存在的 `soft_raytracer.cpp` → 合并回 `renderer.cpp`。

### 兼容性
- 头文件路径与命名空间不变（`#include <PyRecT3D/*.h>`、`namespace pyrect3d`）。
- v0.1 的 `Engine::run/pump`、`Gui::addWidget`、`Audio`、`ObjLoader` 全部保留。

---

## 21. 给 AI 的调用模板（可直接复制）

### 21.1 最小可运行（隐式 API）

```
用 PyRecT3D 写一个程序：
1. #include <PyRecT3D/implicit.h>，using namespace pyrect3d;
2. main 里调用 quick::runSetup(1280, 720, "标题", []{ ... });
3. 场景里用 quick::cube 建方块，quick::clone 复制，quick::move/rotate/scale 变换
4. UI 用 quick::button，挂 onClick / onEnter / onDragOut
5. 需要视频就 quick::video("path", true)
```

### 21.2 多窗口（显式 API）

```
1. #include <PyRecT3D/explicit.h>
2. ExplicitApp app; app.configure(cfg);
3. app.createWindow(desc) × N
4. app.mountQueues()           ← 多窗口必须
5. createUI() + bindWindow()   ← 每个窗口必须
6. createVideoPlayer(desc, win) ← 显式绑定
7. app.run()
```

### 21.3 常见陷阱自查

- [ ] 子线程是否碰了 `gl*`？（必须改为投递渲染指令）
- [ ] WASD / 摇杆是否做了向量钳位？
- [ ] 多窗口是否 `mountQueues()` 了？
- [ ] UI / 播放器在多窗口下是否显式绑定？
- [ ] 是否在 `beginFrame()` 与 `endFrame()` 之间画多个实体？
- [ ] 页面隐藏 / App 暂停是否调用了 `release_gpu_resources()`（且幂等）？
- [ ] 播放器事件是否按 `targetQueue` 路由，没有广播给所有窗口？

---

*PyRecT3D v0.2 · GPL-3.0 · 本文档随代码同步维护*

## 22. 图形后端与 glad 加载层（v0.2 新增）

### 22.1 为什么要自带 glad

引擎只走 GL 家族（GL 3.3 Core / GLES 3.0 / WebGL2），不主推 Vulkan。为了让老机器（含 Win7 无驱动更新）与 Android/Web 共用一套渲染语义，引擎内部自带一个**离线合并版 glad**：

```
cpp/third_party/glad/glad.h   17 KB   类型 + 常量 + X-macro 函数表 + 别名宏
cpp/third_party/glad/glad.c    5 KB   桌面 WGL/GLX 加载、EGL 注入入口
```

- 不 include 任何系统 GL 头 → 不会和 `<GL/gl.h>` 的类型/宏打架。
- 函数入口只有一张表 `PYR3D_GL_FUNC_LIST(F)`，声明 / 定义 / 加载 / `glXxx → glad_glXxx` 别名宏全部由它派生；加一个函数只加一行。
- 官方 glad 的 GL 3.3 Core 单文件 500 KB+，这里 22 KB 就够用，符合"体积精简"目标。

### 22.2 加载与回退

| 场景 | 调用 | 说明 |
|------|------|------|
| 桌面 Windows | `gladLoadGL()` | `LoadLibraryA("opengl32.dll")` + `wglGetProcAddress`，1.1 函数回落 `GetProcAddress`；返回值 1/2/3/-1 视为无效地址 |
| 桌面 Linux | `gladLoadGL()` | `dlopen("libGL.so.1"/"libGL.so"/"libGL.so.1.7")` + `glXGetProcAddressARB`，回落 `dlsym` |
| Android / EGL | `gladLoadGLES3(loader)` | 把 `eglGetProcAddress` 包成 `PYR3D_GL_LOADER` 传进来 |
| Web / Emscripten | `gladLoadGL()` 返回 0 | GL 符号由链接器解析，不经运行期加载；WASM 端走 `WEB/bridge` 调用引擎导出 |
| 无上下文 / 驱动不支持 | `gladLoadGL()` 返回 0 | 渲染器自动回落到内置软件光栅化 |

最低可用性校验：`glClear`、`glDrawElements`、`glCreateProgram` 三者缺一即判定加载失败（`gladLoadGL()` 返回 0）。

### 22.3 后端封装层（内部头，不对外暴露）

`cpp/src/renderer/gl_backend.h` 是引擎唯一的 GL 接触面，上层只说"画什么"：

```cpp
namespace pyrect3d { namespace gl {
bool  init(int width, int height);       // 加载入口 + 建 VAO/VBO/shader
void  shutdown();                        // 幂等
bool  available();                       // 是否处于真实 GL 路径
void  beginFrame(float r, float g, float b, float a);
void  endFrame();
void  drawCube(const float* model4x4, const float* viewProj4x4, bool wireframe);
void  drawScreenRect(float x, float y, float w, float h,
                     float r, float g, float b, float a);
void  resize(int width, int height);
std::string versionString();             // 未开 GL 时返回 "software-rasterizer"
std::string rendererString();
}}
```

内置两个 shader（GLSL 330 / ES 300 同一份源码）：立方体（位置 + 顶色 + `uMvp`）与 2D 矩形（`uColor` + 正交投影）。单位立方体 24 顶点 / 36 索引，一次建好复用。

### 22.4 双路径行为（不破坏测试与老设备）

`Renderer` 启动时尝试 `gl::init(w,h)`：

- 成功 → `useGL = true`，`beginFrame` 调 `glClear`，`drawEntity` 走真实矩阵管线上屏；
- 失败（未编译 GL 后端、无上下文、驱动不支持）→ `useGL = false`，走内置软件光栅化，`saveFrame` 仍能导出 PPM。

因此同一份代码在无 GPU 的 CI 里也能跑 `smoke_v2`（PASS=42 FAIL=0）。

### 22.5 编译开关

```bash
# 默认：不编 glad，纯软件回退（CI / 无 GPU 环境）
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 开启真实 GL（需要上下文，通常配合平台窗口层）
cmake -S . -B build -DPYRECT3D_WITH_GL=ON
```

| 开关 | 默认 | 影响 |
|------|------|------|
| `PYRECT3D_WITH_GL` | OFF | ON 时编入 `glad.c` 并定义 `PYRECT3D_WITH_GL=1`，`gl_backend.cpp` 走真实实现；OFF 时 `gl_backend.cpp` 编译为空实现，保证链接不报错 |
| `CMAKE... LANGUAGES` | C + CXX | `glad.c` 是 C 源，工程已声明双语言 |

### 22.6 自己接入新 GL 调用

1. 在 `glad.h` 的 `PYR3D_GL_FUNC_LIST` 里加一行：`F(void, glBindFramebuffer, (GLenum target, GLuint fb))`；
2. 需要的话补一条常量；
3. 直接写 `glBindFramebuffer(...)` 即可（别名宏已生成）。

不要 include `<GL/gl.h>`，也不要手写 `extern` 原型——两张来源不同的声明会冲突。

---
