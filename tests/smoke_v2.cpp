// ============================================================
//  PyRecT3D — tests/smoke_v2.cpp
//  v0.2 无头冒烟测试：消息队列优先级 / 子队列路由 / 碰撞事件 /
//  双碰撞箱射线 / 显式多窗口 UI 绑定 / 视频帧上传 / 合成层脏区域
// ============================================================
#include <PyRecT3D/message.h>
#include <PyRecT3D/explicit.h>
#include <PyRecT3D/implicit.h>
#include <PyRecT3D/collision.h>
#include <PyRecT3D/video.h>
#include <PyRecT3D/renderer.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>

using namespace pyrect3d;

static int g_pass = 0, g_fail = 0;
static void check(bool cond, const char* what) {
    if (cond) { ++g_pass; std::printf("  [PASS] %s\n", what); }
    else      { ++g_fail; std::printf("  [FAIL] %s\n", what); }
}

// ---------- 1. 事件优先级调度 ----------
static void testPriority() {
    std::printf("[1] 事件优先级调度\n");
    MainMessageQueue& main = MainMessageQueue::instance();

    Message low; low.type = EventType::VideoFrameReady; low.priority = EventPriority::Media;
    Message mid; mid.type = EventType::CollisionEnter;   mid.priority = EventPriority::Collision;
    Message high; high.type = EventType::WindowClose;    high.priority = EventPriority::WindowInput;
    Message phys; phys.type = EventType::PhysicsTick;    phys.priority = EventPriority::Physics;

    // 故意乱序发布
    main.publish(low);
    main.publish(high);
    main.publish(mid);
    main.publish(phys);

    Message out;
    EventType order[4];
    int n = 0;
    while (main.poll(out) && n < 4) order[n++] = out.type;

    check(n == 4, "四条消息全部入队");
    check(order[0] == EventType::WindowClose, "窗口输入最先派发");
    check(order[1] == EventType::PhysicsTick, "物理第二");
    check(order[2] == EventType::CollisionEnter, "碰撞第三");
    check(order[3] == EventType::VideoFrameReady, "媒体最后");
}

// ---------- 2. 子队列路由 ----------
static void testRouting() {
    std::printf("[2] 子消息队列路由\n");
    MessageQueue qa(1001), qb(1002);
    check(qa.attachToMain(), "队列 A 挂载主队列");
    check(qb.attachToMain(), "队列 B 挂载主队列");
    check(MainMessageQueue::instance().childCount() == 2, "主队列记录 2 个子队列");

    Message m;
    m.type = EventType::VideoFinished;
    m.priority = EventPriority::Media;
    m.targetQueue = 1001;             // 只发给 A
    m.payload.playerId = 7;
    MainMessageQueue::instance().publish(m);
    MainMessageQueue::instance().pump();

    Message got;
    check(qa.poll(got) && got.payload.playerId == 7, "A 队列收到自己的媒体事件");
    Message none;
    check(!qb.poll(none), "B 队列不会收到 A 的事件");

    qa.detachFromMain();
    qb.detachFromMain();
}

// ---------- 3. 碰撞区事件状态机 ----------
static void testCollisionZone() {
    std::printf("[3] 碰撞区事件（进入/退出/按下/拖出/区域外松开）\n");
    CollisionWorld world;
    AABB zone(Vec3(0, 0, 0), Vec3(100, 100, 1));
    world.addTriggerZone(zone, 42);

    int enter = 0, exit = 0, press = 0, dragOut = 0, outside = 0, release = 0;
    world.setEventHandler(CollisionEventKind::Enter,        [&](const Message&) { ++enter; });
    world.setEventHandler(CollisionEventKind::Exit,         [&](const Message&) { ++exit; });
    world.setEventHandler(CollisionEventKind::Press,        [&](const Message&) { ++press; });
    world.setEventHandler(CollisionEventKind::DragOut,      [&](const Message&) { ++dragOut; });
    world.setEventHandler(CollisionEventKind::ClickOutside, [&](const Message&) { ++outside; });
    world.setEventHandler(CollisionEventKind::Release,      [&](const Message&) { ++release; });

    world.processPointer(50, 50, false, false, 0.016f);   // 进入
    world.processPointer(50, 50, true,  false, 0.016f);   // 按下
    world.processPointer(150, 50, true, false, 0.016f);   // 按住拖出
    world.processPointer(150, 50, false, true,  0.016f);  // 区域外松开
    world.processPointer(150, 150, false, false, 0.016f); // 后续

    check(enter >= 1, "触发 Enter");
    check(press >= 1, "触发 Press");
    check(dragOut >= 1, "触发 DragOut");
    check(outside >= 1, "触发 ClickOutside（区域外松开）");
    check(release == 0, "区域外松开不算 Release");
}

// ---------- 4. 双碰撞箱射线 ----------
static void testDualCollider() {
    std::printf("[4] 双碰撞箱 + 射线（物理箱 / 模型肢体）\n");
    CollisionWorld world;

    PhysicsCollider pc;
    pc.box = AABB(Vec3(-1, -1, -10), Vec3(1, 1, -6));
    world.addPhysicsBox(pc);

    ModelCollider mc;
    mc.root = AABB(Vec3(-1, -1, -12), Vec3(1, 1, -8));
    Limb head; head.name = "head"; head.localBox = AABB(Vec3(-0.3f, 0.5f, -0.3f), Vec3(0.3f, 1.0f, 0.3f));
    mc.limbs.push_back(head);
    world.addModelBox(mc);

    Ray r(Vec3(0, 0.8f, 0), Vec3(0, 0, -1));   // 对准头部肢体

    // 只查物理箱
    HitResult hp = world.raycast(r, 100.f, /*includeModel=*/false, /*includePhysics=*/true);
    check(hp.hit, "射线命中物理碰撞箱");

    // 两套都查：精细盒优先，返回肢体名
    HitResult hm = world.raycast(r, 100.f, true, true);
    check(hm.hit, "双碰撞箱射线命中");
    check(!hm.limbName.empty(), "命中模型肢体（精细盒优先）");
}

