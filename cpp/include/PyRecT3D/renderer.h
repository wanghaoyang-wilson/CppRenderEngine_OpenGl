// ============================================================
//  PyRecT3D/renderer.h — 渲染 / 图元 / 软光追 / 帧导出（契约层，仅声明）
// ============================================================
#pragma once
#include "export.h"
#include "math.h"
#include "core.h"
#include <cstdint>
#include <string>
#include <vector>

namespace pyrect3d {

// 渲染模式
enum class PYRECT3D_API RenderMode : int { Forward, Wireframe };

// 渲染器：2D+3D 统一
class PYRECT3D_API Renderer {
public:
    Renderer();
    ~Renderer();

    bool init(int width, int height);
    void shutdown();

    void setMode(RenderMode mode);
    RenderMode mode() const;

    // 清屏颜色
    void setClearColor(float r, float g, float b, float a = 1.f);

    // 3D 实体（由图元填充顶点后调用）
    void drawEntity(const Entity& e);

    // 2D 屏幕方块
    void drawScreenRect(float x, float y, float w, float h);

    // 抗锯齿开关
    void setAntiAliasing(bool on);

    // 帧导出
    bool saveFrame(const std::string& path);

private:
    struct Impl;
    Impl* impl_;
};

// ---------- 基础图元 ----------
class PYRECT3D_API Cube : public Entity {
public:
    Cube(const Vec3& position = Vec3(), const Vec3& scale = Vec3(1,1,1));
    ~Cube() override;
};

class PYRECT3D_API ScreenCube : public Entity {
public:
    ScreenCube(float x, float y, float w, float h);
    ~ScreenCube() override;
    float x() const, y() const, width() const, height() const;
};

// ---------- 软件光追 ----------
class PYRECT3D_API SoftRayTracer {
public:
    SoftRayTracer();
    ~SoftRayTracer();

    // 渲染一帧到输出缓冲 (w*h*3 RGB)
    bool render(const Scene& scene, int w, int h,
                std::vector<unsigned char>& outRGB);
};

} // namespace pyrect3d
