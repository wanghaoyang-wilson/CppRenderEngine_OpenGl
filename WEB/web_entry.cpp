// ============================================================
//  PyRecT3D — WASM Web 入口（extern "C"）
//
//  浏览器 / Node 侧通过 Emscripten MODULARIZE 加载本产物：
//      const m = await createPyRecT3D();
//      const v = m._pyr3d_version();      // C 字符串
//      const n = m._pyr3d_smoke_run();    // 返回 PASS-FAIL，0 表示全过
//
//  当前阶段只暴露不依赖真实窗口的核心逻辑（消息队列、碰撞、数学）。
//  canvas / WebGL 渲染接入由后续版本在 JS 桥里 attach。
// ============================================================
#include <PyRecT3D/message.h>
#include <PyRecT3D/collision.h>
#include <PyRecT3D/math.h>
#include <cstdio>
#include <cstring>

using namespace pyrect3d;

static int g_pass = 0, g_fail = 0;

static void check(bool cond, const char* what) {
    if (cond) { ++g_pass; std::printf("  [PASS] %s\n", what); }
    else      { ++g_fail; std::printf("  [FAIL] %s\n", what); }
}

extern "C" const char* pyr3d_version() {
    return PYRECT3D_VERSION_STRING;
}

extern "C" int pyr3d_smoke_run() {
    g_pass = 0; g_fail = 0;
    std::printf("[wasm-smoke] PyRecT3D %s\n", PYRECT3D_VERSION_STRING);

    // 1. 六级优先级：窗口输入 > 物理 > 碰撞 > 音效 > 媒体 > 普通
    {
        auto& q = MainMessageQueue::instance();
        Message a; a.type = EventType::VideoFrameReady; a.priority = EventPriority::Media;
        Message b; b.type = EventType::CollisionEnter;  b.priority = EventPriority::Collision;
        Message c; c.type = EventType::WindowClose;     c.priority = EventPriority::WindowInput;
        Message d; d.type = EventType::PhysicsTick;     d.priority = EventPriority::Physics;
        q.publish(a); q.publish(c); q.publish(b); q.publish(d);

        Message out[4]; int n = 0;
        while (q.poll(out[n]) && n < 4) ++n;
        check(n == 4 &&
              out[0].priority == EventPriority::WindowInput &&
              out[1].priority == EventPriority::Physics &&
              out[2].priority == EventPriority::Collision &&
              out[3].priority == EventPriority::Media,
              "[1] 六级优先级乱序派发正确");
    }

    // 2. 子队列挂载
    {
        MessageQueue child(2001);
        check(child.attachToMain(), "[2] 子队列 attachToMain");
        Message m; m.type = EventType::WindowClose; m.priority = EventPriority::WindowInput;
        child.publishLocal(m);
        Message out;
        check(child.poll(out) && out.type == EventType::WindowClose, "[2] 子队列 publishLocal 可取");
        child.detachFromMain();
    }

    // 3. 双碰撞箱：物理箱射线（与桌面 smoke 同构，已知能命中）
    {
        CollisionWorld w;
        PhysicsCollider c;
        c.box = AABB(Vec3(-1,-1,-10), Vec3(1,1,-6));
        int64_t id = w.addPhysicsBox(c);
        Ray ray{Vec3{0, 0.8f, 0}, Vec3{0, 0, -1}.normalized()};
        HitResult hit = w.raycast(ray, 100.0f);
        check(hit.hit, "[3] 物理碰撞箱被射线命中");
    }

    // 4. 数学：向量归一化（输入向量钳位的底层保障）
    {
        Vec3 v{1.0f, 1.0f, 0.0f};
        float len = v.length();
        Vec3 n = v.normalized();
        float nlen = n.length();
        check(len > 1.4140f && len < 1.4143f, "[4] (1,1,0) 长度 ≈ √2≈1.414");
        check(nlen > 0.999f && nlen < 1.001f, "[4] 归一化后模长 = 1（斜推不超速）");
    }

    std::printf("[wasm-smoke] result: PASS=%d FAIL=%d\n", g_pass, g_fail);
    return g_pass - g_fail;   // 全过则返回 0
}

extern "C" int pyr3d_pass_count() { return g_pass; }
extern "C" int pyr3d_fail_count() { return g_fail; }
