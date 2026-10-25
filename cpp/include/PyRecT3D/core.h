// ============================================================
//  PyRecT3D/core.h — 引擎核心 / 消息循环 / 输入 / 实体（契约层，仅声明）
// ============================================================
#pragma once
#include "export.h"
#include "math.h"
#include <cstdint>
#include <functional>
#include <string>

namespace pyrect3d {

// ---------- 输入按键 ----------
enum class PYRECT3D_API Key : int {
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

// 输入状态
struct PYRECT3D_API InputState {
    bool keys[static_cast<int>(Key::Count)] = {};
    int  mouseX = 0, mouseY = 0;
    int  mouseDeltaX = 0, mouseDeltaY = 0;
    bool mouseLocked = false;

    bool isDown(Key k) const;
};

// ---------- 消息循环回调 ----------
// 外部接管时可注入：每帧渲染 + 输入
using FrameCallback = std::function<void(InputState&, double dt)>;

// ---------- 引擎上下文 ----------
class PYRECT3D_API Engine {
public:
    Engine();
    ~Engine();

    // 创建窗口与 OpenGL 上下文；win 为 native 窗口句柄（外部接管时可传 0）
    bool init(int width, int height, const std::string& title,
              void* nativeWindow = nullptr);
    void shutdown();

    // 模式1：引擎自带消息循环，阻塞运行
    int  run();

    // 模式2：外部接管 —— 单步处理一次消息/输入，返回 false 表示请求关闭
    bool pump();
    // 外部请求关闭消息循环
    void requestClose();

    // 注入外部帧回调（接管渲染/输入用）
    void setFrameCallback(FrameCallback cb);

    const InputState& input() const;
    void lockMouse(bool locked);

    // 平台粘合内部句柄（不对外暴露细节）
    void* nativeHandle() const;

private:
    struct Impl;
    Impl* impl_;
};

// ---------- 场景 / 实体 ----------
class PYRECT3D_API Entity {
public:
    explicit Entity(std::int64_t id);
    virtual ~Entity();

    std::int64_t id() const { return id_; }
    void setPosition(const Vec3& p);
    Vec3 position() const;
    void setScale(const Vec3& s);
    Vec3 scale() const;
    void setRotation(const Vec3& eulerDeg);
    Vec3 rotation() const;

    // AABB（用于射线/碰撞加速）
    AABB bounds() const;

private:
    std::int64_t id_;
    struct Impl;
    Impl* impl_;
};

// 简单场景容器
class PYRECT3D_API Scene {
public:
    Scene();
    ~Scene();

    std::int64_t addEntity(Entity* e);
    void removeEntity(std::int64_t id);
    Entity* getEntity(std::int64_t id);

    // 射线检测：返回命中的实体 id 与距离
    RaycastHit raycast(const Ray& ray);

private:
    struct Impl;
    Impl* impl_;
};

} // namespace pyrect3d
