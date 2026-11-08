// ============================================================
//  PyRecT3D — src/core/message_queue.cpp
//  主消息队列 / 子消息队列 / 事件优先级调度 / APP 启动契约
//
//  调度规则：按 (priority 升序, seq 升序) 出队 ——
//    窗口输入(0) > 物理(1) > 碰撞(2) > 音效(3) > 媒体(4) > 普通(5)
//  线程安全：入队/出队均在互斥锁保护下完成；主队列记录绑定线程 id，
//            GPU 资源操作只在绑定线程执行。
// ============================================================
#include <PyRecT3D/message.h>
#include "core/app_registry.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

namespace pyrect3d {

namespace {

std::atomic<bool> g_appQuit{false};
std::atomic<std::uint64_t> g_seq{0};

inline int prioOf(EventPriority p) { return static_cast<int>(p); }

bool cmpMsg(const Message& a, const Message& b) {
    int pa = prioOf(a.priority), pb = prioOf(b.priority);
    if (pa != pb) return pa < pb;
    return a.seq < b.seq;
}

} // namespace

namespace detail {

static std::function<bool()> g_pumpHook;

void setAppPumpHook(std::function<bool()> hook) { g_pumpHook = std::move(hook); }
std::function<bool()>& appPumpHook() { return g_pumpHook; }
void setAppQuitFlag(bool v) { g_appQuit.store(v); }
bool appQuitRequested() { return g_appQuit.load(); }

} // namespace detail

// ============================================================
//  子消息队列
// ============================================================
struct MessageQueue::Impl {
    std::int64_t id = 0;
    bool attached = false;
    std::mutex mtx;
    std::vector<Message> local;
    EventHandler handler;
};

MessageQueue::MessageQueue(std::int64_t id) : impl_(new Impl()) {
    impl_->id = (id == 0) ? static_cast<std::int64_t>(g_seq.fetch_add(1) + 1) : id;
}

MessageQueue::~MessageQueue() {
    if (impl_->attached) {
        MainMessageQueue::instance().detachChild(this);
    }
    delete impl_;
}

std::int64_t MessageQueue::id() const { return impl_->id; }

bool MessageQueue::attachToMain() {
    if (impl_->attached) return true;
    impl_->attached = MainMessageQueue::instance().attachChild(this);
    return impl_->attached;
}

bool MessageQueue::detachFromMain() {
    if (!impl_->attached) return false;
    MainMessageQueue::instance().detachChild(this);
    impl_->attached = false;
    return true;
}

bool MessageQueue::attached() const { return impl_->attached; }

void MessageQueue::publish(const Message& in) {
    Message m = in;
    m.sourceQueue = impl_->id;
    if (m.targetQueue < 0) m.targetQueue = impl_->id;   // 默认回投自己
    MainMessageQueue::instance().publish(m);
}

void MessageQueue::publishLocal(const Message& in) {
    Message m = in;
    if (m.sourceQueue == 0) m.sourceQueue = impl_->id;
    m.seq = g_seq.fetch_add(1);
    std::lock_guard<std::mutex> lk(impl_->mtx);
    impl_->local.push_back(m);
}

bool MessageQueue::poll(Message& out) {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    if (impl_->local.empty()) return false;
    auto it = std::min_element(impl_->local.begin(), impl_->local.end(), cmpMsg);
    out = *it;
    impl_->local.erase(it);
    return true;
}

std::size_t MessageQueue::pending() const {
    std::lock_guard<std::mutex> lk(const_cast<Impl*>(impl_)->mtx);
    return impl_->local.size();
}

void MessageQueue::setHandler(EventHandler handler) { impl_->handler = std::move(handler); }

void MessageQueue::dispatch() {
    // 没有注册处理器的队列不消费消息，消息保留给调用者 poll()
    if (!impl_->handler) return;
    Message m;
    while (poll(m)) {
        if (impl_->handler) impl_->handler(m);
        if (m.type == EventType::AppQuit || m.type == EventType::WindowClose) {
            detail::setAppQuitFlag(true);
        }
    }
}

// ============================================================
//  主消息队列
// ============================================================
struct MainMessageQueue::Impl {
    std::mutex mtx;
    std::vector<Message> global;
    std::vector<MessageQueue*> children;
    std::vector<EventHandler> subs;
    std::uint64_t seq = 0;
    std::uint64_t threadId = 0;
};

MainMessageQueue::MainMessageQueue() : impl_(new Impl()) {
    impl_->threadId = static_cast<std::uint64_t>(
        std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

MainMessageQueue::~MainMessageQueue() { delete impl_; }

MainMessageQueue& MainMessageQueue::instance() {
    static MainMessageQueue s;
    return s;
}

std::uint64_t MainMessageQueue::publish(const Message& in) {
    Message m = in;
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        m.seq = ++impl_->seq;
    }

    MessageQueue* target = nullptr;
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        if (m.targetQueue >= 0) {
            for (auto* c : impl_->children) {
                if (c->id() == m.targetQueue) { target = c; break; }
            }
        }
        if (!target) impl_->global.push_back(m);
    }

    if (target) target->publishLocal(m);
    return m.seq;
}

bool MainMessageQueue::poll(Message& out) {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    if (impl_->global.empty()) return false;
    auto it = std::min_element(impl_->global.begin(), impl_->global.end(), cmpMsg);
    out = *it;
    impl_->global.erase(it);
    return true;
}

std::size_t MainMessageQueue::pending() const {
    std::lock_guard<std::mutex> lk(const_cast<Impl*>(impl_)->mtx);
    return impl_->global.size();
}

void MainMessageQueue::pump() {
    // 1) 全局消息：按优先级派发；带 targetQueue 的转发给对应子队列
    Message m;
    while (poll(m)) {
        std::vector<EventHandler> subs;
        {
            std::lock_guard<std::mutex> lk(impl_->mtx);
            subs = impl_->subs;
        }
        for (auto& h : subs) if (h) h(m);

        if (m.type == EventType::AppQuit) detail::setAppQuitFlag(true);

        if (m.targetQueue >= 0) {
            MessageQueue* target = findChild(m.targetQueue);
            if (target) target->publishLocal(m);
        }
    }

    // 2) 子队列：各自按其窗口业务派发
    std::vector<MessageQueue*> kids;
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        kids = impl_->children;
    }
    for (auto* c : kids) if (c) c->dispatch();
}

