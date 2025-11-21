/*
 * pv_venus_integration.c
 * Venus protocol integration implementation
 * 
 * Bridges Swift Virtualization.framework with C Venus subsystem
 */

#include "pv_venus_integration.h"
#include "pv_venus_ring.h"
#include "pv_venus_decoder.h"
#include "pv_venus_handlers.h"
#include "pv_moltenvk.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Create ring buffer from existing memory */
struct pv_venus_ring* pv_venus_ring_create_from_memory(void *memory, uint32_t size) {
    if (!memory) {
        fprintf(stderr, "[Venus Integration] NULL memory pointer\n");
        return NULL;
    }
    
    if (size == 0 || (size & (size - 1)) != 0) {
        fprintf(stderr, "[Venus Integration] Size must be power of 2, got %u\n", size);
        return NULL;
    }
    
    printf("[Venus Integration] Creating ring buffer from memory: %p, size: %u bytes\n", 
           memory, size);
    
    /* Create ring buffer layout */
    /* Layout: [head(4)][tail(4)][status(4)][padding(4)][buffer(rest)] */
    struct pv_venus_ring_layout layout = {
        .shared_memory = memory,
        .shared_memory_size = size,
        .head_offset = 0,
        .tail_offset = 4,
        .status_offset = 8,
        .buffer_offset = 16,  /* 16-byte aligned */
        .buffer_size = size - 16,
        .extra_offset = 0,
        .extra_size = 0
    };
    
    /* Create ring using existing API with NULL dispatch context (will set later) */
    struct pv_venus_ring *ring = pv_venus_ring_create(&layout, NULL);
    if (!ring) {
        fprintf(stderr, "[Venus Integration] Failed to create ring buffer\n");
        return NULL;
    }
    
    printf("[Venus Integration] Ring buffer created successfully\n");
    printf("[Venus Integration]   Buffer region: %u bytes at offset %zu\n", 
           (unsigned int)layout.buffer_size, layout.buffer_offset);
    
    return ring;
}

/* Note: pv_venus_ring_destroy is implemented in pv_venus_ring.c */

/* Start ring buffer processing with dispatch context */
int pv_venus_integration_start(struct pv_venus_ring *ring, void *context) {
    if (!ring || !context) {
        fprintf(stderr, "[Venus Integration] NULL ring or context\n");
        return -1;
    }
    
    printf("[Venus Integration] Starting ring buffer processing (polling mode)\n");
    
    /* Set the dispatch context */
    ring->dispatch_context = context;
    ring->running = true;
    
    /* For integration, we use polling instead of background thread */
    /* Commands will be processed when pv_venus_ring_notify() is called */
    
    printf("[Venus Integration] Ring buffer ready to process commands\n");
    return 0;
}

/* Stop ring buffer processing */
void pv_venus_integration_stop(struct pv_venus_ring *ring) {
    if (!ring) {
        return;
    }
    
    printf("[Venus Integration] Stopping ring buffer processing\n");
    
    ring->running = false;
    ring->dispatch_context = NULL;
    
    printf("[Venus Integration] Ring buffer processing stopped\n");
}

/* Note: pv_venus_ring_notify is implemented in pv_venus_ring.c */

/* Get ring buffer utilization */
double pv_venus_ring_utilization(struct pv_venus_ring *ring) {
    if (!ring || ring->buffer.size == 0) {
        return 0.0;
    }
    
    uint32_t head = ring->buffer.current_pos;
    uint32_t tail = pv_venus_ring_get_tail(ring);
    uint32_t used = (tail - head) & ring->buffer.mask;
    
    return (double)used / (double)ring->buffer.size;
}

/* Initialize Venus handler context */
void* pv_venus_init(void) {
    printf("[Venus Integration] Initializing Venus handler context\n");
    
    /* Initialize MoltenVK (returns allocated context) */
    struct pv_moltenvk_context *vk = pv_moltenvk_init();
    if (!vk) {
        fprintf(stderr, "[Venus Integration] MoltenVK initialization failed\n");
        return NULL;
    }
    
    printf("[Venus Integration] MoltenVK initialized\n");
    
    /* Create Venus dispatch context */
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    if (!dispatch_ctx) {
        fprintf(stderr, "[Venus Integration] Failed to create dispatch context\n");
        pv_moltenvk_cleanup(vk);
        return NULL;
    }
    
    printf("[Venus Integration] Dispatch context created\n");
    
    /* Create Venus handler context and attach MoltenVK */
    struct pv_venus_handler_context *ctx = malloc(sizeof(struct pv_venus_handler_context));
    if (!ctx) {
        fprintf(stderr, "[Venus Integration] Failed to allocate handler context\n");
        pv_venus_dispatch_destroy(dispatch_ctx);
        pv_moltenvk_cleanup(vk);
        return NULL;
    }
    
    memset(ctx, 0, sizeof(*ctx));
    ctx->vk = vk;
    dispatch_ctx->user_context = ctx;
    
    /* Object table will be initialized by handlers */
    /* No separate init function needed - handlers initialize on first use */
    
    printf("[Venus Integration] Handler context initialized\n");
    
    /* Register all Venus command handlers */
    pv_venus_handlers_register(dispatch_ctx, ctx);
    
    printf("[Venus Integration] Venus context initialized successfully\n");
    printf("[Venus Integration] Ready to process GPU commands\n");
    
    return dispatch_ctx;
}

/* Cleanup Venus handler context */
void pv_venus_cleanup(void *context) {
    if (!context) {
        return;
    }
    
    printf("[Venus Integration] Cleaning up Venus context\n");
    
    struct pv_venus_dispatch_context *dispatch_ctx = 
        (struct pv_venus_dispatch_context *)context;
    
    if (dispatch_ctx->user_context) {
        struct pv_venus_handler_context *handler_ctx = 
            (struct pv_venus_handler_context *)dispatch_ctx->user_context;
        
        /* Cleanup MoltenVK */
        if (handler_ctx->vk) {
            pv_moltenvk_cleanup(handler_ctx->vk);
        }
        
        free(handler_ctx);
    }
    
    /* Destroy dispatch context */
    pv_venus_dispatch_destroy(dispatch_ctx);
    
    printf("[Venus Integration] Venus context cleaned up\n");
}

/* Get Venus statistics */
struct pv_venus_stats pv_venus_get_stats(void *context) {
    struct pv_venus_stats stats = {0};
    
    if (!context) {
        return stats;
    }
    
    struct pv_venus_dispatch_context *dispatch_ctx = 
        (struct pv_venus_dispatch_context *)context;
    
    if (dispatch_ctx->user_context) {
        struct pv_venus_handler_context *handler_ctx = 
            (struct pv_venus_handler_context *)dispatch_ctx->user_context;
        
        stats.commands_handled = handler_ctx->commands_handled;
        stats.objects_created = handler_ctx->objects_created;
        stats.errors = 0;  /* TODO: Track errors */
    }
    
    return stats;
}
