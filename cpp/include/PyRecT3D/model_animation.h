// ============================================================
//  PyRecT3D/model_animation.h — 模型动画契约层
//  0.2.1：基础骨骼动画骨架；变形器/状态机/混合蒙皮留待后续版本。
// ============================================================
#pragma once

#include "export.h"
#include "math.h"
#include <string>
#include <vector>

namespace pyrect3d {

// 单关节静态绑定信息。0.2.1 只保存层级、绑定矩阵与本地变换。
struct PYRECT3D_API Bone {
    std::string name;
    int parent = -1;
    Mat4 localBind;
    Mat4 worldBind;
    Vec3 bindPosition{0.f, 0.f, 0.f};
    Vec3 bindRotation{0.f, 0.f, 0.f}; // 欧拉角，单位：度
    Vec3 bindScale{1.f, 1.f, 1.f};
};

// 关键帧曲线类型。
enum class PYRECT3D_API AnimationCurveType : int {
    Translation = 0,
    RotationEuler = 1,
    Scale = 2
};

struct PYRECT3D_API AnimationKeyframe {
    float time = 0.f;
    Vec3 value;
};

struct PYRECT3D_API AnimationCurve {
    std::string boneName;
    AnimationCurveType type = AnimationCurveType::Translation;
    std::vector<AnimationKeyframe> keys;
};

struct PYRECT3D_API AnimationClip {
    std::string name;
    float duration = 0.f;
    float framesPerSecond = 24.f;
    std::vector<AnimationCurve> curves;
};

// 已加载模型中的骨骼与动画数据。
struct PYRECT3D_API AnimatedModel {
    std::string name;
    std::vector<Bone> bones;
    std::vector<AnimationClip> clips;

    std::size_t boneCount() const;
    std::size_t clipCount() const;
    bool hasAnimation() const;
};

} // namespace pyrect3d
