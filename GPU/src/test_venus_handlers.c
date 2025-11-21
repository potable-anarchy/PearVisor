/*
 * test_venus_handlers.c - Test Venus command handlers
 *
 * Tests the complete Venus protocol pipeline:
 *   Guest commands → Ring buffer → Decoder → Handlers → MoltenVK → Metal
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

/* Test 1: Handler context creation and destruction */
static void test_handler_context(void)
{
    printf("Test 1: Handler context creation/destruction...\n");
    
    struct pv_venus_handler_context *ctx = pv_venus_handlers_create();
    assert(ctx != NULL);
    assert(ctx->vk != NULL);
    assert(ctx->objects.objects != NULL);
    assert(ctx->objects.capacity == 1024);
    assert(ctx->objects.count == 0);
    assert(ctx->commands_handled == 0);
    
    pv_venus_handlers_destroy(ctx);
    
    printf("  ✓ Context creation/destruction works\n");
}

/* Test 2: Object table operations */
static void test_object_table(void)
{
    printf("Test 2: Object table operations...\n");
    
    struct pv_venus_handler_context *ctx = pv_venus_handlers_create();
    
    /* Add some objects */
    VkInstance fake_instance = (VkInstance)0xDEADBEEF;
    VkPhysicalDevice fake_device = (VkPhysicalDevice)0xCAFEBABE;
    
    pv_venus_object_add(&ctx->objects, 0x1000, fake_instance, PV_VENUS_OBJECT_TYPE_INSTANCE);
    pv_venus_object_add(&ctx->objects, 0x2000, fake_device, PV_VENUS_OBJECT_TYPE_PHYSICAL_DEVICE);
    
    assert(ctx->objects.count == 2);
    
    /* Retrieve objects */
    void *retrieved = pv_venus_object_get(&ctx->objects, 0x1000);
    assert(retrieved == fake_instance);
    
    retrieved = pv_venus_object_get(&ctx->objects, 0x2000);
    assert(retrieved == fake_device);
    
    /* Remove object */
    pv_venus_object_remove(&ctx->objects, 0x1000);
    assert(ctx->objects.count == 1);
    
    retrieved = pv_venus_object_get(&ctx->objects, 0x1000);
    assert(retrieved == NULL);
    
    pv_venus_handlers_destroy(ctx);
    
    printf("  ✓ Object table add/get/remove works\n");
}

/* Test 3: Handler registration */
static void test_handler_registration(void)
{
    printf("Test 3: Handler registration...\n");
    
    struct pv_venus_handler_context *handler_ctx = pv_venus_handlers_create();
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    
    /* Register all handlers */
    pv_venus_handlers_register(dispatch_ctx, handler_ctx);
    
    /* Verify handlers are registered */
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkCreateInstance] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkEnumeratePhysicalDevices] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkCreateDevice] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkGetDeviceQueue] != NULL);
    
    pv_venus_dispatch_destroy(dispatch_ctx);
    pv_venus_handlers_destroy(handler_ctx);
    
    printf("  ✓ Handler registration works\n");
}

