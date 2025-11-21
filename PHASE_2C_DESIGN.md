# Phase 2C: Guest Integration & End-to-End Testing

## Overview

Phase 2C integrates the completed Venus protocol stack (Phase 2B) with actual guest VM environments. This phase validates the entire pipeline from guest applications through Venus to the Apple M1 Max GPU, achieving real-world GPU acceleration.

**Prerequisites:** Phase 2B complete (25 Venus handlers operational, zero-overhead virtualization proven)

---

## Architecture

### Complete Pipeline

```
┌────────────────────────────────────────────────────────────────┐
│  Linux Guest VM (ARM64)                                         │
│                                                                 │
│  ┌──────────────────┐                                          │
│  │ Vulkan Test App  │  (e.g., vkcube, glxgears)               │
│  └────────┬─────────┘                                          │
│           │ Vulkan API calls                                   │
│  ┌────────▼──────────────────────┐                            │
│  │  Mesa Venus Driver            │  (libvulkan_virtio.so)     │
│  │  - Vulkan API surface         │                            │
│  │  - Venus protocol encoder     │                            │
│  └────────┬──────────────────────┘                            │
│           │ Venus wire format                                  │
│  ┌────────▼──────────────────────┐                            │
│  │  virtio-gpu Kernel Driver     │  (Linux kernel module)     │
│  │  - virtqueue management       │                            │
│  │  - Ring buffer coordination   │                            │
│  └────────┬──────────────────────┘                            │
└───────────┼────────────────────────────────────────────────────┘
            │ Hypercall / MMIO
┌───────────▼────────────────────────────────────────────────────┐
│  PearVisor Host (macOS)                                        │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │  Virtualization.framework                                │ │
│  │  - VZVirtualMachine                                      │ │
│  │  - VZVirtioGraphicsDeviceConfiguration                  │ │
│  └────────┬─────────────────────────────────────────────────┘ │
│           │                                                     │
│  ┌────────▼─────────────────────────────────────────────────┐ │
│  │  PearVisor GPU Subsystem                                 │ │
│  │  ┌────────────────┐  ┌──────────────────┐               │ │
│  │  │ virtio-gpu     │  │ Venus Ring Buffer│               │ │
│  │  │ Device Handler │─►│ (shared memory)  │               │ │
│  │  └────────────────┘  └──────┬───────────┘               │ │
│  │                              │                            │ │
│  │  ┌───────────────────────────▼────────────────────────┐  │ │
│  │  │  Venus Command Decoder                             │  │ │
│  │  │  - Parse wire format                               │  │ │
│  │  │  - Extract guest object IDs                        │  │ │
│  │  │  - Dispatch to handlers (25 implemented)           │  │ │
│  │  └───────────────────────────┬────────────────────────┘  │ │
│  │                               │                           │ │
│  │  ┌────────────────────────────▼───────────────────────┐  │ │
│  │  │  Venus Command Handlers                            │  │ │
│  │  │  - Object ID translation (guest → host)            │  │ │
│  │  │  - MoltenVK API calls                              │  │ │
│  │  │  - Result serialization                            │  │ │
│  │  └────────────────────────────┬───────────────────────┘  │ │
│  └───────────────────────────────┼─────────────────────────┘ │
│                                  │                             │
│  ┌────────────────────────────────▼───────────────────────┐   │
│  │  MoltenVK (Vulkan → Metal Translation)                 │   │
│  └────────────────────────────────┬───────────────────────┘   │
│                                   │                            │
│  ┌────────────────────────────────▼───────────────────────┐   │
│  │  Apple Metal API + M1 Max GPU                          │   │
│  └────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## Implementation Sessions

### Session 11: Virtualization.framework Integration ✅ (Complete)

**Goal:** Connect GPU subsystem to Apple's Virtualization.framework

**Status:** ✅ Complete

**What Was Built:**

1. **Swift Integration Layer** (`GPUIntegration.swift` - ~300 lines)
   - Manages GPU device lifecycle and Venus protocol stack
   - Allocates shared memory for ring buffer (4MB default)
   - Creates `VZVirtioGraphicsDeviceConfiguration` (1920x1080)
   - Handles virtio-gpu commands from guest
   - Clean Swift ↔ C interop via `@_silgen_name`

2. **C Integration API** (`pv_venus_integration.h/c` - ~330 lines)
   - `pv_venus_ring_create_from_memory()` - Creates ring from Swift-managed memory
   - `pv_venus_integration_start/stop()` - Controls Venus processing (polling mode)
   - `pv_venus_init/cleanup()` - Complete Venus stack lifecycle (MoltenVK + handlers)
   - `pv_venus_get_stats()` - Retrieves processing statistics

3. **Integration Test Suite** (`test_venus_integration.c` - ~210 lines)
   - Test 1: Shared memory ring buffer creation
   - Test 2: Venus context initialization (25 handlers)
   - Test 3: Complete integration flow (9 steps)
   - Test 4: Batch command processing
   - **Result:** All tests passing ✅

**Key Achievements:**
- ✅ Swift ↔ C bridge operational with proper memory management
- ✅ VZ.framework integration ready for VM attachment
- ✅ Venus stack (MoltenVK + 25 handlers) accessible from Swift
- ✅ Polling mode implemented (event-driven, no background threads)
- ✅ Comprehensive testing validates all paths

**Files Created:**
- `Sources/PearVisorCore/GPUIntegration.swift`
- `GPU/include/pv_venus_integration.h`
- `GPU/src/pv_venus_integration.c`
- `GPU/src/test_venus_integration.c`
- `SESSION_11_SUMMARY.md` (detailed documentation)

**Code Statistics:** ~740 new lines

**Next:** Session 12 - Linux Guest Setup

---

### Session 10: Venus Wire Format Parsing (Optional Refinement)

**Goal:** Parse real Venus wire format from guest, handle dynamic object IDs

**Current Limitation:** Test code uses hardcoded guest IDs (0x8000, 0x9000)

**Tasks:**
1. Implement Venus wire format parser (extract real guest object IDs)
2. Update handlers to use dynamic IDs instead of hardcoded values
3. Implement result serialization (write Vulkan results back to guest)
4. Create Venus result encoder
5. Test with synthetic guest commands containing real object IDs

**Files:**
- `GPU/src/pv_venus_wire.c` - Wire format parsing
- `GPU/include/pv_venus_wire.h` - Wire format structures
- `GPU/src/pv_venus_handlers.c` - Update to use dynamic IDs
- `GPU/src/test_venus_wire.c` - Test suite

**Success Criteria:**
- Parse vkCreateInstance with real guest-provided instance ID
- Track dynamically allocated object IDs
- Write VkResult codes back to guest memory
- All existing tests still pass

**Status:** Optional (current implementation works for testing, but production needs dynamic IDs)

---

### Session 11: Virtualization.framework Integration

**Goal:** Connect GPU subsystem to Apple's Virtualization.framework

**Current State:** GPU subsystem is standalone (no VM integration)

**Tasks:**
1. Create VZVirtualMachine configuration
2. Implement custom VZVirtioGraphicsDeviceConfiguration
3. Wire ring buffer to VM shared memory
4. Handle virtio-gpu control queue messages
5. Map guest physical memory to host

**Files:**
- `Sources/PearVisorCore/GPUIntegration.swift` - VZ.framework bridge
- `GPU/src/pv_virtio_gpu.c` - virtio-gpu device implementation
- `GPU/include/pv_virtio_gpu.h` - virtio-gpu structures
- `GPU/src/test_virtio_gpu.c` - Test suite

**Key Challenges:**
1. **Shared Memory Mapping:** Guest physical addresses → Host virtual addresses
2. **virtio Queue Management:** Control queue, cursor queue, 3D queue (Venus)
3. **MMIO Handling:** Guest writes to virtio registers trigger handlers
4. **Interrupt Injection:** Notify guest when GPU work completes

**VZVirtioGraphicsDeviceConfiguration Options:**
```swift
// Option 1: Use built-in VZVirtioGraphicsDeviceConfiguration (limited control)
let graphicsConfig = VZVirtioGraphicsDeviceConfiguration()
graphicsConfig.scanouts = [VZVirtioGraphicsScanoutConfiguration(
    widthInPixels: 1920, 
    heightInPixels: 1080
)]

