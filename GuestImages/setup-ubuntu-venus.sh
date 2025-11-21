#!/bin/bash
#
# PearVisor Ubuntu Venus Driver Setup
# Automated installation of Mesa Venus driver for GPU acceleration
#
# Usage: sudo bash setup-ubuntu-venus.sh
#

set -e  # Exit on error

# Colors for output
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
    log_error "This script must be run as root (use sudo)"
    exit 1
fi

# Detect Ubuntu version
if [ ! -f /etc/os-release ]; then
    log_error "Cannot detect OS version"
    exit 1
fi

source /etc/os-release
log_info "Detected: $PRETTY_NAME"

if [ "$ID" != "ubuntu" ]; then
    log_error "This script is for Ubuntu only"
    log_info "Use setup-fedora-venus.sh for Fedora"
    exit 1
fi

# Check architecture
ARCH=$(uname -m)
if [ "$ARCH" != "aarch64" ]; then
    log_error "This script requires ARM64 architecture"
    log_error "Detected: $ARCH"
    exit 1
fi

log_info "Architecture: $ARCH (ARM64) ✓"

# Step 1: Check virtio-gpu kernel driver
log_info "Step 1/7: Checking virtio-gpu kernel driver"

if lsmod | grep -q virtio_gpu; then
    log_info "virtio-gpu driver already loaded ✓"
elif modprobe -n virtio_gpu &>/dev/null; then
    log_info "virtio-gpu driver available, loading..."
    modprobe virtio_gpu
    log_info "virtio-gpu driver loaded ✓"
else
    log_error "virtio-gpu driver not available in kernel"
    log_error "Please use a kernel with CONFIG_DRM_VIRTIO_GPU=y"
    exit 1
fi

# Check for DRI devices
if [ -e /dev/dri/card0 ]; then
    log_info "DRI device /dev/dri/card0 found ✓"
else
    log_warn "DRI device not found (VM may not be running with GPU)"
fi

# Step 2: Update system
log_info "Step 2/7: Updating system packages"
apt-get update
apt-get upgrade -y

# Step 3: Install build dependencies
log_info "Step 3/7: Installing build dependencies"

apt-get install -y \
    git \
    meson \
    ninja-build \
    pkg-config \
    python3-mako \
    bison \
    flex \
    libdrm-dev \
    libx11-dev \
    libxext-dev \
    libxfixes-dev \
    libxcb-glx0-dev \
    libxcb-shm0-dev \
    libxcb-dri2-0-dev \
    libxcb-dri3-dev \
    libxcb-present-dev \
    libxcb-sync-dev \
    libxshmfence-dev \
    libxxf86vm-dev \
    libwayland-dev \
    wayland-protocols \
    libwayland-egl-backend-dev \
    llvm-dev \
    libelf-dev \
    libunwind-dev \
    libglvnd-dev \
    glslang-tools \
    vulkan-tools

log_info "Build dependencies installed ✓"

# Step 4: Clone Mesa
log_info "Step 4/7: Cloning Mesa repository"

MESA_DIR="/usr/src/mesa"
if [ -d "$MESA_DIR" ]; then
    log_warn "Mesa directory already exists, removing..."
    rm -rf "$MESA_DIR"
fi

git clone --depth=1 https://gitlab.freedesktop.org/mesa/mesa.git "$MESA_DIR"
cd "$MESA_DIR"

MESA_VERSION=$(git describe --tags 2>/dev/null || echo "main")
log_info "Mesa version: $MESA_VERSION"

# Step 5: Configure Mesa with Venus driver
log_info "Step 5/7: Configuring Mesa build (this may take a moment)"

meson setup build \
    -Dvulkan-drivers=virtio \
    -Dgallium-drivers=virgl,zink \
    -Dplatforms=x11,wayland \
    -Dglx=dri \
    -Degl=enabled \
    -Dgles1=disabled \
    -Dgles2=enabled \
    -Dshared-glapi=enabled \
    -Dbuildtype=release \
    --prefix=/usr/local

log_info "Mesa configured ✓"

# Step 6: Build Mesa
log_info "Step 6/7: Building Mesa (this will take several minutes)"

CPU_CORES=$(nproc)
log_info "Using $CPU_CORES CPU cores for build"

ninja -C build -j"$CPU_CORES"

log_info "Mesa build complete ✓"

# Step 7: Install Mesa
log_info "Step 7/7: Installing Mesa"

ninja -C build install

# Update library cache
ldconfig

log_info "Mesa installed ✓"

# Verify installation
log_info "Verifying Venus driver installation"

if [ -f /usr/local/lib/aarch64-linux-gnu/libvulkan_virtio.so ]; then
    log_info "Venus driver library found ✓"
elif [ -f /usr/local/lib/libvulkan_virtio.so ]; then
    log_info "Venus driver library found ✓"
else
    log_warn "Venus driver library not found in expected location"
    log_warn "Searching for libvulkan_virtio.so..."
    find /usr/local -name "libvulkan_virtio.so" 2>/dev/null || true
fi

# Check for Vulkan ICD
ICD_PATH="/usr/local/share/vulkan/icd.d"
if ls "$ICD_PATH"/virtio_icd*.json &>/dev/null; then
    log_info "Vulkan ICD configuration found ✓"
    ls -la "$ICD_PATH"/virtio_icd*.json
else
    log_warn "Vulkan ICD configuration not found"
fi

# Print summary
echo ""
echo "========================================"
echo "  Mesa Venus Driver Installation"
echo "========================================"
echo ""
log_info "Installation complete!"
echo ""
echo "Next steps:"
echo ""
echo "1. Verify Vulkan installation:"
echo "   vulkaninfo | grep driverName"
echo "   (should show: driverName = venus)"
echo ""
echo "2. List available GPUs:"
echo "   vkcube --enumerate"
echo "   (should show Apple M1 Max GPU)"
echo ""
echo "3. Test GPU acceleration:"
echo "   vkcube"
echo "   (should render spinning cube at >30 FPS)"
echo ""
echo "4. Enable debug output (if needed):"
echo "   export VK_LOADER_DEBUG=all"
echo "   export MESA_DEBUG=1"
echo ""
echo "5. For OpenGL via Zink:"
echo "   glxgears"
echo "   glxinfo | grep 'OpenGL renderer'"
echo ""
echo "========================================"
echo ""

# Test Vulkan (if vulkaninfo is available)
if command -v vulkaninfo &>/dev/null; then
    log_info "Testing Vulkan..."
    echo ""
    vulkaninfo --summary 2>&1 | head -20 || log_warn "Vulkan test failed (VM may need GPU device)"
fi

log_info "Setup script complete!"
log_info "Reboot recommended: sudo reboot"

exit 0
