#!/bin/bash
#
# Download and prepare Ubuntu 24.04 ARM64 for PearVisor
#

set -e

IMAGE_URL="https://cloud-images.ubuntu.com/releases/24.04/release/ubuntu-24.04-server-cloudimg-arm64.img"
IMAGE_NAME="ubuntu-24.04-arm64.img"

echo "PearVisor: Downloading Ubuntu 24.04 ARM64..."
echo "URL: $IMAGE_URL"
echo ""

if [ -f "$IMAGE_NAME" ]; then
    echo "Image already exists: $IMAGE_NAME"
    read -p "Re-download? (y/N): " choice
    if [[ ! $choice =~ ^[Yy]$ ]]; then
        echo "Using existing image."
        exit 0
    fi
    rm -f "$IMAGE_NAME"
fi

curl -L -o "$IMAGE_NAME" "$IMAGE_URL"

echo ""
echo "Download complete: $IMAGE_NAME"
echo ""
echo "Image size:"
ls -lh "$IMAGE_NAME"
echo ""
echo "To use this image with PearVisor:"
echo "1. Open PearVisor"
echo "2. Create a new VM"
echo "3. Select 'Use existing disk image'"
echo "4. Choose: $(pwd)/$IMAGE_NAME"
echo ""
echo "Note: This is a cloud image. You'll need to configure cloud-init"
echo "or access via serial console for initial setup."