void MainMessageQueue::subscribe(EventHandler handler) {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    impl_->subs.push_back(std::move(handler));
}

std::size_t MainMessageQueue::childCount() const {
    std::lock_guard<std::mutex> lk(const_cast<Impl*>(impl_)->mtx);
    return impl_->children.size();
}

MessageQueue* MainMessageQueue::findChild(std::int64_t id) {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    for (auto* c : impl_->children) if (c->id() == id) return c;
    return nullptr;
}

MessageQueue* MainMessageQueue::firstChild() {
    std::lock_guard<std::mutex> lk(impl_->mtx);
    return impl_->children.empty() ? nullptr : impl_->children.front();
}

bool MainMessageQueue::attachChild(MessageQueue* child) {
    if (!child) return false;
    std::lock_guard<std::mutex> lk(impl_->mtx);
    for (auto* c : impl_->children) if (c == child) return true;
    impl_->children.push_back(child);
    return true;
}

bool MainMessageQueue::detachChild(MessageQueue* child) {
    if (!child) return false;
    std::lock_guard<std::mutex> lk(impl_->mtx);
    auto it = std::find(impl_->children.begin(), impl_->children.end(), child);
    if (it == impl_->children.end()) return false;
    impl_->children.erase(it);
    return true;
}

std::uint64_t MainMessageQueue::threadId() const { return impl_->threadId; }

// ============================================================
//  App
// ============================================================
int App::run() {
    detail::setAppQuitFlag(false);
    std::function<bool()>& hook = detail::appPumpHook();
    while (!detail::appQuitRequested()) {
        if (hook) {
            if (!hook()) break;
        } else {
            MainMessageQueue::instance().pump();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    return 0;
}

bool App::pump() {
    std::function<bool()>& hook = detail::appPumpHook();
    if (hook) return hook();
    MainMessageQueue::instance().pump();
    return !detail::appQuitRequested();
}

void App::quit() {
    Message m;
    m.type = EventType::AppQuit;
    m.priority = EventPriority::WindowInput;
    MainMessageQueue::instance().publish(m);
    detail::setAppQuitFlag(true);
}

} // namespace pyrect3d
