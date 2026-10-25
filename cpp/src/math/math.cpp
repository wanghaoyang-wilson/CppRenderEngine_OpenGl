// ============================================================
//  PyRecT3D — math.cpp
//  数学类型实现：向量 / 矩阵 / 射线 / AABB
// ============================================================
#include <PyRecT3D/math.h>
#include <cmath>
#include <algorithm>

namespace pyrect3d {

// ---------- Vec2 ----------
Vec2::Vec2(float x, float y) : x(x), y(y) {}
float Vec2::length() const { return std::sqrt(x*x + y*y); }
Vec2 Vec2::normalized() const { float l = length(); return l > 0 ? Vec2(x/l, y/l) : Vec2(); }
Vec2 Vec2::operator+(const Vec2& o) const { return Vec2(x+o.x, y+o.y); }
Vec2 Vec2::operator-(const Vec2& o) const { return Vec2(x-o.x, y-o.y); }
Vec2 Vec2::operator*(float s) const { return Vec2(x*s, y*s); }
float Vec2::dot(const Vec2& o) const { return x*o.x + y*o.y; }

// ---------- Vec3 ----------
Vec3::Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
float Vec3::length() const { return std::sqrt(x*x + y*y + z*z); }
Vec3 Vec3::normalized() const {
    float l = length();
    return l > 1e-6f ? Vec3(x/l, y/l, z/l) : Vec3();
}
Vec3 Vec3::operator+(const Vec3& o) const { return Vec3(x+o.x, y+o.y, z+o.z); }
Vec3 Vec3::operator-(const Vec3& o) const { return Vec3(x-o.x, y-o.y, z-o.z); }
Vec3 Vec3::operator*(float s) const { return Vec3(x*s, y*s, z*s); }
Vec3 Vec3::operator-() const { return Vec3(-x, -y, -z); }
float Vec3::dot(const Vec3& o) const { return x*o.x + y*o.y + z*o.z; }
Vec3 Vec3::cross(const Vec3& o) const {
    return Vec3(y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x);
}

// ---------- Vec4 ----------
Vec4::Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
Vec4::Vec4(const Vec3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

// ---------- Mat4 ----------
static float rad(float deg) { return deg * 3.14159265358979f / 180.f; }

Mat4 Mat4::perspective(float fovDeg, float aspect, float nearP, float farP) {
    Mat4 m; std::fill(m.m, m.m+16, 0.f);
    float f = 1.f / std::tan(rad(fovDeg) / 2.f);
    m.m[0] = f / aspect;
    m.m[5] = f;
    m.m[10] = (farP + nearP) / (nearP - farP);
    m.m[11] = -1.f;
    m.m[14] = (2.f * farP * nearP) / (nearP - farP);
    return m;
}

Mat4 Mat4::orthographic(float l, float r, float b, float t, float n, float f) {
    Mat4 m; std::fill(m.m, m.m+16, 0.f);
    m.m[0] = 2.f/(r-l); m.m[3] = -(r+l)/(r-l);
    m.m[5] = 2.f/(t-b); m.m[7] = -(t+b)/(t-b);
    m.m[10] = -2.f/(f-n); m.m[11] = -(f+n)/(f-n);
    m.m[15] = 1.f;
    return m;
}

Mat4 Mat4::lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Vec3 f = (center - eye).normalized();
    Vec3 s = f.cross(up).normalized();
    Vec3 u = s.cross(f);
    Mat4 m;
    m.m[0]=s.x; m.m[4]=s.y; m.m[8]=s.z;  m.m[12]=-s.dot(eye);
    m.m[1]=u.x; m.m[5]=u.y; m.m[9]=u.z;  m.m[13]=-u.dot(eye);
    m.m[2]=-f.x;m.m[6]=-f.y;m.m[10]=-f.z;m.m[14]= f.dot(eye);
    m.m[15]=1.f;
    return m;
}

Mat4 Mat4::translation(const Vec3& t) {
    Mat4 m; m.m[12]=t.x; m.m[13]=t.y; m.m[14]=t.z; return m;
}
Mat4 Mat4::scale(const Vec3& s) {
    Mat4 m; std::fill(m.m, m.m+16, 0.f);
    m.m[0]=s.x; m.m[5]=s.y; m.m[10]=s.z; m.m[15]=1.f; return m;
}
Mat4 Mat4::rotationX(float deg) {
    Mat4 m; float c=std::cos(rad(deg)), s=std::sin(rad(deg));
    m.m[5]=c; m.m[6]=s; m.m[9]=-s; m.m[10]=c; return m;
}
Mat4 Mat4::rotationY(float deg) {
    Mat4 m; float c=std::cos(rad(deg)), s=std::sin(rad(deg));
    m.m[0]=c; m.m[2]=-s; m.m[8]=s; m.m[10]=c; return m;
}
Mat4 Mat4::rotationZ(float deg) {
    Mat4 m; float c=std::cos(rad(deg)), s=std::sin(rad(deg));
    m.m[0]=c; m.m[1]=s; m.m[4]=-s; m.m[5]=c; return m;
}

Mat4 Mat4::operator*(const Mat4& o) const {
    Mat4 r;
    for (int c = 0; c < 4; ++c)
        for (int rw = 0; rw < 4; ++rw) {
            float v = 0;
            for (int k = 0; k < 4; ++k) v += m[k*4+rw] * o.m[c*4+k];
            r.m[c*4+rw] = v;
        }
    return r;
}
Vec4 Mat4::operator*(const Vec4& v) const {
    Vec4 r;
    r.x = m[0]*v.x + m[4]*v.y + m[8]*v.z + m[12]*v.w;
    r.y = m[1]*v.x + m[5]*v.y + m[9]*v.z + m[13]*v.w;
    r.z = m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14]*v.w;
    r.w = m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15]*v.w;
    return r;
}
Mat4 Mat4::inverse() const {
    // 4x4 高斯-约当求逆（简化实现）
    float a[16], inv[16];
    std::copy(m, m+16, a);
    std::fill(inv, inv+16, 0.f);
    for (int i = 0; i < 4; ++i) inv[i*4+i] = 1.f;
    for (int c = 0; c < 4; ++c) {
        int piv = c;
        for (int r = c+1; r < 4; ++r)
            if (std::abs(a[r*4+c]) > std::abs(a[piv*4+c])) piv = r;
        if (std::abs(a[piv*4+c]) < 1e-12f) return Mat4();
        if (piv != c)
            for (int j = 0; j < 4; ++j) {
                std::swap(a[c*4+j], a[piv*4+j]);
                std::swap(inv[c*4+j], inv[piv*4+j]);
            }
        float d = a[c*4+c];
        for (int j = 0; j < 4; ++j) { a[c*4+j] /= d; inv[c*4+j] /= d; }
        for (int r = 0; r < 4; ++r) {
            if (r == c) continue;
            float f = a[r*4+c];
            if (f == 0.f) continue;
            for (int j = 0; j < 4; ++j) {
                a[r*4+j] -= f * a[c*4+j];
                inv[r*4+j] -= f * inv[c*4+j];
            }
        }
    }
    Mat4 out; std::copy(inv, inv+16, out.m); return out;
}

