// ============================================================
//  PyRecT3D — src/physics/collision_world.cpp
//  双碰撞箱（物理箱 / 模型箱）+ 触发区 + 碰撞事件 → 消息队列
//
//  物理箱：统一用射线（AABB slab 求交）做碰撞计算
//  模型箱：内部小型物理空间，肢体级碰撞只在该子空间内运算
// ============================================================
#include <PyRecT3D/collision.h>
#include "core/app_registry.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace pyrect3d {

namespace {

// 把 CollisionEventKind 映射到 EventType
EventType toEventType(CollisionEventKind k) {
    switch (k) {
        case CollisionEventKind::Enter:        return EventType::CollisionEnter;
        case CollisionEventKind::Exit:         return EventType::CollisionExit;
        case CollisionEventKind::Stay:         return EventType::CollisionStay;
        case CollisionEventKind::Press:        return EventType::CollisionPress;
        case CollisionEventKind::Release:      return EventType::CollisionRelease;
        case CollisionEventKind::DragMove:     return EventType::CollisionDragMove;
        case CollisionEventKind::DragOut:      return EventType::CollisionDragOut;
        case CollisionEventKind::ClickOutside: return EventType::CollisionClickOutside;
    }
    return EventType::CollisionStay;
}

} // namespace

struct CollisionWorld::Impl {
    std::int64_t nextHandle = 1;

    struct PhysEntry { PhysicsCollider c; std::int64_t entityId = 0; };
    struct ModelEntry { ModelCollider c; std::int64_t entityId = 0; };
    struct ZoneEntry { AABB box; std::int64_t owner = 0; };

    std::unordered_map<std::int64_t, PhysEntry> physics;
    std::unordered_map<std::int64_t, ModelEntry> models;
    std::unordered_map<std::int64_t, ZoneEntry> zones;

    std::unordered_map<int, CollisionCallback> handlers;

    // 指针状态机
    float px = 0, py = 0;
    bool  prevPressed = false;
    std::unordered_map<std::int64_t, bool> wasInside;     // 上一帧是否在区域内
    std::unordered_map<std::int64_t, bool> pressedInside; // 按下时是否在区域内
    std::unordered_map<std::int64_t, bool> draggedOut;    // 已拖出

    void fire(CollisionEventKind kind, std::int64_t handle,
              const MessagePayload& payload) {
        Message m;
        m.type = toEventType(kind);
        m.priority = EventPriority::Collision;
        m.payload = payload;
        m.payload.code = static_cast<std::int32_t>(kind);
        MainMessageQueue::instance().publish(m);

        auto it = handlers.find(static_cast<int>(kind));
        if (it != handlers.end() && it->second) it->second(m);
        (void)handle;
    }
};

CollisionWorld::CollisionWorld() : impl_(new Impl()) {}
CollisionWorld::~CollisionWorld() { delete impl_; }

std::int64_t CollisionWorld::addPhysicsBox(const PhysicsCollider& c) {
    std::int64_t h = impl_->nextHandle++;
    impl_->physics[h] = { c, 0 };
    return h;
}

std::int64_t CollisionWorld::addModelBox(const ModelCollider& c) {
    std::int64_t h = impl_->nextHandle++;
    impl_->models[h] = { c, 0 };
    return h;
}

void CollisionWorld::updatePhysicsBox(std::int64_t handle, const PhysicsCollider& c) {
    auto it = impl_->physics.find(handle);
    if (it != impl_->physics.end()) it->second.c = c;
}

void CollisionWorld::updateModelBox(std::int64_t handle, const ModelCollider& c) {
    auto it = impl_->models.find(handle);
    if (it != impl_->models.end()) it->second.c = c;
}

void CollisionWorld::remove(std::int64_t handle) {
    impl_->physics.erase(handle);
    impl_->models.erase(handle);
    impl_->zones.erase(handle);
}

void CollisionWorld::clear() {
    impl_->physics.clear();
    impl_->models.clear();
    impl_->zones.clear();
    impl_->wasInside.clear();
    impl_->pressedInside.clear();
    impl_->draggedOut.clear();
}

std::int64_t CollisionWorld::addTriggerZone(const AABB& box, std::int64_t ownerEntityId) {
    std::int64_t h = impl_->nextHandle++;
    impl_->zones[h] = { box, ownerEntityId };
    return h;
}

void CollisionWorld::setEventHandler(CollisionEventKind kind, CollisionCallback cb) {
    impl_->handlers[static_cast<int>(kind)] = std::move(cb);
}

void CollisionWorld::clearEventHandlers() { impl_->handlers.clear(); }

void CollisionWorld::step(float dt) {
    (void)dt;
    // 物理推进：对每个物理箱做一次自检式射线（向下），命中报告入队
    for (auto& kv : impl_->physics) {
        if (!kv.second.c.enabled || !kv.second.c.useRaycast) continue;
        const AABB& b = kv.second.c.box;
        Vec3 center((b.min.x + b.max.x) * 0.5f, b.max.y, (b.min.z + b.max.z) * 0.5f);
        Ray r(center, Vec3(0, -1, 0));
        float tNear = 0, tFar = 0;
        if (b.intersect(r, tNear, tFar)) {
            Message m;
            m.type = EventType::PhysicsTick;
            m.priority = EventPriority::Physics;
            m.payload.entityId = static_cast<std::uint64_t>(kv.first);
            m.payload.v[0] = tNear;
            m.payload.v[1] = tFar;
            MainMessageQueue::instance().publish(m);
        }
    }
}

