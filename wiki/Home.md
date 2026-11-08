# PyRecT3D Wiki

**轻量 C++ 2D/3D 渲染引擎** — OpenGL 3.3 Core ／ Android + Web 走 OpenGL ES 3.0（WebGL2）。
版本 0.2.0 · GPL-3.0 · 支持 Windows（Win7+）/ Linux（X11）/ Android / Web(WASM)

---

## 从这里开始

| 我想…… | 看这一页 |
|--------|----------|
| 5 分钟跑起来一个方块 | [[Quick Start]] |
| 搞清"显式 API"和"隐式 API"该用哪个 | [[Explicit vs Implicit]] |
| 查某个函数怎么调 | [[API Reference]]（或仓库内 `docs/API.md` / `API.docx`） |
| 理解消息队列与优先级 | [[Event Model]] |
| 编译 DLL / LIB，裁剪模块 | [[Build]] |
| 发到网页（小游戏平台 / 抖音 / 小红书） | [[WASM Packaging]] |
| 上安卓 | [[Android]] |
| 避免崩溃与显存泄漏 | [[Hard Constraints]] |

---

## 产品由两部分组成

**1）显式 API（Explicit API）** — `#include <PyRecT3D/explicit.h>`

多窗口实例，或需要详细配置（指定渲染后端、按需裁剪模块、手动挂载子消息队列、手动绑定 UI 与播放器）时使用。
一切由调用者决定：窗口逐个创建、UI 逐个绑定、播放器逐个绑定。

**2）隐式 API（Implicit API / Quick API）** — `#include <PyRecT3D/implicit.h>`

最快的方式入门：`quick::run()` 一行启动，单窗口自动弱绑定（窗口→子队列、UI→窗口、播放器→子队列全自动）。

```cpp
#include <PyRecT3D/implicit.h>
using namespace pyrect3d;

int main() {
    quick::runSetup(1280, 720, "Quick Demo", [] {
        EntityHandle a = quick::cube(0, 0, -5);
        EntityHandle c = quick::clone(a);   // 克隆场上现成状态（Scratch 思维）
        quick::move(c, -2, 0, -5);
        quick::button(20, 20, 140, 40, "复位")->onClick = [a] { quick::move(a, 0, 0, -5); };
    });
}
```

---

## 核心特性

- **双消息队列**：主队列（全局单例，绑定渲染主线程）+ 子队列（每窗口一个）
- **六级事件优先级**：窗口输入 > 物理 > 碰撞 > 音效 > 媒体 > 普通
- **渲染指令合并 + 脏区域局部更新**：3D 视口不重绘时只叠加新 UI（Docker 式软更新）
- **双碰撞箱**：物理碰撞箱（射线）+ 模型碰撞箱（内部小型物理空间、肢体级）
- **9 种交互事件**：进入/退出/停留/按下/松开/拖动/拖出/区域外松开
- **克隆 + 召唤双内置**：`clone`（复制场上状态）与 `summon`（按模板新建）
- **视频即材质**：FFmpeg 子线程解码，解码帧直接变成动态材质贴到任意物体
- **跨平台 API 一致**：桌面与 Web 对外接口完全相同

---

## 目录导航（仓库）

| 路径 | 内容 |
|------|------|
| `cpp/include/PyRecT3D/` | 对外契约头文件（只有声明） |
| `cpp/src/` | 内核实现（core / renderer / physics / media / gui / io / audio / math / binding） |
| `docs/API.md` · `docs/API.docx` | 权威接口文档（含 AI 调用模板） |
| `WEB/` | WASM 单包 / 多包 + JS 桥接层 |
| `Android/apk/` | APK 占位契约（Manifest / Gradle / JNI） |
| `examples_cpp/` | basic_cube / explicit_demo / implicit_demo |
| `tests/smoke_v2.cpp` | 无头冒烟测试（42 项） |

---

## 明确边界

- 不内置游戏逻辑，只提供引擎能力
- 仅软件光追，无硬件光追（Vulkan 后端留作未来可选）
- GUI 极简：不含表格 / 树 / 标签页 / Dock / 动画 / 主题
- **不支持 Apple 平台**（不引入 Xcode / Objective-C 工具链）

---

*PyRecT3D v0.2 · GPL-3.0*
