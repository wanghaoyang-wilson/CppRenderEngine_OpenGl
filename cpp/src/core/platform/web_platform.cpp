// ============================================================
//  PyRecT3D — Web 平台实现（Emscripten / WebGL2）
//
//  v0.2.0 现阶段是「无窗口 stub」：
//  - 引擎主循环、消息队列、数学、碰撞、克隆这些纯 CPU 逻辑可以在
//    浏览器 / Node 里跑通；
//  - createWindow 不真开窗口（canvas 由 JS 侧 attach）；
//  - swapBuffers 是空实现，真正的 WebGL 上下文由 emscripten +
//    gl_backend 在后续版本接入。
// ============================================================
#if defined(__EMSCRIPTEN__) || defined(PYRECT3D_PLATFORM_WEB)
#include "platform_private.h"
#include <cstring>

namespace pyrect3d {

class WebPlatform : public PlatformImpl {
public:
    WebPlatform() = default;
    ~WebPlatform() override { destroyWindow(); }

    bool createWindow(int w, int h, const std::string& /*title*/, void* /*nativeWnd*/) override {
        width_ = w;
        height_ = h;
        opened_ = true;
        return true;
    }

    void destroyWindow() override { opened_ = false; }

    // 输入事件由 JS 桥通过 pyr3d_pump_input() 写入 InputState，
    // 这里的 pump 只负责把已记录的状态交给引擎，不轮询。
    void pump(InputState& /*state*/) override {}

    bool keyDown(Key k, const InputState& s) const override {
        if (static_cast<int>(k) < 0 || static_cast<int>(k) >= static_cast<int>(Key::Count))
            return false;
        return s.keys[static_cast<int>(k)];
    }

    void lockMouse(bool /*locked*/) override {}

    void* nativeHandle() const override { return nullptr; }

    void swapBuffers() override {}

private:
    int width_ = 0;
    int height_ = 0;
    bool opened_ = false;
};

PlatformImpl* createPlatform() {
    return new WebPlatform();
}

} // namespace pyrect3d
#endif
