# frame-power-notifier

A VR overlay for the Steam Frame that tells you when external power is lost, so you can swap the battery bank in time.

| External power present | External power gone |
| :---: | :---: |
| ![Battery with a lightning bolt](docs/power-present.svg) | ![Battery with a cross](docs/power-lost.svg) |
| shown for 3 s | shown for 6 s |

Tested on a Steam Frame. The images are renders of the toast design, not headset captures.

## Install

On the Frame, with Developer Mode and SSH enabled:

```sh
curl -fsSL https://raw.githubusercontent.com/Emisune/frame-power-notifier/main/install.sh | bash
```

This downloads the latest release, checks its checksum, and installs a systemd user service that starts with SteamVR. To remove it:

```sh
curl -fsSL https://raw.githubusercontent.com/Emisune/frame-power-notifier/main/install.sh | bash -s -- --uninstall
```

## How it works

- Polls `/sys/class/power_supply` once a second. Power is lost when the charger's `online` flag drops or the battery reports `Discharging`.
- Battery-bank connections can be unstable, so the warning only shows after **2 minutes of continuous loss**. Any return of power restarts the window. The "power present" toast shows only after a warning, once power has been steady for 10 s.
- The icon is an OpenVR overlay about 0.6 m ahead of you, slightly below eye level.
- The Frame can't see an external bank's charge level, so the icon only says whether external power is present.
- Every raw power change is logged: `journalctl --user -u frame-power-notifier -f`

## Build from source

On the Frame (needs `cmake`, `ninja`, `g++` and the cairo dev files, all present on SteamOS):

```sh
cmake -G Ninja -S . -B build && cmake --build build
./install.sh --binary build/frame-power-notifier
```

`frame-power-notifier --print` shows the detected power entries; `--test` shows both toasts. Releases are built by GitHub Actions on every `v*` tag.
