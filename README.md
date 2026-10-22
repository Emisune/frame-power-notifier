# frame-power-notifier

A VR overlay for the Steam Frame that tells you when external power is lost, so you can swap the battery bank in time.

| External power present | External power gone |
| :---: | :---: |
| ![Battery with a lightning bolt](docs/power-present.svg) | ![Battery with a cross](docs/power-lost.svg) |
| shown for 3 s | shown for 6 s |

Tested on a Steam Frame. The images are renders of the toast design, not headset captures.

## How it works

- Polls `/sys/class/power_supply` once a second. Power is lost when the charger's `online` flag drops or the battery reports `Discharging`. Changes must hold 1.5 s, and the first reading never alerts.
- Shows the icon as an OpenVR overlay attached to the headset, about 0.6 m ahead and slightly below.
- The Frame can't see an external bank's charge level, so the icon only shows whether external power is present.
- Runs as a systemd user service tied to `steamvr.service`. It never starts SteamVR itself.

## Install

On the Frame, with Developer Mode and SSH enabled, copy the repo over and run:

```sh
./install.sh            # builds, installs to ~/.local/bin, enables the service
./install.sh --uninstall
```

Needs `cmake`, `ninja`, `g++` and the cairo dev files (all present on SteamOS). It builds against the OpenVR header and library that SteamVR ships in `/opt/steamvr`.

## Commands

```sh
frame-power-notifier --print   # show detected power_supply entries and the current reading
frame-power-notifier --test    # show both toasts once SteamVR is up
```

If a SteamOS update renames the sysfs entries, `--print` shows what was found.
