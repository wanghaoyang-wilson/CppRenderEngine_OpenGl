// ============================================================
//  PyRecT3D/collision.h — 双碰撞箱 / 碰撞事件 / 射线检测（契约层）
//
//  每个可交互对象（典型是玩家）拥有两套碰撞箱：
//  1. PhysicsCollider  —— 物理碰撞箱，负责玩家与世界物理的碰撞计算，
//                        底层统一走射线（ray）判定。
//  2. ModelCollider    —— 人物模型碰撞箱，负责模型展示与精细碰撞；
//                        内部挂一个「小型物理空间」，所有肢体碰撞都在
//                        该子空间内完成，不与大世界物理混算。
// ============================================================
#pragma once
#include "export.h"
#include "math.h"
#include "message.h"
#include <cstdint>
#include <functional>
#include <vector>

namespace pyrect3d {

// ---------- 碰撞事件类型 ----------
// 与 message.h 中的 EventType 一一对应，直接入消息队列
enum class PYRECT3D_API CollisionEventKind : int {
    Enter = 0,        // 进入碰撞区
    Exit,             // 离开碰撞区
    Stay,             // 停留在碰撞区内（每帧）
    Press,            // 在碰撞区内按下
    Release,          // 在碰撞区内松开
    DragMove,         // 按住并在区域内移动
    DragOut,          // 按住移出区域（拖出）
    ClickOutside,     // 拖出后在区域外松开
};

// ---------- 肢体（精细碰撞） ----------
struct PYRECT3D_API Limb {
    std::string name;
    AABB  localBox;          // 相对模型原点的局部 AABB
    Vec3  offset;            // 相对父肢体位移
    bool  enabled = true;
};

// ---------- 物理碰撞箱 ----------
struct PYRECT3D_API PhysicsCollider {
    AABB  box;               // 世界空间 AABB（每帧由变换刷新）
    bool  enabled = true;
    bool  useRaycast = true; // 统一使用射线计算（默认开启）
    float rayStep = 0.08f;   // 射线采样步长
    float maxDistance = 8.f; // 射线最大检测距离
};

// ---------- 模型碰撞箱 ----------
struct PYRECT3D_API ModelCollider {
    AABB root;               // 模型整体粗包围盒
    std::vector<Limb> limbs; // 肢体列表
    bool  enabled = true;
    // 内部小型物理空间（肢体级碰撞只在该空间内运算）
    bool  useLocalSpace = true;
    float localScale = 1.f;
};

// ---------- 命中结果（扩展） ----------
struct PYRECT3D_API HitResult {
    bool  hit = false;
    bool  hitModel = false;        // 命中模型碰撞箱（精细）
    bool  hitPhysics = false;      // 命中物理碰撞箱
    float distance = 0.f;
    Vec3  point;
    Vec3  normal;
    std::int64_t entityId = -1;
    std::string limbName;         // 命中的肢体名（模型碰撞箱）
};

// ---------- 碰撞回调 ----------
using CollisionCallback = std::function<void(const Message&)>;

// ---------- 碰撞世界 ----------
class PYRECT3D_API CollisionWorld {
public:
    CollisionWorld();
    ~CollisionWorld();

    // 注册 / 注销
    std::int64_t addPhysicsBox(const PhysicsCollider& c);
    std::int64_t addModelBox(const ModelCollider& c);
    void updatePhysicsBox(std::int64_t handle, const PhysicsCollider& c);
    void updateModelBox(std::int64_t handle, const ModelCollider& c);
    void remove(std::int64_t handle);
    void clear();

    // 物理推进：内部统一按射线对物理碰撞箱做检测，命中结果发布 PhysicsTick
    void step(float dt);

    // 射线检测：同时检测物理箱与模型箱，精细优先
    HitResult raycast(const Ray& ray, float maxDistance = 100.f,
                      bool includeModel = true, bool includePhysics = true) const;

    // 区域查询（用于 UI / 触发区）
    std::int64_t addTriggerZone(const AABB& box, std::int64_t ownerEntityId = 0);

    // 事件：全部通过主消息队列发布
    void setEventHandler(CollisionEventKind kind, CollisionCallback cb);
    void clearEventHandlers();

    // 每帧对指针位置处理 Enter/Exit/Press/Release/DragOut 等事件
    void processPointer(float x, float y, bool pressed, bool released, float dt);

private:
    struct Impl;
    Impl* impl_;
};

} // namespace pyrect3d
