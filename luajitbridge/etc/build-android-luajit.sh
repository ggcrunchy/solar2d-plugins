#!/usr/bin/env bash
set -e

NDK="$HOME/Library/Android/sdk/ndk/24.0.8215888"
NDKBIN="$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin"
NDKCROSS="$NDKBIN/arm-linux-androideabi-"

API=21                       # 16 for old 32‑bit only, 21+ for 64‑bit
LUAJIT_DIR="$(cd "$(dirname "$0")" && pwd)"

ABIS=("arm64-v8a" "x86_64")

# "armeabi-v7a" "arm64-v8a" "x86" "x86_64"

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
      ARCH_FLAGS="-msoft-float -marm"
      HOST_CC="gcc -m32"
      ;;
    arm64-v8a)
      TARGET=aarch64-linux-android
      ARCH_FLAGS=""
      HOST_CC="clang" #gcc"
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
      HOST_CC="clang" #gcc"
      LJ_DEFS="-DLUAJIT_ENABLE_GC64=1"
      ;;
    *)
      echo "Unknown ABI: $ABI"
      exit 1
      ;;
  esac

NDKCC="$NDKBIN/$TARGET$API-clang"


#make HOST_CC="gcc -m32" CROSS=$NDKCROSS \
     STATIC_CC=$NDKCC DYNAMIC_CC="$NDKCC -fPIC" \
     TARGET_LD=$NDKCC TARGET_AR="$NDKBIN/llvm-ar rcus" \
     TARGET_STRIP=$NDKBIN/llvm-strip


  echo "==== Building LuaJIT for $ABI (target $TARGET, api $API) ===="

  pushd "$LUAJIT_DIR" >/dev/null

make clean TARGET_SYS=Linux CROSS="$NDKCROSS" TARGET_CC="$STATIC_CC"


make TARGET_SYS=Linux HOST_CC=$HOST_CC CROSS=$NDKCROSS \
     STATIC_CC=$NDKCC DYNAMIC_CC="$NDKCC -fPIC" \
     TARGET_LD=$NDKCC TARGET_AR="$NDKBIN/llvm-ar rcus" \
     TARGET_STRIP=$NDKBIN/llvm-strip

  popd >/dev/null

  mkdir -p "$OUT_DIR/$ABI"
  #cp "$LUAJIT_DIR/libluajit.a" "$OUT_DIR/$ABI/"

  ./rename_luajit.sh libluajit.a luajit_symbols.map $NDKBIN
  
  cp "$LUAJIT_DIR/libluajit_lj.a" "$OUT_DIR/$ABI/"
  
  echo "=> Output: $OUT_DIR/$ABI/libluajit.a"
}

for ABI in "${ABIS[@]}"; do
  build_abi "$ABI"
done

echo "All ABIs built into: $OUT_DIR"
