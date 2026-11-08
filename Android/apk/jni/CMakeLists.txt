# ============================================================
#  PyRecT3D — Android NDK 构建入口
#  引擎源码复用 cpp/src（新分层路径），渲染后端固定 GLES3。
#  用法（在项目根）：
#    cmake -S Android/apk/jni -B /tmp/apk-build \
#          -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
#          -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 \
#          -DCMAKE_BUILD_TYPE=Release
#    cmake --build /tmp/apk-build -j
# ============================================================
cmake_minimum_required(VERSION 3.22.1)
project(pyrect3d_android LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PYR3D_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(PYR3D_SRC ${PYR3D_ROOT}/cpp/src)
set(PYR3D_INC ${PYR3D_ROOT}/cpp/include)
set(PYR3D_GLAD ${PYR3D_ROOT}/cpp/third_party/glad)

# 静态库：随 APK 一并打包
add_library(pyrect3d STATIC
  ${PYR3D_SRC}/foundation/math/math.cpp
  ${PYR3D_SRC}/foundation/io/obj_loader.cpp
  ${PYR3D_SRC}/core/engine.cpp
  ${PYR3D_SRC}/core/event_loop.cpp
  ${PYR3D_SRC}/core/message_queue.cpp
  ${PYR3D_SRC}/core/explicit_app.cpp
  ${PYR3D_SRC}/core/implicit_api.cpp
  ${PYR3D_SRC}/core/platform/android_platform.cpp
  ${PYR3D_SRC}/graphics/renderer.cpp
  ${PYR3D_SRC}/graphics/compositor.cpp
  ${PYR3D_SRC}/graphics/gl_backend.cpp
  ${PYR3D_SRC}/ui/gui.cpp
  ${PYR3D_SRC}/media/video.cpp
  ${PYR3D_SRC}/physics/collision_world.cpp
  ${PYR3D_SRC}/audio/audio.cpp
)

target_include_directories(pyrect3d
  PUBLIC  ${PYR3D_INC}
  PRIVATE ${PYR3D_SRC} ${PYR3D_GLAD}
)

target_compile_definitions(pyrect3d PRIVATE
  PYRECT3D_PLATFORM_ANDROID=1
  # GL 后端暂时关闭：GLES3 接入由后续版本通过 Java GLSurfaceView +
  # native_window_* 完成，当前 .so 走软件光栅化路径，保证链接先过。
  PYRECT3D_WITH_GL=0
  PYRECT3D_WITH_FFMPEG=0
)

target_compile_options(pyrect3d PRIVATE -Wall -Wno-unused-parameter)

# JNI 胶水层：Java 层可加载的 .so
add_library(native_bridge SHARED native_bridge.cpp)
target_link_libraries(native_bridge PRIVATE pyrect3d android log EGL GLESv3 OpenSLES)
