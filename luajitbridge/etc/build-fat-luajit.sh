#!/bin/bash
# build-fat-macos.sh - Fat libluajit.a for macOS 10.13+ (run from src/)

set -e

LIB_NAME="libluajit.a"
BUILD_DIR="../luajit-build"
SLICES_DIR="$BUILD_DIR/slices"
rm -rf "$BUILD_DIR" && mkdir -p "$SLICES_DIR"

echo "Building fat LuaJIT for macOS 10.13+..."

# Build slices
for ARCH in x86_64 arm64; do
    echo "Building $ARCH..."
    make clean >/dev/null 2>&1
    make HOST_CC=clang \
         CC="clang -arch $ARCH -mmacosx-version-min=10.13" \
         CFLAGS="-O3 -fvisibility=hidden -DLUAJIT_ENABLE_LUA52COMPAT" \
         MACOSX_DEPLOYMENT_TARGET=10.13 \
         "$LIB_NAME" || exit 1
    cp "$LIB_NAME" "$SLICES_DIR/$LIB_NAME.$ARCH"
done

# Merge
lipo -output "$LIB_NAME" -create "$SLICES_DIR/$LIB_NAME".*
echo "Fat libluajit.a created: $(lipo -info "$LIB_NAME")"
