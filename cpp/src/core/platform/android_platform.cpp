// ============================================================
//  PyRecT3D — Android 平台实现（NDK + GLES3）
//
//  v0.2.0 现阶段是「Java 提供 EGL 上下文」的 stub：
//  - createWindow：只记录尺寸，不真开窗口（Activity 的 SurfaceView
//    由 Java 侧 attach，引擎拿到的是 nativeHandle()=nullptr，后续由
//    native_window_* 系列接入 ANativeWindow）；
//  - swapBuffers：实际由 Java 的 GLSurfaceView eglSwapBuffers 完成；
//  - 输入：Java 层通过 nativeTouch 把事件写入消息队列，pump() 不轮询。
// ============================================================
#if defined(__ANDROID__)
#include "platform_private.h"
#include <jni.h>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "PyRecT3D", __VA_ARGS__)

namespace pyrect3d {

class AndroidPlatform : public PlatformImpl {
public:
    AndroidPlatform() = default;
    ~AndroidPlatform() override { destroyWindow(); }

    bool createWindow(int w, int h, const std::string& /*title*/, void* /*nativeWnd*/) override {
        width_ = w; height_ = h; opened_ = true;
        LOGI("AndroidPlatform::createWindow %dx%d (SurfaceView 由 Java 持有)", w, h);
        return true;
    }

    void destroyWindow() override { opened_ = false; }

    void pump(InputState& /*state*/) override {}

    bool keyDown(Key k, const InputState& s) const override {
        if (static_cast<int>(k) < 0 || static_cast<int>(k) >= static_cast<int>(Key::Count))
            return false;
        return s.keys[static_cast<int>(k)];
    }

    void lockMouse(bool /*locked*/) override {}

    void* nativeHandle() const override { return nullptr; }

    void swapBuffers() override { /* 由 Java eglSwapBuffers 完成 */ }

private:
    int width_ = 0;
    int height_ = 0;
    bool opened_ = false;
};

PlatformImpl* createPlatform() {
    return new AndroidPlatform();
}

} // namespace pyrect3d
#endif
