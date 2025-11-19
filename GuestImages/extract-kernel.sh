#!/bin/bash
#
# Extract kernel and initrd from Ubuntu cloud image for PearVisor
#

set -e

if [ $# -lt 1 ]; then
    echo "Usage: $0 <ubuntu-image.img> [output-dir]"
    echo ""
    echo "Example:"
    echo "  $0 ubuntu-24.04-arm64.img ./vm-files"
    exit 1
fi

IMAGE="$1"
OUTPUT_DIR="${2:-.}"

if [ ! -f "$IMAGE" ]; then
    echo "Error: Image not found: $IMAGE"
    exit 1
fi

echo "PearVisor: Extracting kernel and initrd from $IMAGE"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Mount image (requires sudo on macOS)
echo "Mounting image..."
MOUNT_POINT=$(mktemp -d)
echo "Mount point: $MOUNT_POINT"

# On macOS, we need to use hdiutil
if [[ "$OSTYPE" == "darwin"* ]]; then
    # Attach disk image
    DEV=$(hdiutil attach -imagekey diskimage-class=CRawDiskImage -nomount "$IMAGE" | head -1 | awk '{print $1}')
    echo "Attached to: $DEV"
    
    # Mount first partition
    sudo mount -t apfs "${DEV}s1" "$MOUNT_POINT" 2>/dev/null || \
    sudo mount -t hfs "${DEV}s1" "$MOUNT_POINT" 2>/dev/null || \
    sudo mount "${DEV}s1" "$MOUNT_POINT"
else
    # Linux
    sudo mount -o loop,offset=1048576 "$IMAGE" "$MOUNT_POINT"
fi

echo "Mounted successfully"
echo ""

# Find kernel and initrd
KERNEL=$(sudo find "$MOUNT_POINT/boot" -name "vmlinuz-*" | head -1)
INITRD=$(sudo find "$MOUNT_POINT/boot" -name "initrd.img-*" | head -1)

if [ -z "$KERNEL" ]; then
    echo "Error: Kernel not found in image"
    sudo umount "$MOUNT_POINT"
    hdiutil detach "$DEV" 2>/dev/null || true
    exit 1
fi

echo "Found kernel: $(basename $KERNEL)"
if [ -n "$INITRD" ]; then
    echo "Found initrd: $(basename $INITRD)"
fi
echo ""

# Copy files
echo "Copying kernel..."
sudo cp "$KERNEL" "$OUTPUT_DIR/vmlinuz"
sudo chown $(whoami) "$OUTPUT_DIR/vmlinuz"

if [ -n "$INITRD" ]; then
    echo "Copying initrd..."
    sudo cp "$INITRD" "$OUTPUT_DIR/initrd"
    sudo chown $(whoami) "$OUTPUT_DIR/initrd"
fi

# Unmount
echo "Unmounting..."
sudo umount "$MOUNT_POINT"
if [[ "$OSTYPE" == "darwin"* ]]; then
    hdiutil detach "$DEV"
fi
rmdir "$MOUNT_POINT"

echo ""
echo "✓ Extraction complete!"
echo ""
echo "Files created:"
echo "  $(ls -lh $OUTPUT_DIR/vmlinuz)"
if [ -f "$OUTPUT_DIR/initrd" ]; then
    echo "  $(ls -lh $OUTPUT_DIR/initrd)"
fi
echo ""
echo "To use with PearVisor:"
echo "1. Place vmlinuz and initrd in your VM directory"
echo "2. Create VM with Ubuntu 24.04 ARM64"
echo "3. PearVisor will automatically find and use these files"
