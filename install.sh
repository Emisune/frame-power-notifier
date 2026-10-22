#!/bin/bash
# Installs frame-power-notifier on a Steam Frame as a systemd user service.
#
#   curl -fsSL https://raw.githubusercontent.com/Emisune/frame-power-notifier/main/install.sh | bash
#   curl -fsSL .../install.sh | bash -s -- --uninstall
#
# For a locally built binary: ./install.sh --binary build/frame-power-notifier
set -euo pipefail

REPO="Emisune/frame-power-notifier"
ASSET="frame-power-notifier-aarch64.tar.gz"
SERVICE="frame-power-notifier.service"
BIN="$HOME/.local/bin/frame-power-notifier"
UNIT_DIR="$HOME/.config/systemd/user"
UNIT="$UNIT_DIR/$SERVICE"
WANTS="$UNIT_DIR/steamvr.service.wants/$SERVICE"

# PIDs running the installed binary (matched by exe path; the process name is
# truncated to 15 characters, so pgrep -x can't be used).
pids() {
    local p
    for p in /proc/[0-9]*; do
        if [ "$(readlink "$p/exe" 2>/dev/null)" = "$BIN" ]; then echo "${p#/proc/}"; fi
    done
}

stop_service() {
    systemctl --user disable --now "$SERVICE" 2>/dev/null || true
    # Anything systemd doesn't own (e.g. a manual --test run), or a stop that failed.
    local left
    left=$(pids)
    [ -z "$left" ] && return 0
    kill $left 2>/dev/null || true
    for _ in $(seq 1 30); do
        [ -z "$(pids)" ] && return 0
        sleep 0.1
    done
    kill -9 $(pids) 2>/dev/null || true
    sleep 0.3
    if [ -n "$(pids)" ]; then
        echo "could not stop frame-power-notifier (pid $(pids | tr '\n' ' '))" >&2
        exit 1
    fi
}

uninstall() {
    stop_service
    rm -f "$BIN" "$UNIT" "$WANTS"
    systemctl --user daemon-reload 2>/dev/null || true
    systemctl --user reset-failed "$SERVICE" 2>/dev/null || true
    echo "uninstalled"
}

write_unit() {
    mkdir -p "$UNIT_DIR"
    cat > "$UNIT" <<'EOF'
[Unit]
Description=Frame power-state VR overlay notifier
After=steamvr.service
PartOf=steamvr.service

[Service]
ExecStart=%h/.local/bin/frame-power-notifier
Restart=always
RestartSec=5
TimeoutStopSec=5

[Install]
WantedBy=steamvr.service
EOF
}

install_from() {
    local binary="$1"
    stop_service
    mkdir -p "$(dirname "$BIN")"
    install -m 755 "$binary" "$BIN"
    write_unit
    systemctl --user daemon-reload
    systemctl --user enable --now "$SERVICE"
    echo "installed and running; it restarts with SteamVR"
}

download() {
    local tmp
    tmp=$(mktemp -d)
    trap 'rm -rf "$tmp"' EXIT
    local base="https://github.com/$REPO/releases/latest/download"
    curl -fsSL "$base/$ASSET" -o "$tmp/$ASSET"
    curl -fsSL "$base/SHA256SUMS" -o "$tmp/SHA256SUMS"
    (cd "$tmp" && grep " $ASSET\$" SHA256SUMS | sha256sum -c --quiet -)
    tar -xzf "$tmp/$ASSET" -C "$tmp"
    install_from "$tmp/frame-power-notifier"
}

main() {
    local mode="download" binary=""
    while [ $# -gt 0 ]; do
        case "$1" in
            --uninstall) mode="uninstall" ;;
            --binary) mode="binary"; binary="${2:?--binary needs a path}"; shift ;;
            *) echo "usage: install.sh [--uninstall | --binary PATH]" >&2; exit 2 ;;
        esac
        shift
    done

    if [ "$(uname -m)" != "aarch64" ]; then
        echo "this is meant to run on the Steam Frame (aarch64)" >&2
        exit 1
    fi

    case "$mode" in
        uninstall) uninstall ;;
        binary) install_from "$binary" ;;
        download) download ;;
    esac
}

# Wrapped in a function and called last so a truncated download can't run half a script.
main "$@"
