// ============================================================
//  PyRecT3D — renderer.cpp
//  渲染器实现：2D/3D 图元、Cube/ScreenCube、帧导出、抗锯齿开关
//  （演示版本：采用纯软件光栅化回退，不依赖外部 GL 库加载器；
//   生产环境可在此接入 wgl/GLX 上下文 + OpenGL 3.3 调用）
// ============================================================
#include <PyRecT3D/renderer.h>
#include <PyRecT3D/core.h>
#include <cstring>
#include <cmath>
#include <fstream>

namespace pyrect3d {

// 软件帧缓冲（RGB）
struct Framebuffer {
    int w = 0, h = 0;
    std::vector<unsigned char> rgb;
    void resize(int w_, int h_) {
        w = w_; h = h_; rgb.assign((size_t)w*h*3, 0);
    }
    void clear(float r, float g, float b) {
        for (int i = 0; i < w*h; ++i) { rgb[i*3]=(unsigned char)(r*255); rgb[i*3+1]=(unsigned char)(g*255); rgb[i*3+2]=(unsigned char)(b*255); }
    }
    void put(int x, int y, float r, float g, float b) {
        if (x<0||y<0||x>=w||y>=h) return;
        int i=(y*w+x)*3; rgb[i]=(unsigned char)(r*255); rgb[i+1]=(unsigned char)(g*255); rgb[i+2]=(unsigned char)(b*255);
    }
};

struct Renderer::Impl {
    Framebuffer fb;
    RenderMode mode = RenderMode::Forward;
    float clearR=0.15f, clearG=0.15f, clearB=0.2f;
    bool aa = false;
};

Renderer::Renderer() : impl_(new Impl()) {}
Renderer::~Renderer() { delete impl_; }

bool Renderer::init(int width, int height) {
    impl_->fb.resize(width, height);
    return true;
}
void Renderer::shutdown() {}
void Renderer::setMode(RenderMode m) { impl_->mode = m; }
RenderMode Renderer::mode() const { return impl_->mode; }
void Renderer::setClearColor(float r, float g, float b, float a) { (void)a; impl_->clearR=r; impl_->clearG=g; impl_->clearB=b; }

// 简单投影：把 3D 实体中心投影到屏幕（演示）
void Renderer::drawEntity(const Entity& e) {
    impl_->fb.clear(impl_->clearR, impl_->clearG, impl_->clearB);
    Vec3 pos = e.position();
    float f = 500.f;
    float sx = impl_->fb.w/2 + pos.x * f / (pos.z + 5.f);
    float sy = impl_->fb.h/2 - pos.y * f / (pos.z + 5.f);
    float s = e.scale().x * f / (pos.z + 5.f);
    if (s <= 0) return;
    // 绘制一个简单方框表示实体（演示线框）
    int cx = (int)sx, cy = (int)sy, r = (int)(s * 0.5f);
    if (r < 1) r = 1;
    for (int dx = -r; dx <= r; ++dx) {
        impl_->fb.put(cx+dx, cy-r, 1,1,1);
        impl_->fb.put(cx+dx, cy+r, 1,1,1);
    }
    for (int dy = -r; dy <= r; ++dy) {
        impl_->fb.put(cx-r, cy+dy, 1,1,1);
        impl_->fb.put(cx+r, cy+dy, 1,1,1);
    }
}

void Renderer::drawScreenRect(float x, float y, float w, float h) {
    for (int j=(int)y; j<(int)(y+h); ++j)
        for (int i=(int)x; i<(int)(x+w); ++i)
            impl_->fb.put(i, j, 1,1,1);
}

void Renderer::setAntiAliasing(bool on) { impl_->aa = on; }

bool Renderer::saveFrame(const std::string& path) {
    // 简易 PPM 导出（未做 AA 合成，作为帧导出演示）
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f << "P6\n" << impl_->fb.w << " " << impl_->fb.h << "\n255\n";
    f.write((const char*)impl_->fb.rgb.data(), impl_->fb.rgb.size());
    return true;
}

// ---------- 图元 ----------
Cube::Cube(const Vec3& position, const Vec3& scale)
    : Entity(0) { setPosition(position); setScale(scale); }
Cube::~Cube() {}

ScreenCube::ScreenCube(float x, float y, float w, float h)
    : Entity(0) { setPosition(Vec3(x,y,0)); setScale(Vec3(w,h,1)); }
ScreenCube::~ScreenCube() {}
float ScreenCube::x() const { return position().x; }
float ScreenCube::y() const { return position().y; }
float ScreenCube::width() const { return scale().x; }
float ScreenCube::height() const { return scale().y; }

// ---------- 软光追 ----------
struct SoftRayTracer::Impl {
    // 球体场景（演示）
    struct Sphere { Vec3 c; float r; Vec3 color; };
    std::vector<Sphere> spheres;
    void reset() { spheres.clear(); }
};
SoftRayTracer::SoftRayTracer() : impl_(new Impl()) {}
SoftRayTracer::~SoftRayTracer() { delete impl_; }

bool SoftRayTracer::render(const Scene& scene, int w, int h, std::vector<unsigned char>& outRGB) {
    (void)scene;
    outRGB.assign((size_t)w*h*3, 0);
    impl_->reset();
    impl_->spheres.push_back({Vec3(0,0,-6), 1.2f, Vec3(1,0.3f,0.3f)});
    impl_->spheres.push_back({Vec3(2,-0.5f,-8), 1.0f, Vec3(0.3f,0.8f,0.3f)});
    impl_->spheres.push_back({Vec3(-2,-0.5f,-9), 1.4f, Vec3(0.3f,0.4f,0.9f)});
    impl_->spheres.push_back({Vec3(0,-100.5f,-10), 100.f, Vec3(0.6f,0.6f,0.6f)});

    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            // 针孔相机：主光线
            float u = (i - w/2.f) / (float)h;
            float v = (j - h/2.f) / (float)h;
            Vec3 dir = Vec3(u, -v, -1.f).normalized();
            Vec3 color(0.1f,0.1f,0.15f);
            float bestT = 1e30f;
            Vec3 bestColor;
            Vec3 bestNormal;
            for (auto& s : impl_->spheres) {
                Vec3 oc = Vec3(0,0,0) - s.c;
                float b = oc.dot(dir);
                float c = oc.dot(oc) - s.r*s.r;
                float disc = b*b - c;
                if (disc < 0) continue;
                float t = -b - std::sqrt(disc);
                if (t < 0.001f) t = -b + std::sqrt(disc);
                if (t > 0.001f && t < bestT) {
                    bestT = t;
                    bestColor = s.color;
                    bestNormal = (dir*t - s.c).normalized();
                }
            }
            if (bestT < 1e29f) {
                // 简单漫反射 + 顶光
                float diff = std::max(bestNormal.y, 0.f) * 0.8f + 0.3f;
                color = Vec3(bestColor.x*diff, bestColor.y*diff, bestColor.z*diff);
            }
            int idx = (j*w+i)*3;
            outRGB[idx]   = (unsigned char)(color.x*255);
            outRGB[idx+1] = (unsigned char)(color.y*255);
            outRGB[idx+2] = (unsigned char)(color.z*255);
        }
    }
    return true;
}

} // namespace pyrect3d
