/*
 * PearVisor - Venus Protocol Ring Buffer Implementation
 */

#include "pv_venus_ring.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

/* Forward declaration of command processing thread */
static void *pv_venus_ring_thread(void *arg);

/* Check if a value is a power of 2 */
static inline bool is_power_of_two(uint32_t value)
{
    return value != 0 && (value & (value - 1)) == 0;
}

/*
 * Create a Venus ring buffer
 */
struct pv_venus_ring *pv_venus_ring_create(
    const struct pv_venus_ring_layout *layout,
    void *dispatch_context)
{
    if (!layout || !layout->shared_memory) {
        fprintf(stderr, "[Venus Ring] Invalid layout\n");
        return NULL;
    }

    /* Validate buffer size is power of 2 */
    if (!is_power_of_two(layout->buffer_size)) {
        fprintf(stderr, "[Venus Ring] Buffer size must be power of 2\n");
        return NULL;
    }

    /* Allocate ring structure */
    struct pv_venus_ring *ring = calloc(1, sizeof(*ring));
    if (!ring) {
        fprintf(stderr, "[Venus Ring] Failed to allocate ring\n");
        return NULL;
    }

    /* Setup control region */
    uint8_t *base = (uint8_t *)layout->shared_memory;
    ring->control.head = (atomic_uint *)(base + layout->head_offset);
    ring->control.tail = (atomic_uint *)(base + layout->tail_offset);
    ring->control.status = (atomic_uint *)(base + layout->status_offset);

    /* Initialize head and status to 0 */
    atomic_store_explicit(ring->control.head, 0, memory_order_relaxed);
    atomic_store_explicit(ring->control.status, PV_VENUS_RING_STATUS_IDLE, 
                         memory_order_relaxed);

    /* Setup buffer region */
    ring->buffer.size = layout->buffer_size;
    ring->buffer.mask = layout->buffer_size - 1;
    ring->buffer.current_pos = 0;
    ring->buffer.data = base + layout->buffer_offset;

    /* Setup extra region */
    if (layout->extra_size > 0) {
        ring->extra.data = base + layout->extra_offset;
        ring->extra.size = layout->extra_size;
        ring->extra.offset = 0;
    }

    /* Initialize threading primitives */
    if (pthread_mutex_init(&ring->mutex, NULL) != 0) {
        fprintf(stderr, "[Venus Ring] Failed to init mutex\n");
        free(ring);
        return NULL;
    }

    if (pthread_cond_init(&ring->cond, NULL) != 0) {
        fprintf(stderr, "[Venus Ring] Failed to init cond\n");
        pthread_mutex_destroy(&ring->mutex);
        free(ring);
        return NULL;
    }

    ring->running = false;
    ring->dispatch_context = dispatch_context;

    /* Initialize stats */
    memset(&ring->stats, 0, sizeof(ring->stats));

    printf("[Venus Ring] Created ring buffer: size=%u extra=%zu\n",
           ring->buffer.size, ring->extra.size);

    return ring;
}

/*
 * Destroy a Venus ring buffer
 */
void pv_venus_ring_destroy(struct pv_venus_ring *ring)
{
    if (!ring) {
        return;
    }

    /* Stop thread if running */
    if (ring->running) {
        pv_venus_ring_stop(ring);
    }

    /* Cleanup threading primitives */
    pthread_mutex_destroy(&ring->mutex);
    pthread_cond_destroy(&ring->cond);

    /* Print final stats */
    printf("[Venus Ring] Final stats: commands=%llu bytes=%llu errors=%llu waits=%llu\n",
           ring->stats.commands_processed,
           ring->stats.bytes_read,
           ring->stats.errors,
           ring->stats.waits);

    free(ring);
}

/*
 * Read data from ring buffer
 */
int pv_venus_ring_read(struct pv_venus_ring *ring, void *dest, size_t size)
{
    if (!ring || !dest || size == 0) {
        return -1;
    }

    uint8_t *dst = (uint8_t *)dest;
    size_t remaining = size;
    uint32_t pos = ring->buffer.current_pos;

    while (remaining > 0) {
        /* Calculate how much we can read in one chunk (until wrap) */
        uint32_t pos_masked = pos & ring->buffer.mask;
        uint32_t available_to_wrap = ring->buffer.size - pos_masked;
        size_t chunk = (remaining < available_to_wrap) ? remaining : available_to_wrap;

        /* Copy chunk */
        memcpy(dst, ring->buffer.data + pos_masked, chunk);

        /* Update pointers */
        dst += chunk;
        pos += chunk;
        remaining -= chunk;
    }

    /* Update current position */
    ring->buffer.current_pos = pos;
    ring->stats.bytes_read += size;

    return 0;
}

