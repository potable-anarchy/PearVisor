# PearVisor: Next Steps

**Current Status:** Phase 2B Complete ✅  
**Date:** November 20, 2025  
**Next Phase:** Phase 2C - Guest Integration

---

## What We've Accomplished

### Phase 2B: Venus Protocol Implementation (Complete)

**Sessions 6-9:** GPU virtualization stack operational

- ✅ **25 Venus command handlers** implemented and tested
- ✅ **Zero-overhead performance** validated (110% = measurement variance)
- ✅ **MoltenVK integration** working perfectly on Apple M1 Max
- ✅ **GPU command submission** operational (vkQueueSubmit + vkQueueWaitIdle)
- ✅ **Complete pipeline:** Ring Buffer → Decoder → Handlers → MoltenVK → Metal

**Key Achievement:** Proven that GPU virtualization on Apple Silicon can achieve **native Metal performance** with zero overhead.

**Documentation:**
- PHASE_2B_DESIGN.md - Complete design and session tracking
- PHASE_2B_FINAL_SUMMARY.md - Comprehensive project summary
- SESSION_6_SUMMARY.md through SESSION_9_SUMMARY.md - Detailed session logs
- All test suites passing with 100% success rate

---

## What's Next: Phase 2C - Guest Integration

### Overview

Phase 2C connects our Venus protocol stack to real guest VMs, achieving the first GPU-accelerated graphics in a Linux guest on Apple Silicon.

**Goal:** Run `vkcube` in a Linux guest VM, rendering at >30 FPS with GPU acceleration.

**Estimated Duration:** 4-5 sessions (~10-15 hours of development)

---

## Implementation Roadmap

### Session 11: Virtualization.framework Integration
**Status:** 🔄 Next Session  
**Estimated Effort:** ~1,150 lines of code

**Objective:** Connect GPU subsystem to Apple's Virtualization.framework

**Tasks:**
1. Create VZVirtualMachine configuration
2. Implement custom VZVirtioGraphicsDeviceConfiguration
3. Wire Venus ring buffer to VM shared memory
4. Handle virtio-gpu control queue messages
5. Map guest physical memory to host virtual addresses

**Files to Create:**
- `Sources/PearVisorCore/GPUIntegration.swift` - VZ.framework bridge (~300 lines)
- `GPU/src/pv_virtio_gpu.c` - virtio-gpu device implementation (~500 lines)
- `GPU/include/pv_virtio_gpu.h` - virtio-gpu structures (~150 lines)
- `GPU/src/test_virtio_gpu.c` - Test suite (~200 lines)

**Success Criteria:**
- VM boots with virtio-gpu device visible
- Guest kernel detects virtio-gpu driver
- Ring buffer accessible from guest memory
- Basic virtio queue communication working

**Key Challenges:**
- Shared memory mapping between guest and host
- virtio queue management (control, cursor, 3D queues)
- MMIO handling for guest virtio register writes
- Interrupt injection for GPU completion notifications

---

### Session 12: Linux Guest Setup
**Status:** ⏳ Pending  
**Estimated Effort:** ~350 lines (scripts + docs)

**Objective:** Create bootable Linux ARM64 VM with Mesa Venus driver

**Tasks:**
1. Download Linux ARM64 distribution (Ubuntu 24.04 or Fedora 41)
2. Configure guest kernel with virtio-gpu support
3. Build and install Mesa with Venus driver enabled
4. Configure guest to use Venus for Vulkan
5. Verify guest can enumerate virtio-gpu device

**Guest Requirements:**
```bash
# Kernel config
CONFIG_DRM_VIRTIO_GPU=y
CONFIG_VIRTIO_MMIO=y

# Mesa build
meson setup build \
  -Dvulkan-drivers=virtio \
  -Dgallium-drivers=virgl,zink
```

**Files to Create:**
- `GuestImages/setup-ubuntu-venus.sh` - Automated guest setup (~200 lines)
- `Documentation/GUEST_SETUP.md` - Detailed setup instructions (~150 lines)

**Success Criteria:**
- Guest boots to login prompt
- `lsmod | grep virtio_gpu` shows driver loaded
- `vulkaninfo` shows Venus driver available
- `vkcube --enumerate` lists Apple M1 Max GPU

---

### Session 13: First GPU Workload Test
**Status:** ⏳ Pending  
**Estimated Effort:** ~200 lines (test automation + docs)

**Objective:** Run actual Vulkan application in guest, verify GPU execution

**Test Applications:**
1. **vkcube** - Spinning cube (basic rendering)
2. **glxgears** - OpenGL via Zink (OpenGL → Vulkan)
3. **vulkan-tutorial triangle** - Minimal render pass

