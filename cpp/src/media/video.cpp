// ============================================================
//  PyRecT3D — src/media/video.cpp
//  全局播放器管理器 + 多播放器实例 + 子线程解码 + 主线程上传
//
//  帧同步策略（关键）：
//    子线程解码 → 填 CPU 帧缓冲 → 置 frameReady 原子标志
//    主线程 OpenGL 循环检测 frameReady → 执行 GPU 纹理上传
//    （GL 上下文不可跨线程调用，因此绝不在子线程触碰 GL）
//
//  FFmpeg 接入点：定义宏 PYRECT3D_WITH_FFMPEG=1 时在此文件里把
//  decodeWorker 内部替换为 avformat/avcodec 的 sws_scale 输出 RGB 帧。
//  未定义时使用内置占位帧生成（保证无外部依赖也能编译运行）。
// ============================================================
#include <PyRecT3D/video.h>
#include "core/app_registry.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

namespace pyrect3d {

namespace {
std::atomic<std::int64_t> g_texSerial{1};
std::atomic<std::int64_t> g_playerSerial{1};
}

// ============================================================
//  VideoPlayer
// ============================================================
struct VideoPlayer::Impl {
    std::int64_t id = 0;
    std::int64_t boundQueue = -1;
    VideoDesc desc;

    std::mutex mtx;
    std::thread worker;
    std::atomic<bool> running{false};
    std::atomic<bool> playing{false};
    std::atomic<bool> frameReady{false};
    std::atomic<bool> finishPending{false};
    std::atomic<bool> errorPending{false};

    std::vector<unsigned char> cpuFrame;   // w*h*3
    int w = 0, h = 0;

    std::atomic<std::uint64_t> frameSerial{0};
    VideoTexture tex;
    std::atomic<float> position{0.f};
    float duration = 0.f;
    std::atomic<float> volume{1.f};
    float targetFps = 30.f;

    // ---- 解码工作线程：只填 CPU 帧 与 置标志，不碰 GL ----
    void decodeWorker() {
        float t = 0.f;
        double frameDt = 1.0 / (targetFps > 0 ? targetFps : 30.f);

        while (running.load()) {
            if (!playing.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }

            if (cpuFrame.empty()) cpuFrame.assign((size_t)w * h * 3, 0);

            // ---- FFmpeg 接入点 ----
            // av_read_frame → avcodec_send_packet/receive_frame → sws_scale 到 RGB24
            // 写入 cpuFrame；随后 frameReady.store(true)
            // ---- 占位实现：生成时间渐变帧（演示多实例并发解码）----
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    size_t i = ((size_t)y * w + x) * 3;
                    unsigned char r = (unsigned char)((x * 255 / (w > 0 ? w : 1) + (int)(t * 40)) & 0xFF);
                    unsigned char g = (unsigned char)((y * 255 / (h > 0 ? h : 1) + (int)(t * 25)) & 0xFF);
                    unsigned char b = (unsigned char)((int)(t * 60) & 0xFF);
                    cpuFrame[i] = r; cpuFrame[i + 1] = g; cpuFrame[i + 2] = b;
                }
            }

            frameSerial.fetch_add(1);
            frameReady.store(true);

            position.store(position.load() + (float)frameDt);
            t += frameDt;
            if (desc.loop && duration > 0.f && position.load() >= duration) position.store(0.f);

            std::this_thread::sleep_for(std::chrono::duration<double>(frameDt));
        }
    }
};

VideoPlayer::VideoPlayer(std::int64_t id) : impl_(new Impl()) { impl_->id = id; }

VideoPlayer::~VideoPlayer() {
    impl_->running.store(false);
    if (impl_->worker.joinable()) impl_->worker.join();
    delete impl_;
}

std::int64_t VideoPlayer::id() const { return impl_->id; }
std::int64_t VideoPlayer::boundQueueId() const { return impl_->boundQueue; }

bool VideoPlayer::bindQueue(MessageQueue* queue) {
    if (!queue) return false;
    impl_->boundQueue = queue->id();
    if (!queue->attached()) queue->attachToMain();
    return true;
}

bool VideoPlayer::autoBind() {
    // 单窗口弱绑定：只有唯一子队列时才自动绑定；多窗口必须显式 bindQueue()
    MainMessageQueue& main = MainMessageQueue::instance();
    if (main.childCount() != 1) return false;
    MessageQueue* only = main.firstChild();
    if (!only) return false;
    impl_->boundQueue = only->id();
    if (!only->attached()) only->attachToMain();
    return true;
}

