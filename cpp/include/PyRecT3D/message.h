// ============================================================
//  PyRecT3D/message.h — 主消息队列 / 子消息队列 / 事件优先级（契约层）
//
//  架构：主消息队列（全局唯一，绑定渲染主线程）
//        子消息队列（每创建一个窗口实例自动分配一个）
//        事件优先级由高到低：窗口输入 > 物理 > 碰撞 > 音效 > 媒体 > 普通
//  线程模型：OpenGL 上下文只能在持有它的主线程调用，因此所有渲染
//           指令与 GPU 资源操作都通过消息队列投递到主线程执行。
// ============================================================
#pragma once
#include "export.h"
#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>

namespace pyrect3d {

// ---------- 事件优先级 ----------
// 数值越小优先级越高；调度器按 (priority, seq) 排序出队
enum class PYRECT3D_API EventPriority : std::int32_t {
    WindowInput = 0,   // 窗口级：关闭/最小化/最大化/边框变更/WASD/鼠标键/鼠标位置
    Physics     = 1,   // 物理事件
    Collision   = 2,   // 碰撞事件
    Audio       = 3,   // 音效事件
    Media       = 4,   // 播放器 / 视频帧事件
    Normal      = 5,   // 普通业务消息
};

// ---------- 事件类型 ----------
enum class PYRECT3D_API EventType : int {
    None = 0,

    // 窗口（最高优先级）
    WindowClose,
    WindowMinimize,
    WindowMaximize,
    WindowRestore,
    WindowResize,
    WindowBorderChange,
    WindowFocusGained,
    WindowFocusLost,

    // 输入（最高优先级）
    KeyDown,
    KeyUp,
    MouseMove,
    MouseButtonDown,
    MouseButtonUp,
    MouseWheel,

    // 全局 / 程序级（由主消息队列发布）
    AppQuit,
    AppPause,
    AppResume,
    AppResumeFromBackground,
    InputBindingChanged,      // 切换鼠标模式 / 绑定 WASD 等
    ViewportTransfer,         // 例：据点转交至 B 视口
    SceneSwitch,
    WindowCreate,
    WindowDestroy,

    // 场景（以主消息队列身份发布）
    SceneCreate,
    SceneDestroy,
    TransformMove,
    TransformRotate,
    TransformScale,
    EntityClone,              // 克隆（复制现有实体状态），区别于 EntitySummon
    EntitySummon,             // 召唤（从预制体生成）
    EntityRemove,
    PhysicsTick,

    // 碰撞（碰撞箱事件）
    CollisionEnter,
    CollisionExit,
    CollisionStay,
    CollisionPress,           // 在碰撞区点击
    CollisionRelease,         // 在碰撞区松开
    CollisionDragOut,         // 按住并在碰撞区内移动后拖出区域
    CollisionDragMove,        // 按住拖动中移动
    CollisionClickOutside,    // 拖出后在区域外松开
    RaycastHitReport,         // 射线命中结果入队

    // 音频 / 媒体
    AudioFinished,
    VideoFinished,
    VideoPaused,
    VideoError,
    VideoFrameReady,

    RenderCommandSubmit,      // 渲染指令合并提交
    RenderRegionInvalidate,   // 2D/3D 分离局部更新（脏区域）

    Count
};

// ---------- 消息载荷 ----------
// 为避免跨编译器 ABI 差异，不使用 std::any / 变体模板，统一用定长结构
struct PYRECT3D_API MessagePayload {
    std::uint64_t entityId = 0;      // 实体 id（场景 / 碰撞 / 射线）
    std::int64_t  windowId  = 0;     // 窗口实例 id
    std::int64_t  playerId  = 0;     // 播放器实例 id
    std::int64_t  queueId   = 0;     // 来源子队列 id
    std::int32_t  code      = 0;     // 键码 / 按钮码 / 错误码
    float         v[8]      = {};    // 通用数值：坐标、变换、拖拽量等
    std::string   text;              // 文本类载荷
    void*         user      = nullptr;
};

// ---------- 消息 ----------
struct PYRECT3D_API Message {
    EventType     type     = EventType::None;
    EventPriority priority = EventPriority::Normal;
    std::int64_t  sourceQueue = 0;   // 发布者队列 id（0 = 主队列）
    std::int64_t  targetQueue = -1;  // -1 = 广播；否则只投递给该子队列
    std::uint64_t seq      = 0;      // 由队列自动填充，用于同优先级 FIFO
    MessagePayload payload;
};

using EventHandler = std::function<void(const Message&)>;

// ============================================================
//  子消息队列 —— 每个窗口实例自动分配一个
// ============================================================
class PYRECT3D_API MessageQueue {
public:
    explicit MessageQueue(std::int64_t id = 0);
    ~MessageQueue();

    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;

    std::int64_t id() const;

    // 多窗口情况下必须显式把子队列挂载到主队列；单窗口由引擎自动弱绑定
    bool attachToMain();
    bool detachFromMain();
    bool attached() const;

    // 发布事件：进入主消息队列，由主队列按 targetQueue 路由回来
    void publish(const Message& m);

    // 仅在本子队列内部投递（不经过主队列）
    void publishLocal(const Message& m);

    // 取出一个本地已路由到本队列的消息
    bool poll(Message& out);
    std::size_t pending() const;

    // 设置本队列的事件处理器（窗口级业务）
    void setHandler(EventHandler handler);
    void dispatch();

private:
    struct Impl;
    Impl* impl_;
    friend class MainMessageQueue;
};

// ============================================================
//  主消息队列 —— 全局唯一单例，绑定渲染主线程
// ============================================================
class PYRECT3D_API MainMessageQueue {
public:
    static MainMessageQueue& instance();

    // 发布全局事件（窗口/输入/程序级/全局场景操作）
    std::uint64_t publish(const Message& m);

    // 按优先级调度：一次性把当前积压消息派发完毕
    void pump();

    bool poll(Message& out);
    std::size_t pending() const;

    // 全局订阅（程序级事件、渲染指令执行点等）
    void subscribe(EventHandler handler);

    // 当前在线的子队列数量（多窗口检测）
    std::size_t childCount() const;
    MessageQueue* findChild(std::int64_t id);
    MessageQueue* firstChild();   // 单窗口弱绑定用

    // 挂载 / 卸载子队列（多窗口场景由引擎或调用者显式挂载）
    bool attachChild(MessageQueue* child);
    bool detachChild(MessageQueue* child);

    // 主队列绑定的线程 id（OpenGL 上下文所在线程）
    std::uint64_t threadId() const;

private:
    MainMessageQueue();
    ~MainMessageQueue();
    struct Impl;
    Impl* impl_;
};

// ---------- APP::run 启动契约 ----------
// 由显式/隐式 API 调用，统一拉起 3D 场景 + 消息队列
class PYRECT3D_API App {
public:
    // 启动消息循环与场景，阻塞直到关闭。返回进程退出码。
    static int run();

    // 请求退出（等价于发布 EventType::AppQuit）
    static void quit();

    // 单帧推进（外部接管模式使用）
    static bool pump();
};

} // namespace pyrect3d
