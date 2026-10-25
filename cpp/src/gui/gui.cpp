// ============================================================
//  PyRecT3D — gui.cpp
//  自研极简 GUI 实现
//  组件：按钮/文本/输入框/复杂容器/滚动容器
//  事件：OnHover / OnClick；支持贴图（简化加载）
//  不含：表格/树/标签页/Dock/动画/主题
// ============================================================
#include <PyRecT3D/gui.h>
#include <vector>
#include <algorithm>

namespace pyrect3d {

bool Rect::contains(float px, float py) const {
    return px >= x && px <= x+w && py >= y && py <= y+h;
}

// ---------- Widget ----------
Widget::Widget(const Rect& r) : rect_(r) {}
Widget::~Widget() {}
void Widget::setRect(const Rect& r) { rect_ = r; }
Rect Widget::rect() const { return rect_; }
void Widget::setVisible(bool v) { visible_ = v; }
bool Widget::visible() const { return visible_; }
void Widget::setTexture(const std::string& texPath) { texture_ = texPath; }
const std::string& Widget::texture() const { return texture_; }

// ---------- Button ----------
Button::Button(const Rect& r, const std::string& text)
    : Widget(r), text(text) {}
void Button::onEvent(const GuiEvent& e) {
    bool hov = visible_ && rect_.contains((float)e.mouseX, (float)e.mouseY);
    if (hov && !hovered_ && onHover) onHover();
    hovered_ = hov;
    if (hov && e.clicked && onClick) onClick();
}
void Button::draw() {
    // 简化：无实际渲染，占位（生产接 renderer 绘制矩形 + 文本 + 贴图）
}

// ---------- Text ----------
Text::Text(const Rect& r, const std::string& text)
    : Widget(r), text(text) {}
void Text::draw() {}

// ---------- InputBox ----------
InputBox::InputBox(const Rect& r, const std::string& text)
    : Widget(r), text(text) {}
void InputBox::onEvent(const GuiEvent& e) {
    bool hov = visible_ && rect_.contains((float)e.mouseX, (float)e.mouseY);
    if (hov && e.clicked) focused = true;
    else if (e.clicked) focused = false;
}
void InputBox::draw() {}

// ---------- Container ----------
struct Container::Impl {
    std::vector<Widget*> children;
};
Container::Container(const Rect& r) : Widget(r), impl_(new Impl()) {}
void Container::addChild(Widget* w) { if (w) impl_->children.push_back(w); }
void Container::draw() { for (auto* c : impl_->children) if (c->visible()) c->draw(); }

// ---------- ScrollContainer ----------
ScrollContainer::ScrollContainer(const Rect& r) : Container(r) {}
void ScrollContainer::onEvent(const GuiEvent& e) {
    // 处理滚轮：此处演示用鼠标中键滚动
    if (visible_ && rect_.contains((float)e.mouseX, (float)e.mouseY)) {
        // 简化：e 未带 wheel，实际由平台层提供滚轮增量
    }
}
void ScrollContainer::setScroll(float offset) { scroll_ = offset; }
float ScrollContainer::scroll() const { return scroll_; }

// ---------- Gui ----------
struct Gui::Impl {
    std::vector<Widget*> widgets;
};
Gui::Gui() : impl_(new Impl()) {}
Gui::~Gui() { delete impl_; }
void Gui::addWidget(Widget* w) { if (w) impl_->widgets.push_back(w); }
void Gui::removeWidget(Widget* w) {
    impl_->widgets.erase(std::remove(impl_->widgets.begin(), impl_->widgets.end(), w),
                         impl_->widgets.end());
}
void Gui::dispatch(const GuiEvent& base) {
    // 逆序派发（上层组件优先）
    for (auto it = impl_->widgets.rbegin(); it != impl_->widgets.rend(); ++it) {
        if ((*it)->visible()) (*it)->onEvent(base);
    }
}
void Gui::drawAll() {
    for (auto* w : impl_->widgets) if (w->visible()) w->draw();
}

} // namespace pyrect3d
