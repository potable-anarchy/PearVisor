/*
 * PearVisor - Venus Command Handlers
 * 
 * Implements Vulkan command handlers for Venus protocol
 */

#ifndef PV_VENUS_HANDLERS_H
#define PV_VENUS_HANDLERS_H

#include "pv_venus_protocol.h"
#include "pv_venus_decoder.h"
#include "pv_moltenvk.h"
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Object ID type
 * 
 * Venus uses 64-bit IDs to reference Vulkan objects.
 * We need to map these guest IDs to host Vulkan handles.
 */
typedef uint64_t pv_venus_object_id;

/*
 * Object types
 */
typedef enum {
    PV_VENUS_OBJECT_TYPE_INSTANCE = 0,
    PV_VENUS_OBJECT_TYPE_PHYSICAL_DEVICE,
    PV_VENUS_OBJECT_TYPE_DEVICE,
    PV_VENUS_OBJECT_TYPE_QUEUE,
    PV_VENUS_OBJECT_TYPE_SEMAPHORE,
    PV_VENUS_OBJECT_TYPE_FENCE,
    PV_VENUS_OBJECT_TYPE_DEVICE_MEMORY,
    PV_VENUS_OBJECT_TYPE_BUFFER,
    PV_VENUS_OBJECT_TYPE_IMAGE,
    PV_VENUS_OBJECT_TYPE_COMMAND_POOL,
    PV_VENUS_OBJECT_TYPE_COMMAND_BUFFER,
} pv_venus_object_type;

/*
 * Object table entry
 */
struct pv_venus_object {
    pv_venus_object_id guest_id;      /* ID from guest */
    void *host_handle;                 /* VkInstance, VkDevice, etc */
    pv_venus_object_type type;
    bool in_use;
};

/*
 * Object table for tracking guest ID → host handle mappings
 */
struct pv_venus_object_table {
    struct pv_venus_object *objects;
    size_t capacity;
    size_t count;
};

/*
 * Venus handler context
 * 
 * Contains MoltenVK context and object tracking
 */
struct pv_venus_handler_context {
    /* MoltenVK/Vulkan state */
    struct pv_moltenvk_context *vk;
    
    /* Object tracking */
    struct pv_venus_object_table objects;
    
    /* Statistics */
    uint64_t commands_handled;
    uint64_t objects_created;
    uint64_t objects_destroyed;
};

/*
 * Create handler context
 */
struct pv_venus_handler_context *pv_venus_handlers_create(void);

/*
 * Destroy handler context
 */
void pv_venus_handlers_destroy(struct pv_venus_handler_context *ctx);

/*
 * Register all command handlers with decoder
 */
void pv_venus_handlers_register(
    struct pv_venus_dispatch_context *dispatch_ctx,
    struct pv_venus_handler_context *handler_ctx
);

/*
 * Object table operations
 */

/* Add object to table */
int pv_venus_object_add(
    struct pv_venus_object_table *table,
    pv_venus_object_id guest_id,
    void *host_handle,
    pv_venus_object_type type
);

/* Get object from table */
void *pv_venus_object_get(
    struct pv_venus_object_table *table,
    pv_venus_object_id guest_id
);

/* Remove object from table */
void pv_venus_object_remove(
    struct pv_venus_object_table *table,
    pv_venus_object_id guest_id
);

/*
 * Command Handlers
 * 
 * Each handler receives:
 * - context: Dispatch context (contains handler_ctx as user data)
 * - header: Command header
 * - data: Command payload
 * - data_size: Payload size
 * 
 * Returns: 0 on success, negative on error
 */

/* Instance management */
int pv_venus_handle_vkCreateInstance(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkDestroyInstance(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkEnumeratePhysicalDevices(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

/* Physical device queries */
int pv_venus_handle_vkGetPhysicalDeviceProperties(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkGetPhysicalDeviceFeatures(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkGetPhysicalDeviceMemoryProperties(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

/* Device management */
int pv_venus_handle_vkCreateDevice(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkDestroyDevice(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkGetDeviceQueue(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

/* Memory management */
int pv_venus_handle_vkAllocateMemory(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkFreeMemory(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

/* Buffer management */
int pv_venus_handle_vkCreateBuffer(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkDestroyBuffer(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkBindBufferMemory(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

/* Image management */
int pv_venus_handle_vkCreateImage(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkDestroyImage(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkBindImageMemory(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

/* Command pool management */
int pv_venus_handle_vkCreateCommandPool(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkDestroyCommandPool(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

/* Command buffer management */
int pv_venus_handle_vkAllocateCommandBuffers(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkFreeCommandBuffers(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkBeginCommandBuffer(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkEndCommandBuffer(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

/* Queue operations */
int pv_venus_handle_vkQueueSubmit(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

int pv_venus_handle_vkQueueWaitIdle(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

#ifdef __cplusplus
}
#endif

#endif /* PV_VENUS_HANDLERS_H */
