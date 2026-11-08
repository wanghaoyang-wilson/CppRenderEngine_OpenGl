cmake_minimum_required(VERSION 3.10)
project(PyRecT3D VERSION 0.2.0 LANGUAGES CXX)

# ============================================================
#  PyRecT3D — C++ OpenGL 2D/3D 渲染引擎（v0.2）
#  平台：Windows (最低 Win7) / Linux / Android(GLES3) / Web(WASM+WebGL2)
#  图形：OpenGL 3.3 Core（桌面） / OpenGL ES 3.0（Android / WebGL2）
#  构建：源码优先编译，不加载任何预编译二进制；不使用预编译头
#
#  产物形态由 PYRECT3D_LIB_TYPE 决定：
#    - shared → PyRecT3D.dll / libPyRecT3D.so   （LoadLibrary 动态加载）
#    - static → PyRecT3D.lib / libPyRecT3D.a    （静态链接）
#  模块裁剪由 PYRECT3D_WITH_* 开关决定，未开启的模块不参与编译。
# ============================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(PYRECT3D_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include")
set(PYRECT3D_SRC_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src")

# ---------- 产物类型（DLL / LIB） ----------
set(PYRECT3D_LIB_TYPE "shared" CACHE STRING "shared=DLL, static=LIB")
if(PYRECT3D_LIB_TYPE STREQUAL "static")
  set(_libtype STATIC)
else()
  set(_libtype SHARED)
endif()

# ---------- 模块裁剪开关 ----------
option(PYRECT3D_WITH_GUI      "编译 GUI 模块"      ON)
option(PYRECT3D_WITH_VIDEO    "编译视频模块"       ON)
option(PYRECT3D_WITH_AUDIO    "编译音频模块"       ON)
option(PYRECT3D_WITH_PHYSICS  "编译物理/碰撞模块"   ON)
option(PYRECT3D_WITH_RAYTRACER "编译软光追模块"     ON)
option(PYRECT3D_WITH_FFMPEG   "使用外部 FFmpeg 解码" OFF)

# ---------- 源文件清单（分类） ----------
set(PYRECT3D_SOURCES
  # core
  ${PYRECT3D_SRC_DIR}/core/app_registry.h
  ${PYRECT3D_SRC_DIR}/core/engine.cpp
  ${PYRECT3D_SRC_DIR}/core/event_loop.cpp
  ${PYRECT3D_SRC_DIR}/core/message_queue.cpp
  ${PYRECT3D_SRC_DIR}/core/explicit_app.cpp
  ${PYRECT3D_SRC_DIR}/core/implicit_api.cpp
  ${PYRECT3D_SRC_DIR}/core/platform/platform_private.h
  # math
  ${PYRECT3D_SRC_DIR}/math/math.cpp
  # renderer
  ${PYRECT3D_SRC_DIR}/renderer/renderer.cpp
  ${PYRECT3D_SRC_DIR}/renderer/compositor.cpp
)

if(PYRECT3D_WITH_GUI)
  list(APPEND PYRECT3D_SOURCES ${PYRECT3D_SRC_DIR}/gui/gui.cpp)
endif()
if(PYRECT3D_WITH_VIDEO)
  list(APPEND PYRECT3D_SOURCES ${PYRECT3D_SRC_DIR}/media/video.cpp)
endif()
if(PYRECT3D_WITH_AUDIO)
  list(APPEND PYRECT3D_SOURCES ${PYRECT3D_SRC_DIR}/audio/audio.cpp)
endif()
if(PYRECT3D_WITH_PHYSICS)
  list(APPEND PYRECT3D_SOURCES ${PYRECT3D_SRC_DIR}/physics/collision_world.cpp)
endif()

# IO 始终编译（OBJ 加载是基础能力）
list(APPEND PYRECT3D_SOURCES ${PYRECT3D_SRC_DIR}/io/obj_loader.cpp)

# 平台粘合
if(WIN32)
  list(APPEND PYRECT3D_SOURCES ${PYRECT3D_SRC_DIR}/core/platform/win7_input.cpp)
else()
  list(APPEND PYRECT3D_SOURCES ${PYRECT3D_SRC_DIR}/core/platform/x11_input.cpp)
endif()

add_library(PyRecT3D ${_libtype} ${PYRECT3D_SOURCES})

target_include_directories(PyRecT3D
  PUBLIC  ${PYRECT3D_INCLUDE_DIR}
  PRIVATE ${PYRECT3D_SRC_DIR}
)
target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_BUILD_DLL=1)

