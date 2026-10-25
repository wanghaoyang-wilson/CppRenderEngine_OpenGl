// ============================================================
//  PyRecT3D/export.h — 导出宏定义（契约层唯一入口）
//
//  本头文件仅定义符号导出/导入宏，无任何实现体。
//  - Windows: PYRECT3D_BUILD_DLL 定义时为 dllexport，否则 dllimport
//  - Linux:   宏为空，依赖 soname 符号导出
// ============================================================
#pragma once

#if defined(_WIN32)
  #if defined(PYRECT3D_BUILD_DLL)
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
#define PYRECT3D_VERSION_MINOR 1
#define PYRECT3D_VERSION_PATCH 0
