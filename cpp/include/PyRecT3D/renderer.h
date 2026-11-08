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

    // v0.2 帧边界：beginFrame 清屏一次，之后多次 drawEntity 累积，
    // endFrame 上屏。解决了旧版每次 drawEntity 都清屏导致只显示最后一个实体的问题。
    void beginFrame();
    void endFrame();

    int width() const;
    int height() const;

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

// ============================================================
//  v0.2 新增：渲染指令合并 / 脏区域局部更新（Docker 式软更新）
//
//  目的：2D UI 需要更新时，不重绘 3D 视口 —— 保留旧 3D 帧缓冲
//        直接把新的 UI 绘制指令合批叠加，再上屏。
//  同时支持在合成层（DWM 类比）上做 3D 特效（如眩晕），
//  不再局限于 2D 特效。
// ============================================================
enum class PYRECT3D_API CommandType : int {
    Clear,
    DrawRect2D,
    DrawEntity3D,
    SetUniform,
    DrawTexture,
    Composite,
    Present,
};

struct PYRECT3D_API RenderCommand {
    CommandType type = CommandType::DrawRect2D;
    float x = 0, y = 0, w = 0, h = 0;
    float r = 1, g = 1, b = 1, a = 1;
    std::int64_t entityId = -1;
    std::int64_t textureId = -1;
    bool is2D() const { return type == CommandType::DrawRect2D || type == CommandType::DrawTexture; }
};

// 脏区域（只标记发生变化的屏幕范围）
struct PYRECT3D_API DirtyRegion {
    int x = 0, y = 0, w = 0, h = 0;
    bool valid = false;
    void unionWith(const DirtyRegion& o);
};

// 合成层：2D UI 与 3D 视口分层合成
class PYRECT3D_API Compositor {
public:
    Compositor();
    ~Compositor();

    // 绑定渲染器
    void attach(Renderer* renderer);

    // 提交一批渲染指令（会做合批合并，相同状态只设置一次）
    void submit(const RenderCommand* cmds, std::size_t count);

    // 标记区域脏 → 只重绘该区域
    void invalidate(const DirtyRegion& region);
    DirtyRegion dirtyRegion() const;
    void clearDirty();

    // 3D 视口是否需要在下一帧重绘（false 表示复用旧帧缓冲）
    void setSceneDirty(bool dirty);
    bool sceneDirty() const;

    // 在合成层上叠加 3D 特效（眩晕等），不需要重绘整个 3D 场景
    void applyEffect3D(std::int32_t effectCode, float strength);

    // 合成上屏
    void present();

    // 统计：本帧实际合并了多少条绘制指令
    std::size_t mergedCount() const;

private:
    struct Impl;
    Impl* impl_;
};

// ---------- 软件光追 ----------
class PYRECT3D_API SoftRayTracer {
public:
    SoftRayTracer();
    ~SoftRayTracer();

    // 渲染一帧到输出缓冲 (w*h*3 RGB)
    bool render(const Scene& scene, int w, int h,
                std::vector<unsigned char>& outRGB);

private:
    struct Impl;
    Impl* impl_;
};

} // namespace pyrect3d
