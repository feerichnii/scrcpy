# scrcpy GUI

Modern multi-device GUI for scrcpy (React + Vite + Tailwind).

It lives in [`gui/`](../gui) and talks to a small local Node server that:

- scans ADB devices
- starts / stops independent `scrcpy` processes per serial
- stores presets locally in `gui-presets.json` (optional Supabase cloud presets)

The classic C++ ImGui binary (`scrcpy-gui` from Meson) remains available but the
**primary UX is this web GUI**.

## Quick start

```bash
# build CLI scrcpy (optional but needed for Run)
meson setup build-gui -Dcompile_server=false -Dportable=true -Dusb=false -Dv4l2=false
ninja -C build-gui app/scrcpy

cd gui
npm install
npm run dev
```

Open **http://localhost:5173** (Vite UI). API runs on **http://localhost:3000**.

Production (single port):

```bash
cd gui
npm start
# open http://localhost:3000
```

## Features

- Scan connected USB / TCP devices
- Per-device enable + common or custom settings
- Full scrcpy option sections (video / audio / recording / control / …)
- Live command preview
- **Run** / **Stop** — launches real `scrcpy` processes
- Presets (local JSON by default)

## Config locations

| Data | Path |
|------|------|
| Local presets | macOS: `~/Library/Application Support/scrcpy/gui-presets.json` |
| | Windows: `%APPDATA%\scrcpy\gui-presets.json` |
| | Linux: `~/.config/scrcpy/gui-presets.json` |

Optional cloud presets: copy `gui/.env.example` → `gui/.env` and set
`VITE_SUPABASE_URL` / `VITE_SUPABASE_ANON_KEY`.

## Environment

| Variable | Meaning |
|----------|---------|
| `ADB` | Path to adb |
| `SCRCPY` | Path to scrcpy binary |
| `SCRCPY_GUI_PORT` | API/static port (default `3000`) |
