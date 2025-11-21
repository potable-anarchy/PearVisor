/*
 * pv_venus_integration.h
 * Venus protocol integration API for Swift interop
 * 
 * Exposes Venus subsystem to Virtualization.framework
 */

#ifndef PV_VENUS_INTEGRATION_H
#define PV_VENUS_INTEGRATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct pv_venus_ring;
struct pv_venus_handler_context;

/* Venus statistics for Swift */
typedef struct pv_venus_stats {
    uint32_t commands_handled;
    uint32_t objects_created;
    uint32_t errors;
    uint32_t _padding;  /* Align to 16 bytes */
} pv_venus_stats;

/*
 * Create Venus ring buffer from existing memory region
 * Used by Swift to create ring in VM shared memory
 * 
 * @param memory Pointer to shared memory region (page-aligned)
 * @param size Size of memory region in bytes (power of 2)
 * @return Ring buffer handle or NULL on failure
 */
struct pv_venus_ring* pv_venus_ring_create_from_memory(void *memory, uint32_t size);

/* Note: pv_venus_ring_destroy declared in pv_venus_ring.h */
/* Note: pv_venus_ring_notify declared in pv_venus_ring.h */
/* Note: pv_venus_ring_utilization declared below (integration-specific) */

/*
 * Start ring buffer processing with dispatch context
 * Integration-specific function that sets up polling mode
 * 
 * @param ring Ring buffer handle
 * @param context Venus dispatch context
 * @return 0 on success, negative on error
 */
int pv_venus_integration_start(struct pv_venus_ring *ring, void *context);

/*
 * Stop ring buffer processing
 * Integration-specific cleanup
 */
void pv_venus_integration_stop(struct pv_venus_ring *ring);

/*
 * Get ring buffer utilization (0.0 - 1.0)
 * Integration helper function
 */
double pv_venus_ring_utilization(struct pv_venus_ring *ring);

/*
 * Initialize Venus handler context
 * Creates MoltenVK instance, device, queues
 * Registers all command handlers
 * 
 * @return Context handle or NULL on failure
 */
void* pv_venus_init(void);

/*
 * Cleanup Venus handler context
 * Destroys all Vulkan objects and frees resources
 */
void pv_venus_cleanup(void *context);

/*
 * Get Venus protocol statistics
 */
struct pv_venus_stats pv_venus_get_stats(void *context);

#ifdef __cplusplus
}
#endif

#endif /* PV_VENUS_INTEGRATION_H */
