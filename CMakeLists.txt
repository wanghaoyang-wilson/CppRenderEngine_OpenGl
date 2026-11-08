cmake_minimum_required(VERSION 3.10)
project(PyRecT3D VERSION 0.2.0 LANGUAGES C CXX)

# ============================================================
#  PyRecT3D — C++ OpenGL 2D/3D 渲染引擎（v0.2）
#  平台：Windows (最低 Win7) / Linux / Android(GLES3) / Web(WASM+WebGL2)
#  图形：OpenGL 3.3 Core（桌面） / OpenGL ES 3.0（Android / WebGL2）
#  构建：源码优先编译，不加载任何预编译二进制；不使用预编译头
#
#  注意：CMake 已不是必经路径。日常构建用 build.bat（Windows）或 build.sh（Linux），
#        两者都直接用 cl.exe / g++。这个 CMakeLists 只留给 IDE 与 find_package 集成。
#
#  产物形态由 PYRECT3D_LIB_TYPE 决定：
#    - shared → PyRecT3D.dll / libPyRecT3D.so
#    - static → PyRecT3D.lib / libPyRecT3D.a
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
option(PYRECT3D_WITH_GL       "启用 glad + 真实 OpenGL 后端（默认软件回退）" OFF)

# ---------- 源文件清单（按 cpp/LAYOUT.md 的分层） ----------
set(PYRECT3D_SOURCES
  # core
  ${PYRECT3D_SRC_DIR}/core/app_registry.h
  ${PYRECT3D_SRC_DIR}/core/engine.cpp
  ${PYRECT3D_SRC_DIR}/core/event_loop.cpp
  ${PYRECT3D_SRC_DIR}/core/message_queue.cpp
  ${PYRECT3D_SRC_DIR}/core/explicit_app.cpp
  ${PYRECT3D_SRC_DIR}/core/implicit_api.cpp
  ${PYRECT3D_SRC_DIR}/core/platform/platform_private.h
  # foundation
  ${PYRECT3D_SRC_DIR}/foundation/math/math.cpp
  ${PYRECT3D_SRC_DIR}/foundation/io/obj_loader.cpp
  # graphics
  ${PYRECT3D_SRC_DIR}/graphics/renderer.cpp
  ${PYRECT3D_SRC_DIR}/graphics/compositor.cpp
  ${PYRECT3D_SRC_DIR}/graphics/gl_backend.h
  ${PYRECT3D_SRC_DIR}/graphics/gl_backend.cpp
)

# GL 后端：glad 加载器（GL 3.3 Core / GLES 3.0 共用）
if(PYRECT3D_WITH_GL)
  list(APPEND PYRECT3D_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cpp/third_party/glad/glad.c)
endif()

if(PYRECT3D_WITH_GUI)
  list(APPEND PYRECT3D_SOURCES ${PYRECT3D_SRC_DIR}/ui/gui.cpp)
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
          ${CMAKE_CURRENT_SOURCE_DIR}/cpp/third_party/glad
)

# DLL 模式导出符号；静态库模式改用 PYRECT3D_STATIC（export.h 里展开为空宏）
if(PYRECT3D_LIB_TYPE STREQUAL "static")
  target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_STATIC=1)
else()
  target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_BUILD_DLL=1)
endif()

foreach(_mod GUI VIDEO AUDIO PHYSICS)
  if(PYRECT3D_WITH_${_mod})
    target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_WITH_${_mod}=1)
  else()
    target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_WITH_${_mod}=0)
  endif()
endforeach()

if(PYRECT3D_WITH_FFMPEG)
  target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_WITH_FFMPEG=1)
endif()
if(PYRECT3D_WITH_GL)
  target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_WITH_GL=1)
  message(STATUS "PyRecT3D: GL 后端已启用（glad + GL33/GLES3）")
else()
  target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_WITH_GL=0)
  message(STATUS "PyRecT3D: GL 后端未启用，使用内置软件光栅化回退")
endif()

# ---------- 平台链接 ----------
if(WIN32)
  target_compile_definitions(PyRecT3D PRIVATE _WIN32_WINNT=0x0601)
  target_link_libraries(PyRecT3D PRIVATE opengl32 user32 gdi32 winmm ole32)
  if(MSVC)
    target_compile_options(PyRecT3D PRIVATE /W4)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
      target_compile_options(PyRecT3D PRIVATE /O2 /GL)
    endif()
  endif()
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(PyRecT3D PRIVATE X11 GL dl pthread)
  target_compile_options(PyRecT3D PRIVATE -Wall -Wextra -fPIC)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
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
    pybind11_add_module(pyrect3d_binding
      ${PYRECT3D_SRC_DIR}/bindings/python/binding.cpp
      ${PYRECT3D_SRC_DIR}/bindings/python/binding_common.h
      ${PYRECT3D_SRC_DIR}/bindings/python/bind_math.cpp
      ${PYRECT3D_SRC_DIR}/bindings/python/bind_core.cpp
      ${PYRECT3D_SRC_DIR}/bindings/python/bind_message.cpp
      ${PYRECT3D_SRC_DIR}/bindings/python/bind_explicit.cpp
      ${PYRECT3D_SRC_DIR}/bindings/python/bind_renderer.cpp
      ${PYRECT3D_SRC_DIR}/bindings/python/bind_gui.cpp
      ${PYRECT3D_SRC_DIR}/bindings/python/bind_collision.cpp
      ${PYRECT3D_SRC_DIR}/bindings/python/bind_media.cpp
    )
    target_include_directories(pyrect3d_binding PRIVATE ${PYRECT3D_INCLUDE_DIR})
    target_link_libraries(pyrect3d_binding PRIVATE PyRecT3D)
    set_target_properties(pyrect3d_binding PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/py/package"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/py/package"
      INSTALL_RPATH "$ORIGIN"
      BUILD_WITH_INSTALL_RPATH TRUE
    )
    if(PYRECT3D_LIB_TYPE STREQUAL "shared")
      add_custom_command(TARGET pyrect3d_binding POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:PyRecT3D>
                "${CMAKE_CURRENT_SOURCE_DIR}/py/package"
        COMMENT "Copy libPyRecT3D to py/package"
      )
    endif()
  else()
    message(WARNING "pybind11 / Python3 not found. Python binding skipped.")
  endif()
endif()

# ---------- C++ 原生示例 ----------
option(PYRECT3D_BUILD_EXAMPLES "Build C++ examples" ON)
if(PYRECT3D_BUILD_EXAMPLES)
  foreach(_ex basic_cube explicit_demo implicit_demo)
    add_executable(${_ex} ${CMAKE_CURRENT_SOURCE_DIR}/examples_cpp/${_ex}.cpp)
    target_include_directories(${_ex} PRIVATE ${PYRECT3D_INCLUDE_DIR})
    target_link_libraries(${_ex} PRIVATE PyRecT3D)
  endforeach()
endif()

# ---------- 测试 ----------
option(PYRECT3D_BUILD_TESTS "Build smoke tests" ON)
if(PYRECT3D_BUILD_TESTS)
  add_executable(smoke_v2 ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke_v2.cpp)
  target_include_directories(smoke_v2 PRIVATE ${PYRECT3D_INCLUDE_DIR})
  target_link_libraries(smoke_v2 PRIVATE PyRecT3D)
endif()
