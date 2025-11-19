# PearVisor Design Document

**Version:** 0.1.0  
**Last Updated:** 2025-11-19  
**Status:** Pre-Development Planning

---

## Executive Summary

PearVisor is an open-source hypervisor for macOS (and eventually other platforms) that provides near-native GPU performance for Linux guest operating systems on Apple Silicon. The primary goal is to achieve 95% native Metal performance through intelligent GPU API translation and passthrough, enabling gaming, AI workloads, and containerized applications to run at speeds competitive with commercial solutions like Parallels Desktop and VMware Fusion.

**Target Users:**
- Gamers wanting to run Linux/Windows games on Apple Silicon
- AI/ML developers needing GPU-accelerated containers and VMs
- Developers requiring high-performance Linux environments
- Anyone needing full-featured virtualization with native-class GPU performance

---

## Project Goals

### Performance Targets
- **Phase 1 (MVP):** 80% of native Metal performance
- **Phase 2 (Stable):** 95% of native Metal performance
- **Phase 3 (Advanced):** Explore direct Metal API passthrough for >95% performance

### Feature Scope

**Phase 1: Linux ARM64 VMs (6-9 months)**
- Linux ARM64 guest OS support (Ubuntu, Fedora, Debian)
- GPU-accelerated graphics via Venus + MoltenVK
- GPU-accelerated containers (libkrun-style)
- Full VM features (networking, storage, file sharing)
- macOS GUI application
- Basic gaming support (native ARM64 Linux games)

**Phase 2: x86_64 Emulation (12-18 months)**
- FEX-Emu integration for x86_64 Linux binaries
- Windows guest OS support (ARM64 Windows)
- Enhanced gaming support (x86_64 Linux games via FEX)
- Performance optimizations (85-90% target)

**Phase 3: Windows x86_64 Gaming (18-24 months)**
- Windows x86_64 guest support via FEX
- DXVK/VKD3D-Proton support (DirectX → Vulkan → Metal)
- Wine/Proton integration for Steam gaming
- Advanced GPU features (ray tracing, compute shaders)
- Performance target: 95%+

---

## Technical Architecture

### Architecture Decision: Hybrid Approach

After evaluating three main approaches:

1. **libkrun Fork** (Proven 75-80% performance, faster MVP)
2. **Apple Virtualization.framework** (Best long-term performance potential)
3. **QEMU Fork** (Maximum flexibility, most complex)

**Chosen Approach: Hybrid - Apple Virtualization.framework + libkrun GPU Stack**

**Rationale:**
- Use Apple's Virtualization.framework for VM management (stability, native integration)
- Adopt libkrun's proven Venus + virglrenderer + MoltenVK GPU stack (faster time-to-market)
- Build custom Metal passthrough layer in parallel for Phase 2/3 optimization
- Leverage existing open-source components where possible

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    PearVisor GUI (SwiftUI)                  │
│  - VM Management  - Settings  - Performance Monitoring      │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────┐
│              PearVisor Core (Swift + C/C++)                 │
│  - VM Lifecycle   - Resource Management   - API Layer       │
└────────────────────────────┬────────────────────────────────┘
                             │
        ┌────────────────────┴────────────────────┐
        │                                         │
┌───────▼────────┐                    ┌──────────▼──────────┐
│ Virtualization │                    │   GPU Passthrough   │
│   Framework    │                    │      Subsystem      │
│                │                    │                     │
│ - VZVirtual    │                    │ - virtio-gpu device │
│   Machine      │                    │ - virglrenderer     │
│ - VZVirtio     │◄──────────────────►│ - Venus protocol    │
│   Devices      │   Shared Memory    │ - MoltenVK          │
│ - Rosetta 2    │                    │ - Metal backend     │
└────────┬───────┘                    └──────────┬──────────┘
         │                                       │
         │                                       │
    ┌────▼───────────────────────────────────────▼────┐
    │            macOS Host (Apple Silicon)           │
    │  - Hypervisor.framework  - Metal API  - IOKit   │
    └────────────────────────┬────────────────────────┘
                             │
                             │
    ┌────────────────────────▼────────────────────────┐
    │              Linux ARM64 Guest VM               │
    │                                                  │
    │  ┌─────────────────┐      ┌──────────────────┐ │
    │  │ User Applications│      │   Mesa Drivers   │ │
    │  │ - Games         │      │  - Venus (Vulkan)│ │
    │  │ - Docker        │◄────►│  - Zink (OpenGL) │ │
    │  │ - AI/ML Tools   │      │  - virtio-gpu    │ │
    │  └─────────────────┘      └──────────────────┘ │
    │                                                  │
    │  ┌──────────────────────────────────────────┐  │
    │  │        Linux Kernel (vanilla ARM64)      │  │
    │  │  - virtio-gpu drivers  - KVM  - cgroups  │  │
    │  └──────────────────────────────────────────┘  │
    └──────────────────────────────────────────────────┘
