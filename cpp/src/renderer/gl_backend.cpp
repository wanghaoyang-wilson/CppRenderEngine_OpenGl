// ============================================================
//  PyRecT3D — cpp/src/renderer/gl_backend.cpp
//  OpenGL 3.3 Core / GLES 3.0 硬后端实现
//
//  只做三件事：
//    1. 用 glad 加载入口并建立一次性的 VAO/VBO/EBO；
//    2. 提供两个极简 shader（立方体 / 2D 矩形），避免为每种图元建程序；
//    3. 把 beginFrame / endFrame 与清屏、深度、混合状态管住。
//
//  所有 GL 调用都发生在本进程的渲染主线程（详见 docs/API.md §14.1）。
// ============================================================
#include "gl_backend.h"

#if defined(PYRECT3D_WITH_GL) && (PYRECT3D_WITH_GL)

#include "../../third_party/glad/glad.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace pyrect3d {
namespace gl {
namespace {

struct Backend {
    bool ready = false;
    int  w = 0, h = 0;

    GLuint cubeVao = 0, cubeVbo = 0, cubeEbo = 0;
    GLuint quadVao = 0, quadVbo = 0;

    GLuint cubeProg = 0, quadProg = 0;
    GLint  cubeMvp = -1,  cubeModel = -1, cubeWire = -1;
    GLint  quadMvp = -1,  quadColor = -1;
};

Backend g;

// ---------------- shader 工具 ----------------
GLuint compile(GLenum type, const char* src, const char* tag) {
    if (!glad_glCreateShader) return 0;
    GLuint s = glCreateShader(type);
    if (!s) return 0;
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        if (len > 0) {
            std::vector<char> log((size_t)len + 1, '\0');
            glGetShaderInfoLog(s, len, NULL, &log[0]);
            fprintf(stderr, "[PyRecT3D][gl] %s shader 编译失败: %s\n", tag, &log[0]);
        }
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint link(const char* vs, const char* fs, const char* tag) {
    GLuint v = compile(GL_VERTEX_SHADER, vs, tag);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs, tag);
    if (!v || !f) { if (v) glDeleteShader(v); if (f) glDeleteShader(f); return 0; }

    GLuint p = glCreateProgram();
    if (!p) { glDeleteShader(v); glDeleteShader(f); return 0; }

    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);

    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    glDeleteShader(v);
    glDeleteShader(f);