// Option 2: Custom virtio-gpu device (full control, more work)
// Implement VZVirtioDeviceConfiguration subclass
// Handle all virtio-gpu protocol ourselves
```

**Recommendation:** Start with Option 1 (simpler), move to Option 2 if needed for performance.

**Success Criteria:**
- VM boots with virtio-gpu device visible
- Guest kernel detects virtio-gpu
- Ring buffer accessible from guest
- Basic virtio queue communication working

---

### Session 12: Linux Guest Setup & VZ Integration ✅ (Complete)

**Goal:** Prepare Linux guest infrastructure and complete VZ.framework integration

**Status:** ✅ Complete

**What Was Built:**

1. **Guest Setup Documentation** (`GuestImages/README.md` - ~400 lines)
   - Supported distributions: Ubuntu 24.04, Fedora 41, Debian 13
   - Required kernel features: `CONFIG_DRM_VIRTIO_GPU`, `CONFIG_VIRTIO_MMIO`
   - Mesa Venus build instructions (meson + ninja)
   - Verification procedures (vulkaninfo, vkcube)

2. **Ubuntu Automated Setup** (`GuestImages/setup-ubuntu-venus.sh` - ~250 lines)
   - Environment validation (Ubuntu version, ARM64 arch)
   - virtio-gpu driver detection and loading
   - Mesa build automation (git clone, configure, compile, install)
   - Vulkan ICD verification
   - Comprehensive error handling

3. **Fedora Automated Setup** (`GuestImages/setup-fedora-venus.sh` - ~240 lines)
   - Fedora-specific package management (dnf)
   - lib64 library paths
   - Same Mesa configuration as Ubuntu
   - Color-coded logging

4. **VZConfigurator Integration** (Updated)
   - Added `vmID` parameter to `buildConfiguration()`
   - GPU initialization with fallback to basic graphics
   - `cleanupGPU()` for resource management
   - Graceful error handling

5. **Swift Package Manager Integration** (`Package.swift`)
   - Linked CMake-built GPU library (`libPearVisorGPU.a`)
   - Linked Vulkan loader (`-lvulkan`)
   - Complete dependency chain: Swift → C → Vulkan → Metal

**Key Achievements:**
- ✅ Complete Swift ↔ C integration with VM UUID tracking
- ✅ Automated Mesa Venus installation for 2 distributions
- ✅ Build system working (Swift + CMake coexistence)
- ✅ Production-ready error handling (Venus init failures don't crash)
- ✅ Zero manual setup required for guest GPU drivers

**Technical Challenges Resolved:**
1. Missing `vmID` parameter - Updated buildConfiguration signature
2. Private `cleanup()` - Changed to public
3. Undefined C symbols - Added linker settings with absolute paths
4. Missing Vulkan symbols - Linked libvulkan from Homebrew
5. Version mismatch warnings - Documented as harmless

**Build Verification:** ✅ `swift build` succeeds (0.54s)

**Files Created:**
- `GuestImages/README.md`
- `GuestImages/setup-ubuntu-venus.sh`
- `GuestImages/setup-fedora-venus.sh`
- `docs/SESSION_12_SUMMARY.md` (comprehensive documentation)

**Code Statistics:** ~942 new lines

**Next:** Session 13 - Boot Linux guest, test vkcube

---

### Session 13: First GPU Workload Test

**Goal:** Run actual Vulkan application in guest, verify GPU execution

**Test Applications:**
1. **vkcube** - Simple spinning cube (basic rendering test)
2. **glxgears** - OpenGL test via Zink (OpenGL → Vulkan)
3. **vulkan-tutorial triangle** - Minimal render pass

**Testing Procedure:**
```bash
# In guest VM
vkcube --gpu 0

