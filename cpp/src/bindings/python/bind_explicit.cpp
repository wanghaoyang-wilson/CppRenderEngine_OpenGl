// ============================================================
//  PyRecT3D — cpp/src/binding/bind_explicit.cpp
//  显式 API 与隐式 quick 的 Python 绑定
// ============================================================
#include "binding_common.h"

static void bind_explicit_impl(py::module_& m) {
    // ==========================================================
    //  explicit.h
    // ==========================================================
    py::enum_<Backend>(m, "Backend")
        .value("Auto", Backend::Auto)
        .value("GL33", Backend::GL33)
        .value("GLES3", Backend::GLES3);

    py::enum_<Module>(m, "Module")
        .value("Renderer", Module::Renderer)
        .value("GUI", Module::GUI)
        .value("Video", Module::Video)
        .value("Audio", Module::Audio)
        .value("Physics", Module::Physics)
        .value("RayTracer", Module::RayTracer)
        .value("Python", Module::Python);

    py::class_<AppConfig>(m, "AppConfig")
        .def(py::init<>())
        .def_readwrite("width", &AppConfig::width)
        .def_readwrite("height", &AppConfig::height)
        .def_readwrite("title", &AppConfig::title)
        .def_readwrite("backend", &AppConfig::backend)
        .def_readwrite("vsync", &AppConfig::vsync)
        .def_readwrite("enable_video", &AppConfig::enableVideo)
        .def_readwrite("enable_audio", &AppConfig::enableAudio)
        .def_readwrite("enable_physics", &AppConfig::enablePhysics)
        .def_readwrite("enable_gui_local_update", &AppConfig::enableGUILocalUpdate)
        .def_readwrite("multithread_render", &AppConfig::multithreadRender)
        .def_readwrite("render_scale", &AppConfig::renderScale);

    py::class_<WindowDesc>(m, "WindowDesc")
        .def(py::init<>())
        .def_readwrite("width", &WindowDesc::width)
        .def_readwrite("height", &WindowDesc::height)
        .def_readwrite("title", &WindowDesc::title)
        .def_readwrite("resizable", &WindowDesc::resizable)
        .def_readwrite("fullscreen", &WindowDesc::fullscreen)
        .def_readwrite("ui_auto_bind", &WindowDesc::uiAutoBind);

    py::class_<UIInstance>(m, "UIInstance")
        .def("bind_window", &UIInstance::bindWindow, py::arg("window"))
        .def("bound_window_id", &UIInstance::boundWindowId)
        .def("gui", &UIInstance::gui, py::return_value_policy::reference_internal)
        .def("add_button", &UIInstance::addButton, py::arg("rect"), py::arg("text"),
             py::return_value_policy::reference_internal)
        .def("add_text", &UIInstance::addText, py::arg("rect"), py::arg("text"),
             py::return_value_policy::reference_internal)
        .def("add_input_box", &UIInstance::addInputBox, py::arg("rect"), py::arg("text"),
             py::return_value_policy::reference_internal)
        .def("add_container", &UIInstance::addContainer, py::arg("rect"),
             py::return_value_policy::reference_internal)
        .def("add_scroll_container", &UIInstance::addScrollContainer, py::arg("rect"),
             py::return_value_policy::reference_internal)
        .def("draw_all", &UIInstance::drawAll);

    py::class_<WindowInstance, std::unique_ptr<WindowInstance, py::nodelete>>(m, "WindowInstance")
        .def("id", &WindowInstance::id)
        .def("queue", &WindowInstance::queue, py::return_value_policy::reference_internal)
        .def("camera", &WindowInstance::camera, py::return_value_policy::reference_internal)
        .def("scene", &WindowInstance::scene, py::return_value_policy::reference_internal)
        .def("create_ui", &WindowInstance::createUI, py::return_value_policy::reference_internal)
        .def("render_scene", &WindowInstance::renderScene)
        .def("present", &WindowInstance::present)
        .def("set_title", &WindowInstance::setTitle, py::arg("title"))
        .def("width", &WindowInstance::width)
        .def("height", &WindowInstance::height);

    py::class_<ExplicitApp>(m, "ExplicitApp")
        .def(py::init<>())
        .def("configure", &ExplicitApp::configure, py::arg("cfg"),
             "必须在 create_window 之前调用")
        .def("create_window", &ExplicitApp::createWindow, py::arg("desc"),
             py::return_value_policy::reference_internal)
        .def("window", &ExplicitApp::window, py::arg("window_id"),
             py::return_value_policy::reference_internal)
        .def("window_count", &ExplicitApp::windowCount)
        .def("mount_queues", &ExplicitApp::mountQueues, "多窗口时必须调用")
        .def("create_video_player", &ExplicitApp::createVideoPlayer,
             py::arg("desc"), py::arg("bind_to") = py::none(),
             py::return_value_policy::reference_internal)
        .def("publish_global", &ExplicitApp::publishGlobal, py::arg("msg"))
        .def("run", &ExplicitApp::run)
        .def("pump", &ExplicitApp::pump)
        .def("quit", &ExplicitApp::quit)
        .def("has_module", &ExplicitApp::hasModule, py::arg("module"));
}

