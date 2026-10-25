// ============================================================
//  PyRecT3D — audio.cpp
//  音频子系统（演示实现）
//  Windows: winmm 音频；Linux: 占位（可接 ALSA/pulse）
// ============================================================
#include <PyRecT3D/audio.h>

namespace pyrect3d {

#ifdef _WIN32
// 简化占位：完整实现使用 mciSendString 或波形 API
#include <windows.h>
#include <mmsystem.h>
#endif

struct Audio::Impl {
    float volume_ = 0.8f;
    bool playing = false;
    bool paused = false;
};

Audio::Audio() : impl_(new Impl()) {}
Audio::~Audio() { delete impl_; }

bool Audio::playMusic(const std::string& path, bool loop) {
    (void)loop;
#ifdef _WIN32
    // mciSendString 播放（Win7 兼容）
    std::string cmd = "open \"" + path + "\" type mpegvideo alias pyr3d_music";
    if (mciSendStringA(cmd.c_str(), nullptr, 0, nullptr) == 0) {
        mciSendStringA("play pyr3d_music repeat", nullptr, 0, nullptr);
        impl_->playing = true;
        return true;
    }
    return false;
#else
    // Linux 占位：可通过外部播放器集成
    (void)path;
    return false;
#endif
}

void Audio::stopMusic() {
#ifdef _WIN32
    mciSendStringA("stop pyr3d_music", nullptr, 0, nullptr);
    mciSendStringA("close pyr3d_music", nullptr, 0, nullptr);
#endif
    impl_->playing = false;
    impl_->paused = false;
}

void Audio::pauseMusic() {
#ifdef _WIN32
    mciSendStringA("pause pyr3d_music", nullptr, 0, nullptr);
#endif
    impl_->paused = true;
}

void Audio::resumeMusic() {
#ifdef _WIN32
    mciSendStringA("resume pyr3d_music", nullptr, 0, nullptr);
#endif
    impl_->paused = false;
}

void Audio::setVolume(float v) { impl_->volume_ = v < 0 ? 0 : (v > 1 ? 1 : v); }
float Audio::volume() const { return impl_->volume_; }

} // namespace pyrect3d
