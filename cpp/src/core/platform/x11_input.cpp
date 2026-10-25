// ============================================================
//  PyRecT3D — x11_input.cpp
//  Linux 平台实现（基于 X11，不默认 Wayland）
//
//  关键点：
//  - X11 窗口 + XOpenDisplay 消息循环
//  - 鼠标锁定：XGrabPointer + XWarpPointer
//  - Alt+F4 / 窗口关闭 → WM_DELETE_WINDOW
//  - GL 上下文：GLX
// ============================================================
#if defined(__linux__) && !defined(_WIN32)
#include "platform_private.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <cstring>

namespace pyrect3d {

class X11Platform : public PlatformImpl {
public:
    X11Platform() = default;
    ~X11Platform() override { destroyWindow(); }

    bool createWindow(int w, int h, const std::string& title, void* nativeWnd) override {
        dpy_ = XOpenDisplay(nullptr);
        if (!dpy_) return false;
        screen_ = DefaultScreen(dpy_);

        Window root = RootWindow(dpy_, screen_);
        // 简单 visual 选择（GLX 完整实现需查询 FBConfig）
        XVisualInfo vinfo = {};
        if (!XMatchVisualInfo(dpy_, screen_, 24, TrueColor, &vinfo)) {
            XCloseDisplay(dpy_); dpy_ = nullptr; return false;
        }

        XSetWindowAttributes attrs = {};
        attrs.colormap = XCreateColormap(dpy_, root, vinfo.visual, AllocNone);
        attrs.border_pixel = 0;
        attrs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                           ButtonPressMask | ButtonReleaseMask |
                           PointerMotionMask | StructureNotifyMask;

        if (nativeWnd) {
            win_ = (Window)nativeWnd;
        } else {
            win_ = XCreateWindow(dpy_, root, 0, 0, w, h, 0,
                                 vinfo.depth, InputOutput, vinfo.visual,
                                 CWColormap | CWEventMask, &attrs);
            XStoreName(dpy_, win_, title.c_str());
            XMapWindow(dpy_, win_);
        }

        // 窗口关闭协议 (Alt+F4 / WM_DELETE_WINDOW)
        Atom wmDelete = XInternAtom(dpy_, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(dpy_, win_, &wmDelete, 1);

        // 键盘映射
        keycodeW_ = XKeysymToKeycode(dpy_, XK_w);
        keycodeA_ = XKeysymToKeycode(dpy_, XK_a);
        keycodeS_ = XKeysymToKeycode(dpy_, XK_s);
        keycodeD_ = XKeysymToKeycode(dpy_, XK_d);
        keycodeSpace_ = XKeysymToKeycode(dpy_, XK_space);
        keycodeEsc_ = XKeysymToKeycode(dpy_, XK_Escape);
        return true;
    }

    void destroyWindow() override {
        if (win_) { XDestroyWindow(dpy_, win_); win_ = 0; }
        if (dpy_) { XCloseDisplay(dpy_); dpy_ = nullptr; }
    }

    void pump(InputState& state) override {
        if (!dpy_) return;
        while (XPending(dpy_)) {
            XEvent ev;
            XNextEvent(dpy_, &ev);
            switch (ev.type) {
                case KeyPress: {
                    KeyCode kc = ev.xkey.keycode;
                    if (kc == keycodeW_) state.keys[(int)Key::W] = true;
                    else if (kc == keycodeA_) state.keys[(int)Key::A] = true;
                    else if (kc == keycodeS_) state.keys[(int)Key::S] = true;
                    else if (kc == keycodeD_) state.keys[(int)Key::D] = true;
                    else if (kc == keycodeSpace_) state.keys[(int)Key::Space] = true;
                    else if (kc == keycodeEsc_) state.keys[(int)Key::Escape] = true;
                    break;
                }
                case KeyRelease:
                    state.keys[(int)Key::W] = false;
                    state.keys[(int)Key::A] = false;
                    state.keys[(int)Key::S] = false;
                    state.keys[(int)Key::D] = false;
                    state.keys[(int)Key::Space] = false;
                    state.keys[(int)Key::Escape] = false;
                    break;
                case ButtonPress:
                    if (ev.xbutton.button == 1) state.keys[(int)Key::MouseLeft] = true;
                    else if (ev.xbutton.button == 3) state.keys[(int)Key::MouseRight] = true;
                    else if (ev.xbutton.button == 2) state.keys[(int)Key::MouseMiddle] = true;
                    break;
                case ButtonRelease:
                    if (ev.xbutton.button == 1) state.keys[(int)Key::MouseLeft] = false;
                    else if (ev.xbutton.button == 3) state.keys[(int)Key::MouseRight] = false;
                    else if (ev.xbutton.button == 2) state.keys[(int)Key::MouseMiddle] = false;
                    break;
                case MotionNotify:
                    state.mouseX = ev.xmotion.x;
                    state.mouseY = ev.xmotion.y;
                    break;
                case ClientMessage: {
                    Atom wmDelete = XInternAtom(dpy_, "WM_DELETE_WINDOW", False);
                    if ((Atom)ev.xclient.data.l[0] == wmDelete) closeRequested_ = true;
                    break;
                }
            }
        }
        state.mouseLocked = mouseLocked_;
        if (mouseLocked_ && win_) {
            // XGrabPointer 锁定鼠标
            XGrabPointer(dpy_, win_, True, 0, GrabModeAsync, GrabModeAsync,
                         win_, None, CurrentTime);
        } else if (win_) {
            XUngrabPointer(dpy_, CurrentTime);
        }
    }

    bool keyDown(Key k, const InputState& s) const override {
        if (k == Key::AltF4) return false;
        return s.keys[(int)k];
    }

    void lockMouse(bool locked) override { mouseLocked_ = locked; }
    void* nativeHandle() const override { return (void*)win_; }

    void swapBuffers() override {
        // GLX 完整实现调用 glXSwapBuffers(dpy_, win_)
    }

    bool closeRequested() const { return closeRequested_; }
    void resetClose() { closeRequested_ = false; }

private:
    Display* dpy_ = nullptr;
    Window win_ = 0;
    int screen_ = 0;
    bool mouseLocked_ = false;
    bool closeRequested_ = false;
    KeyCode keycodeW_ = 0, keycodeA_ = 0, keycodeS_ = 0, keycodeD_ = 0;
    KeyCode keycodeSpace_ = 0, keycodeEsc_ = 0;
};

PlatformImpl* createPlatform() { return new X11Platform(); }

} // namespace pyrect3d
#endif // __linux__
