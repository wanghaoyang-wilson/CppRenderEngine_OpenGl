// ============================================================
//  PyRecT3D/video.h — FFmpeg 多线程解码 / 视频材质（契约层）
//
//  规则：
//  - 全局播放器管理器（MediaManager）整个进程只有一个，自动绑定主线程与主消息队列。
//  - 通过管理器创建多个播放器实例（多路视频）。
//  - 解码在子线程完成，只准备 CPU 帧缓冲并置「帧就绪」标志；
//    GPU 纹理上传由主线程 OpenGL 循环检测标志后执行（GL 上下文不可跨线程）。
//  - 解码帧包装成动态材质（每帧变化的纹理），用户只需把它应用到任意物体表面。
//  - 播放器事件（播放完成 / 出错 / 暂停）投递到主消息队列，主队列依据播放器 id
//    路由到该播放器绑定的子消息队列；只有发出该事件的队列最终收到。
// ============================================================
#pragma once
#include "export.h"
#include "message.h"
#include <cstdint>
#include <string>
#include <vector>

namespace pyrect3d {

// ---------- 视频材质 ----------
// 动态材质：每帧随解码画面更新，可直接应用到任意实体 / GUI 组件
struct PYRECT3D_API VideoTexture {
    std::int64_t id = 0;          // 材质 id（引擎内部纹理句柄）
    int   width = 0;
    int   height = 0;
    std::uint64_t frameSerial = 0; // 帧序号，每次上传 +1
    bool  valid = false;
};

// ---------- 播放器配置 ----------
struct PYRECT3D_API VideoDesc {
    std::string path;             // 视频文件路径
    bool  loop = false;
    bool  decodeAudio = true;     // 是否同时解码音轨
    int   decodeThreads = 2;      // 解码线程数
    float volume = 1.0f;
    bool  autoPlay = true;
};

// ---------- 播放器实例 ----------
class PYRECT3D_API VideoPlayer {
public:
    std::int64_t id() const;
    std::int64_t boundQueueId() const;

    // 绑定子消息队列：单窗口可省略（自动弱绑定）；多窗口必须显式绑定
    bool bindQueue(MessageQueue* queue);
    bool autoBind();

    bool open(const VideoDesc& desc);
    bool play();
    bool pause();
    bool stop();
    bool seek(float seconds);

    bool  playing() const;
    float duration() const;
    float position() const;

    // 取当前动态材质（解码帧 → 纹理）
    VideoTexture texture() const;

    // 主线程每帧调用：检测子线程已就绪的帧并执行 GPU 上传
    bool uploadPendingFrame();

    void setVolume(float v);
    float volume() const;

private:
    friend class MediaManager;
    explicit VideoPlayer(std::int64_t id);
    ~VideoPlayer();
    struct Impl;
    Impl* impl_;
};

// ============================================================
//  全局播放器管理器（进程唯一）
// ============================================================
class PYRECT3D_API MediaManager {
public:
    static MediaManager& instance();

    // 创建播放器实例；多窗口时需随后 bindQueue()
    VideoPlayer* createPlayer();
    void destroyPlayer(VideoPlayer* player);

    // 主线程每帧调用：统一处理所有实例的帧上传
    void tick();

    std::size_t playerCount() const;

private:
    MediaManager();
    ~MediaManager();
    struct Impl;
    Impl* impl_;
};

} // namespace pyrect3d
