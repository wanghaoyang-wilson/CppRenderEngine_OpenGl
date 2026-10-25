#!/usr/bin/env python3
# ============================================================
#  PyRecT3D — py/build.py
#  一键编译 Python 扩展模块（依赖核心动态库）
#  用法：python py/build.py
# ============================================================
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(ROOT, "build")
PKG_DIR = os.path.join(ROOT, "py", "package")


def run(cmd):
    print(">>", " ".join(cmd))
    r = subprocess.run(cmd, cwd=BUILD_DIR)
    if r.returncode != 0:
        sys.exit(r.returncode)


def main():
    os.makedirs(BUILD_DIR, exist_ok=True)
    os.makedirs(PKG_DIR, exist_ok=True)

    # 配置
    if not os.path.exists(os.path.join(BUILD_DIR, "CMakeCache.txt")):
        run(["cmake", ROOT, "-DCMAKE_BUILD_TYPE=Release",
             "-DPYRECT3D_BUILD_PYTHON=ON"])

    # 构建（core dll + binding）
    if os.name == "nt":
        run(["cmake", "--build", ".", "--config", "Release"])
    else:
        run(["cmake", "--build", ".", "-j", str(os.cpu_count() or 2)])

    # 生成包入口（让 import pyrect3d 找到扩展）
    init_py = os.path.join(PKG_DIR, "__init__.py")
    with open(init_py, "w") as f:
        f.write('"""PyRecT3D Python binding entry."""\n'
                'from .pyrect3d_binding import *  # noqa\n'
                '__version__ = "0.1.0"\n')

    print("\nBuild OK. Python extension in:", PKG_DIR)
    print("Add to path or run: pip install -e ./py  (see README)")


if __name__ == "__main__":
    main()
