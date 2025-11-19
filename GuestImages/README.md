# Guest OS Images

This directory contains scripts to download and prepare guest operating system images for PearVisor.

## Supported Operating Systems (Phase 1)

### Linux ARM64
- Ubuntu 24.04 LTS (recommended)
- Fedora 40
- Debian 12
- Arch Linux ARM

## Downloading Images

### Ubuntu 24.04 ARM64

```bash
./ubuntu-24.04-arm64.sh
```

This will download the Ubuntu 24.04 ARM64 cloud image and prepare it for PearVisor.

**Manual download:**
```bash
wget https://cloud-images.ubuntu.com/releases/24.04/release/ubuntu-24.04-server-cloudimg-arm64.img
```

### Fedora 40 ARM64

```bash
curl -O https://download.fedoraproject.org/pub/fedora/linux/releases/40/Cloud/aarch64/images/Fedora-Cloud-Base-Generic-40-1.14.aarch64.qcow2
```

### Debian 12 ARM64

```bash
curl -O https://cloud.debian.org/images/cloud/bookworm/latest/debian-12-generic-arm64.qcow2
```

## Image Requirements

PearVisor expects guest images to have:
- **Architecture:** ARM64/AArch64
- **Format:** Raw (.img) or qcow2
- **Kernel:** Linux 6.x+ with virtio drivers
- **Graphics:** Mesa 24.x+ with Venus driver for GPU acceleration

## Preparing Custom Images

To prepare a custom Linux image for PearVisor:

1. **Install required packages:**
   ```bash
   # On Ubuntu/Debian
   sudo apt install mesa-vulkan-drivers mesa-utils
   
   # On Fedora
   sudo dnf install mesa-vulkan-drivers mesa-demos
   ```

2. **Verify Venus support:**
   ```bash
   # Inside the guest VM
   VK_DRIVER_FILES=/usr/share/vulkan/icd.d/venus_icd.x86_64.json vulkaninfo | grep Venus
   ```

3. **Test GPU acceleration:**
   ```bash
   # Simple OpenGL test
   glxgears
   
   # Vulkan test
   vkcube
   ```

## Image Storage

By default, PearVisor stores VM images at:
```
~/Library/Containers/PearVisor/VMs/
```

Each VM has its own directory:
```
~/Library/Containers/PearVisor/VMs/
├── Ubuntu-VM-1/
│   ├── disk.img
│   ├── config.json
│   └── snapshots/
└── Fedora-VM-1/
    ├── disk.qcow2
    ├── config.json
    └── snapshots/
```

## Image Formats

### Raw Images (.img)
- Simple, no compression
- Fast I/O performance
- Larger file size
- Good for development

### qcow2 Images
- Compressed, copy-on-write
- Smaller file size
- Snapshot support
- Better for storage efficiency

## Converting Images

Convert qcow2 to raw:
```bash
qemu-img convert -f qcow2 -O raw source.qcow2 destination.img
```

Convert raw to qcow2:
```bash
qemu-img convert -f raw -O qcow2 source.img destination.qcow2
```

## Cloud-Init Support

PearVisor supports cloud-init for automated guest configuration:

```yaml
# user-data
#cloud-config
users:
  - name: pearvisor
    ssh_authorized_keys:
      - ssh-rsa AAAA...
    sudo: ALL=(ALL) NOPASSWD:ALL
    shell: /bin/bash

packages:
  - build-essential
  - git
  - mesa-vulkan-drivers
```

## Troubleshooting

### GPU Not Working
1. Verify Mesa Venus driver is installed
2. Check kernel supports virtio-gpu: `lsmod | grep virtio`
3. Verify Vulkan device: `vulkaninfo`

### Slow Performance
1. Use raw disk images instead of qcow2
2. Allocate more CPU cores and memory
3. Check GPU acceleration is enabled

### Boot Failure
1. Verify image is ARM64 architecture
2. Check kernel has virtio drivers
3. Inspect console logs for errors

## Pre-configured Images (Coming Soon)

We plan to provide pre-configured PearVisor images with:
- Optimized kernel and drivers
- Pre-installed Mesa Venus drivers
- Development tools and gaming support
- FEX-Emu for x86_64 emulation (Phase 2)

Stay tuned!
