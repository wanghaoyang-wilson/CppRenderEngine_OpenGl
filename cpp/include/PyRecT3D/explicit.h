// ============================================================
//  PyRecT3D/explicit.h — 显式 API（Explicit API）
//
//  适用场景：多窗口实例、需要详细配置（自选渲染后端 / 模块开关 /
//  消息队列挂载 / UI 与播放器手动绑定）的工程。
//
//  显式 API 的哲学：一切由调用者决定。
//   - 每个窗口实例显式创建，各自带一个子消息队列；
//   - UI 实例必须手动创建并绑定到目标窗口（多窗口时强制）；
//   - 播放器实例必须手动绑定子消息队列（多窗口时强制）；
//   - 窗口级输入事件由主消息队列统一接收，再按窗口路由。
// ============================================================
#pragma once
#include "export.h"
#include "core.h"
#include "message.h"
#include "renderer.h"
#include "gui.h"
#include "video.h"
#include "collision.h"
#include <cstdint>
#include <string>

namespace pyrect3d {

// 前置声明
class WindowInstance;

// ---------- 渲染后端选择 ----------
enum class PYRECT3D_API Backend : int {
    Auto   = 0,   // 按平台自动选择：桌面→GL33，Android/Web→GLES3
    GL33   = 1,   // 桌面 OpenGL 3.3 Core（Windows / Linux）
    GLES3  = 2,   // OpenGL ES 3.0（Android / WebGL2）
};

// ---------- 编译期可裁剪模块 ----------
// 与 build.bat / CMake 的模块裁剪选项对应；未编译进来的模块调用返回 false
enum class PYRECT3D_API Module : int {
    Renderer = 1 << 0,
    GUI      = 1 << 1,
    Video    = 1 << 2,
    Audio    = 1 << 3,
    Physics  = 1 << 4,
    RayTracer= 1 << 5,
    Python   = 1 << 6,
};

// ---------- 应用配置 ----------
struct PYRECT3D_API AppConfig {
    int   width  = 1280;
    int   height = 720;
    std::string title = "PyRecT3D";
    Backend backend = Backend::Auto;
    bool  vsync = true;
    bool  enableVideo  = true;   // 模块裁剪构建里若未编译，会被忽略
    bool  enableAudio  = true;
    bool  enablePhysics = true;
    bool  enableGUILocalUpdate = true;  // 2D/3D 分离局部更新（脏区域）
    bool  multithreadRender = false;    // true = 子线程生产渲染指令，主线程消费
    float renderScale = 1.0f;
};

// ---------- 窗口描述 ----------
struct PYRECT3D_API WindowDesc {
    int   width  = 1280;
    int   height = 720;
    std::string title = "Window";
    bool  resizable = true;
    bool  fullscreen = false;
    bool  uiAutoBind = false;    // 单窗口可置 true 走弱绑定
    void* nativeHandle = nullptr; // 外部接管（如 QT 启动器 / Android Surface）
};

// ---------- UI 实例 ----------
class PYRECT3D_API UIInstance {
public:
    UIInstance();
    ~UIInstance();

    // 绑定目标窗口实例；单窗口环境可省略（弱绑定）
    bool bindWindow(WindowInstance* window);
    std::int64_t boundWindowId() const;

    Gui& gui();

    // 组件便捷挂载
    Button* addButton(const Rect& r, const std::string& text);
    Text*   addText(const Rect& r, const std::string& text);
    InputBox* addInputBox(const Rect& r, const std::string& text);
    Container* addContainer(const Rect& r);
    ScrollContainer* addScrollContainer(const Rect& r);

    // 提交本 UI 层的绘制（渲染指令合并后一次提交）
    void drawAll();

private:
    struct Impl;
    Impl* impl_;
};

// ---------- 窗口实例 ----------
class PYRECT3D_API WindowInstance {
public:
    std::int64_t id() const;
    MessageQueue* queue();      // 窗口自带子消息队列
    Entity*       camera();     // 相机实体

    // 3D 场景
    Scene& scene();

    // 创建并绑定 UI（多窗口下必须显式调用并 bindWindow）
    UIInstance* createUI();

    // 提交 3D 帧（保留帧缓冲，供 2D UI 叠加上去，实现“只更新 UI”）
    void renderScene();
    // 用当前 3D 帧缓冲 + 新 UI 指令合成上屏
    void present();

    void setTitle(const std::string& title);
    int  width() const;
    int  height() const;

private:
    friend class ExplicitApp;
    explicit WindowInstance(std::int64_t id);
    ~WindowInstance();
    struct Impl;
    Impl* impl_;
};

// ---------- 显式应用入口 ----------
class PYRECT3D_API ExplicitApp {
public:
    ExplicitApp();
    ~ExplicitApp();

    // 全局配置（必须在 createWindow 之前调用）
    bool configure(const AppConfig& cfg);

    // 创建窗口实例（同时分配子消息队列）
    WindowInstance* createWindow(const WindowDesc& desc);

    // 取已创建的窗口
    WindowInstance* window(std::int64_t id);
    std::size_t windowCount() const;

    // 把所有窗口的子消息队列挂载到主队列；多窗口时由 createWindow 后自动要求调用
    bool mountQueues();

    // 创建播放器实例并（多窗口时）显式绑定到窗口子队列
    VideoPlayer* createVideoPlayer(const VideoDesc& desc, WindowInstance* bindTo = nullptr);

    // 全局事件发布（窗口级输入 / 程序级 / 全局场景操作走这里）
    std::uint64_t publishGlobal(const Message& m);

    // 运行 / 单步 / 退出
    int  run();
    bool pump();
    void quit();

    // 查询某模块是否被编入当前构建
    bool hasModule(Module m) const;

private:
    struct Impl;
    Impl* impl_;
};
} // namespace pyrect3d
