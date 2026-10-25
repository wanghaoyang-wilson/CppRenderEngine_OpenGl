// ============================================================
//  PyRecT3D — event_loop.cpp
//  消息循环辅助：时间步长、GUI 事件派发粘合
//  （Engine::run/pump 在 engine.cpp，这里补充分布式调度辅助）
// ============================================================
#include <PyRecT3D/core.h>
#include <PyRecT3D/gui.h>
#include <chrono>

namespace pyrect3d {

namespace {

// 简单高精度时钟，用于帧时间
class FrameClock {
public:
    FrameClock() : last_(now()) {}
    double tick() {
        auto cur = now();
        double dt = std::chrono::duration<double>(cur - last_).count();
        last_ = cur;
        return dt;
    }
private:
    static std::chrono::steady_clock::time_point now() {
        return std::chrono::steady_clock::now();
    }
    std::chrono::steady_clock::time_point last_;
};

// 每帧由外部调用：把当前输入转成 GUI 事件并派发
void dispatchGui(Gui& gui, const InputState& in, bool leftClicked) {
    GuiEvent ev;
    ev.mouseX = in.mouseX;
    ev.mouseY = in.mouseY;
    ev.clicked = leftClicked;
    ev.hovered = false;
    gui.dispatch(ev);
}

} // namespace

} // namespace pyrect3d