/*
 * Get pointer to data in extra region
 */
const void *pv_venus_ring_get_extra(
    struct pv_venus_ring *ring,
    size_t offset,
    size_t size)
{
    if (!ring || !ring->extra.data) {
        return NULL;
    }

    /* Validate offset and size */
    if (offset + size > ring->extra.size) {
        fprintf(stderr, "[Venus Ring] Extra region access out of bounds: "
                "offset=%zu size=%zu max=%zu\n",
                offset, size, ring->extra.size);
        ring->stats.errors++;
        return NULL;
    }

    return (const uint8_t *)ring->extra.data + offset;
}

/*
 * Command processing thread
 */
static void *pv_venus_ring_thread(void *arg)
{
    struct pv_venus_ring *ring = (struct pv_venus_ring *)arg;

    printf("[Venus Ring] Thread started\n");

    /* Set status to running */
    atomic_store_explicit(ring->control.status, PV_VENUS_RING_STATUS_RUNNING,
                         memory_order_relaxed);

    while (ring->running) {
        /* Get current tail (guest's write position) */
        uint32_t tail = pv_venus_ring_get_tail(ring);
        uint32_t head = ring->buffer.current_pos;

        /* Check if we have data to process */
        if (head == tail) {
            /* No data available, wait for notification */
            pthread_mutex_lock(&ring->mutex);
            
            /* Double-check tail after acquiring lock */
            tail = pv_venus_ring_get_tail(ring);
            if (head == tail && ring->running) {
                ring->stats.waits++;
                
                /* Wait with timeout (1 second) */
                struct timespec timeout;
                clock_gettime(CLOCK_REALTIME, &timeout);
                timeout.tv_sec += 1;
                
                pthread_cond_timedwait(&ring->cond, &ring->mutex, &timeout);
            }
            
            pthread_mutex_unlock(&ring->mutex);
            continue;
        }

        /* TODO: Process commands here */
        /* For now, just skip to demonstrate ring buffer operation */
        printf("[Venus Ring] Data available: head=%u tail=%u available=%u bytes\n",
               head, tail, pv_venus_ring_available(ring));

        /* Simulate processing by advancing head */
        ring->buffer.current_pos = tail;
        pv_venus_ring_set_head(ring, tail);
        ring->stats.commands_processed++;
        
        /* Small sleep to avoid busy loop in this stub */
        usleep(10000);  /* 10ms */
    }

    /* Set status back to idle */
    atomic_store_explicit(ring->control.status, PV_VENUS_RING_STATUS_IDLE,
                         memory_order_relaxed);

    printf("[Venus Ring] Thread stopped\n");
    return NULL;
}

/*
 * Start ring buffer processing thread
 */
int pv_venus_ring_start(struct pv_venus_ring *ring)
{
    if (!ring) {
        return -1;
    }

    if (ring->running) {
        fprintf(stderr, "[Venus Ring] Already running\n");
        return -1;
    }

    ring->running = true;

    if (pthread_create(&ring->thread, NULL, pv_venus_ring_thread, ring) != 0) {
        fprintf(stderr, "[Venus Ring] Failed to create thread\n");
        ring->running = false;
        return -1;
    }

    printf("[Venus Ring] Started processing thread\n");
    return 0;
}

/*
 * Stop ring buffer processing thread
 */
int pv_venus_ring_stop(struct pv_venus_ring *ring)
{
    if (!ring || !ring->running) {
        return -1;
    }

    printf("[Venus Ring] Stopping thread...\n");

    /* Signal thread to stop */
    ring->running = false;

    /* Wake up thread if it's waiting */
    pthread_mutex_lock(&ring->mutex);
    pthread_cond_signal(&ring->cond);
    pthread_mutex_unlock(&ring->mutex);

    /* Wait for thread to finish */
    pthread_join(ring->thread, NULL);

    printf("[Venus Ring] Thread stopped\n");
    return 0;
}

/*
 * Notify ring that new commands are available
 */
void pv_venus_ring_notify(struct pv_venus_ring *ring)
{
    if (!ring) {
        return;
    }

    /* Wake up processing thread */
    pthread_mutex_lock(&ring->mutex);
    pthread_cond_signal(&ring->cond);
    pthread_mutex_unlock(&ring->mutex);
}
