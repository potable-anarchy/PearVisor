#!/bin/bash
#
# PearVisor Mesa Venus Setup Script for Fedora 41
#
# This script automates the installation of Mesa with Venus Vulkan driver
# on Fedora ARM64 guests for use with PearVisor GPU virtualization.
#
# Usage: sudo ./setup-fedora-venus.sh
#

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    log_error "Please run as root (use sudo)"
    exit 1
fi

# Step 1: Verify environment
log_info "Step 1/7: Verifying environment..."

# Check Fedora version
if [ ! -f /etc/fedora-release ]; then
    log_error "This script is for Fedora only"
    exit 1
fi

FEDORA_VERSION=$(rpm -E %fedora)
log_info "Detected Fedora $FEDORA_VERSION"

if [ "$FEDORA_VERSION" -lt 40 ]; then
    log_warn "Fedora $FEDORA_VERSION may have outdated packages. Fedora 41+ recommended."
fi

# Check architecture
ARCH=$(uname -m)
if [ "$ARCH" != "aarch64" ]; then
    log_error "This script is for ARM64 (aarch64) only. Detected: $ARCH"
    exit 1
fi
log_info "Architecture: $ARCH ✓"

# Step 2: Check virtio-gpu kernel driver
log_info "Step 2/7: Checking virtio-gpu kernel driver..."

if lsmod | grep -q virtio_gpu; then
    log_info "virtio-gpu kernel module loaded ✓"
elif modinfo virtio_gpu &>/dev/null; then
    log_warn "virtio-gpu module available but not loaded. Loading..."
    modprobe virtio_gpu
    log_info "virtio-gpu kernel module loaded ✓"
else
    log_error "virtio-gpu kernel module not found!"
    log_error "Please ensure your kernel is compiled with CONFIG_DRM_VIRTIO_GPU=m"
    exit 1
fi

# Check for virtio MMIO support
if [ -d /sys/bus/platform/drivers/virtio-mmio ]; then
    log_info "virtio-mmio support detected ✓"
else
    log_warn "virtio-mmio driver not found. May be needed for full GPU support."
fi

# Step 3: Update system and install build dependencies
log_info "Step 3/7: Installing build dependencies..."

# Update package database
dnf check-update || true

# Install Mesa build dependencies
dnf install -y \
    gcc \
    gcc-c++ \
    meson \
    ninja-build \
    python3-mako \
    libdrm-devel \
    libxcb-devel \
    libX11-devel \
    libxshmfence-devel \
    libXext-devel \
    libXfixes-devel \
    libXrandr-devel \
    wayland-devel \
    wayland-protocols-devel \
    elfutils-libelf-devel \
    llvm-devel \
    clang \
    expat-devel \
    zlib-devel \
    bison \
    flex \
    gettext \
    pkgconf \
    glslang \
    vulkan-headers \
    vulkan-loader-devel

log_info "Build dependencies installed ✓"

# Step 4: Clone Mesa repository
log_info "Step 4/7: Cloning Mesa repository..."

MESA_DIR="/tmp/mesa-venus-build"

if [ -d "$MESA_DIR" ]; then
    log_warn "Mesa directory already exists. Removing..."
    rm -rf "$MESA_DIR"
fi

# Clone Mesa (using shallow clone for speed)
git clone --depth 1 --branch main https://gitlab.freedesktop.org/mesa/mesa.git "$MESA_DIR"

cd "$MESA_DIR"

MESA_VERSION=$(cat VERSION)
log_info "Mesa version: $MESA_VERSION"

# Step 5: Configure Mesa with Venus driver
log_info "Step 5/7: Configuring Mesa build..."

# Create build directory
mkdir -p build

# Configure with Meson
# Key options:
#   -Dvulkan-drivers=virtio: Build Venus Vulkan driver
#   -Dgallium-drivers=virgl,zink: Build virgl for OpenGL and zink for Vulkan-on-Vulkan
#   -Dplatforms=x11,wayland: Support both X11 and Wayland
#   -Dbuildtype=release: Optimized build
meson setup build/ \
    -Dvulkan-drivers=virtio \
    -Dgallium-drivers=virgl,zink \
    -Dplatforms=x11,wayland \
    -Dbuildtype=release \
    -Dprefix=/usr/local

log_info "Mesa configured ✓"

# Step 6: Build Mesa
log_info "Step 6/7: Building Mesa (this may take 10-20 minutes)..."

# Build with all available cores
CORES=$(nproc)
log_info "Building with $CORES parallel jobs..."

meson compile -C build/ -j "$CORES"

log_info "Mesa built successfully ✓"

# Step 7: Install Mesa
log_info "Step 7/7: Installing Mesa..."

meson install -C build/

# Update library cache
ldconfig

log_info "Mesa installed to /usr/local ✓"

# Verification
log_info "Verifying installation..."

# Check for Venus Vulkan driver library
if [ -f /usr/local/lib64/libvulkan_virtio.so ] || [ -f /usr/local/lib/libvulkan_virtio.so ]; then
    log_info "Venus Vulkan driver library found ✓"
else
    log_error "Venus Vulkan driver library NOT found!"
    log_error "Expected: /usr/local/lib64/libvulkan_virtio.so"
    exit 1
fi

# Check for Vulkan ICD file
ICD_PATHS=(
    "/usr/local/share/vulkan/icd.d/virtio_icd.aarch64.json"
    "/usr/local/share/vulkan/icd.d/virtio_icd.json"
)

ICD_FOUND=false
for ICD_PATH in "${ICD_PATHS[@]}"; do
    if [ -f "$ICD_PATH" ]; then
        log_info "Vulkan ICD file found: $ICD_PATH ✓"
        ICD_FOUND=true
        break
    fi
done

if [ "$ICD_FOUND" = false ]; then
    log_warn "Vulkan ICD file not found in expected locations"
    log_warn "You may need to set VK_ICD_FILENAMES environment variable"
fi

# Print success message
echo ""
log_info "═══════════════════════════════════════════════════════════"
log_info "Mesa Venus driver installation complete! ✓"
log_info "═══════════════════════════════════════════════════════════"
echo ""
log_info "Next steps:"
echo "  1. Install Vulkan tools: sudo dnf install vulkan-tools"
echo "  2. Test Vulkan: vulkaninfo | grep Venus"
echo "  3. Test 3D rendering: vkcube (if available)"
echo ""
log_info "Expected Vulkan device name: 'Venus Virtio-GPU Venus'"
log_info "Expected driver: 'Venus Mesa (virtio-gpu driver)'"
echo ""
log_info "Troubleshooting:"
echo "  - If Vulkan not detected, check: ls -la /usr/local/share/vulkan/icd.d/"
echo "  - Set ICD path: export VK_ICD_FILENAMES=/usr/local/share/vulkan/icd.d/virtio_icd.aarch64.json"
echo "  - Check library path: export LD_LIBRARY_PATH=/usr/local/lib64:\$LD_LIBRARY_PATH"
echo "  - Verify driver loaded: lsmod | grep virtio_gpu"
echo ""
log_info "To verify Venus is working with PearVisor:"
echo "  1. vulkaninfo --summary"
echo "  2. Check for 'Venus' in device name"
echo "  3. Run vkcube to test rendering"
echo ""

# Cleanup
log_info "Cleaning up build directory..."
cd /
rm -rf "$MESA_DIR"

log_info "Setup complete! Reboot recommended."