// ---------- 5. 显式 API 多窗口 UI 绑定 ----------
static void testExplicit() {
    std::printf("[5] 显式 API：多窗口 + UI / 播放器显式绑定\n");
    ExplicitApp app;
    AppConfig cfg;
    cfg.width = 320; cfg.height = 240; cfg.title = "Explicit";
    cfg.enableGUILocalUpdate = true;
    check(app.configure(cfg), "configure 成功");

    WindowDesc d1; d1.title = "A"; d1.width = 320; d1.height = 240;
    WindowDesc d2; d2.title = "B"; d2.width = 320; d2.height = 240;
    WindowInstance* wa = app.createWindow(d1);
    WindowInstance* wb = app.createWindow(d2);
    check(wa && wb, "创建两个窗口实例");
    check(wa->queue()->attached(), "单窗口阶段 A 自动挂载");
    check(!wb->queue()->attached(), "第二个窗口不自动挂载（需显式）");
    check(app.mountQueues(), "mountQueues 显式挂载全部子队列");

    UIInstance* uiA = wa->createUI();
    UIInstance* uiB = wb->createUI();
    check(uiA->bindWindow(wa) && uiB->bindWindow(wb), "UI 显式绑定各自窗口");

    Button* b = uiA->addButton(Rect{ 10, 10, 100, 30 }, "Quit");
    int clicked = 0;
    b->onClick = [&] { ++clicked; };
    int entered = 0;
    b->onEnter = [&] { ++entered; };
    check(b != nullptr, "UI 组件可创建");

    VideoDesc vd; vd.path = "demo.mp4"; vd.autoPlay = false;
    VideoPlayer* p = app.createVideoPlayer(vd, wb);
    check(p != nullptr, "创建播放器实例");
    check(p->boundQueueId() == wb->queue()->id(), "播放器显式绑定到 B 窗口子队列");

    check(app.hasModule(Module::Renderer), "Renderer 模块存在");
}

// ---------- 6. 隐式 API ----------
static void testImplicit() {
    std::printf("[6] 隐式 API：单窗口弱绑定\n");
    EntityHandle c = quick::cube(0, 0, -5, 1, 1, 1);
    check(c.valid(), "隐式创建 Cube");
    check(quick::scene().entityCount() >= 1, "实体已加入唯一场景");

    EntityHandle c2 = quick::clone(c);
    check(c2.valid(), "克隆现有实体（复制场上状态）");

    Entity* e = quick::scene().getEntity(c.id);
    Entity* e2 = quick::scene().getEntity(c2.id);
    check(e && e2 && (e->position().z == e2->position().z), "克隆体继承原位置");

    quick::rotate(c, 0, 45, 0);
    check(quick::scene().getEntity(c.id)->rotation().y == 45, "旋转 API 生效");

    HitResult h = quick::raycast(0, 0, 0, 0, 0, -1, 100);
    check(h.hit, "射线检测命中");

    Button* btn = quick::button(10, 10, 80, 30, "OK");
    check(btn != nullptr, "隐式 UI 自动绑定唯一窗口");
}

// ---------- 7. 视频解码 / 主线程上传 ----------
static void testVideo() {
    std::printf("[7] 视频：子线程解码 + 主线程上传\n");
    VideoPlayer* p = MediaManager::instance().createPlayer();
    VideoDesc d; d.path = "demo.mp4"; d.autoPlay = true;
    check(p->open(d), "打开视频");
    std::this_thread::sleep_for(std::chrono::milliseconds(120));

    bool uploaded = false;
    for (int i = 0; i < 50; ++i) {
        if (p->uploadPendingFrame()) { uploaded = true; break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    check(uploaded, "检测帧就绪并在主线程完成上传");
    check(p->texture().valid, "获得动态视频材质");
    check(MediaManager::instance().playerCount() >= 1, "全局管理器持有实例");
}

// ---------- 8. 渲染指令合并 / 脏区域 ----------
static void testCompositor() {
    std::printf("[8] 渲染指令合并 + 局部更新\n");
    Renderer r; r.init(320, 240);
    Compositor comp; comp.attach(&r);

    RenderCommand cmds[4];
    for (int i = 0; i < 4; ++i) {
        cmds[i].type = CommandType::DrawRect2D;
        cmds[i].x = (float)(10 + i * 12); cmds[i].y = 10;
        cmds[i].w = 10; cmds[i].h = 10;
        cmds[i].r = 1; cmds[i].g = 1; cmds[i].b = 1;
    }
    comp.setSceneDirty(false);          // 模拟：3D 画面本帧已渲染完毕
    comp.submit(cmds, 4);
    DirtyRegion d = comp.dirtyRegion();
    check(d.valid, "2D 指令标记脏区域");
    check(comp.sceneDirty() == false, "只更新 UI 时 3D 视口不脏");

    comp.setSceneDirty(false);
    comp.present();
    check(comp.mergedCount() >= 1, "指令被合并提交");
}

int main() {
    std::printf("==== PyRecT3D v0.2 冒烟测试 ====\n");
    testPriority();
    testRouting();
    testCollisionZone();
    testDualCollider();
    testExplicit();
    testImplicit();
    testVideo();
    testCompositor();
    std::printf("\n结果：PASS=%d FAIL=%d\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
