// ============================================================
//  PyRecT3D — cpp/src/binding/binding_common.h
//  Python 绑定层公共头：统一 include 与命名空间，供各 bind_*.cpp 复用
// ============================================================
#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <memory>

#include <PyRecT3D/math.h>
#include <PyRecT3D/core.h>
#include <PyRecT3D/message.h>
#include <PyRecT3D/explicit.h>
#include <PyRecT3D/implicit.h>
#include <PyRecT3D/renderer.h>
#include <PyRecT3D/gui.h>
#include <PyRecT3D/collision.h>
#include <PyRecT3D/video.h>
#include <PyRecT3D/audio.h>
#include <PyRecT3D/io.h>

namespace py = pybind11;
using namespace pyrect3d;

// 各模块注册入口（按 docs/API.md 章节划分，一一对应）
void bind_math(py::module_& m);
void bind_core(py::module_& m);
void bind_message(py::module_& m);
void bind_explicit(py::module_& m);   // 含隐式 quick 子模块
void bind_renderer(py::module_& m);
void bind_gui(py::module_& m);
void bind_collision(py::module_& m);
void bind_media(py::module_& m);      // 视频 + 音频 + 模型 IO
