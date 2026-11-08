// ============================================================
//  PyRecT3D — cpp/src/binding/bind_gui.cpp
//  自研 GUI 组件 的 Python 绑定
// ============================================================
#include "binding_common.h"

void bind_gui(py::module_& m) {
    // ==========================================================
    //  gui.h
    // ==========================================================
    py::class_<Rect>(m, "Rect")
        .def(py::init<>())
        .def(py::init<float, float, float, float>(),
             py::arg("x") = 0.f, py::arg("y") = 0.f, py::arg("w") = 0.f, py::arg("h") = 0.f)
        .def_readwrite("x", &Rect::x)
        .def_readwrite("y", &Rect::y)
        .def_readwrite("w", &Rect::w)
        .def_readwrite("h", &Rect::h)
        .def("contains", &Rect::contains, py::arg("px"), py::arg("py"));

    py::class_<GuiEvent>(m, "GuiEvent")
        .def(py::init<>())
        .def_readwrite("hovered", &GuiEvent::hovered)
        .def_readwrite("clicked", &GuiEvent::clicked)
        .def_readwrite("entered", &GuiEvent::entered)
        .def_readwrite("exited", &GuiEvent::exited)
        .def_readwrite("pressed", &GuiEvent::pressed)
        .def_readwrite("released", &GuiEvent::released)
        .def_readwrite("drag_move", &GuiEvent::dragMove)
        .def_readwrite("dragged_out", &GuiEvent::draggedOut)
        .def_readwrite("click_outside", &GuiEvent::clickOutside)
        .def_readwrite("mouse_x", &GuiEvent::mouseX)
        .def_readwrite("mouse_y", &GuiEvent::mouseY)
        .def_readwrite("drag_dx", &GuiEvent::dragDx)
        .def_readwrite("drag_dy", &GuiEvent::dragDy);

    py::class_<Widget>(m, "Widget")
        .def("set_rect", &Widget::setRect, py::arg("rect"))
        .def("rect", &Widget::rect)
        .def("set_visible", &Widget::setVisible, py::arg("v"))
        .def("visible", &Widget::visible)
        .def("set_texture", &Widget::setTexture, py::arg("tex_path"))
        .def("texture", &Widget::texture);

    py::class_<Button, Widget>(m, "Button")
        .def(py::init<const Rect&, const std::string&>(),
             py::arg("rect"), py::arg("text") = "")
        .def_readwrite("text", &Button::text)
        .def_readwrite("on_click", &Button::onClick)
        .def_readwrite("on_hover", &Button::onHover)
        .def_readwrite("on_enter", &Button::onEnter)
        .def_readwrite("on_exit", &Button::onExit)
        .def_readwrite("on_press", &Button::onPress)
        .def_readwrite("on_release", &Button::onRelease)
        .def_readwrite("on_drag_move", &Button::onDragMove)
        .def_readwrite("on_drag_out", &Button::onDragOut)
        .def_readwrite("on_click_outside", &Button::onClickOutside)
        .def("on_event", &Button::onEvent, py::arg("event"))
        .def("draw", &Button::draw);

    py::class_<Text, Widget>(m, "Text")
        .def(py::init<const Rect&, const std::string&>(),
             py::arg("rect"), py::arg("text") = "")
        .def_readwrite("text", &Text::text)
        .def_readwrite("size", &Text::size)
        .def("draw", &Text::draw);

    py::class_<InputBox, Widget>(m, "InputBox")
        .def(py::init<const Rect&, const std::string&>(),
             py::arg("rect"), py::arg("text") = "")
        .def_readwrite("text", &InputBox::text)
        .def_readwrite("focused", &InputBox::focused)
        .def("on_event", &InputBox::onEvent, py::arg("event"))
        .def("draw", &InputBox::draw);

    py::class_<Container, Widget>(m, "Container")
        .def(py::init<const Rect&>(), py::arg("rect"))
        .def("add_child", &Container::addChild, py::arg("widget"))
        .def("draw", &Container::draw);

    py::class_<ScrollContainer, Container>(m, "ScrollContainer")
        .def(py::init<const Rect&>(), py::arg("rect"))
        .def("on_event", &ScrollContainer::onEvent, py::arg("event"))
        .def("set_scroll", &ScrollContainer::setScroll, py::arg("offset"))
        .def("scroll", &ScrollContainer::scroll);

    py::class_<Gui>(m, "Gui")
        .def(py::init<>())
        .def("add_widget", &Gui::addWidget, py::arg("widget"))
        .def("remove_widget", &Gui::removeWidget, py::arg("widget"))
        .def("dispatch", &Gui::dispatch, py::arg("base_event"))
        .def("draw_all", &Gui::drawAll);
}