HitResult CollisionWorld::raycast(const Ray& ray, float maxDistance,
                                  bool includeModel, bool includePhysics) const {
    HitResult best;
    float bestT = maxDistance;

    // 1) 模型碰撞箱（精细优先）：整体粗盒 + 肢体盒
    if (includeModel) {
        for (auto& kv : impl_->models) {
            if (!kv.second.c.enabled) continue;
            float tN = 0, tF = 0;
            float tHit = 0.f;
            if (kv.second.c.root.intersect(ray, tN, tF)) tHit = (tN > 0.f) ? tN : tF;
            if (tHit > 0.f && tHit < bestT) {
                bestT = tHit;
                best.hit = true;
                best.hitModel = true;
                best.distance = tHit;
                best.point = ray.origin + ray.direction * tHit;
                best.normal = Vec3(0, 1, 0);
                best.entityId = kv.second.entityId;
                best.limbName.clear();
            }
            for (const Limb& limb : kv.second.c.limbs) {
                if (!limb.enabled) continue;
                AABB local = limb.localBox;
                Vec3 off = limb.offset * kv.second.c.localScale;
                local.min = local.min + off;
                local.max = local.max + off;
                float lt = 0, lf = 0;
                float ltHit = 0.f;
                if (local.intersect(ray, lt, lf)) ltHit = (lt > 0.f) ? lt : lf;
                if (ltHit > 0.f && ltHit < bestT) {
                    bestT = ltHit;
                    best.hit = true;
                    best.hitModel = true;
                    best.distance = ltHit;
                    best.point = ray.origin + ray.direction * ltHit;
                    best.entityId = kv.second.entityId;
                    best.limbName = limb.name;
                }
            }
        }
    }

    // 2) 物理碰撞箱（粗判定）
    if (includePhysics) {
        for (auto& kv : impl_->physics) {
            if (!kv.second.c.enabled) continue;
            float tN = 0, tF = 0;
            float tHit = 0.f;
            if (kv.second.c.box.intersect(ray, tN, tF)) tHit = (tN > 0.f) ? tN : tF;
            if (tHit > 0.f && tHit < bestT) {
                bestT = tHit;
                best.hit = true;
                best.hitPhysics = true;
                best.distance = tHit;
                best.point = ray.origin + ray.direction * tHit;
                best.normal = Vec3(0, 1, 0);
                best.entityId = kv.second.entityId;
            }
        }
    }

    return best;
}

// ============================================================
//  指针 / 区域事件状态机
//  Enter / Exit / Stay / Press / Release / DragMove / DragOut / ClickOutside
// ============================================================
void CollisionWorld::processPointer(float x, float y, bool pressed, bool released, float dt) {
    (void)dt;
    for (auto& kv : impl_->zones) {
        std::int64_t h = kv.first;
        const AABB& box = kv.second.box;
        bool inside = (x >= box.min.x && x <= box.max.x && y >= box.min.y && y <= box.max.y);

        bool was = impl_->wasInside.count(h) ? impl_->wasInside[h] : false;

        MessagePayload base;
        base.entityId = static_cast<std::uint64_t>(kv.second.owner);
        base.v[0] = x;
        base.v[1] = y;
        base.v[2] = box.min.x;
        base.v[3] = box.min.y;
        base.v[4] = box.max.x;
        base.v[5] = box.max.y;

        if (inside && !was) {
            impl_->fire(CollisionEventKind::Enter, h, base);
        } else if (!inside && was) {
            impl_->fire(CollisionEventKind::Exit, h, base);
        } else if (inside) {
            impl_->fire(CollisionEventKind::Stay, h, base);
        }

        // 按下
        if (pressed && inside) {
            impl_->pressedInside[h] = true;
            impl_->draggedOut[h] = false;
            impl_->fire(CollisionEventKind::Press, h, base);
        }

        // 按住移动
        if (pressed && !released && inside) {
            MessagePayload mv = base;
            mv.v[6] = x - impl_->px;
            mv.v[7] = y - impl_->py;
            impl_->fire(CollisionEventKind::DragMove, h, mv);
        }

        // 按住拖出
        bool dragging = impl_->pressedInside.count(h) ? impl_->pressedInside[h] : false;
        if (dragging && !inside) {
            if (!impl_->draggedOut[h]) {
                impl_->draggedOut[h] = true;
                impl_->fire(CollisionEventKind::DragOut, h, base);
            }
        }

        // 松开
        if (released) {
            if (dragging) {
                if (inside) {
                    impl_->fire(CollisionEventKind::Release, h, base);
                } else if (impl_->draggedOut[h]) {
                    impl_->fire(CollisionEventKind::ClickOutside, h, base);
                }
                impl_->pressedInside[h] = false;
            }
        }

        impl_->wasInside[h] = inside;
    }

    impl_->px = x;
    impl_->py = y;
    impl_->prevPressed = pressed;
}

} // namespace pyrect3d