```

---

## GPU Acceleration Stack

### Phase 1: Venus + MoltenVK (Proven Path)

**Guest Side (Linux VM):**
1. Application makes Vulkan/OpenGL calls
2. Mesa's Venus driver (Vulkan) or Zink (OpenGL→Vulkan) intercepts calls
3. Venus serializes Vulkan commands over virtio-gpu protocol
4. Commands sent to host via shared memory

**Host Side (macOS):**
1. virtio-gpu device receives serialized commands
2. virglrenderer deserializes and forwards to Vulkan API
3. MoltenVK translates Vulkan calls to Metal API
4. Metal executes on native Apple GPU

**Expected Performance:** 75-80% of native Metal (proven by Red Hat/libkrun)

### Phase 2: Custom Metal Passthrough (Performance Goal)

**Optimizations to reach 85-90%:**
- Replace virglrenderer with custom Metal-aware renderer
- Reduce serialization overhead (binary protocol vs. text-based)
- Direct shared memory GPU buffer mapping
- Metal command buffer optimization
- Bypass unnecessary Vulkan validation layers

### Phase 3: Direct Metal API (95%+ Goal)

**Theoretical Approaches:**
- Custom Mesa driver that speaks Metal API directly (no Vulkan layer)
- Paravirtualized Metal device (similar to VZVirtioGraphicsDeviceConfiguration)
- Metal-native guest drivers (requires kernel modifications)

**Challenges:**
- Metal API is not open/documented like Vulkan
- Would require reverse engineering or Apple cooperation
- Guest kernel would need custom drivers (breaks vanilla Linux goal)

**Alternative:** Focus on optimizing the Vulkan→Metal path to near-perfect efficiency

---

## Component Breakdown

### 1. PearVisor Core (Swift/Objective-C)

**Responsibilities:**
- VM lifecycle management (create, start, stop, pause, snapshot)
- Resource allocation (CPU, memory, disk, network)
- Device emulation coordination
- Configuration management
- Integration with macOS Virtualization.framework

**Key Classes:**
- `PVVirtualMachine`: VM lifecycle and state management
- `PVConfiguration`: VM settings and resource allocation
- `PVGPUController`: GPU passthrough coordination
- `PVNetworkController`: Networking (NAT, bridged, host-only)
- `PVStorageController`: Disk image management (qcow2, raw, etc.)

**Technologies:**
- Swift 5.9+ for modern macOS development
- Apple's Virtualization.framework (macOS 15+)
- Combine for reactive state management

### 2. GPU Passthrough Subsystem (C/C++)

**Responsibilities:**
- virtio-gpu device emulation
- Venus protocol handling
- virglrenderer integration
- MoltenVK bridge
- GPU memory management
- Performance monitoring and optimization

**Key Components:**
- `pv_virtio_gpu`: Custom virtio-gpu device implementation
- `pv_renderer`: virglrenderer wrapper with macOS optimizations
- `pv_metal_bridge`: MoltenVK integration and Metal optimization
- `pv_gpu_memory`: Shared memory and buffer management

**Technologies:**
- C/C++ for performance-critical GPU code
- Mesa's virglrenderer (forked with macOS patches)
- MoltenVK 1.3+ (Vulkan 1.3 → Metal)
- Metal API for direct GPU access

**External Dependencies:**
- **virglrenderer** (forked): https://gitlab.freedesktop.org/virgl/virglrenderer
- **MoltenVK** (submodule): https://github.com/KhronosGroup/MoltenVK
- **Mesa** (Venus driver reference): https://gitlab.freedesktop.org/mesa/mesa

### 3. FEX-Emu Integration (Phase 2)

**Responsibilities:**
- x86_64 binary translation for Linux guests
- Syscall forwarding and emulation
- Library call thunking (OpenGL/Vulkan passthrough)
- Performance optimization (JIT, caching)

**Integration Approach:**
- Bundle FEX-Emu binaries within Linux guest images
- Pre-configure FEX for optimal ARM64 translation
- Enable GPU thunking (forward Vulkan/OpenGL calls to host)
- Provide GUI controls for FEX settings

**Technologies:**
- FEX-Emu 2511+ (latest release)
- Custom rootfs images with FEX pre-installed
- Integration with Steam/Proton for gaming

### 4. Container Support (libkrun-inspired)

**Responsibilities:**
- Lightweight container runtime with GPU access
- Docker-compatible API
- Minimal overhead compared to full VMs
- Shared kernel for efficiency

**Architecture:**
- Leverage macOS Virtualization.framework for lightweight VMs
- Use libkrun's Venus + MoltenVK stack (proven container GPU access)
- Provide Docker-compatible CLI/API
- Support standard OCI images

**Use Cases:**
- AI/ML inference with GPU acceleration (llama.cpp, etc.)
- Development environments
- CI/CD pipelines
- Microservices testing

### 5. GUI Application (SwiftUI)

**Features:**
- VM creation wizard (ISO/cloud-image import)
- VM library and management
- Live VM preview (VNC/Spice)
- Performance monitoring (CPU, GPU, memory, network)
- Settings and configuration
- Snapshot management
- File sharing configuration

**Design Principles:**
- Native macOS look and feel
- Responsive and performant (async/await)
- Accessible (VoiceOver, keyboard navigation)
- Dark mode support

**Technologies:**
- SwiftUI for modern macOS UI
- Combine for reactive data flow
- Metal for VM display rendering
- Core Animation for smooth transitions

---

## Networking & Storage

### Networking

**Modes:**
- **NAT**: Default, simple internet access
- **Bridged**: Direct network access (requires admin)
- **Host-Only**: Isolated network for testing
- **Shared**: macOS VZFileHandleNetworkDeviceAttachment

**Implementation:**
- Use VZNATNetworkDeviceAttachment for NAT
- Use VZBridgedNetworkDeviceAttachment for bridged
- Custom port forwarding for NAT mode
- mDNS/Bonjour for service discovery

### Storage

**Disk Formats:**
- Raw disk images (.img)
- qcow2 (with compression)
- Apple Disk Image (.dmg) for macOS guests
- NVMe passthrough for performance

**File Sharing:**
- VZVirtioFileSystemDeviceConfiguration (9p/virtfs)
- Samba/CIFS for Windows compatibility
- NFS for Linux compatibility
- Drag-and-drop file transfer (GUI)

**Snapshots:**
- Full VM snapshots (memory + disk state)
- Disk-only snapshots (faster, less space)
- Snapshot management UI
- Automatic snapshot scheduling

---

## Performance Optimization Strategy

### Measurement & Benchmarking

**Benchmarks:**
- **Graphics:** glxgears, Unigine Heaven, 3DMark (via Wine)
- **Compute:** llama.cpp, Blender, TensorFlow
- **Gaming:** Native ARM64 games, x86_64 via FEX
- **Real-world:** Docker builds, web development, video encoding

**Metrics:**
- Frame rate (FPS) vs. native Metal
- GPU utilization (% of max throughput)
- Latency (command submission to execution)
- Memory bandwidth (GB/s)
- Power consumption (efficiency)

### Optimization Priorities

**Phase 1 (80% target):**
1. Minimize Venus protocol serialization overhead
2. Optimize virglrenderer for macOS (remove Linux-specific code)
3. Enable MoltenVK fast-math and aggressive optimizations
4. Use shared memory for zero-copy GPU buffer access
5. Profile and eliminate CPU-side bottlenecks

**Phase 2 (85-90% target):**
1. Custom binary Venus protocol (reduce packet size)
2. GPU command batching and coalescing
3. Metal-specific optimizations (tile memory, render pipelines)
4. Async command submission (reduce stalls)
5. FEX JIT optimizations for graphics-heavy code

**Phase 3 (95% target):**
1. Direct Metal API exposure (experimental)
2. Paravirtualized Metal device driver
3. Kernel-bypass techniques (user-space GPU access)
4. Custom Mesa driver for Metal backend
5. Collaborate with Apple on official support

---

## Development Roadmap

### Phase 1: MVP (Months 1-9)

**Month 1-2: Foundation**
- [ ] Project setup (Xcode, build system, CI/CD)
- [ ] Basic Virtualization.framework integration
- [ ] Simple Linux ARM64 VM creation (Ubuntu)
- [ ] Console-only guest access (no GUI yet)

**Month 3-4: GPU Passthrough**
- [ ] virtio-gpu device implementation
- [ ] virglrenderer integration (fork and adapt for macOS)
- [ ] MoltenVK integration
- [ ] Venus protocol validation
- [ ] First GPU-accelerated graphics (glxgears)

**Month 5-6: Stability & Features**
- [ ] Networking (NAT, bridged, host-only)
- [ ] Storage (disk images, file sharing)
- [ ] Performance benchmarking (baseline)
- [ ] Memory optimization (reduce overhead)

**Month 7-9: GUI & Polish**
- [ ] SwiftUI GUI application
- [ ] VM creation wizard
- [ ] Live VM display (Spice/VNC)
- [ ] Settings and configuration UI
- [ ] Documentation and tutorials
- [ ] Alpha release (public testing)

**Deliverable:** PearVisor 0.1.0 - Linux ARM64 VMs with 80% GPU performance

### Phase 2: x86_64 & Optimization (Months 10-18)

**Month 10-12: FEX Integration**
- [ ] Bundle FEX-Emu in guest images
- [ ] FEX configuration and optimization
- [ ] x86_64 Linux binary support
- [ ] Gaming testing (Steam, Lutris)

**Month 13-15: Performance Optimization**
- [ ] Custom Metal renderer (bypass virglrenderer)
- [ ] Venus protocol optimization
- [ ] Async GPU command submission
- [ ] Profiling and bottleneck elimination
- [ ] 85-90% performance target

**Month 16-18: Container Support**
- [ ] libkrun-style container runtime
- [ ] Docker-compatible API
- [ ] GPU-accelerated containers
- [ ] AI/ML use case validation
- [ ] Beta release (feature-complete)

**Deliverable:** PearVisor 1.0.0 - x86_64 support, 85-90% GPU performance, containers

### Phase 3: Advanced Features (Months 19-24)

**Month 19-20: Windows Support**
- [ ] ARM64 Windows guest support
- [ ] Windows drivers (virtio-gpu, network, storage)
- [ ] Windows gaming validation

**Month 21-22: Advanced Graphics**
- [ ] DXVK/VKD3D-Proton integration
- [ ] DirectX → Vulkan → Metal path
- [ ] Ray tracing support
- [ ] DLSS/FSR upscaling (if applicable)

**Month 23-24: Direct Metal Exploration**
- [ ] Research direct Metal API passthrough
- [ ] Custom Mesa driver (Metal backend)
- [ ] Paravirtualized device (if feasible)
- [ ] 95%+ performance target
- [ ] Stable release (production-ready)

**Deliverable:** PearVisor 2.0.0 - Windows x86_64 gaming, 95% GPU performance

---

## Technology Stack

### Core Technologies
- **Language:** Swift 5.9+ (GUI, core), C/C++ (GPU subsystem)
- **Frameworks:** Virtualization.framework, Hypervisor.framework, Metal, IOKit
- **UI:** SwiftUI, Combine, Core Animation
- **Build:** Xcode 15+, SPM (Swift Package Manager), CMake (C++ components)

### GPU Stack
- **virtio-gpu:** Custom implementation (C)
- **virglrenderer:** Forked from freedesktop.org (C)
- **Venus:** Mesa driver (bundled in guest)
- **MoltenVK:** Submodule from KhronosGroup (C++)
- **Metal:** Apple's native GPU API

### Guest OS Support
- **Linux:** Ubuntu 24.04+, Fedora 40+, Debian 12+, Arch Linux
- **Kernel:** Vanilla ARM64 kernel (6.x+)
- **Graphics:** Mesa 24.x+ with Venus driver
- **Emulation:** FEX-Emu 2511+ (Phase 2)

### External Dependencies
- **MoltenVK** (Apache 2.0): https://github.com/KhronosGroup/MoltenVK
- **virglrenderer** (MIT): https://gitlab.freedesktop.org/virgl/virglrenderer
- **FEX-Emu** (MIT): https://github.com/FEX-Emu/FEX
- **QEMU** (GPL, reference only): https://www.qemu.org/

---

## Competitive Analysis

### Existing Solutions

| Feature | UTM | Parallels | VMware Fusion | PearVisor (Target) |
|---------|-----|-----------|---------------|---------------------|
| **License** | GPL | Proprietary | Proprietary | MIT |
| **GPU Performance** | 60-70% | 85-90% | 80-85% | 80-95% |
| **Linux ARM64** | ✅ | ✅ | ✅ | ✅ |
| **x86_64 Emulation** | ✅ (Rosetta/TCG) | ✅ (FEX) | ✅ (Rosetta) | ✅ (FEX) |
| **Windows Support** | ✅ | ✅ | ✅ | 🔄 (Phase 2) |
| **Container Support** | ❌ | ❌ | ❌ | ✅ (Phase 2) |
| **Open Source** | ✅ | ❌ | ❌ | ✅ |
| **Cost** | Free | $99/year | $199/year | Free |
| **Gaming Focus** | ❌ | ❌ | ❌ | ✅ |

### Differentiation

**PearVisor's Unique Value:**
1. **Open Source (MIT):** Fully transparent, community-driven, no vendor lock-in
2. **Performance-First:** Designed from the ground up for maximum GPU throughput
3. **Gaming Optimized:** Native support for FEX, DXVK, Proton, Steam
4. **Container Support:** GPU-accelerated containers for AI/ML workloads
5. **Developer-Friendly:** Comprehensive APIs, documentation, extensibility
6. **Native macOS Integration:** Feels like a native Mac app, not a cross-platform port

---

## Project Management

### Repository Structure

```
pearvisor/
├── README.md                 # Project overview, getting started
├── DESIGN.md                 # This document
├── LICENSE                   # MIT License
├── CONTRIBUTING.md           # Contribution guidelines
├── CODE_OF_CONDUCT.md        # Community standards
│
├── Sources/                  # Swift source code
│   ├── PearVisor/            # Main application
│   │   ├── App.swift
│   │   ├── ViewModels/
│   │   ├── Views/
│   │   └── Utils/
│   ├── PearVisorCore/        # Core VM management
│   │   ├── VirtualMachine.swift
│   │   ├── Configuration.swift
│   │   ├── GPUController.swift
│   │   └── NetworkController.swift
│   └── PearVisorGPU/         # C/C++ GPU subsystem wrapper
│       └── GPUBridge.swift
│
├── GPU/                      # C/C++ GPU subsystem
│   ├── include/
│   │   ├── pv_virtio_gpu.h
│   │   ├── pv_renderer.h
│   │   └── pv_metal_bridge.h
│   ├── src/
│   │   ├── virtio_gpu.c
│   │   ├── renderer.cpp
│   │   └── metal_bridge.mm
│   └── CMakeLists.txt
│
├── Submodules/               # External dependencies
│   ├── MoltenVK/
│   └── virglrenderer/
│
├── GuestImages/              # Pre-configured guest OS images
│   ├── ubuntu-24.04-arm64.sh # Download script
│   └── README.md
│
├── Benchmarks/               # Performance testing
│   ├── graphics_bench.sh
│   ├── compute_bench.sh
│   └── gaming_bench.sh
│
├── Documentation/            # Extended docs
│   ├── Architecture.md
│   ├── GPUPassthrough.md
│   ├── FEXIntegration.md
│   └── Benchmarking.md
│
└── Tests/                    # Unit and integration tests
    ├── PearVisorTests/
    ├── PearVisorCoreTests/
    └── GPUTests/
