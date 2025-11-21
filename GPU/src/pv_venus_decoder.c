/*
 * PearVisor - Venus Command Decoder Implementation
 */

#include "pv_venus_decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Create dispatch context
 */
struct pv_venus_dispatch_context *pv_venus_dispatch_create(void)
{
    struct pv_venus_dispatch_context *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fprintf(stderr, "[Venus Decoder] Failed to allocate context\n");
        return NULL;
    }

    /* Initialize handler table to NULL */
    memset(ctx->handlers, 0, sizeof(ctx->handlers));

    printf("[Venus Decoder] Created dispatch context\n");
    return ctx;
}

/*
 * Destroy dispatch context
 */
void pv_venus_dispatch_destroy(struct pv_venus_dispatch_context *ctx)
{
    if (!ctx) {
        return;
    }

    printf("[Venus Decoder] Stats: dispatched=%llu unknown=%llu failed=%llu\n",
           ctx->commands_dispatched,
           ctx->commands_unknown,
           ctx->commands_failed);

    free(ctx);
}

/*
 * Register a command handler
 */
void pv_venus_dispatch_register(
    struct pv_venus_dispatch_context *ctx,
    uint32_t command_id,
    pv_venus_command_handler_t handler)
{
    if (!ctx || command_id >= PV_VENUS_MAX_COMMAND_ID) {
        return;
    }

    ctx->handlers[command_id] = handler;
    printf("[Venus Decoder] Registered handler for %s (id=%u)\n",
           pv_venus_command_name(command_id), command_id);
}

/*
 * Process one command from ring buffer
 */
int pv_venus_decode_command(
    struct pv_venus_ring *ring,
    struct pv_venus_dispatch_context *ctx)
{
    if (!ring || !ctx) {
        return -1;
    }

    /* Read command header */
    struct pv_venus_command_header header;
    if (pv_venus_ring_read(ring, &header, sizeof(header)) != 0) {
        fprintf(stderr, "[Venus Decoder] Failed to read command header\n");
        ctx->commands_failed++;
        return -1;
    }

    /* Validate header */
    if (pv_venus_validate_command_header(&header) != 0) {
        fprintf(stderr, "[Venus Decoder] Invalid command header\n");
        ctx->commands_failed++;
        return -1;
    }

    /* Calculate data size (header.command_size includes header) */
    size_t data_size = header.command_size - sizeof(header);

    /* Read command data (if any) */
    void *data = NULL;
    if (data_size > 0) {
        data = malloc(data_size);
        if (!data) {
            fprintf(stderr, "[Venus Decoder] Failed to allocate command data\n");
            ctx->commands_failed++;
            return -1;
        }

        if (pv_venus_ring_read(ring, data, data_size) != 0) {
            fprintf(stderr, "[Venus Decoder] Failed to read command data\n");
            free(data);
            ctx->commands_failed++;
            return -1;
        }
    }

    /* Log command for debugging */
    printf("[Venus Decoder] Command: %s (id=%u size=%u)\n",
           pv_venus_command_name(header.command_id),
           header.command_id,
           header.command_size);

    /* Dispatch to handler */
    pv_venus_command_handler_t handler = ctx->handlers[header.command_id];
    
    int ret = 0;
    if (handler) {
        ret = handler(ctx, &header, data, data_size);
        if (ret != 0) {
            fprintf(stderr, "[Venus Decoder] Handler failed for %s: %d\n",
                    pv_venus_command_name(header.command_id), ret);
            ctx->commands_failed++;
        } else {
            ctx->commands_dispatched++;
        }
    } else {
        printf("[Venus Decoder] No handler for %s\n",
               pv_venus_command_name(header.command_id));
        ctx->commands_unknown++;
    }

    /* Cleanup */
    if (data) {
        free(data);
    }

    return ret;
}

/*
 * Process all available commands from ring buffer
 */
int pv_venus_decode_all(
    struct pv_venus_ring *ring,
    struct pv_venus_dispatch_context *ctx)
{
    if (!ring || !ctx) {
        return -1;
    }

    int processed = 0;
    uint32_t tail = pv_venus_ring_get_tail(ring);
    uint32_t head = ring->buffer.current_pos;

    /* Process all available commands */
    while (head != tail) {
        if (pv_venus_decode_command(ring, ctx) != 0) {
            /* Error occurred, but continue processing */
            fprintf(stderr, "[Venus Decoder] Error processing command, continuing...\n");
        }

        processed++;
        
        /* Update head and tail */
        head = ring->buffer.current_pos;
        tail = pv_venus_ring_get_tail(ring);
    }

    /* Update ring head */
    if (processed > 0) {
        pv_venus_ring_set_head(ring, head);
    }

    return processed;
}
