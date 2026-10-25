// ============================================================
//  PyRecT3D — examples_cpp/basic_cube.cpp
//  原生 C++ 示例：不依赖 Python，直接用 C++ 调用引擎
// ============================================================
#include <PyRecT3D/core.h>
#include <PyRecT3D/renderer.h>
#include <PyRecT3D/gui.h>
#include <cstdio>

int main() {
    using namespace pyrect3d;

    Engine engine;
    if (!engine.init(800, 600, "PyRecT3D Basic Cube")) {
        std::fprintf(stderr, "Failed to init engine\n");
        return 1;
    }

    // 场景 + 立方体
    Scene scene;
    Cube cube(Vec3(0, 0, -5), Vec3(1, 1, 1));
    scene.addEntity(&cube);

    // GUI
    Gui gui;
    Button btn(Rect(10, 10, 120, 40), "Quit");
    btn.onClick = [&]() { engine.requestClose(); };
    gui.addWidget(&btn);

    Renderer renderer;
    renderer.init(800, 600);
    renderer.setClearColor(0.15f, 0.15f, 0.2f);

    std::printf("Running PyRecT3D C++ demo. Close window or click Quit to exit.\n");

    // 外部接管模式示例：每帧渲染
    engine.setFrameCallback([&](InputState& in, double) {
        renderer.drawEntity(cube);
        if (in.isDown(Key::Escape)) engine.requestClose();
    });

    int rc = engine.run();
    engine.shutdown();
    return rc;
}
