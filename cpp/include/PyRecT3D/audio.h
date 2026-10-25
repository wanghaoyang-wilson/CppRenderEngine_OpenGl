// ============================================================
//  PyRecT3D/audio.h — 音频子系统（契约层，仅声明）
// ============================================================
#pragma once
#include "export.h"
#include <string>

namespace pyrect3d {

// 简单音乐播放接口
class PYRECT3D_API Audio {
public:
    Audio();
    ~Audio();

    // 播放音乐文件（wav/mp3 等，按平台支持）
    bool playMusic(const std::string& path, bool loop = true);
    void stopMusic();
    void pauseMusic();
    void resumeMusic();

    void setVolume(float v);   // 0..1
    float volume() const;

private:
    struct Impl;
    Impl* impl_;
};

} // namespace pyrect3d
