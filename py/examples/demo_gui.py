#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PyRecT3D 示例：自研极简 GUI（按钮/文本/输入框/容器 + OnClick/OnHover）"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import pyrect3d as p3d


def main():
    gui = p3d.Gui()

    # 按钮 + 事件
    btn = p3d.Button(p3d.Rect(10, 10, 120, 40), "Click me")
    btn.on_click = lambda: print("Button clicked!")
    btn.on_hover = lambda: print("Hovering button")
    gui.add_widget(btn)

    # 文本
    text = p3d.Text(p3d.Rect(10, 60, 300, 30), "Hello PyRecT3D GUI")
    gui.add_widget(text)

    # 输入框
    inp = p3d.InputBox(p3d.Rect(10, 100, 200, 30), "type here")
    gui.add_widget(inp)

    # 滚动容器
    sc = p3d.ScrollContainer(p3d.Rect(10, 150, 300, 200))
    sc.add_child(p3d.Text(p3d.Rect(0, 0, 280, 20), "scrollable content"))
    gui.add_widget(sc)

    print("GUI built. (Run inside an Engine loop to dispatch events.)")


if __name__ == "__main__":
    main()
