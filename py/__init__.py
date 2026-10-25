# ============================================================
#  PyRecT3D — py/__init__.py
#  Python 库入口：动态导入编译好的底层扩展
#  （扩展依赖核心动态库 PyRecT3D.dll / libPyRecT3D.so）
# ============================================================
import os
import sys

# 允许从 py/package 直接加载扩展
_pkg = os.path.dirname(os.path.abspath(__file__))

# 尝试加载编译产物目录
for _cand in (os.path.join(_pkg, "package"), _pkg):
    _p = os.path.join(_cand, "pyrect3d_binding")
    if os.path.isdir(_cand) and any(
        f.startswith("pyrect3d_binding") and f.endswith((".pyd", ".so"))
        for f in os.listdir(_cand)
    ):
        sys.path.insert(0, _cand)
        break

try:
    from pyrect3d_binding import *  # noqa
    from pyrect3d_binding import __doc__  # noqa
except ImportError as e:
    raise ImportError(
        "PyRecT3D Python extension not built. Run: python py/build.py\n"
        "Original error: %s" % e
    ) from e

__version__ = "0.1.0"
