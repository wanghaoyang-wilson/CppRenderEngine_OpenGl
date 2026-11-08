#!/usr/bin/env bash
# ============================================================
#  PyRecT3D v0.2 — Linux / Android(GLES3) 构建脚本
#
#  与 build.bat 保持一致：不依赖 CMake，直接调 g++ 编译源码。
#  用法：
#    ./build.sh                      默认：Release + 共享库 + GL 关
#    ./build.sh --static             静态库 libPyRecT3D.a
#    ./build.sh --gl=on              启用 glad + OpenGL 3.3 Core
#    ./build.sh --trim=video,audio   裁剪模块（gui / video / audio / physics）
#    ./build.sh --examples           额外编三个 C++ 示例
#    ./build.sh --tests              编译并运行 smoke_v2 冒烟测试
#    ./build.sh --debug              -O0 -g
# ============================================================
set -euo pipefail
cd "$(dirname "$0")"

SRC=cpp/src
INC=cpp/include
GLAD=cpp/third_party/glad
OUT=build
OBJDIR="$OUT/obj"

LIBTYPE=shared
GL=0
CFG=Release
EXAMPLES=n
TESTS=n
GUI=1; VIDEO=1; AUDIO=1; PHYSICS=1
CXX=${CXX:-g++}

for arg in "$@"; do
  case "$arg" in
    --static)   LIBTYPE=static ;;
    --gl=on)    GL=1 ;;
    --gl=off)   GL=0 ;;
    --debug)    CFG=Debug ;;
    --examples) EXAMPLES=y ;;
    --tests)    TESTS=y ;;
    --trim=*)
      list="${arg#--trim=}"
      IFS=',' read -ra mods <<< "$list"
      for m in "${mods[@]}"; do
        case "$m" in
          gui)     GUI=0 ;;
          video)   VIDEO=0 ;;
          audio)   AUDIO=0 ;;
          physics) PHYSICS=0 ;;
          *) echo "[build] 未知模块：$m（可选 gui/video/audio/physics）" >&2 ; exit 1 ;;
        esac
      done ;;
    -h|--help) sed -n '2,16p' "$0" ; exit 0 ;;
    *) echo "[build] 未知参数：$arg" >&2 ; exit 1 ;;
  esac
done

if [ "$CFG" = "Release" ]; then
  OPT="-O3 -fPIC -ffunction-sections -fdata-sections -DNDEBUG"
  LOPT="-Wl,--gc-sections"
else
  OPT="-O0 -g -fPIC"
  LOPT=""
fi

DEFS="-DPYRECT3D_WITH_GL=$GL -DPYRECT3D_WITH_GUI=$GUI -DPYRECT3D_WITH_VIDEO=$VIDEO -DPYRECT3D_WITH_AUDIO=$AUDIO -DPYRECT3D_WITH_PHYSICS=$PHYSICS"
DEFS="$DEFS -I$INC -I$SRC -I$GLAD"

FILES=(
  "$SRC/foundation/math/math.cpp"
  "$SRC/foundation/io/obj_loader.cpp"
  "$SRC/core/engine.cpp"
  "$SRC/core/event_loop.cpp"
  "$SRC/core/message_queue.cpp"
  "$SRC/core/explicit_app.cpp"
  "$SRC/core/implicit_api.cpp"
  "$SRC/graphics/renderer.cpp"
  "$SRC/graphics/compositor.cpp"
  "$SRC/graphics/gl_backend.cpp"
  "$SRC/core/platform/x11_input.cpp"
)
[ "$GUI" = "1" ]     && FILES+=("$SRC/ui/gui.cpp")
[ "$VIDEO" = "1" ]   && FILES+=("$SRC/media/video.cpp")
[ "$AUDIO" = "1" ]   && FILES+=("$SRC/audio/audio.cpp")
[ "$PHYSICS" = "1" ] && FILES+=("$SRC/physics/collision_world.cpp")

rm -rf "$OUT"
mkdir -p "$OBJDIR"

echo "[build] 产物=$LIBTYPE 配置=$CFG GL=$GL GUI=$GUI VIDEO=$VIDEO AUDIO=$AUDIO PHYSICS=$PHYSICS"

OBJS=()
for f in "${FILES[@]}"; do
  o="$OBJDIR/$(echo "$f" | sed 's|/|_|g').o"
  $CXX -std=c++17 $OPT $DEFS -c "$f" -o "$o"
  OBJS+=("$o")
done

if [ "$GL" = "1" ]; then
  gcc $OPT $DEFS -c "$GLAD/glad.c" -o "$OBJDIR/glad.o"
  OBJS+=("$OBJDIR/glad.o")
fi

LIBS="-lX11 -ldl -lpthread"
[ "$GL" = "1" ] && LIBS="$LIBS -lGL"

if [ "$LIBTYPE" = "shared" ]; then
  $CXX -shared $LOPT -o "$OUT/libPyRecT3D.so" "${OBJS[@]}" $LIBS
else
  ar rcs "$OUT/libPyRecT3D.a" "${OBJS[@]}"
fi

if [ "$EXAMPLES" = "y" ]; then
  for ex in basic_cube explicit_demo implicit_demo; do
    if [ "$LIBTYPE" = "shared" ]; then
      $CXX -std=c++17 $OPT -I"$INC" "examples_cpp/$ex.cpp" -L"$OUT" -lPyRecT3D \
        -Wl,-rpath,'$ORIGIN' -o "$OUT/$ex"
    else
      $CXX -std=c++17 $OPT -I"$INC" "examples_cpp/$ex.cpp" "$OUT/libPyRecT3D.a" $LIBS -o "$OUT/$ex"
    fi
  done
fi

if [ "$TESTS" = "y" ]; then
  if [ "$LIBTYPE" = "shared" ]; then
    $CXX -std=c++17 $OPT -I"$INC" tests/smoke_v2.cpp -L"$OUT" -lPyRecT3D \
      -Wl,-rpath,'$ORIGIN' -o "$OUT/smoke_v2"
  else
    $CXX -std=c++17 $OPT -I"$INC" tests/smoke_v2.cpp "$OUT/libPyRecT3D.a" $LIBS -o "$OUT/smoke_v2"
  fi
  echo "[build] 冒烟测试："
  "$OUT/smoke_v2" | tail -1
fi

echo "[build] 完成：$(ls -1 "$OUT" | grep -v obj | tr '\n' ' ')"
