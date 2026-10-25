#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""PyRecT3D 示例：射线检测 + 软光追帧渲染"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import pyrect3d as p3d


def main():
    scene = p3d.Scene()
    cube = p3d.Cube(p3d.Vec3(0, 0, -5), p3d.Vec3(1, 1, 1))
    scene.add_entity(cube)

    # 射线检测：命中返回实体实例，上层处理业务
    hit = scene.raycast(p3d.Ray(p3d.Vec3(0, 0, 0), p3d.Vec3(0, 0, -1)))
    print("Raycast hit:", hit.hit, "dist=", hit.distance, "entity_id=", hit.entity_id)

    # 软光追渲染一帧
    tracer = p3d.SoftRayTracer()
    out = bytearray(800 * 600 * 3)
    tracer.render(scene, 800, 600, out)

    # 写出 PPM
    path = os.path.join(os.getcwd(), "raytrace.ppm")
    with open(path, "wb") as f:
        f.write(b"P6\n800 600\n255\n")
        f.write(bytes(out))
    print("Saved raytrace.ppm")


if __name__ == "__main__":
    main()
