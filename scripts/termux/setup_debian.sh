#!/bin/bash
# ============================================================
#  setup_debian.sh
#  Step 2: Run inside proot Debian to install C++ toolchain
#  and all engine dependencies. This runs as root.
# ============================================================
set -e

echo "============================================================"
echo "  PocketEngine - Debian proot setup (Step 2 of 3)"
echo "============================================================"

# 1) Update + base tools
echo "[1/9] apt update + base tools..."
apt-get update -y
apt-get install -y \
    build-essential cmake ninja-build git pkg-config curl wget unzip \
    clang clang-format clang-tidy llvm \
    python3 python3-pip \
    libglm-dev libglfw3-dev libstb-dev \
    libsqlite3-dev liblua5.4-dev \
    libopenal-dev libvorbis-dev libflac-dev \
    libegl1 libgles2-mesa-dev libgl1-mesa-dev \
    libx11-dev libxext-dev libxrandr-dev libxinerama-dev libxcursor-dev \
    libxi-dev libxxf86vm-dev \
    mesa-utils vulkan-tools \
    fonts-dejavu fonts-noto-core \
    mesa-common-dev \
    libfreetype6-dev libpng-dev libjpeg-dev

# 2) Box2D (2D physics)
echo "[2/9] Installing Box2D..."
if [ ! -d /opt/Box2D ]; then
    git clone --depth 1 --branch v3.0.0 https://github.com/erincatto/Box2D.git /opt/Box2D
    cd /opt/Box2D && mkdir build && cd build && \
    cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=/usr -DBOX2D_BUILD_TESTBED=OFF -DBOX2D_BUILD_UNIT_TESTS=OFF && \
    ninja install
fi

# 3) Bullet3 (3D physics)
echo "[3/9] Installing Bullet3..."
if [ ! -d /opt/bullet3 ]; then
    git clone --depth 1 https://github.com/bulletphysics/bullet3.git /opt/bullet3
    cd /opt/bullet3 && mkdir build && cd build && \
    cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=/usr \
             -DBUILD_SHARED_LIBS=ON \
             -DINSTALL_LIBS=ON \
             -DBUILD_BULLET2_DEMOS=OFF \
             -DBUILD_BULLET3=OFF \
             -DBUILD_EXTRAS=OFF \
             -DBUILD_UNIT_TESTS=OFF && \
    ninja install
fi

# 4) ImGui (latest)
echo "[4/9] Installing ImGui..."
if [ ! -d /opt/imgui ]; then
    git clone --depth 1 https://github.com/ocornut/imgui.git /opt/imgui
fi

# 5) sol2 (Lua bindings)
echo "[5/9] Installing sol2..."
if [ ! -d /opt/sol2 ]; then
    git clone --depth 1 https://github.com/ThePhD/sol2.git /opt/sol2
    cd /opt/sol2 && mkdir build && cd build && \
    cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=/usr && ninja install
fi

# 6) nlohmann/json
echo "[6/9] Installing nlohmann/json..."
if [ ! -d /opt/json ]; then
    git clone --depth 1 https://github.com/nlohmann/json.git /opt/json
    cd /opt/json && mkdir build && cd build && \
    cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=/usr -DJSON_BuildTests=OFF && ninja install
fi

# 7) Create user (non-root) for safety
echo "[7/9] Creating 'pocket' user..."
if ! id -u pocket >/dev/null 2>&1; then
    useradd -m -s /bin/bash pocket
    echo "pocket ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
fi

# 8) Set up project directory
echo "[8/9] Setting up project dir at /opt/pocketengine..."
mkdir -p /opt/pocketengine
chown -R pocket:pocket /opt/pocketengine

# 9) Symlink termux home
echo "[9/9] Symlinking Termux home -> Debian..."
ln -sf /data/data/com.termux/files/home/PocketEngine-Termux /opt/pocketengine/src

echo ""
echo "============================================================"
echo "  Debian setup complete!"
echo "============================================================"
echo ""
echo "Build the engine:"
echo "  proot-distro login debian --user pocket --shared-tmp -- \\"
echo "    bash /opt/pocketengine/src/scripts/build/build.sh"
echo ""
echo "Run the editor:"
echo "  (from Termux) bash ~/pocket_x11.sh \\"
echo "    /opt/pocketengine/build/bin/pocketeditor"
