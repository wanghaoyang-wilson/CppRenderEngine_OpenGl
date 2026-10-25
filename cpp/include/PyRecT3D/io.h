// ============================================================
//  PyRecT3D/io.h — 模型 / 材质加载（契约层，仅声明）
// ============================================================
#pragma once
#include "export.h"
#include "math.h"
#include <string>
#include <vector>

namespace pyrect3d {

// 网格数据（顶点、索引）
struct PYRECT3D_API MeshData {
    std::vector<float> positions;   // xyz 平铺
    std::vector<float> normals;     // xyz 平铺
    std::vector<float> uvs;         // uv 平铺
    std::vector<unsigned int> indices;
};

// 材质（对应 .mtl）
struct PYRECT3D_API Material {
    std::string name;
    std::string diffuseTex;      // map_Kd 路径
    Vec3 diffuseColor{0.8f, 0.8f, 0.8f};
    Vec3 specularColor{1,1,1};
    float shininess = 32.f;
    bool hasTexture = false;
};

// OBJ 模型（含关联材质）
struct PYRECT3D_API ObjModel {
    MeshData mesh;
    std::vector<Material> materials;
    std::string name;
};

class PYRECT3D_API ObjLoader {
public:
    // 从 .obj 文件加载，可附带同名 .mtl
    static bool load(const std::string& objPath, ObjModel& out);
    // 从内存字符串加载
    static bool loadFromMemory(const std::string& objContent, ObjModel& out);
};

} // namespace pyrect3d
