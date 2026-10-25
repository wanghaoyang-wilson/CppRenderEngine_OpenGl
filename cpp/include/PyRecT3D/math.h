// ============================================================
//  PyRecT3D/math.h — 数学类型声明（契约层，仅声明）
//  向量 / 矩阵 / 射线 / AABB 接口
// ============================================================
#pragma once
#include "export.h"
#include <cstdint>

namespace pyrect3d {

// ---------- Vec2 / Vec3 / Vec4 ----------
struct PYRECT3D_API Vec2 {
    float x = 0.f, y = 0.f;
    Vec2() = default;
    Vec2(float x, float y);
    float length() const;
    Vec2 normalized() const;
    Vec2 operator+(const Vec2& o) const;
    Vec2 operator-(const Vec2& o) const;
    Vec2 operator*(float s) const;
    float dot(const Vec2& o) const;
};

struct PYRECT3D_API Vec3 {
    float x = 0.f, y = 0.f, z = 0.f;
    Vec3() = default;
    Vec3(float x, float y, float z);
    float length() const;
    Vec3 normalized() const;
    Vec3 operator+(const Vec3& o) const;
    Vec3 operator-(const Vec3& o) const;
    Vec3 operator*(float s) const;
    Vec3 operator-() const;
    float dot(const Vec3& o) const;
    Vec3 cross(const Vec3& o) const;
};

struct PYRECT3D_API Vec4 {
    float x = 0.f, y = 0.f, z = 0.f, w = 1.f;
    Vec4() = default;
    Vec4(float x, float y, float z, float w);
    Vec4(const Vec3& v, float w);
};

// ---------- 4x4 矩阵（列主序，OpenGL 惯例）----------
struct PYRECT3D_API Mat4 {
    float m[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };
    Mat4() = default;

    static Mat4 perspective(float fovDeg, float aspect, float nearP, float farP);
    static Mat4 orthographic(float left, float right, float bottom, float top,
                             float nearP, float farP);
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up);
    static Mat4 translation(const Vec3& t);
    static Mat4 scale(const Vec3& s);
    static Mat4 rotationX(float deg);
    static Mat4 rotationY(float deg);
    static Mat4 rotationZ(float deg);

    Mat4 operator*(const Mat4& o) const;
    Vec4 operator*(const Vec4& v) const;
    Mat4 inverse() const;
};

// ---------- 射线 & 命中 ----------
struct PYRECT3D_API Ray {
    Vec3 origin;
    Vec3 direction;   // 归一化
    Ray() = default;
    Ray(const Vec3& o, const Vec3& d);
};

// 命中结果：返回命中的实体实例句柄，上层自行处理业务
struct PYRECT3D_API RaycastHit {
    bool hit = false;
    float distance = 0.f;
    Vec3 point;
    Vec3 normal;
    std::int64_t entityId = -1;   // 命中的实体 id
};

// ---------- AABB ----------
struct PYRECT3D_API AABB {
    Vec3 min;
    Vec3 max;

    AABB() = default;
    AABB(const Vec3& min, const Vec3& max);
    // 射线求交（slab 法），命中返回 true 并写出 t 区间
    bool intersect(const Ray& ray, float& tNear, float& tFar) const;
    bool contains(const Vec3& p) const;
};

} // namespace pyrect3d
