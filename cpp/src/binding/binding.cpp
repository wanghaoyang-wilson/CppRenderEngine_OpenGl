// ============================================================
//  PyRecT3D — cpp/src/binding/binding.cpp
//  pybind11 模块入口：只负责建模块与按章节调用各 bind_* 注册函数
//
//  拆分原则：一个头文件 → 一个 bind_*.cpp，便于单独维护与按需裁剪。
// ============================================================
#include "binding_common.h"

PYBIND11_MODULE(pyrect3d_binding, m) {
    m.doc() = "PyRecT3D v0.2 — C++/OpenGL 2D/3D 渲染引擎的 Python 绑定"
              "（显式 API + 隐式 API / 消息队列 / 双碰撞箱 / 视频材质）";
    m.attr("__version__") = "0.2.0";

    bind_math(m);
    bind_core(m);
    bind_message(m);
    bind_explicit(m);
    bind_renderer(m);
    bind_gui(m);
    bind_collision(m);
    bind_media(m);
}
