#!/usr/bin/env bash
set -e

# Adjust these for your setup
NDK="${ANDROID_NDK_HOME:-/path/to/android-ndk}"
API=21                       # 16 for old 32‑bit only, 21+ for 64‑bit
HOST_TAG=darwin-x86_64        # or linux-x86_64, windows-x86_64
LUAJIT_DIR="$(cd "$(dirname "$0")" && pwd)"

ABIS=("armeabi-v7a" "arm64-v8a" "x86" "x86_64")

BUILD_DIR="$LUAJIT_DIR/build-android"
OUT_DIR="$LUAJIT_DIR/out-android"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR" "$OUT_DIR"

build_abi() {
  local ABI="$1"
  local TARGET=
  local ARCH_FLAGS=
  local HOST_CC=
  local LJ_DEFS=

  case "$ABI" in
    armeabi-v7a)
      TARGET=armv7a-linux-androideabi
      ARCH_FLAGS="-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -Wl,--fix-cortex-a8"
      HOST_CC="gcc -m32"
      ;;
    arm64-v8a)
      TARGET=aarch64-linux-android
      ARCH_FLAGS=""
      HOST_CC="gcc -m64"
      LJ_DEFS="-DLUAJIT_ENABLE_GC64=1 -DLJ_ABI_SOFTFP=0 -DLJ_ARCH_HASFPU=1"
      ;;
    x86)
      TARGET=i686-linux-android
      ARCH_FLAGS=""
      HOST_CC="gcc -m32"
      ;;
    x86_64)
      TARGET=x86_64-linux-android
      ARCH_FLAGS=""
      HOST_CC="gcc -m64"
      LJ_DEFS="-DLUAJIT_ENABLE_GC64=1"
      ;;
    *)
      echo "Unknown ABI: $ABI"
      exit 1
      ;;
  esac

  local TOOLCHAIN_BIN="$NDK/toolchains/llvm/prebuilt/$HOST_TAG/bin"
  local TARGET_CC="${TOOLCHAIN_BIN}/${TARGET}${API}-clang"
  local TARGET_CXX="${TOOLCHAIN_BIN}/${TARGET}${API}-clang++"
  local CROSS="${TOOLCHAIN_BIN}/${TARGET}-"

  echo "==== Building LuaJIT for $ABI (target $TARGET, api $API) ===="

  pushd "$LUAJIT_DIR" >/dev/null

  make clean TARGET_SYS=Linux CROSS="$CROSS" TARGET_CC="$TARGET_CC"

  make \
    TARGET_SYS=Linux \
    CROSS="$CROSS" \
    TARGET_CC="$TARGET_CC" \
    TARGET_CFLAGS="-D__ANDROID_API__=$API $ARCH_FLAGS $LJ_DEFS" \
    HOST_CC="$HOST_CC" \
    CCDEBUG=-g \
    amalg

  popd >/dev/null

  mkdir -p "$OUT_DIR/$ABI"
  cp "$LUAJIT_DIR/src/libluajit.a" "$OUT_DIR/$ABI/"

  echo "=> Output: $OUT_DIR/$ABI/libluajit.a"
}

for ABI in "${ABIS[@]}"; do
  build_abi "$ABI"
done

echo "All ABIs built into: $OUT_DIR"
