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
