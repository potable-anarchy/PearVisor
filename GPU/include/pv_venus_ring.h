/*
 * PearVisor - Venus Protocol Ring Buffer
 * 
 * Ring buffer implementation for Venus protocol communication
 * between guest VM and host renderer.
 */

#ifndef PV_VENUS_RING_H
#define PV_VENUS_RING_H

#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Ring buffer status flags */
#define PV_VENUS_RING_STATUS_IDLE       0x0
#define PV_VENUS_RING_STATUS_RUNNING    0x1
#define PV_VENUS_RING_STATUS_ERROR      0x2

/* Ring buffer control region */
struct pv_venus_ring_control {
    volatile atomic_uint *head;      /* Renderer position (we write) */
    const volatile atomic_uint *tail;/* Guest position (guest writes) */
    volatile atomic_uint *status;    /* Ring status flags */
};

/* Ring buffer data region */
struct pv_venus_ring_buffer {
    uint32_t size;                   /* Buffer size (must be power of 2) */
    uint32_t mask;                   /* size - 1, for fast wrapping */
    uint32_t current_pos;            /* Current read position */
    const uint8_t *data;             /* Pointer to buffer data */
};

/* Ring buffer extra region (for large structures) */
struct pv_venus_ring_extra {
    void *data;                      /* Pointer to extra region */
    size_t size;                     /* Size of extra region */
    size_t offset;                   /* Current offset */
};

/* Ring buffer statistics */
struct pv_venus_ring_stats {
    uint64_t commands_processed;     /* Total commands processed */
    uint64_t bytes_read;             /* Total bytes read from ring */
    uint64_t errors;                 /* Number of errors encountered */
    uint64_t waits;                  /* Number of times we waited for data */
};

/* Venus ring buffer */
struct pv_venus_ring {
    /* Ring regions */
    struct pv_venus_ring_control control;
    struct pv_venus_ring_buffer buffer;
    struct pv_venus_ring_extra extra;
    
    /* Command processing thread */
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool running;
    
    /* Statistics */
    struct pv_venus_ring_stats stats;
    
    /* Context for command dispatch */
    void *dispatch_context;
};

/* Ring buffer layout (for initialization) */
struct pv_venus_ring_layout {
    /* Shared memory pointers */
    void *shared_memory;
    size_t shared_memory_size;
    
    /* Region offsets in shared memory */
    size_t head_offset;
    size_t tail_offset;
    size_t status_offset;
    size_t buffer_offset;
    size_t buffer_size;
    size_t extra_offset;
    size_t extra_size;
};

/*
 * Create a Venus ring buffer
 * 
 * @layout: Memory layout configuration
 * @dispatch_context: Context passed to command handlers
 * Returns: Allocated ring buffer, or NULL on failure
 */
struct pv_venus_ring *pv_venus_ring_create(
    const struct pv_venus_ring_layout *layout,
    void *dispatch_context
);

/*
 * Destroy a Venus ring buffer
 * 
 * @ring: Ring buffer to destroy
 */
void pv_venus_ring_destroy(struct pv_venus_ring *ring);

/*
 * Start ring buffer processing thread
 * 
 * @ring: Ring buffer to start
 * Returns: 0 on success, negative on failure
 */
int pv_venus_ring_start(struct pv_venus_ring *ring);

/*
 * Stop ring buffer processing thread
 * 
 * @ring: Ring buffer to stop
 * Returns: 0 on success, negative on failure
 */
int pv_venus_ring_stop(struct pv_venus_ring *ring);

/*
 * Notify ring that new commands are available
 * 
 * @ring: Ring buffer to notify
 */
void pv_venus_ring_notify(struct pv_venus_ring *ring);

/*
 * Read data from ring buffer
 * 
 * @ring: Ring buffer to read from
 * @data: Destination buffer
 * @size: Number of bytes to read
 * Returns: 0 on success, negative on failure
 */
int pv_venus_ring_read(struct pv_venus_ring *ring, void *data, size_t size);

/*
 * Get pointer to data in extra region
 * 
 * @ring: Ring buffer
 * @offset: Offset in extra region
 * @size: Expected size of data
 * Returns: Pointer to data, or NULL if invalid offset
 */
const void *pv_venus_ring_get_extra(
    struct pv_venus_ring *ring,
    size_t offset,
    size_t size
);

/*
 * Get current head position
 * 
 * @ring: Ring buffer
 * Returns: Current head value
 */
static inline uint32_t pv_venus_ring_get_head(const struct pv_venus_ring *ring)
{
    return atomic_load_explicit(ring->control.head, memory_order_acquire);
}

/*
 * Get current tail position
 * 
 * @ring: Ring buffer
 * Returns: Current tail value
 */
static inline uint32_t pv_venus_ring_get_tail(const struct pv_venus_ring *ring)
{
    return atomic_load_explicit(ring->control.tail, memory_order_acquire);
}

/*
 * Update head position (after consuming commands)
 * 
 * @ring: Ring buffer
 * @new_head: New head value
 */
static inline void pv_venus_ring_set_head(struct pv_venus_ring *ring, uint32_t new_head)
{
    atomic_store_explicit(ring->control.head, new_head, memory_order_release);
}

/*
 * Get number of bytes available to read
 * 
 * @ring: Ring buffer
 * Returns: Number of bytes available
 */
static inline uint32_t pv_venus_ring_available(const struct pv_venus_ring *ring)
{
    uint32_t tail = pv_venus_ring_get_tail(ring);
    uint32_t head = ring->buffer.current_pos;
    
    /* Handle wrapping */
    if (tail >= head) {
        return tail - head;
    } else {
        return ring->buffer.size - head + tail;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* PV_VENUS_RING_H */
