# PyRecT3D v0.2 — 公共 API 完整参考

> **本文件的定位**：写给「AI 或人类读者只看这一份文档，就能完整使用/复刻本引擎全部能力」的**唯一权威接口文档**。
> 你不需要通读整个库的源码；把本文档喂给 AI，按本文档调用即可产出可用代码。

> **分片说明**（v0.2 起为跨单文件上传上限拆为三份，`docs/make_api.py` 自动拼接）：
> - `docs/API.md`：第 0–9 章（总览 / 数学 / 核心 / 消息队列 / 显式 API / 隐式 API / 渲染）
> - `docs/API-part2.md`：第 10–17 章（GUI / 碰撞 / 视频 / IO / 硬约束 / 构建 / WASM / Android）
> - `docs/API-part3.md`：第 18–22 章（Python 绑定 / 错误约定 / 变更 / AI 模板 / GL+glad 后端）

- 引擎版本：`0.2.0`
- 语言：C++17（对外契约头文件）+ Python（pybind11 绑定）
- 图形：桌面 OpenGL 3.3 Core ／ Android + Web `OpenGL ES 3.0`（WebGL2）
- 平台：Windows（最低 Win7）、Linux（X11）、Android（NDK）、Web（WASM）
- 许可证：**GPL-3.0**

---

## 0. 阅读路线（给 AI 的最短路径）

1. 先看 **第 1 章「产品由两部分组成」**，判断该用 *显式 API* 还是 *隐式 API*。
2. 再看 **第 2 章「公共 API 总索引」**，找你要用的函数名。
3. 用 **第 3 章以后** 的逐条签名抄代码。
4. 涉及线程、渲染顺序、资源释放，**必须遵守第 14 章「硬约束」**，否则会崩溃或显存泄漏。

> 一句话总结用法：
> - 想做小游戏、演示、教学 → 用 **隐式 API**（`quick::` 命名空间），一行启动，全自动弱绑定。
> - 想做多窗口、需要精细配置（后端/模块/队列/绑定） → 用 **显式 API**（`ExplicitApp` + `WindowInstance` + `UIInstance`）。

---

## 1. 产品由两部分组成

PyRecT3D 对外暴露 **两套平行 API**，共用同一个内核（同一份渲染 / 消息 / 碰撞 / 媒体实现）。

### 1.1 显式 API（Explicit API）

**头文件**：`#include <PyRecT3D/explicit.h>`

适用：**多窗口实例**，或**需要详细配置**（指定渲染后端、按需裁剪模块、手动挂载消息队列、手动绑定 UI 与播放器）的工程。

显式 API 的哲学：**一切由调用者决定**。

| 事项 | 显式 API 行为 |
|------|---------------|
| 窗口 | 必须逐个 `createWindow()`；每个窗口自动分配一个**子消息队列** |
| 子队列挂载 | 单窗口自动挂载；**出现第二个窗口后必须调用 `mountQueues()`** |
| UI | 必须 `createUI()` 并 `bindWindow()` 显式绑定 |
| 视频播放器 | 必须 `createVideoPlayer(desc, 目标窗口)` 显式绑定子队列 |
| 全局事件 | `publishGlobal()` 以主消息队列身份发布 |
| 主循环 | `ExplicitApp::run()`（内部由 `App::run()` 统一启动） |

### 1.2 隐式 API（Implicit / Quick API）

**头文件**：`#include <PyRecT3D/implicit.h>`

适用：**想用最快的方式入门**——小游戏、教学示例、Scratch 式原型。**单窗口自动弱绑定**。

| 事项 | 隐式 API 行为 |
|------|---------------|
| 启动 | `quick::run(w,h,title)` 一行启动 |
| 窗口 | 只允许一个；自动挂载主队列 |
| UI | 自动绑定唯一窗口，`quick::button()` 即可 |
| 视频 | 自动绑定唯一窗口子队列，`quick::video()` 即可 |
| 对象 id | 用 `EntityHandle`（`quick::` 返回），不是裸指针 |
| 出现第二个窗口 | 隐式 API 不支持，必须切显式 API |

### 1.3 两套 API 的选择判据

```
只有一个窗口 ── 需要精细配置？ ── 否 → 隐式 API（quick::）
                   │
                   └── 是 → 显式 API（单窗口也能用）
有多个窗口 ──────────────────────── → 显式 API（强制）
```

---

## 2. 公共 API 总索引

### 2.1 头文件 → 模块对照

| 头文件 | 模块 | 主要内容 |
|--------|------|----------|
| `PyRecT3D/export.h` | 导出宏 / 版本 / 模块开关 | `PYRECT3D_API`、`PYRECT3D_VERSION_*`、`PYRECT3D_WITH_*` |
| `PyRecT3D/math.h` | 数学 | `Vec2/3/4`、`Mat4`、`Ray`、`RaycastHit`、`AABB` |
| `PyRecT3D/core.h` | 核心 / 场景 / 输入 | `Key`、`InputState`、`Entity`、`Scene`、`Engine` |
| `PyRecT3D/message.h` | 消息队列 / 事件 / 启动 | `EventPriority`、`EventType`、`Message`、`MessageQueue`、`MainMessageQueue`、`App` |
| `PyRecT3D/explicit.h` | **显式 API** | `Backend`、`Module`、`AppConfig`、`WindowDesc`、`WindowInstance`、`UIInstance`、`ExplicitApp` |
| `PyRecT3D/implicit.h` | **隐式 API** | `EntityHandle`、`quick::*` 全套函数 |
| `PyRecT3D/renderer.h` | 渲染 | `RenderMode`、`Renderer`、`RenderCommand`、`DirtyRegion`、`Compositor`、`Cube`、`ScreenCube`、`SoftRayTracer` |
| `PyRecT3D/gui.h` | GUI | `Rect`、`GuiEvent`、`Widget`、`Button`、`Text`、`InputBox`、`Container`、`ScrollContainer`、`Gui` |
| `PyRecT3D/collision.h` | 碰撞 | `CollisionEventKind`、`Limb`、`PhysicsCollider`、`ModelCollider`、`HitResult`、`CollisionWorld` |
| `PyRecT3D/video.h` | 视频（FFmpeg） | `VideoTexture`、`VideoDesc`、`VideoPlayer`、`MediaManager` |
| `PyRecT3D/audio.h` | 音频 | `Audio` |
| `PyRecT3D/io.h` | 模型加载 | `MeshData`、`Material`、`ObjModel`、`ObjLoader` |

### 2.2 函数速查表

