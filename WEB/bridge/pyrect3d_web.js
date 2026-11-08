// ============================================================
//  PyRecT3D — WEB/bridge/pyrect3d_web.js
//  浏览器侧对接层：WebGL2 上下文 / 输入转发 / 生命周期 / 资源释放
//  上层（C++ 与 JS）API 与桌面端保持完全一致，平台差异只在本文件消化。
// ============================================================
(function (global) {
  "use strict";

  const PyRecT3DWeb = {
    module: null,
    canvas: null,
    gl: null,
    _input: {
      keys: {}, mouseX: 0, mouseY: 0,
      mouseDX: 0, mouseDY: 0,
      buttons: [false, false, false],
      points: new Map(), // 多点触控
    },
    _joy: { active: false, x: 0, y: 0, id: -1 },

    // ---------- 初始化 ----------
    async init(options = {}) {
      const {
        canvas,
        wasmPath = "pyr3d_single.js",
        width = 1280,
        height = 720,
        clampInput = true,
      } = options;

      this.canvas = canvas || document.createElement("canvas");
      this.canvas.width = width;
      this.canvas.height = height;
      document.body.appendChild(this.canvas);

      // WebGL2 = ES 3.0 语义，与引擎 GLES3 后端一一对应
      this.gl = this.canvas.getContext("webgl2", {
        antialias: true, alpha: false, depth: true, stencil: false,
        preserveDrawingBuffer: false, powerPreference: "high-performance",
      });
      if (!this.gl) throw new Error("当前浏览器不支持 WebGL2，无法运行 PyRecT3D");

      this._bindInput(clampInput);
      this._bindLifecycle();

      this.module = await this._loadWasm(wasmPath);
      if (this.module && this.module._pyr3d_init) {
        this.module._pyr3d_init(width, height);
      }
      return this;
    },

    _loadWasm(path) {
      // 优先使用预编译的 MODULARIZE 入口
      if (global.createPyRecT3D) {
        return global.createPyRecT3D({ locateFile: (f) => f === path ? path : f });
      }
      return new Promise((resolve, reject) => {
        const s = document.createElement("script");
        s.src = path;
        s.onload = () => {
          if (global.createPyRecT3D) {
            resolve(global.createPyRecT3D({}));
          } else {
            reject(new Error("WASM 入口未导出 createPyRecT3D"));
          }
        };
        s.onerror = () => reject(new Error("WASM 加载失败: " + path));
        document.head.appendChild(s);
      });
    },

    // ---------- 输入转发（键盘 / 鼠标 / 触屏摇杆） ----------
    _bindInput(clampInput) {
      const c = this.canvas;

      window.addEventListener("keydown", (e) => {
        this._input.keys[e.code] = true;
        this._publish("KeyDown", e.keyCode || 0);
      });
      window.addEventListener("keyup", (e) => {
        this._input.keys[e.code] = false;
        this._publish("KeyUp", e.keyCode || 0);
      });

      c.addEventListener("mousemove", (e) => {
        const r = c.getBoundingClientRect();
        const nx = e.clientX - r.left, ny = e.clientY - r.top;
        this._input.mouseDX = nx - this._input.mouseX;
        this._input.mouseDY = ny - this._input.mouseY;
        this._input.mouseX = nx; this._input.mouseY = ny;
        this._publish("MouseMove", 0);
      });

      ["mousedown", "mouseup"].forEach((type) => {
        c.addEventListener(type, (e) => {
          const idx = e.button === 0 ? 0 : e.button === 2 ? 1 : 2;
          this._input.buttons[idx] = (type === "mousedown");
          this._publish(type === "mousedown" ? "MouseButtonDown" : "MouseButtonUp", e.button);
        });
      });
      c.addEventListener("contextmenu", (e) => e.preventDefault());

      // 多点触控：单指喂鼠标，双指/摇杆区喂虚拟摇杆
      c.addEventListener("touchstart", (e) => {
        for (const t of e.changedTouches) {
          this._input.points.set(t.identifier, { x: t.clientX, y: t.clientY });
        }
        e.preventDefault();
      }, { passive: false });
      c.addEventListener("touchmove", (e) => {
        for (const t of e.changedTouches) {
          if (!this._input.points.has(t.identifier)) continue;
          this._input.points.set(t.identifier, { x: t.clientX, y: t.clientY });
        }
        this._updateJoystick(clampInput);
        e.preventDefault();
      }, { passive: false });
      c.addEventListener("touchend", (e) => {
        for (const t of e.changedTouches) this._input.points.delete(t.identifier);
        if (this._input.points.size === 0) { this._joy.active = false; this._joy.x = 0; this._joy.y = 0; }
        e.preventDefault();
      }, { passive: false });
    },

    // 虚拟摇杆：向量钳位到模长 <= 1，避免斜向 1.414 倍加速
    _updateJoystick(clampInput) {
      const pts = [...this._input.points.values()];
      if (pts.length < 2) return;
      const origin = pts[0], cur = pts[1];
      let dx = (cur.x - origin.x) / 80;
      let dy = (cur.y - origin.y) / 80;
      if (clampInput) {
        const len = Math.hypot(dx, dy);
        if (len > 1) { dx /= len; dy /= len; }
      }
      this._joy.active = true; this._joy.x = dx; this._joy.y = dy;
      this._publish("JoystickMove", 0);
    },

    _publish(type, code) {
      // 统一进入引擎消息队列（主队列按优先级调度）
      if (this.module && this.module._pyr3d_publish_event) {
        this.module._pyr3d_publish_event(type, code,
          this._input.mouseX, this._input.mouseY,
          this._joy.x, this._joy.y);
      }
    },

    // ---------- 生命周期：暂停即释放 GPU 资源 ----------
    _bindLifecycle() {
      document.addEventListener("visibilitychange", () => {
        if (document.hidden) {
          this._call("_pyr3d_app_pause");
          this._call("_pyr3d_release_gpu_resources");
        } else {
          this._call("_pyr3d_app_resume");
        }
      });
      window.addEventListener("pagehide", () => {
        this._call("_pyr3d_release_gpu_resources");
      });
      window.addEventListener("resize", () => {
        this._call("_pyr3d_on_resize", this.canvas.clientWidth, this.canvas.clientHeight);
      });
    },

    _call(fn, ...args) {
      if (this.module && typeof this.module[fn] === "function") {
        return this.module[fn](...args);
      }
      return undefined;
    },

    // ---------- 主循环 ----------
    start() {
      const loop = () => {
        const ok = this._call("_pyr3d_app_pump");
        if (ok === false) return;      // 引擎请求退出
        requestAnimationFrame(loop);
      };
      requestAnimationFrame(loop);
    },

    // ---------- 便捷 API（与桌面端同名） ----------
    cube(x = 0, y = 0, z = -5, sx = 1, sy = 1, sz = 1) {
      return this._call("_pyr3d_quick_cube", x, y, z, sx, sy, sz);
    },
    clone(id) { return this._call("_pyr3d_quick_clone", id); },
    move(id, x, y, z) { return this._call("_pyr3d_quick_move", id, x, y, z); },
    video(path, loop) { return this._call("_pyr3d_quick_video", path, loop ? 1 : 0); },
  };

  global.PyRecT3DWeb = PyRecT3DWeb;
}(window));
