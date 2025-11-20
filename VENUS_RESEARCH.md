# Venus Protocol Research

**Date:** 2025-11-19  
**Purpose:** Understanding Venus protocol for Phase 2 integration  
**Source:** virglrenderer submodule analysis  

---

## 🔬 Venus Protocol Overview

### What is Venus?

Venus is a **Vulkan virtualization protocol** developed by Google for ChromeOS crosvm:
- Serializes Vulkan API calls into a compact binary format
- Transmits commands over virtio-gpu shared memory
- Decodes and executes commands on host GPU
- Returns results back to guest

**Key Advantages:**
1. **Standard Protocol:** Used in production (ChromeOS, Android VMs)
2. **Complete Implementation:** virglrenderer already has full Venus support
3. **Well-Tested:** Mature codebase with extensive testing
4. **High Performance:** Designed for low overhead

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│ Guest VM (Linux ARM64)                                      │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │ Application (vkcube, game, etc.)                   │    │
│  └───────────────────────┬────────────────────────────┘    │
│                          │ Vulkan API calls                 │
│                          ▼                                  │
│  ┌────────────────────────────────────────────────────┐    │
│  │ Mesa Venus Vulkan Driver (virtio_gpu.ko)           │    │
│  │  - Serializes Vulkan calls                         │    │
│  │  - Manages Venus protocol encoding                 │    │
│  └───────────────────────┬────────────────────────────┘    │
│                          │ Venus commands                   │
│                          ▼                                  │
│  ┌────────────────────────────────────────────────────┐    │
│  │ virtio-gpu Device Interface                        │    │
│  │  - Ring buffer for command submission              │    │
│  │  - Shared memory for data                          │    │
│  └───────────────────────┬────────────────────────────┘    │
└──────────────────────────┼──────────────────────────────────┘
                           │ virtio-gpu transport
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ PearVisor Host (macOS)                                      │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │ virtio-gpu Device (pv_virtio_gpu.c)                │    │
│  │  - Receives Venus commands from ring buffer        │    │
│  │  - Manages shared memory                           │    │
│  └───────────────────────┬────────────────────────────┘    │
│                          │ Raw command data                 │
│                          ▼                                  │
│  ┌────────────────────────────────────────────────────┐    │
│  │ virglrenderer Venus Decoder                        │    │
│  │  - Parses Venus protocol                           │    │
│  │  - Decodes Vulkan commands                         │    │
│  └───────────────────────┬────────────────────────────┘    │
│                          │ Vulkan API calls                 │
│                          ▼                                  │
│  ┌────────────────────────────────────────────────────┐    │
│  │ MoltenVK (Vulkan → Metal translation)              │    │
│  │  - Implements Vulkan API                           │    │
│  │  - Translates to Metal commands                    │    │
│  └───────────────────────┬────────────────────────────┘    │
│                          │ Metal API calls                  │
│                          ▼                                  │
│  ┌────────────────────────────────────────────────────┐    │
│  │ Metal Framework                                    │    │
│  │  - Native macOS GPU API                            │    │
│  └───────────────────────┬────────────────────────────┘    │
└──────────────────────────┼──────────────────────────────────┘
                           ▼
                    Apple M1 Max GPU
```

---

## 📚 virglrenderer API Research

### Initialization API

```c
// From virglrenderer.h

#define VIRGL_RENDERER_VENUS         (1 << 6)  // Enable Venus
#define VIRGL_RENDERER_NO_VIRGL      (1 << 7)  // Disable virgl
#define VIRGL_RENDERER_RENDER_SERVER (1 << 9)  // Use render server

struct virgl_renderer_callbacks {
   int version;
   void (*write_fence)(void *cookie, uint32_t fence);
   virgl_renderer_gl_context (*create_gl_context)(...);
   void (*destroy_gl_context)(...);
   int (*make_current)(...);
   int (*get_drm_fd)(void *cookie);
   void (*write_context_fence)(void *cookie, uint32_t ctx_id, ...);
   int (*get_server_fd)(void *cookie, uint32_t version);
   void *(*get_egl_display)(void *cookie);
};

