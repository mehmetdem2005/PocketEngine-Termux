#!/data/data/com.termux/files/usr/bin/bash
# ============================================================
#  run_editor.sh
#  Launches the PocketEngine editor on Termux:X11
#  Usage: bash run_editor.sh
# ============================================================
set -e

export DISPLAY=:0
export PULSE_SERVER=127.0.0.1

EDITOR_BIN="/home/pocket/PocketEngine-build/bin/pocketeditor"

# Check that the editor binary exists
if ! proot-distro login debian --user pocket --shared-tmp -- \
        bash -c "test -x $EDITOR_BIN" 2>/dev/null; then
    echo "============================================================"
    echo "  Editor binary not found at: $EDITOR_BIN"
    echo "  Build it first:"
    echo "    proot-distro login debian --user pocket --shared-tmp -- \\"
    echo "      env SRC_DIR=/home/pocket/PocketEngine-Termux \\"
    echo "          BUILD_DIR=/home/pocket/PocketEngine-build \\"
    echo "      bash /home/pocket/PocketEngine-Termux/scripts/build/build.sh"
    echo "============================================================"
    exit 1
fi

# Start Termux:X11 server if not running
if ! pgrep -x "termux-x11" >/dev/null 2>&1; then
    echo "Starting Termux:X11 server..."
    termux-x11 :0 &
    sleep 2
fi

# Launch editor inside proot Debian
echo "Launching PocketEngine editor..."
proot-distro login debian --user pocket --shared-tmp -- \
    bash -c "export DISPLAY=:0; export PULSE_SERVER=127.0.0.1; $EDITOR_BIN"