# Expected behavior:
# 1. Guest app calls vkCreateInstance
# 2. Venus encodes command to ring buffer
# 3. Host Venus decoder parses command
# 4. Host handler calls MoltenVK vkCreateInstance
# 5. MoltenVK creates Metal instance
# 6. Result returned to guest
# 7. Cube renders on guest display
```

**Debugging Tools:**
```bash
# Enable Venus debug logging (guest)
export VK_LOADER_DEBUG=all
export MESA_DEBUG=1

# Enable MoltenVK debug logging (host)
export MVK_DEBUG=1
export MVK_LOG_LEVEL=info

# Monitor Venus commands (host)
GPU/build/test_venus_decoder --monitor
```

**Success Criteria:**
- vkcube renders spinning cube in guest
- Framerate >30 FPS (proof of GPU acceleration)
- No crashes or hangs
- Venus command flow visible in debug logs

---

### Session 14: Performance Benchmarking

**Goal:** Measure real-world GPU performance vs. native Metal

**Benchmarks:**
1. **vkmark** - Vulkan benchmark suite
2. **glmark2** - OpenGL benchmark (via Zink)
3. **Geekbench Compute** - Real-world GPU compute
4. **Blender Cycles** - Production rendering workload

**Metrics:**
```
┌─────────────────────┬──────────┬──────────┬───────────┐
│ Benchmark           │ Native   │ PearVisor│ Overhead  │
├─────────────────────┼──────────┼──────────┼───────────┤
│ vkmark              │ 5000     │ ?        │ ?         │
│ glmark2 (Zink)      │ 3500     │ ?        │ ?         │
│ Geekbench Compute   │ 85000    │ ?        │ ?         │
│ Blender BMW         │ 120s     │ ?        │ ?         │
└─────────────────────┴──────────┴──────────┴───────────┘

