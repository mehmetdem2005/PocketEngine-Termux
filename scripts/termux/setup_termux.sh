#!/data/data/com.termux/files/usr/bin/bash
# ============================================================
#  setup_termux.sh
#  Step 1: Run inside Termux (not proot) to install base deps.
#  Then bootstraps a proot-distro Debian env with X11 + OpenGL ES
#  + C++ toolchain for PocketEngine.
# ============================================================
set -e

echo "============================================================"
echo "  PocketEngine - Termux setup (Step 1 of 3)"
echo "============================================================"

# 1) Update Termux
echo "[1/8] Updating Termux packages..."
pkg update -y
pkg install -y git cmake ninja clang make pkg-config \
                proot-distro termux-x11 termux-api \
                curl wget unzip jq python

# 2) Clone the engine source INTO TERMUX HOME FIRST
#    (so setup_debian.sh can symlink it later)
echo "[2/8] Cloning PocketEngine into Termux home..."
if [ ! -d "$HOME/PocketEngine-Termux" ]; then
    git clone https://github.com/mehmetdem2005/PocketEngine-Termux.git $HOME/PocketEngine-Termux
else
    echo "  (already cloned, pulling latest...)"
    cd $HOME/PocketEngine-Termux && git pull --rebase || true
fi

# 3) Install proot-distro Debian
echo "[3/8] Installing proot Debian (bookworm)..."
if ! proot-distro list | grep -q "debian.*installed"; then
    proot-distro install debian
fi

# 4) Set up Termux:X11 launch script
echo "[4/8] Creating Termux:X11 launch helper..."
cat > $HOME/pocket_x11.sh << 'X11'
#!/data/data/com.termux/files/usr/bin/bash
# Launch Termux:X11 server in background and run command inside proot
export DISPLAY=:0
termux-x11 :0 &
X11_PID=$!
sleep 2

# Run command in proot with DISPLAY forwarded (pocket user)
proot-distro login debian --user pocket --shared-tmp -- \
    bash -c "export DISPLAY=:0; export PULSE_SERVER=127.0.0.1; $*"

kill $X11_PID 2>/dev/null
X11
chmod +x $HOME/pocket_x11.sh

# 5) Bootstrap Debian env (copy setup_debian.sh into Termux home)
echo "[5/8] Copying setup_debian.sh into place..."
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cp "$SCRIPT_DIR/setup_debian.sh" $HOME/

# 6) Also clone the repo INTO Debian (so pocket user can access it)
echo "[6/8] Cloning repo into Debian proot..."
proot-distro login debian --user pocket --shared-tmp -- \
    bash -c "git clone https://github.com/mehmetdem2005/PocketEngine-Termux.git /home/pocket/PocketEngine-Termux 2>/dev/null || (cd /home/pocket/PocketEngine-Termux && git pull --rebase)"

# 7) Run debian setup (installs all C++ deps)
echo "[7/8] Bootstrapping Debian (this takes ~10 min)..."
proot-distro login debian --user root --shared-tmp -- \
    bash -c "cd /root && bash /data/data/com.termux/files/home/setup_debian.sh"

# 8) Build the engine
echo "[8/8] Building PocketEngine..."
proot-distro login debian --user pocket --shared-tmp -- \
    env SRC_DIR=/home/pocket/PocketEngine-Termux \
        BUILD_DIR=/home/pocket/PocketEngine-build \
    bash /home/pocket/PocketEngine-Termux/scripts/build/build.sh

echo ""
echo "============================================================"
echo "  Setup complete!"
echo "============================================================"
echo ""
echo "Run the editor:"
echo "  bash \$HOME/PocketEngine-Termux/scripts/termux/run_editor.sh"
echo ""
echo "Or manually:"
echo "  termux-x11 :0 &"
echo "  proot-distro login debian --user pocket --shared-tmp -- \\"
echo "    env DISPLAY=:0 /home/pocket/PocketEngine-build/bin/pocketeditor"
echo ""
echo "For more info: https://github.com/mehmetdem2005/PocketEngine-Termux"
