// ============================================================
//  PyRecT3D — cpp/src/binding/bind_renderer.cpp
//  渲染器与合成层 的 Python 绑定
// ============================================================
#include "binding_common.h"

void bind_renderer(py::module_& m) {
    // ==========================================================
    //  renderer.h
    // ==========================================================
    py::enum_<RenderMode>(m, "RenderMode")
        .value("Forward", RenderMode::Forward)
        .value("Wireframe", RenderMode::Wireframe);

    py::class_<Renderer>(m, "Renderer")
        .def(py::init<>())
        .def("init", &Renderer::init, py::arg("width"), py::arg("height"))
        .def("shutdown", &Renderer::shutdown)
        .def("begin_frame", &Renderer::beginFrame, "清屏一次，之后可多次 draw_entity")
        .def("end_frame", &Renderer::endFrame)
        .def("width", &Renderer::width)
        .def("height", &Renderer::height)
        .def("set_mode", &Renderer::setMode, py::arg("mode"))
        .def("mode", &Renderer::mode)
        .def("set_clear_color", &Renderer::setClearColor,
             py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a") = 1.f)
        .def("draw_entity", &Renderer::drawEntity, py::arg("entity"))
        .def("draw_screen_rect", &Renderer::drawScreenRect,
             py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"))
        .def("set_anti_aliasing", &Renderer::setAntiAliasing, py::arg("on"))
        .def("save_frame", &Renderer::saveFrame, py::arg("path"));

    py::enum_<CommandType>(m, "CommandType")
        .value("Clear", CommandType::Clear)
        .value("DrawRect2D", CommandType::DrawRect2D)
        .value("DrawEntity3D", CommandType::DrawEntity3D)
        .value("SetUniform", CommandType::SetUniform)
        .value("DrawTexture", CommandType::DrawTexture)
        .value("Composite", CommandType::Composite)
        .value("Present", CommandType::Present);

    py::class_<RenderCommand>(m, "RenderCommand")
        .def(py::init<>())
        .def_readwrite("type", &RenderCommand::type)
        .def_readwrite("x", &RenderCommand::x)
        .def_readwrite("y", &RenderCommand::y)
        .def_readwrite("w", &RenderCommand::w)
        .def_readwrite("h", &RenderCommand::h)
        .def_readwrite("r", &RenderCommand::r)
        .def_readwrite("g", &RenderCommand::g)
        .def_readwrite("b", &RenderCommand::b)
        .def_readwrite("a", &RenderCommand::a)
        .def_readwrite("entity_id", &RenderCommand::entityId)
        .def_readwrite("texture_id", &RenderCommand::textureId)
        .def("is_2d", &RenderCommand::is2D);

    py::class_<DirtyRegion>(m, "DirtyRegion")
        .def(py::init<>())
        .def_readwrite("x", &DirtyRegion::x)
        .def_readwrite("y", &DirtyRegion::y)
        .def_readwrite("w", &DirtyRegion::w)
        .def_readwrite("h", &DirtyRegion::h)
        .def_readwrite("valid", &DirtyRegion::valid)
        .def("union_with", &DirtyRegion::unionWith, py::arg("other"));

    py::class_<Compositor>(m, "Compositor")
        .def(py::init<>())
        .def("attach", &Compositor::attach, py::arg("renderer"))
        .def("submit", [](Compositor& c, py::sequence cmds) {
            std::vector<RenderCommand> v;
            for (auto item : cmds) v.push_back(item.cast<RenderCommand>());
            if (!v.empty()) c.submit(v.data(), v.size());
        }, py::arg("cmds"), "提交渲染指令列表（内部自动合批合并）")
        .def("submit_rect", [](Compositor& c, float x, float y, float w, float h,
                               float r, float g, float b, float a) {
            RenderCommand cmd;
            cmd.type = CommandType::DrawRect2D;
            cmd.x = x; cmd.y = y; cmd.w = w; cmd.h = h;
            cmd.r = r; cmd.g = g; cmd.b = b; cmd.a = a;
            c.submit(&cmd, 1);
        }, py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"),
           py::arg("r") = 1.f, py::arg("g") = 1.f, py::arg("b") = 1.f, py::arg("a") = 1.f)
        .def("invalidate", &Compositor::invalidate, py::arg("region"))
        .def("dirty_region", &Compositor::dirtyRegion)
        .def("clear_dirty", &Compositor::clearDirty)
        .def("set_scene_dirty", &Compositor::setSceneDirty, py::arg("dirty"))
        .def("scene_dirty", &Compositor::sceneDirty)
        .def("apply_effect_3d", &Compositor::applyEffect3D,
             py::arg("effect_code"), py::arg("strength"))
        .def("present", &Compositor::present)
        .def("merged_count", &Compositor::mergedCount);

    py::class_<Cube, Entity>(m, "Cube")
        .def(py::init<Vec3, Vec3>(),
             py::arg("position") = Vec3(), py::arg("scale") = Vec3(1, 1, 1));

    py::class_<ScreenCube, Entity>(m, "ScreenCube")
        .def(py::init<float, float, float, float>(),
             py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"))
        .def("x", &ScreenCube::x)
        .def("y", &ScreenCube::y)
        .def("width", &ScreenCube::width)
        .def("height", &ScreenCube::height);

    py::class_<SoftRayTracer>(m, "SoftRayTracer")
        .def(py::init<>())
        .def("render", [](SoftRayTracer& t, const Scene& scene, int w, int h) -> py::object {
            std::vector<unsigned char> rgb;
            if (!t.render(scene, w, h, rgb)) return py::object(py::none());
            return py::bytes(reinterpret_cast<const char*>(rgb.data()), rgb.size());
        }, py::arg("scene"), py::arg("w"), py::arg("h"),
           "软件光追渲染，返回 RGB24 字节串（可直接写 PPM / 转 PIL Image）");
}
