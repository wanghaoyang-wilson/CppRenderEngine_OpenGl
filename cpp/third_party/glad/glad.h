// ============================================================
//  PyRecT3D — third_party/glad/glad.h
//  轻量 OpenGL 函数加载器（GL 3.3 Core / GLES 3.0 共用）
//
//  为什么不是官方 glad 全量生成：
//    官方 glad 的 GL 3.3 Core 单文件约 500KB+，包含引擎永远用不到的
//    上千个扩展入口。这里只导出引擎真正使用的 ~55 个核心函数，
//    头 + 实现合计控制在 20KB 量级，符合引擎"合并头文件 + 内联转换层 + O3"
//    的瘦身目标；新增函数时只需在 PYR3D_GL_FUNC_LIST 里加一行。
//
//  用法（C / C++ 均可）：
//      #include "glad.h"
//      创建好上下文后调用一次：gladLoadGL();            // 桌面（WGL / GLX 自动探测）
//      Android / EGL：       gladLoadGLES3(myEglLoader); // 传入 eglGetProcAddress 包装
//      Emscripten：          gladLoadGL();（无需加载，直接绑定已链接符号）
//      之后直接调用 glClear / glDrawElements 等名字即可，
//      它们会被宏映射到 glad_glXxx 函数指针（也可显式写 glad_glXxx）。
//
//  注意：
//    - 本头文件不 include 任何系统 GL 头，避免与 <GL/gl.h> 的类型/宏冲突。
//    - GLES 上不存在的入口（如 glPolygonMode）加载后为 NULL，调用前请判空。
//    - 一个进程只需加载一次；函数指针是全局的，与具体上下文无关。
// ============================================================
#ifndef PYRECT3D_GLAD_H
#define PYRECT3D_GLAD_H

