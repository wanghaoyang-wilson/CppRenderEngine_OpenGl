// ============================================================
//  PyRecT3D — cpp/src/binding/bind_media.cpp
//  视频材质与音频 的 Python 绑定
// ============================================================
#include "binding_common.h"

static void bind_video(py::module_& m) {
    // ==========================================================
    //  video.h —— 全局 MediaManager 单例 + 多播放器实例
    // ==========================================================
    py::class_<VideoTexture>(m, "VideoTexture")
        .def(py::init<>())
        .def_readonly("id", &VideoTexture::id)
        .def_readonly("width", &VideoTexture::width)
        .def_readonly("height", &VideoTexture::height)
        .def_readonly("frame_serial", &VideoTexture::frameSerial)
        .def_readonly("valid", &VideoTexture::valid);

    py::class_<VideoDesc>(m, "VideoDesc")
        .def(py::init<>())
        .def_readwrite("path", &VideoDesc::path)
        .def_readwrite("loop", &VideoDesc::loop)
        .def_readwrite("decode_audio", &VideoDesc::decodeAudio)
        .def_readwrite("decode_threads", &VideoDesc::decodeThreads)
        .def_readwrite("volume", &VideoDesc::volume)
        .def_readwrite("auto_play", &VideoDesc::autoPlay);

    py::class_<VideoPlayer, std::unique_ptr<VideoPlayer, py::nodelete>>(m, "VideoPlayer")
        .def("id", &VideoPlayer::id)
        .def("bound_queue_id", &VideoPlayer::boundQueueId)
        .def("bind_queue", &VideoPlayer::bindQueue, py::arg("queue"),
             "多窗口时必须显式绑定子消息队列")
        .def("auto_bind", &VideoPlayer::autoBind,
             "单窗口时自动绑定主消息循环")
        .def("open", &VideoPlayer::open, py::arg("desc"))
        .def("play", &VideoPlayer::play)
        .def("pause", &VideoPlayer::pause)
        .def("stop", &VideoPlayer::stop)
        .def("seek", &VideoPlayer::seek, py::arg("seconds"))
        .def("playing", &VideoPlayer::playing)
        .def("duration", &VideoPlayer::duration)
        .def("position", &VideoPlayer::position)
        .def("texture", &VideoPlayer::texture,
             "返回每帧变化的动态材质，可贴到任意物体上")
        .def("upload_pending_frame", &VideoPlayer::uploadPendingFrame,
             "主线程每帧调用：检测子线程就绪帧并上传 GPU")
        .def("set_volume", &VideoPlayer::setVolume, py::arg("v"))
        .def("volume", &VideoPlayer::volume);

    py::class_<MediaManager, std::unique_ptr<MediaManager, py::nodelete>>(m, "MediaManager")
        .def_static("instance", &MediaManager::instance,
                    py::return_value_policy::reference,
                    "全局唯一实例：整个程序只有一个，不随窗口/UI 循环增加")
        .def("create_player", &MediaManager::createPlayer,
             py::return_value_policy::reference_internal)
        .def("destroy_player", &MediaManager::destroyPlayer, py::arg("player"))
        .def("tick", &MediaManager::tick)
        .def("player_count", &MediaManager::playerCount);
}

static void bind_extra(py::module_& m) {
    // ==========================================================
    //  audio.h / io.h
    // ==========================================================
    py::class_<Audio>(m, "Audio")
        .def(py::init<>())
        .def("play_music", &Audio::playMusic, py::arg("path"), py::arg("loop") = true)
        .def("stop_music", &Audio::stopMusic)
        .def("pause_music", &Audio::pauseMusic)
        .def("resume_music", &Audio::resumeMusic)
        .def("set_volume", &Audio::setVolume, py::arg("v"))
        .def("volume", &Audio::volume);

    py::class_<Material>(m, "Material")
        .def_readonly("name", &Material::name)
        .def_readonly("diffuse_tex", &Material::diffuseTex)
        .def_readonly("diffuse_color", &Material::diffuseColor)
        .def_readonly("specular_color", &Material::specularColor)
        .def_readonly("shininess", &Material::shininess)
        .def_readonly("has_texture", &Material::hasTexture);

    py::class_<ObjModel>(m, "ObjModel")
        .def_readonly("name", &ObjModel::name)
        .def_readonly("materials", &ObjModel::materials)
        .def_property_readonly("vertex_count", [](const ObjModel& o) {
            return o.mesh.positions.size() / 3;
        })
        .def_property_readonly("index_count", [](const ObjModel& o) {
            return o.mesh.indices.size();
        });

    py::class_<ObjLoader>(m, "ObjLoader")
        .def_static("load", [](const std::string& path) {
            ObjModel model;
            if (!ObjLoader::load(path, model)) return py::object(py::none());
            return py::cast(model);
        }, py::arg("path"))
        .def_static("load_from_memory", [](const std::string& content) {
            ObjModel model;
            if (!ObjLoader::loadFromMemory(content, model)) return py::object(py::none());
            return py::cast(model);
        }, py::arg("content"));
}

void bind_media(py::module_& m) {
    bind_video(m);
    bind_extra(m);
}