bool VideoPlayer::open(const VideoDesc& desc) {
    impl_->desc = desc;
    impl_->volume.store(desc.volume);
    impl_->w = 640;
    impl_->h = 360;
    impl_->duration = 30.f;            // 占位时长；FFmpeg 接入后取 stream duration
    impl_->cpuFrame.assign((size_t)impl_->w * impl_->h * 3, 0);

    impl_->running.store(true);
    impl_->worker = std::thread([this] { impl_->decodeWorker(); });

    if (desc.autoPlay) return play();
    return true;
}

bool VideoPlayer::play() {
    if (!impl_->running.load()) return false;
    impl_->playing.store(true);
    return true;
}

bool VideoPlayer::pause() {
    if (!impl_->playing.load()) return false;
    impl_->playing.store(false);
    Message m;
    m.type = EventType::VideoPaused;
    m.priority = EventPriority::Media;
    m.targetQueue = impl_->boundQueue;
    m.payload.playerId = impl_->id;
    MainMessageQueue::instance().publish(m);
    return true;
}

bool VideoPlayer::stop() {
    impl_->playing.store(false);
    impl_->position.store(0.f);
    return true;
}

bool VideoPlayer::seek(float seconds) {
    float d = impl_->duration;
    if (d > 0.f && seconds > d) seconds = d;
    if (seconds < 0.f) seconds = 0.f;
    impl_->position.store(seconds);
    return true;
}

bool VideoPlayer::playing() const { return impl_->playing.load(); }
float VideoPlayer::duration() const { return impl_->duration; }
float VideoPlayer::position() const { return impl_->position.load(); }

VideoTexture VideoPlayer::texture() const {
    std::lock_guard<std::mutex> lk(const_cast<Impl*>(impl_)->mtx);
    return impl_->tex;
}

// 主线程调用：检测子线程已就绪帧并上传 GPU
bool VideoPlayer::uploadPendingFrame() {
    if (!impl_->frameReady.load()) return false;
    impl_->frameReady.store(false);

    std::lock_guard<std::mutex> lk(impl_->mtx);
    if (impl_->tex.id == 0) impl_->tex.id = g_texSerial.fetch_add(1);
    impl_->tex.width = impl_->w;
    impl_->tex.height = impl_->h;
    impl_->tex.frameSerial = impl_->frameSerial.load();
    impl_->tex.valid = true;
    // GL 纹理上传点：此处必须在持有 GL 上下文的渲染主线程执行
    // glTexImage2D / glTexSubImage2D(cpuFrame)
    return true;
}

void VideoPlayer::setVolume(float v) {
    if (v < 0.f) v = 0.f;
    if (v > 1.f) v = 1.f;
    impl_->volume.store(v);
}
float VideoPlayer::volume() const { return impl_->volume.load(); }

// ============================================================
//  MediaManager（全局唯一）
// ============================================================
struct MediaManager::Impl {
    std::mutex mtx;
    std::vector<VideoPlayer*> players;
};

MediaManager::MediaManager() : impl_(new Impl()) {}
MediaManager::~MediaManager() {
    for (auto* p : impl_->players) delete p;
    impl_->players.clear();
    delete impl_;
}

MediaManager& MediaManager::instance() {
    static MediaManager s;
    return s;
}

VideoPlayer* MediaManager::createPlayer() {
    VideoPlayer* p = new VideoPlayer(g_playerSerial.fetch_add(1));
    std::lock_guard<std::mutex> lk(impl_->mtx);
    impl_->players.push_back(p);
    return p;
}

void MediaManager::destroyPlayer(VideoPlayer* player) {
    if (!player) return;
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        for (auto it = impl_->players.begin(); it != impl_->players.end(); ++it) {
            if (*it == player) { impl_->players.erase(it); break; }
        }
    }
    delete player;
}

void MediaManager::tick() {
    std::vector<VideoPlayer*> ps;
    {
        std::lock_guard<std::mutex> lk(impl_->mtx);
        ps = impl_->players;
    }
    for (auto* p : ps) {
        if (!p) continue;
        p->uploadPendingFrame();

        // 播放完成 / 出错 → 主队列发布，主队列按 playerId 路由到绑定子队列
        VideoPlayer::Impl* pi = p->impl_;
        if (pi->finishPending.exchange(false)) {
            Message m;
            m.type = EventType::VideoFinished;
            m.priority = EventPriority::Media;
            m.targetQueue = pi->boundQueue;
            m.payload.playerId = p->id();
            MainMessageQueue::instance().publish(m);
        }
        if (pi->errorPending.exchange(false)) {
            Message m;
            m.type = EventType::VideoError;
            m.priority = EventPriority::Media;
            m.targetQueue = pi->boundQueue;
            m.payload.playerId = p->id();
            m.payload.code = -1;
            MainMessageQueue::instance().publish(m);
        }
    }
}

std::size_t MediaManager::playerCount() const {
    std::lock_guard<std::mutex> lk(const_cast<Impl*>(impl_)->mtx);
    return impl_->players.size();
}

} // namespace pyrect3d