// Initialize virglrenderer
int virgl_renderer_init(void *cookie, int flags, 
                       struct virgl_renderer_callbacks *cb);
```

**For PearVisor:**
- Use `VIRGL_RENDERER_VENUS | VIRGL_RENDERER_NO_VIRGL` flags
- Implement callbacks for fence signaling
- Provide Metal-backed Vulkan context via MoltenVK

### Context API

```c
// Create Venus context
int virgl_renderer_context_create(uint32_t ctx_id, uint32_t nlen,
                                  const char *name);

// Destroy context
void virgl_renderer_context_destroy(uint32_t ctx_id);
```

### Resource API

```c
struct virgl_renderer_resource_create_args {
   uint32_t handle;
   uint32_t target;
   uint32_t format;
   uint32_t bind;
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t array_size;
   uint32_t last_level;
   uint32_t nr_samples;
   uint32_t flags;
};

// Create resource (texture, buffer, etc.)
int virgl_renderer_resource_create(
   struct virgl_renderer_resource_create_args *args,
   struct iovec *iov, uint32_t num_iovs);

// Destroy resource
void virgl_renderer_resource_unref(uint32_t res_handle);
```

### Command Submission API

```c
// Submit commands to context
int virgl_renderer_submit_cmd(void *buffer, uint32_t ctx_id, int ndw);

// Execute commands
int virgl_renderer_context_destroy(uint32_t ctx_id);

// Poll for fence completion
void virgl_renderer_poll(void);
```

---

## 🏗️ Venus Protocol Format

### Command Structure

From `src/venus/vkr_cs.h`:

```c
struct vkr_cs_encoder {
   void *buffer;           // Output buffer
   size_t size;            // Buffer size
   size_t offset;          // Current offset
   uint32_t error;         // Error state
};

struct vkr_cs_decoder {
   const void *buffer;     // Input buffer
   size_t size;            // Buffer size
   size_t offset;          // Current offset
};
```

### Venus Command Format

Venus commands are encoded as:
```c
struct venus_command {
   uint32_t size;          // Total command size in bytes
   uint32_t type;          // VK_COMMAND_* type
   uint8_t data[];         // Serialized parameters
};
```

**Example: vkCreateDevice**
```c
// Guest encodes:
{
   .size = sizeof(cmd),
   .type = VK_COMMAND_CREATE_DEVICE,
   .data = {
      .physicalDevice = handle,
      .pCreateInfo = serialized_create_info,
      .pAllocator = NULL,
      .pDevice = output_handle_ptr
   }
}

// Host decodes and executes:
VkResult vkCreateDevice(
   physicalDevice,
   pCreateInfo,
   pAllocator,
   pDevice
);

// Returns result via fence
```

---

## 🔧 Venus Integration Points

### 1. Ring Buffer (virtio-gpu)

**Current PearVisor Implementation:**
- `pv_virtio_gpu.c` has basic device structure
- 16MB shared memory allocated
- pthread command processing thread
- Mutex for synchronization

**Need to Add:**
- Ring buffer structure (head/tail pointers)
- Command submission interface
- Fence mechanism for completion
- Multiple ring support (per-context)

**Data Structures:**
```c
struct pv_venus_ring {
   // Ring buffer metadata
   void* buffer;               // Ring buffer memory
   size_t size;                // 1MB default
   volatile uint32_t head;     // Host read position
   volatile uint32_t tail;     // Guest write position
   
   // Synchronization
   pthread_mutex_t mutex;
   pthread_cond_t cond;
   
   // Context
   uint32_t ctx_id;
   void* virgl_ctx;
};
```

### 2. virglrenderer Integration

**Files to Create:**
- `GPU/src/pv_virgl.c` - virglrenderer bridge
- `GPU/include/pv_virgl.h` - Public API

**Key Functions:**
```c
// Initialize virglrenderer with Venus
pv_gpu_error_t pv_virgl_init(void);

