# Build（编译与分发）

产物二选一：**DLL** 或 **LIB**。构建入口是交互式脚本，不再依赖手工 CMake / NMake 流程。

## Windows：`build.bat`

```bat
build.bat
```

五步交互：

```
[1/5] 产物类型
      1 = DLL  （LoadLibrary 动态加载，可热替换）
      2 = LIB  （静态链接，无外部依赖，体积更大）

[2/5] CL 编译器路径
      回车 = 使用 PATH 中的 cl.exe
      也可填完整路径（如 C:\VC\bin\cl.exe）

[3/5] 构建配置
      1 = Release  (/O2 /GL 全程序优化)
      2 = Debug    (/Od /Zi)

[4/5] 是否裁剪模块（逐项 y/n）
      GUI / 视频(FFmpeg) / 音频 / 物理 / 软光追

[5/5] 是否启用外部 FFmpeg（需要 FFmpeg 开发库）
```

产物：
- DLL 模式 → `PyRecT3D.dll` + `PyRecT3D.lib`（导入库）+ `PyRecT3D.pdb`
- LIB 模式 → `PyRecT3D.lib`（静态库）

> **Win7 兼容**：只用经典 Win32 消息循环与 `wglCreateContext`，不碰 Win10+ 专属 API（如 DWM 合成器高级接口）。

## Linux：`build.sh`

```bash
./build.sh                         # shared + Release + 全模块
./build.sh shared                  # 动态库
./build.sh static                  # 静态库
./build.sh static release trim     # 交互式裁剪模块
./build.sh shared debug            # 调试构建
```

产物：`libPyRecT3D.so` / `libPyRecT3D.a`

## 直接 CMake

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DPYRECT3D_LIB_TYPE=shared \
      -DPYRECT3D_WITH_GUI=ON \
      -DPYRECT3D_WITH_VIDEO=ON \
      -DPYRECT3D_WITH_AUDIO=ON \
      -DPYRECT3D_WITH_PHYSICS=ON \
      -DPYRECT3D_WITH_RAYTRACER=ON \
      -DPYRECT3D_WITH_FFMPEG=OFF \
      -DPYRECT3D_BUILD_PYTHON=OFF \
      -DPYRECT3D_BUILD_EXAMPLES=ON \
      -DPYRECT3D_BUILD_TESTS=ON
cmake --build build -j
```

## 模块裁剪宏

| 宏 | 对应模块 | 关闭后 |
|----|----------|--------|
| `PYRECT3D_WITH_GUI` | GUI | `Gui` 相关 API 返回空/假 |
| `PYRECT3D_WITH_VIDEO` | 视频 | `MediaManager::createPlayer()` 返回 `nullptr` |
| `PYRECT3D_WITH_AUDIO` | 音频 | `Audio::playMusic()` 返回 `false` |
| `PYRECT3D_WITH_PHYSICS` | 碰撞 | `CollisionWorld::raycast()` 恒 `hit=false` |
| `PYRECT3D_WITH_RAYTRACER` | 软光追 | `SoftRayTracer::render()` 返回 `false` |
| `PYRECT3D_WITH_FFMPEG` | 外部 FFmpeg | 内置解码回退 |

> 裁剪是**源码级**的：关闭模块后其 `.cpp` 根本不参与编译 —— 这就是瘦身的主要手段。

## 体积优化策略

1. **编译期锁定单一渲染后端**：发布版不带 `GL33` + `GLES3` 两套实现，未选的成为死代码。
2. **合并头文件 + 内联转换层**：把 OpenGL 转换层内联进调用点，减少调用开销与符号。
3. `-O3 -ffunction-sections -fdata-sections` + `-Wl,--gc-sections`（Linux）；`/O2 /GL` + `LTCG`（MSVC）。
4. `strip`（Linux）/ `/DEBUG:NONE`（MSVC release 可选）。
5. 按需裁剪模块。

> v0.1 的 DX11 构建是 376 KB；改用 GL 后端 + 头文件合并 + O3 后可压到 **百 KB 量级**。

## ABI 与分发边界

- 调用方只需 `cpp/include/PyRecT3D/*.h`（极轻量，纯声明）。
- DLL 模式：`LoadLibrary` + `GetProcAddress`（或直接链接导入库）。
- **已发布版本不得随意改函数签名与结构体布局** —— `MessagePayload` 用定长字段而非 `std::any`，就是为了跨编译器 ABI 稳定。

## 运行测试

```bash
./build/smoke_v2        # 42 项断言
```

覆盖：数学、场景克隆/召唤、消息队列优先级与路由、双碰撞箱射线、视频帧上传、合成层脏区域、GUI 事件、模块裁剪。

---

返回 [[Home]]
