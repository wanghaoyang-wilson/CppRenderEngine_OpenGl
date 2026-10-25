cmake_minimum_required(VERSION 3.10)
project(PyRecT3D VERSION 0.1.0 LANGUAGES CXX)

# ============================================================
#  PyRecT3D — C++ OpenGL 2D/3D 渲染引擎 + pybind11 Python 绑定
#  平台：Windows (最低 Win7) / Linux
#  图形：OpenGL 3.3 Core
#  构建：源码优先编译，不加载任何预编译二进制；不使用预编译头
# ============================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---------- 对外公共头目录 ----------
set(PYRECT3D_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include")
set(PYRECT3D_SRC_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src")

# ---------- 引擎内核核心库 ----------
add_library(PyRecT3D SHARED
  ${PYRECT3D_SRC_DIR}/core/engine.cpp
  ${PYRECT3D_SRC_DIR}/core/event_loop.cpp
  ${PYRECT3D_SRC_DIR}/core/platform/platform_private.h
  ${PYRECT3D_SRC_DIR}/math/math.cpp
  ${PYRECT3D_SRC_DIR}/renderer/renderer.cpp
  ${PYRECT3D_SRC_DIR}/renderer/soft_raytracer.cpp
  ${PYRECT3D_SRC_DIR}/gui/gui.cpp
  ${PYRECT3D_SRC_DIR}/io/obj_loader.cpp
  ${PYRECT3D_SRC_DIR}/audio/audio.cpp
)
target_include_directories(PyRecT3D PUBLIC ${PYRECT3D_INCLUDE_DIR})
target_compile_definitions(PyRecT3D PRIVATE PYRECT3D_BUILD_DLL=1)

# Windows 平台
if(WIN32)
  target_compile_definitions(PyRecT3D PRIVATE _WIN32_WINNT=0x0601) # Win7 最低
  target_link_libraries(PyRecT3D PRIVATE
    opengl32
    user32
    gdi32
    winmm      # Windows 多媒体（音频）
    ole32
  )
  # Win7 兼容：禁用较新的 DPI 上下文，使用经典 Win32
  if(MSVC)
    target_compile_options(PyRecT3D PRIVATE /W4)
  endif()
endif()

# Linux 平台
if(UNIX AND NOT APPLE)
  target_link_libraries(PyRecT3D PRIVATE
    X11       # X11 输入/窗口（不默认 Wayland）
    GL
    dl
    pthread
  )
  target_compile_options(PyRecT3D PRIVATE -Wall -Wextra)
endif()

# ---------- 可选：pybind11 Python 绑定 ----------
option(PYRECT3D_BUILD_PYTHON "Build Python binding with pybind11" ON)
if(PYRECT3D_BUILD_PYTHON)
  find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
  # 内置 pybind11：优先使用系统 find_package，否则跳过（可后续安装）
  find_package(pybind11 CONFIG QUIET)
  if(pybind11_FOUND)
    pybind11_add_module(pyrect3d_binding
      ${PYRECT3D_SRC_DIR}/binding/binding.cpp
    )
    target_include_directories(pyrect3d_binding PRIVATE ${PYRECT3D_INCLUDE_DIR})
    target_link_libraries(pyrect3d_binding PRIVATE PyRecT3D)
    # 把产物复制到 py/package/
    set_target_properties(pyrect3d_binding PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/py/package"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/py/package"
    )
  else()
    message(WARNING "pybind11 not found via CMake. Python binding skipped. Install pybind11 (pip install pybind11) and rerun.")
  endif()
endif()

# ---------- C++ 原生示例 ----------
option(PYRECT3D_BUILD_EXAMPLES "Build C++ examples" ON)
if(PYRECT3D_BUILD_EXAMPLES)
  add_executable(basic_cube ${CMAKE_CURRENT_SOURCE_DIR}/examples_cpp/basic_cube.cpp)
  target_include_directories(basic_cube PRIVATE ${PYRECT3D_INCLUDE_DIR})
  target_link_libraries(basic_cube PRIVATE PyRecT3D)
endif()
