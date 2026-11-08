# Explicit vs Implicit（显式 API 与隐式 API）

PyRecT3D 对外有**两套平行 API**，共用同一个内核（同一份渲染 / 消息 / 碰撞 / 媒体实现）。
选错了会写出很别扭的代码，先看这张表。

## 一眼判据

```
只有一个窗口 ── 需要精细配置？ ── 否 → 隐式 API（quick::）
                   │
                   └── 是 → 显式 API（单窗口也能用）
有多个窗口 ──────────────────────── → 显式 API（强制）
```

## 对比表

| 事项 | 显式 API | 隐式 API |
|------|----------|----------|
| 头文件 | `<PyRecT3D/explicit.h>` | `<PyRecT3D/implicit.h>` |
| 启动 | `ExplicitApp::run()` | `quick::run()` / `quick::runSetup()` |
| 窗口 | 逐个 `createWindow()` | 自动创建唯一窗口 |
| 子消息队列 | 每窗口一个；多窗口**必须** `mountQueues()` | 自动挂载 |
| UI | `createUI()` + `bindWindow()` 显式绑定 | 自动绑定唯一窗口 |
| 播放器 | `createVideoPlayer(desc, win)` 显式绑定 | `quick::video()` 自动绑定 |
| 对象身份 | `Entity*` / `std::int64_t id` | `EntityHandle` |
| 全局事件 | `publishGlobal(Message)` | 内部自动以主队列身份发布 |
| 适用 | 多窗口、深度定制、发行产品 | 小游戏、教学、Scratch 式原型 |

## 显式 API：多窗口完整示例

```cpp
#include <PyRecT3D/explicit.h>
using namespace pyrect3d;

int main() {
    ExplicitApp app;

    AppConfig cfg;
    cfg.backend = Backend::Auto;              // 桌面→GL33，Android/Web→GLES3
    cfg.enableGUILocalUpdate = true;          // 2D/3D 分离局部更新
    cfg.multithreadRender = false;            // 渲染指令队列（按需开）
    app.configure(cfg);

    WindowDesc da; da.title = "A"; da.width = 800; da.height = 600;
    WindowDesc db; db.title = "B"; db.width = 640; db.height = 480;
    WindowInstance* wa = app.createWindow(da);
    WindowInstance* wb = app.createWindow(db);

    app.mountQueues();                        // ← 多窗口必须

    UIInstance* uiA = wa->createUI();
    uiA->bindWindow(wa);                      // ← 多窗口必须显式绑定
    uiA->addButton(Rect{ 20, 20, 160, 44 }, "退出")->onClick = [] { App::quit(); };

    VideoDesc vd; vd.path = "assets/demo.mp4"; vd.loop = true; vd.autoPlay = true;
    app.createVideoPlayer(vd, wb);             // ← 显式绑定到 B 窗口子队列

    return app.run();
}
```

## 隐式 API：一行启动

```cpp
#include <PyRecT3D/implicit.h>
using namespace pyrect3d;

int main() {
    quick::runSetup(1280, 720, "Quick Demo", [] {
        EntityHandle a = quick::cube(0, 0, -5);
        EntityHandle c = quick::clone(a);      // 克隆场上现成状态
        quick::move(c, -2, 0, -5);
        quick::button(20, 20, 140, 40, "复位")->onClick = [a] { quick::move(a, 0, 0, -5); };
        quick::video("assets/demo.mp4", true);
    });
}
```

## 弱绑定规则（关键）

| 组件 | 单窗口 | 多窗口 |
|------|--------|--------|
| 子消息队列 | 自动挂载到主队列 | **必须** `mountQueues()` |
| UI 实例 | 可省略 `bindWindow`（弱绑定） | **必须** `bindWindow` 到具体窗口 |
| 视频播放器 | `autoBind()` / `quick::video()` 自动绑 | **必须** `bindQueue` / 指定窗口 |
| 全局播放器管理器 | 进程唯一，自动绑定主线程与主队列 | 同 |

> 隐式 API 在出现第二个窗口时失效——这是刻意设计：多窗口必须走显式 API，避免隐式上下文产生歧义。

## 迁移方向

- 隐式 → 显式：把 `quick::` 调用换成 `ExplicitApp`/`WindowInstance` 上的显式调用，对象 id 从 `EntityHandle` 换成 `std::int64_t`（`handle.id` 即同一 id）。
- 显式 → 隐式：只在单窗口且不需要模块/后端定制时做减法，一般不建议反向迁移。

---

返回 [[Home]] · 完整签名见 [[API Reference]]
