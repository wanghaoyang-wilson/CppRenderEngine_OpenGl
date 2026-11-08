// ============================================================
//  PyRecT3D — Android JNI 胶水层（占位 / 接口契约）
//
//  职责：只做「平台事件 → 引擎消息队列」的翻译，不含业务逻辑。
//  渲染与场景全部在 C++ 引擎内完成；Java 层只是薄壳。
// ============================================================
#include <jni.h>
#include <android/log.h>
#include <PyRecT3D/message.h>
#include <PyRecT3D/explicit.h>
#include <cmath>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "PyRecT3D", __VA_ARGS__)

namespace {
pyrect3d::ExplicitApp* g_app = nullptr;
pyrect3d::WindowInstance* g_window = nullptr;

// 发布一条全局事件（窗口级输入最高优先级）
void publish(pyrect3d::EventType type, pyrect3d::EventPriority prio,
             int code, float x, float y, float joyX, float joyY) {
    pyrect3d::Message m;
    m.type = type;
    m.priority = prio;
    m.sourceQueue = 0;                 // 主消息队列身份
    m.payload.code = code;
    m.payload.v[0] = x;
    m.payload.v[1] = y;
    m.payload.v[2] = joyX;
    m.payload.v[3] = joyY;
    pyrect3d::MainMessageQueue::instance().publish(m);
}
} // namespace

extern "C" {

// ---------- 引擎生命周期 ----------
JNIEXPORT jboolean JNICALL
Java_org_pyrect3d_runtime_MainActivity_nativeInit(JNIEnv*, jobject, jint w, jint h) {
    g_app = new pyrect3d::ExplicitApp();

    pyrect3d::AppConfig cfg;
    cfg.backend = pyrect3d::Backend::GLES3;    // Android 固定 GLES3
    cfg.enableGUILocalUpdate = true;
    g_app->configure(cfg);

    pyrect3d::WindowDesc d;
    d.width = w; d.height = h; d.title = "PyRecT3D";
    g_window = g_app->createWindow(d);
    g_app->mountQueues();
    LOGI("nativeInit %dx%d", w, h);
    return g_window != nullptr ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_pyrect3d_runtime_MainActivity_nativePump(JNIEnv*, jobject) {
    if (!g_app) return JNI_FALSE;
    return g_app->pump() ? JNI_TRUE : JNI_FALSE;
}

// ---------- 生命周期：暂停必须释放 GPU 资源 ----------
JNIEXPORT void JNICALL
Java_org_pyrect3d_runtime_MainActivity_nativeOnPause(JNIEnv*, jobject) {
    publish(pyrect3d::EventType::AppPause, pyrect3d::EventPriority::WindowInput, 0, 0, 0, 0, 0);
    // 真实实现：release_gpu_resources() —— 幂等，防止后台被回收时显存泄漏
}

JNIEXPORT void JNICALL
Java_org_pyrect3d_runtime_MainActivity_nativeOnResume(JNIEnv*, jobject) {
    publish(pyrect3d::EventType::AppResume, pyrect3d::EventPriority::WindowInput, 0, 0, 0, 0, 0);
}

JNIEXPORT void JNICALL
Java_org_pyrect3d_runtime_MainActivity_nativeOnDestroy(JNIEnv*, jobject) {
    delete g_app;
    g_app = nullptr;
    g_window = nullptr;
}

// ---------- 触摸输入：转引擎事件（含摇杆向量钳位） ----------
JNIEXPORT void JNICALL
Java_org_pyrect3d_runtime_MainActivity_nativeTouch(
        JNIEnv*, jobject, jint action, jfloat x, jfloat y,
        jfloat joyX, jfloat joyY) {
    // 摇杆向量钳位：模长 <= 1，避免斜向 1.414 倍加速
    float len = (joyX * joyX + joyY * joyY);
    if (len > 1.f) {
        float l = std::sqrt(len);
        joyX /= l; joyY /= l;
    }
    publish(pyrect3d::EventType::MouseMove, pyrect3d::EventPriority::WindowInput,
            action, x, y, joyX, joyY);
}

} // extern "C"
