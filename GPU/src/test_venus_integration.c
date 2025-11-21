/*
 * test_venus_integration.c
 * Test suite for Venus integration API
 * 
 * Tests the bridge between Swift and C Venus subsystem
 */

#include "pv_venus_integration.h"
#include "pv_venus_ring.h"
#include "pv_venus_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Test 1: Shared memory ring buffer creation */
static void test_shared_memory_ring(void) {
    printf("\n=== Test 1: Shared Memory Ring Buffer ===\n");
    
    /* Allocate 4MB of memory (like Swift will do) */
    size_t size = 4 * 1024 * 1024;
    void *memory = aligned_alloc(4096, size);
    assert(memory != NULL);
    memset(memory, 0, size);
    
    printf("Allocated %zu bytes at %p\n", size, memory);
    
    /* Create ring buffer from memory */
    struct pv_venus_ring *ring = pv_venus_ring_create_from_memory(memory, (uint32_t)size);
    assert(ring != NULL);
    printf("  ✓ Ring buffer created from shared memory\n");
    
    /* Verify ring buffer properties */
    assert(ring->buffer == memory);
    assert(ring->size == size);
    assert(ring->read_pos == 0);
    assert(ring->write_pos == 0);
    printf("  ✓ Ring buffer properties correct\n");
    
    /* Cleanup */
    pv_venus_ring_destroy(ring);
    free(memory);
    
    printf("Test 1 passed!\n");
}

/* Test 2: Venus context initialization */
static void test_venus_init(void) {
    printf("\n=== Test 2: Venus Context Initialization ===\n");
    
    /* Initialize Venus (MoltenVK + handlers) */
    void *ctx = pv_venus_init();
    assert(ctx != NULL);
    printf("  ✓ Venus context initialized\n");
    
    /* Get initial statistics */
    struct pv_venus_stats stats = pv_venus_get_stats(ctx);
    printf("Initial statistics:\n");
    printf("  Commands handled: %u\n", stats.commands_handled);
    printf("  Objects created: %u\n", stats.objects_created);
    printf("  Errors: %u\n", stats.errors);
    assert(stats.commands_handled == 0);
    assert(stats.objects_created == 0);
    printf("  ✓ Statistics initialized correctly\n");
    
    /* Cleanup */
    pv_venus_cleanup(ctx);
    printf("  ✓ Venus context cleaned up\n");
    
    printf("Test 2 passed!\n");
}

/* Test 3: Complete integration flow */
static void test_integration_flow(void) {
    printf("\n=== Test 3: Complete Integration Flow ===\n");
    
    /* Step 1: Allocate shared memory */
    size_t size = 1 * 1024 * 1024;  // 1MB
    void *memory = aligned_alloc(4096, size);
    assert(memory != NULL);
    memset(memory, 0, size);
    printf("Step 1: Allocated shared memory (%zu bytes)\n", size);
    
    /* Step 2: Create ring buffer */
    struct pv_venus_ring *ring = pv_venus_ring_create_from_memory(memory, (uint32_t)size);
    assert(ring != NULL);
    printf("Step 2: Created ring buffer\n");
    
    /* Step 3: Initialize Venus context */
    void *ctx = pv_venus_init();
    assert(ctx != NULL);
    printf("Step 3: Initialized Venus context\n");
    
    /* Step 4: Start ring buffer processing */
    int result = pv_venus_integration_start(ring, ctx);
    assert(result == 0);
    printf("Step 4: Started ring buffer processing\n");
    
    /* Step 5: Write some test commands to ring buffer (after header region) */
    /* Ring layout: [head(4)][tail(4)][status(4)][padding(4)][buffer...] */
    uint8_t *buffer_region = (uint8_t *)memory + 16;  // Skip control region
    uint32_t *tail_ptr = (uint32_t *)((uint8_t *)memory + 4);  // Tail at offset 4
    uint32_t write_offset = 0;
    
    /* Command 1: vkCreateInstance */
    struct pv_venus_command_header *cmd = (struct pv_venus_command_header *)(buffer_region + write_offset);
    cmd->command_id = PV_VK_COMMAND_vkCreateInstance;
    cmd->command_size = sizeof(struct pv_venus_command_header);
    write_offset += cmd->command_size;
    
    /* Command 2: vkEnumeratePhysicalDevices */
    cmd = (struct pv_venus_command_header *)(buffer_region + write_offset);
    cmd->command_id = PV_VK_COMMAND_vkEnumeratePhysicalDevices;
    cmd->command_size = sizeof(struct pv_venus_command_header);
    write_offset += cmd->command_size;
    
    /* Command 3: vkCreateDevice */
    cmd = (struct pv_venus_command_header *)(buffer_region + write_offset);
    cmd->command_id = PV_VK_COMMAND_vkCreateDevice;
    cmd->command_size = sizeof(struct pv_venus_command_header);
    write_offset += cmd->command_size;
    
    /* Update tail pointer to indicate available commands */
    *tail_ptr = write_offset;
    
    printf("Step 5: Wrote 3 test commands to ring buffer\n");
    
    /* Step 6: Notify ring buffer to process commands */
    pv_venus_ring_notify(ring);
    printf("Step 6: Notified ring buffer (processed commands)\n");
    
    /* Step 7: Check statistics */
    struct pv_venus_stats stats = pv_venus_get_stats(ctx);
    printf("Statistics after processing:\n");
    printf("  Commands handled: %u\n", stats.commands_handled);
    printf("  Objects created: %u\n", stats.objects_created);
    assert(stats.commands_handled == 3);
    assert(stats.objects_created == 3);  // Instance, physical device, device
    printf("  ✓ All commands processed correctly\n");
    
    /* Step 8: Check ring buffer utilization */
    double util = pv_venus_ring_utilization(ring);
    printf("Ring buffer utilization: %.2f%%\n", util * 100.0);
    
    /* Step 9: Cleanup */
    pv_venus_integration_stop(ring);
    pv_venus_ring_destroy(ring);
    pv_venus_cleanup(ctx);
    free(memory);
    printf("Step 9: Cleaned up all resources\n");
    
    printf("Test 3 passed!\n");
}

