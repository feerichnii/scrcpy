#!/usr/bin/env bash
# Local helper: package currently built binaries into Scrcpy GUI.app
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-$ROOT/build-gui}"
OUT_DIR="${2:-$ROOT/dist}"

mkdir -p "$OUT_DIR"
DIST="$OUT_DIR/macos-portable"
rm -rf "$DIST"
mkdir -p "$DIST"

cp "$BUILD_DIR/app/scrcpy-gui" "$DIST/"
if [[ -f "$BUILD_DIR/app/scrcpy" ]]; then
    cp "$BUILD_DIR/app/scrcpy" "$DIST/"
fi
cp "$ROOT/app/data/scrcpy.png" "$DIST/" 2>/dev/null || true
cp "$ROOT/LICENSE" "$DIST/" 2>/dev/null || true

# If scrcpy-server exists from a previous build, include it
if [[ -f "$ROOT/scrcpy-server" ]]; then
    cp "$ROOT/scrcpy-server" "$DIST/"
fi

APP="$OUT_DIR/Scrcpy GUI.app"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cp "$DIST/scrcpy-gui" "$APP/Contents/MacOS/"
[[ -f "$DIST/scrcpy" ]] && cp "$DIST/scrcpy" "$APP/Contents/MacOS/"
[[ -f "$DIST/scrcpy-server" ]] && cp "$DIST/scrcpy-server" "$APP/Contents/MacOS/"
[[ -f "$DIST/scrcpy.png" ]] && cp "$DIST/scrcpy.png" "$APP/Contents/Resources/"
chmod +x "$APP/Contents/MacOS/"*

cat > "$APP/Contents/Info.plist" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleExecutable</key><string>scrcpy-gui</string>
  <key>CFBundleIdentifier</key><string>com.genymobile.scrcpy.gui</string>
  <key>CFBundleName</key><string>Scrcpy GUI</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>4.1</string>
  <key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
EOF

echo "Packaged:"
echo "  $DIST"
echo "  $APP"