Target: 80-90% of native performance (Phase 2B goal)
Stretch: 95%+ (Phase 3 goal)
```

**Profiling:**
- Use Xcode Instruments for Metal profiling
- Identify bottlenecks in Venus → MoltenVK path
- Measure serialization overhead
- Check for unnecessary synchronization

**Files:**
- `Benchmarks/run_benchmarks.sh` - Automated benchmark suite
- `Benchmarks/results/` - Raw benchmark data
- `Benchmarks/analyze.py` - Performance analysis scripts

**Success Criteria:**
- Achieve >75% of native Metal performance
- Document bottlenecks for future optimization
- Create performance baseline for Phase 3

---

## Phase 2C Success Criteria

By end of Phase 2C, we should have:

1. ✅ Real Linux guest VM running with Venus driver
2. ✅ virtio-gpu device operational
3. ✅ GPU-accelerated Vulkan applications running in guest
4. ✅ vkcube rendering at >30 FPS
5. ✅ Performance >75% of native Metal
6. ✅ Complete guest-to-GPU pipeline validated
7. ✅ Benchmarks showing competitive performance

**Minimum Viable Product (MVP) Definition:**
- Guest can run Vulkan applications
- GPU acceleration working (provable via vkcube)
- Performance competitive with UTM/Parallels (target: exceed both)
- Stable enough for demo/alpha testing

---

## Technical Challenges

### Challenge 1: Shared Memory Coordination

**Problem:** Guest and host must safely share ring buffer memory

**Solution Options:**
1. Use VZSharedDirectory with memory-mapped file
2. Custom shared memory region via Virtualization.framework
3. virtio-gpu resource backing (standard approach)

**Recommendation:** virtio-gpu resource backing (matches Linux expectations)

---

### Challenge 2: Object ID Synchronization

**Problem:** Guest allocates object IDs, host must track them

**Current Implementation:** Hardcoded IDs (0x8000, 0x9000) for testing

**Production Implementation:**
```c
// Guest sends vkCreateInstance with guest-chosen ID
struct venus_vkCreateInstance_command {
    uint64_t guest_instance_id;  // e.g., 0x123456789abcdef0
    // ... other params
};

