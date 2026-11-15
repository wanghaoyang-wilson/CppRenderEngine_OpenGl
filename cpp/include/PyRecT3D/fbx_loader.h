// ============================================================
//  PyRecT3D/fbx_loader.h — FBX 加载契约层
//
//  0.2.1 边界：
//  - 识别 ASCII FBX 文本段，导入静态网格、UV、法线、基础材质色。
//  - 保留骨骼/动画占位结构，完整二进制 FBX、蒙皮变形器与动画状态机后续补。
//  - 不链接 Autodesk FBX SDK，避免 500MB 级二进制依赖。
// ============================================================
#pragma once

#include "export.h"
#include "io.h"
#include "model_animation.h"
#include <string>

namespace pyrect3d {

struct PYRECT3D_API FbxModel {
    ObjModel mesh;
    AnimatedModel animation;
    bool binary = false;
    bool valid = false;
    std::string warning;
};

class PYRECT3D_API FbxLoader {
public:
    static bool load(const std::string& path, FbxModel& out);
    static bool loadFromMemory(const std::string& content, FbxModel& out);
    static bool isBinaryFbx(const std::string& path);
};

} // namespace pyrect3d
