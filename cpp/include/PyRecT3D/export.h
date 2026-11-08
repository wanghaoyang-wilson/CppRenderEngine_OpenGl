// ============================================================
//  PyRecT3D/export.h — 导出宏定义（契约层唯一入口）
//
//  本头文件仅定义符号导出/导入宏，无任何实现体。
//  - Windows DLL 构建：定义 PYRECT3D_BUILD_DLL → dllexport
//  - Windows 调用侧（DLL）：不定义任何宏 → dllimport
//  - Windows 静态库（LIB）：定义 PYRECT3D_STATIC → 空宏，直接链接
//  - Linux: 宏为空，依赖 soname 符号导出
// ============================================================
#pragma once

#if defined(_WIN32)
  #if defined(PYRECT3D_STATIC)
    // 静态库（LIB）：不导出也不导入，直接链接
    #define PYRECT3D_API
  #elif defined(PYRECT3D_BUILD_DLL)
    #define PYRECT3D_API __declspec(dllexport)
  #else
    #define PYRECT3D_API __declspec(dllimport)
  #endif
  #define PYRECT3D_CALL __cdecl
#else
  #define PYRECT3D_API
  #define PYRECT3D_CALL
#endif

// 版本
#define PYRECT3D_VERSION_MAJOR 0
#define PYRECT3D_VERSION_MINOR 2
#define PYRECT3D_VERSION_PATCH 0
#define PYRECT3D_VERSION_STRING "0.2.0"

// ---------- 编译期模块开关（由构建脚本 / CMake 注入） ----------
// 未定义的模块在构建时被整体裁剪，调用对应 API 返回 false。
#define PYRECT3D_HAS_RENDERER   1
#ifndef PYRECT3D_WITH_GUI
  #define PYRECT3D_WITH_GUI 1
#endif
#ifndef PYRECT3D_WITH_VIDEO
  #define PYRECT3D_WITH_VIDEO 1
#endif
#ifndef PYRECT3D_WITH_AUDIO
  #define PYRECT3D_WITH_AUDIO 1
#endif
#ifndef PYRECT3D_WITH_PHYSICS
  #define PYRECT3D_WITH_PHYSICS 1
#endif
#ifndef PYRECT3D_WITH_RAYTRACER
  #define PYRECT3D_WITH_RAYTRACER 1
#endif
#ifndef PYRECT3D_WITH_FFMPEG
  #define PYRECT3D_WITH_FFMPEG 0
#endif
