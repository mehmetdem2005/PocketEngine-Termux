#!/bin/bash
# ============================================================
#  build.sh
#  Build the engine inside proot Debian
# ============================================================
set -e

SRC_DIR="${SRC_DIR:-/opt/pocketengine/src}"
BUILD_DIR="${BUILD_DIR:-/opt/pocketengine/build}"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "============================================================"
echo "  Building PocketEngine"
echo "  SRC:    $SRC_DIR"
echo "  BUILD:  $BUILD_DIR"
echo "============================================================"

# Configure
cmake "$SRC_DIR" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_COMPILER=clang \
    -DPOCKET_BUILD_EDITOR=ON \
    -DPOCKET_BUILD_RUNTIME=ON \
    -DPOCKET_BUILD_TESTS=OFF \
    -DPOCKET_USE_OPENGL_ES=ON \
    -DPOCKET_USE_BOX2D=OFF \
    -DPOCKET_USE_BULLET=OFF \
    -DPOCKET_USE_LUA=OFF \
    -DPOCKET_USE_OPENAL=OFF \
    -DPOCKET_USE_SQLITE=ON

# Build (use all cores)
NPROC=$(nproc)
echo "Building with $NPROC jobs..."
ninja -j"$NPROC"

echo ""
echo "Build OK. Binaries:"
ls -la "$BUILD_DIR/bin/"
