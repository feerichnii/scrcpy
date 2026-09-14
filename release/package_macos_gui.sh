#!/bin/bash
# Build a Scrcpy GUI.app bundle from a macOS dist directory.
set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")"
. build_common
cd ..

if [[ $# -lt 1 ]]; then
    echo "Syntax: $0 <arch> [dist_dir]" >&2
    echo "  arch: arm64 or x86_64" >&2
    echo "  dist_dir: optional path containing scrcpy and scrcpy-gui binaries" >&2
    exit 1
fi

ARCH="$1"
MACOS_BUILD_DIR="$WORK_DIR/build-macos-$ARCH"
DIST_DIR="${2:-$MACOS_BUILD_DIR/dist}"

if [[ ! -x "$DIST_DIR/scrcpy-gui" ]]; then
    echo "Missing scrcpy-gui in $DIST_DIR" >&2
    exit 1
fi
if [[ ! -x "$DIST_DIR/scrcpy" ]]; then
    echo "Missing scrcpy in $DIST_DIR" >&2
    exit 1
fi

APP_NAME="Scrcpy GUI.app"
APP_ROOT="$OUTPUT_DIR/$APP_NAME"
CONTENTS="$APP_ROOT/Contents"
MACOS_DIR="$CONTENTS/MacOS"
RES_DIR="$CONTENTS/Resources"

rm -rf "$APP_ROOT"
mkdir -p "$MACOS_DIR" "$RES_DIR"

cp "$DIST_DIR/scrcpy-gui" "$MACOS_DIR/scrcpy-gui"
cp "$DIST_DIR/scrcpy" "$MACOS_DIR/scrcpy"
chmod +x "$MACOS_DIR/scrcpy-gui" "$MACOS_DIR/scrcpy"

# Portable helpers expected next to the GUI binary
if [[ -f "$DIST_DIR/adb" ]]; then
    cp "$DIST_DIR/adb" "$MACOS_DIR/adb"
    chmod +x "$MACOS_DIR/adb"
fi
if [[ -f "$DIST_DIR/scrcpy-server" ]]; then
    cp "$DIST_DIR/scrcpy-server" "$MACOS_DIR/scrcpy-server"
elif [[ -f "$WORK_DIR/build-server/server/scrcpy-server" ]]; then
    cp "$WORK_DIR/build-server/server/scrcpy-server" "$MACOS_DIR/scrcpy-server"
fi

if [[ -f "$DIST_DIR/scrcpy.png" ]]; then
    cp "$DIST_DIR/scrcpy.png" "$RES_DIR/scrcpy.png"
elif [[ -f app/data/scrcpy.png ]]; then
    cp app/data/scrcpy.png "$RES_DIR/scrcpy.png"
fi

cat > "$CONTENTS/Info.plist" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>scrcpy-gui</string>
    <key>CFBundleIdentifier</key>
    <string>com.genymobile.scrcpy.gui</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>Scrcpy GUI</string>
    <key>CFBundleDisplayName</key>
    <string>Scrcpy GUI</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>4.1</string>
    <key>CFBundleVersion</key>
    <string>4.1</string>
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF

echo "Created $APP_ROOT"
