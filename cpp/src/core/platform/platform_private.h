// ============================================================
//  PyRecT3D — platform_private.h
//  平台粘合层内部接口（不对外）
//  统一抽象窗口/上下文/输入/消息循环，屏蔽 Win7 vs Linux X11 差异
// ============================================================
#pragma once
#include <PyRecT3D/core.h>
#include <string>

namespace pyrect3d {

// 平台实例（每种平台一个实现）
class PlatformImpl {
public:
    virtual ~PlatformImpl() = default;

    // 创建窗口与 GL 上下文。nativeWnd 为外部提供的 native 句柄（可选）。
    virtual bool createWindow(int width, int height, const std::string& title,
                              void* nativeWnd) = 0;
    virtual void destroyWindow() = 0;

    // 处理一次消息，刷新输入状态
    virtual void pump(InputState& state) = 0;

    // 输入
    virtual bool keyDown(Key k, const InputState& s) const = 0;
    virtual void lockMouse(bool locked) = 0;

    // 原生句柄
    virtual void* nativeHandle() const = 0;

    // 交换缓冲
    virtual void swapBuffers() = 0;
};

// 工厂：按平台创建
PlatformImpl* createPlatform();

} // namespace pyrect3d
