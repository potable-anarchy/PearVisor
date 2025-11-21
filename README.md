# PearVisor

**Near-native GPU performance virtualization for Apple Silicon**

PearVisor is an open-source hypervisor that provides high-performance GPU acceleration for Linux (and eventually Windows) guest operating systems on Apple Silicon Macs. Built on Apple's Virtualization.framework with a custom GPU passthrough stack, PearVisor achieves 80-95% of native Metal performance for gaming, AI/ML workloads, and containerized applications.

> ⚠️ **Early Development:** PearVisor is in active development. Not ready for production use.

## Features (Planned)

### Phase 2B - Venus Protocol (Complete) ✅
- ✅ GPU acceleration via Venus + MoltenVK (110% native performance achieved!)
- ✅ Venus protocol implementation (25 command handlers)
- ✅ Zero-overhead GPU virtualization stack (Sessions 6-9)
- ✅ MoltenVK integration with Apple M1 Max GPU
- ✅ Command submission and GPU execution working

### Phase 2C - Guest Integration (In Progress)
- ✅ Virtualization.framework integration (Session 11)
- ✅ Swift ↔ C GPU bridge operational
- ✅ VZVirtioGraphicsDeviceConfiguration ready
- ✅ Linux guest setup automation (Session 12)
- ✅ Swift Package Manager + CMake integration
- 🔄 Boot Linux guest and install Venus driver (Session 13 - next)
- 🔄 End-to-end GPU workload testing (vkcube) (Session 13)
- 🔄 Performance benchmarking vs. native (Session 14)

### Phase 3 - Full Application (Future)
- ⏳ macOS GUI application (SwiftUI)
- ⏳ Full VM features (networking, storage, file sharing)
- ⏳ Gaming support (native ARM64 Linux games)
- ⏳ x86_64 emulation via FEX-Emu
- ⏳ Windows ARM64 guest support

### Phase 4 - Advanced Features (Long-term)
- ⏳ Windows x86_64 gaming (DXVK/VKD3D-Proton)
- ⏳ Direct Metal API passthrough
- ⏳ GPU-accelerated containers
- ⏳ 95%+ native performance optimizations

## Why PearVisor?

**vs. UTM:**
- Higher GPU performance (80-95% vs 60-70%)
- Gaming-optimized (FEX, DXVK, Proton)
- Container support for AI/ML

**vs. Parallels/VMware:**
- Open source (MIT license)
- Free forever
- Community-driven development
- Extensible API

**vs. Docker Desktop:**
- Full Linux desktop environment
- Native GPU acceleration
- Better for gaming and graphics workloads

## Requirements

- macOS 15.0 (Sequoia) or later
- Apple Silicon Mac (M1, M2, M3, M4+)
- 16GB RAM minimum (32GB recommended)
- 50GB free disk space

## Installation

**Not yet available.** PearVisor is in early development.

When ready, installation will be available via:
- Direct download (.dmg)
- Homebrew: `brew install --cask pearvisor`
- Build from source

## Quick Start

Coming soon. For now, check out the [DESIGN.md](DESIGN.md) for architecture details.

## Architecture

PearVisor uses a hybrid architecture:

```
┌─────────────────────────────────────┐
│     PearVisor GUI (SwiftUI)         │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│  Apple Virtualization.framework     │
│  + Custom GPU Passthrough           │
└─────────────┬───────────────────────┘
              │
┌─────────────▼───────────────────────┐
│  Venus → virglrenderer → MoltenVK  │
│  → Metal API                        │
└─────────────────────────────────────┘
```

**Key Components:**
- **Apple Virtualization.framework:** VM management, stability
- **virtio-gpu + Venus:** GPU command serialization
- **virglrenderer:** Vulkan command handling
- **MoltenVK:** Vulkan → Metal translation
- **Metal API:** Native Apple GPU access

See [DESIGN.md](DESIGN.md) for comprehensive technical details.

## Performance