    if (!ok) {
        GLint len = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        if (len > 0) {
            std::vector<char> log((size_t)len + 1, '\0');
            glGetProgramInfoLog(p, len, NULL, &log[0]);
            fprintf(stderr, "[PyRecT3D][gl] %s program 链接失败: %s\n", tag, &log[0]);
        }
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

// ---------------- 内置 shader（GLSL 330 / ES 300 通用写法） ----------------
const char* kCubeVS =
    "#version 300 es\n"
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec3 aColor;\n"
    "uniform mat4 uMvp;\n"
    "out vec3 vColor;\n"
    "void main(){ vColor = aColor; gl_Position = uMvp * vec4(aPos, 1.0); }\n";

const char* kCubeFS =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec3 vColor;\n"
    "out vec4 fragColor;\n"
    "void main(){ fragColor = vec4(vColor, 1.0); }\n";

const char* kQuadVS =
    "#version 300 es\n"
    "layout(location=0) in vec2 aPos;\n"
    "uniform mat4 uMvp;\n"
    "void main(){ gl_Position = uMvp * vec4(aPos, 0.0, 1.0); }\n";

const char* kQuadFS =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform vec4 uColor;\n"
    "out vec4 fragColor;\n"
    "void main(){ fragColor = uColor; }\n";

// ---------------- 几何：单位立方体 ----------------
// 24 顶点（每面独立法线方向色彩），36 索引
const float kCubeVerts[] = {
    // +Z
    -0.5f,-0.5f, 0.5f, 1,0,0,   0.5f,-0.5f, 0.5f, 1,0,0,
     0.5f, 0.5f, 0.5f, 1,0,0,  -0.5f, 0.5f, 0.5f, 1,0,0,
    // -Z
     0.5f,-0.5f,-0.5f, 0,1,0,  -0.5f,-0.5f,-0.5f, 0,1,0,
    -0.5f, 0.5f,-0.5f, 0,1,0,   0.5f, 0.5f,-0.5f, 0,1,0,
    // +Y
    -0.5f, 0.5f, 0.5f, 0,0,1,   0.5f, 0.5f, 0.5f, 0,0,1,
     0.5f, 0.5f,-0.5f, 0,0,1,  -0.5f, 0.5f,-0.5f, 0,0,1,
    // -Y
    -0.5f,-0.5f,-0.5f, 1,1,0,   0.5f,-0.5f,-0.5f, 1,1,0,
     0.5f,-0.5f, 0.5f, 1,1,0,  -0.5f,-0.5f, 0.5f, 1,1,0,
    // +X
     0.5f,-0.5f, 0.5f, 1,0,1,   0.5f,-0.5f,-0.5f, 1,0,1,
     0.5f, 0.5f,-0.5f, 1,0,1,   0.5f, 0.5f, 0.5f, 1,0,1,
    // -X
    -0.5f,-0.5f,-0.5f, 0,1,1,  -0.5f,-0.5f, 0.5f, 0,1,1,
    -0.5f, 0.5f, 0.5f, 0,1,1,  -0.5f, 0.5f,-0.5f, 0,1,1,
};

const unsigned int kCubeIdx[] = {
     0, 1, 2,  0, 2, 3,  4, 5, 6,  4, 6, 7,
     8, 9,10,  8,10,11, 12,13,14, 12,14,15,
    16,17,18, 16,18,19, 20,21,22, 20,22,23,
};

const float kQuadVerts[] = { 0,0, 1,0, 1,1,  0,0, 1,1, 0,1 };

bool buildCube() {
    glGenVertexArrays(1, &g.cubeVao);
    glGenBuffers(1, &g.cubeVbo);
    glGenBuffers(1, &g.cubeEbo);
    if (!g.cubeVao || !g.cubeVbo || !g.cubeEbo) return false;

    glBindVertexArray(g.cubeVao);
    glBindBuffer(GL_ARRAY_BUFFER, g.cubeVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVerts), kCubeVerts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.cubeEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kCubeIdx), kCubeIdx, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
    return true;
}

bool buildQuad() {
    glGenVertexArrays(1, &g.quadVao);
    glGenBuffers(1, &g.quadVbo);
    if (!g.quadVao || !g.quadVbo) return false;

    glBindVertexArray(g.quadVao);
    glBindBuffer(GL_ARRAY_BUFFER, g.quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVerts), kQuadVerts, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    return true;
}

// 正交投影：屏幕像素坐标 → NDC
void ortho(float l, float r, float b, float t, float n, float f, float* out) {
    memset(out, 0, 16 * sizeof(float));
    out[0]  = 2.0f / (r - l);
    out[5]  = 2.0f / (t - b);
    out[10] = -2.0f / (f - n);
    out[12] = -(r + l) / (r - l);
    out[13] = -(t + b) / (t - b);
    out[14] = -(f + n) / (f - n);
    out[15] = 1.0f;
}

}  // namespace

// ---------------- 对外接口 ----------------
bool init(int width, int height) {
    g.w = width > 0 ? width : 1;
    g.h = height > 0 ? height : 1;

    if (!gladLoadGL()) {
        fprintf(stderr, "[PyRecT3D][gl] gladLoadGL 失败（%d 个入口），回落到软件光栅化\n",
                gladLoadedCount());
        g.ready = false;
        return false;
    }

    g.cubeProg = link(kCubeVS, kCubeFS, "cube");
    g.quadProg = link(kQuadVS, kQuadFS, "quad");
    if (!g.cubeProg || !g.quadProg) { g.ready = false; return false; }

    g.cubeMvp   = glGetUniformLocation(g.cubeProg, "uMvp");
    g.cubeModel = glGetUniformLocation(g.cubeProg, "uModel");
    g.cubeWire  = glGetUniformLocation(g.cubeProg, "uWire");
    g.quadMvp   = glGetUniformLocation(g.quadProg, "uMvp");
    g.quadColor = glGetUniformLocation(g.quadProg, "uColor");

    if (!buildCube() || !buildQuad()) { g.ready = false; return false; }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    g.ready = true;
    return true;
}

void shutdown() {
    if (!g.ready) return;
    if (g.cubeVao)     { glDeleteVertexArrays(1, &g.cubeVao); g.cubeVao = 0; }
    if (g.cubeVbo)     { glDeleteBuffers(1, &g.cubeVbo); g.cubeVbo = 0; }
    if (g.cubeEbo)     { glDeleteBuffers(1, &g.cubeEbo); g.cubeEbo = 0; }
    if (g.quadVao)     { glDeleteVertexArrays(1, &g.quadVao); g.quadVao = 0; }
    if (g.quadVbo)     { glDeleteBuffers(1, &g.quadVbo); g.quadVbo = 0; }
    if (g.cubeProg)    { glDeleteProgram(g.cubeProg); g.cubeProg = 0; }
    if (g.quadProg)    { glDeleteProgram(g.quadProg); g.quadProg = 0; }
    g.ready = false;   // 幂等：重复调用安全
}

bool available() { return g.ready; }

void resize(int width, int height) {
    g.w = width > 0 ? width : 1;
    g.h = height > 0 ? height : 1;
    if (g.ready) glViewport(0, 0, g.w, g.h);
}

void beginFrame(float r, float gg, float b, float a) {
    if (!g.ready) return;
    glViewport(0, 0, g.w, g.h);
    glClearColor(r, gg, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void endFrame() {
    if (!g.ready) return;
    glFlush();  // 交换缓冲由平台层（WGL / GLX / EGL）负责
}

void drawCube(const float* model4x4, const float* viewProj4x4, bool wireframe) {
    if (!g.ready || !g.cubeProg) return;

    float mvp[16];
    // mvp = viewProj * model（列主序，直接按行展开计算）
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            float s = 0.f;
            for (int k = 0; k < 4; ++k)
                s += viewProj4x4[k * 4 + r] * model4x4[c * 4 + k];
            mvp[c * 4 + r] = s;
        }
    }

    glUseProgram(g.cubeProg);
    glUniformMatrix4fv(g.cubeMvp, 1, GL_FALSE, mvp);
    if (g.cubeModel >= 0) glUniformMatrix4fv(g.cubeModel, 1, GL_FALSE, model4x4);

    if (wireframe && glad_glPolygonMode) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glBindVertexArray(g.cubeVao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (void*)0);
    glBindVertexArray(0);

    if (wireframe && glad_glPolygonMode) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glUseProgram(0);
}

void drawScreenRect(float x, float y, float w, float h,
                    float r, float gg, float b, float a) {
    if (!g.ready || !g.quadProg) return;

    float mvp[16];
    float m[16] = { w, 0, 0, 0,   0, h, 0, 0,   0, 0, 1, 0,   x, y, 0, 1 };
    float orth[16];
    ortho(0.f, (float)g.w, 0.f, (float)g.h, -1.f, 1.f, orth);

    for (int c = 0; c < 4; ++c)
        for (int rr = 0; rr < 4; ++rr) {
            float s = 0.f;
            for (int k = 0; k < 4; ++k) s += orth[k * 4 + rr] * m[c * 4 + k];
            mvp[c * 4 + rr] = s;
        }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(g.quadProg);
    glUniformMatrix4fv(g.quadMvp, 1, GL_FALSE, mvp);
    glUniform4f(g.quadColor, r, gg, b, a);

    glBindVertexArray(g.quadVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

std::string versionString() { return std::string(gladGetVersionString()); }
std::string rendererString() { return std::string(gladGetRendererString()); }

}  // namespace gl
}  // namespace pyrect3d

#else  // PYRECT3D_WITH_GL 未开启：提供空实现，保证链接不失败

namespace pyrect3d {
namespace gl {

bool init(int, int) { return false; }
void shutdown() {}
bool available() { return false; }
void beginFrame(float, float, float, float) {}
void endFrame() {}
void drawCube(const float*, const float*, bool) {}
void drawScreenRect(float, float, float, float, float, float, float, float) {}
void resize(int, int) {}
std::string versionString() { return std::string("software-rasterizer"); }
std::string rendererString() { return std::string("builtin"); }

}  // namespace gl
}  // namespace pyrect3d

#endif
