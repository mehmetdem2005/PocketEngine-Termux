#!/bin/bash
# ============================================================
#  fix_and_build.sh - TEK KOMUTLA KURTARMA
#  Debian proot içine tüm eksik dependency'leri yükler,
#  ImGui'yi clone eder, stb_image çeker, build alır.
# ============================================================
set -e

echo "============================================================"
echo "  PocketEngine - FIX & BUILD (one-shot rescue)"
echo "============================================================"

# Step 1: Tüm C++ dependency'leri yeniden yükle
echo "[1/6] Installing ALL dependencies (apt)..."
apt-get update -y
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build git pkg-config curl wget unzip \
    clang clang-format \
    libglm-dev libglfw3-dev libstb-dev \
    libsqlite3-dev \
    libegl1 libgles2-mesa-dev libgl1-mesa-dev \
    libx11-dev libxext-dev libxrandr-dev libxinerama-dev libxcursor-dev \
    libxi-dev libxxf86vm-dev \
    mesa-common-dev \
    libfreetype6-dev libpng-dev libjpeg-dev
echo "  -> apt deps OK"

# Step 2: ImGui (docking branch - required for editor dockspace)
echo "[2/6] Installing ImGui (docking branch)..."
if [ ! -f /opt/imgui/imgui.h ]; then
    rm -rf /opt/imgui
    git clone --depth 1 --branch docking https://github.com/ocornut/imgui.git /opt/imgui
fi
echo "  -> ImGui at $(ls /opt/imgui/imgui.h 2>&1)"
echo "  -> Branch: $(cd /opt/imgui && git branch --show-current 2>&1)"

# Step 3: Verify pkg-config libs
echo "[3/6] Verifying pkg-config..."
for lib in glfw3 glesv2 egl sqlite3; do
    if pkg-config --exists $lib; then
        echo "  -> $lib: OK ($(pkg-config --modversion $lib))"
    else
        echo "  -> $lib: MISSING!"
        exit 1
    fi
done

# Step 4: Pull latest repo
echo "[4/6] Pulling latest PocketEngine..."
cd /home/pocket/PocketEngine-Termux
git pull --rebase 2>&1 | tail -3

# Step 5: Clean old build
echo "[5/6] Cleaning old build..."
rm -rf /home/pocket/PocketEngine-build
mkdir -p /home/pocket/PocketEngine-build

# Step 6: Build
echo "[6/6] Building..."
cd /home/pocket/PocketEngine-build
cmake /home/pocket/PocketEngine-Termux \
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
    -DPOCKET_USE_SQLITE=ON 2>&1 | tail -30

echo ""
echo "=== NINJA BUILD (full output to build.log) ==="
ninja 2>&1 | tee /home/pocket/build.log | tail -10
BUILD_EXIT=$?

echo ""
echo "============================================================"
echo "  BUILD RESULT"
echo "============================================================"
if [ -x /home/pocket/PocketEngine-build/bin/pocketeditor ]; then
    echo "✅ SUCCESS! Editor built."
    ls -la /home/pocket/PocketEngine-build/bin/
    echo ""
    echo "Run: bash /home/pocket/PocketEngine-Termux/scripts/termux/run_editor.sh"
else
    echo "❌ FAILED."
    echo ""
    echo "=== ACTUAL ERRORS (filtered from build.log) ==="
    grep -E "error:|FAILED:" /home/pocket/build.log | head -30
    echo ""
    echo "=== Last 30 lines of build.log ==="
    tail -30 /home/pocket/PocketEngine-build/build.log 2>/dev/null || tail -30 /home/pocket/build.log
    echo ""
    echo "Full log: /home/pocket/build.log"
    echo "To see all: cat /home/pocket/build.log"
fi

exit $BUILD_EXIT
