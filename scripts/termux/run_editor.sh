#!/data/data/com.termux/files/usr/bin/bash
# ============================================================
#  run_editor.sh
#  Launches the PocketEngine editor on Termux:X11
#  Usage: bash run_editor.sh
# ============================================================
set -e

export DISPLAY=:0
export PULSE_SERVER=127.0.0.1

# Start Termux:X11 server if not running
if ! pgrep -x "termux-x11" >/dev/null 2>&1; then
    echo "Starting Termux:X11 server..."
    termux-x11 :0 &
    sleep 2
fi

# Launch editor inside proot Debian
proot-distro login debian --user pocket --shared-tmp -- \
    bash -c "export DISPLAY=:0; /opt/pocketengine/build/bin/pocketeditor"