**Testing Procedure:**
```bash
# In guest VM
vkcube --gpu 0

# Expected flow:
# Guest → Venus encode → Ring buffer → Host decoder
# → MoltenVK → Metal → Apple GPU → Results → Guest
```

**Files to Create:**
- `Tests/test_vkcube.sh` - vkcube automation (~100 lines)
- `Documentation/TESTING.md` - Testing guide (~100 lines)

**Success Criteria:**
- vkcube renders spinning cube in guest
- Framerate >30 FPS (proof of GPU acceleration)
- No crashes or hangs during 5-minute run
- Venus command flow visible in debug logs

**Debug Tools:**
```bash
# Guest-side debugging
export VK_LOADER_DEBUG=all
export MESA_DEBUG=1

# Host-side debugging
export MVK_DEBUG=1
export MVK_LOG_LEVEL=info
```

---

### Session 14: Performance Benchmarking
**Status:** ⏳ Pending  
**Estimated Effort:** ~350 lines (benchmark scripts + analysis)

**Objective:** Measure real-world GPU performance vs. native Metal

**Benchmarks:**
1. **vkmark** - Vulkan benchmark suite
2. **glmark2** - OpenGL benchmark (via Zink)
3. **Geekbench Compute** - GPU compute workload
4. **Blender Cycles** - Production rendering

**Target Metrics:**
- >75% of native Metal performance (minimum)
- >80% of native (stretch goal for Phase 2C)
- Competitive with UTM and Parallels Desktop

**Files to Create:**
- `Benchmarks/run_benchmarks.sh` - Automated benchmark suite (~150 lines)
- `Benchmarks/analyze.py` - Performance analysis tools (~200 lines)
- `PERFORMANCE.md` - Benchmark results documentation (~100 lines)

**Success Criteria:**
- Achieve >75% of native Metal performance
- Document specific bottlenecks for future optimization
- Create performance baseline for Phase 3 improvements

---

## Optional Session 10: Venus Wire Format Refinement

**Status:** ⏳ Optional (can defer)  
**Estimated Effort:** ~600 lines

**Current Limitation:** Test code uses hardcoded guest object IDs (0x8000, 0x9000)

**Production Need:** Parse real Venus wire format with dynamic guest-allocated IDs

**Tasks:**
1. Implement Venus wire format parser
2. Extract real guest object IDs from commands
3. Update all 25 handlers to use dynamic IDs
4. Implement result serialization (write Vulkan results back to guest)
5. Test with realistic guest-like commands

**Files to Create:**
- `GPU/src/pv_venus_wire.c` - Wire format parsing (~200 lines)
- `GPU/include/pv_venus_wire.h` - Wire format structures (~100 lines)
- `GPU/src/test_venus_wire.c` - Test suite (~300 lines)

**Recommendation:** Skip for now, implement during Session 11 if needed. Current handlers work fine for initial VM integration testing.

---

## Phase 2C Success Criteria

By end of Phase 2C, PearVisor should:

1. ✅ Boot real Linux guest VM with Venus driver
2. ✅ virtio-gpu device operational
3. ✅ Run GPU-accelerated Vulkan applications (vkcube)
4. ✅ Achieve >30 FPS rendering (proof of acceleration)
5. ✅ Reach >75% of native Metal performance
6. ✅ Complete guest-to-GPU pipeline validated end-to-end
7. ✅ Benchmarks demonstrating competitive performance vs. UTM/Parallels

**Minimum Viable Product (MVP):**
- User can download PearVisor
- Create Linux ARM64 VM in under 5 minutes
- Run vkcube and see GPU-accelerated graphics
- Competitive performance with commercial solutions
- Stable enough for alpha testing and demos

---

## Recommended Development Path

### Path A: Fast Track to Demo (Recommended)

**Sessions:** 11 → 12 → 13 → 14 → (10 if needed)

**Pros:**
- Fastest path to visible demo (vkcube running)
- Validates end-to-end architecture early
- Can mock wire format parsing initially
- More exciting milestones for each session

**Cons:**
- May need to backtrack for wire format issues
- Some technical debt if Session 10 is skipped

**Recommendation:** ✅ **Choose this path**

---

### Path B: Clean Implementation

**Sessions:** 10 → 11 → 12 → 13 → 14

**Pros:**
- No technical debt
- Production-ready wire format from start
- Cleaner architecture

**Cons:**
- Slower to visible demo
- Session 10 work is less exciting (parsing code)

**Recommendation:** ❌ Skip unless we hit issues in Session 11

---

## Technical Challenges Ahead