#ifdef __cplusplus
extern "C" {
#endif

// ---------------- 平台调用约定 ----------------
#ifdef _WIN32
#  ifdef __GNUC__
#    define PYR3D_GLAPI __attribute__((stdcall))
#  else
#    define PYR3D_GLAPI __stdcall
#  endif
#else
#  define PYR3D_GLAPI
#endif

// ---------------- 基础类型（自定义，不依赖系统 GL 头） ----------------
typedef unsigned int    GLenum;
typedef unsigned char   GLboolean;
typedef unsigned int    GLbitfield;
typedef signed char     GLbyte;
typedef short           GLshort;
typedef int             GLint;
typedef int             GLsizei;
typedef unsigned char   GLubyte;
typedef unsigned short  GLushort;
typedef unsigned int    GLuint;
typedef float           GLfloat;
typedef float           GLclampf;
typedef double          GLdouble;
typedef double          GLclampd;
typedef long            GLintptr;
typedef long            GLsizeiptr;
typedef char            GLchar;

typedef void            GLvoid;

// 属性/索引
typedef unsigned int    GL_ATTRIB;
#define GL_FALSE 0
#define GL_TRUE  1

// ---------------- 常量（取用到的子集，值与 GL/GLES 规范一致） ----------------
#define GL_NO_ERROR                        0x0000
#define GL_DEPTH_BUFFER_BIT                0x00000100
#define GL_COLOR_BUFFER_BIT                0x00004000

#define GL_POINTS                          0x0000
#define GL_LINES                           0x0001
#define GL_LINE_STRIP                      0x0003
#define GL_TRIANGLES                       0x0004
#define GL_TRIANGLE_STRIP                  0x0005
#define GL_TRIANGLE_FAN                    0x0006

#define GL_NEVER                           0x0200
#define GL_LESS                            0x0201
#define GL_EQUAL                           0x0202
#define GL_LEQUAL                          0x0203
#define GL_GREATER                         0x0204
#define GL_ALWAYS                          0x0207

#define GL_SRC_ALPHA                       0x0302
#define GL_ONE_MINUS_SRC_ALPHA             0x0303
#define GL_ONE                             1
#define GL_ZERO                            0

#define GL_FRONT                           0x0404
#define GL_FRONT_AND_BACK                  0x0408
#define GL_BACK                            0x0405
#define GL_CW                              0x0900
#define GL_CCW                             0x0901

#define GL_CULL_FACE                       0x0B44
#define GL_DEPTH_TEST                      0x0B71
#define GL_BLEND                           0x0BE2
#define GL_SCISSOR_TEST                    0x0C11
#define GL_UNPACK_ALIGNMENT                0x0CF5

#define GL_TEXTURE_2D                      0x0DE1
#define GL_TEXTURE_MAG_FILTER              0x2800
#define GL_TEXTURE_MIN_FILTER              0x2801

#define GL_NEAREST                         0x2600
#define GL_LINEAR                          0x2601
#define GL_NEAREST_MIPMAP_NEAREST          0x2700
#define GL_LINEAR_MIPMAP_LINEAR            0x2703

#define GL_TEXTURE0                        0x84C0
#define GL_CLAMP_TO_EDGE                   0x812F
#define GL_REPEAT                          0x2901

#define GL_RGB                             0x1907
#define GL_RGBA                            0x1908
#define GL_UNSIGNED_BYTE                   0x1401
#define GL_UNSIGNED_SHORT                  0x1403
#define GL_UNSIGNED_INT                    0x1405
#define GL_FLOAT                           0x1406
#define GL_DEPTH_COMPONENT                 0x1902

#define GL_ARRAY_BUFFER                    0x8892
#define GL_ELEMENT_ARRAY_BUFFER            0x8893
#define GL_STREAM_DRAW                     0x88E0
#define GL_STATIC_DRAW                     0x88E4
#define GL_DYNAMIC_DRAW                    0x88E8

#define GL_FRAGMENT_SHADER                 0x8B30
#define GL_VERTEX_SHADER                   0x8B31
#define GL_COMPILE_STATUS                  0x8B81
#define GL_LINK_STATUS                     0x8B82
#define GL_INFO_LOG_LENGTH                 0x8B84

#define GL_LINE                            0x1B01
#define GL_FILL                            0x1B02

#define GL_VENDOR                          0x1F00
#define GL_RENDERER                        0x1F01
#define GL_VERSION                         0x1F02
#define GL_SHADING_LANGUAGE_VERSION        0x8B8C
#define GL_MAX_TEXTURE_SIZE                0x0D33

// ---------------- 函数表（唯一维护点） ----------------
//   格式：F(ret, name, args)
//   增删函数只改这一段；声明 / 定义 / 加载 / 别名宏都从这里派生。
#define PYR3D_GL_FUNC_LIST(F)                                                    \
    /* 状态与清屏 */                                                              \
    F(void, glClear,        (GLbitfield mask))                                   \
    F(void, glClearColor,   (GLclampf r, GLclampf g, GLclampf b, GLclampf a))    \
    F(void, glViewport,     (GLint x, GLint y, GLsizei w, GLsizei h))            \
    F(void, glScissor,      (GLint x, GLint y, GLsizei w, GLsizei h))            \
    F(void, glEnable,       (GLenum cap))                                        \
    F(void, glDisable,      (GLenum cap))                                        \
    F(void, glBlendFunc,    (GLenum sfactor, GLenum dfactor))                    \
    F(void, glDepthFunc,    (GLenum func))                                       \
    F(void, glCullFace,     (GLenum mode))                                       \
    F(void, glFrontFace,    (GLenum mode))                                       \
    F(void, glPolygonMode,  (GLenum face, GLenum mode))   /* GLES 无，判空 */     \
    F(void, glPixelStorei,  (GLenum pname, GLint param))                         \
    F(void, glFlush,        (void))                                              \
    F(void, glFinish,       (void))                                              \
    F(GLenum, glGetError,   (void))                                              \
    F(const GLubyte*, glGetString, (GLenum name))                                \
    F(void, glGetIntegerv,  (GLenum pname, GLint* data))                         \
    F(void, glReadPixels,   (GLint x, GLint y, GLsizei w, GLsizei h,             \
                             GLenum format, GLenum type, void* pixels))          \
    /* 缓冲区 */                                                                  \
    F(void, glGenBuffers,   (GLsizei n, GLuint* buffers))                        \
    F(void, glBindBuffer,   (GLenum target, GLuint buffer))                      \
    F(void, glBufferData,   (GLenum target, GLsizeiptr size,                     \
                             const void* data, GLenum usage))                     \
    F(void, glBufferSubData,(GLenum target, GLintptr offset,                     \
                             GLsizeiptr size, const void* data))                 \
    F(void, glDeleteBuffers,(GLsizei n, const GLuint* buffers))                  \
    /* VAO（GLES3 需要 OES 或 3.0 原生，桌面 3.3 原生） */                         \
    F(void, glGenVertexArrays,    (GLsizei n, GLuint* arrays))                   \
    F(void, glBindVertexArray,    (GLuint array))                                \
    F(void, glDeleteVertexArrays, (GLsizei n, const GLuint* arrays))             \
    /* 纹理 */                                                                    \
    F(void, glGenTextures,  (GLsizei n, GLuint* textures))                       \
    F(void, glBindTexture,  (GLenum target, GLuint texture))                     \
    F(void, glDeleteTextures, (GLsizei n, const GLuint* textures))               \
    F(void, glTexImage2D,   (GLenum target, GLint level, GLint internalFormat,   \
                             GLsizei w, GLsizei h, GLint border,                 \
                             GLenum format, GLenum type, const void* pixels))    \
    F(void, glTexSubImage2D,(GLenum target, GLint level, GLint xoffset,          \
                             GLint yoffset, GLsizei w, GLsizei h,                \
                             GLenum format, GLenum type, const void* pixels))    \
    F(void, glTexParameteri,(GLenum target, GLenum pname, GLint param))          \
    F(void, glActiveTexture, (GLenum texture))                                   \
    /* 着色器 */                                                                  \
    F(GLuint, glCreateShader, (GLenum type))                                     \
    F(void, glShaderSource,  (GLuint shader, GLsizei count,                      \
                              const GLchar* const* string, const GLint* length)) \
    F(void, glCompileShader, (GLuint shader))                                    \
    F(void, glGetShaderiv,   (GLuint shader, GLenum pname, GLint* params))       \
    F(void, glGetShaderInfoLog, (GLuint shader, GLsizei bufSize,                 \
                                 GLsizei* length, GLchar* infoLog))              \
    F(void, glDeleteShader,  (GLuint shader))                                    \
    F(GLuint, glCreateProgram, (void))                                           \
    F(void, glAttachShader,  (GLuint program, GLuint shader))                    \
    F(void, glLinkProgram,   (GLuint program))                                   \
    F(void, glGetProgramiv,  (GLuint program, GLenum pname, GLint* params))      \
    F(void, glGetProgramInfoLog, (GLuint program, GLsizei bufSize,               \
                                  GLsizei* length, GLchar* infoLog))             \
    F(void, glDeleteProgram, (GLuint program))                                   \
    F(void, glUseProgram,    (GLuint program))                                   \
    /* uniform */                                                                 \
    F(GLint, glGetUniformLocation, (GLuint program, const GLchar* name))         \
    F(void, glUniform1i,     (GLint loc, GLint v0))                              \
    F(void, glUniform1f,     (GLint loc, GLfloat v0))                            \
    F(void, glUniform2f,     (GLint loc, GLfloat v0, GLfloat v1))                \
    F(void, glUniform3f,     (GLint loc, GLfloat v0, GLfloat v1, GLfloat v2))    \
    F(void, glUniform4f,     (GLint loc, GLfloat v0, GLfloat v1,                 \
                              GLfloat v2, GLfloat v3))                           \
    F(void, glUniformMatrix4fv, (GLint loc, GLsizei count,                       \
                                 GLboolean transpose, const GLfloat* value))     \
    /* 顶点属性 */                                                                \
    F(void, glEnableVertexAttribArray,  (GLuint index))                          \
    F(void, glDisableVertexAttribArray, (GLuint index))                          \
    F(void, glVertexAttribPointer, (GLuint index, GLint size, GLenum type,       \
                                    GLboolean normalized, GLsizei stride,        \
                                    const void* pointer))                        \
    /* 绘制 */                                                                    \
    F(void, glDrawArrays,   (GLenum mode, GLint first, GLsizei count))           \
    F(void, glDrawElements, (GLenum mode, GLsizei count, GLenum type,            \
                             const void* indices))

// ---- 声明：typedef + extern 指针 ----
#define PYR3D_GLAD_DECL(ret, name, args)                       \
    typedef ret (PYR3D_GLAPI *PFN_##name) args;                \
    extern PFN_##name glad_##name;

PYR3D_GL_FUNC_LIST(PYR3D_GLAD_DECL)
#undef PYR3D_GLAD_DECL

// ---- 别名：让源码可以直接写 glXxx(...) ----
#define glClear                  glad_glClear
#define glClearColor             glad_glClearColor
#define glViewport               glad_glViewport
#define glScissor                glad_glScissor
#define glEnable                 glad_glEnable
#define glDisable                glad_glDisable
#define glBlendFunc              glad_glBlendFunc
#define glDepthFunc              glad_glDepthFunc
#define glCullFace               glad_glCullFace
#define glFrontFace              glad_glFrontFace
#define glPolygonMode            glad_glPolygonMode
#define glPixelStorei            glad_glPixelStorei
#define glFlush                  glad_glFlush
#define glFinish                 glad_glFinish
#define glGetError               glad_glGetError
#define glGetString              glad_glGetString
#define glGetIntegerv            glad_glGetIntegerv
#define glReadPixels             glad_glReadPixels

#define glGenBuffers             glad_glGenBuffers
#define glBindBuffer             glad_glBindBuffer
#define glBufferData             glad_glBufferData
#define glBufferSubData          glad_glBufferSubData
#define glDeleteBuffers          glad_glDeleteBuffers

#define glGenVertexArrays        glad_glGenVertexArrays
#define glBindVertexArray        glad_glBindVertexArray
#define glDeleteVertexArrays     glad_glDeleteVertexArrays

#define glGenTextures            glad_glGenTextures
#define glBindTexture            glad_glBindTexture
#define glDeleteTextures         glad_glDeleteTextures
#define glTexImage2D             glad_glTexImage2D
#define glTexSubImage2D          glad_glTexSubImage2D
#define glTexParameteri          glad_glTexParameteri
#define glActiveTexture          glad_glActiveTexture

#define glCreateShader           glad_glCreateShader
#define glShaderSource           glad_glShaderSource
#define glCompileShader          glad_glCompileShader
#define glGetShaderiv            glad_glGetShaderiv
#define glGetShaderInfoLog       glad_glGetShaderInfoLog
#define glDeleteShader           glad_glDeleteShader
#define glCreateProgram          glad_glCreateProgram
#define glAttachShader           glad_glAttachShader
#define glLinkProgram            glad_glLinkProgram
#define glGetProgramiv           glad_glGetProgramiv
#define glGetProgramInfoLog      glad_glGetProgramInfoLog
#define glDeleteProgram          glad_glDeleteProgram
#define glUseProgram             glad_glUseProgram

#define glGetUniformLocation     glad_glGetUniformLocation
#define glUniform1i              glad_glUniform1i
#define glUniform1f              glad_glUniform1f
#define glUniform2f              glad_glUniform2f
#define glUniform3f              glad_glUniform3f
#define glUniform4f              glad_glUniform4f
#define glUniformMatrix4fv       glad_glUniformMatrix4fv

#define glEnableVertexAttribArray   glad_glEnableVertexAttribArray
#define glDisableVertexAttribArray  glad_glDisableVertexAttribArray
#define glVertexAttribPointer       glad_glVertexAttribPointer

#define glDrawArrays             glad_glDrawArrays
#define glDrawElements           glad_glDrawElements

// ---------------- 加载接口 ----------------
typedef void* (PYR3D_GLAPI *PYR3D_GL_LOADER)(const char* name);

// 桌面：自动探测 WGL（Windows）/ GLX（Linux）
// 成功返回 1，失败返回 0
int gladLoadGL(void);

// 移动端 / EGL / Web：由调用方传入加载器（如 eglGetProcAddress 的包装）
int gladLoadGLES3(PYR3D_GL_LOADER loader);

// 通用入口：任意平台注入加载器（返回 1 = 成功）
int gladLoadGLFromLoader(PYR3D_GL_LOADER loader);

// 已成功加载的入口数量（排查"某个函数是 NULL"时用）
int gladLoadedCount(void);

// 版本/厂商信息（加载后可直接读，无需再调 glGetString）
const char* gladGetVersionString(void);
const char* gladGetRendererString(void);

#ifdef __cplusplus
}
#endif

#endif // PYRECT3D_GLAD_H