/* Test 4: Ring buffer notification and processing */
static void test_ring_notification(void) {
    printf("\n=== Test 4: Ring Buffer Notification ===\n");
    
    /* Setup */
    size_t size = 256 * 1024;  // 256KB
    void *memory = aligned_alloc(4096, size);
    assert(memory != NULL);
    memset(memory, 0, size);
    
    struct pv_venus_ring *ring = pv_venus_ring_create_from_memory(memory, (uint32_t)size);
    void *ctx = pv_venus_init();
    pv_venus_integration_start(ring, ctx);
    
    /* Get buffer region and tail pointer */
    uint8_t *buffer_region = (uint8_t *)memory + 16;
    uint32_t *tail_ptr = (uint32_t *)((uint8_t *)memory + 4);
    uint32_t write_offset = 0;
    
    /* Write multiple batches of commands */
    for (int batch = 0; batch < 3; batch++) {
        printf("Batch %d: Writing commands...\n", batch + 1);
        
        /* Write 5 commands per batch */
        for (int i = 0; i < 5; i++) {
            struct pv_venus_command_header *cmd = 
                (struct pv_venus_command_header *)(buffer_region + write_offset);
            
            cmd->command_id = PV_VK_COMMAND_vkCreateInstance + i;
            cmd->command_size = sizeof(struct pv_venus_command_header);
            write_offset += cmd->command_size;
        }
        
        /* Update tail and notify */
        *tail_ptr = write_offset;
        pv_venus_ring_notify(ring);
        
        struct pv_venus_stats stats = pv_venus_get_stats(ctx);
        printf("  Processed: %u commands total\n", stats.commands_handled);
    }
    
    /* Final statistics */
    struct pv_venus_stats final_stats = pv_venus_get_stats(ctx);
    printf("Final statistics:\n");
    printf("  Total commands: %u\n", final_stats.commands_handled);
    printf("  Total objects: %u\n", final_stats.objects_created);
    assert(final_stats.commands_handled == 15);  // 3 batches × 5 commands
    printf("  ✓ All batches processed correctly\n");
    
    /* Cleanup */
    pv_venus_integration_stop(ring);
    pv_venus_ring_destroy(ring);
    pv_venus_cleanup(ctx);
    free(memory);
    
    printf("Test 4 passed!\n");
}

/* Main test runner */
int main(void) {
    printf("===========================================\n");
    printf("  Venus Integration Test Suite\n");
    printf("===========================================\n");
    
    test_shared_memory_ring();
    test_venus_init();
    test_integration_flow();
    test_ring_notification();
    
    printf("\n===========================================\n");
    printf("All tests passed! ✅\n");
    printf("===========================================\n");
    
    return 0;
}
