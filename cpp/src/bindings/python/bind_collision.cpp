// ============================================================
//  PyRecT3D — cpp/src/binding/bind_collision.cpp
//  双碰撞箱与射线 的 Python 绑定
// ============================================================
#include "binding_common.h"

void bind_collision(py::module_& m) {
    // ==========================================================
    //  collision.h —— 双碰撞箱
    // ==========================================================
    py::enum_<CollisionEventKind>(m, "CollisionEventKind")
        .value("Enter", CollisionEventKind::Enter)
        .value("Exit", CollisionEventKind::Exit)
        .value("Stay", CollisionEventKind::Stay)
        .value("Press", CollisionEventKind::Press)
        .value("Release", CollisionEventKind::Release)
        .value("DragMove", CollisionEventKind::DragMove)
        .value("DragOut", CollisionEventKind::DragOut)
        .value("ClickOutside", CollisionEventKind::ClickOutside);

    py::class_<Limb>(m, "Limb")
        .def(py::init<>())
        .def_readwrite("name", &Limb::name)
        .def_readwrite("local_box", &Limb::localBox)
        .def_readwrite("offset", &Limb::offset)
        .def_readwrite("enabled", &Limb::enabled);

    py::class_<PhysicsCollider>(m, "PhysicsCollider")
        .def(py::init<>())
        .def_readwrite("box", &PhysicsCollider::box)
        .def_readwrite("enabled", &PhysicsCollider::enabled)
        .def_readwrite("use_raycast", &PhysicsCollider::useRaycast)
        .def_readwrite("ray_step", &PhysicsCollider::rayStep)
        .def_readwrite("max_distance", &PhysicsCollider::maxDistance);

    py::class_<ModelCollider>(m, "ModelCollider")
        .def(py::init<>())
        .def_readwrite("root", &ModelCollider::root)
        .def_readwrite("limbs", &ModelCollider::limbs)
        .def_readwrite("enabled", &ModelCollider::enabled)
        .def_readwrite("use_local_space", &ModelCollider::useLocalSpace)
        .def_readwrite("local_scale", &ModelCollider::localScale);

    py::class_<HitResult>(m, "HitResult")
        .def(py::init<>())
        .def_readonly("hit", &HitResult::hit)
        .def_readonly("hit_model", &HitResult::hitModel)
        .def_readonly("hit_physics", &HitResult::hitPhysics)
        .def_readonly("distance", &HitResult::distance)
        .def_readonly("point", &HitResult::point)
        .def_readonly("normal", &HitResult::normal)
        .def_readonly("entity_id", &HitResult::entityId)
        .def_readonly("limb_name", &HitResult::limbName);

    py::class_<CollisionWorld>(m, "CollisionWorld")
        .def(py::init<>())
        .def("add_physics_box", &CollisionWorld::addPhysicsBox, py::arg("collider"))
        .def("add_model_box", &CollisionWorld::addModelBox, py::arg("collider"))
        .def("update_physics_box", &CollisionWorld::updatePhysicsBox,
             py::arg("handle"), py::arg("collider"))
        .def("update_model_box", &CollisionWorld::updateModelBox,
             py::arg("handle"), py::arg("collider"))
        .def("remove", &CollisionWorld::remove, py::arg("handle"))
        .def("clear", &CollisionWorld::clear)
        .def("step", &CollisionWorld::step, py::arg("dt"))
        .def("raycast", &CollisionWorld::raycast,
             py::arg("ray"), py::arg("max_distance") = 100.f,
             py::arg("include_model") = true, py::arg("include_physics") = true)
        .def("add_trigger_zone", &CollisionWorld::addTriggerZone,
             py::arg("box"), py::arg("owner_entity_id") = 0)
        .def("set_event_handler", &CollisionWorld::setEventHandler,
             py::arg("kind"), py::arg("callback"))
        .def("clear_event_handlers", &CollisionWorld::clearEventHandlers)
        .def("process_pointer", &CollisionWorld::processPointer,
             py::arg("x"), py::arg("y"), py::arg("pressed"),
             py::arg("released"), py::arg("dt"));
}