| 能力 | 显式 API | 隐式 API |
|------|----------|----------|
| 启动 | `ExplicitApp::run()` | `quick::run()` / `quick::runSetup()` |
| 创建窗口 | `ExplicitApp::createWindow(WindowDesc)` | 自动（单窗口） |
| 创建 UI | `WindowInstance::createUI()` + `bindWindow()` | `quick::button()` / `quick::text()` |
| 创建立方体 | `new Cube(...)` + `Scene::addEntity()` | `quick::cube(...)` |
| 克隆实体 | `Scene::cloneEntity(id)` | `quick::clone(handle)` |
| 召唤实体 | `Scene::summonEntity(id, pos)` | `quick::summon(handle,x,y,z)` |
| 平移/旋转/缩放 | 直接 `Entity::set*` + `publishGlobal(Transform*)` | `quick::move/rotate/scale` |
| 射线 | `Scene::raycast(Ray)` / `CollisionWorld::raycast` | `quick::raycast(...)` |
| 视频 | `ExplicitApp::createVideoPlayer(desc, win)` | `quick::video(path, loop)` |
| 事件发布 | `ExplicitApp::publishGlobal(Message)` | 内部自动 |
| 退出 | `App::quit()` / `ExplicitApp::quit()` | `App::quit()` |

---

## 3. 基础契约（export.h）

```cpp
#include <PyRecT3D/export.h>

PYRECT3D_API        // 导出宏：Windows dllexport/dllimport；Linux 为空
PYRECT3D_CALL       // 调用约定：Windows __cdecl；其它为空

PYRECT3D_VERSION_MAJOR   // 0
PYRECT3D_VERSION_MINOR   // 2
PYRECT3D_VERSION_PATCH   // 0
PYRECT3D_VERSION_STRING  // "0.2.0"

PYRECT3D_HAS_RENDERER    // 恒为 1
PYRECT3D_WITH_GUI        // 1/0 —— 由构建脚本注入
PYRECT3D_WITH_VIDEO      // 1/0
PYRECT3D_WITH_AUDIO      // 1/0
PYRECT3D_WITH_PHYSICS    // 1/0
PYRECT3D_WITH_RAYTRACER  // 1/0
PYRECT3D_WITH_FFMPEG     // 1/0 —— 是否启用外部 FFmpeg 解码
```

> **裁剪语义**：`PYRECT3D_WITH_* = 0` 表示该模块**未参与编译**，对应 API 仍可调用但会返回 `false` / `nullptr`，不会链接失败。

---

## 4. 数学 API（math.h）

### 4.1 `Vec2`

```cpp
struct Vec2 {
    float x = 0.f, y = 0.f;
    Vec2() = default;
    Vec2(float x, float y);

    float length() const;
    Vec2  normalized() const;

    Vec2  operator+(const Vec2& o) const;
    Vec2  operator-(const Vec2& o) const;
    Vec2  operator*(float s) const;
    float dot(const Vec2& o) const;
};
```

**用途**：输入向量（WASD / 摇杆）必须用 `Vec2` 处理并做钳位，见 §14.2。

### 4.2 `Vec3`

```cpp
struct Vec3 {
    float x = 0.f, y = 0.f, z = 0.f;
    Vec3() = default;
    Vec3(float x, float y, float z);

    float length() const;
    Vec3  normalized() const;

    Vec3  operator+(const Vec3& o) const;
    Vec3  operator-(const Vec3& o) const;
    Vec3  operator*(float s) const;
    Vec3  operator-() const;          // 取负
    float dot(const Vec3& o) const;
    Vec3  cross(const Vec3& o) const;
};
```

### 4.3 `Vec4`

```cpp
struct Vec4 {
    float x = 0.f, y = 0.f, z = 0.f, w = 1.f;
    Vec4() = default;
    Vec4(float x, float y, float z, float w);
    Vec4(const Vec3& v, float w);
};
```

### 4.4 `Mat4`（列主序，OpenGL 惯例）

```cpp
struct Mat4 {
    float m[16];   // 单位矩阵初始化
    Mat4() = default;

    static Mat4 perspective(float fovDeg, float aspect, float nearP, float farP);
    static Mat4 orthographic(float left, float right, float bottom, float top,
                             float nearP, float farP);
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up);
    static Mat4 translation(const Vec3& t);
    static Mat4 scale(const Vec3& s);
    static Mat4 rotationX(float deg);
    static Mat4 rotationY(float deg);
    static Mat4 rotationZ(float deg);

    Mat4 operator*(const Mat4& o) const;
    Vec4 operator*(const Vec4& v) const;
    Mat4 inverse() const;
};
```

典型组合：

```cpp
Mat4 model = Mat4::translation(pos) * Mat4::rotationY(yaw) * Mat4::scale(scl);
Mat4 view  = Mat4::lookAt(camPos, camPos + camFront, Vec3(0,1,0));
Mat4 proj  = Mat4::perspective(70.f, aspect, 0.01f, 200.f);
```

### 4.5 `Ray` / `RaycastHit`

```cpp
struct Ray {
    Vec3 origin;
    Vec3 direction;   // 约定：调用方传单位向量（引擎内部会 normalize）
    Ray() = default;
    Ray(const Vec3& o, const Vec3& d);
};

struct RaycastHit {
    bool  hit = false;
    float distance = 0.f;
    Vec3  point;
    Vec3  normal;
    std::int64_t entityId = -1;   // 命中的实体 id，上层拿 id 去 Scene::getEntity 取实例
};
```

> **设计原则**：射线**只返回命中的实体 id**，不替上层做业务判断。你要挖方块、选中物体、做拾取，全部在上层用 `entityId` 处理。

### 4.6 `AABB`

```cpp
struct AABB {
    Vec3 min, max;
    AABB() = default;
    AABB(const Vec3& min, const Vec3& max);

    // slab 法射线求交：命中返回 true 并写出参数区间
    bool intersect(const Ray& ray, float& tNear, float& tFar) const;
    bool contains(const Vec3& p) const;
};
```

**起点在盒内时**：`intersect` 返回 `true`，`tNear` 为负、`tFar` 为正 —— 取命中距离时写 `t = (tNear > 0) ? tNear : tFar`。

---

## 5. 核心 / 场景 / 输入（core.h）

### 5.1 `Key`

```cpp
enum class Key : int {
    Unknown = 0,
    W, A, S, D,
    Space,
    Escape,
    AltF4,
    MouseLeft,
    MouseRight,
    MouseMiddle,
    Count
};
```

> 这是**默认输入接管集合**，不追求全键位覆盖。需要更多键位时走消息队列自定义事件（§7）。

### 5.2 `InputState`

```cpp
struct InputState {
    bool keys[static_cast<int>(Key::Count)] = {};
    int  mouseX = 0, mouseY = 0;
    int  mouseDeltaX = 0, mouseDeltaY = 0;
    bool mouseLocked = false;

    bool isDown(Key k) const;   // AltF4 / Unknown / Count 恒返回 false
};
```

