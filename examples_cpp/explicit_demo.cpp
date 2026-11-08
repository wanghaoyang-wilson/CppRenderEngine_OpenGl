// ============================================================
//  PyRecT3D 示例 —— 显式 API（Explicit API）
//  适用：多窗口 / 需要详细配置（后端、模块、队列、UI 绑定）的工程
//
//  编译：cmake -DPYRECT3D_LIB_TYPE=shared .. && cmake --build .
//  运行：./explicit_demo
// ============================================================
#include <PyRecT3D/explicit.h>
#include <PyRecT3D/collision.h>
#include <cstdio>

using namespace pyrect3d;

int main() {
    ExplicitApp app;

    // ---- 详细配置：后端、模块、局部更新、多线程渲染指令 ----
    AppConfig cfg;
    cfg.width = 1280;
    cfg.height = 720;
    cfg.title = "PyRecT3D — Explicit";
    cfg.backend = Backend::Auto;        // 桌面→GL33，Android/Web→GLES3
    cfg.enableGUILocalUpdate = true;    // 2D/3D 分离局部更新
    cfg.multithreadRender = false;      // 渲染指令队列（需要时应开）
    app.configure(cfg);

    // ---- 窗口 A + 窗口 B（每个窗口自动带一个子消息队列）----
    WindowDesc da; da.title = "窗口 A"; da.width = 800; da.height = 600;
    WindowDesc db; db.title = "窗口 B"; db.width = 640; db.height = 480;
    WindowInstance* wa = app.createWindow(da);
    WindowInstance* wb = app.createWindow(db);

    // 多窗口必须显式挂载子队列
    app.mountQueues();

    // ---- UI 手动创建 + 显式绑定（多窗口强制）----
    UIInstance* uiA = wa->createUI();
    uiA->bindWindow(wa);
    Button* btn = uiA->addButton(Rect{ 20, 20, 160, 44 }, "关闭程序");
    btn->onClick = [] { App::quit(); };
    btn->onEnter = [] { std::printf("指针进入按钮\n"); };
    btn->onExit  = [] { std::printf("指针离开按钮\n"); };

    // ---- 场景对象：以主消息队列身份发布操作 ----
    Entity* cube = new Cube(Vec3(0, 0, -5), Vec3(1, 1, 1));
    std::int64_t id = wa->scene().addEntity(cube);

    Message mv;
    mv.type = EventType::TransformRotate;
    mv.sourceQueue = 0;                 // 主消息队列身份
    mv.payload.entityId = static_cast<std::uint64_t>(id);
    mv.payload.v[0] = 0; mv.payload.v[1] = 45; mv.payload.v[2] = 0;
    app.publishGlobal(mv);

    // ---- 克隆（复制场上状态）而非召唤（按模板新建）----
    std::int64_t cloned = wa->scene().cloneEntity(id);
    std::int64_t summoned = wa->scene().summonEntity(id, Vec3(2, 0, -5));
    std::printf("克隆 id=%lld 召唤 id=%lld\n",
                (long long)cloned, (long long)summoned);

    // ---- 双碰撞箱 + 碰撞区事件 ----
    CollisionWorld world;
    PhysicsCollider pc;
    pc.box = AABB(Vec3(-0.5f, -0.5f, -5.5f), Vec3(0.5f, 0.5f, -4.5f));
    world.addPhysicsBox(pc);

    ModelCollider mc;
    mc.root = AABB(Vec3(-0.6f, -0.6f, -5.6f), Vec3(0.6f, 0.6f, -4.4f));
    Limb head; head.name = "head";
    head.localBox = AABB(Vec3(-0.2f, 0.35f, -0.2f), Vec3(0.2f, 0.6f, 0.2f));
    mc.limbs.push_back(head);
    world.addModelBox(mc);

    auto zone = world.addTriggerZone(AABB(Vec3(-1, -1, 0), Vec3(1, 1, 0)), id);
    (void)zone;
    world.setEventHandler(CollisionEventKind::Enter, [](const Message&) {
        std::printf("进入碰撞区\n");
    });

    // ---- 视频：多路实例，多窗口下显式绑定子队列 ----
    VideoDesc vd; vd.path = "assets/demo.mp4"; vd.loop = true; vd.autoPlay = true;
    VideoPlayer* p1 = app.createVideoPlayer(vd, wa);
    VideoPlayer* p2 = app.createVideoPlayer(vd, wb);
    if (p1) std::printf("播放器1 绑定队列 %lld\n", (long long)p1->boundQueueId());
    if (p2) std::printf("播放器2 绑定队列 %lld\n", (long long)p2->boundQueueId());

    // ---- 启动（统一由 App::run 拉起消息队列 + 3D 场景）----
    return app.run();
}
