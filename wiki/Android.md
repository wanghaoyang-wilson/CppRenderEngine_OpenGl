# Android

策略：**原生 C++（NDK）为主，Java/Kotlin 只做薄壳**。渲染后端固定 **GLES3**，与 Web 共用同一份渲染实现。

> 当前 `Android/apk/` 为**占位契约**：`AndroidManifest.xml`、`build.gradle`、`jni/CMakeLists.txt`、`jni/native_bridge.cpp` 已锁定目录结构与接口，完整 APK 链路在后续版本补齐。

## 目录

```
Android/
├── README.md
└── apk/
    ├── AndroidManifest.xml   # 清单占位（GLES3 feature / minSdk 21）
    ├── build.gradle          # Gradle 占位（arm64-v8a + armeabi-v7a）
    └── jni/
        ├── CMakeLists.txt    # NDK 构建入口（复用 cpp/src，静态库 + bridge）
        └── native_bridge.cpp # JNI 胶水层占位（生命周期 / 触摸 / 摇杆钳位）
```

## JNI 胶水层必做事件表

| 方向 | 事件 | 处理 |
|------|------|------|
| Java → C++ | `onSurfaceCreated` | 创建 EGL 上下文（≈ 桌面的 WGL/GLX） |
| Java → C++ | `onSurfaceChanged` | 更新视口尺寸，标记渲染脏区域 |
| Java → C++ | `onTouchEvent` | 转 `InputState` / 摇杆向量，入主消息队列 |
| Java → C++ | `onPause` | 发布 `AppPause`，调用 `release_gpu_resources()` |
| Java → C++ | `onResume` | 发布 `AppResume` |
| Java → C++ | `onDestroy` | 销毁全部 GPU 资源 |
| C++ → Java | 无 | 引擎不回调 Java（除退出/错误提示） |

## 关键原则

1. **GLES3 上下文只能在一个线程使用**：所有 `gl*` 调用固定在渲染线程，其它线程通过消息队列投递渲染指令。
2. **斜向移动**：摇杆输入进入引擎前做向量钳位（模长 ≤ 1），与桌面 WASD 一致。
3. **生命周期**：Android 随时可能被回收，`release_gpu_resources()` 必须**幂等**。
4. **ABI**：`arm64-v8a`（主）+ `armeabi-v7a`（老设备）。
5. **不引入 Xcode / Objective-C**：苹果全家桶一律不适配。

## 构建（待补）

```bash
export ANDROID_NDK=/path/to/ndk
cd Android/apk
./gradlew assembleRelease
# 产物：app/build/outputs/apk/release/app-release.apk
```

---

返回 [[Home]] · 也可走 [[WASM Packaging]] 把同一套代码跑在浏览器里
