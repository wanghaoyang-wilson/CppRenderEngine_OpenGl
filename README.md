# PyRecT3D — C++ OpenGL 2D/3D 渲染引擎 + Python 绑定

一个 **C++ 编写的轻量 3D 渲染引擎**，底层使用 OpenGL 3.3 Core。既是可独立链接的 C++ 动态库，也通过 **pybind11** 绑定为 Python 扩展，**C++ 端与 Python 端都可以完整使用全部功能**。API 设计对标 [Ursina](https://www.ursinaengine.org/)，简单友好。

> 开发代号：PyRecT3D（Python + ReCursion + 3D）

---

## 功能总览

| 模块 | 能力 |
|------|------|
| **渲染** | 2D + 3D 统一渲染、基础图元 `Cube` / `ScreenCube`、抗锯齿、帧导出、**软件光追（非 RTX 硬件光追）**、坐标系系统 |
| **模型 IO** | OBJ 模型加载、材质加载 |
| **数学 / 碰撞** | 矩阵 / 向量、**射线检测**（命中返回实体实例）、AABB 加速 |
| **消息循环 / 输入** | 独立消息循环，支持**外部接管**（如 QT 启动器）与外部关闭；接管 WASD/空格/ESC/Alt+F4/三键鼠标；**鼠标锁定** |
| **自研极简 GUI** | 按钮、文本、输入框、**复杂容器、滚动容器**；支持贴图美化；`OnHover` / `OnClick` 事件 |
| **音频** | 音乐播放子系统 |
| **绑定** | pybind11 导出，Python 端同等调用 |

### 明确边界
- 不内置任何游戏逻辑（如 MC），只提供底层渲染/输入/射线/图元能力
- 不自带硬件光追，仅软件光追
- GUI 保持极简，不含表格、树、标签页、Dock、动画、主题

---

## 平台要求

| 平台 | 要求 |
|------|------|
| **Windows** | 最低兼容 **Windows 7**（经典 Win32 消息机制，不用 Win10+ 专属 API） |
| **Linux** | 基于 **X11**（不默认 Wayland） |
| 图形 API | OpenGL **3.3 Core**（兼顾 Win7 老显卡） |

> 不支持 macOS。

---

## 目录结构

```
PyRecT3D/
├── CMakeLists.txt          # 主构建（跨平台，区分导出/导入宏）
├── Makefile.nmake          # Windows NMake 备选方案
├── build.bat               # Windows 一键构建
├── build.sh                # Linux 一键构建
├── docs/
│   └── api.md              # 接口文档 + 调用示例
├── cpp/
│   ├── include/PyRecT3D/   # 对外暴露头文件（仅声明，契约层）
│   └── src/                # 内部实现源码
│       ├── core/           # 引擎上下文、消息循环、平台粘合(Win7/X11)
│       ├── renderer/       # OpenGL 渲染、图元、软光追、帧导出
│       ├── gui/            # 极简GUI + 容器
│       ├── math/           # 矩阵/向量/射线/AABB
│       ├── io/             # OBJ/材质加载
│       ├── audio/          # 音频
│       └── binding/        # pybind11 绑定
├── py/                     # Python 侧
│   ├── build.py            # 一键编译脚本
│   ├── examples/           # Python 示例
│   └── package/            # 编译产物 (.pyd/.so)
└── examples_cpp/           # 原生 C++ 示例
```

---

## 快速开始

### 依赖
- CMake ≥ 3.10
- C++17 编译器（MSVC / GCC / Clang）
- Python3 + pybind11（`pip install pybind11`，可选，构建 Python 绑定用）
- Windows：无需额外图形库，使用系统 OpenGL；**不用 GLFW/GLEW**，全部自研封装
- Linux：`libx11-dev`（输入/窗口）、Mesa GL

### 构建

**Linux**
```bash
chmod +x build.sh
./build.sh
```

**Windows (Win7+)**
```bat
build.bat
```

**Python 绑定一键构建**
```bash
python py/build.py
```

产物：
- C++ 动态库：`build/PyRecT3D.dll` / `build/libPyRecT3D.so`
- Python 扩展：`py/package/pyrect3d_binding*.pyd|.so`

---

## C++ 示例

```cpp
#include <PyRecT3D/core.h>
#include <PyRecT3D/renderer.h>
#include <PyRecT3D/gui.h>

int main() {
    using namespace pyrect3d;
    Engine engine;
    engine.init(800, 600, "PyRecT3D");

    auto cube = Scene::create<Cube>(Vec3(0, 0, -5));
    engine.run();   // 自带消息循环

    return 0;
}
```

## Python 示例

```python
import pyrect3d

app = pyrect3d.Engine(title="PyRecT3D")
cube = pyrect3d.Cube(position=(0, 0, -5))
app.run()
```

---

## 构建说明

- **源码优先编译**：不加载任何预编译 DLL / 预编译头，全部 `.h` + `.cpp` 参与正常编译，可自由修改源码后重新构建。
- **头文件即契约**：`cpp/include/PyRecT3D/` 下对外头文件**只含声明**，不含实现体，仅供编译器做类型/签名校验与 IDE 提示。
- **导出宏**：`PYRECT3D_API` 依据 `PYRECT3D_BUILD_DLL` 自动展开为 `__declspec(dllexport|dllimport)`（Windows）或空（Linux，靠 soname 导出）。
- Python 扩展是依赖核心动态库的**薄绑定层**，不把内核静态打包。

---

## License

MIT
