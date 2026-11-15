// ============================================================
//  PyRecT3D — model_animation.cpp
//  基础骨骼动画数据结构实现；不依赖外部 FBX/SDK。
// ============================================================
#include <PyRecT3D/model_animation.h>

namespace pyrect3d {

std::size_t AnimatedModel::boneCount() const {
    return bones.size();
}

std::size_t AnimatedModel::clipCount() const {
    return clips.size();
}

bool AnimatedModel::hasAnimation() const {
    return !clips.empty();
}

} // namespace pyrect3d