### 5.3 `Entity`

```cpp
class Entity {
public:
    explicit Entity(std::int64_t id);
    virtual ~Entity();

    std::int64_t id() const;

    void setPosition(const Vec3& p);   Vec3 position() const;
    void setScale(const Vec3& s);      Vec3 scale() const;
    void setRotation(const Vec3& eulerDeg);  Vec3 rotation() const;

    AABB bounds() const;   // 由 position + scale 推出的世界 AABB
};
```

**注意**：`Entity` 只持有变换状态；渲染由 `Renderer` 决定，业务逻辑不写在实体里。

### 5.4 `Scene`

```cpp
class Scene {
public:
    Scene();
    ~Scene();

    std::int64_t addEntity(Entity* e);        // 返回自增 id（从 1 开始）
    void        removeEntity(std::int64_t id);
    Entity*     getEntity(std::int64_t id);   // 不存在返回 nullptr

    // 克隆：复制一个「场上现成实体」的当前完整状态（位置/旋转/缩放）
    std::int64_t cloneEntity(std::int64_t id);

    // 召唤：按模板实体的缩放入参、在给定位置新建实例
    std::int64_t summonEntity(std::int64_t prototypeId, const Vec3& position);

    RaycastHit raycast(const Ray& ray);

    std::size_t entityCount() const;
    void forEach(const std::function<void(std::int64_t, Entity&)>& fn) const;
};
```

> **克隆 vs 召唤（本引擎的差异化能力）**
> - `cloneEntity` 拷贝场内实体的**当前状态快照** → 对 Scratch 用户最直观（"选中角色 → 克隆一份"）。
> - `summonEntity` 按模板初始参数**新建** → 对应 Unity/UE 的 Prefab 实例化语义。
> 两者都内置，入门用户先用 `clone`，进阶再学 `summon`。

### 5.5 `Engine`（窗口 + 消息循环）

```cpp
class Engine {
public:
    Engine();
    ~Engine();

    // nativeWindow = 0 表示自建窗口；传入外部句柄表示外部接管（如 QT / Android Surface）
    bool init(int width, int height, const std::string& title,
              void* nativeWindow = nullptr);
    void shutdown();

    int  run();                        // 模式1：自带消息循环，阻塞
    bool pump();                       // 模式2：外部接管，单步推进；false = 请求关闭
    void requestClose();

    void setFrameCallback(FrameCallback cb);   // void(InputState&, double dt)
    const InputState& input() const;
    void lockMouse(bool locked);
    void* nativeHandle() const;
};
```

两种运行模式：

```cpp
// 模式1：引擎自带循环
Engine engine;
engine.init(800, 600, "demo");
engine.run();

// 模式2：外部接管（如 QT 启动器）
engine.init(800, 600, "embedded", qtWinId);
while (running) {
    if (!engine.pump()) break;
    const InputState& in = engine.input();
    // ...
}
```

---

## 6. 消息队列与事件（message.h）

这是 v0.2 的**架构核心**：所有输入、场景操作、碰撞、媒体事件都走消息队列；渲染指令也在主线程消费。

### 6.1 `EventPriority`（数值越小优先级越高）

```cpp
enum class EventPriority : std::int32_t {
    WindowInput = 0,   // 窗口级：关闭/最小化/最大化/边框变更/WASD/鼠标键/鼠标位置
    Physics     = 1,   // 物理事件
    Collision   = 2,   // 碰撞事件
    Audio       = 3,   // 音效事件
    Media       = 4,   // 播放器 / 视频帧事件
    Normal      = 5,   // 普通业务消息
};
```

**调度顺序固定为**：
`窗口输入 > 物理 > 碰撞 > 音效 > 媒体 > 普通`

同一优先级内按发布顺序 FIFO（`seq` 递增）。

### 6.2 `EventType`

```cpp
enum class EventType : int {
    None = 0,

    // 窗口（最高优先级）
    WindowClose, WindowMinimize, WindowMaximize, WindowRestore, WindowResize,
    WindowBorderChange, WindowFocusGained, WindowFocusLost,

    // 输入（最高优先级）
    KeyDown, KeyUp, MouseMove, MouseButtonDown, MouseButtonUp, MouseWheel,

    // 全局 / 程序级（由主消息队列发布）
    AppQuit, AppPause, AppResume, AppResumeFromBackground,
    InputBindingChanged,      // 切换鼠标模式 / 绑定 WASD
    ViewportTransfer,         // 例：据点转交至 B 视口
    SceneSwitch,
    WindowCreate, WindowDestroy,

    // 场景（以主消息队列身份发布）
    SceneCreate, SceneDestroy,
    TransformMove, TransformRotate, TransformScale,
    EntityClone, EntitySummon, EntityRemove,
    PhysicsTick,

    // 碰撞
    CollisionEnter, CollisionExit, CollisionStay,
    CollisionPress, CollisionRelease,
    CollisionDragOut, CollisionDragMove, CollisionClickOutside,
    RaycastHitReport,

    // 音频 / 媒体
    AudioFinished, VideoFinished, VideoPaused, VideoError, VideoFrameReady,

    // 渲染
    RenderCommandSubmit, RenderRegionInvalidate,

    Count
};
```

### 6.3 `MessagePayload`

```cpp
struct MessagePayload {
    std::uint64_t entityId = 0;
    std::int64_t  windowId = 0;
    std::int64_t  playerId = 0;
    std::int64_t  queueId  = 0;
    std::int32_t  code     = 0;      // 键码 / 按钮码 / 错误码
    float         v[8]     = {};     // 通用数值：坐标、变换、拖拽量
    std::string   text;
    void*         user     = nullptr;
};
```

> 用定长结构而非 `std::any`，是为了**跨编译器 ABI 稳定**（DLL 分发场景必须）。

### 6.4 `Message`

```cpp
struct Message {
    EventType     type = EventType::None;
    EventPriority priority = EventPriority::Normal;
    std::int64_t  sourceQueue = 0;    // 0 = 主消息队列身份
    std::int64_t  targetQueue = -1;   // -1 = 广播；>=0 = 只投递给该子队列
    std::uint64_t seq = 0;            // 队列自动填充
    MessagePayload payload;
};

using EventHandler = std::function<void(const Message&)>;
```

### 6.5 `MessageQueue`（子消息队列）

```cpp
class MessageQueue {
public:
    explicit MessageQueue(std::int64_t id = 0);   // id=0 时自动分配
    ~MessageQueue();

    std::int64_t id() const;

    bool attachToMain();       // 多窗口必须显式挂载；单窗口由引擎自动挂载
    bool detachFromMain();
    bool attached() const;

    void publish(const Message& m);       // 经主队列，默认回投自己
    void publishLocal(const Message& m);  // 仅本队列内部投递

    bool poll(Message& out);              // 按优先级取出一个
    std::size_t pending() const;

    void setHandler(EventHandler handler);
    void dispatch();                      // 有 handler 时才消费消息
};
```

