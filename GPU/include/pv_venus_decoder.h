/*
 * PearVisor - Venus Command Decoder
 * 
 * Decodes and dispatches Venus protocol commands from ring buffer
 */

#ifndef PV_VENUS_DECODER_H
#define PV_VENUS_DECODER_H

#include "pv_venus_ring.h"
#include "pv_venus_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Command dispatch context
 * 
 * This holds the state needed to dispatch commands,
 * including Vulkan instance/device handles
 */
struct pv_venus_dispatch_context {
    /* Command handlers */
    pv_venus_command_handler_t handlers[PV_VENUS_MAX_COMMAND_ID];
    
    /* User context (e.g., pv_venus_handler_context) */
    void *user_context;
    
    /* Vulkan state (will be populated later) */
    void *vk_instance;
    void *vk_physical_device;
    void *vk_device;
    
    /* Statistics */
    uint64_t commands_dispatched;
    uint64_t commands_unknown;
    uint64_t commands_failed;
};

/*
 * Create dispatch context
 */
struct pv_venus_dispatch_context *pv_venus_dispatch_create(void);

/*
 * Destroy dispatch context
 */
void pv_venus_dispatch_destroy(struct pv_venus_dispatch_context *ctx);

/*
 * Register a command handler
 */
void pv_venus_dispatch_register(
    struct pv_venus_dispatch_context *ctx,
    uint32_t command_id,
    pv_venus_command_handler_t handler
);

/*
 * Process one command from ring buffer
 * 
 * Returns: 0 on success, negative on error
 */
int pv_venus_decode_command(
    struct pv_venus_ring *ring,
    struct pv_venus_dispatch_context *ctx
);

/*
 * Process all available commands from ring buffer
 * 
 * Returns: Number of commands processed
 */
int pv_venus_decode_all(
    struct pv_venus_ring *ring,
    struct pv_venus_dispatch_context *ctx
);

#ifdef __cplusplus
}
#endif

#endif /* PV_VENUS_DECODER_H */
