#!/data/data/com.termux/files/usr/bin/bash
# ============================================================
#  setup_termux.sh
#  Step 1: Run inside Termux (not proot) to install base deps
#  Then this script will bootstrap a proot-distro Debian env
#  with X11 + OpenGL ES + C++ toolchain for PocketEngine.
# ============================================================
set -e

echo "============================================================"
echo "  PocketEngine - Termux setup (Step 1 of 3)"
echo "============================================================"

# 1) Update Termux
echo "[1/6] Updating Termux packages..."
pkg update -y
pkg install -y git cmake ninja clang make pkg-config \
                proot-distro termux-x11 termux-api \
                curl wget unzip jq python

# 2) Install proot-distro Debian
echo "[2/6] Installing proot Debian (bookworm)..."
if ! proot-distro list | grep -q "debian.*installed"; then
    proot-distro install debian
fi

# 3) Set up Termux:X11 launch script
echo "[3/6] Creating Termux:X11 launch helper..."
cat > $HOME/pocket_x11.sh << 'X11'
#!/data/data/com.termux/files/usr/bin/bash
# Launch Termux:X11 server in background and run command inside proot
export DISPLAY=:0
termux-x11 :0 &
X11_PID=$!
sleep 1

# Run command in proot with DISPLAY forwarded
proot-distro login debian --user $USER --shared-tmp -- \
    bash -c "export DISPLAY=:0; export PULSE_SERVER=127.0.0.1; $*"

kill $X11_PID 2>/dev/null
X11
chmod +x $HOME/pocket_x11.sh

# 4) Bootstrap Debian env (copy step 2 script inside)
echo "[4/6] Bootstrapping Debian proot env..."
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cp "$SCRIPT_DIR/setup_debian.sh" $HOME/

# 5) Run debian setup
proot-distro login debian --user root --shared-tmp -- \
    bash -c "cd /root && bash /data/data/com.termux/files/home/setup_debian.sh"

# 6) Clone the engine source
echo "[5/6] Cloning PocketEngine..."
if [ ! -d "$HOME/PocketEngine-Termux" ]; then
    git clone https://github.com/mehmetdem2005/PocketEngine-Termux.git $HOME/PocketEngine-Termux
fi

echo "[6/6] Setup complete!"
echo ""
echo "Next steps:"
echo "  1. Run the editor:    bash ~/pocket_x11.sh /opt/pocketengine/build/bin/pocketeditor"
echo "  2. Or build manually: proot-distro login debian -- bash /opt/pocketengine/scripts/build/build.sh"
echo ""
echo "For more info see: https://github.com/mehmetdem2005/PocketEngine-Termux"