> **重要**：`dispatch()` 在**未设置 handler** 时**不消费消息**，消息会保留给 `poll()`。这是为了支持"手动轮询"与"回调派发"两种用法。

### 6.6 `MainMessageQueue`（全局唯一单例）

```cpp
class MainMessageQueue {
public:
    static MainMessageQueue& instance();

    std::uint64_t publish(const Message& m);   // targetQueue>=0 → 自动路由到该子队列
    void          pump();                      // 一次性派发：先全局，再各子队列
    bool          poll(Message& out);
    std::size_t   pending() const;

    void subscribe(EventHandler handler);      // 全局订阅

    std::size_t   childCount() const;
    MessageQueue* findChild(std::int64_t id);
    MessageQueue* firstChild();                // 单窗口弱绑定用

    bool attachChild(MessageQueue* child);
    bool detachChild(MessageQueue* child);

    std::uint64_t threadId() const;            // 绑定的渲染主线程 id
};
```

**路由规则**：

| 场景 | 行为 |
|------|------|
| `targetQueue = -1` | 广播：进入全局队列，所有 `subscribe` 的处理器都会收到 |
| `targetQueue >= 0` 且子队列已挂载 | 直接投递到该子队列（只有它能收到） |
| `targetQueue >= 0` 但子队列不存在 | 退化进入全局队列 |

### 6.7 `App`（统一启动契约）

```cpp
class App {
public:
    static int  run();     // 拉起消息循环 + 3D 场景，阻塞到关闭
    static void quit();    // 发布 AppQuit
    static bool pump();    // 单步：外部接管模式用
};
```

> `ExplicitApp::run()` 与 `quick::run()` 内部都调用 `App::run()`，保证启动路径统一。

---

## 7. 显式 API（explicit.h）

### 7.1 `Backend`

```cpp
enum class Backend : int {
    Auto  = 0,   // 桌面→GL33，Android/Web→GLES3
    GL33  = 1,   // 桌面 OpenGL 3.3 Core
    GLES3 = 2,   // OpenGL ES 3.0（Android / WebGL2）
};
```

### 7.2 `Module`（模块裁剪查询位）

```cpp
enum class Module : int {
    Renderer = 1 << 0,
    GUI      = 1 << 1,
    Video    = 1 << 2,
    Audio    = 1 << 3,
    Physics  = 1 << 4,
    RayTracer= 1 << 5,
    Python   = 1 << 6,
};
```

配合 `ExplicitApp::hasModule(Module)` 判断当前构建是否含某模块。

### 7.3 `AppConfig`

```cpp
struct AppConfig {
    int   width  = 1280;
    int   height = 720;
    std::string title = "PyRecT3D";
    Backend backend = Backend::Auto;
    bool  vsync = true;
    bool  enableVideo  = true;
    bool  enableAudio  = true;
    bool  enablePhysics = true;
    bool  enableGUILocalUpdate = true;  // 2D/3D 分离局部更新（脏区域）
    bool  multithreadRender = false;    // true = 子线程生产渲染指令，主线程消费
    float renderScale = 1.0f;
};
```

### 7.4 `WindowDesc`

```cpp
struct WindowDesc {
    int   width  = 1280;
    int   height = 720;
    std::string title = "Window";
    bool  resizable = true;
    bool  fullscreen = false;
    bool  uiAutoBind = false;      // 单窗口可置 true 走弱绑定
    void* nativeHandle = nullptr;  // 外部接管（QT / Android Surface）
};
```

### 7.5 `WindowInstance`

```cpp
class WindowInstance {
public:
    std::int64_t id() const;
    MessageQueue* queue();      // 本窗口专属子消息队列
    Entity*       camera();     // 相机实体
    Scene&        scene();      // 本窗口 3D 场景

    UIInstance*   createUI();   // 创建 UI（多窗口必须再 bindWindow）

    void renderScene();         // 提交 3D 帧，保留帧缓冲
    void present();             // 3D 帧缓冲 + 新 UI 指令合成上屏

    void setTitle(const std::string& title);
    int  width() const;
    int  height() const;
};
```

> `renderScene()` + `present()` 的分离，就是「3D 视口不重绘、只更新 2D UI」的落地方式：
> 3D 画面不动时 `Compositor::sceneDirty() == false`，`present()` 只把新的 UI 指令叠加。

### 7.6 `UIInstance`

```cpp
class UIInstance {
public:
    UIInstance();

    bool bindWindow(WindowInstance* window);   // 单窗口可省略（弱绑定）
    std::int64_t boundWindowId() const;

    Gui& gui();

    Button*          addButton(const Rect& r, const std::string& text);
    Text*            addText(const Rect& r, const std::string& text);
    InputBox*        addInputBox(const Rect& r, const std::string& text);
    Container*       addContainer(const Rect& r);
    ScrollContainer* addScrollContainer(const Rect& r);

    void drawAll();
};
```

### 7.7 `ExplicitApp`

```cpp
class ExplicitApp {
public:
    ExplicitApp();
    ~ExplicitApp();

    bool configure(const AppConfig& cfg);              // 必须在 createWindow 前
    WindowInstance* createWindow(const WindowDesc& desc);
    WindowInstance* window(std::int64_t id);
    std::size_t     windowCount() const;

    bool mountQueues();                                // 多窗口必须调用

    VideoPlayer* createVideoPlayer(const VideoDesc& desc,
                                   WindowInstance* bindTo = nullptr);

    std::uint64_t publishGlobal(const Message& m);

    int  run();
    bool pump();
    void quit();

    bool hasModule(Module m) const;
};
```

完整显式示例：

```cpp
#include <PyRecT3D/explicit.h>
using namespace pyrect3d;

int main() {
    ExplicitApp app;

    AppConfig cfg;
    cfg.backend = Backend::Auto;
    cfg.enableGUILocalUpdate = true;
    app.configure(cfg);

    WindowDesc da; da.title = "A"; da.width = 800; da.height = 600;
    WindowDesc db; db.title = "B"; db.width = 640; db.height = 480;
    WindowInstance* wa = app.createWindow(da);
    WindowInstance* wb = app.createWindow(db);

    app.mountQueues();                       // 多窗口：必须显式挂载

    UIInstance* uiA = wa->createUI();
    uiA->bindWindow(wa);                     // 多窗口：必须显式绑定
    Button* btn = uiA->addButton(Rect{20,20,160,44}, "退出");
    btn->onClick = [] { App::quit(); };

    Entity* cube = new Cube(Vec3(0,0,-5), Vec3(1,1,1));
    std::int64_t id = wa->scene().addEntity(cube);

    Message mv;
    mv.type = EventType::TransformRotate;
    mv.sourceQueue = 0;                                  // 主队列身份
    mv.payload.entityId = (std::uint64_t)id;
    mv.payload.v[1] = 45.f;
    app.publishGlobal(mv);

    return app.run();
}
```

