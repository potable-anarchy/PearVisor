#!/bin/bash
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
VIRGL_DIR="$PROJECT_ROOT/Submodules/virglrenderer"
BUILD_DIR="$VIRGL_DIR/build-macos"

echo "Building virglrenderer with Venus support for macOS..."
echo "Source directory: $VIRGL_DIR"
echo "Build directory: $BUILD_DIR"

# Clean previous build if it exists
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning previous build..."
    rm -rf "$BUILD_DIR"
fi

# Configure with meson
echo "Configuring with meson..."
meson setup "$BUILD_DIR" "$VIRGL_DIR" \
    --prefix="$BUILD_DIR/install" \
    --buildtype=release \
    -Dvenus=true \
    -Dplatforms=[] \
    -Dcheck-gl-errors=false \
    -Dtests=false \
    -Dfuzzer=false \
    -Dvalgrind=false \
    -Dvideo=false \
    -Drender-server-worker=thread

# Build
echo "Building..."
ninja -C "$BUILD_DIR"

# Install to local prefix
echo "Installing..."
ninja -C "$BUILD_DIR" install

echo "Build complete! virglrenderer installed to $BUILD_DIR/install"
