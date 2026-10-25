#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PyRecT3D 示例：基础 3D 立方体 + 渲染 + 帧导出"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import pyrect3d as p3d


def main():
    engine = p3d.Engine()
    if not engine.init(800, 600, "PyRecT3D Demo Cube"):
        print("Failed to init engine")
        return

    scene = p3d.Scene()
    cube = p3d.Cube(p3d.Vec3(0, 0, -5), p3d.Vec3(1, 1, 1))
    scene.add_entity(cube)

    renderer = p3d.Renderer()
    renderer.init(800, 600)
    renderer.set_clear_color(0.1, 0.1, 0.2)

    print("Running PyRecT3D demo. Press close window to exit.")

    def frame(state, dt):
        # 每帧渲染当前场景
        renderer.draw_entity(cube)

    engine.set_frame_callback(frame)
    engine.run()

    # 保存最后一帧
    renderer.save_frame(os.path.join(os.getcwd(), "frame.ppm"))
    print("Saved frame.ppm")


if __name__ == "__main__":
    main()
