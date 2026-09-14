# scrcpy GUI (multi-device)

`scrcpy-gui` is a desktop manager for multiple Android devices. It discovers ADB
devices, launches an independent `scrcpy` process per serial, and keeps the
original CLI unchanged.

## Features (Release 0.1 / 0.2)

- Native GUI (SDL3 + Dear ImGui), no Terminal/CMD required for daily use
- Lists all ADB devices (USB / TCP-IP / emulator) with state badges
- Start / Stop one device, selected devices, or all devices
- Isolated sessions: one failure does not stop the others
- Staggered multi-device startup
- Global + per-device settings with presets
- Optional auto-start when a device becomes authorized
- Auto window grid layout via `--window-x/y/width/height`
- Per-session logs inside the GUI

## Build

Requirements: Meson, Ninja, pkg-config, SDL3, FFmpeg (libavutil at least for
GUI; full FFmpeg for the CLI `scrcpy` binary).

```bash
meson setup build-gui -Dcompile_server=false -Dportable=true -Dusb=false -Dv4l2=false
ninja -C build-gui app/scrcpy app/scrcpy-gui
```

Disable the GUI target with `-Dcompile_gui=false` if needed.

### Local macOS app bundle

```bash
./release/package_local_gui.sh build-gui dist
open "dist/Scrcpy GUI.app"
```

Release packaging:

- macOS: `release/build_macos.sh <arch>` then `release/package_macos_gui.sh <arch>`
- Windows: `release/build_windows.sh 64` copies `scrcpy-gui.exe` next to `scrcpy.exe`
  (GUI subsystem, no console window)

## Run

```bash
# from a portable build directory containing both binaries
./scrcpy-gui
```

`scrcpy-gui` looks for a `scrcpy` binary beside itself (portable) or on `PATH`.

The classic CLI still works:

```bash
./scrcpy -s <serial>
```

## Configuration

Settings are stored in `scrcpy-gui.json`:

| Platform | Path |
|----------|------|
| macOS | `~/Library/Application Support/scrcpy/scrcpy-gui.json` |
| Windows | `%APPDATA%\scrcpy\scrcpy-gui.json` |
| Linux | `~/.config/scrcpy/scrcpy-gui.json` |

Session logs are written under a `logs/` subdirectory of that folder.

Example:

```json
{
  "global": {
    "max_fps": 60,
    "max_size": 0,
    "audio": true,
    "stagger_ms": 200,
    "auto_layout": true
  },
  "devices": {
    "8C69ABCDEF": {
      "name": "HONOR test",
      "preset": "high_quality",
      "auto_start": true
    }
  }
}
```

## Architecture

```text
scrcpy-gui
  ├─ DeviceManager      (adb devices -l polling)
  ├─ SessionManager     (state machine per serial)
  ├─ ProcessManager     (spawns scrcpy -s SERIAL ...)
  ├─ SettingsManager
  └─ LayoutManager
```

Each device runs as a separate `scrcpy` process. This keeps upstream
compatibility and isolates crashes until a future embedded multi-view lands.
