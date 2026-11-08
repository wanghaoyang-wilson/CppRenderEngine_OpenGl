# API Reference

> **权威全文**：仓库 [`docs/API.md`](https://github.com/wanghaoyang-wilson/CppRenderEngine_OpenGl/blob/pyr3d-v2-upgrade/docs/API.md)（同步维护 `docs/API.docx`）
> 本页是 Wiki 版**总索引 + 关键签名速查**。发布到真实 Wiki 时，`publish_wiki.sh` 会用 `docs/API.md` 全文（经 `docs/make_api.py` 拼接 `API.md` + `API-part2.md`）覆盖本页，保证 Wiki 上是完整文档。

返回 [[Home]] · 双 API 选择见 [[Explicit vs Implicit]] · 事件见 [[Event Model]] · 坑见 [[Hard Constraints]]

---

## 1. 产品由两部分组成

| 部分 | 头文件 | 适用 |
|------|--------|------|
| **显式 API（Explicit API）** | `<PyRecT3D/explicit.h>` | 多窗口实例、需要详细配置（渲染后端 / 模块裁剪 / 队列挂载 / UI 与播放器绑定） |
| **隐式 API（Implicit API）** | `<PyRecT3D/implicit.h>` | 最快入门：单窗口自动弱绑定，一行启动 |

---

## 2. 头文件 → 模块

| 头文件 | 主要内容 |
|--------|----------|
| `export.h` | `PYRECT3D_API`、版本宏、`PYRECT3D_WITH_*` 模块开关 |
| `math.h` | `Vec2/3/4`、`Mat4`、`Ray`、`RaycastHit`、`AABB` |
| `core.h` | `Key`、`InputState`、`Entity`、`Scene`、`Engine` |
| `message.h` | `EventPriority`、`EventType`、`MessagePayload`、`Message`、`MessageQueue`、`MainMessageQueue`、`App` |
| `explicit.h` | `Backend`、`Module`、`AppConfig`、`WindowDesc`、`WindowInstance`、`UIInstance`、`ExplicitApp` |
| `implicit.h` | `EntityHandle`、`quick::*` |
| `renderer.h` | `RenderMode`、`Renderer`、`RenderCommand`、`DirtyRegion`、`Compositor`、`Cube`、`ScreenCube`、`SoftRayTracer` |
| `gui.h` | `Rect`、`GuiEvent`、`Widget`、`Button`、`Text`、`InputBox`、`Container`、`ScrollContainer`、`Gui` |
| `collision.h` | `CollisionEventKind`、`Limb`、`PhysicsCollider`、`ModelCollider`、`HitResult`、`CollisionWorld` |
| `video.h` | `VideoTexture`、`VideoDesc`、`VideoPlayer`、`MediaManager` |
| `audio.h` / `io.h` | `Audio` / `MeshData`、`Material`、`ObjModel`、`ObjLoader` |

---

## 3. 启动与窗口

```cpp
// 隐式（单窗口）
int quick::run(int w = 1280, int h = 720, const std::string& title = "PyRecT3D");
int quick::runSetup(int w, int h, const std::string& title, void (*setup)());

// 显式（多窗口 / 详细配置）
bool ExplicitApp::configure(const AppConfig& cfg);
WindowInstance* ExplicitApp::createWindow(const WindowDesc& desc);
bool ExplicitApp::mountQueues();                     // 多窗口必须
VideoPlayer* ExplicitApp::createVideoPlayer(const VideoDesc&, WindowInstance* bindTo = nullptr);
std::uint64_t ExplicitApp::publishGlobal(const Message& m);
int ExplicitApp::run();  bool ExplicitApp::pump();  void ExplicitApp::quit();
bool ExplicitApp::hasModule(Module m) const;

// 窗口
std::int64_t WindowInstance::id() const;
MessageQueue* WindowInstance::queue();
Entity* WindowInstance::camera();
Scene& WindowInstance::scene();
UIInstance* WindowInstance::createUI();
void WindowInstance::renderScene();   // 提交 3D 帧，保留帧缓冲
void WindowInstance::present();       // 3D 帧缓冲 + 新 UI 合成上屏
```

```cpp
struct AppConfig {
    int width = 1280, height = 720;
    std::string title = "PyRecT3D";
    Backend backend = Backend::Auto;     // Auto / GL33 / GLES3
    bool vsync = true;
    bool enableVideo = true, enableAudio = true, enablePhysics = true;
    bool enableGUILocalUpdate = true;    // 2D/3D 分离局部更新
    bool multithreadRender = false;      // 子线程生产渲染指令
    float renderScale = 1.0f;
};

struct WindowDesc {
    int width = 1280, height = 720;
    std::string title = "Window";
    bool resizable = true, fullscreen = false, uiAutoBind = false;
    void* nativeHandle = nullptr;        // 外部接管
};

enum class Module : int {
    Renderer = 1<<0, GUI = 1<<1, Video = 1<<2,
    Audio = 1<<3, Physics = 1<<4, RayTracer = 1<<5, Python = 1<<6
};
```

---

## 4. 场景与实体

```cpp
std::int64_t Scene::addEntity(Entity* e);
void         Scene::removeEntity(std::int64_t id);
Entity*      Scene::getEntity(std::int64_t id);
std::int64_t Scene::cloneEntity(std::int64_t id);                        // 克隆场上现成状态
std::int64_t Scene::summonEntity(std::int64_t prototypeId, const Vec3&);// 按模板新建
RaycastHit   Scene::raycast(const Ray& ray);
std::size_t  Scene::entityCount() const;
void         Scene::forEach(const std::function<void(std::int64_t, Entity&)>&) const;

void Entity::setPosition(const Vec3&);  Vec3 Entity::position() const;
void Entity::setScale(const Vec3&);     Vec3 Entity::scale() const;
void Entity::setRotation(const Vec3&);  Vec3 Entity::rotation() const;   // 欧拉角（度）
AABB Entity::bounds() const;
```

---

## 5. 消息队列

```cpp
enum class EventPriority : std::int32_t {
    WindowInput = 0, Physics = 1, Collision = 2, Audio = 3, Media = 4, Normal = 5
};   // 越小越优先；同级 FIFO（seq 递增）

struct Message {
    EventType type; EventPriority priority;
    std::int64_t sourceQueue;   // 0 = 主消息队列身份
    std::int64_t targetQueue;   // -1 = 广播；>=0 = 只投递该子队列
    std::uint64_t seq;          // 自动填充
    MessagePayload payload;
};

struct MessagePayload {
    std::uint64_t entityId; std::int64_t windowId, playerId, queueId;
    std::int32_t code;  float v[8];  std::string text;  void* user;
};

// 子队列
std::int64_t MessageQueue::id() const;
bool MessageQueue::attachToMain();  bool MessageQueue::detachFromMain();
void MessageQueue::publish(const Message&);      // 经主队列
void MessageQueue::publishLocal(const Message&);
bool MessageQueue::poll(Message& out);
void MessageQueue::setHandler(EventHandler);  void MessageQueue::dispatch();

// 主队列（单例）
MainMessageQueue& MainMessageQueue::instance();
std::uint64_t MainMessageQueue::publish(const Message&);
void MainMessageQueue::pump();   void MainMessageQueue::subscribe(EventHandler);
bool MainMessageQueue::attachChild(MessageQueue*);

// 启动契约
int App::run();  void App::quit();  bool App::pump();
```

---

## 6. 渲染

```cpp
void Renderer::beginFrame();   void Renderer::endFrame();      // 帧边界（必须成对）
void Renderer::drawEntity(const Entity& e);                    // 3D
void Renderer::drawScreenRect(float x, float y, float w, float h);  // 2D
void Renderer::setMode(RenderMode);  void Renderer::setClearColor(r,g,b,a);
int  Renderer::width() const;        int  Renderer::height() const;

void Compositor::attach(Renderer*);
void Compositor::submit(const RenderCommand* cmds, std::size_t count);  // 合批
void Compositor::invalidate(const DirtyRegion&);  DirtyRegion Compositor::dirtyRegion() const;
void Compositor::setSceneDirty(bool);  bool Compositor::sceneDirty() const;
void Compositor::applyEffect3D(std::int32_t effectCode, float strength); // 3D 特效（眩晕等）
void Compositor::present();
bool SoftRayTracer::render(const Scene&, int w, int h, std::vector<unsigned char>& outRGB);
```

「Docker 式软更新」——3D 画面不动、只换 UI：

```cpp
compositor.setSceneDirty(false);
compositor.submit(uiCmds, n);   // 只提交 2D 指令
compositor.present();           // 旧 3D 帧缓冲 + 新 UI 叠加
```

---

## 7. GUI

```cpp
struct Rect { float x, y, w, h; Rect(); Rect(float,float,float,float); bool contains(float,float) const; };

struct GuiEvent {   // 9 种状态
    bool hovered, clicked, entered, exited, pressed, released,
         dragMove, draggedOut, clickOutside;
    int mouseX, mouseY;  float dragDx, dragDy;
};

Button(Rect, text); Text(Rect, text); InputBox(Rect, text);
Container(Rect); ScrollContainer(Rect);   Gui::addWidget / removeWidget / dispatch / drawAll();
```

`Button` 回调：`onClick`、`onHover`、`onEnter`、`onExit`、`onPress`、`onRelease`、
`onDragMove(dx,dy)`、`onDragOut`、`onClickOutside`。

---

## 8. 碰撞（双碰撞箱）

```cpp
enum class CollisionEventKind : int {
    Enter, Exit, Stay, Press, Release, DragMove, DragOut, ClickOutside
};

struct PhysicsCollider { AABB box; bool enabled; bool useRaycast; float rayStep, maxDistance; };
struct ModelCollider   { AABB root; std::vector<Limb> limbs; bool enabled, useLocalSpace; float localScale; };
struct Limb            { std::string name; AABB localBox; Vec3 offset; bool enabled; };
struct HitResult       { bool hit, hitModel, hitPhysics; float distance; Vec3 point, normal;
                         std::int64_t entityId; std::string limbName; };

std::int64_t CollisionWorld::addPhysicsBox(const PhysicsCollider&);
std::int64_t CollisionWorld::addModelBox(const ModelCollider&);
HitResult    CollisionWorld::raycast(const Ray&, float maxDistance = 100,
                                     bool includeModel = true, bool includePhysics = true) const;
void CollisionWorld::setEventHandler(CollisionEventKind, CollisionCallback);
void CollisionWorld::processPointer(float x, float y, bool pressed, bool released, float dt);
```

| 碰撞箱 | 职责 | 计算 |
|--------|------|------|
| 物理碰撞箱 | 与物理世界碰撞 | 统一射线（AABB slab） |
| 模型碰撞箱 | 模型展示 + 肢体级精细碰撞 | 内部小型物理空间，逐肢体 |

---

## 9. 视频（FFmpeg 多线程）

```cpp
struct VideoDesc { std::string path; bool loop = false; bool decodeAudio = true;
                   int decodeThreads = 2; float volume = 1.f; bool autoPlay = true; };
struct VideoTexture { std::int64_t id; int width, height; std::uint64_t frameSerial; bool valid; };

MediaManager& MediaManager::instance();
VideoPlayer* MediaManager::createPlayer();
void MediaManager::destroyPlayer(VideoPlayer*);
void MediaManager::tick();                 // 主线程统一上传帧

bool VideoPlayer::bindQueue(MessageQueue*); bool VideoPlayer::autoBind();
bool VideoPlayer::open(const VideoDesc&);  bool play(); bool pause(); bool stop(); bool seek(float);
VideoTexture VideoPlayer::texture() const;
bool VideoPlayer::uploadPendingFrame();    // 必须主线程调用
```

线程：子线程只解码到 CPU 缓冲并置 ready；**主线程**才上传纹理。

---

## 10. 隐式 API（quick::）

```cpp
EntityHandle cube(float x=0, float y=0, float z=-5, float sx=1, float sy=1, float sz=1);
EntityHandle screenCube(float x, float y, float w, float h);
EntityHandle clone(EntityHandle src);
EntityHandle summon(EntityHandle prototype, float x, float y, float z);
void move(EntityHandle, float x, float y, float z);
void rotate(EntityHandle, float rx, float ry, float rz);
void scale(EntityHandle, float sx, float sy, float sz);
void remove(EntityHandle);
HitResult raycast(float ox, float oy, float oz, float dx, float dy, float dz, float maxDistance = 100);
Button* button(float x, float y, float w, float h, const std::string& text);
Text*   text(float x, float y, const std::string& content);
VideoPlayer* video(const std::string& path, bool loop = false);
Scene& scene();  Gui& gui();
```

---

## 11. 优先级与路由速记

| `targetQueue` | 行为 |
|---------------|------|
| `-1` | 广播到全局 |
| `>= 0` 且已挂载 | 只投递该子队列 |
| `>= 0` 但不存在 | 退化为全局 |

| 发布者身份 | 用于 |
|------------|------|
| `sourceQueue = 0`（主队列身份） | 单场景常规操作：变换 / 克隆 / 召唤 / 射线结果 |
| 主队列 `publishGlobal` | 全局大型事件：切换鼠标模式、绑定 WASD、关闭程序、跨视口转交 |

---

*完整签名与示例见仓库 `docs/API.md`。PyRecT3D v0.2 · GPL-3.0*