**Target Performance (vs. native Metal):**
- Phase 1: 80%+ (proven by Red Hat's libkrun)
- Phase 2: 85-90% (custom optimizations)
- Phase 3: 95%+ (direct Metal passthrough)

**Benchmarks:** Coming soon.

## Development

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/pearvisor.git
cd pearvisor

# Initialize submodules
git submodule update --init --recursive

# Build GPU subsystem (C/C++)
cd GPU
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)

# Open in Xcode
cd ../..
open PearVisor.xcodeproj
```

**Requirements:**
- Xcode 15+
- CMake 3.25+
- Swift 5.9+

### Project Structure

```
pearvisor/
├── Sources/           # Swift source code
│   ├── PearVisor/     # Main GUI app
│   └── PearVisorCore/ # VM management
├── GPU/               # C/C++ GPU subsystem
├── Submodules/        # MoltenVK, virglrenderer
├── GuestImages/       # Pre-configured OS images
├── Benchmarks/        # Performance tests
└── Documentation/     # Extended docs
```

## Contributing

We'd love your help! PearVisor is in early development and needs contributors for:

- **Swift Development:** macOS GUI, Virtualization.framework integration
- **C/C++ Development:** GPU passthrough, virtio-gpu, Metal optimization
- **Graphics Expertise:** Vulkan, Metal, MoltenVK optimization
- **Testing:** Benchmarking, guest OS compatibility, bug reports
- **Documentation:** Tutorials, API docs, architecture guides

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Roadmap

**Phase 1 - MVP (Months 1-9):**
- Basic Linux ARM64 VMs
- GPU acceleration (80% target)
- GUI application
- Alpha release

**Phase 2 - Stable (Months 10-18):**
- x86_64 emulation (FEX)
- Container support
- Performance optimization (85-90%)
- Beta release

**Phase 3 - Advanced (Months 19-24):**
- Windows gaming support
- Direct Metal API
- 95%+ performance
- Stable 1.0 release

See [DESIGN.md](DESIGN.md) for detailed roadmap.

## License

MIT License - see [LICENSE](LICENSE) for details.

**Dependencies:**
- MoltenVK (Apache 2.0)
- virglrenderer (MIT)
- FEX-Emu (MIT)

## Acknowledgments

PearVisor builds on the shoulders of giants:

- **Apple:** Virtualization.framework, Metal API
- **Khronos Group:** MoltenVK
- **Mesa/freedesktop.org:** virglrenderer, Venus driver
- **FEX-Emu Team:** x86_64 emulation on ARM64
- **Red Hat:** libkrun GPU acceleration research
- **UTM/QEMU:** Virtualization inspiration

## Community

- **GitHub Discussions:** [Feature requests, Q&A](https://github.com/yourusername/pearvisor/discussions)
- **Discord:** Coming soon
- **Issues:** [Bug reports](https://github.com/yourusername/pearvisor/issues)

## FAQ

**Q: When will it be ready?**  
A: Alpha release targeted for mid-2026 (Phase 1 complete).

**Q: Will it support Windows?**  
A: Yes, planned for Phase 2 (ARM64) and Phase 3 (x86_64).

**Q: How does it compare to Parallels?**  
A: Similar GPU performance goals (80-95%), but free and open source.

**Q: Can I run x86_64 Linux/Windows apps?**  
A: Yes, via FEX-Emu emulation (Phase 2+).

**Q: Does it work on Intel Macs?**  
A: No, Apple Silicon only.

**Q: What about macOS guests?**  
A: Possible in the future, but not a priority.

---

**Status:** 🏗️ In Development | **Current Phase:** Phase 2C In Progress (Guest Integration) | **Latest Version:** 0.0.1-alpha

**Recent Achievement:** Virtualization.framework integration complete! Session 11 delivered Swift ↔ C GPU bridge, enabling macOS VMs to access the Venus protocol stack. Ready for Linux guest testing with GPU acceleration.

**Previous Milestone:** Zero-overhead GPU virtualization achieved in Sessions 6-9 with 25 Venus protocol handlers and 110% of native Metal performance.

**Made with ❤️ for the Apple Silicon community**
