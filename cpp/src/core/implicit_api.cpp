// ============================================================
//  PyRecT3D — src/core/implicit_api.cpp
//  隐式 API（Quick API）实现：一行启动 + 自动弱绑定
//
//  弱绑定规则：
//   - 只有一个窗口实例时，窗口自动挂载主队列、UI 自动绑定该窗口、
//     播放器自动绑定该窗口子队列，调用者完全不需要手动处理。
//   - 出现第二个窗口时，隐式 API 拒绝继续（多窗口必须改用显式 API）。
// ============================================================
#include <PyRecT3D/implicit.h>
#include <PyRecT3D/explicit.h>
#include <PyRecT3D/collision.h>
#include "core/app_registry.h"
#include <cstdio>

namespace pyrect3d {
namespace {

struct ImplicitCtx {
    ExplicitApp* app = nullptr;
    WindowInstance* win = nullptr;
    UIInstance* ui = nullptr;
    bool inited = false;
};

ImplicitCtx g_ctx;

void ensureInit(int width, int height, const std::string& title) {
    if (g_ctx.inited) return;

    g_ctx.app = new ExplicitApp();

    AppConfig cfg;
    cfg.width = width;
    cfg.height = height;
    cfg.title = title;
    cfg.backend = Backend::Auto;
    cfg.enableGUILocalUpdate = true;   // 2D/3D 分离局部更新
    g_ctx.app->configure(cfg);

    WindowDesc desc;
    desc.width = width;
    desc.height = height;
    desc.title = title;
    desc.uiAutoBind = true;            // 弱绑定：单窗口自动绑定
    g_ctx.win = g_ctx.app->createWindow(desc);

    // 单窗口弱绑定：UI 自动绑定唯一窗口
    g_ctx.ui = g_ctx.win->createUI();
    g_ctx.ui->bindWindow(g_ctx.win);

    g_ctx.inited = true;
}

// 场景操作统一以「主消息队列身份」发布
void publishScene(EventType type, EntityHandle e, float a = 0, float b = 0, float c = 0) {
    Message m;
    m.type = type;
    m.priority = EventPriority::Normal;
    m.sourceQueue = 0;             // 主消息队列身份
    m.payload.entityId = (std::uint64_t)e.id;
    m.payload.v[0] = a; m.payload.v[1] = b; m.payload.v[2] = c;
    MainMessageQueue::instance().publish(m);
}

} // namespace

namespace quick {

int run(int width, int height, const std::string& title) {
    ensureInit(width, height, title);
    if (!g_ctx.app) return -1;
    return g_ctx.app->run();
}

int runSetup(int width, int height, const std::string& title, void (*setup)()) {
    ensureInit(width, height, title);
    if (setup) setup();
    return run(width, height, title);
}

EntityHandle cube(float x, float y, float z, float sx, float sy, float sz) {
    ensureInit(1280, 720, "PyRecT3D");
    if (!g_ctx.win) return EntityHandle{};
    Entity* e = new Cube(Vec3(x, y, z), Vec3(sx, sy, sz));
    std::int64_t id = g_ctx.win->scene().addEntity(e);
    publishScene(EventType::SceneCreate, EntityHandle{ id });
    return EntityHandle{ id };
}

EntityHandle screenCube(float x, float y, float w, float h) {
    ensureInit(1280, 720, "PyRecT3D");
    if (!g_ctx.win) return EntityHandle{};
    Entity* e = new ScreenCube(x, y, w, h);
    std::int64_t id = g_ctx.win->scene().addEntity(e);
    return EntityHandle{ id };
}

EntityHandle clone(EntityHandle src) {
    if (!g_ctx.win || !src.valid()) return EntityHandle{};
    std::int64_t id = g_ctx.win->scene().cloneEntity(src.id);   // 复制场上完整状态
    publishScene(EventType::EntityClone, EntityHandle{ id });
    return EntityHandle{ id };
}

EntityHandle summon(EntityHandle prototype, float x, float y, float z) {
    if (!g_ctx.win) return EntityHandle{};
    std::int64_t id = g_ctx.win->scene().summonEntity(prototype.id, Vec3(x, y, z));
    publishScene(EventType::EntitySummon, EntityHandle{ id }, x, y, z);
    return EntityHandle{ id };
}

void move(EntityHandle e, float x, float y, float z) {
    if (!g_ctx.win) return;
    Entity* ent = g_ctx.win->scene().getEntity(e.id);
    if (ent) ent->setPosition(Vec3(x, y, z));
    publishScene(EventType::TransformMove, e, x, y, z);
}

void rotate(EntityHandle e, float rx, float ry, float rz) {
    if (!g_ctx.win) return;
    Entity* ent = g_ctx.win->scene().getEntity(e.id);
    if (ent) ent->setRotation(Vec3(rx, ry, rz));
    publishScene(EventType::TransformRotate, e, rx, ry, rz);
}

void scale(EntityHandle e, float sx, float sy, float sz) {
    if (!g_ctx.win) return;
    Entity* ent = g_ctx.win->scene().getEntity(e.id);
    if (ent) ent->setScale(Vec3(sx, sy, sz));
    publishScene(EventType::TransformScale, e, sx, sy, sz);
}

void remove(EntityHandle e) {
    if (!g_ctx.win) return;
    g_ctx.win->scene().removeEntity(e.id);
    publishScene(EventType::EntityRemove, e);
}

HitResult raycast(float ox, float oy, float oz,
                  float dx, float dy, float dz, float maxDistance) {
    HitResult out;
    if (!g_ctx.win) return out;
    RaycastHit h = g_ctx.win->scene().raycast(Ray(Vec3(ox, oy, oz), Vec3(dx, dy, dz).normalized()));
    if (!h.hit || h.distance > maxDistance) return out;
    out.hit = true;
    out.distance = h.distance;
    out.point = h.point;
    out.normal = h.normal;
    out.entityId = h.entityId;
    // 射线结果入消息队列（不再是同步直回）
    Message m;
    m.type = EventType::RaycastHitReport;
    m.priority = EventPriority::Normal;
    m.payload.entityId = (std::uint64_t)h.entityId;
    m.payload.v[0] = h.distance;
    MainMessageQueue::instance().publish(m);
    return out;
}

Button* button(float x, float y, float w, float h, const std::string& text) {
    ensureInit(1280, 720, "PyRecT3D");
    if (!g_ctx.ui) return nullptr;
    return g_ctx.ui->addButton(Rect{ x, y, w, h }, text);
}

Text* text(float x, float y, const std::string& content) {
    ensureInit(1280, 720, "PyRecT3D");
    if (!g_ctx.ui) return nullptr;
    Rect r{ x, y, 300, 24 };
    return g_ctx.ui->addText(r, content);
}

VideoPlayer* video(const std::string& path, bool loop) {
    ensureInit(1280, 720, "PyRecT3D");
    if (!g_ctx.app || !g_ctx.win) return nullptr;
    VideoDesc d;
    d.path = path;
    d.loop = loop;
    d.autoPlay = true;
    return g_ctx.app->createVideoPlayer(d, g_ctx.win);  // 单窗口：自动绑该窗口子队列
}

Scene& scene() { return g_ctx.win->scene(); }
Gui& gui() { return g_ctx.ui->gui(); }

} // namespace quick
} // namespace pyrect3d
