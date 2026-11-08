// ============================================================
//  PyRecT3D/implicit.h — 隐式 API（Implicit API / Quick API）
//
//  适用场景：小游戏、教学示例、Scratch 式快速入门。
//  一行启动，单窗口自动弱绑定：窗口 → 子消息队列、UI → 窗口、播放器 → 子队列
//  全部由引擎自动完成，调用者不需要关心内存归属与队列挂载。
// ============================================================
#pragma once
#include "export.h"
#include "core.h"
#include "renderer.h"
#include "gui.h"
#include "video.h"
#include "collision.h"

namespace pyrect3d {

// ---------- 实体句柄（隐式 API 用 id 而非裸指针） ----------
struct PYRECT3D_API EntityHandle {
    std::int64_t id = -1;
    bool valid() const { return id >= 0; }
};

namespace quick {

// 一行启动：创建单窗口 + 3D 场景 + 自动弱绑定子消息队列，阻塞运行
PYRECT3D_API int run(int width = 1280, int height = 720,
                     const std::string& title = "PyRecT3D");

// 带初始化回调的一行启动
PYRECT3D_API int runSetup(int width, int height, const std::string& title,
                          void (*setup)());

// ---------- 对象创建（隐式：自动加入当前唯一场景） ----------
PYRECT3D_API EntityHandle cube(float x = 0.f, float y = 0.f, float z = -5.f,
                               float sx = 1.f, float sy = 1.f, float sz = 1.f);
PYRECT3D_API EntityHandle screenCube(float x, float y, float w, float h);

// 克隆：复制当前场上已有实体的完整状态（位置/旋转/缩放），区别于 summon
PYRECT3D_API EntityHandle clone(EntityHandle src);
// 召唤：按给定初始参数新建实体
PYRECT3D_API EntityHandle summon(EntityHandle prototype, float x, float y, float z);

// ---------- 变换（内部转为消息，由主消息队列身份发布） ----------
PYRECT3D_API void move(EntityHandle e, float x, float y, float z);
PYRECT3D_API void rotate(EntityHandle e, float rx, float ry, float rz);
PYRECT3D_API void scale(EntityHandle e, float sx, float sy, float sz);
PYRECT3D_API void remove(EntityHandle e);

// ---------- 射线 ----------
PYRECT3D_API HitResult raycast(float ox, float oy, float oz,
                               float dx, float dy, float dz,
                               float maxDistance = 100.f);

// ---------- UI（自动绑定唯一窗口） ----------
PYRECT3D_API Button* button(float x, float y, float w, float h,
                            const std::string& text);
PYRECT3D_API Text* text(float x, float y, const std::string& content);

// ---------- 视频 ----------
PYRECT3D_API VideoPlayer* video(const std::string& path, bool loop = false);

// ---------- 当前窗口 / 场景 ----------
PYRECT3D_API Scene& scene();
PYRECT3D_API Gui& gui();

} // namespace quick
} // namespace pyrect3d
