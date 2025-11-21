# Venus Protocol Blocker - Render Server Dependency

## Issue Summary

**Date:** November 20, 2025  
**Phase:** 2A - virglrenderer Foundation  
**Status:** BLOCKED - Architectural Issue

## Problem

virglrenderer's Venus protocol support requires a render server architecture that cannot be built on macOS due to Linux-specific dependencies.

## Technical Details

### What We Discovered

1. **Venus Requires Render Server**
   - The `VIRGL_RENDERER_VENUS` flag is defined but never actually used in the code
   - Venus support is actually activated via `VIRGL_RENDERER_RENDER_SERVER` flag
   - The proxy renderer (Venus) initialization happens in `proxy_renderer_init()` only when `VIRGL_RENDERER_RENDER_SERVER` is set

2. **Render Server Architecture**
   ```
   virgl_renderer_init(VIRGL_RENDERER_RENDER_SERVER)
   └─> proxy_renderer_init()
       └─> proxy_server_init_fd()
           └─> Requires get_server_fd() callback
               └─> Returns connection to render server process
   ```

3. **macOS Incompatibility**
   - Render server code (`server/render_worker.c`) uses Linux-specific headers:
     - `sys/signalfd.h` - Linux signal handling
     - `sys/eventfd.h` - Linux event notification
     - `clock_nanosleep()` - POSIX realtime extension
   - We successfully patched the core library files for macOS
   - We disabled server build to get virglrenderer library to compile
   - **But** Venus requires the render server to function

### Build Status

✅ Successfully built:
- Core virglrenderer library (libvirglrenderer.1.dylib)
- macOS compatibility patches for library code
- Basic virgl API integration in PearVisor

❌ Could not build:
- Render server executable (Linux dependencies)
- Venus protocol support (requires render server)

### Test Results

Test hung during `virgl_renderer_init()` with Venus flags because:
1. `VIRGL_RENDERER_RENDER_SERVER` flag triggers `proxy_renderer_init()`
2. Proxy renderer expects `get_server_fd()` callback
3. Without render server process, there's no server FD to provide
4. Initialization blocks waiting for server connection

## Options Forward

### Option 1: Direct MoltenVK Integration (RECOMMENDED)

Skip virglrenderer entirely and implement Venus protocol directly:

**Pros:**
- No dependency on Linux-specific code
- Direct Metal → Vulkan via MoltenVK
- Full control over implementation
- Potentially better performance (fewer layers)

**Cons:**
- More complex implementation
- Need to implement Venus protocol parser ourselves
- More code to maintain

**Effort:** 3-4 sessions

### Option 2: Port Render Server to macOS

Rewrite render server components for macOS compatibility:

**Pros:**
- Keep virglrenderer as-is
- Leverage existing Venus implementation

**Cons:**
- Significant porting effort (signal handling, event mechanisms)
- Still requires process architecture (complexity)
- May hit more Linux dependencies
- Upstream virglrenderer unlikely to accept macOS server patches

**Effort:** 5-7 sessions (high risk of more blockers)

### Option 3: In-Process Proxy Renderer

Modify virglrenderer to run proxy renderer in-process instead of separate server:

**Pros:**
- Avoids process management complexity
- Keeps Venus protocol implementation

**Cons:**
- Requires forking virglrenderer
- Deep architectural changes to proxy renderer
- Thread-safety concerns
- Maintenance burden

**Effort:** 4-6 sessions

### Option 4: Hybrid Approach

Use virglrenderer for virtio-gpu and implement custom Venus handler:

**Pros:**
- Leverage virglrenderer's virtio-gpu handling
- Custom Venus implementation optimized for macOS
- Best of both worlds

**Cons:**
- Two major components to integrate
- Higher initial complexity

**Effort:** 4-5 sessions

## Recommendation

**Proceed with Option 1: Direct MoltenVK Integration**

**Rationale:**
1. **Clean Architecture** - No dependency on Linux-specific code paths
2. **Performance** - Fewer layers between guest and Metal
3. **Maintainability** - We control the entire stack
4. **Learning** - Better understanding of Venus protocol
5. **Alignment** - Matches PearVisor's goal of macOS-native implementation

## Next Steps (Option 1 Path)

### Phase 2B: Direct Venus Protocol Implementation

1. **Venus Protocol Parser** (1 session)
   - Parse Venus ring buffer commands
   - Implement Venus virtio-gpu command handling
   - Create command dispatch framework

2. **Vulkan Command Translation** (1-2 sessions)
   - Map Venus commands to Vulkan API calls
   - Implement resource management
   - Handle synchronization primitives

3. **MoltenVK Integration** (1 session)
   - Initialize MoltenVK Vulkan instance
   - Create logical devices
   - Setup command submission

4. **Testing & Validation** (1 session)
   - Unit tests for Venus parser
   - Integration tests with MoltenVK
   - Basic Vulkan triangle test

**Total Effort:** 4-5 sessions vs 5-7 sessions for render server port

## Files Created This Session

✅ Successfully created:
- `GPU/src/pv_virgl.c` - virglrenderer integration (won't be used)
- `GPU/include/pv_virgl.h` - API header (reference for future)
- `GPU/src/test_virgl.c` - Test program (documented the blocker)
- `GPU/build-virglrenderer.sh` - Build script (keep for reference)
- Submodules/virglrenderer with macOS patches (library still useful for docs)

## Performance Impact

Original target: 80-95% native performance with Venus

With direct implementation:
- **Best case:** 85-100% (fewer layers)
- **Realistic:** 80-95% (same as Venus)
- **Worst case:** 75-90% (if we're less efficient than virglrenderer)

Direct implementation gives us more control over performance optimization.

## References

- virglrenderer source: `Submodules/virglrenderer/`
- Proxy renderer code: `src/proxy/proxy_renderer.c`
- Render server: `server/render_worker.c` (Linux-only)
- Venus protocol: https://gitlab.freedesktop.org/virgl/virglrenderer/-/tree/main/src/venus

## Decision

**PIVOT to Direct Venus Implementation**

Update PHASE_2_PLAN.md to reflect new approach.