// Create Venus context
pv_gpu_error_t pv_virgl_create_context(uint32_t ctx_id);

// Submit Venus commands
pv_gpu_error_t pv_virgl_submit_commands(
   uint32_t ctx_id,
   void* cmd_buffer,
   size_t cmd_size
);

// Process fences
void pv_virgl_poll_fences(void);

// Cleanup
void pv_virgl_shutdown(void);
```

### 3. MoltenVK Bridge

**Files to Create:**
- `GPU/src/pv_moltenvk.mm` - MoltenVK interface
- `GPU/include/pv_moltenvk.h` - Public API

**Key Functions:**
```objc
// Initialize MoltenVK Vulkan instance
pv_gpu_error_t pv_moltenvk_init(void);

// Get Vulkan instance for virglrenderer
VkInstance pv_moltenvk_get_instance(void);

// Get physical device
VkPhysicalDevice pv_moltenvk_get_physical_device(void);

// Create logical device
VkDevice pv_moltenvk_create_device(VkDeviceCreateInfo* info);
```

---

## 📊 Performance Expectations

### Overhead Analysis

**Phase 1 Baseline:** 110% native performance

**Expected Phase 2 Overhead:**
```
Component                        Overhead    Notes
─────────────────────────────────────────────────────────────
Venus Serialization              ~5%         Command encoding
Ring Buffer Sync                 ~3%         Mutex/condition vars
virglrenderer Decode             ~7%         Command parsing
Vulkan API Calls                 ~5%         Function call overhead
MoltenVK Translation             ~10%        Vulkan → Metal
─────────────────────────────────────────────────────────────
Total Expected Overhead          ~30%
Expected Performance             80%         ✓ MEETS TARGET
```

**With Optimizations:**
- Zero-copy command buffers: +5%
- Command batching: +3%
- Hot path inlining: +2%
- SIMD optimizations: +2%

**Optimized Target:** 90-95% native performance

### Latency Targets

| Operation | Target | Acceptable |
|-----------|--------|------------|
| Command submission | <100 μs | <200 μs |
| Command decode | <200 μs | <400 μs |
| Fence signaling | <50 μs | <100 μs |
| Total round-trip | <500 μs | <1000 μs |

---

## 🎯 Integration Strategy

### Phase 2A: virglrenderer Foundation (Sessions 4-5)

**Goals:**
1. Build virglrenderer with Venus support
2. Link with PearVisor
3. Test basic initialization
4. Create Venus context

**Success Criteria:**
- virglrenderer builds on macOS ARM64
- Venus context creates successfully
- No crashes or memory leaks

### Phase 2B: Ring Buffer Implementation (Sessions 6-7)

**Goals:**
1. Implement ring buffer in virtio-gpu
2. Add command submission interface
3. Connect to virglrenderer
4. Test simple commands

**Success Criteria:**
- Commands flow from ring → virglrenderer
- Fences signal correctly
- Basic Vulkan calls work

### Phase 2C: MoltenVK Integration (Sessions 8-9)

**Goals:**
1. Initialize MoltenVK Vulkan instance
2. Connect virglrenderer to MoltenVK
3. Test full Vulkan pipeline
4. Run vkcube in guest

**Success Criteria:**
- Guest enumerates Vulkan device
- vkcube renders correctly
- Performance ≥80% native

### Phase 2D: Optimization (Session 10)

**Goals:**
1. Profile hot paths
2. Optimize command submission
3. Reduce latency
4. Target 85-95% efficiency

**Success Criteria:**
- Benchmarks show ≥85% efficiency
- vkcube runs at ≥80% native FPS
- Memory usage <100 MB overhead

---

## 🚧 Known Challenges

### 1. Building virglrenderer on macOS

**Issue:** virglrenderer uses meson build system, requires dependencies

**Solution:**
```bash
brew install meson ninja pkg-config
brew install libepoxy # For GL/EGL
meson setup build -Dvenus=true
ninja -C build
```

**Workaround:** May need to disable some features for macOS

### 2. MoltenVK Vulkan Version

**Issue:** MoltenVK may not support all Vulkan 1.3 features

**Solution:**
- Document supported features
- Gracefully fail on unsupported calls
- Focus on Vulkan 1.1/1.2 compatibility

### 3. Shared Memory Model

**Issue:** Linux guest expects dma-buf, macOS uses different mechanism

**Solution:**
- Use regular shared memory for now
- Implement proper memory import/export later
- Document limitations

### 4. Thread Synchronization

**Issue:** Multiple threads accessing ring buffer

**Solution:**
- Careful mutex usage
- Condition variables for blocking
- Lock-free ring buffer if needed

---

## 📁 Required Files (Phase 2)

### New Source Files

```
GPU/
├── src/
│   ├── pv_virgl.c           # virglrenderer integration
│   ├── pv_venus.c           # Venus protocol helpers
│   ├── pv_moltenvk.mm       # MoltenVK bridge
│   └── pv_ring_buffer.c     # Ring buffer implementation
├── include/
│   ├── pv_virgl.h           # virglrenderer API
│   ├── pv_venus.h           # Venus protocol API
│   ├── pv_moltenvk.h        # MoltenVK API
│   └── pv_ring_buffer.h     # Ring buffer API
└── build.sh                 # Updated build script
```

### Modified Files

```
GPU/
├── src/
│   └── pv_virtio_gpu.c      # Add ring buffer support
└── include/
    └── pv_gpu.h             # Extend API
