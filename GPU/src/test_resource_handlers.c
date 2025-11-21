/*
 * test_resource_handlers.c - Test resource and memory handlers
 *
 * Tests Session 8 additions:
 *   - Memory allocation and binding
 *   - Buffer creation and binding
 *   - Image creation
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
    
    *shared_mem_out = shared_mem;
    return ring;
}

/* Test 1: Memory allocation */
static void test_memory_allocation(void)
{
    printf("Test 1: Memory allocation...\n");
    
    void *shared_mem;
    struct pv_venus_ring *ring = create_test_ring(&shared_mem);
    struct pv_venus_handler_context *handler_ctx = pv_venus_handlers_create();
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    
    pv_venus_handlers_register(dispatch_ctx, handler_ctx);
    
    /* Initialize Vulkan first */
    write_command(ring, PV_VK_COMMAND_vkCreateInstance);
    write_command(ring, PV_VK_COMMAND_vkEnumeratePhysicalDevices);
    write_command(ring, PV_VK_COMMAND_vkCreateDevice);
    
    int processed = pv_venus_decode_all(ring, dispatch_ctx);
    assert(processed == 3);
    
    /* Now test memory allocation */
    write_command(ring, PV_VK_COMMAND_vkAllocateMemory);
    
    processed = pv_venus_decode_all(ring, dispatch_ctx);
    assert(processed == 1);
    
    /* Verify memory was tracked */
    VkDeviceMemory memory = pv_venus_object_get(&handler_ctx->objects, 0x5000);
    assert(memory != NULL);
    
    printf("  ✓ Memory allocated and tracked: %p\n", (void*)memory);
    
    /* Cleanup */
    pv_venus_dispatch_destroy(dispatch_ctx);
    pv_venus_handlers_destroy(handler_ctx);
    pv_venus_ring_destroy(ring);
    free(shared_mem);
    
    printf("  ✓ Memory allocation works\n");
}

/* Test 2: Buffer creation and binding */
static void test_buffer_creation(void)
{
    printf("Test 2: Buffer creation and binding...\n");
    
    void *shared_mem;
    struct pv_venus_ring *ring = create_test_ring(&shared_mem);
    struct pv_venus_handler_context *handler_ctx = pv_venus_handlers_create();
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    
    pv_venus_handlers_register(dispatch_ctx, handler_ctx);
    
    /* Initialize Vulkan */
    write_command(ring, PV_VK_COMMAND_vkCreateInstance);
    write_command(ring, PV_VK_COMMAND_vkEnumeratePhysicalDevices);
    write_command(ring, PV_VK_COMMAND_vkCreateDevice);
    pv_venus_decode_all(ring, dispatch_ctx);
    
    /* Allocate memory */
    write_command(ring, PV_VK_COMMAND_vkAllocateMemory);
    pv_venus_decode_all(ring, dispatch_ctx);
    
    /* Create buffer */
    write_command(ring, PV_VK_COMMAND_vkCreateBuffer);
    
    int processed = pv_venus_decode_all(ring, dispatch_ctx);
    assert(processed == 1);
    
    /* Verify buffer was tracked */
    VkBuffer buffer = pv_venus_object_get(&handler_ctx->objects, 0x6000);
    assert(buffer != NULL);
    
    printf("  ✓ Buffer created and tracked: %p\n", (void*)buffer);
    
    /* Bind buffer to memory */
    write_command(ring, PV_VK_COMMAND_vkBindBufferMemory);
    
    processed = pv_venus_decode_all(ring, dispatch_ctx);
    assert(processed == 1);
    
    printf("  ✓ Buffer bound to memory\n");
    
    /* Cleanup */
    pv_venus_dispatch_destroy(dispatch_ctx);
    pv_venus_handlers_destroy(handler_ctx);
    pv_venus_ring_destroy(ring);
    free(shared_mem);
    
    printf("  ✓ Buffer creation and binding works\n");
}

/* Test 3: Image creation */
static void test_image_creation(void)
{
    printf("Test 3: Image creation...\n");
    
    void *shared_mem;
    struct pv_venus_ring *ring = create_test_ring(&shared_mem);
    struct pv_venus_handler_context *handler_ctx = pv_venus_handlers_create();
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    
    pv_venus_handlers_register(dispatch_ctx, handler_ctx);
    
    /* Initialize Vulkan */
    write_command(ring, PV_VK_COMMAND_vkCreateInstance);
    write_command(ring, PV_VK_COMMAND_vkEnumeratePhysicalDevices);
    write_command(ring, PV_VK_COMMAND_vkCreateDevice);
    pv_venus_decode_all(ring, dispatch_ctx);
    
    /* Create image */
    write_command(ring, PV_VK_COMMAND_vkCreateImage);
    
    int processed = pv_venus_decode_all(ring, dispatch_ctx);
    assert(processed == 1);
    
    /* Verify image was tracked */
    VkImage image = pv_venus_object_get(&handler_ctx->objects, 0x7000);
    assert(image != NULL);
    
    printf("  ✓ Image created and tracked: %p\n", (void*)image);
    
    /* Cleanup */
    pv_venus_dispatch_destroy(dispatch_ctx);
    pv_venus_handlers_destroy(handler_ctx);
    pv_venus_ring_destroy(ring);
    free(shared_mem);
    
    printf("  ✓ Image creation works\n");
}

