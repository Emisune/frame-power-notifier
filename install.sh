#!/bin/sh
# Build on the headset and install as a systemd user service tied to SteamVR.
# Run as the normal user (no sudo). Usage: ./install.sh [--uninstall]
set -eu

BIN="$HOME/.local/bin/frame-power-notifier"
UNIT_DIR="$HOME/.config/systemd/user"
UNIT="$UNIT_DIR/frame-power-notifier.service"

if [ "${1:-}" = "--uninstall" ]; then
    systemctl --user disable --now frame-power-notifier.service 2>/dev/null || true
    rm -f "$BIN" "$UNIT"
    systemctl --user daemon-reload
    echo "uninstalled"
    exit 0
fi

if [ "$(uname -m)" != "aarch64" ]; then
    echo "this installer is meant to run on the Steam Frame (aarch64)" >&2
    exit 1
fi

cd "$(dirname "$0")"
cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

mkdir -p "$(dirname "$BIN")" "$UNIT_DIR"
install -m 755 build/frame-power-notifier "$BIN"
install -m 644 contrib/frame-power-notifier.service "$UNIT"

systemctl --user daemon-reload
systemctl --user enable frame-power-notifier.service
echo "installed; it will start with SteamVR."
echo "try: $BIN --print   (shows detected power_supply entries)"
