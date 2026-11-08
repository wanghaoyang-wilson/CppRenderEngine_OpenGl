// ============================================================
//  PyRecT3D — src/core/explicit_app.cpp
//  显式 API 实现：多窗口 / 子消息队列挂载 / UI 与播放器手动绑定
// ============================================================
#include <PyRecT3D/explicit.h>
#include <PyRecT3D/audio.h>
#include <PyRecT3D/io.h>
#include "core/app_registry.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace pyrect3d {

// ============================================================
//  UIInstance
// ============================================================
struct UIInstance::Impl {
    Gui gui;
    std::int64_t windowId = -1;
    WindowInstance* window = nullptr;
};

UIInstance::UIInstance() : impl_(new Impl()) {}
UIInstance::~UIInstance() { delete impl_; }

bool UIInstance::bindWindow(WindowInstance* window) {
    if (!window) return false;
    impl_->window = window;
    impl_->windowId = window->id();
    return true;
}

std::int64_t UIInstance::boundWindowId() const { return impl_->windowId; }
Gui& UIInstance::gui() { return impl_->gui; }

Button* UIInstance::addButton(const Rect& r, const std::string& text) {
    Button* b = new Button(r, text);
    impl_->gui.addWidget(b);
    return b;
}
Text* UIInstance::addText(const Rect& r, const std::string& text) {
    Text* t = new Text(r, text);
    impl_->gui.addWidget(t);
    return t;
}
InputBox* UIInstance::addInputBox(const Rect& r, const std::string& text) {
    InputBox* i = new InputBox(r, text);
    impl_->gui.addWidget(i);
    return i;
}
Container* UIInstance::addContainer(const Rect& r) {
    Container* c = new Container(r);
    impl_->gui.addWidget(c);
    return c;
}
ScrollContainer* UIInstance::addScrollContainer(const Rect& r) {
    ScrollContainer* c = new ScrollContainer(r);
    impl_->gui.addWidget(c);
    return c;
}

void UIInstance::drawAll() { impl_->gui.drawAll(); }

// ============================================================
//  WindowInstance
// ============================================================
struct WindowInstance::Impl {
    std::int64_t id = 0;
    std::string title;
    int w = 1280, h = 720;

    MessageQueue queue;
    Engine engine;
    Renderer renderer;
    Compositor compositor;
    Scene scene;
    Entity* camera = nullptr;
    UIInstance* ui = nullptr;

    bool engineReady = false;   // 无显示环境下为 false（headless）
    bool uiWeakBound = false;

    Impl() : queue(0) {}
};

WindowInstance::WindowInstance(std::int64_t id) : impl_(new Impl()) {
    impl_->id = id;
    // 子消息队列 id 由 MessageQueue 内部自增分配，窗口 id 与之独立
}

WindowInstance::~WindowInstance() {
    delete impl_->ui;
    delete impl_->camera;
    delete impl_;
}

std::int64_t WindowInstance::id() const { return impl_->id; }
MessageQueue* WindowInstance::queue() { return &impl_->queue; }
Scene& WindowInstance::scene() { return impl_->scene; }
Entity* WindowInstance::camera() { return impl_->camera; }
int WindowInstance::width() const { return impl_->w; }
int WindowInstance::height() const { return impl_->h; }
void WindowInstance::setTitle(const std::string& title) { impl_->title = title; }

UIInstance* WindowInstance::createUI() {
    if (impl_->ui) return impl_->ui;
    impl_->ui = new UIInstance();
    // 单窗口由 ExplicitApp 调用 bindWindow 完成弱绑定；多窗口要求显式绑定
    return impl_->ui;
}

void WindowInstance::renderScene() {
    impl_->renderer.beginFrame();
    impl_->scene.forEach([this](std::int64_t, Entity& e) {
        impl_->renderer.drawEntity(e);
    });
    impl_->compositor.setSceneDirty(false);
}

void WindowInstance::present() {
    if (impl_->compositor.sceneDirty()) renderScene();
    if (impl_->ui) impl_->ui->drawAll();
    impl_->compositor.present();
}

// ============================================================
//  ExplicitApp
// ============================================================
struct ExplicitApp::Impl {
    AppConfig cfg;
    bool configured = false;
    std::atomic<bool> quit{false};
    std::vector<WindowInstance*> windows;
    std::int64_t nextWindowId = 1;
    std::vector<CollisionWorld*> worlds;
};

ExplicitApp::ExplicitApp() : impl_(new Impl()) {}

ExplicitApp::~ExplicitApp() {
    for (auto* w : impl_->windows) { delete w; }
    impl_->windows.clear();
    for (auto* c : impl_->worlds) delete c;
    impl_->worlds.clear();
    delete impl_;
}

bool ExplicitApp::configure(const AppConfig& cfg) {
    impl_->cfg = cfg;
    impl_->configured = true;
    if (cfg.enableVideo) impl_->cfg.enableVideo = true;
    return true;
}

