# Quick Start

5 分钟跑起来第一个 scene。

## 1. 拿到库

- **Windows**：运行 `build.bat`，选择 `1 = DLL` 或 `2 = LIB`
- **Linux**：`./build.sh`
- **Web**：直接 `WEB/single-package/index.html`（需先 `./WEB/build_wasm_single.sh`）

产物：`PyRecT3D.dll` / `libPyRecT3D.so`（或静态库）+ `cpp/include/PyRecT3D/` 头文件目录。

## 2. 隐式 API（推荐先试这个）

```cpp
#include <PyRecT3D/implicit.h>
using namespace pyrect3d;

int main() {
    quick::runSetup(1280, 720, "My First Scene", [] {
        // 建两个方块
        EntityHandle a = quick::cube(0, 0, -5, 1, 1, 1);
        EntityHandle b = quick::cube(2, 0, -6, 1, 1, 1);

        // 克隆 a 的当前状态（位置/旋转/缩放），挪到左边
        EntityHandle c = quick::clone(a);
        quick::move(c, -2, 0, -5);

        // 变换
        quick::rotate(b, 0, 30, 0);
        quick::scale(b, 1.5f, 1.5f, 1.5f);

        // UI（自动绑定唯一窗口）
        Button* btn = quick::button(20, 20, 140, 40, "复位");
        btn->onClick  = [a] { quick::move(a, 0, 0, -5); };
        btn->onEnter  = [] { /* 指针进入 */ };
        btn->onDragOut= [] { /* 按住拖出按钮 */ };
    });
}
```

## 3. 编译

```bash
# Linux
g++ -std=c++17 main.cpp -Icpp/include -Lbuild -lPyRecT3D -o demo
LD_LIBRARY_PATH=build ./demo

# Windows (MSVC)
cl /std:c++17 main.cpp /Icpp/include /link build\PyRecT3D.lib
```

## 4. 显式 API（多窗口 / 详细配置）

见 [[Explicit vs Implicit]]，完整示例在仓库 `examples_cpp/explicit_demo.cpp`。

## 5. 验证环境

```bash
./build/smoke_v2       # 42 项无头测试，全绿即环境正常
```

## 常见问题

| 现象 | 原因 | 解决 |
|------|------|------|
| 只有最后一个方块显示 | v0.1 的 `drawEntity` 每帧清屏 | v0.2 用 `beginFrame()` → 多次 `drawEntity()` → `endFrame()` |
| 斜着走比直着快 | 未做输入向量钳位 | 见 [[Hard Constraints]] §输入钳位 |
| 页面切后台后越来越卡 | 没释放 GPU 资源 | 见 [[Hard Constraints]] §资源释放 |
| 第二个窗口收不到事件 | 子队列没挂载 | `app.mountQueues()` |

---

*PyRecT3D v0.2 · GPL-3.0*
