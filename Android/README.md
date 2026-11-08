# PyRecT3D — Android 平台

本目录是 Android 端目录。策略：**原生 C++（NDK）为主，Java/Kotlin 只做薄壳**。

## 与其它平台的关系

| 平台 | 渲染后端 | 输入 | 说明 |
|------|----------|------|------|
| Windows / Linux | `GL33Backend` | 键盘 + 鼠标 | 桌面主力 |
| **Android** | `GLES3Backend` | 触屏 + 虚拟摇杆 | 本目录 |
| Web(WASM) | `GLES3Backend` | DOM 事件 + 摇杆 | `WEB/` |

> Android 与 Web **共用同一套 GLES3 渲染后端**，渲染代码只维护一份。

## 目录

```
Android/
├── README.md            # 本文档
└── apk/                 # APK 打包目录（占位）
    ├── AndroidManifest.xml   # 清单占位（需按实际包名/SDK 修改）
    ├── build.gradle          # Gradle 占位
    ├── jni/
    │   ├── CMakeLists.txt    # NDK 构建入口（占位）
    │   └── native_bridge.cpp # JNI 胶水层（占位）
    └── .gitkeep
```

## 必须实现的胶水层（JNI）

| 方向 | 事件 | 处理 |
|------|------|------|
| Java → C++ | `onTouchEvent` | 转成引擎 `InputState` / 摇杆向量，入主消息队列 |
| Java → C++ | `onPause` / `onResume` | 发布 `AppPause` / `AppResume`，调用 `release_gpu_resources()` |
| Java → C++ | `onSurfaceCreated` | 创建 EGL 上下文（EGL ≈ Android 的 WGL/GLX） |
| Java → C++ | `onSurfaceChanged` | 更新视口尺寸，标记渲染脏区域 |
| Java → C++ | `onDestroy` | 销毁全部 GPU 资源，避免显存泄漏 |

## 关键原则

1. **GLES3 上下文只能在一个线程使用**：所有 OpenGL ES 调用固定在渲染线程，
   其它线程通过消息队列投递渲染指令。
2. **斜向移动**：摇杆输入在进入引擎前做向量钳位（模长 ≤ 1），与桌面 WASD 一致。
3. **生命周期**：Android 随时可能被系统回收，`release_gpu_resources()` 必须幂等。
4. **ABI**：`arm64-v8a`（主）+ `armeabi-v7a`（老设备）。

## 构建（待补）

```bash
# 需要 Android NDK
export ANDROID_NDK=/path/to/ndk
cd Android/apk
./gradlew assembleRelease    # 产物：app/build/outputs/apk/release/app-release.apk
```

> 现阶段 `apk/` 内均为**占位文件**，用于锁定目录结构与接口契约；
> 完整 APK 打包链路在后续版本补齐（不引入 Xcode / 苹果工具链，仅 NDK + Gradle）。