static void bind_implicit_impl(py::module_& m) {
    // ==========================================================
    //  implicit.h —— quick 子模块
    // ==========================================================
    py::class_<EntityHandle>(m, "EntityHandle")
        .def(py::init<>())
        .def_readwrite("id", &EntityHandle::id)
        .def("valid", &EntityHandle::valid)
        .def("__repr__", [](const EntityHandle& h) {
            return "EntityHandle(id=" + std::to_string(h.id) + ")";
        });

    py::module_ quick = m.def_submodule("quick",
        "隐式 API：单窗口自动弱绑定，最快入门（Scratch 式一行启动）");

    quick.def("run", &quick::run, py::arg("width") = 1280, py::arg("height") = 720,
              py::arg("title") = "PyRecT3D");
    quick.def("cube", &quick::cube,
              py::arg("x") = 0.f, py::arg("y") = 0.f, py::arg("z") = -5.f,
              py::arg("sx") = 1.f, py::arg("sy") = 1.f, py::arg("sz") = 1.f);
    quick.def("screen_cube", &quick::screenCube,
              py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"));
    quick.def("clone", &quick::clone, py::arg("src"),
              "复制场上现成实体的当前完整状态");
    quick.def("summon", &quick::summon,
              py::arg("prototype"), py::arg("x"), py::arg("y"), py::arg("z"),
              "按模板实体初始参数新建");
    quick.def("move", &quick::move, py::arg("e"), py::arg("x"), py::arg("y"), py::arg("z"));
    quick.def("rotate", &quick::rotate,
              py::arg("e"), py::arg("rx"), py::arg("ry"), py::arg("rz"));
    quick.def("scale", &quick::scale,
              py::arg("e"), py::arg("sx"), py::arg("sy"), py::arg("sz"));
    quick.def("remove", &quick::remove, py::arg("e"));
    quick.def("raycast", &quick::raycast,
              py::arg("ox"), py::arg("oy"), py::arg("oz"),
              py::arg("dx"), py::arg("dy"), py::arg("dz"),
              py::arg("max_distance") = 100.f);
    quick.def("button", &quick::button,
              py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"), py::arg("text"),
              py::return_value_policy::reference);
    quick.def("text", &quick::text,
              py::arg("x"), py::arg("y"), py::arg("content"),
              py::return_value_policy::reference);
    quick.def("video", &quick::video, py::arg("path"), py::arg("loop") = false,
              py::return_value_policy::reference);
    quick.def("scene", &quick::scene, py::return_value_policy::reference);
    quick.def("gui", &quick::gui, py::return_value_policy::reference);
}

void bind_explicit(py::module_& m) {
    bind_explicit_impl(m);
    bind_implicit_impl(m);
}