# 模块开关透传到源码
if(PYRECT3D_WITH_GUI)
  target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_WITH_GUI=1)
else()
  target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_WITH_GUI=0)
endif()
if(PYRECT3D_WITH_VIDEO)
  target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_WITH_VIDEO=1)
else()
  target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_WITH_VIDEO=0)
endif()
if(PYRECT3D_WITH_AUDIO)
  target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_WITH_AUDIO=1)
else()
  target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_WITH_AUDIO=0)
endif()
if(PYRECT3D_WITH_PHYSICS)
  target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_WITH_PHYSICS=1)
else()
  target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_WITH_PHYSICS=0)
endif()
if(PYRECT3D_WITH_FFMPEG)
  target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_WITH_FFMPEG=1)
endif()

# ---------- 平台链接 ----------
if(WIN32)
  target_compile_definitions(PyRecT3D PRIVATE _WIN32_WINNT=0x0601) # Win7 最低
  target_link_libraries(PyRecT3D PRIVATE opengl32 user32 gdi32 winmm ole32)
  if(MSVC)
    target_compile_options(PyRecT3D PRIVATE /W4)
    # 发布瘦身：O2 + 去除调试符号
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
      target_compile_options(PyRecT3D PRIVATE /O2 /GL)
    endif()
  endif()
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(PyRecT3D PRIVATE X11 GL dl pthread)
  target_compile_options(PyRecT3D PRIVATE -Wall -Wextra)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # O3 + 内联 + 死代码消除，配合单后端锁定可压到百 KB 级
    target_compile_options(PyRecT3D PRIVATE -O3 -ffunction-sections -fdata-sections)
    target_link_options(PyRecT3D PRIVATE -Wl,--gc-sections)
  endif()
endif()

# ---------- 可选：pybind11 Python 绑定 ----------
option(PYRECT3D_BUILD_PYTHON "Build Python binding with pybind11" ON)
if(PYRECT3D_BUILD_PYTHON)
  find_package(Python3 COMPONENTS Interpreter Development QUIET)
  find_package(pybind11 CONFIG QUIET)
  if(pybind11_FOUND AND Python3_FOUND)
    pybind11_add_module(pyrect3d_binding ${PYRECT3D_SRC_DIR}/binding/binding.cpp)
    target_include_directories(pyrect3d_binding PRIVATE ${PYRECT3D_INCLUDE_DIR})
    target_link_libraries(pyrect3d_binding PRIVATE PyRecT3D)
    set_target_properties(pyrect3d_binding PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/py/package"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/py/package"
    )
  else()
    message(WARNING "pybind11 / Python3 not found. Python binding skipped.")
  endif()
endif()

# ---------- C++ 原生示例 ----------
option(PYRECT3D_BUILD_EXAMPLES "Build C++ examples" ON)
if(PYRECT3D_BUILD_EXAMPLES)
  add_executable(basic_cube ${CMAKE_CURRENT_SOURCE_DIR}/examples_cpp/basic_cube.cpp)
  target_include_directories(basic_cube PRIVATE ${PYRECT3D_INCLUDE_DIR})
  target_link_libraries(basic_cube PRIVATE PyRecT3D)

  add_executable(explicit_demo ${CMAKE_CURRENT_SOURCE_DIR}/examples_cpp/explicit_demo.cpp)
  target_include_directories(explicit_demo PRIVATE ${PYRECT3D_INCLUDE_DIR})
  target_link_libraries(explicit_demo PRIVATE PyRecT3D)

  add_executable(implicit_demo ${CMAKE_CURRENT_SOURCE_DIR}/examples_cpp/implicit_demo.cpp)
  target_include_directories(implicit_demo PRIVATE ${PYRECT3D_INCLUDE_DIR})
  target_link_libraries(implicit_demo PRIVATE PyRecT3D)
endif()

# ---------- 测试 ----------
option(PYRECT3D_BUILD_TESTS "Build smoke tests" ON)
if(PYRECT3D_BUILD_TESTS)
  add_executable(smoke_v2 ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke_v2.cpp)
  target_include_directories(smoke_v2 PRIVATE ${PYRECT3D_INCLUDE_DIR})
  target_link_libraries(smoke_v2 PRIVATE PyRecT3D)
endif()