```

### Test Files

```
Sources/PearVisor/
├── VenusTest.swift          # Venus protocol test
└── GPUBenchmark.swift       # Extend for Venus
```

---

## 🎯 Next Steps

### Immediate (Next Session)

1. **Research meson build for virglrenderer**
   - Check meson requirements
   - List dependencies
   - Plan macOS-specific patches

2. **Create virglrenderer build script**
   - Write GPU/build-virglrenderer.sh
   - Test build on macOS ARM64
   - Fix any build issues

3. **Create pv_virgl skeleton**
   - Basic initialization
   - Callback stubs
   - Error handling

4. **Test virglrenderer init**
   - Link with PearVisor
   - Call virgl_renderer_init()
   - Verify no crashes

### Short Term (Sessions 5-7)

- Implement ring buffer
- Connect to virglrenderer
- Add MoltenVK integration
- Test with simple commands

### Long Term (Sessions 8-10)

- Guest driver setup
- vkcube testing
- Performance optimization
- Documentation

---

## 📝 Conclusions

### Key Findings

1. **virglrenderer is well-structured**
   - Clean API
   - Comprehensive Venus support
   - Production-tested codebase

2. **Integration is straightforward**
   - Clear callback model
   - Standard virtio-gpu interface
   - Documented protocol

3. **MoltenVK is the right choice**
   - Official Vulkan → Metal translator
   - Used in production (Dota 2, etc.)
   - Good performance characteristics

4. **Performance target is achievable**
   - Starting from 110% baseline
   - 30% overhead budget
   - Expected 80%+ final efficiency

### Confidence Level

**High (90%+)** that Phase 2 will succeed:
- ✓ All components exist and are proven
- ✓ Clear integration path
- ✓ Strong Phase 1 foundation
- ✓ Sufficient performance margin

### Risks

**Low Risk:**
- Building virglrenderer (may need patches)
- Ring buffer implementation (known patterns)

**Medium Risk:**
- MoltenVK compatibility (may lack features)
- Guest driver configuration (version deps)

**High Risk:**
- None identified

---

**Status:** Research complete, ready to begin implementation  
**Next:** Create virglrenderer build script  
**Timeline:** On track for 5-10 session Phase 2  
**Confidence:** High  
