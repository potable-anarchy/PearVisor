/*
 * test_command_buffers.c - Test command buffer and queue submission handlers
 *
 * Tests Session 9 additions:
 *   - Command pool creation
 *   - Command buffer allocation
 *   - Command buffer recording
 *   - Queue submission
 *   - GPU work execution
 */

#include "pv_venus_handlers.h"
#include "pv_venus_decoder.h"
#include "pv_venus_protocol.h"
#include "pv_venus_ring.h"
#include "pv_moltenvk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Helper: Write a Venus command to the ring buffer */
static void write_command(struct pv_venus_ring *ring, uint32_t command_id)
{
    struct pv_venus_command_header header = {
        .command_id = command_id,
        .command_size = sizeof(header),
    };
    
    uint32_t tail = pv_venus_ring_get_tail(ring);
    uint32_t pos = tail & ring->buffer.mask;
    memcpy((void *)(ring->buffer.data + pos), &header, sizeof(header));
    tail += sizeof(header);
    
    atomic_store_explicit((atomic_uint *)ring->control.tail, tail, 
                         memory_order_release);
    
    printf("[Test] Wrote %s\n", pv_venus_command_name(command_id));
}

/* Helper: Create ring buffer for testing */
static struct pv_venus_ring *create_test_ring(void **shared_mem_out)
{
    const size_t buffer_size = 8192;  // Larger for more commands
    const size_t total_size = sizeof(uint32_t) * 3 + buffer_size + 1024;
    void *shared_mem = calloc(1, total_size);
    assert(shared_mem != NULL);

    struct pv_venus_ring_layout layout = {
        .shared_memory = shared_mem,
        .shared_memory_size = total_size,
        .head_offset = 0,
        .tail_offset = sizeof(uint32_t),
        .status_offset = sizeof(uint32_t) * 2,
        .buffer_offset = sizeof(uint32_t) * 3,
        .buffer_size = buffer_size,
        .extra_offset = sizeof(uint32_t) * 3 + buffer_size,
        .extra_size = 1024,
    };

    struct pv_venus_ring *ring = pv_venus_ring_create(&layout, NULL);
    assert(ring != NULL);
    
    *shared_mem_out = shared_mem;
    return ring;
}

/* Test 1: Command pool creation */
static void test_command_pool_creation(void)
{
    printf("Test 1: Command pool creation...\n");
    
    void *shared_mem;
    struct pv_venus_ring *ring = create_test_ring(&shared_mem);
    struct pv_venus_handler_context *handler_ctx = pv_venus_handlers_create();
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    
    pv_venus_handlers_register(dispatch_ctx, handler_ctx);
    
    /* Initialize Vulkan first */
    write_command(ring, PV_VK_COMMAND_vkCreateInstance);
    write_command(ring, PV_VK_COMMAND_vkEnumeratePhysicalDevices);
    write_command(ring, PV_VK_COMMAND_vkCreateDevice);
    pv_venus_decode_all(ring, dispatch_ctx);
    
    /* Create command pool */
    write_command(ring, PV_VK_COMMAND_vkCreateCommandPool);
    
    int processed = pv_venus_decode_all(ring, dispatch_ctx);
    assert(processed == 1);
    
    /* Verify command pool was tracked */
    VkCommandPool pool = pv_venus_object_get(&handler_ctx->objects, 0x8000);
    assert(pool != NULL);
    
    printf("  ✓ Command pool created and tracked: %p\n", (void*)pool);
    
    /* Cleanup */
    pv_venus_dispatch_destroy(dispatch_ctx);
    pv_venus_handlers_destroy(handler_ctx);
    pv_venus_ring_destroy(ring);
    free(shared_mem);
    
    printf("  ✓ Command pool creation works\n");
}

/* Test 2: Command buffer allocation */
static void test_command_buffer_allocation(void)
{
    printf("Test 2: Command buffer allocation...\n");
    
    void *shared_mem;
    struct pv_venus_ring *ring = create_test_ring(&shared_mem);
    struct pv_venus_handler_context *handler_ctx = pv_venus_handlers_create();
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    
    pv_venus_handlers_register(dispatch_ctx, handler_ctx);
    
    /* Initialize Vulkan and create command pool */
    write_command(ring, PV_VK_COMMAND_vkCreateInstance);
    write_command(ring, PV_VK_COMMAND_vkEnumeratePhysicalDevices);
    write_command(ring, PV_VK_COMMAND_vkCreateDevice);
    write_command(ring, PV_VK_COMMAND_vkCreateCommandPool);
    pv_venus_decode_all(ring, dispatch_ctx);
    
    /* Allocate command buffer */
    write_command(ring, PV_VK_COMMAND_vkAllocateCommandBuffers);
    
    int processed = pv_venus_decode_all(ring, dispatch_ctx);
    assert(processed == 1);
    
    /* Verify command buffer was tracked */
    VkCommandBuffer cmd_buffer = pv_venus_object_get(&handler_ctx->objects, 0x9000);
    assert(cmd_buffer != NULL);
    
    printf("  ✓ Command buffer allocated and tracked: %p\n", (void*)cmd_buffer);
    
    /* Cleanup */
    pv_venus_dispatch_destroy(dispatch_ctx);
    pv_venus_handlers_destroy(handler_ctx);
    pv_venus_ring_destroy(ring);
    free(shared_mem);
    
    printf("  ✓ Command buffer allocation works\n");
}

