// ============================================================
//  PyRecT3D — engine.cpp
//  引擎上下文实现：窗口、消息循环调度、输入状态、外部接管
// ============================================================
#include <PyRecT3D/core.h>
#include "core/platform/platform_private.h"
#include <cstring>
#include <unordered_map>

namespace pyrect3d {

// ---------- InputState ----------
bool InputState::isDown(Key k) const {
    if (k == Key::Unknown || k == Key::Count || k == Key::AltF4) return false;
    return keys[static_cast<int>(k)];
}

// ---------- Engine ----------
struct Engine::Impl {
    PlatformImpl* platform = nullptr;
    InputState state;
    FrameCallback frameCb;
    bool running = false;
    bool closeRequested = false;
};

Engine::Engine() : impl_(new Impl()) {}
Engine::~Engine() { shutdown(); delete impl_; }

bool Engine::init(int width, int height, const std::string& title, void* nativeWindow) {
    if (impl_->platform) return true;
    impl_->platform = createPlatform();
    if (!impl_->platform) return false;
    if (!impl_->platform->createWindow(width, height, title, nativeWindow)) {
        delete impl_->platform; impl_->platform = nullptr;
        return false;
    }
    impl_->running = true;
    return true;
}

void Engine::shutdown() {
    if (impl_->platform) { impl_->platform->destroyWindow(); delete impl_->platform; impl_->platform = nullptr; }
    impl_->running = false;
}

int Engine::run() {
    if (!impl_->platform) return -1;
    impl_->running = true;
    while (impl_->running && !impl_->closeRequested) {
        pump();
        if (impl_->platform && impl_->platform->nativeHandle()) {
            impl_->platform->swapBuffers();
        }
        // 简单帧率控制（演示）
        // 真实实现会做垂直同步 / 时间步长
    }
    return 0;
}

bool Engine::pump() {
    if (!impl_->platform) return false;
    // 刷新输入
    impl_->platform->pump(impl_->state);

    // 关闭请求（外部 requestClose 或平台关闭消息）
    if (impl_->closeRequested) return false;

    // 调用外部接管帧回调
    if (impl_->frameCb) {
        double dt = 1.0 / 60.0; // 演示固定步长
        impl_->frameCb(impl_->state, dt);
    }
    return true;
}

void Engine::requestClose() { impl_->closeRequested = true; }

void Engine::setFrameCallback(FrameCallback cb) { impl_->frameCb = std::move(cb); }

const InputState& Engine::input() const { return impl_->state; }

void Engine::lockMouse(bool locked) {
    if (impl_->platform) impl_->platform->lockMouse(locked);
}

void* Engine::nativeHandle() const {
    return impl_->platform ? impl_->platform->nativeHandle() : nullptr;
}

// ---------- Entity ----------
struct Entity::Impl {
    Vec3 position, scale{1,1,1}, rotation;
};

Entity::Entity(std::int64_t id) : id_(id), impl_(new Impl()) {}
Entity::~Entity() { delete impl_; }
void Entity::setPosition(const Vec3& p) { impl_->position = p; }
Vec3 Entity::position() const { return impl_->position; }
void Entity::setScale(const Vec3& s) { impl_->scale = s; }
Vec3 Entity::scale() const { return impl_->scale; }
void Entity::setRotation(const Vec3& eulerDeg) { impl_->rotation = eulerDeg; }
Vec3 Entity::rotation() const { return impl_->rotation; }

AABB Entity::bounds() const {
    Vec3 half = Vec3(impl_->scale.x * 0.5f, impl_->scale.y * 0.5f, impl_->scale.z * 0.5f);
    return AABB(impl_->position - half, impl_->position + half);
}

// ---------- Scene ----------
struct Scene::Impl {
    struct Item { Entity* ptr; AABB box; };
    std::unordered_map<std::int64_t, Item> entities;
    std::int64_t nextId = 1;
};

Scene::Scene() : impl_(new Impl()) {}
Scene::~Scene() { delete impl_; }

std::int64_t Scene::addEntity(Entity* e) {
    if (!e) return -1;
    std::int64_t id = impl_->nextId++;
    impl_->entities[id] = { e, e->bounds() };
    return id;
}

void Scene::removeEntity(std::int64_t id) { impl_->entities.erase(id); }

Entity* Scene::getEntity(std::int64_t id) {
    auto it = impl_->entities.find(id);
    return it == impl_->entities.end() ? nullptr : it->second.ptr;
}

// 克隆：拷贝场上现有实体当前状态（位置 / 旋转 / 缩放）
std::int64_t Scene::cloneEntity(std::int64_t id) {
    Entity* src = getEntity(id);
    if (!src) return -1;
    Entity* dst = new Entity(0);
    dst->setPosition(src->position());
    dst->setScale(src->scale());
    dst->setRotation(src->rotation());
    return addEntity(dst);
}

// 召唤：以模板实体的缩放入参、给定位置新建实例
std::int64_t Scene::summonEntity(std::int64_t prototypeId, const Vec3& position) {
    Entity* proto = getEntity(prototypeId);
    Vec3 scaleRef = proto ? proto->scale() : Vec3(1, 1, 1);
    Entity* dst = new Entity(0);
    dst->setPosition(position);
    dst->setScale(scaleRef);
    return addEntity(dst);
}

std::size_t Scene::entityCount() const { return impl_->entities.size(); }

void Scene::forEach(const std::function<void(std::int64_t, Entity&)>& fn) const {
    for (auto& kv : impl_->entities) fn(kv.first, *kv.second.ptr);
}

RaycastHit Scene::raycast(const Ray& ray) {
    RaycastHit best;
    float bestT = 1e30f;
    for (auto& kv : impl_->entities) {
        float tNear, tFar;
        if (!kv.second.box.intersect(ray, tNear, tFar)) continue;
        float t = (tNear > 0.f) ? tNear : tFar;   // 起点在盒内时 tNear<0，取 tFar
        if (t < bestT) {
            bestT = t;
            best.hit = true;
            best.distance = t;
            best.point = ray.origin + ray.direction * t;
            best.normal = Vec3(0, 1, 0); // 演示法线
            best.entityId = kv.first;
        }
    }
    return best;
}

} // namespace pyrect3d
