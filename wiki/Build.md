# Build（编译与分发）

产物二选一：**DLL** 或 **LIB**。从 v0.2 起构建入口是交互式脚本，**只剩 cl.exe / link.exe / lib.exe**，
CMake 与 NMake 都不再是必经路径。

## Windows：`build.bat`

```bat
build.bat
```

六步交互（回车即取默认值）：

```
[1/6] 产物类型
      1 = DLL  （调用侧只用 cpp\include + LoadLibraryA，可热替换）
      2 = LIB  （静态链接；调用侧必须加 /DPYRECT3D_STATIC=1）

[2/6] CL 编译路径
      回车 = 用 vswhere 自动找 Visual Studio 并 call vcvarsall.bat x64
      找不到再退到 PATH 里的 cl.exe；也可手填
      C:\VS2022\VC\Tools\MSVC\14.29.30133\bin\Hostx64\x64

[3/6] 配置
      1 = Release  (/O2 /GL，链接加 /LTCG)
      2 = Debug    (/Od /Zi /MDd，链接加 /DEBUG)

[4/6] GL 后端
      0 = 只有软件光栅化（一个 GL 符号都不碰，体积最小）
      1 = glad + OpenGL 3.3 Core / GLES 3.0（会额外编译 glad.c）

[5/6] 模块裁剪（逐项 y/n）
      GUI / 视频(FFmpeg) / 音频 / 物理，外加「是否额外编 examples」

[6/6] 确认清单
      单独一问：是否把被裁掉的源码从磁盘删掉（默认 n，删了要 git restore 才能回来）
```

产物都落在 `build\`：

| 模式 | 产物 |
|------|------|
| DLL | `PyRecT3D.dll` + `PyRecT3D.lib`（导入库） |
| LIB | `PyRecT3D.lib`（静态库） |

> **Win7 兼容**：只用经典 Win32 消息循环与 `wglCreateContext`，`_WIN32_WINNT=0x0601` 写死在编译宏里。

## Linux：`build.sh`

```bash
./build.sh                      # 共享库 + Release + 全模块 + GL 关
./build.sh --static             # 静态库 libPyRecT3D.a
./build.sh --gl=on              # 开 glad + GL33
./build.sh --gl=on --tests      # 编完顺手跑冒烟测试
./build.sh --trim=video,audio   # 裁剪视频与音频
./build.sh --examples           # 额外编三个示例
./build.sh --debug              # -O0 -g
```

`build.sh` 内部同样是直接调 `g++`，不经过 CMake。

## CMake（可选，不再是必经路径）

需要 IDE 工程或 `find_package` 集成时才用它：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DPYRECT3D_LIB_TYPE=shared \
      -DPYRECT3D_WITH_GL=ON \
      -DPYRECT3D_BUILD_PYTHON=OFF
cmake --build build -j
```

- `PYRECT3D_LIB_TYPE` = `shared` / `static`
- `PYRECT3D_WITH_GL` = `ON` / `OFF`
- 模块开关：`PYRECT3D_WITH_GUI` / `_WITH_VIDEO` / `_WITH_AUDIO` / `_WITH_PHYSICS`

## 模块裁剪宏

| 宏 | 对应模块 | 关闭后 |
|----|----------|--------|
| `PYRECT3D_WITH_GUI` | GUI | `Gui` 相关 API 返回空/假 |
| `PYRECT3D_WITH_VIDEO` | 视频 | `MediaManager::createPlayer()` 返回 `nullptr` |
| `PYRECT3D_WITH_AUDIO` | 音频 | `Audio::playMusic()` 返回 `false` |
| `PYRECT3D_WITH_PHYSICS` | 碰撞 | `CollisionWorld::raycast()` 恒 `hit=false` |
| `PYRECT3D_WITH_RAYTRACER` | 软光追 | `SoftRayTracer::render()` 返回 `false` |
| `PYRECT3D_WITH_GL` | GL 后端 | `gl_backend` 变空实现，全走软件光栅化 |

> 裁剪是**源码级**的：关闭模块后其 `.cpp` 根本不参与编译 —— 这是瘦身的主要手段，
> 不是靠 `return` 硬截断底层（那样会连带影响没被裁掉的部分）。

## 体积优化策略

1. **编译期锁定单一渲染后端**：发布版不带两套实现，未选的成为死代码。
2. **合并头文件 + 内联转换层**：GL 转换层内联进调用点，减少符号与调用开销。
3. `-O3 -ffunction-sections -fdata-sections` + `-Wl,--gc-sections`（Linux）；`/O2 /GL` + `/LTCG`（MSVC）。
4. `strip`（Linux）/ `/DEBUG:NONE`（MSVC release 可选）。
5. 按需裁剪模块 + 不编 glad（`GL=0` 时 glad.c 不入编译单元）。

> v0.1 的 DX11 构建是 376 KB；改用 GL 后端 + 头文件合并 + O3 后可压到 **百 KB 量级**。

## ABI 与分发边界

- 调用方只需 `cpp/include/PyRecT3D/*.h`（极轻量，纯声明）。
- DLL 模式：`LoadLibrary` + `GetProcAddress`（或直接链接导入库）。
- LIB 模式：调用侧定义 `PYRECT3D_STATIC`，`export.h` 才会把 `PYRECT3D_API` 展开成空宏。
  漏了这一步会变成 `dllimport`，链接期报「找不到 __imp_ 符号」。
- **已发布版本不得随意改函数签名与结构体布局** —— `MessagePayload` 用定长字段而非 `std::any`，就是为了跨编译器 ABI 稳定。

## 运行测试

```bash
./build.sh --tests            # Linux：编完直接跑
./build/smoke_v2              # 或单独跑，42 项断言
```

覆盖：数学、场景克隆/召唤、消息队列优先级与路由、双碰撞箱射线、视频帧上传、合成层脏区域、GUI 事件、模块裁剪。

---

返回 [[Home]]
