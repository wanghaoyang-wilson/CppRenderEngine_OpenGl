# Hard Constraints（不准踩的坑）

这份文档是**事故复现记录**。每一条都对应一个真实崩溃 / 视觉错误 / 性能问题。

## 1. OpenGL 上下文只能在渲染主线程使用

一个 GL 上下文同一时刻只能绑定在一个线程。

- 子线程**不得**调用任何 `gl*` 函数
- 子线程要渲染 → 构造渲染指令 → 压入队列 → 主线程 `MainMessageQueue::pump()` 消费执行
- `MainMessageQueue::threadId()` 用于校验当前线程是否有权操作 GPU 资源

```cpp
// ✅ 正确：子线程只生产指令
std::thread worker([&] {
    Message m;
    m.type = EventType::RenderCommandSubmit;
    m.payload.v[0] = 1.f;
    queue->publish(m);
});

// ❌ 错误：子线程直接 glBindTexture → 上下文错乱 / 崩
```

## 2. 输入向量必须钳位（斜向加速 bug）

按 W+A 时向量是 `(-1, 1)`，模长 `√2 ≈ 1.414`，**斜着走快 41%**。虚拟摇杆斜推同理。

```cpp
Vec2 dir = { x, y };
float len = dir.length();
if (len > 1.f) dir = dir.normalized();   // 模长 ≤ 1
velocity = dir * moveSpeed;              // 任意方向速度一致
```

- PC（WASD）与手机（摇杆）**共用同一套修正**，不需要各写一份
- 想要古早"斜向更快"手感才可关闭钳位（`clampInput = false`）

## 3. GPU 资源释放：父子依赖计数

规则：**父资源只有在所有子资源都不再依赖时才能删除**。

- 每有一个子资源，父资源依赖计数 +1
- 子资源全部释放 → 计数归零 → 父资源可删
- `release_gpu_resources()` 必须**幂等**（重复调用不崩溃、不重复释放）

触发时机：

| 平台 | 时机 |
|------|------|
| Web | `visibilitychange`（hidden）/ `pagehide` |
| Android | `onPause` / `onDestroy` |
| 桌面 | `WindowClose` / 进程退出 |

> 不释放、或释放后仍被引用 → 显存泄漏 / 段错误。

## 4. 渲染帧边界

v0.1 的坑：`drawEntity()` 内部每帧清屏，一帧画多个实体只剩最后一个。v0.2 必须：

```cpp
renderer.beginFrame();     // 只清一次
renderer.drawEntity(a);
renderer.drawEntity(b);
renderer.drawEntity(c);
renderer.endFrame();
```

## 5. 消息队列纪律

| 场景 | 发布者 |
|------|--------|
| 单个场景常规操作（变换 / 克隆 / 召唤 / 射线结果） | **主消息队列身份**（`sourceQueue = 0`） |
| 全局大型事件（切换鼠标模式、绑定 WASD、关闭程序、跨视口转交） | **主消息队列**（`publishGlobal`） |
| 窗口内部私有事件 | 该窗口子队列 |

- 多窗口时每个窗口**必须**有独立子队列，且**必须** `mountQueues()`
- `dispatch()` 未设置 handler 时不消费消息（保留给 `poll()`），别以为消息"丢了"

## 6. 射线：起点在盒内

`AABB::intersect` 在起点位于盒内时 `tNear < 0`。取距离必须写：

```cpp
float t = (tNear > 0.f) ? tNear : tFar;
```

只判 `tNear > 0` 会漏掉"贴着/在内部"的命中。

## 7. 双碰撞箱查询顺序

模型碰撞箱（含肢体）先判，物理箱兜底；命中取更小 `t`。
**要单独查物理箱时，请显式传 `includeModel=false`**，否则会被模型箱遮挡后误判"没命中物理箱"。

## 8. 视频上传只能在主线程

```
子线程：解码 → CPU 帧 → frameReady.store(true)
主线程：VideoPlayer::uploadPendingFrame() → glTexSubImage2D
```

不要在子线程调 `uploadPendingFrame()`。

## 9. 播放器事件必须按子队列路由

`targetQueue` = 播放器绑定的子队列 id。广播出去会让所有窗口都收到 `VideoFinished`，造成"别的窗口莫名停播"。

## 10. 不要用 return 硬截断底层

想关掉某条渲染路径时，**用编译期宏 + 源码级裁剪**（`PYRECT3D_WITH_*`），不要在原光栅化代码中间插 `return` 截断 —— 那会污染底层实现、难以回滚、牋一发动全身。

---

返回 [[Home]] · 事件细节见 [[Event Model]]