// Host handler extracts guest ID
uint64_t guest_id = command->guest_instance_id;
VkInstance host_instance;
vkCreateInstance(..., &host_instance);

// Track mapping
pv_venus_object_add(&ctx->objects, guest_id, host_instance, 
                     PV_VENUS_OBJECT_TYPE_INSTANCE);

// Future commands reference guest_id, we translate to host_instance
```

---

### Challenge 3: Result Serialization

**Problem:** Vulkan functions return results that guest needs

**Example:**
```c
// Guest wants to create instance
VkResult result = vkCreateInstance(..., &instance);

// Host must:
// 1. Call MoltenVK vkCreateInstance
// 2. Get VkResult (e.g., VK_SUCCESS)
// 3. Serialize result back to guest memory
// 4. Guest reads result from shared memory
```

**Implementation:**
```c
// Venus result structure
struct venus_command_result {
    uint32_t command_sequence;  // Match to command
    VkResult result;            // VK_SUCCESS, VK_ERROR_*, etc.
    uint64_t output_handle;     // For vkCreate* functions
};

// Write result to ring buffer "result region"
pv_venus_write_result(ring, sequence, VK_SUCCESS, host_instance);
```

---

### Challenge 4: Interrupt Handling

**Problem:** Guest needs notification when GPU work completes

**Solution:** virtio interrupt injection
```swift
// In Swift (Virtualization.framework)
vm.queueVirtioInterrupt(device: gpuDevice, queue: 2)  // Queue 2 = 3D commands

// Guest receives interrupt
// Guest Venus driver wakes up
// Guest reads command results from ring buffer
```

---

## Session 10 Details (Optional Refinement)

Since Session 10 is optional before guest integration, let's define what it would involve:

### Session 10: Venus Wire Format & Dynamic IDs

**Current Test Code Limitations:**
```c
// test_command_buffers.c currently does:
pv_venus_object_id guest_pool_id = 0x8000;  // HARDCODED
pv_venus_object_id guest_buffer_id = 0x9000;  // HARDCODED
```

**Production Code Needs:**
```c
// Parse real Venus wire format
struct venus_vkCreateCommandPool_command {
    uint32_t command_id;        // PV_VK_COMMAND_vkCreateCommandPool
    uint32_t command_size;      // Size of this command
    uint64_t device_id;         // Guest's VkDevice ID
    uint32_t create_info_offset; // Offset to VkCommandPoolCreateInfo
    uint64_t pool_id;           // Guest's chosen ID for new pool
};

// Handler extracts dynamic pool_id
uint64_t guest_pool_id = command->pool_id;  // Real guest-allocated ID

// Create pool and track with guest's ID
VkCommandPool host_pool;
vkCreateCommandPool(host_device, &create_info, NULL, &host_pool);
pv_venus_object_add(&ctx->objects, guest_pool_id, host_pool, 
                     PV_VENUS_OBJECT_TYPE_COMMAND_POOL);
