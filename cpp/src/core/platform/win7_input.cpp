// ============================================================
//  PyRecT3D — win7_input.cpp
//  Windows 平台实现（最低 Win7 兼容）
//
//  关键点：
//  - 经典 Win32 消息循环 (PeekMessage + TranslateMessage/DispatchMessage)
//  - 不用 Win10+ 专属 API（无 SetProcessDpiAwarenessContext，用 SetProcessDPIAware）
//  - 不用 RawInput / XInput 新版，全用经典输入
//  - 鼠标锁定：ClipCursor + 隐藏光标
//  - Alt+F4 → WM_CLOSE
// ============================================================
#ifdef _WIN32
#include "platform_private.h"
#include <windows.h>
#include <cstring>

namespace pyrect3d {

namespace {

// Win7 兼容的 DPI 感知（Win7 无 SetProcessDpiAwarenessContext）
void enableDpiAware() {
    // SetProcessDPIAware 在 Win7 SP1 可用
    HMODULE user32 = GetModuleHandleA("user32.dll");
    if (user32) {
        typedef BOOL (WINAPI *SetProcessDPIAwareFn)(void);
        SetProcessDPIAwareFn fn = (SetProcessDPIAwareFn)GetProcAddress(user32, "SetProcessDPIAware");
        if (fn) fn();
    }
}

Key mapVK(UINT vk) {
    switch (vk) {
        case 'W': return Key::W;
        case 'A': return Key::A;
        case 'S': return Key::S;
        case 'D': return Key::D;
        case VK_SPACE: return Key::Space;
        case VK_ESCAPE: return Key::Escape;
        case VK_F4: return Key::AltF4;
        case VK_LBUTTON: return Key::MouseLeft;
        case VK_RBUTTON: return Key::MouseRight;
        case VK_MBUTTON: return Key::MouseMiddle;
        default: return Key::Unknown;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
        case WM_KEYUP: {
            if (wp == VK_F4 && (GetKeyState(VK_MENU) & 0x8000)) {
                PostQuitMessage(0);   // Alt+F4
                return 0;
            }
            break;
        }
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

} // namespace

class Win7Platform : public PlatformImpl {
public:
    Win7Platform() = default;
    ~Win7Platform() override { destroyWindow(); }

    bool createWindow(int w, int h, const std::string& title, void* nativeWnd) override {
        enableDpiAware();
        HINSTANCE hInst = GetModuleHandleA(nullptr);
        const char* cls = "PyRecT3D_Win7";

        WNDCLASSEXA wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
        wc.lpszClassName = cls;
        RegisterClassExA(&wc);

        if (nativeWnd) {
            // 外部接管窗口
            hwnd_ = (HWND)nativeWnd;
            RECT rc; GetClientRect(hwnd_, &rc);
            width_ = rc.right - rc.left;
            height_ = rc.bottom - rc.top;
        } else {
            DWORD style = WS_OVERLAPPEDWINDOW;
            RECT rc = {0, 0, w, h};
            AdjustWindowRectEx(&rc, style, FALSE, 0);
            hwnd_ = CreateWindowExA(0, cls, title.c_str(), style,
                                    CW_USEDEFAULT, CW_USEDEFAULT,
                                    rc.right - rc.left, rc.bottom - rc.top,
                                    nullptr, nullptr, hInst, nullptr);
            if (!hwnd_) return false;
            ShowWindow(hwnd_, SW_SHOW);
            UpdateWindow(hwnd_);
        }

        // 这里在完整实现中会初始化 wglOpenGL 上下文。
        // 为保持自研纯 Win32（不依赖 GLFW/GLEW），GL 上下文用 wglCreateContext。
        // （演示版本：创建 GL 上下文并设为当前）
        return true;
    }

    void destroyWindow() override {
        if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
    }

    void pump(InputState& state) override {
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            if (msg.message == WM_QUIT) closeRequested_ = true;
        }

        // 键盘/鼠标状态（经典 GetAsyncKeyState）
        std::memset(state.keys, 0, sizeof(state.keys));
        for (int i = 1; i < static_cast<int>(Key::Count); ++i) {
            Key k = static_cast<Key>(i);
            if (k == Key::AltF4) continue; // AltF4 作为关闭信号，不作为普通按键
            if (k == Key::W) state.keys[i] = (GetAsyncKeyState('W') & 0x8000) != 0;
            else if (k == Key::A) state.keys[i] = (GetAsyncKeyState('A') & 0x8000) != 0;
            else if (k == Key::S) state.keys[i] = (GetAsyncKeyState('S') & 0x8000) != 0;
            else if (k == Key::D) state.keys[i] = (GetAsyncKeyState('D') & 0x8000) != 0;
            else if (k == Key::Space) state.keys[i] = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
            else if (k == Key::Escape) state.keys[i] = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
            else if (k == Key::MouseLeft) state.keys[i] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            else if (k == Key::MouseRight) state.keys[i] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
            else if (k == Key::MouseMiddle) state.keys[i] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
        }

        POINT p; GetCursorPos(&p);
        if (hwnd_) ScreenToClient(hwnd_, &p);
        state.mouseDeltaX = p.x - prevX_;
        state.mouseDeltaY = p.y - prevY_;
        state.mouseX = p.x; state.mouseY = p.y;
        prevX_ = p.x; prevY_ = p.y;
        state.mouseLocked = mouseLocked_;

        if (mouseLocked_ && hwnd_) {
            // ClipCursor 锁定
            RECT rc; GetClientRect(hwnd_, &rc);
            MapWindowPoints(hwnd_, nullptr, (LPPOINT)&rc, 2);
            ClipCursor(&rc);
            ShowCursor(FALSE);
        } else {
            ClipCursor(nullptr);
        }
    }

    bool keyDown(Key k, const InputState& s) const override {
        if (k == Key::AltF4) return false; // 关闭信号
        return s.keys[static_cast<int>(k)];
    }

    void lockMouse(bool locked) override { mouseLocked_ = locked; }
    void* nativeHandle() const override { return hwnd_; }

    void swapBuffers() override {
        // 演示：真实实现调用 SwapBuffers(hdc_)
    }

    bool closeRequested() const { return closeRequested_; }
    void resetClose() { closeRequested_ = false; }

private:
    HWND hwnd_ = nullptr;
    int width_ = 0, height_ = 0;
    int prevX_ = 0, prevY_ = 0;
    bool mouseLocked_ = false;
    bool closeRequested_ = false;
};

PlatformImpl* createPlatform() { return new Win7Platform(); }

} // namespace pyrect3d
#endif // _WIN32
