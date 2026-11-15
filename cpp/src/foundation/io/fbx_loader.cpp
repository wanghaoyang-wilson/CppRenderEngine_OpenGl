// ============================================================
//  PyRecT3D — fbx_loader.cpp
//  轻量 ASCII FBX 解析器；二进制 FBX 先标记为不支持，不静默损坏。
// ============================================================
#include <PyRecT3D/fbx_loader.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>

namespace pyrect3d {
namespace {

std::string trimCopy(std::string s) {
    auto first = std::find_if_not(s.begin(), s.end(), [](unsigned char c) {
        return std::isspace(c);
    });
    auto last = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c) {
        return std::isspace(c);
    }).base();
    if (first >= last) return {};
    return std::string(first, last);
}

std::string unquote(std::string token) {
    token = trimCopy(token);
    if (token.size() >= 2 && token.front() == '\'' && token.back() == '\'') {
        token = token.substr(1, token.size() - 2);
    }
    return token;
}

bool startsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

std::vector<float> parseFloats(const std::string& line, const std::string& tag) {
    std::vector<float> values;
    auto pos = line.find(tag);
    if (pos == std::string::npos) return values;

    std::string body = line.substr(pos + tag.size());
    if (!body.empty() && body.front() == ':') body.erase(body.begin());
    for (char& c : body) {
        if (c == ',' || c == ';' || c == '(' || c == ')' || c == '\'') c = ' ';
    }
    std::istringstream ss(body);
    float v;
    while (ss >> v) values.push_back(v);
    return values;
}

void appendVertex(ObjModel& out, const std::vector<Vec3>& positions,
                  const std::vector<Vec3>& normals, const std::vector<Vec2>& uvs,
                  std::size_t vi, std::size_t ni, std::size_t ti) {
    if (vi >= positions.size()) return;
    const Vec3& p = positions[vi];
    out.mesh.positions.push_back(p.x);
    out.mesh.positions.push_back(p.y);
    out.mesh.positions.push_back(p.z);

    if (ni < normals.size()) {
        const Vec3& n = normals[ni];
        out.mesh.normals.push_back(n.x);
        out.mesh.normals.push_back(n.y);
        out.mesh.normals.push_back(n.z);
    }
    if (ti < uvs.size()) {
        const Vec2& t = uvs[ti];
        out.mesh.uvs.push_back(t.x);
        out.mesh.uvs.push_back(t.y);
    }
    out.mesh.indices.push_back(static_cast<unsigned int>(out.mesh.positions.size() / 3 - 1));
}

void parseAsciiFbx(const std::string& content, FbxModel& out) {
    std::istringstream ss(content);
    std::string line;
    std::vector<Vec3> positions, normals;
    std::vector<Vec2> uvs;
    std::string currentNode;

    Material material;
    material.name = "fbx_default";
    material.diffuseColor = Vec3(0.8f, 0.8f, 0.8f);

    while (std::getline(ss, line)) {
        std::string trimmed = trimCopy(line);
        if (trimmed.empty() || trimmed.front() == ';') continue;

        if (startsWith(trimmed, "Model:")) {
            currentNode = unquote(trimmed.substr(trimmed.find("Model:") + 6));
            continue;
        }

        if (startsWith(trimmed, "Vertices:") || startsWith(trimmed, "Vertices")) {
            auto vals = parseFloats(trimmed, "Vertices");
            for (std::size_t i = 0; i + 2 < vals.size(); i += 3) {
                positions.push_back(Vec3(vals[i], vals[i + 1], vals[i + 2]));
            }
            continue;
        }

        if (startsWith(trimmed, "Normals:") || startsWith(trimmed, "Normals")) {
            auto vals = parseFloats(trimmed, "Normals");
            for (std::size_t i = 0; i + 2 < vals.size(); i += 3) {
                normals.push_back(Vec3(vals[i], vals[i + 1], vals[i + 2]));
            }
            continue;
        }

        if (startsWith(trimmed, "TextureCoord:") || startsWith(trimmed, "TextureCoord") || startsWith(trimmed, "UV")) {
            auto vals = parseFloats(trimmed, "TextureCoord:");
            for (std::size_t i = 0; i + 1 < vals.size(); i += 2) {
                uvs.push_back(Vec2(vals[i], vals[i + 1]));
            }
            continue;
        }

        if (startsWith(trimmed, "PolygonVertexIndex:") || startsWith(trimmed, "PolygonVertexIndex")) {
            auto vals = parseFloats(trimmed, "PolygonVertexIndex");
            for (std::size_t i = 0; i + 2 < vals.size(); i += 3) {
                auto sanitize = [](float f, std::size_t count) -> std::size_t {
                    auto idx = static_cast<std::int64_t>(f);
                    if (idx < 0) idx = -idx - 1;
                    if (idx < 0 || static_cast<std::size_t>(idx) >= count) return 0;
                    return static_cast<std::size_t>(idx);
                };
                appendVertex(out.mesh, positions, normals, uvs,
                             sanitize(vals[i], positions.size()),
                             sanitize(vals[i], normals.size()),
                             sanitize(vals[i], uvs.size()));
                appendVertex(out.mesh, positions, normals, uvs,
                             sanitize(vals[i + 1], positions.size()),
                             sanitize(vals[i + 1], normals.size()),
                             sanitize(vals[i + 1], uvs.size()));
                appendVertex(out.mesh, positions, normals, uvs,
                             sanitize(vals[i + 2], positions.size()),
                             sanitize(vals[i + 2], normals.size()),
                             sanitize(vals[i + 2], uvs.size()));
            }
            continue;
        }

        if (startsWith(trimmed, "Material:")) {
            material.name = unquote(trimmed.substr(trimmed.find("Material:") + 9));
        }
    }

    if (!currentNode.empty()) out.mesh.name = currentNode;
    out.mesh.materials.push_back(material);
    out.animation.name = currentNode;
    out.valid = !out.mesh.mesh.positions.empty();
    if (!out.valid) {
        out.warning = "No readable static mesh found in ASCII FBX.";
    }
}

bool readBinaryHeader(const std::string& path, bool& binary) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    char header[23] = {};
    f.read(header, 21);
    binary = std::memcmp(header, "Kaydara FBX Binary", 19) == 0;
    return true;
}

} // namespace

bool FbxLoader::isBinaryFbx(const std::string& path) {
    bool binary = false;
    return readBinaryHeader(path, binary) && binary;
}

bool FbxLoader::load(const std::string& path, FbxModel& out) {
    out = FbxModel();
    out.binary = isBinaryFbx(path);
    if (out.binary) {
        out.warning = "Binary FBX is not supported in 0.2.1; use ASCII export or later binary reader.";
        return false;
    }

    std::ifstream f(path);
    if (!f) {
        out.warning = "Cannot open FBX file.";
        return false;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return loadFromMemory(ss.str(), out);
}

bool FbxLoader::loadFromMemory(const std::string& content, FbxModel& out) {
    out = FbxModel();

    if (content.size() >= 19 && std::memcmp(content.data(), "Kaydara FBX Binary", 19) == 0) {
        out.binary = true;
        out.warning = "Binary FBX is not supported in 0.2.1.";
        return false;
    }

    parseAsciiFbx(content, out);
    return out.valid;
}

} // namespace pyrect3d
