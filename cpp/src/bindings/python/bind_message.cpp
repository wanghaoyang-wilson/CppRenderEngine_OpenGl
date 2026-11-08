// ============================================================
//  PyRecT3D — cpp/src/binding/bind_message.cpp
//  消息队列与事件优先级 的 Python 绑定
// ============================================================
#include "binding_common.h"

void bind_message(py::module_& m) {
    // ==========================================================
    //  message.h
    // ==========================================================
    py::enum_<EventPriority>(m, "EventPriority")
        .value("WindowInput", EventPriority::WindowInput)
        .value("Physics", EventPriority::Physics)
        .value("Collision", EventPriority::Collision)
        .value("Audio", EventPriority::Audio)
        .value("Media", EventPriority::Media)
        .value("Normal", EventPriority::Normal);

    py::enum_<EventType>(m, "EventType")
        .value("None", EventType::None)
        .value("WindowClose", EventType::WindowClose)
        .value("WindowMinimize", EventType::WindowMinimize)
        .value("WindowMaximize", EventType::WindowMaximize)
        .value("WindowRestore", EventType::WindowRestore)
        .value("WindowResize", EventType::WindowResize)
        .value("WindowBorderChange", EventType::WindowBorderChange)
        .value("WindowFocusGained", EventType::WindowFocusGained)
        .value("WindowFocusLost", EventType::WindowFocusLost)
        .value("KeyDown", EventType::KeyDown)
        .value("KeyUp", EventType::KeyUp)
        .value("MouseMove", EventType::MouseMove)
        .value("MouseButtonDown", EventType::MouseButtonDown)
        .value("MouseButtonUp", EventType::MouseButtonUp)
        .value("MouseWheel", EventType::MouseWheel)
        .value("AppQuit", EventType::AppQuit)
        .value("AppPause", EventType::AppPause)
        .value("AppResume", EventType::AppResume)
        .value("AppResumeFromBackground", EventType::AppResumeFromBackground)
        .value("InputBindingChanged", EventType::InputBindingChanged)
        .value("ViewportTransfer", EventType::ViewportTransfer)
        .value("SceneSwitch", EventType::SceneSwitch)
        .value("WindowCreate", EventType::WindowCreate)
        .value("WindowDestroy", EventType::WindowDestroy)
        .value("SceneCreate", EventType::SceneCreate)
        .value("SceneDestroy", EventType::SceneDestroy)
        .value("TransformMove", EventType::TransformMove)
        .value("TransformRotate", EventType::TransformRotate)
        .value("TransformScale", EventType::TransformScale)
        .value("EntityClone", EventType::EntityClone)
        .value("EntitySummon", EventType::EntitySummon)
        .value("EntityRemove", EventType::EntityRemove)
        .value("PhysicsTick", EventType::PhysicsTick)
        .value("CollisionEnter", EventType::CollisionEnter)
        .value("CollisionExit", EventType::CollisionExit)
        .value("CollisionStay", EventType::CollisionStay)
        .value("CollisionPress", EventType::CollisionPress)
        .value("CollisionRelease", EventType::CollisionRelease)
        .value("CollisionDragOut", EventType::CollisionDragOut)
        .value("CollisionDragMove", EventType::CollisionDragMove)
        .value("CollisionClickOutside", EventType::CollisionClickOutside)
        .value("RaycastHitReport", EventType::RaycastHitReport)
        .value("AudioFinished", EventType::AudioFinished)
        .value("VideoFinished", EventType::VideoFinished)
        .value("VideoPaused", EventType::VideoPaused)
        .value("VideoError", EventType::VideoError)
        .value("VideoFrameReady", EventType::VideoFrameReady)
        .value("RenderCommandSubmit", EventType::RenderCommandSubmit)
        .value("RenderRegionInvalidate", EventType::RenderRegionInvalidate);

    py::class_<MessagePayload>(m, "MessagePayload")
        .def(py::init<>())
        .def_readwrite("entity_id", &MessagePayload::entityId)
        .def_readwrite("window_id", &MessagePayload::windowId)
        .def_readwrite("player_id", &MessagePayload::playerId)
        .def_readwrite("queue_id", &MessagePayload::queueId)
        .def_readwrite("code", &MessagePayload::code)
        .def_readwrite("text", &MessagePayload::text)
        .def_property("v",
            [](const MessagePayload& p) {
                py::list out;
                for (int i = 0; i < 8; ++i) out.append(p.v[i]);
                return out;
            },
            [](MessagePayload& p, py::sequence seq) {
                int i = 0;
                for (auto item : seq) {
                    if (i >= 8) break;
                    p.v[i++] = item.cast<float>();
                }
            },
            "通用数值向量 v[0..7]（坐标 / 变换 / 拖拽量）");

    py::class_<Message>(m, "Message")
        .def(py::init<>())
        .def_readwrite("type", &Message::type)
        .def_readwrite("priority", &Message::priority)
        .def_readwrite("source_queue", &Message::sourceQueue)
        .def_readwrite("target_queue", &Message::targetQueue)
        .def_readwrite("payload", &Message::payload);

    py::class_<MessageQueue>(m, "MessageQueue")
        .def(py::init<std::int64_t>(), py::arg("queue_id") = 0)
        .def("id", &MessageQueue::id)
        .def("attach_to_main", &MessageQueue::attachToMain,
             "多窗口时必须显式挂载到主队列")
        .def("detach_from_main", &MessageQueue::detachFromMain)
        .def("attached", &MessageQueue::attached)
        .def("publish", &MessageQueue::publish, py::arg("msg"),
             "经主队列按优先级路由后回到本队列")
        .def("publish_local", &MessageQueue::publishLocal, py::arg("msg"))
        .def("pending", &MessageQueue::pending)
        .def("poll", [](MessageQueue& q) {
            Message out;
            if (!q.poll(out)) return py::object(py::none());
            return py::cast(out);
        })
        .def("set_handler", &MessageQueue::setHandler, py::arg("handler"))
        .def("dispatch", &MessageQueue::dispatch);

    py::class_<MainMessageQueue, std::unique_ptr<MainMessageQueue, py::nodelete>>(m, "MainMessageQueue")
        .def_static("instance", &MainMessageQueue::instance,
                    py::return_value_policy::reference)
        .def("publish", &MainMessageQueue::publish, py::arg("msg"))
        .def("pump", &MainMessageQueue::pump)
        .def("pending", &MainMessageQueue::pending)
        .def("subscribe", &MainMessageQueue::subscribe, py::arg("handler"))
        .def("child_count", &MainMessageQueue::childCount)
        .def("find_child", &MainMessageQueue::findChild, py::arg("queue_id"),
             py::return_value_policy::reference)
        .def("first_child", &MainMessageQueue::firstChild,
             py::return_value_policy::reference)
        .def("attach_child", &MainMessageQueue::attachChild, py::arg("child"))
        .def("detach_child", &MainMessageQueue::detachChild, py::arg("child"))
        .def("thread_id", &MainMessageQueue::threadId,
             "主队列绑定的线程 id（GL 上下文所在线程）");

    py::class_<App>(m, "App")
        .def_static("run", &App::run)
        .def_static("quit", &App::quit)
        .def_static("pump", &App::pump);
}