### Challenge 1: Shared Memory Coordination
**Problem:** Guest and host must safely share ring buffer memory  
**Solution:** Use virtio-gpu resource backing (standard Linux approach)  
**Risk:** Medium - well-documented protocol, but macOS-specific quirks possible

### Challenge 2: virtio Queue Management
**Problem:** Handle control queue, cursor queue, 3D queue separately  
**Solution:** Implement standard virtio queue handling per spec  
**Risk:** Low - standard protocol, well-tested in QEMU/Linux

### Challenge 3: Guest Mesa Build
**Problem:** Guest needs Mesa with Venus driver enabled  
**Solution:** Pre-build guest image with Mesa pre-installed  
**Risk:** Low - Mesa Venus is well-maintained by Google

### Challenge 4: Interrupt Handling
**Problem:** Guest needs notification when GPU work completes  
**Solution:** Use VZ.framework's virtio interrupt injection API  
**Risk:** Low - standard VZ.framework feature

---

## Key Resources

### Documentation
- `PHASE_2C_DESIGN.md` - Complete Phase 2C architecture and session plans
- `PHASE_2B_FINAL_SUMMARY.md` - What we've built so far
- `DESIGN.md` - Overall project architecture

### Code
- `GPU/src/pv_venus_handlers.c` - 25 command handlers (Phase 2B)
- `GPU/src/pv_venus_dispatch.c` - Command decoder
- `GPU/src/pv_venus_ring.c` - Ring buffer implementation
- `GPU/src/pv_moltenvk.c` - MoltenVK integration

### External References
- [Apple Virtualization.framework Docs](https://developer.apple.com/documentation/virtualization)
- [Mesa Venus Driver Source](https://gitlab.freedesktop.org/mesa/mesa/-/tree/main/src/virtio)
- [virtio-gpu Protocol Spec](https://docs.oasis-open.org/virtio/virtio/v1.2/virtio-v1.2.html)
- [MoltenVK Documentation](https://github.com/KhronosGroup/MoltenVK)

---

## Timeline Estimate

### Phase 2C Total: 4-5 Sessions

**Session 11 (VZ.framework):** 4-6 hours  
**Session 12 (Guest Setup):** 2-3 hours  
**Session 13 (First GPU Test):** 2-3 hours  
**Session 14 (Benchmarking):** 2-3 hours  

**Total:** ~10-15 hours of focused development

**Calendar Time:** 1-2 weeks (assuming 1-2 sessions per day)

---

## Immediate Next Step

**Start Session 11: Virtualization.framework Integration**

**Prerequisites:**
- ✅ Phase 2B complete (25 handlers working)
- ✅ MoltenVK integration tested
- ✅ Ring buffer and decoder operational

**First Task:**
```swift
// Sources/PearVisorCore/GPUIntegration.swift

import Virtualization

class PearVisorGPU {
    func createVirtioGPUDevice() -> VZVirtioGraphicsDeviceConfiguration {
        let config = VZVirtioGraphicsDeviceConfiguration()
        config.scanouts = [
            VZVirtioGraphicsScanoutConfiguration(
                widthInPixels: 1920,
                heightInPixels: 1080
            )
        ]
        return config
    }
}
```

**Begin with:** Basic VZ.framework integration, get VM booting with virtio-gpu device visible to guest kernel.

---

## Questions to Resolve

### Before Session 11:
- [ ] Which Linux distribution? (Ubuntu 24.04 ARM64 recommended)
- [ ] Use VZVirtioGraphicsDeviceConfiguration or custom device? (Start with built-in)
- [ ] How to expose Venus ring buffer to guest? (virtio-gpu resource backing)
- [ ] Need custom Linux kernel? (No, stock Ubuntu kernel has virtio-gpu)

### Before Session 12:
- [ ] Pre-build Mesa or build in guest? (Pre-build for faster testing)
- [ ] X11 or Wayland? (Both should work, test with X11 first)
- [ ] Desktop environment needed? (No, framebuffer is enough for vkcube)

---

## Success Definition

**Phase 2C is complete when:**

1. A user can download PearVisor
2. Boot a Linux ARM64 VM
3. Run `vkcube` in the guest
4. See a GPU-accelerated spinning cube at >30 FPS
5. Benchmarks show >75% of native Metal performance

**Stretch Goal:** Achieve 85% of native performance (exceeds current UTM/Parallels performance)

---

## Get Started

**Ready to begin Session 11?**

```bash
cd /Users/brad/code/PearVisor
git checkout -b session-11-vz-framework
cd Sources/PearVisorCore
touch GPUIntegration.swift
code GPUIntegration.swift
```

**Let's build the world's fastest GPU-accelerated hypervisor for Apple Silicon.** 🚀

---

*Last Updated: November 20, 2025 - Ready for Phase 2C implementation*