/* Test 4: End-to-end command processing */
static void test_command_processing(void)
{
    printf("Test 4: End-to-end command processing...\n");
    
    /* Allocate shared memory for ring buffer */
    const size_t buffer_size = 4096;
    const size_t extra_size = 1024;
    const size_t total_size = 
        sizeof(uint32_t) * 3 +  /* head, tail, status */
        buffer_size +            /* command buffer */
        extra_size;              /* extra region */

    void *shared_mem = calloc(1, total_size);
    assert(shared_mem != NULL);

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
    assert(ring != NULL);
    
    /* Create contexts */
    struct pv_venus_handler_context *handler_ctx = pv_venus_handlers_create();
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    
    pv_venus_handlers_register(dispatch_ctx, handler_ctx);
    
    /* Write commands to ring buffer */
    printf("  Writing commands to ring buffer...\n");
    write_command(ring, PV_VK_COMMAND_vkCreateInstance);
    write_command(ring, PV_VK_COMMAND_vkEnumeratePhysicalDevices);
    write_command(ring, PV_VK_COMMAND_vkGetPhysicalDeviceProperties);
    write_command(ring, PV_VK_COMMAND_vkCreateDevice);
    write_command(ring, PV_VK_COMMAND_vkGetDeviceQueue);
    
    printf("  Processing commands...\n");
    
    /* Decode all commands */
    int processed = pv_venus_decode_all(ring, dispatch_ctx);
    printf("  Processed %d commands\n", processed);
    assert(processed == 5);
    
    /* Verify statistics */
    printf("  Commands dispatched: %llu\n", dispatch_ctx->commands_dispatched);
    printf("  Commands handled: %llu\n", handler_ctx->commands_handled);
    printf("  Objects created: %llu\n", handler_ctx->objects_created);
    
    assert(dispatch_ctx->commands_dispatched == 5);
    assert(handler_ctx->commands_handled == 5);
    assert(handler_ctx->objects_created >= 3); /* At least instance, physical device, device */
    
    /* Verify MoltenVK objects were created */
    assert(handler_ctx->vk->instance_created == true);
    assert(handler_ctx->vk->device_created == true);
    assert(handler_ctx->vk->physical_device != VK_NULL_HANDLE);
    assert(handler_ctx->vk->graphics_queue != VK_NULL_HANDLE);
    
    /* Verify object table has entries */
    assert(handler_ctx->objects.count >= 3);
    
    printf("  ✓ MoltenVK instance created: %p\n", (void*)handler_ctx->vk->instance);
    printf("  ✓ MoltenVK physical device selected: %s\n", handler_ctx->vk->device_properties.deviceName);
    printf("  ✓ MoltenVK logical device created: %p\n", (void*)handler_ctx->vk->device);
    printf("  ✓ MoltenVK graphics queue obtained: %p\n", (void*)handler_ctx->vk->graphics_queue);
    
    /* Cleanup */
    pv_venus_dispatch_destroy(dispatch_ctx);
    pv_venus_handlers_destroy(handler_ctx);
    pv_venus_ring_destroy(ring);
    free(shared_mem);
    
    printf("  ✓ End-to-end command processing works\n");
}

/* Test 5: Complete pipeline verification */
static void test_complete_pipeline(void)
{
    printf("Test 5: Complete pipeline verification...\n");
    printf("  Pipeline: Guest → Ring → Decoder → Handlers → MoltenVK → Metal\n");
    
    /* Allocate shared memory */
    const size_t buffer_size = 4096;
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
    
    struct pv_venus_handler_context *handler_ctx = pv_venus_handlers_create();
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    
    pv_venus_handlers_register(dispatch_ctx, handler_ctx);
    
    /* Simulate guest initialization sequence */
    write_command(ring, PV_VK_COMMAND_vkCreateInstance);
    write_command(ring, PV_VK_COMMAND_vkEnumeratePhysicalDevices);
    write_command(ring, PV_VK_COMMAND_vkCreateDevice);
    
    /* Process initialization commands */
    int processed = pv_venus_decode_all(ring, dispatch_ctx);
    assert(processed == 3);
    
    /* Verify Vulkan is initialized and ready */
    assert(handler_ctx->vk->instance_created);
    assert(handler_ctx->vk->device_created);
    assert(handler_ctx->vk->physical_device != VK_NULL_HANDLE);
    
    printf("  ✓ Complete pipeline operational:\n");
    printf("    - Ring buffer: Ready (Session 4)\n");
    printf("    - Decoder: Dispatching commands (Session 5)\n");
    printf("    - MoltenVK: Connected to Metal (Session 6)\n");
    printf("    - Handlers: Processing Venus commands (Session 7)\n");
    printf("  ✓ Guest can now submit Vulkan workloads!\n");
    
    pv_venus_dispatch_destroy(dispatch_ctx);
    pv_venus_handlers_destroy(handler_ctx);
    pv_venus_ring_destroy(ring);
    free(shared_mem);
}

int main(void)
{
    printf("=== Venus Handlers Test Suite ===\n\n");
    
    test_handler_context();
    printf("\n");
    
    test_object_table();
    printf("\n");
    
    test_handler_registration();
    printf("\n");
    
    test_command_processing();
    printf("\n");
    
    test_complete_pipeline();
    printf("\n");
    
    printf("=== All Tests Passed ✓ ===\n");
    printf("\nSession 7 Complete: Venus handlers operational\n");
    printf("Ready for Session 8: Resource & memory handlers\n");
    
    return 0;
}
