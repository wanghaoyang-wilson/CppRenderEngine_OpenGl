// ============================================================
//  PyRecT3D — cpp/src/renderer/gl_backend.h
//  OpenGL 3.3 Core / GLES 3.0 硬后端（内部头文件，不对外暴露）
//
//  设计原则（与引擎"给一块画布、其余交给你"的思路一致）：
//    上层只说"画什么"，不出现任何平台/后端专属接口调用；
//    这里把 glad 的入口、VAO/VBO 生命周期、极简 shader 全部封死在内部。
//
//  编译开关：PYRECT3D_WITH_GL
//    - 未开启：renderer.cpp 走软件光栅化回退（默认，便于无 GPU 环境跑测试）
//    - 开启：  渲染器尝试真实 GL；gladLoadGL() 失败时自动回落软件路径
// ============================================================
#pragma once

#include <string>

namespace pyrect3d {
namespace gl {

// 加载 GL 入口并建立渲染资源（需已有当前上下文）
bool init(int width, int height);
void shutdown();

// 是否处于真实 GL 路径（未加载成功则上层自动走软件路径）
bool available();

// 帧边界（对应 Renderer::beginFrame / endFrame）
void beginFrame(float r, float g, float b, float a);
void endFrame();

// 画一个单位立方体：model 为 4x4 列主序浮点数组，viewProj 同理
void drawCube(const float* model4x4, const float* viewProj4x4, bool wireframe);

// 2D 矩形（屏幕坐标，原点左下），用于 UI 合批
void drawScreenRect(float x, float y, float w, float h,
                    float r, float g, float b, float a);

// 视口尺寸变化
void resize(int width, int height);

// 后端信息（排查驱动问题时打印）
std::string versionString();
std::string rendererString();

}  // namespace gl
}  // namespace pyrect3d
