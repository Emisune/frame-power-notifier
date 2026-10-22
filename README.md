# frame-power-notifier

A small VR overlay for the Steam Frame that pops up a toast when the headset starts or stops charging.

Unofficial and untested on real hardware as of the first commit. The approach follows
[frame-perf-overlay](https://github.com/sasaken1102r/frame-perf-overlay) (MIT): a native aarch64
OpenVR overlay app run as a systemd user service tied to `steamvr.service`.

## How it works

- `src/power.cpp` polls `/sys/class/power_supply` once a second. The charger entry's `online`
  flag is the main signal; the battery's `status` text is the fallback.
- A change must hold for 1.5 s before it alerts. The first reading after start never alerts.
- `src/overlay.cpp` connects to a running SteamVR as an overlay app (it never launches SteamVR),
  draws a battery icon with cairo, uploads it with `SetOverlayRaw`, and attaches it to the HMD
  about 0.6 m ahead. A battery with a bolt means external power is present (3 s); a battery
  with a cross means external power is gone (6 s). It then fades out. There is no charge level:
  the Frame can't see an external battery bank's level.

## Build and install (on the headset)

Requires Developer Mode and SSH. The toolchain (cmake, ninja, g++, cairo dev files) is expected
to be present on SteamOS, as it is for frame-perf-overlay.

### Getting the OpenVR SDK

`openvr.h` is not committed. Put the header from the
[OpenVR SDK](https://github.com/ValveSoftware/openvr) (`headers/openvr.h`) in
`third_party/openvr/`. `libopenvr_api.so` comes with SteamVR; if CMake can't find it, pass
`-DOPENVR_LIB=/path/to/libopenvr_api.so`.

```sh
scp -r . steamos@<headset-ip>:frame-power-notifier
ssh steamos@<headset-ip>
cd frame-power-notifier && ./install.sh
```

Useful commands:

```sh
frame-power-notifier --print   # list detected power_supply entries and current reading
frame-power-notifier --test    # show a test toast as soon as SteamVR is up
./install.sh --uninstall
```

## Open questions

- Verified on one Frame by manual unplug/replug: `pm8550b-charger/online` and the battery
  `status` flip together. Not yet verified with a bank that actually runs flat, which may sag
  without disconnecting; a `Discharging` battery is treated as power lost for that reason.
- `SetOverlayRaw` renders correctly on the Frame, so no Vulkan texture path is needed.
- Run `frame-power-notifier --print` to see the detected sysfs entries if a SteamOS update
  renames them.
