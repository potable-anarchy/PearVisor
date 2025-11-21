# PearVisor Guest Images

This directory contains VM disk images and installation resources for PearVisor guest operating systems.

## Supported Distributions

### Linux ARM64

**Ubuntu 24.04 LTS (Recommended)**
- **Download:** https://cdimage.ubuntu.com/releases/24.04/release/ubuntu-24.04-live-server-arm64.iso
- **Kernel:** 6.8+ (virtio-gpu support included)
- **Mesa:** Available in repositories, Venus requires building from source
- **Best For:** General purpose, well-tested, stable

**Fedora 41 Server**
- **Download:** https://download.fedoraproject.org/pub/fedora/linux/releases/41/Server/aarch64/iso/
- **Kernel:** 6.11+ (latest virtio-gpu features)
- **Mesa:** More recent versions available
- **Best For:** Cutting-edge features, newer Mesa versions

**Debian 13 (Trixie)**
- **Download:** https://cdimage.debian.org/cdimage/weekly-builds/arm64/iso-cd/
- **Kernel:** 6.7+
- **Mesa:** Stable, well-tested
- **Best For:** Stability, minimal bloat

## Required Kernel Features

All guest kernels must have these features enabled:

```
CONFIG_DRM_VIRTIO_GPU=y          # virtio-gpu kernel driver
CONFIG_VIRTIO_MMIO=y             # MMIO transport
CONFIG_VIRTIO_PCI=y              # PCI transport
CONFIG_DRM=y                     # DRM subsystem
CONFIG_DRM_FBDEV_EMULATION=y     # Framebuffer emulation
CONFIG_FB=y                      # Framebuffer support
```

Most modern ARM64 distributions include these by default.

## Mesa Venus Driver

The Venus Vulkan driver must be built from source with specific options:

### Building Mesa (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt install -y \
    meson ninja-build \
    libdrm-dev libx11-dev libxext-dev libxfixes-dev \
    libxcb-glx0-dev libxcb-shm0-dev libxcb-dri2-0-dev \
    libxcb-dri3-dev libxcb-present-dev \
    libwayland-dev wayland-protocols \
    python3-mako bison flex

# Clone Mesa
git clone https://gitlab.freedesktop.org/mesa/mesa.git
cd mesa

# Configure with Venus driver
meson setup build \
    -Dvulkan-drivers=virtio \
    -Dgallium-drivers=virgl,zink \
    -Dplatforms=x11,wayland \
    -Dglx=dri \
    -Degl=enabled \
    -Dgles1=disabled \
    -Dgles2=enabled \
    -Dshared-glapi=enabled

# Build and install
ninja -C build
sudo ninja -C build install
```

### Building Mesa (Fedora)

```bash
# Install dependencies
sudo dnf install -y \
    meson ninja-build \
    libdrm-devel libX11-devel libxcb-devel \
    wayland-devel wayland-protocols-devel \
    python3-mako bison flex

# Clone and build (same as above)
git clone https://gitlab.freedesktop.org/mesa/mesa.git
cd mesa
meson setup build -Dvulkan-drivers=virtio -Dgallium-drivers=virgl,zink
ninja -C build
sudo ninja -C build install
```

## Verification

After installing Mesa, verify Venus driver is available:

```bash
# Check for Venus Vulkan driver
ls -la /usr/local/lib/aarch64-linux-gnu/libvulkan_virtio.so

# Test Vulkan
vulkaninfo | grep "driverName"
# Should show: driverName = venus

# List available GPUs
vkcube --enumerate
# Should show: Apple M1 Max GPU
```

## Automated Setup Scripts

### Quick Setup (Ubuntu)

```bash
# Download and run automated setup
curl -fsSL https://raw.githubusercontent.com/potable-anarchy/PearVisor/main/GuestImages/setup-ubuntu-venus.sh | sudo bash
```

### Quick Setup (Fedora)

```bash
# Download and run automated setup
curl -fsSL https://raw.githubusercontent.com/potable-anarchy/PearVisor/main/GuestImages/setup-fedora-venus.sh | sudo bash
```

## Disk Image Management

### Creating a New Disk Image

```bash
# Create 64GB disk image
qemu-img create -f qcow2 ubuntu-24.04-venus.qcow2 64G

# Or use raw format (better performance)
dd if=/dev/zero of=ubuntu-24.04-venus.img bs=1G count=64
```

### Resizing Disk Images

```bash
# Resize qcow2 image
qemu-img resize ubuntu-24.04-venus.qcow2 +32G

# Resize raw image
dd if=/dev/zero bs=1G count=32 >> ubuntu-24.04-venus.img
```

## Pre-configured Images

Pre-configured disk images with Venus driver already installed will be available for download:

- **ubuntu-24.04-venus-arm64.qcow2** (~4GB compressed)
  - Ubuntu 24.04 LTS
  - Mesa 24.x with Venus driver
  - Vulkan tools installed
  - Desktop environment: Minimal (no GUI by default)

- **fedora-41-venus-arm64.qcow2** (~5GB compressed)
  - Fedora 41 Server
  - Mesa 24.x with Venus driver
  - Vulkan tools installed

Download links will be added once images are available.

## Troubleshooting

### virtio-gpu driver not loading

```bash
# Check if module exists
modprobe -n virtio_gpu

# Load manually
sudo modprobe virtio_gpu

# Check kernel logs
dmesg | grep virtio
```

### Venus driver not found

```bash
# Verify Mesa installation
ldconfig -p | grep vulkan_virtio

# Check Vulkan ICD
cat /usr/share/vulkan/icd.d/virtio_icd.*.json

# Rebuild Mesa if needed
cd mesa
sudo ninja -C build uninstall
ninja -C build clean
meson setup build --wipe -Dvulkan-drivers=virtio
ninja -C build
sudo ninja -C build install
```

### Vulkan initialization fails

```bash
# Enable debug output
export VK_LOADER_DEBUG=all
export MESA_DEBUG=1
export VK_ICD_FILENAMES=/usr/local/share/vulkan/icd.d/virtio_icd.aarch64.json

# Test basic Vulkan
vulkaninfo
```

## Directory Structure

```
GuestImages/
├── README.md                           # This file
├── setup-ubuntu-venus.sh              # Ubuntu automated setup
├── setup-fedora-venus.sh              # Fedora automated setup
├── Downloads/                          # Downloaded ISOs
│   ├── ubuntu-24.04-live-server-arm64.iso
│   └── Fedora-Server-dvd-aarch64-41.iso
├── DiskImages/                         # VM disk images
│   ├── ubuntu-24.04-venus.qcow2
│   └── fedora-41-venus.qcow2
└── Kernels/                            # Custom kernels (if needed)
    └── config-6.8-virtio-gpu          # Sample kernel config
```

## Contributing

If you create a working guest configuration, please contribute:

1. Test thoroughly (Venus driver works, vkcube runs)
2. Document the setup process
3. Create an automated setup script
4. Submit a pull request

## Support

For guest setup issues:
- Check kernel logs: `dmesg | grep virtio`
- Check Mesa logs: `MESA_DEBUG=1 vulkaninfo`
- Check Vulkan: `VK_LOADER_DEBUG=all vkcube`
- Ask in GitHub Discussions

## License

Guest setup scripts and documentation are MIT licensed, same as PearVisor.