---

## 8. 隐式 API（implicit.h）

### 8.1 `EntityHandle`

```cpp
struct EntityHandle {
    std::int64_t id = -1;
    bool valid() const { return id >= 0; }
};
```

### 8.2 `quick::` 全量函数

```cpp
namespace quick {

// 启动
int run(int width = 1280, int height = 720,
        const std::string& title = "PyRecT3D");
int runSetup(int width, int height, const std::string& title,
             void (*setup)());

// 对象
EntityHandle cube(float x = 0, float y = 0, float z = -5,
                  float sx = 1, float sy = 1, float sz = 1);
EntityHandle screenCube(float x, float y, float w, float h);

// 克隆 / 召唤
EntityHandle clone(EntityHandle src);                              // 复制场上状态
EntityHandle summon(EntityHandle prototype, float x, float y, float z); // 按模板新建

// 变换（内部转成消息，以主消息队列身份发布）
void move(EntityHandle e, float x, float y, float z);
void rotate(EntityHandle e, float rx, float ry, float rz);
void scale(EntityHandle e, float sx, float sy, float sz);
void remove(EntityHandle e);

// 射线
HitResult raycast(float ox, float oy, float oz,
                  float dx, float dy, float dz,
                  float maxDistance = 100.f);

// UI（自动绑定唯一窗口）
Button* button(float x, float y, float w, float h, const std::string& text);
Text*   text(float x, float y, const std::string& content);

// 视频（自动绑定唯一窗口子队列）
VideoPlayer* video(const std::string& path, bool loop = false);

// 当前上下文
Scene& scene();
Gui&   gui();

}
```

### 8.3 隐式 API 完整示例

```cpp
#include <PyRecT3D/implicit.h>
using namespace pyrect3d;

int main() {
    quick::runSetup(1280, 720, "Quick Demo", [] {
        EntityHandle a = quick::cube(0, 0, -5);
        EntityHandle b = quick::cube(2, 0, -6);

        EntityHandle c = quick::clone(a);   // 克隆（Scratch 思维）
        quick::move(c, -2, 0, -5);

        Button* btn = quick::button(20, 20, 140, 40, "复位");
        btn->onClick = [a] { quick::move(a, 0, 0, -5); };
        btn->onDragOut = [] { /* 拖出按钮区域 */ };

        quick::video("assets/demo.mp4", true);
    });
}
```

> **隐式 API 的弱绑定**：只有**唯一窗口**时才自动绑定。创建第二个窗口会让隐式上下文失效，必须切显式 API。

---

## 9. 渲染 API（renderer.h）

### 9.1 `RenderMode`

```cpp
enum class RenderMode : int { Forward, Wireframe };
```

### 9.2 `Renderer`

```cpp
class Renderer {
public:
    bool init(int width, int height);
    void shutdown();

    // v0.2 帧边界：beginFrame 清屏一次，之后多次 drawEntity 累积
    void beginFrame();
    void endFrame();

    int width() const;
    int height() const;

    void setMode(RenderMode mode);
    RenderMode mode() const;

    void setClearColor(float r, float g, float b, float a = 1.f);

    void drawEntity(const Entity& e);                 // 3D
    void drawScreenRect(float x, float y, float w, float h);  // 2D

    void setAntiAliasing(bool on);
    bool saveFrame(const std::string& path);          // 导出 PPM
};
```

> **v0.1 的坑已修**：旧版 `drawEntity` 内部每次清屏，导致一帧内画多个实体只剩最后一个。
> 现在必须 `beginFrame()` → 多次 `drawEntity()` → `endFrame()`。

### 9.3 渲染指令合并与局部更新

```cpp
enum class CommandType : int {
    Clear, DrawRect2D, DrawEntity3D, SetUniform, DrawTexture, Composite, Present,
};

struct RenderCommand {
    CommandType type = CommandType::DrawRect2D;
    float x = 0, y = 0, w = 0, h = 0;
    float r = 1, g = 1, b = 1, a = 1;
    std::int64_t entityId = -1;
    std::int64_t textureId = -1;
    bool is2D() const;
};

struct DirtyRegion {
    int x = 0, y = 0, w = 0, h = 0;
    bool valid = false;
    void unionWith(const DirtyRegion& o);
};

class Compositor {
public:
    void attach(Renderer* renderer);

    void submit(const RenderCommand* cmds, std::size_t count);  // 自动合批
    void invalidate(const DirtyRegion& region);
    DirtyRegion dirtyRegion() const;
    void clearDirty();

    void setSceneDirty(bool dirty);   // false = 复用旧 3D 帧缓冲
    bool sceneDirty() const;

    void applyEffect3D(std::int32_t effectCode, float strength);  // 眩晕等 3D 特效
    void present();

    std::size_t mergedCount() const;  // 本帧合并后的提交次数
};
```

**「Docker 式软更新」用法**：

```cpp
compositor.setSceneDirty(false);   // 3D 画面这帧不动
RenderCommand ui[4] = { /* 新 UI 矩形 */ };
compositor.submit(ui, 4);          // 只提交 UI 指令，不重绘 3D
compositor.present();              // 旧 3D 帧缓冲 + 新 UI 叠加
```

### 9.4 图元

```cpp
class Cube : public Entity {
public:
    Cube(const Vec3& position = Vec3(), const Vec3& scale = Vec3(1,1,1));
};

class ScreenCube : public Entity {
public:
    ScreenCube(float x, float y, float w, float h);
    float x() const, y() const, width() const, height() const;
};
```

### 9.5 `SoftRayTracer`（软件光追，非 RTX）

```cpp
class SoftRayTracer {
public:
    SoftRayTracer();
    bool render(const Scene& scene, int w, int h,
                std::vector<unsigned char>& outRGB);   // 输出 w*h*3 RGB
};
```

> 纯 CPU 计算，适合预览/离线渲染。实时场景请配合消息队列把光追计算丢到子线程，
> 主线程只负责 `Compositor::present()`，避免卡死。

---

## 10. GUI API（gui.h）

### 10.1 `Rect`

```cpp
struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    Rect() = default;
    Rect(float x_, float y_, float w_, float h_);
    bool contains(float px, float py) const;
};
```

### 10.2 `GuiEvent`（v0.2 事件全集）

