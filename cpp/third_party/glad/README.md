# glad: 轻量 OpenGL/GLES 加载器（离线合并头 + X-macro 函数表）

桌面走 WGL/GLX 自动探测，Android 注入 EGL loader，Web 由链接期解析。
函数入口只有一张 `PYR3D_GL_FUNC_LIST(F)` 表，声明/定义/加载/别名宏全部由它派生。
