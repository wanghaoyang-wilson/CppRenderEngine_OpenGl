// ============================================================
//  PyRecT3D — obj_loader.cpp
//  OBJ 模型加载 + 材质(MTL) 加载
// ============================================================
#include <PyRecT3D/io.h>
#include <fstream>
#include <sstream>
#include <cstring>

namespace pyrect3d {

// 简单的 OBJ 解析：支持 v / vn / vt / f / usemtl / mtllib
bool ObjLoader::load(const std::string& objPath, ObjModel& out) {
    std::ifstream f(objPath);
    if (!f) return false;
    std::stringstream ss;
    ss << f.rdbuf();
    return loadFromMemory(ss.str(), out);
}

bool ObjLoader::loadFromMemory(const std::string& objContent, ObjModel& out) {
    out = ObjModel();
    std::istringstream ss(objContent);
    std::string line;
    std::vector<Vec3> positions, normals;
    std::vector<Vec2> uvs;
    std::vector<unsigned int> posIdx, norIdx, uvIdx;

    Material cur;
    bool haveCur = false;

    auto commitMesh = [&]() {
        if (posIdx.empty()) return;
        for (size_t i = 0; i < posIdx.size(); ++i) {
            Vec3 p = positions[posIdx[i]];
            out.mesh.positions.push_back(p.x);
            out.mesh.positions.push_back(p.y);
            out.mesh.positions.push_back(p.z);
            if (i < norIdx.size() && norIdx[i] < normals.size()) {
                Vec3 n = normals[norIdx[i]];
                out.mesh.normals.push_back(n.x); out.mesh.normals.push_back(n.y); out.mesh.normals.push_back(n.z);
            }
            if (i < uvIdx.size() && uvIdx[i] < uvs.size()) {
                Vec2 t = uvs[uvIdx[i]];
                out.mesh.uvs.push_back(t.x); out.mesh.uvs.push_back(t.y);
            }
        }
        for (size_t i = 0; i < posIdx.size(); ++i) out.mesh.indices.push_back((unsigned)i);
        if (haveCur) { out.materials.push_back(cur); haveCur = false; }
        posIdx.clear(); norIdx.clear(); uvIdx.clear();
    };

    while (std::getline(ss, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "v") { float x,y,z; ls>>x>>y>>z; positions.push_back(Vec3(x,y,z)); }
        else if (tag == "vn") { float x,y,z; ls>>x>>y>>z; normals.push_back(Vec3(x,y,z)); }
        else if (tag == "vt") { float x,y; ls>>x>>y; uvs.push_back(Vec2(x,y)); }
        else if (tag == "f") {
            // 支持 v/vt/vn
            for (int k = 0; k < 3; ++k) {
                std::string tok; ls >> tok;
                unsigned vi=0, ti=0, ni=0;
                if (tok.empty()) break;
                std::stringstream ts(tok);
                char c;
                ts >> vi; if (ts.peek()=='/'){ts>>c; if(ts.peek()!='/'){ts>>ti;} ts>>c>>ni;}
                posIdx.push_back(vi ? vi-1 : 0);
                uvIdx.push_back(ti ? ti-1 : 0);
                norIdx.push_back(ni ? ni-1 : 0);
            }
        }
        else if (tag == "usemtl") {
            commitMesh();
            cur = Material();
            ls >> cur.name;
            haveCur = true;
        }
        else if (tag == "mtllib") {
            // 材质文件由外部加载（简化）
        }
    }
    commitMesh();
    if (!haveCur) out.materials.push_back(cur);
    return !out.mesh.positions.empty();
}

} // namespace pyrect3d
