// ============================================================
//  PyRecT3D — binding.cpp
//  pybind11 Python 绑定：把 C++ 引擎 API 导出给 Python
//  （与 C++ 端同一套底层引擎逻辑，Python 端同等使用全部功能）
// ============================================================
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include <PyRecT3D/core.h>
#include <PyRecT3D/renderer.h>
#include <PyRecT3D/gui.h>
#include <PyRecT3D/io.h>
#include <PyRecT3D/audio.h>
#include <PyRecT3D/math.h>

namespace py = pybind11;
using namespace pyrect3d;

PYBIND11_MODULE(pyrect3d_binding, m) {
    m.doc() = "PyRecT3D — C++ OpenGL 2D/3D rendering engine, Ursina-like API";

    // ---------- 数学 ----------
    py::class_<Vec3>(m, "Vec3")
        .def(py::init<>())
        .def(py::init<float,float,float>(), py::arg("x"), py::arg("y"), py::arg("z"))
        .def_readwrite("x", &Vec3::x)
        .def_readwrite("y", &Vec3::y)
        .def_readwrite("z", &Vec3::z)
        .def("length", &Vec3::length)
        .def("normalized", &Vec3::normalized)
        .def("dot", &Vec3::dot)
        .def("cross", &Vec3::cross);

    py::class_<Ray>(m, "Ray")
        .def(py::init<Vec3, Vec3>(), py::arg("origin"), py::arg("direction"))
        .def_readwrite("origin", &Ray::origin)
        .def_readwrite("direction", &Ray::direction);

    py::class_<RaycastHit>(m, "RaycastHit")
        .def_readonly("hit", &RaycastHit::hit)
        .def_readonly("distance", &RaycastHit::distance)
        .def_readonly("entity_id", &RaycastHit::entityId);

    // ---------- 输入按键 ----------
    py::enum_<Key>(m, "Key")
        .value("W", Key::W).value("A", Key::A).value("S", Key::S).value("D", Key::D)
        .value("Space", Key::Space).value("Escape", Key::Escape).value("AltF4", Key::AltF4)
        .value("MouseLeft", Key::MouseLeft).value("MouseRight", Key::MouseRight)
        .value("MouseMiddle", Key::MouseMiddle);

    py::class_<InputState>(m, "InputState")
        .def("is_down", &InputState::isDown, py::arg("key"))
        .def_readonly("mouse_x", &InputState::mouseX)
        .def_readonly("mouse_y", &InputState::mouseY)
        .def_readonly("mouse_locked", &InputState::mouseLocked);

    // ---------- 实体 ----------
    py::class_<Entity>(m, "Entity")
        .def("set_position", &Entity::setPosition)
        .def("position", &Entity::position)
        .def("set_scale", &Entity::setScale)
        .def("scale", &Entity::scale)
        .def("set_rotation", &Entity::setRotation)
        .def("rotation", &Entity::rotation)
        .def("id", &Entity::id);

    // ---------- 场景 ----------
    py::class_<Scene>(m, "Scene")
        .def(py::init<>())
        .def("add_entity", &Scene::addEntity)
        .def("remove_entity", &Scene::removeEntity)
        .def("get_entity", &Scene::getEntity, py::return_value_policy::reference)
        .def("raycast", &Scene::raycast);

    // ---------- 引擎 ----------
    py::class_<Engine>(m, "Engine")
        .def(py::init<>())
        .def("init", &Engine::init, py::arg("width"), py::arg("height"),
             py::arg("title"), py::arg("native_window") = nullptr)
        .def("shutdown", &Engine::shutdown)
        .def("run", &Engine::run)
        .def("pump", &Engine::pump)
        .def("request_close", &Engine::requestClose)
        .def("set_frame_callback", &Engine::setFrameCallback)
        .def("input", &Engine::input, py::return_value_policy::reference)
        .def("lock_mouse", &Engine::lockMouse);

    // ---------- 渲染 ----------
    py::class_<Renderer>(m, "Renderer")
        .def(py::init<>())
        .def("init", &Renderer::init)
        .def("set_clear_color", &Renderer::setClearColor)
        .def("draw_entity", &Renderer::drawEntity)
        .def("set_anti_aliasing", &Renderer::setAntiAliasing)
        .def("save_frame", &Renderer::saveFrame);

    py::class_<Cube, Entity>(m, "Cube")
        .def(py::init<Vec3, Vec3>(), py::arg("position") = Vec3(),
             py::arg("scale") = Vec3(1,1,1));

    // ---------- GUI ----------
    py::class_<Rect>(m, "Rect")
        .def(py::init<>())
        .def(py::init<float,float,float,float>(), py::arg("x"), py::arg("y"),
             py::arg("w"), py::arg("h"));

    py::class_<Widget>(m, "Widget")
        .def("set_rect", &Widget::setRect)
        .def("rect", &Widget::rect)
        .def("set_visible", &Widget::setVisible)
        .def("set_texture", &Widget::setTexture);

    py::class_<Button, Widget>(m, "Button")
        .def(py::init<Rect, std::string>(), py::arg("rect"), py::arg("text") = "")
        .def_readwrite("text", &Button::text)
        .def_readwrite("on_click", &Button::onClick)
        .def_readwrite("on_hover", &Button::onHover);

    py::class_<Text, Widget>(m, "Text")
        .def(py::init<Rect, std::string>(), py::arg("rect"), py::arg("text") = "")
        .def_readwrite("text", &Text::text);

    py::class_<InputBox, Widget>(m, "InputBox")
        .def(py::init<Rect, std::string>(), py::arg("rect"), py::arg("text") = "")
        .def_readwrite("text", &InputBox::text);

    py::class_<Container, Widget>(m, "Container")
        .def(py::init<Rect>(), py::arg("rect"))
        .def("add_child", &Container::addChild);

    py::class_<ScrollContainer, Container>(m, "ScrollContainer")
        .def(py::init<Rect>(), py::arg("rect"))
        .def("set_scroll", &ScrollContainer::setScroll)
        .def("scroll", &ScrollContainer::scroll);

    py::class_<Gui>(m, "Gui")
        .def(py::init<>())
        .def("add_widget", &Gui::addWidget)
        .def("remove_widget", &Gui::removeWidget)
        .def("dispatch", &Gui::dispatch)
        .def("draw_all", &Gui::drawAll);

    // ---------- IO ----------
    py::class_<ObjModel>(m, "ObjModel")
        .def_readwrite("name", &ObjModel::name)
        .def_readwrite("materials", &ObjModel::materials);

    py::class_<Material>(m, "Material")
        .def_readwrite("name", &Material::name)
        .def_readwrite("diffuse_tex", &Material::diffuseTex);

    py::class_<ObjLoader>(m, "ObjLoader")
        .def_static("load", &ObjLoader::load, py::arg("obj_path"), py::arg("out"));

    // ---------- 音频 ----------
    py::class_<Audio>(m, "Audio")
        .def(py::init<>())
        .def("play_music", &Audio::playMusic)
        .def("stop_music", &Audio::stopMusic)
        .def("pause_music", &Audio::pauseMusic)
        .def("resume_music", &Audio::resumeMusic)
        .def("set_volume", &Audio::setVolume)
        .def("volume", &Audio::volume);

    // ---------- 软光追 ----------
    py::class_<SoftRayTracer>(m, "SoftRayTracer")
        .def(py::init<>())
        .def("render", &SoftRayTracer::render);
}
