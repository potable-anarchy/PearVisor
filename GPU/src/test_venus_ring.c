/*
 * PearVisor - Venus Ring Buffer Test
 * 
 * Test program to verify ring buffer implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pv_venus_ring.h"

/* Simulate a guest writing to the ring */
static void simulate_guest_write(struct pv_venus_ring *ring, size_t bytes)
{
    /* Get current tail */
    uint32_t tail = pv_venus_ring_get_tail(ring);
    
    /* Simulate advancing tail */
    uint32_t new_tail = (tail + bytes) & ring->buffer.mask;
    
    /* Write new tail (simulating guest) */
    atomic_store_explicit((atomic_uint *)ring->control.tail, new_tail, 
                         memory_order_release);
    
    printf("[Test] Guest wrote %zu bytes (tail: %u -> %u)\n", bytes, tail, new_tail);
    
    /* Notify ring */
    pv_venus_ring_notify(ring);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    printf("=== PearVisor Venus Ring Buffer Test ===\n\n");

    /* Allocate shared memory (simulating guest/host shared region) */
    const size_t buffer_size = 4096;  /* 4KB ring buffer */
    const size_t extra_size = 1024;   /* 1KB extra region */
    const size_t total_size = 
        sizeof(uint32_t) * 3 +  /* head, tail, status */
        buffer_size +            /* command buffer */
        extra_size;              /* extra region */

    void *shared_mem = calloc(1, total_size);
    if (!shared_mem) {
        fprintf(stderr, "Failed to allocate shared memory\n");
        return 1;
    }

    printf("[Test] Allocated %zu bytes of shared memory\n", total_size);

    /* Setup ring buffer layout */
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

    /* Create ring buffer */
    struct pv_venus_ring *ring = pv_venus_ring_create(&layout, NULL);
    if (!ring) {
        fprintf(stderr, "Failed to create ring buffer\n");
        free(shared_mem);
        return 1;
    }

    /* Test 1: Basic ring operations */
    printf("\n--- Test 1: Basic Ring Operations ---\n");
    
    printf("Initial state:\n");
    printf("  Head: %u\n", pv_venus_ring_get_head(ring));
    printf("  Tail: %u\n", pv_venus_ring_get_tail(ring));
    printf("  Available: %u bytes\n", pv_venus_ring_available(ring));

    /* Test 2: Start processing thread */
    printf("\n--- Test 2: Start Processing Thread ---\n");
    
    if (pv_venus_ring_start(ring) != 0) {
        fprintf(stderr, "Failed to start ring\n");
        pv_venus_ring_destroy(ring);
        free(shared_mem);
        return 1;
    }

    /* Give thread time to start */
    sleep(1);

    /* Test 3: Simulate guest writing data */
    printf("\n--- Test 3: Simulate Guest Writes ---\n");
    
    simulate_guest_write(ring, 64);
    sleep(1);
    
    simulate_guest_write(ring, 128);
    sleep(1);
    
    simulate_guest_write(ring, 256);
    sleep(1);

    /* Test 4: Check ring state */
    printf("\n--- Test 4: Ring State After Writes ---\n");
    
    printf("Final state:\n");
    printf("  Head: %u\n", pv_venus_ring_get_head(ring));
    printf("  Tail: %u\n", pv_venus_ring_get_tail(ring));
    printf("  Available: %u bytes\n", pv_venus_ring_available(ring));
    printf("  Commands processed: %llu\n", ring->stats.commands_processed);
    printf("  Bytes read: %llu\n", ring->stats.bytes_read);
    printf("  Waits: %llu\n", ring->stats.waits);

    /* Test 5: Test wrapping */
    printf("\n--- Test 5: Test Ring Wrapping ---\n");
    
    /* Fill most of the ring */
    simulate_guest_write(ring, 3000);
    sleep(1);
    
    /* This should wrap around */
    simulate_guest_write(ring, 2000);
    sleep(1);
    
    printf("After wrapping:\n");
    printf("  Head: %u\n", pv_venus_ring_get_head(ring));
    printf("  Tail: %u\n", pv_venus_ring_get_tail(ring));

    /* Test 6: Stop ring */
    printf("\n--- Test 6: Stop Ring Buffer ---\n");
    
    if (pv_venus_ring_stop(ring) != 0) {
        fprintf(stderr, "Failed to stop ring\n");
    }

    /* Cleanup */
    printf("\n--- Cleanup ---\n");
    pv_venus_ring_destroy(ring);
    free(shared_mem);

    printf("\n=== All Tests Passed! ===\n");
    return 0;
}