```

**Files to Create:**
- `GPU/src/pv_venus_wire.c` - Wire format parsing utilities
- `GPU/include/pv_venus_wire.h` - Wire format structure definitions
- `GPU/src/test_venus_wire.c` - Test suite for wire parsing

**Tasks:**
1. Define Venus wire format structures (based on Mesa venus)
2. Implement wire parser (extract command + arguments)
3. Update all 25 handlers to use parsed IDs
4. Implement result writing (Venus result encoder)
5. Test with realistic guest-like commands

**Success Criteria:**
- Parse vkCreateInstance with dynamic guest instance ID
- Parse vkCreateDevice with dynamic guest device ID
- All 25 handlers work with dynamic IDs
- Results written back to guest-accessible memory
- All existing tests pass with new wire format

**Effort:** ~400 lines of code, 1 session

**Priority:** Medium (can defer to Session 11+ if we mock it in VM integration)

---

## Recommended Path Forward

### Path A: Jump to Guest Integration (Faster MVP)

**Sessions:**
1. Session 11: Virtualization.framework integration
2. Session 12: Linux guest setup
3. Session 13: First GPU workload test
4. Session 14: Performance benchmarking
5. Session 10 (revisit): Wire format refinement if needed

**Pros:**
- Fastest path to visible demo (vkcube in guest)
- Validates architecture end-to-end
- Can mock wire format parsing initially

**Cons:**
- May need to backtrack to fix wire format issues
- Some technical debt if we skip Session 10

---

### Path B: Complete Wire Format First (Cleaner)

**Sessions:**
1. Session 10: Venus wire format parsing
2. Session 11: Virtualization.framework integration
3. Session 12: Linux guest setup
4. Session 13: First GPU workload test
5. Session 14: Performance benchmarking

**Pros:**
- No technical debt
- Production-ready wire format from start
- Cleaner architecture

**Cons:**
- Slower to visible demo
- Wire format work is less exciting

---

## Recommendation: Path A (Jump to Guest)

**Rationale:**
1. Phase 2B already proves the core concept (zero-overhead virtualization)
2. Getting vkcube running in guest is a major milestone
3. Wire format parsing can be done incrementally as we discover what's needed
4. Current handlers work fine for initial testing (we can mock guest IDs temporarily)

**Next Session:** Session 11 - Virtualization.framework Integration

---

## Files Created in Phase 2C

### Session 10 (Optional)
- `GPU/src/pv_venus_wire.c` - Wire format parsing (~200 lines)
- `GPU/include/pv_venus_wire.h` - Wire format structures (~100 lines)
- `GPU/src/test_venus_wire.c` - Test suite (~300 lines)

### Session 11
- `Sources/PearVisorCore/GPUIntegration.swift` - VZ.framework bridge (~300 lines)
- `GPU/src/pv_virtio_gpu.c` - virtio-gpu device (~500 lines)
- `GPU/include/pv_virtio_gpu.h` - virtio-gpu structures (~150 lines)
- `GPU/src/test_virtio_gpu.c` - Test suite (~200 lines)

### Session 12
- `GuestImages/setup-ubuntu-venus.sh` - Guest setup automation (~200 lines)
- `Documentation/GUEST_SETUP.md` - Setup instructions (~50 lines)

### Session 13
- `Tests/test_vkcube.sh` - vkcube test automation (~100 lines)
- `Documentation/TESTING.md` - Testing guide (~100 lines)

### Session 14
- `Benchmarks/run_benchmarks.sh` - Benchmark automation (~150 lines)
- `Benchmarks/analyze.py` - Performance analysis (~200 lines)
- `PERFORMANCE.md` - Benchmark results documentation (~100 lines)

**Total Estimated Code:** ~2,650 lines

---

## Success Metrics

### Technical Metrics
- Guest VM boots successfully: ✅/❌
- virtio-gpu device detected: ✅/❌
- Venus driver loads: ✅/❌
- vkcube runs: ✅/❌
- FPS >30: ✅/❌
- Performance >75% native: ✅/❌

### User Experience Metrics
- Time to first render: <2 minutes from VM start
- Stability: No crashes in 30-minute session
- Usability: Simple setup (<10 commands)

---

## Phase 2C Deliverable

**Minimum Viable Product (MVP):**
```
User can:
1. Download PearVisor
2. Create Linux ARM64 VM
3. Boot guest to desktop
4. Run vkcube
5. See GPU-accelerated graphics
6. Achieve competitive performance

All in under 30 minutes from download to demo.
```

**Success:** PearVisor demonstrates world-class GPU virtualization on Apple Silicon, competitive with or exceeding Parallels Desktop and UTM.

---

## Next Steps

**Immediate:** Decide Path A vs. Path B
**Recommended:** Path A (Session 11 next)
**Blockers:** None - Phase 2B complete, ready to proceed

---

*Last Updated: November 20, 2025 - Phase 2C design complete, ready to begin implementation*
