// ============================================================
//  PyRecT3D — cpp/src/binding/bind_math.cpp
//  math 类型 的 Python 绑定
// ============================================================
#include "binding_common.h"

void bind_math(py::module_& m) {
    // ==========================================================
    //  math.h
    // ==========================================================
    py::class_<Vec2>(m, "Vec2")
        .def(py::init<>())
        .def(py::init<float, float>(), py::arg("x"), py::arg("y"))
        .def_readwrite("x", &Vec2::x)
        .def_readwrite("y", &Vec2::y)
        .def("length", &Vec2::length)
        .def("normalized", &Vec2::normalized)
        .def("dot", &Vec2::dot, py::arg("other"));

    py::class_<Vec3>(m, "Vec3")
        .def(py::init<>())
        .def(py::init<float, float, float>(), py::arg("x"), py::arg("y"), py::arg("z"))
        .def_readwrite("x", &Vec3::x)
        .def_readwrite("y", &Vec3::y)
        .def_readwrite("z", &Vec3::z)
        .def("length", &Vec3::length)
        .def("normalized", &Vec3::normalized)
        .def("dot", &Vec3::dot, py::arg("other"))
        .def("cross", &Vec3::cross, py::arg("other"));

    py::class_<Vec4>(m, "Vec4")
        .def(py::init<>())
        .def(py::init<float, float, float, float>(),
             py::arg("x"), py::arg("y"), py::arg("z"), py::arg("w"))
        .def_readwrite("x", &Vec4::x)
        .def_readwrite("y", &Vec4::y)
        .def_readwrite("z", &Vec4::z)
        .def_readwrite("w", &Vec4::w);

    py::class_<Mat4>(m, "Mat4")
        .def(py::init<>())
        .def_static("perspective", &Mat4::perspective,
                    py::arg("fov_deg"), py::arg("aspect"), py::arg("near_p"), py::arg("far_p"))
        .def_static("orthographic", &Mat4::orthographic,
                    py::arg("left"), py::arg("right"), py::arg("bottom"),
                    py::arg("top"), py::arg("near_p"), py::arg("far_p"))
        .def_static("look_at", &Mat4::lookAt,
                    py::arg("eye"), py::arg("center"), py::arg("up"))
        .def_static("translation", &Mat4::translation, py::arg("t"))
        .def_static("scale", &Mat4::scale, py::arg("s"))
        .def_static("rotation_x", &Mat4::rotationX, py::arg("deg"))
        .def_static("rotation_y", &Mat4::rotationY, py::arg("deg"))
        .def_static("rotation_z", &Mat4::rotationZ, py::arg("deg"));

    py::class_<Ray>(m, "Ray")
        .def(py::init<>())
        .def(py::init<Vec3, Vec3>(), py::arg("origin"), py::arg("direction"))
        .def_readwrite("origin", &Ray::origin)
        .def_readwrite("direction", &Ray::direction);

    py::class_<RaycastHit>(m, "RaycastHit")
        .def(py::init<>())
        .def_readonly("hit", &RaycastHit::hit)
        .def_readonly("distance", &RaycastHit::distance)
        .def_readonly("point", &RaycastHit::point)
        .def_readonly("normal", &RaycastHit::normal)
        .def_readonly("entity_id", &RaycastHit::entityId);

    py::class_<AABB>(m, "AABB")
        .def(py::init<>())
        .def(py::init<Vec3, Vec3>(), py::arg("min"), py::arg("max"))
        .def_readwrite("min", &AABB::min)
        .def_readwrite("max", &AABB::max)
        .def("contains", &AABB::contains, py::arg("p"))
        .def("intersect", [](const AABB& a, const Ray& r) {
            float tNear = 0.f, tFar = 0.f;
            bool ok = a.intersect(r, tNear, tFar);
            return py::make_tuple(ok, tNear, tFar);
        }, py::arg("ray"));
}
