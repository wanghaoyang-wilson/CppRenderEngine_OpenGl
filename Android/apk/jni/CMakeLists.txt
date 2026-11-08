# ============================================================
#  PyRecT3D — Android NDK 构建入口（占位）
#  引擎源码复用 cpp/src，渲染后端固定 GLES3（与 WEB 共用）
# ============================================================
cmake_minimum_required(VERSION 3.22.1)
project(pyrect3d_android LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(PYR3D_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(PYR3D_SRC ${PYR3D_ROOT}/cpp/src)
set(PYR3D_INC ${PYR3D_ROOT}/cpp/include)

# 原生引擎（静态库，随 APK 打包）
add_library(pyrect3d STATIC
  ${PYR3D_SRC}/core/engine.cpp
  ${PYR3D_SRC}/core/event_loop.cpp
  ${PYR3D_SRC}/core/message_queue.cpp
  ${PYR3D_SRC}/core/explicit_app.cpp
  ${PYR3D_SRC}/core/implicit_api.cpp
  ${PYR3D_SRC}/math/math.cpp
  ${PYR3D_SRC}/renderer/renderer.cpp
  ${PYR3D_SRC}/renderer/compositor.cpp
  ${PYR3D_SRC}/gui/gui.cpp
  ${PYR3D_SRC}/media/video.cpp
  ${PYR3D_SRC}/physics/collision_world.cpp
  ${PYR3D_SRC}/io/obj_loader.cpp
  ${PYR3D_SRC}/audio/audio.cpp
)

target_include_directories(pyrect3d PUBLIC ${PYR3D_INC} PRIVATE ${PYR3D_SRC})
target_compile_definitions(pyrect3d PRIVATE
  PYRECT3D_PLATFORM_ANDROID=1
  PYRECT3D_WITH_VIDEO=1
  PYRECT3D_WITH_PHYSICS=1
)
target_link_libraries(pyrect3d PUBLIC android log EGL GLESv3 OpenSLES)

# JNI 胶水层
add_library(native_bridge SHARED native_bridge.cpp)
target_link_libraries(native_bridge PRIVATE pyrect3d)
