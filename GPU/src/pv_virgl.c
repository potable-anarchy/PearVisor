/*
 * PearVisor - virglrenderer Integration
 * 
 * This file integrates virglrenderer with Venus protocol support
 * to provide Vulkan virtualization for guest VMs.
 * 
 * Architecture:
 *   Guest VM -> virtio-gpu -> virglrenderer (Venus) -> MoltenVK -> Metal
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <virgl/virglrenderer.h>
#include "pv_virgl.h"

/* Global state */
static struct {
    bool initialized;
    void *cookie;
    struct virgl_renderer_callbacks callbacks;
} g_virgl_state = {0};

/*
 * Callback: write_fence
 * Called when a fence completes on the render timeline
 */
static void pv_virgl_write_fence(void *cookie, uint32_t fence)
{
    printf("[PearVisor/virgl] Fence completed: %u\n", fence);
    /* TODO: Signal guest that fence has completed */
}

/*
 * Callback: write_context_fence  
 * Called when a per-context fence completes (Venus uses this)
 */
static void pv_virgl_write_context_fence(void *cookie, uint32_t ctx_id, 
                                         uint32_t ring_idx, uint64_t fence_id)
{
    printf("[PearVisor/virgl] Context fence: ctx=%u ring=%u fence=%llu\n",
           ctx_id, ring_idx, fence_id);
    /* TODO: Signal guest that context fence has completed */
}

/*
 * Initialize virglrenderer with Venus support
 */
int pv_virgl_init(void)
{
    if (g_virgl_state.initialized) {
        fprintf(stderr, "[PearVisor/virgl] Already initialized\n");
        return -1;
    }

    printf("[PearVisor/virgl] Initializing virglrenderer with Venus support...\n");

    /* Setup callbacks */
    g_virgl_state.callbacks.version = VIRGL_RENDERER_CALLBACKS_VERSION;
    g_virgl_state.callbacks.write_fence = pv_virgl_write_fence;
    g_virgl_state.callbacks.write_context_fence = pv_virgl_write_context_fence;

    /* Initialize virglrenderer with Venus flag */
    uint32_t flags = VIRGL_RENDERER_VENUS | VIRGL_RENDERER_THREAD_SYNC;
    int ret = virgl_renderer_init(&g_virgl_state.cookie, flags, 
                                   &g_virgl_state.callbacks);
    
    if (ret != 0) {
        fprintf(stderr, "[PearVisor/virgl] Failed to initialize: %d\n", ret);
        return ret;
    }

    g_virgl_state.initialized = true;
    printf("[PearVisor/virgl] Successfully initialized with Venus support\n");

    /* Query capabilities to verify Venus is available */
    uint32_t capset_id = 4; /* VIRTGPU_DRM_CAPSET_VENUS */
    uint32_t version;
    uint32_t size;
    virgl_renderer_get_cap_set(capset_id, &version, &size);
    
    printf("[PearVisor/virgl] Venus capset: version=%u size=%u\n", version, size);

    return 0;
}

/*
 * Create a Venus rendering context
 */
int pv_virgl_create_venus_context(uint32_t ctx_id)
{
    if (!g_virgl_state.initialized) {
        fprintf(stderr, "[PearVisor/virgl] Not initialized\n");
        return -1;
    }

    printf("[PearVisor/virgl] Creating Venus context: %u\n", ctx_id);

    /* VIRGL_RENDERER_CONTEXT_FLAG_CAPSET_ID_MASK allows specifying capset */
    uint32_t capset_id = 4; /* VIRTGPU_DRM_CAPSET_VENUS */
    uint32_t flags = capset_id & VIRGL_RENDERER_CONTEXT_FLAG_CAPSET_ID_MASK;
    const char *name = "PearVisor-Venus";
    
    int ret = virgl_renderer_context_create_with_flags(ctx_id, 
                                                        flags,
                                                        strlen(name),
                                                        name);
    
    if (ret != 0) {
        fprintf(stderr, "[PearVisor/virgl] Failed to create context: %d\n", ret);
        return ret;
    }

    printf("[PearVisor/virgl] Venus context %u created successfully\n", ctx_id);
    return 0;
}

/*
 * Destroy a Venus context
 */
void pv_virgl_destroy_context(uint32_t ctx_id)
{
    if (!g_virgl_state.initialized) {
        return;
    }

    printf("[PearVisor/virgl] Destroying context: %u\n", ctx_id);
    virgl_renderer_context_destroy(ctx_id);
}

/*
 * Cleanup virglrenderer
 */
void pv_virgl_cleanup(void)
{
    if (!g_virgl_state.initialized) {
        return;
    }

    printf("[PearVisor/virgl] Cleaning up virglrenderer...\n");
    virgl_renderer_cleanup(&g_virgl_state.cookie);
    
    memset(&g_virgl_state, 0, sizeof(g_virgl_state));
    printf("[PearVisor/virgl] Cleanup complete\n");
}

/*
 * Simple test function to verify basic functionality
 */
int pv_virgl_test(void)
{
    printf("\n=== PearVisor virglrenderer Venus Test ===\n");

    /* Initialize */
    if (pv_virgl_init() != 0) {
        fprintf(stderr, "Initialization failed\n");
        return -1;
    }

    /* Create a test context */
    uint32_t test_ctx = 1;
    if (pv_virgl_create_venus_context(test_ctx) != 0) {
        fprintf(stderr, "Context creation failed\n");
        pv_virgl_cleanup();
        return -1;
    }

    /* Destroy context */
    pv_virgl_destroy_context(test_ctx);

    /* Cleanup */
    pv_virgl_cleanup();

    printf("=== Test Complete ===\n\n");
    return 0;
}
