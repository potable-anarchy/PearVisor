# GPU Passthrough Implementation Plan

**Status:** In Progress  
**Branch:** feature/gpu-passthrough  
**Goal:** 80% native Metal performance

---

## Problem: Building MoltenVK Requires Xcode

MoltenVK build requires full Xcode, not just command line tools.

### Solution: Use Pre-built MoltenVK

Instead of building from source, we'll:
1. Download pre-built MoltenVK from Vulkan SDK
2. Link against pre-built libraries
3. Focus on our custom GPU passthrough code

---

## Revised Architecture

### Phase 1: Simplified Stack (Achievable Now)
```
Guest Linux
    ↓
virtio-gpu (our custom device)
    ↓
Shared memory buffer
    ↓
Simple Metal renderer (direct, no Venus/virglrenderer)
    ↓
Metal API
```

**Why:** Simpler, faster to implement, proves concept

### Phase 2: Full Venus Stack (Later)
```
Guest Linux (Mesa Venus driver)
    ↓
virtio-gpu (Venus protocol)
    ↓
virglrenderer (Venus handler)
    ↓
MoltenVK (Vulkan → Metal)
    ↓
Metal API
```

**Why:** Full compatibility, better performance, standard approach

---

## Implementation Plan

### Step 1: Simple virtio-gpu Device (C)
**File:** `GPU/src/pv_virtio_gpu.c`

```c
// Basic virtio-gpu device
// - Allocate shared memory
// - Handle guest commands
// - Pass to Metal renderer
```

### Step 2: Metal Renderer (Objective-C++)
**File:** `GPU/src/pv_metal_renderer.mm`

```objc
// Simple Metal renderer
// - Receive draw commands from virtio-gpu
// - Execute on Metal device
// - Display results
```

### Step 3: Swift Bridge
**File:** `Sources/PearVisorCore/GPUPassthrough.swift`

```swift
// Bridge C GPU device to Swift VM
// - Attach GPU to VM
// - Monitor performance
// - Handle errors
```

### Step 4: Test & Benchmark
- Boot VM with GPU attached
- Run `glxgears` in guest
- Measure FPS vs native
- Target: 60+ FPS (80% of native)

---

## What We'll Build First

### Minimal Viable GPU (Phase 1)

**Goal:** Get SOMETHING rendering on screen

1. **virtio-gpu device** that accepts basic commands
2. **Simple Metal renderer** that draws to a texture
3. **Display** texture in host window
4. **Test** with simple OpenGL app in guest

**Not worrying about:**
- Full Vulkan compatibility
- Venus protocol
- virglrenderer
- Optimal performance

**Just proving:**
- We can pass GPU commands from guest to host
- We can render with Metal
- It's fast enough to be useful

---

## Code to Write (This Session)

### 1. pv_virtio_gpu.c (~200 lines)
- Device initialization
- Command queue
- Shared memory setup
- Basic command handling

### 2. pv_metal_renderer.mm (~150 lines)
- Metal device initialization
- Command buffer creation
- Simple triangle rendering (test)
- Texture output

### 3. GPUPassthrough.swift (~100 lines)
- Attach GPU to VM
- Forward commands to C layer
- Display Metal texture in window

### 4. Test app (~50 lines)
- Render triangle in guest
- Verify it appears on host

---

## Success Metrics

### Phase 1 (This Session)
- [ ] virtio-gpu device initializes
- [ ] Can send commands from "guest" (test harness)
- [ ] Metal renders something on host
- [ ] Proof of concept works

### Phase 2 (Next Session)
- [ ] Real VM integration
- [ ] Guest Linux with Mesa drivers
- [ ] Basic OpenGL app works
- [ ] 30+ FPS in glxgears

### Phase 3 (Future)
- [ ] Venus protocol integration
- [ ] virglrenderer port
- [ ] MoltenVK integration
- [ ] 80%+ native performance

---

## Let's Build Phase 1 NOW

Focus: Get something working, even if it's simple.

**Next:** Implement basic virtio-gpu device in C
