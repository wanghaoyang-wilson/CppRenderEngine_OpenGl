// ============================================================
//  PyRecT3D — third_party/glad/glad.c
//  OpenGL 函数加载器实现（GL 3.3 Core / GLES 3.0）
//
//  加载策略：
//    Windows : LoadLibrary(opengl32.dll) + wglGetProcAddress，1.1 函数回落到 GetProcAddress
//    Linux   : dlopen(libGL.so.1) + glXGetProcAddressARB，1.1 函数回落到 dlsym
//    Android : 调用方把 eglGetProcAddress 包装成 PYR3D_GL_LOADER 传给 gladLoadGLES3()
//    Web     : Emscripten 里 GL 符号在链接期已解析，不由本加载器负责（返回 0，见注释）
//
//  本文件不 include 任何系统 GL 头，全部类型/常量/入口来自 glad.h。
// ============================================================
#include "glad.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// ---------------- 全局函数指针定义 ----------------
#define PYR3D_GLAD_DEF(ret, name, args) PFN_##name glad_##name = NULL;
PYR3D_GL_FUNC_LIST(PYR3D_GLAD_DEF)
#undef PYR3D_GLAD_DEF

static int   g_loadedCount = 0;
static char  g_version[128]  = "unknown";
static char  g_renderer[128] = "unknown";

// ---------------- 平台加载器 ----------------
#ifdef _WIN32
#include <windows.h>

typedef void* (PYR3D_GLAPI *PYR3D_WGLGETPROC)(const char*);

static PYR3D_WGLGETPROC g_wglGetProc   = NULL;
static HMODULE          g_openglModule = NULL;

static int isBadAddr(void* p) {
    // wglGetProcAddress 对不支持的入口可能返回 1/2/3/-1
    uintptr_t v = (uintptr_t)p;
    return (v == 0 || v == 1 || v == 2 || v == 3 || v == (uintptr_t)-1);
}

static void* win_loader(const char* name) {
    if (g_wglGetProc) {
        void* p = (void*)g_wglGetProc(name);
        if (!isBadAddr(p)) return p;
    }
    if (g_openglModule) {
        FARPROC f = GetProcAddress(g_openglModule, name);
        if (f) return (void*)f;
    }
    return NULL;
}

int gladLoadGL(void) {
    if (!g_openglModule) g_openglModule = LoadLibraryA("opengl32.dll");
    if (!g_openglModule) return 0;
    if (!g_wglGetProc) {
        g_wglGetProc = (PYR3D_WGLGETPROC)GetProcAddress(g_openglModule, "wglGetProcAddress");
    }
    return gladLoadGLFromLoader(win_loader);
}

#elif defined(__EMSCRIPTEN__)

// Web 端：GL ES 符号由 Emscripten 在链接期解析，不需要运行期加载。
// 若确实需要指针形式，把 WEB/bridge 里的函数表直接填进 glad_glXxx 即可。
int gladLoadGL(void) { return 0; }

#elif defined(__linux__) || defined(__unix__)

#include <dlfcn.h>

typedef void* (*PYR3D_GLXGETPROC)(const char*);

static PYR3D_GLXGETPROC g_glxGetProc = NULL;
static void*            g_glLibrary  = NULL;

static void* nix_loader(const char* name) {
    if (g_glxGetProc) {
        void* p = g_glxGetProc(name);
        if (p) return p;
    }
    if (g_glLibrary) {
        void* p = dlsym(g_glLibrary, name);
        if (p) return p;
    }
    return NULL;
}

int gladLoadGL(void) {
    if (!g_glLibrary) {
        g_glLibrary = dlopen("libGL.so.1", RTLD_LAZY | RTLD_LOCAL);
        if (!g_glLibrary) g_glLibrary = dlopen("libGL.so", RTLD_LAZY | RTLD_LOCAL);
        if (!g_glLibrary) g_glLibrary = dlopen("libGL.so.1.7", RTLD_LAZY | RTLD_LOCAL);
    }
    if (!g_glLibrary) return 0;
    if (!g_glxGetProc) {
        g_glxGetProc = (PYR3D_GLXGETPROC)dlsym(g_glLibrary, "glXGetProcAddressARB");
        if (!g_glxGetProc)
            g_glxGetProc = (PYR3D_GLXGETPROC)dlsym(g_glLibrary, "glXGetProcAddress");
    }
    return gladLoadGLFromLoader(nix_loader);
}

#else

// 其它平台（如 Android）：必须由调用方显式提供加载器
int gladLoadGL(void) { return 0; }

#endif

// ---------------- 通用加载 ----------------
static void captureInfo(void) {
    if (glad_glGetString) {
        const GLubyte* v = glad_glGetString(GL_VERSION);
        const GLubyte* r = glad_glGetString(GL_RENDERER);
        if (v) {
            size_t n = strlen((const char*)v);
            if (n > sizeof(g_version) - 1) n = sizeof(g_version) - 1;
            memcpy(g_version, v, n);
            g_version[n] = '\0';
        }
        if (r) {
            size_t n = strlen((const char*)r);
            if (n > sizeof(g_renderer) - 1) n = sizeof(g_renderer) - 1;
            memcpy(g_renderer, r, n);
            g_renderer[n] = '\0';
        }
    }
}

int gladLoadGLFromLoader(PYR3D_GL_LOADER loader) {
    if (!loader) return 0;

    g_loadedCount = 0;

#define PYR3D_GLAD_LOAD(ret, name, args)                    \
    glad_##name = (PFN_##name)loader(#name);                \
    if (glad_##name) ++g_loadedCount;

    PYR3D_GL_FUNC_LIST(PYR3D_GLAD_LOAD)

#undef PYR3D_GLAD_LOAD

    // 最低可用性校验：没有清屏和绘制就无法渲染
    if (!glad_glClear || !glad_glDrawElements || !glad_glCreateProgram) {
        g_loadedCount = 0;
        return 0;
    }

    captureInfo();
    return 1;
}

int gladLoadGLES3(PYR3D_GL_LOADER loader) {
    return gladLoadGLFromLoader(loader);
}

int gladLoadedCount(void) { return g_loadedCount; }

const char* gladGetVersionString(void) { return g_version; }

const char* gladGetRendererString(void) { return g_renderer; }
