// ============================================================
//  PyRecT3D — cpp/src/binding/bind_core.cpp
//  核心实体与输入 的 Python 绑定
// ============================================================
#include "binding_common.h"

void bind_core(py::module_& m) {
    // ==========================================================
    //  core.h
    // ==========================================================
    py::enum_<Key>(m, "Key")
        .value("Unknown", Key::Unknown)
        .value("W", Key::W).value("A", Key::A).value("S", Key::S).value("D", Key::D)
        .value("Space", Key::Space).value("Escape", Key::Escape).value("AltF4", Key::AltF4)
        .value("MouseLeft", Key::MouseLeft)
        .value("MouseRight", Key::MouseRight)
        .value("MouseMiddle", Key::MouseMiddle);

    py::class_<InputState>(m, "InputState")
        .def("is_down", &InputState::isDown, py::arg("key"))
        .def_readonly("mouse_x", &InputState::mouseX)
        .def_readonly("mouse_y", &InputState::mouseY)
        .def_readonly("mouse_dx", &InputState::mouseDeltaX)
        .def_readonly("mouse_dy", &InputState::mouseDeltaY)
        .def_readonly("mouse_locked", &InputState::mouseLocked);

    py::class_<Entity>(m, "Entity")
        .def("id", &Entity::id)
        .def("set_position", &Entity::setPosition, py::arg("p"))
        .def("position", &Entity::position)
        .def("set_scale", &Entity::setScale, py::arg("s"))
        .def("scale", &Entity::scale)
        .def("set_rotation", &Entity::setRotation, py::arg("euler_deg"))
        .def("rotation", &Entity::rotation)
        .def("bounds", &Entity::bounds);

    py::class_<Scene>(m, "Scene")
        .def(py::init<>())
        .def("add_entity", &Scene::addEntity)
        .def("remove_entity", &Scene::removeEntity, py::arg("entity_id"))
        .def("get_entity", &Scene::getEntity, py::arg("entity_id"),
             py::return_value_policy::reference_internal)
        .def("clone_entity", &Scene::cloneEntity, py::arg("entity_id"),
             "克隆：复制场上现成实体的当前状态，返回新实体 id")
        .def("summon_entity", &Scene::summonEntity,
             py::arg("prototype_id"), py::arg("position"),
             "召唤：按模板实体的初始参数在指定位置新建")
        .def("raycast", &Scene::raycast, py::arg("ray"))
        .def("entity_count", &Scene::entityCount)
        .def("for_each", [](Scene& s, py::function fn) {
            s.forEach([&](std::int64_t id, Entity& e) {
                fn(id, e.position().x, e.position().y, e.position().z);
            });
        }, py::arg("fn"), "遍历场景：(id, x, y, z)");

    py::class_<Engine>(m, "Engine")
        .def(py::init<>())
        .def("init", [](Engine& e, int w, int h, const std::string& title) {
            return e.init(w, h, title);
        }, py::arg("width"), py::arg("height"), py::arg("title"))
        .def("shutdown", &Engine::shutdown)
        .def("run", &Engine::run)
        .def("pump", &Engine::pump)
        .def("request_close", &Engine::requestClose)
        .def("input", &Engine::input, py::return_value_policy::reference_internal)
        .def("lock_mouse", &Engine::lockMouse, py::arg("locked"));
}