```cpp
struct GuiEvent {
    bool hovered = false;   // 指针在组件内
    bool clicked = false;   // 在组件内按下并松开
    bool entered = false;   // 指针进入
    bool exited  = false;   // 指针离开
    bool pressed = false;   // 在组件内按下
    bool released = false;  // 在组件内松开
    bool dragMove = false;  // 按住并在组件内移动
    bool draggedOut = false;// 按住并移出组件
    bool clickOutside = false; // 拖出后在区域外松开
    int  mouseX = 0, mouseY = 0;
    float dragDx = 0.f, dragDy = 0.f;
};
```

### 10.3 `Widget` / `Button` / `Text` / `InputBox`

```cpp
class Widget {
public:
    explicit Widget(const Rect& r);
    virtual ~Widget();

    void setRect(const Rect& r);
    Rect rect() const;
    void setVisible(bool v);
    bool visible() const;

    void setTexture(const std::string& texPath);   // 组件贴图
    const std::string& texture() const;

    virtual void onEvent(const GuiEvent& e);
    virtual void draw() = 0;
};

class Button : public Widget {
public:
    Button(const Rect& r, const std::string& text = "");
    void onEvent(const GuiEvent& e) override;
    void draw() override;

    // v0.1
    std::function<void()> onClick;
    std::function<void()> onHover;

    // v0.2 新增
    std::function<void()> onEnter;
    std::function<void()> onExit;
    std::function<void()> onPress;
    std::function<void()> onRelease;
    std::function<void(float dx, float dy)> onDragMove;
    std::function<void()> onDragOut;
    std::function<void()> onClickOutside;

    std::string text;
};

class Text : public Widget {
public:
    Text(const Rect& r, const std::string& text = "");
    std::string text;
    float size = 16.f;
};

class InputBox : public Widget {
public:
    InputBox(const Rect& r, const std::string& text = "");
    std::string text;
    bool focused = false;
};
```

### 10.4 容器 / GUI 系统

```cpp
class Container : public Widget {
public:
    explicit Container(const Rect& r);
    void addChild(Widget* w);
};

class ScrollContainer : public Container {
public:
    explicit ScrollContainer(const Rect& r);
    void onEvent(const GuiEvent& e) override;   // 处理滚轮
    void setScroll(float offset);
    float scroll() const;
};

class Gui {
public:
    void addWidget(Widget* w);
    void removeWidget(Widget* w);
    void dispatch(const GuiEvent& base);       // 由消息循环调用
    void drawAll();
};
```

**边界**：GUI 刻意保持极简 —— **不含**表格、树、标签页、Dock、动画、主题系统。
需要复杂控件时在 `Widget` 之上自行组合（`Container` 已支持子组件与布局）。

---

## 11. 碰撞 API（collision.h）

### 11.1 `CollisionEventKind`

```cpp
enum class CollisionEventKind : int {
    Enter = 0,        // 进入碰撞区
    Exit,             // 离开碰撞区
    Stay,             // 停留在区内（每帧）
    Press,            // 在区内按下
    Release,          // 在区内松开
    DragMove,         // 按住并在区内移动
    DragOut,          // 按住移出区域
    ClickOutside,     // 拖出后在区域外松开
};
```

映射关系：`Enter→CollisionEnter`、`Exit→CollisionExit`、`Stay→CollisionStay`、
`Press→CollisionPress`、`Release→CollisionRelease`、`DragMove→CollisionDragMove`、
`DragOut→CollisionDragOut`、`ClickOutside→CollisionClickOutside`。

### 11.2 双碰撞箱

```cpp
struct Limb {
    std::string name;      // 肢体名（命中后回填 HitResult::limbName）
    AABB  localBox;        // 相对模型原点的局部 AABB
    Vec3  offset;          // 相对父肢体位移
    bool  enabled = true;
};

// 物理碰撞箱：负责与世界物理的碰撞计算，统一用射线判定
struct PhysicsCollider {
    AABB  box;
    bool  enabled = true;
    bool  useRaycast = true;
    float rayStep = 0.08f;
    float maxDistance = 8.f;
};

// 模型碰撞箱：负责模型展示与精细碰撞；内部挂一个小型物理空间
struct ModelCollider {
    AABB root;                 // 整体粗包围盒
    std::vector<Limb> limbs;   // 肢体列表
    bool  enabled = true;
    bool  useLocalSpace = true;// 肢体碰撞只在内部子空间运算
    float localScale = 1.f;
};
```

**为什么两套？**

| 碰撞箱 | 职责 | 计算方式 | 用途 |
|--------|------|----------|------|
| 物理碰撞箱 | 玩家与世界物理碰撞 | 统一射线（AABB slab） | 移动阻挡、地形碰撞、快速粗判 |
| 模型碰撞箱 | 模型展示 + 肢体级精细碰撞 | 内部小型物理空间，逐肢体 AABB | 命中判定到具体肢体、精确拾取 |

### 11.3 `HitResult`

```cpp
struct HitResult {
    bool  hit = false;
    bool  hitModel = false;    // 命中模型碰撞箱
    bool  hitPhysics = false;  // 命中物理碰撞箱
    float distance = 0.f;
    Vec3  point;
    Vec3  normal;
    std::int64_t entityId = -1;
    std::string limbName;     // 命中的肢体名
};
```

### 11.4 `CollisionWorld`

```cpp
class CollisionWorld {
public:
    CollisionWorld();

    std::int64_t addPhysicsBox(const PhysicsCollider& c);
    std::int64_t addModelBox(const ModelCollider& c);
    void updatePhysicsBox(std::int64_t handle, const PhysicsCollider& c);
    void updateModelBox(std::int64_t handle, const ModelCollider& c);
    void remove(std::int64_t handle);
    void clear();

    void step(float dt);    // 物理推进，命中发布 PhysicsTick（优先级 Physics）

    HitResult raycast(const Ray& ray, float maxDistance = 100.f,
                      bool includeModel = true, bool includePhysics = true) const;

    std::int64_t addTriggerZone(const AABB& box, std::int64_t ownerEntityId = 0);

    void setEventHandler(CollisionEventKind kind, CollisionCallback cb);
    void clearEventHandlers();

    void processPointer(float x, float y, bool pressed, bool released, float dt);
};
```

**射线优先级**：模型箱（含肢体）先判，命中即取更小 `t`；物理箱兜底。

**区域事件实例**：

```cpp
CollisionWorld world;
world.addTriggerZone(AABB(Vec3(-1,-1,0), Vec3(1,1,0)), entityId);
world.setEventHandler(CollisionEventKind::Enter, [](const Message& m){
    // m.payload.entityId / v[0..5] 区域坐标
});
world.setEventHandler(CollisionEventKind::DragOut, [](const Message&){ /* 拖出 */ });
world.setEventHandler(CollisionEventKind::ClickOutside, [](const Message&){ /* 区域外松开 */ });

// 每帧喂指针状态
world.processPointer(mx, my, pressedNow, releasedNow, dt);
```

