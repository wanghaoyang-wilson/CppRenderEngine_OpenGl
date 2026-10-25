# PyRecT3D API 参考文档

> C++ 与 Python 两端共享同一套底层引擎逻辑。本文档给出公共接口与两端调用示例。
> 对外头文件：`cpp/include/PyRecT3D/*.h`（仅声明，契约层）。

## 命名空间 / 模块
- C++：`namespace pyrect3d`
- Python：`import pyrect3d`

---

## 1. 数学类型

### `Vec3`
C++:
```cpp
pyrect3d::Vec3 v(1.f, 2.f, 3.f);
float l = v.length();
pyrect3d::Vec3 n = v.normalized();
```
Python:
```python
v = pyrect3d.Vec3(1, 2, 3)
l = v.length()
n = v.normalized()
```

### `Ray` / `RaycastHit`
```python
ray = pyrect3d.Ray(pyrect3d.Vec3(0,0,0), pyrect3d.Vec3(0,0,-1))
hit = scene.raycast(ray)   # hit.hit / hit.distance / hit.entity_id
```

---

## 2. 引擎 & 消息循环

### `Engine`
两种运行模式：
- **模式1：自带消息循环** —— `engine.run()` 阻塞运行
- **模式2：外部接管** —— `engine.pump()` 单步处理，注入 `set_frame_callback`

```cpp
pyrect3d::Engine engine;
engine.init(800, 600, "PyRecT3D");
engine.setFrameCallback([](pyrect3d::InputState& in, double dt){
    // 每帧渲染 / 输入处理
});
engine.run();
```

**输入接管**（Windows Win7 / Linux X11 均支持）：
`Key::W/A/S/D`、`Space`、`Escape`、`AltF4`、`MouseLeft/Right/Middle`；
`engine.lockMouse(true)` 锁定鼠标。

### 外部接管（如 QT 启动器）
```cpp
// 由外部窗口句柄初始化
engine.init(800, 600, "embedded", qtWinId);
while (running) {
    if (!engine.pump()) break;   // 单步处理
    engine.input();              // 读取输入状态
}
```

---

## 3. 渲染 & 图元

```cpp
pyrect3d::Renderer renderer;
renderer.init(800, 600);
renderer.setClearColor(0.1f, 0.1f, 0.2f);
renderer.setAntiAliasing(true);
renderer.drawEntity(cube);
renderer.saveFrame("frame.ppm");   // 帧导出
```

图元：`Cube`（3D）、`ScreenCube`（2D 屏幕方块）。

---

## 4. 场景 / 实体 / 射线

```cpp
pyrect3d::Scene scene;
auto cube = new pyrect3d::Cube({0,0,-5}, {1,1,1});
scene.addEntity(cube);                       // 返回 id
pyrect3d::RaycastHit hit = scene.raycast({ {0,0,0}, {0,0,-1} });
// hit.entityId → 命中的实体，scene.getEntity(id) 取实例，上层处理业务
```

---

## 5. 自研 GUI

组件：`Button` / `Text` / `InputBox` / `Container`（复杂容器）/ `ScrollContainer`（滚动容器）
- 事件：`OnClick` / `OnHover`（挂在消息循环，由 `Gui::dispatch` 派发）
- 支持贴图：`widget.setTexture("tex.png")`
- 不含：表格 / 树 / 标签页 / Dock / 动画 / 主题

```cpp
pyrect3d::Gui gui;
auto btn = new pyrect3d::Button({10,10,120,40}, "Quit");
btn->onClick = []{ /* ... */ };
gui.addWidget(btn);
```

---

## 6. 模型 & 音频

```cpp
pyrect3d::ObjModel model;
pyrect3d::ObjLoader::load("assets/model.obj", model);

pyrect3d::Audio audio;
audio.playMusic("assets/music.wav", true);
audio.setVolume(0.8f);
```

---

## 7. 软光追

```cpp
pyrect3d::SoftRayTracer tracer;
std::vector<unsigned char> rgb;
tracer.render(scene, 800, 600, rgb);   // 输出 RGB 帧
```

---

## 边界说明
- 不内置游戏逻辑，只提供底层渲染/输入/射线/图元能力
- 仅软件光追，非 RTX 硬件光追
- GUI 保持极简
