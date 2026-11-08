# Event Model（消息队列与事件优先级）

v0.2 的架构核心：**所有输入、场景操作、碰撞、媒体事件都走消息队列**，渲染指令也在主线程消费。

## 两级队列

```
┌─────────────────────────────────┐
│  MainMessageQueue（全局唯一，绑定渲染主线程） │
│   · 接收窗口级输入 / 程序级事件                │
│   · 按 targetQueue 路由到子队列                │
└───────────┬───────────────────┘
                │ attachChild
   ┌────────────┼────────────┐
   ▼            ▼            ▼
窗口 A 子队列  窗口 B 子队列  窗口 C 子队列
```

- **每个窗口实例自动获得一个子消息队列**（`WindowInstance::queue()`）
- 单窗口：引擎自动挂载；**多窗口：必须 `mountQueues()`**
- 主队列记录绑定线程 id（`threadId()`），GPU 资源操作只在该线程执行

## 六级优先级（数值越小越先派发）

| 优先级 | 值 | 事件 |
|--------|----|------|
| `WindowInput` | 0 | `WindowClose`、`WindowMinimize`、`WindowMaximize`、`WindowRestore`、`WindowResize`、`WindowBorderChange`、`WindowFocus*`、`KeyDown/Up`、`MouseMove`、`MouseButtonDown/Up`、`MouseWheel` |
| `Physics` | 1 | `PhysicsTick` |
| `Collision` | 2 | `CollisionEnter/Exit/Stay`、`CollisionPress/Release`、`CollisionDragOut/DragMove`、`CollisionClickOutside` |
| `Audio` | 3 | `AudioFinished` |
| `Media` | 4 | `VideoFinished`、`VideoPaused`、`VideoError`、`VideoFrameReady` |
| `Normal` | 5 | `TransformMove/Rotate/Scale`、`EntityClone`、`EntitySummon`、`EntityRemove`、`RaycastHitReport`、`RenderCommandSubmit`、`RenderRegionInvalidate` |

同优先级内按发布顺序 FIFO（`seq` 递增）。

> 为什么窗口输入最高？窗口关闭/最小化不能被长时间物理或媒体计算阻塞，否则界面"无响应"。

## 路由规则

| `targetQueue` | 行为 |
|---------------|------|
| `-1` | 广播：进全局队列，所有 `subscribe` 的处理器都收到 |
| `>= 0` 且子队列已挂载 | 直接投递到该子队列，**只有它收到** |
| `>= 0` 但子队列不存在 | 退化为全局队列 |

## 发布者身份

| 场景 | 发布方式 |
|------|----------|
| 单个场景内常规操作（变换 / 克隆 / 召唤 / 射线结果） | `sourceQueue = 0`（**主消息队列身份**） |
| 全局大型事件（切换鼠标模式、绑定 WASD、关闭程序、跨视口转交） | 主消息队列（`publishGlobal`） |
| 窗口内部私有事件 | 该窗口子队列 `publish()` |

## 消费方式

```cpp
// 方式 A：回调派发（需要先 setHandler）
queue->setHandler([](const Message& m) {
    if (m.type == EventType::KeyDown && m.payload.code == (int)Key::Escape) App::quit();
});
queue->dispatch();      // 没有 handler 时不会消费消息

// 方式 B：手动轮询
Message m;
while (queue->poll(m)) {
    // 处理
}

// 一帧调度
MainMessageQueue::instance().pump();   // 先全局，再各子队列 dispatch
```

> **注意**：`dispatch()` 在未设置 handler 时**不消费**消息，消息保留给 `poll()`。这是为了同时支持"回调"与"轮询"两种用法。

## 播放器事件路由示例

```
播放器 id=7，绑定子队列 1001
  ↓ 播放完成
Message{type=VideoFinished, priority=Media, targetQueue=1001, playerId=7}
  ↓ 投递主队列
主队列按 targetQueue 找到子队列 1001
  ↓
只有窗口 A 收到；窗口 B 不受影响
```

## 常用事件载荷约定

| 事件 | payload 用法 |
|------|--------------|
| `KeyDown` / `KeyUp` | `code` = 键码 |
| `MouseMove` | `v[0..3]` = x, y, dx, dy |
| `TransformMove/Rotate/Scale` | `entityId` + `v[0..2]` = 目标值 |
| `EntityClone` / `EntitySummon` | `entityId` = 新实体 id |
| `Collision*` | `entityId` = 区域所有者；`v[0..1]` 指针坐标；`v[2..5]` 区域边界；`v[6..7]` 拖拽位移 |
| `VideoFinished/Error` | `playerId` = 播放器 id；`code` = 错误码 |

---

返回 [[Home]] · 完整签名见 [[API Reference]]