> `processPointer` 内部维护状态机：`Enter/Exit/Stay` 由前后帧内外变化推导，
> `Press/Release` 由按下与松开边沿触发，`DragOut` 由「按住 + 移出」触发，
> `ClickOutside` 由「拖出后松开」触发。**区域外松开发 `ClickOutside`，不算 `Release`**。

---

## 12. 视频 API（video.h，FFmpeg 多线程解码）

### 12.1 `VideoTexture`（动态材质）

```cpp
struct VideoTexture {
    std::int64_t id = 0;           // 引擎内部纹理句柄
    int   width = 0, height = 0;
    std::uint64_t frameSerial = 0; // 每次上传 +1
    bool  valid = false;
};
```

> 解码帧直接被包装成**引擎标准材质**：贴到任意实体 / GUI 组件即可播放视频，
> 不需要额外的"视频渲染通道"。

### 12.2 `VideoDesc`

```cpp
struct VideoDesc {
    std::string path;
    bool  loop = false;
    bool  decodeAudio = true;
    int   decodeThreads = 2;
    float volume = 1.0f;
    bool  autoPlay = true;
};
```

### 12.3 `VideoPlayer`

```cpp
class VideoPlayer {
public:
    std::int64_t id() const;
    std::int64_t boundQueueId() const;

    bool bindQueue(MessageQueue* queue);   // 多窗口必须显式绑定
    bool autoBind();                       // 单窗口弱绑定（childCount==1 时成功）

    bool open(const VideoDesc& desc);
    bool play();
    bool pause();
    bool stop();
    bool seek(float seconds);

    bool  playing() const;
    float duration() const;
    float position() const;

    VideoTexture texture() const;
    bool uploadPendingFrame();   // 主线程每帧调用：检测就绪帧并上传 GPU

    void  setVolume(float v);   // 0..1
    float volume() const;
};
```

### 12.4 `MediaManager`（进程唯一）

```cpp
class MediaManager {
public:
    static MediaManager& instance();

    VideoPlayer* createPlayer();                 // 通过全局实例创建多路播放器
    void         destroyPlayer(VideoPlayer* p);

    void tick();                                 // 主线程每帧：统一上传所有实例的帧
    std::size_t playerCount() const;
};
```

### 12.5 线程模型（必须理解）

```
子线程（FFmpeg 解码）
   ↓ 解出 RGB 帧写入 CPU 缓冲
   ↓ frameReady.store(true)
主线程（持有 GL 上下文）
   ↓ uploadPendingFrame() 检测标志
   ↓ glTexSubImage2D 上传 → VideoTexture.frameSerial++
```

**禁止**在子线程直接触碰任何 GL 调用。

### 12.6 播放器事件路由

| 步骤 | 行为 |
|------|------|
| 1 | 播放完成 / 暂停 / 出错 → 构造 `Message`，`priority = Media`，`targetQueue = boundQueueId` |
| 2 | 投递到 **主消息队列** |
| 3 | 主队列按 `playerId` 与 `targetQueue` 路由 |
| 4 | **只有该播放器绑定的子队列收到**，其它窗口不受影响 |

事件类型：`VideoFinished` / `VideoPaused` / `VideoError` / `VideoFrameReady`。

### 12.7 绑定规则

| 环境 | 规则 |
|------|------|
| 单窗口 | `quick::video()` 或 `autoBind()` 自动绑定唯一子队列 |
| 多窗口 | 必须 `createVideoPlayer(desc, 目标窗口)` 或 `bindQueue()` 显式绑定 |

---

## 13. 音频 / 模型 IO

### 13.1 `Audio`

```cpp
class Audio {
public:
    bool playMusic(const std::string& path, bool loop = true);
    void stopMusic();
    void pauseMusic();
    void resumeMusic();
    void  setVolume(float v);   // 0..1
    float volume() const;
};
```

### 13.2 `ObjLoader`

```cpp
struct MeshData {
    std::vector<float> positions;   // xyz 平铺
    std::vector<float> normals;     // xyz 平铺
    std::vector<float> uvs;         // uv 平铺
    std::vector<unsigned int> indices;
};

struct Material {
    std::string name;
    std::string diffuseTex;         // map_Kd
    Vec3 diffuseColor{0.8f,0.8f,0.8f};
    Vec3 specularColor{1,1,1};
    float shininess = 32.f;
    bool hasTexture = false;
};

struct ObjModel {
    MeshData mesh;
    std::vector<Material> materials;
    std::string name;
};

class ObjLoader {
public:
    static bool load(const std::string& objPath, ObjModel& out);
    static bool loadFromMemory(const std::string& objContent, ObjModel& out);
};
```

---

## 14. 硬约束（违反必崩或必泄漏）

### 14.1 OpenGL 上下文只能在渲染主线程使用

- 一个 GL 上下文同一时刻**只能绑定在一个线程**。
- 子线程**不得**调用任何 `gl*` 函数。
- 子线程要渲染 → 构造渲染指令 → 压入消息队列 → 主线程 `MainMessageQueue::pump()` 消费执行。
- 主队列的绑定线程 id 不匹配时，GPU 资源操作必须拒绝执行。

### 14.2 输入向量必须钳位（斜向加速 bug）

WASD 同时按 W+A → 向量 `(-1, 1)`，模长 `√2 ≈ 1.414` → 斜向移动快 1.414 倍。
摇杆斜推同理。**所有输入在进入移动逻辑前统一处理**：

```cpp
Vec2 dir = { x, y };            // WASD 或摇杆输出
float len = dir.length();
if (len > 1.f) dir = dir.normalized();
velocity = dir * moveSpeed;     // 任意方向速度一致
```

想要古早"斜向更快"手感时，可在配置层关闭钳位（`clampInput = false`）。

### 14.3 GPU 资源释放必须幂等且成链

规则：**父资源只有在所有子资源都不再依赖时才能删除**。

- 每有一个子资源，父资源依赖计数 +1；
- 子资源全部释放 → 计数归零 → 父资源可删；
- `release_gpu_resources()` 必须可重复调用（重复调用不崩溃）。

触发时机：
- Web：`visibilitychange` 隐藏 / `pagehide`
- Android：`onPause` / `onDestroy`
- 桌面：`WindowClose` / 进程退出

### 14.4 消息队列使用纪律

