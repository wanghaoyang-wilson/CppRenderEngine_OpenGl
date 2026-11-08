# PyRecT3D v0.2 — 公共 API 完整参考

> **本文件的定位**：写给「AI 或人类读者只看这一份文档，就能完整使用/复刻本引擎全部能力」的**唯一权威接口文档**。
> 你不需要通读整个库的源码；把本文档喂给 AI，按本文档调用即可产出可用代码。

- 引擎版本：`0.2.0`
- 语言：C++17（对外契约头文件）+ Python（pybind11 绑定）
- 图形：桌面 OpenGL 3.3 Core ／ Android + Web `OpenGL ES 3.0`（WebGL2）
- 平台：Windows（最低 Win7）、Linux（X11）、Android（NDK）、Web（WASM）
- 许可证：**GPL-3.0**

---

## 0. 阅读路线（给 AI 的最短路径）

1. 先看 **第 1 章「产品由两部分组成」**，判断该用 *显式 API* 还是 *隐式 API*。
2. 再看 **第 2 章「公共 API 总索引」**，找你要用的函数名。
3. 用 **第 3 章以后** 的逐条签名拄代码。
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
| 出现第二个窗口 | 隐式 API 不支持，必须切换显式 API |

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

> 这是**默认输入接管集合**，不追求全键位覆盖。需要更多键位时走消息队列自定义事件（§6/§7）。

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
EntityHandle clone(EntityHandle src);                                  // 复制场上状态
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