// ---------- Ray ----------
Ray::Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d.normalized()) {}

// ---------- AABB ----------
AABB::AABB(const Vec3& min_, const Vec3& max_) : min(min_), max(max_) {}

bool AABB::intersect(const Ray& ray, float& tNear, float& tFar) const {
    tNear = -1e30f; tFar = 1e30f;
    for (int i = 0; i < 3; ++i) {
        float o = (i==0)?ray.origin.x:(i==1)?ray.origin.y:ray.origin.z;
        float d = (i==0)?ray.direction.x:(i==1)?ray.direction.y:ray.direction.z;
        float mn = (i==0)?min.x:(i==1)?min.y:min.z;
        float mx = (i==0)?max.x:(i==1)?max.y:max.z;
        if (std::abs(d) < 1e-9f) {
            if (o < mn || o > mx) return false;
        } else {
            float t1 = (mn - o) / d;
            float t2 = (mx - o) / d;
            if (t1 > t2) std::swap(t1, t2);
            tNear = std::max(tNear, t1);
            tFar  = std::min(tFar, t2);
            if (tNear > tFar) return false;
        }
    }
    return true;
}

bool AABB::contains(const Vec3& p) const {
    return p.x >= min.x && p.x <= max.x &&
           p.y >= min.y && p.y <= max.y &&
           p.z >= min.z && p.z <= max.z;
}

} // namespace pyrect3d