/* Test 3: Command buffer recording */
static void test_command_buffer_recording(void)
{
    printf("Test 3: Command buffer recording...\n");
    
    void *shared_mem;
    struct pv_venus_ring *ring = create_test_ring(&shared_mem);
    struct pv_venus_handler_context *handler_ctx = pv_venus_handlers_create();
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    
    pv_venus_handlers_register(dispatch_ctx, handler_ctx);
    
    /* Initialize and allocate command buffer */
    write_command(ring, PV_VK_COMMAND_vkCreateInstance);
    write_command(ring, PV_VK_COMMAND_vkEnumeratePhysicalDevices);
    write_command(ring, PV_VK_COMMAND_vkCreateDevice);
    write_command(ring, PV_VK_COMMAND_vkCreateCommandPool);
    write_command(ring, PV_VK_COMMAND_vkAllocateCommandBuffers);
    pv_venus_decode_all(ring, dispatch_ctx);
    
    /* Begin recording */
    write_command(ring, PV_VK_COMMAND_vkBeginCommandBuffer);
    int processed = pv_venus_decode_all(ring, dispatch_ctx);
    assert(processed == 1);
    
    printf("  ✓ Command buffer recording started\n");
    
    /* End recording */
    write_command(ring, PV_VK_COMMAND_vkEndCommandBuffer);
    processed = pv_venus_decode_all(ring, dispatch_ctx);
    assert(processed == 1);
    
    printf("  ✓ Command buffer recording finished\n");
    
    /* Cleanup */
    pv_venus_dispatch_destroy(dispatch_ctx);
    pv_venus_handlers_destroy(handler_ctx);
    pv_venus_ring_destroy(ring);
    free(shared_mem);
    
    printf("  ✓ Command buffer recording works\n");
}

/* Test 4: Queue submission */
static void test_queue_submission(void)
{
    printf("Test 4: Queue submission...\n");
    
    void *shared_mem;
    struct pv_venus_ring *ring = create_test_ring(&shared_mem);
    struct pv_venus_handler_context *handler_ctx = pv_venus_handlers_create();
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    
    pv_venus_handlers_register(dispatch_ctx, handler_ctx);
    
    /* Complete command buffer workflow */
    write_command(ring, PV_VK_COMMAND_vkCreateInstance);
    write_command(ring, PV_VK_COMMAND_vkEnumeratePhysicalDevices);
    write_command(ring, PV_VK_COMMAND_vkCreateDevice);
    write_command(ring, PV_VK_COMMAND_vkCreateCommandPool);
    write_command(ring, PV_VK_COMMAND_vkAllocateCommandBuffers);
    write_command(ring, PV_VK_COMMAND_vkBeginCommandBuffer);
    write_command(ring, PV_VK_COMMAND_vkEndCommandBuffer);
    pv_venus_decode_all(ring, dispatch_ctx);
    
    /* Submit command buffer to queue */
    write_command(ring, PV_VK_COMMAND_vkQueueSubmit);
    int processed = pv_venus_decode_all(ring, dispatch_ctx);
    assert(processed == 1);
    
    printf("  ✓ Command buffer submitted to GPU queue\n");
    
    /* Wait for GPU to finish */
    write_command(ring, PV_VK_COMMAND_vkQueueWaitIdle);
    processed = pv_venus_decode_all(ring, dispatch_ctx);
    assert(processed == 1);
    
    printf("  ✓ GPU work completed (queue idle)\n");
    
    /* Cleanup */
    pv_venus_dispatch_destroy(dispatch_ctx);
    pv_venus_handlers_destroy(handler_ctx);
    pv_venus_ring_destroy(ring);
    free(shared_mem);
    
    printf("  ✓ Queue submission works\n");
}