| 场景 | 发布者 |
|------|--------|
| 单个场景内的常规操作（变换、克隆、召唤、射线结果） | **主消息队列身份**（`sourceQueue = 0`） |
| 全局大型事件（切换鼠标模式、绑定 WASD、关闭程序、跨视口转交） | **主消息队列**（`publishGlobal` / `publish`） |
| 窗口内部私有事件 | 该窗口子队列 `publish()` |

多窗口时每个窗口**必须**有独立子队列，且**必须** `mountQueues()` 挂到主队列。

### 14.5 渲染顺序

```
MainMessageQueue::pump()      // 1. 窗口输入最高优先级
  → 物理 step()
  → 碰撞 processPointer()
  → 媒体 tick()（帧上传）
  → sceneDirty? renderScene() : 复用帧缓冲
  → UI 指令提交 + Compositor::present()
```

---

## 15. 构建与分发

### 15.1 目录结构

```
PyRecT3D/
├── CMakeLists.txt          # 主构建（DLL/LIB、模块裁剪）
├── build.bat               # Windows 交互式一键构建
├── build.sh                # Linux 一键构建
├── cpp/
│   ├── include/PyRecT3D/   # 对外契约头文件（仅声明）
│   └── src/
│       ├── core/           # 引擎/消息队列/显式/隐式/平台粘合
│       ├── math/
│       ├── renderer/       # 渲染 + 合成层 + gl_backend
│       ├── gui/
│       ├── io/
│       ├── audio/
│       ├── physics/        # 双碰撞箱
│       ├── media/          # FFmpeg 播放器
│       └── binding/        # pybind11（一个头文件一份）
│   └── third_party/glad/   # 自带 GL 加载器
├── examples_cpp/           # basic_cube / explicit_demo / implicit_demo
├── tests/smoke_v2.cpp      # 无头冒烟测试
├── docs/                   # API.md + API-part2/3.md + API.docx
├── WEB/                    # WASM 单包 / 多包
├── Android/apk/            # APK 占位
└── wiki/                   # GitHub Wiki 页源文件
```

### 15.2 `build.bat` 交互项（Windows）

```
[1/5] 产物类型：1 = DLL（LoadLibrary 动态加载）  2 = LIB（静态链接）
[2/5] CL 编译器路径（回车 = 使用 PATH 中的 cl.exe）
[3/5] 构建配置：1 = Release（O2 全程序优化）  2 = Debug
[4/5] 是否裁剪模块：GUI / 视频 / 音频 / 物理 / 软光追 逐项 y/n
[5/5] 是否启用外部 FFmpeg
```

### 15.3 CMake 选项

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DPYRECT3D_LIB_TYPE=shared        # shared=DLL / static=LIB
      -DPYRECT3D_WITH_GUI=ON \
      -DPYRECT3D_WITH_VIDEO=ON \
      -DPYRECT3D_WITH_AUDIO=ON \
      -DPYRECT3D_WITH_PHYSICS=ON \
      -DPYRECT3D_WITH_RAYTRACER=ON \
      -DPYRECT3D_WITH_FFMPEG=OFF \
      -DPYRECT3D_WITH_GL=OFF \
      -DPYRECT3D_BUILD_PYTHON=ON \
      -DPYRECT3D_BUILD_EXAMPLES=ON \
      -DPYRECT3D_BUILD_TESTS=ON ..
```

### 15.4 体积优化（发布瘦身）

- 编译期锁定**单一渲染后端**（不带 GL33 与 GLES3 两套），另一套作为死代码被删。
- `-O3 -ffunction-sections -fdata-sections` + `-Wl,--gc-sections`（Linux）；`/O2 /GL`（MSVC）。
- 发布版 `strip` 掉符号表。
- 按需裁剪模块（`PYRECT3D_WITH_* = OFF` 的源码根本不参与编译）。

> v0.1 实测参考：DX11 构建 376 KB；合并头文件 + 内联转换层 + O3 后可压到百 KB 量级。

### 15.5 DLL 分发（ABI 契约）

- 调用方**只包含 `cpp/include/PyRecT3D/*.h`**（极轻量，只有声明）。
- 运行期 `LoadLibrary("PyRecT3D.dll")` + `GetProcAddress`（或直接导入库）。
- **ABI 稳定要求**：已发布版本不得随意改动函数签名与结构体布局。

---

## 16. WASM（Web）分发

### 16.1 两种模式

| 模式 | 目录 | 适用 | 特征 |
|------|------|------|------|
| 单包 | `WEB/single-package/` | 小游戏平台（不支持分包） | 全部打一个 wasm，`index.html` 直载 |
| 多包 | `WEB/multi-package/` | 抖音 / 小红书等 | 主包 + 资源分片 + `manifest.json`（含 sha256，可断点续传） |

### 16.2 平台一致性

- Web 端 API 与桌面端**完全一致**：同名、同参、同语义。
- 渲染后端固定 `Backend::GLES3`（WebGL2 ≈ ES 3.0）。
- 平台差异只在 `WEB/bridge/pyrect3d_web.js` 内消化。

### 16.3 JS 侧 API

```js
const app = await PyRecT3DWeb.init({
  wasmPath: "pyr3d_single.js",
  width: 1280, height: 720,
  clampInput: true,      // 摇杆钳位
});
const a = app.cube(0, 0, -5);
const c = app.clone(a);
app.move(c, -2, 0, -5);
app.video("assets/demo.mp4", true);
app.start();
```

### 16.4 已知限制

- WebGL2 不支持部分桌面 GL 纹理内部格式（压缩纹理需按浏览器能力协商）。
- 浏览器沙盒不能直接读本地文件，资源走 `fetch` 或打包进虚拟文件系统。
- 单包 `.wasm` 建议 < 4 MB。

---

## 17. Android 分发

- 引擎本体是原生 C++（NDK），Java/Kotlin 只做薄壳。
- 渲染后端固定 `Backend::GLES3`，与 Web 共用同一份渲染实现。
- JNI 胶水层职责：触摸 → 输入事件、生命周期 → `AppPause/AppResume` + `release_gpu_resources()`、Surface → EGL 上下文。
- 目录：`Android/apk/`（`AndroidManifest.xml`、`build.gradle`、`jni/CMakeLists.txt`、`jni/native_bridge.cpp` 均为占位契约）。

> **不支持 Apple 平台**（iOS / macOS）。不引入 Xcode / Objective-C 工具链。

---

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
| `cpp/src/bindings/python/binding.cpp` | 模块入口 `PYBIND11_MODULE(pyrect3d_binding)`，只调用各 `bind_*` |
| `cpp/src/bindings/python/binding_common.h` | 公共 include + 统一命名空间 + 各注册函数声明 |
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

`cpp/src/graphics/gl_backend.h` 是引擎唯一的 GL 接触面，上层只说"画什么"：

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