```

### Development Tools

**Required:**
- Xcode 15+ (macOS 15+)
- CMake 3.25+
- Swift 5.9+
- Clang/LLVM (bundled with Xcode)

**Recommended:**
- SwiftFormat (code formatting)
- SwiftLint (linting)
- Instruments (profiling)
- RenderDoc (GPU debugging, via Vulkan)

### CI/CD Pipeline

**GitHub Actions:**
- Build and test on every PR
- Run benchmarks on main branch
- Generate release binaries (dmg, pkg)
- Update documentation automatically

**Testing Strategy:**
- Unit tests for Swift code (XCTest)
- Integration tests for VM lifecycle
- GPU passthrough validation (automated guest tests)
- Performance regression testing (baseline tracking)

---

## Open Questions & Risks

### Technical Risks

1. **Metal API Limitations**
   - Risk: Metal may not expose all features needed for Vulkan translation
   - Mitigation: Work closely with MoltenVK team, contribute improvements

2. **Apple Virtualization.framework Restrictions**
   - Risk: Framework may limit low-level GPU access
   - Mitigation: Hybrid approach (Virtualization.framework + custom virtio-gpu)

3. **FEX-Emu Performance on Apple Silicon**
   - Risk: AVX/AVX2 emulation may be slow without SVE2
   - Mitigation: Focus on ARM64-native workloads first, optimize FEX later

4. **Venus Protocol Overhead**
   - Risk: Serialization/deserialization may limit performance
   - Mitigation: Profile early, custom binary protocol if needed

### Open Questions

1. **Should we support macOS guests with GPU acceleration?**
   - Pros: Useful for testing, macOS development
   - Cons: Complex, Apple may restrict, lower priority
   - Decision: Defer to Phase 3+

2. **Should we build a custom Mesa driver for Metal?**
   - Pros: Best performance (no Vulkan layer)
   - Cons: Huge effort, breaks vanilla Linux goal
   - Decision: Explore in Phase 3 if performance target not met

3. **Should we support older macOS versions (Ventura, Sonoma)?**
   - Pros: Wider compatibility
   - Cons: More testing, older APIs
   - Decision: macOS 15+ only for simplicity (can revisit)

4. **Should we integrate with existing tools (Docker, Lima, Podman)?**
   - Pros: Ecosystem integration, adoption
   - Cons: More complexity, API compatibility
   - Decision: Phase 2+ (after MVP)

---

## Community & Contribution

### Communication Channels
- **GitHub Discussions:** Feature requests, Q&A, design discussions
- **Discord Server:** Real-time chat, support, development coordination
- **Issue Tracker:** Bug reports, feature requests, project planning
- **Blog/Newsletter:** Release announcements, development updates

### Contribution Guidelines
- Code of Conduct (Contributor Covenant)
- Pull request process (review, testing, documentation)
- Coding standards (Swift style guide, C++ conventions)
- Commit message format (Conventional Commits)

### License: MIT

**Rationale:**
- Permissive (allows commercial use, modifications)
- Compatible with most dependencies (MoltenVK, virglrenderer, FEX)
- Encourages adoption and contribution
- Simple and well-understood

**Note:** Some dependencies (MoltenVK) use Apache 2.0, virglrenderer uses MIT, FEX uses MIT. All compatible with MIT parent license.

---

## Success Metrics

### Phase 1 (MVP Success)
- [ ] Launch Linux ARM64 VM with GPU acceleration
- [ ] Achieve 80%+ of native Metal performance (benchmarked)
- [ ] 100+ GitHub stars in first month
- [ ] 5+ community contributors
- [ ] Zero critical bugs in core VM functionality

### Phase 2 (Adoption Success)
- [ ] 1000+ active users
- [ ] 50+ community contributors
- [ ] Featured in major tech publications (Phoronix, Ars Technica)
- [ ] x86_64 Linux gaming validated (Steam games playable)
- [ ] 85-90% GPU performance achieved

### Phase 3 (Market Success)
- [ ] 10,000+ active users
- [ ] Competitive with Parallels/VMware for gaming use case
- [ ] 95%+ GPU performance achieved
- [ ] Commercial partnerships or sponsorships
- [ ] Considered "the" open-source VM solution for Apple Silicon

---

## Conclusion

PearVisor aims to fill a critical gap in the Apple Silicon ecosystem: high-performance, open-source virtualization with near-native GPU acceleration. By combining proven technologies (Apple's Virtualization.framework, Venus, MoltenVK) with custom optimizations and a developer-friendly architecture, we can deliver a solution that rivals commercial offerings while remaining free and open-source.

The phased approach ensures we deliver value early (80% performance MVP) while building toward the ultimate goal (95%+ performance with full gaming support). By focusing on gaming, AI/ML, and containerized workloads, we address the most performance-sensitive use cases and differentiate from existing solutions.

**Next Steps:**
1. Finalize project name and branding
2. Set up GitHub repository and project structure
3. Begin Phase 1 Month 1-2 tasks (foundation)
4. Recruit initial contributors (GPU experts, macOS developers)
5. Create initial benchmark suite and baseline measurements

---

**Let's build this.**