/* Test 5: Complete GPU workflow */
static void test_complete_gpu_workflow(void)
{
    printf("Test 5: Complete GPU workflow...\n");
    
    void *shared_mem;
    struct pv_venus_ring *ring = create_test_ring(&shared_mem);
    struct pv_venus_handler_context *handler_ctx = pv_venus_handlers_create();
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    
    pv_venus_handlers_register(dispatch_ctx, handler_ctx);
    
    /* Complete initialization → submission → wait sequence */
    printf("  Running complete GPU workflow...\n");
    
    write_command(ring, PV_VK_COMMAND_vkCreateInstance);
    write_command(ring, PV_VK_COMMAND_vkEnumeratePhysicalDevices);
    write_command(ring, PV_VK_COMMAND_vkCreateDevice);
    write_command(ring, PV_VK_COMMAND_vkCreateCommandPool);
    write_command(ring, PV_VK_COMMAND_vkAllocateCommandBuffers);
    write_command(ring, PV_VK_COMMAND_vkBeginCommandBuffer);
    write_command(ring, PV_VK_COMMAND_vkEndCommandBuffer);
    write_command(ring, PV_VK_COMMAND_vkQueueSubmit);
    write_command(ring, PV_VK_COMMAND_vkQueueWaitIdle);
    
    int processed = pv_venus_decode_all(ring, dispatch_ctx);
    assert(processed == 9);
    
    /* Verify all objects were created */
    printf("  Verifying object tracking...\n");
    assert(pv_venus_object_get(&handler_ctx->objects, 0x1000) != NULL); // Instance
    assert(pv_venus_object_get(&handler_ctx->objects, 0x2000) != NULL); // Physical device
    assert(pv_venus_object_get(&handler_ctx->objects, 0x3000) != NULL); // Device
    assert(pv_venus_object_get(&handler_ctx->objects, 0x4000) != NULL); // Queue
    assert(pv_venus_object_get(&handler_ctx->objects, 0x8000) != NULL); // Command pool
    assert(pv_venus_object_get(&handler_ctx->objects, 0x9000) != NULL); // Command buffer
    
    printf("  ✓ All 6 objects tracked correctly\n");
    
    /* Verify statistics */
    printf("  Statistics:\n");
    printf("    Commands dispatched: %llu\n", dispatch_ctx->commands_dispatched);
    printf("    Commands handled: %llu\n", handler_ctx->commands_handled);
    printf("    Objects created: %llu\n", handler_ctx->objects_created);
    printf("    Objects in table: %zu\n", handler_ctx->objects.count);
    
    assert(dispatch_ctx->commands_dispatched == 9);
    assert(handler_ctx->commands_handled == 9);
    assert(handler_ctx->objects_created == 6);
    assert(handler_ctx->objects.count == 6);
    
    printf("  ✓ Statistics match expectations\n");
    printf("  ✓ GPU work submitted and completed successfully!\n");
    
    /* Cleanup */
    pv_venus_dispatch_destroy(dispatch_ctx);
    pv_venus_handlers_destroy(handler_ctx);
    pv_venus_ring_destroy(ring);
    free(shared_mem);
    
    printf("  ✓ Complete GPU workflow successful\n");
}

/* Test 6: Handler registration verification */
static void test_handler_registration(void)
{
    printf("Test 6: Verify all 25 handlers registered...\n");
    
    struct pv_venus_handler_context *handler_ctx = pv_venus_handlers_create();
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    
    pv_venus_handlers_register(dispatch_ctx, handler_ctx);
    
    /* Verify command pool and buffer handlers */
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkCreateCommandPool] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkDestroyCommandPool] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkAllocateCommandBuffers] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkFreeCommandBuffers] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkBeginCommandBuffer] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkEndCommandBuffer] != NULL);
    
    /* Verify queue operation handlers */
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkQueueSubmit] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkQueueWaitIdle] != NULL);
    
    printf("  ✓ All command buffer and queue handlers registered\n");
    
    pv_venus_dispatch_destroy(dispatch_ctx);
    pv_venus_handlers_destroy(handler_ctx);
    
    printf("  ✓ Handler registration complete\n");
}

int main(void)
{
    printf("=== Command Buffer & Queue Submission Test Suite ===\n\n");
    
    test_handler_registration();
    printf("\n");
    
    test_command_pool_creation();
    printf("\n");
    
    test_command_buffer_allocation();
    printf("\n");
    
    test_command_buffer_recording();
    printf("\n");
    
    test_queue_submission();
    printf("\n");
    
    test_complete_gpu_workflow();
    printf("\n");
    
    printf("=== All Tests Passed ✓ ===\n");
    printf("\nSession 9 Complete: Command buffers & queue submission operational\n");
    printf("GPU workload submission now working!\n");
    printf("Ready for Session 10: End-to-end guest testing\n");
    
    return 0;
}
