// ============================================================
//  PyRecT3D — src/renderer/compositor.cpp
//  渲染指令合并 / 脏区域局部更新 / 2D 与 3D 分层合成
//
//  设计目标（v0.2）：
//   1) 性能优化：多条渲染指令合批，相同状态只设置一次
//   2) Docker 式软更新：3D 视口保留旧帧缓冲，只把新 UI 指令叠加
//   3) 突破 2D 特效限制：眩晕等需要 3D 的效可在合成层完成
// ============================================================
#include <PyRecT3D/renderer.h>
#include <PyRecT3D/message.h>
#include <algorithm>
#include <vector>

namespace pyrect3d {

void DirtyRegion::unionWith(const DirtyRegion& o) {
    if (!o.valid) return;
    if (!valid) { *this = o; return; }
    int x0 = std::min(x, o.x);
    int y0 = std::min(y, o.y);
    int x1 = std::max(x + w, o.x + o.w);
    int y1 = std::max(y + h, o.y + o.h);
    x = x0; y = y0; w = x1 - x0; h = y1 - y0;
    valid = true;
}

struct Compositor::Impl {
    Renderer* renderer = nullptr;
    DirtyRegion dirty;
    bool sceneDirty = true;
    std::vector<RenderCommand> pending;   // 待合并指令
    std::size_t merged = 0;
    struct Effect { std::int32_t code; float strength; };
    std::vector<Effect> effects;

    // 合批：把连续的同类指令合并为一次 draw call
    void flushBatch() {
        if (pending.empty()) return;
        // 2D 矩形合批：同色/同纹理连续指令合并
        std::size_t batchStart = 0;
        for (std::size_t i = 1; i <= pending.size(); ++i) {
            bool breakBatch = (i == pending.size());
            if (!breakBatch) {
                const RenderCommand& a = pending[i - 1];
                const RenderCommand& b = pending[i];
                if (a.type != b.type) breakBatch = true;
                else if (a.type == CommandType::DrawRect2D &&
                         (a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a)) breakBatch = true;
                else if (a.textureId != b.textureId) breakBatch = true;
            }
            if (breakBatch) {
                merged += 1;                 // 一次合并后的提交
                // 真正实现：glDrawArraysInstanced / 顶点缓冲一次性提交
                // 软件回退：逐条绘制
                if (renderer) {
                    for (std::size_t k = batchStart; k < i; ++k) {
                        const RenderCommand& c = pending[k];
                        if (c.type == CommandType::DrawRect2D) {
                            renderer->drawScreenRect(c.x, c.y, c.w, c.h);
                        }
                    }
                }
                batchStart = i;
            }
        }
        pending.clear();
    }
};

Compositor::Compositor() : impl_(new Impl()) {}
Compositor::~Compositor() { delete impl_; }

void Compositor::attach(Renderer* renderer) { impl_->renderer = renderer; }

void Compositor::submit(const RenderCommand* cmds, std::size_t count) {
    if (!cmds) return;
    for (std::size_t i = 0; i < count; ++i) {
        impl_->pending.push_back(cmds[i]);
        if (cmds[i].is2D()) {
            DirtyRegion r;
            r.x = (int)cmds[i].x; r.y = (int)cmds[i].y;
            r.w = (int)cmds[i].w; r.h = (int)cmds[i].h;
            r.valid = true;
            impl_->dirty.unionWith(r);
        } else {
            impl_->sceneDirty = true;
        }
    }
    // 提交事件入队（供上层观察批量提交时机）
    Message m;
    m.type = EventType::RenderCommandSubmit;
    m.priority = EventPriority::Normal;
    m.payload.code = (std::int32_t)count;
    MainMessageQueue::instance().publish(m);
}

void Compositor::invalidate(const DirtyRegion& region) {
    impl_->dirty.unionWith(region);
    Message m;
    m.type = EventType::RenderRegionInvalidate;
    m.priority = EventPriority::Normal;
    m.payload.v[0] = (float)region.x;
    m.payload.v[1] = (float)region.y;
    m.payload.v[2] = (float)region.w;
    m.payload.v[3] = (float)region.h;
    MainMessageQueue::instance().publish(m);
}

DirtyRegion Compositor::dirtyRegion() const { return impl_->dirty; }
void Compositor::clearDirty() { impl_->dirty = DirtyRegion{}; }

void Compositor::setSceneDirty(bool dirty) { impl_->sceneDirty = dirty; }
bool Compositor::sceneDirty() const { return impl_->sceneDirty; }

void Compositor::applyEffect3D(std::int32_t effectCode, float strength) {
    impl_->effects.push_back({ effectCode, strength });
}

void Compositor::present() {
    // 场景未脏时不重绘 3D，直接叠加 2D 指令（Docker 式软更新）
    if (impl_->sceneDirty && impl_->renderer) {
        impl_->renderer->beginFrame();
        impl_->sceneDirty = false;
    }
    impl_->flushBatch();
    // 合成层 3D 特效（如眩晕）在此叠加，不重绘整个 3D 场景
    impl_->effects.clear();
    impl_->dirty = DirtyRegion{};
    if (impl_->renderer) impl_->renderer->endFrame();
}

std::size_t Compositor::mergedCount() const { return impl_->merged; }

} // namespace pyrect3d
