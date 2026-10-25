// ============================================================
//  PyRecT3D/gui.h — 自研极简 GUI（契约层，仅声明）
//  组件：按钮 / 文本 / 输入框 / 复杂容器 / 滚动容器
//  事件：OnHover / OnClick，挂载在消息循环
//  支持：组件贴图
//  不含：表格 / 树 / 标签页 / Dock / 动画 / 主题
// ============================================================
#pragma once
#include "export.h"
#include "math.h"
#include <functional>
#include <string>

namespace pyrect3d {

struct PYRECT3D_API Rect {
    float x = 0, y = 0, w = 0, h = 0;
    bool contains(float px, float py) const;
};

// 事件类型
struct PYRECT3D_API GuiEvent {
    bool hovered = false;
    bool clicked = false;
    int  mouseX = 0, mouseY = 0;
};

class PYRECT3D_API Widget {
public:
    explicit Widget(const Rect& r);
    virtual ~Widget();

    void setRect(const Rect& r);
    Rect rect() const;
    void setVisible(bool v);
    bool visible() const;

    // 贴图（路径，GUI 组件支持贴图美化）
    void setTexture(const std::string& texPath);
    const std::string& texture() const;

    virtual void onEvent(const GuiEvent& e) {}
    // 内部渲染由 gui 系统调用
    virtual void draw() = 0;

protected:
    Rect rect_;
    bool visible_ = true;
    std::string texture_;
};

class PYRECT3D_API Button : public Widget {
public:
    Button(const Rect& r, const std::string& text = "");
    void onEvent(const GuiEvent& e) override;
    void draw() override;

    std::function<void()> onClick;
    std::function<void()> onHover;
    std::string text;

private:
    bool hovered_ = false;
};

class PYRECT3D_API Text : public Widget {
public:
    Text(const Rect& r, const std::string& text = "");
    void draw() override;

    std::string text;
    float size = 16.f;
};

class PYRECT3D_API InputBox : public Widget {
public:
    InputBox(const Rect& r, const std::string& text = "");
    void onEvent(const GuiEvent& e) override;
    void draw() override;

    std::string text;
    bool focused = false;
};

// 复杂容器：可含子组件、布局管理
class PYRECT3D_API Container : public Widget {
public:
    explicit Container(const Rect& r);
    void addChild(Widget* w);
    void draw() override;

private:
    struct Impl;
    Impl* impl_;
};

// 滚动容器：包含可滚动内容区域
class PYRECT3D_API ScrollContainer : public Container {
public:
    explicit ScrollContainer(const Rect& r);
    void onEvent(const GuiEvent& e) override;  // 处理滚轮
    void setScroll(float offset);
    float scroll() const;

private:
    float scroll_ = 0.f;
};

// GUI 系统：管理所有组件、事件派发
class PYRECT3D_API Gui {
public:
    Gui();
    ~Gui();

    void addWidget(Widget* w);
    void removeWidget(Widget* w);

    // 由消息循环调用：根据鼠标状态派发 OnHover / OnClick
    void dispatch(const GuiEvent& base);
    void drawAll();

private:
    struct Impl;
    Impl* impl_;
};

} // namespace pyrect3d
