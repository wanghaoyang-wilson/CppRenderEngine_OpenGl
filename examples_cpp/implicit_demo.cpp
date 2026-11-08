// ============================================================
//  PyRecT3D 示例 —— 隐式 API（Implicit / Quick API）
//  适用：小游戏、教学、Scratch 式快速入门
//  单窗口全自动弱绑定：窗口→子队列、UI→窗口、播放器→子队列
// ============================================================
#include <PyRecT3D/implicit.h>
#include <cstdio>

using namespace pyrect3d;

int main() {
    // 一行启动：单窗口 + 3D 场景 + 消息队列自动弱绑定
    quick::runSetup(1280, 720, "PyRecT3D — Quick", [] {
        // 创建方块
        EntityHandle a = quick::cube(0, 0, -5, 1, 1, 1);
        EntityHandle b = quick::cube(2, 0, -6, 1, 1, 1);

        // 克隆：复制 a 当前状态（Scratch 用户最熟悉的思路）
        EntityHandle c = quick::clone(a);
        quick::move(c, -2, 0, -5);

        // 变换
        quick::rotate(b, 0, 30, 0);
        quick::scale(b, 1.5f, 1.5f, 1.5f);

        // UI 自动绑定唯一窗口
        Button* btn = quick::button(20, 20, 140, 40, "复位");
        btn->onClick = [a] { quick::move(a, 0, 0, -5); };
        btn->onDragOut = [] { std::printf("按钮被拖出\n"); };

        // 视频（单窗口自动绑定当前窗口子队列）
        quick::video("assets/demo.mp4", true);
    });

    return 0;
}
