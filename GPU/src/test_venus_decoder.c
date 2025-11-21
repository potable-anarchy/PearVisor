/*
 * PearVisor - Venus Decoder Test
 * 
 * Test program to verify command decoding and dispatch
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pv_venus_ring.h"
#include "pv_venus_decoder.h"
#include "pv_venus_protocol.h"

/*
 * Mock command handlers
 */
static int handle_create_instance(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)context;
    (void)header;
    (void)data;
    (void)data_size;
    
    printf("  [Handler] vkCreateInstance called\n");
    return 0;
}

static int handle_enumerate_physical_devices(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)context;
    (void)header;
    (void)data;
    (void)data_size;
    
    printf("  [Handler] vkEnumeratePhysicalDevices called\n");
    return 0;
}

static int handle_create_device(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)context;
    (void)header;
    (void)data;
    (void)data_size;
    
    printf("  [Handler] vkCreateDevice called\n");
    return 0;
}

/*
 * Write a mock Venus command to the ring buffer
 */
static void write_mock_command(
    struct pv_venus_ring *ring,
    uint32_t command_id,
    const void *payload,
    size_t payload_size)
{
    /* Create command header */
    struct pv_venus_command_header header = {
        .command_id = command_id,
        .command_size = sizeof(header) + payload_size,
    };
    
    /* Get current tail */
    uint32_t tail = pv_venus_ring_get_tail(ring);
    
    /* Write header to ring buffer memory */
    uint32_t pos = tail & ring->buffer.mask;
    memcpy((void *)(ring->buffer.data + pos), &header, sizeof(header));
    tail += sizeof(header);
    
    /* Write payload if any */
    if (payload_size > 0 && payload) {
        pos = tail & ring->buffer.mask;
        
        /* Handle wrapping */
        size_t space_to_end = ring->buffer.size - pos;
        if (payload_size <= space_to_end) {
            memcpy((void *)(ring->buffer.data + pos), payload, payload_size);
        } else {
            /* Split across wrap boundary */
            memcpy((void *)(ring->buffer.data + pos), payload, space_to_end);
            memcpy((void *)ring->buffer.data, 
                   (const uint8_t *)payload + space_to_end,
                   payload_size - space_to_end);
        }
        tail += payload_size;
    }
    
    /* Update tail */
    atomic_store_explicit((atomic_uint *)ring->control.tail, tail, 
                         memory_order_release);
    
    printf("[Test] Wrote %s (size=%u)\n",
           pv_venus_command_name(command_id),
           header.command_size);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    printf("=== PearVisor Venus Decoder Test ===\n\n");

    /* Allocate shared memory */
    const size_t buffer_size = 4096;
    const size_t extra_size = 1024;
    const size_t total_size = 
        sizeof(uint32_t) * 3 +  /* head, tail, status */
        buffer_size +            /* command buffer */
        extra_size;              /* extra region */

    void *shared_mem = calloc(1, total_size);
    if (!shared_mem) {
        fprintf(stderr, "Failed to allocate shared memory\n");
        return 1;
    }

    /* Setup ring buffer */
    struct pv_venus_ring_layout layout = {
        .shared_memory = shared_mem,
        .shared_memory_size = total_size,
        .head_offset = 0,
        .tail_offset = sizeof(uint32_t),
        .status_offset = sizeof(uint32_t) * 2,
        .buffer_offset = sizeof(uint32_t) * 3,
        .buffer_size = buffer_size,
        .extra_offset = sizeof(uint32_t) * 3 + buffer_size,
        .extra_size = extra_size,
    };

    struct pv_venus_ring *ring = pv_venus_ring_create(&layout, NULL);
    if (!ring) {
        fprintf(stderr, "Failed to create ring buffer\n");
        free(shared_mem);
        return 1;
    }

    /* Create dispatch context */
    struct pv_venus_dispatch_context *ctx = pv_venus_dispatch_create();
    if (!ctx) {
        fprintf(stderr, "Failed to create dispatch context\n");
        pv_venus_ring_destroy(ring);
        free(shared_mem);
        return 1;
    }

    /* Test 1: Register handlers */
    printf("\n--- Test 1: Register Handlers ---\n");
    pv_venus_dispatch_register(ctx, PV_VK_COMMAND_vkCreateInstance, 
                                handle_create_instance);
    pv_venus_dispatch_register(ctx, PV_VK_COMMAND_vkEnumeratePhysicalDevices,
                                handle_enumerate_physical_devices);
    pv_venus_dispatch_register(ctx, PV_VK_COMMAND_vkCreateDevice,
                                handle_create_device);

    /* Test 2: Write and decode single command */
    printf("\n--- Test 2: Single Command ---\n");
    write_mock_command(ring, PV_VK_COMMAND_vkCreateInstance, NULL, 0);
    
    int ret = pv_venus_decode_command(ring, ctx);
    printf("Decode result: %d\n", ret);
    
    /* Update head */
    pv_venus_ring_set_head(ring, ring->buffer.current_pos);

    /* Test 3: Write multiple commands */
    printf("\n--- Test 3: Multiple Commands ---\n");
    write_mock_command(ring, PV_VK_COMMAND_vkEnumeratePhysicalDevices, NULL, 0);
    write_mock_command(ring, PV_VK_COMMAND_vkCreateDevice, NULL, 0);
    write_mock_command(ring, PV_VK_COMMAND_vkGetDeviceQueue, NULL, 0);  /* No handler */
    
    int processed = pv_venus_decode_all(ring, ctx);
    printf("Processed %d commands\n", processed);

    /* Test 4: Commands with payload */
    printf("\n--- Test 4: Commands with Payload ---\n");
    char payload[64] = "Test payload data";
    write_mock_command(ring, PV_VK_COMMAND_vkCreateInstance, payload, sizeof(payload));
    
    ret = pv_venus_decode_command(ring, ctx);
    printf("Decode result: %d\n", ret);
    pv_venus_ring_set_head(ring, ring->buffer.current_pos);

    /* Test 5: Validate error handling */
    printf("\n--- Test 5: Invalid Command ---\n");
    write_mock_command(ring, 999, NULL, 0);  /* Invalid command ID */
    
    ret = pv_venus_decode_command(ring, ctx);
    printf("Decode result (should fail): %d\n", ret);
    pv_venus_ring_set_head(ring, ring->buffer.current_pos);

    /* Test 6: Check statistics */
    printf("\n--- Test 6: Statistics ---\n");
    printf("Commands dispatched: %llu\n", ctx->commands_dispatched);
    printf("Commands unknown: %llu\n", ctx->commands_unknown);
    printf("Commands failed: %llu\n", ctx->commands_failed);

    /* Cleanup */
    printf("\n--- Cleanup ---\n");
    pv_venus_dispatch_destroy(ctx);
    pv_venus_ring_destroy(ring);
    free(shared_mem);

    printf("\n=== All Tests Passed! ===\n");
    return 0;
}