WindowInstance* ExplicitApp::createWindow(const WindowDesc& desc) {
    if (!impl_->configured) {
        // 未 configure 时用默认配置兜底
        impl_->cfg = AppConfig{};
        impl_->configured = true;
    }

    std::int64_t id = impl_->nextWindowId++;
    WindowInstance* win = new WindowInstance(id);
    win->impl_->title = desc.title;
    win->impl_->w = desc.width;
    win->impl_->h = desc.height;

    // 渲染器初始化（软件回退始终可用）
    win->impl_->renderer.init(desc.width, desc.height);
    win->impl_->compositor.attach(&win->impl_->renderer);

    // 相机占位实体
    win->impl_->camera = new Entity(0);
    win->impl_->camera->setPosition(Vec3(0, 0, 0));

    // 窗口创建（无显示环境 headless，不视为失败）
    win->impl_->engineReady =
        win->impl_->engine.init(desc.width, desc.height, desc.title, desc.nativeHandle);

    // 每个窗口自动获得一个子消息队列
    if (impl_->windows.empty()) {
        // 单窗口：自动挂载 + 弱绑定
        win->impl_->queue.attachToMain();
        win->impl_->uiWeakBound = true;
    } else {
        // 多窗口：必须由调用者 mountQueues() 显式挂载
        win->impl_->uiWeakBound = false;
    }

    impl_->windows.push_back(win);

    Message m;
    m.type = EventType::WindowCreate;
    m.priority = EventPriority::WindowInput;
    m.payload.windowId = id;
    MainMessageQueue::instance().publish(m);

    return win;
}

WindowInstance* ExplicitApp::window(std::int64_t id) {
    for (auto* w : impl_->windows) if (w->id() == id) return w;
    return nullptr;
}

std::size_t ExplicitApp::windowCount() const { return impl_->windows.size(); }

bool ExplicitApp::mountQueues() {
    bool all = true;
    for (auto* w : impl_->windows) {
        if (!w->queue()->attachToMain()) all = false;
    }
    return all;
}

VideoPlayer* ExplicitApp::createVideoPlayer(const VideoDesc& desc, WindowInstance* bindTo) {
#if !PYRECT3D_WITH_VIDEO
    (void)desc; (void)bindTo;
    return nullptr;
#else
    VideoPlayer* p = MediaManager::instance().createPlayer();
    if (!p) return nullptr;
    if (bindTo) {
        p->bindQueue(bindTo->queue());          // 多窗口：显式绑定
    } else if (impl_->windows.size() == 1) {
        p->bindQueue(impl_->windows.front()->queue());  // 单窗口：弱绑定
    }
    p->open(desc);
    return p;
#endif
}

std::uint64_t ExplicitApp::publishGlobal(const Message& m) {
    return MainMessageQueue::instance().publish(m);
}

bool ExplicitApp::hasModule(Module m) const {
    switch (m) {
        case Module::Renderer:   return true;
        case Module::GUI:        return PYRECT3D_WITH_GUI;
        case Module::Video:      return PYRECT3D_WITH_VIDEO;
        case Module::Audio:      return PYRECT3D_WITH_AUDIO;
        case Module::Physics:    return PYRECT3D_WITH_PHYSICS;
        case Module::RayTracer:  return PYRECT3D_WITH_RAYTRACER;
        case Module::Python:     return true;
        default: return false;
    }
}

bool ExplicitApp::pump() {
    if (impl_->quit.load()) return false;

    // 1) 主消息队列：窗口级输入最高优先级，先派发
    MainMessageQueue::instance().pump();

    // 2) 逐窗口：读取平台输入 → 发布输入事件 → 物理/渲染
    for (auto* w : impl_->windows) {
        if (!w) continue;
        if (w->impl_->engineReady) {
            w->impl_->engine.pump();
            const InputState& in = w->impl_->engine.input();

            Message mv;
            mv.type = EventType::MouseMove;
            mv.priority = EventPriority::WindowInput;
            mv.payload.windowId = w->id();
            mv.payload.v[0] = (float)in.mouseX;
            mv.payload.v[1] = (float)in.mouseY;
            mv.payload.v[2] = (float)in.mouseDeltaX;
            mv.payload.v[3] = (float)in.mouseDeltaY;
            MainMessageQueue::instance().publish(mv);

            Message kd;
            kd.type = EventType::KeyDown;
            kd.priority = EventPriority::WindowInput;
            kd.payload.windowId = w->id();
            if (in.isDown(Key::W)) { kd.payload.code = (std::int32_t)Key::W; MainMessageQueue::instance().publish(kd); }
            if (in.isDown(Key::A)) { kd.payload.code = (std::int32_t)Key::A; MainMessageQueue::instance().publish(kd); }
            if (in.isDown(Key::S)) { kd.payload.code = (std::int32_t)Key::S; MainMessageQueue::instance().publish(kd); }
            if (in.isDown(Key::D)) { kd.payload.code = (std::int32_t)Key::D; MainMessageQueue::instance().publish(kd); }
            if (in.isDown(Key::Escape)) {
                Message q;
                q.type = EventType::AppQuit;
                q.priority = EventPriority::WindowInput;
                MainMessageQueue::instance().publish(q);
            }
        }

        // 渲染：场景脏才重绘，UI 只叠加（Docker 式软更新）
        w->present();
    }

    // 3) 媒体：子线程解码帧在主线程上传
    MediaManager::instance().tick();

    // 4) 无事可做时让出 CPU（真实实现由 vsync 驱动）
    if (!impl_->windows.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
    return true;
}

int ExplicitApp::run() {
    detail::setAppQuitFlag(false);
    impl_->quit.store(false);

    detail::setAppPumpHook([this]() -> bool {
        return pump();
    });

    int rc = App::run();
    detail::setAppPumpHook(nullptr);
    return rc;
}

void ExplicitApp::quit() {
    impl_->quit.store(true);
    detail::setAppQuitFlag(true);
}

} // namespace pyrect3d