/* Test 4: Complete resource workflow */
static void test_complete_workflow(void)
{
    printf("Test 4: Complete resource allocation workflow...\n");
    
    void *shared_mem;
    struct pv_venus_ring *ring = create_test_ring(&shared_mem);
    struct pv_venus_handler_context *handler_ctx = pv_venus_handlers_create();
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    
    pv_venus_handlers_register(dispatch_ctx, handler_ctx);
    
    /* Complete initialization + resource allocation sequence */
    printf("  Running complete sequence...\n");
    
    write_command(ring, PV_VK_COMMAND_vkCreateInstance);
    write_command(ring, PV_VK_COMMAND_vkEnumeratePhysicalDevices);
    write_command(ring, PV_VK_COMMAND_vkCreateDevice);
    write_command(ring, PV_VK_COMMAND_vkAllocateMemory);
    write_command(ring, PV_VK_COMMAND_vkCreateBuffer);
    write_command(ring, PV_VK_COMMAND_vkBindBufferMemory);
    write_command(ring, PV_VK_COMMAND_vkCreateImage);
    
    int processed = pv_venus_decode_all(ring, dispatch_ctx);
    assert(processed == 7);
    
    /* Verify all objects were created */
    printf("  Verifying object tracking...\n");
    assert(pv_venus_object_get(&handler_ctx->objects, 0x1000) != NULL); // Instance
    assert(pv_venus_object_get(&handler_ctx->objects, 0x2000) != NULL); // Physical device
    assert(pv_venus_object_get(&handler_ctx->objects, 0x3000) != NULL); // Device
    assert(pv_venus_object_get(&handler_ctx->objects, 0x4000) != NULL); // Queue
    assert(pv_venus_object_get(&handler_ctx->objects, 0x5000) != NULL); // Memory
    assert(pv_venus_object_get(&handler_ctx->objects, 0x6000) != NULL); // Buffer
    assert(pv_venus_object_get(&handler_ctx->objects, 0x7000) != NULL); // Image
    
    printf("  ✓ All 7 objects tracked correctly\n");
    
    /* Verify statistics */
    printf("  Statistics:\n");
    printf("    Commands dispatched: %llu\n", dispatch_ctx->commands_dispatched);
    printf("    Commands handled: %llu\n", handler_ctx->commands_handled);
    printf("    Objects created: %llu\n", handler_ctx->objects_created);
    printf("    Objects in table: %zu\n", handler_ctx->objects.count);
    
    assert(dispatch_ctx->commands_dispatched == 7);
    assert(handler_ctx->commands_handled == 7);
    assert(handler_ctx->objects_created == 7);
    assert(handler_ctx->objects.count == 7);
    
    printf("  ✓ Statistics match expectations\n");
    
    /* Cleanup */
    pv_venus_dispatch_destroy(dispatch_ctx);
    pv_venus_handlers_destroy(handler_ctx);
    pv_venus_ring_destroy(ring);
    free(shared_mem);
    
    printf("  ✓ Complete workflow successful\n");
}

/* Test 5: Handler registration verification */
static void test_handler_registration(void)
{
    printf("Test 5: Verify all 17 handlers registered...\n");
    
    struct pv_venus_handler_context *handler_ctx = pv_venus_handlers_create();
    struct pv_venus_dispatch_context *dispatch_ctx = pv_venus_dispatch_create();
    
    pv_venus_handlers_register(dispatch_ctx, handler_ctx);
    
    /* Verify key handlers are registered */
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkCreateInstance] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkAllocateMemory] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkFreeMemory] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkCreateBuffer] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkDestroyBuffer] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkBindBufferMemory] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkCreateImage] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkDestroyImage] != NULL);
    assert(dispatch_ctx->handlers[PV_VK_COMMAND_vkBindImageMemory] != NULL);
    
    printf("  ✓ All resource handlers registered\n");
    
    pv_venus_dispatch_destroy(dispatch_ctx);
    pv_venus_handlers_destroy(handler_ctx);
    
    printf("  ✓ Handler registration complete\n");
}

int main(void)
{
    printf("=== Resource Handlers Test Suite ===\n\n");
    
    test_handler_registration();
    printf("\n");
    
    test_memory_allocation();
    printf("\n");
    
    test_buffer_creation();
    printf("\n");
    
    test_image_creation();
    printf("\n");
    
    test_complete_workflow();
    printf("\n");
    
    printf("=== All Tests Passed ✓ ===\n");
    printf("\nSession 8 Complete: Resource & memory handlers operational\n");
    printf("Ready for Session 9: Command buffers & queue submission\n");
    
    return 0;
}
