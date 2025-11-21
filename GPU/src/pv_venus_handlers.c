/*
 * PearVisor - Venus Command Handlers Implementation
 */

#include "pv_venus_handlers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Create handler context
 */
struct pv_venus_handler_context *pv_venus_handlers_create(void)
{
    struct pv_venus_handler_context *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fprintf(stderr, "[Venus Handlers] Failed to allocate context\n");
        return NULL;
    }

    /* Create MoltenVK context */
    ctx->vk = pv_moltenvk_init();
    if (!ctx->vk) {
        free(ctx);
        return NULL;
    }

    /* Initialize object table */
    ctx->objects.capacity = 1024;  /* Start with 1024 objects */
    ctx->objects.objects = calloc(ctx->objects.capacity, 
                                   sizeof(struct pv_venus_object));
    if (!ctx->objects.objects) {
        pv_moltenvk_cleanup(ctx->vk);
        free(ctx);
        return NULL;
    }
    ctx->objects.count = 0;

    printf("[Venus Handlers] Context created\n");
    return ctx;
}

/*
 * Destroy handler context
 */
void pv_venus_handlers_destroy(struct pv_venus_handler_context *ctx)
{
    if (!ctx) {
        return;
    }

    printf("[Venus Handlers] Stats: handled=%llu created=%llu destroyed=%llu\n",
           ctx->commands_handled, ctx->objects_created, ctx->objects_destroyed);

    /* Cleanup MoltenVK */
    if (ctx->vk) {
        pv_moltenvk_cleanup(ctx->vk);
    }

    /* Free object table */
    if (ctx->objects.objects) {
        free(ctx->objects.objects);
    }

    free(ctx);
}

/*
 * Object table: Add object
 */
int pv_venus_object_add(
    struct pv_venus_object_table *table,
    pv_venus_object_id guest_id,
    void *host_handle,
    pv_venus_object_type type)
{
    if (!table || !host_handle) {
        return -1;
    }

    /* Find free slot */
    for (size_t i = 0; i < table->capacity; i++) {
        if (!table->objects[i].in_use) {
            table->objects[i].guest_id = guest_id;
            table->objects[i].host_handle = host_handle;
            table->objects[i].type = type;
            table->objects[i].in_use = true;
            table->count++;
            
            printf("[Venus Handlers] Added object: guest_id=0x%llx type=%d\n",
                   guest_id, type);
            return 0;
        }
    }

    fprintf(stderr, "[Venus Handlers] Object table full!\n");
    return -1;
}

/*
 * Object table: Get object
 */
void *pv_venus_object_get(
    struct pv_venus_object_table *table,
    pv_venus_object_id guest_id)
{
    if (!table) {
        return NULL;
    }

    for (size_t i = 0; i < table->capacity; i++) {
        if (table->objects[i].in_use && 
            table->objects[i].guest_id == guest_id) {
            return table->objects[i].host_handle;
        }
    }

    return NULL;
}

/*
 * Object table: Remove object
 */
void pv_venus_object_remove(
    struct pv_venus_object_table *table,
    pv_venus_object_id guest_id)
{
    if (!table) {
        return;
    }

    for (size_t i = 0; i < table->capacity; i++) {
        if (table->objects[i].in_use && 
            table->objects[i].guest_id == guest_id) {
            table->objects[i].in_use = false;
            table->objects[i].host_handle = NULL;
            table->count--;
            
            printf("[Venus Handlers] Removed object: guest_id=0x%llx\n", guest_id);
            return;
        }
    }
}

/*
 * Handler: vkCreateInstance
 */
int pv_venus_handle_vkCreateInstance(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkCreateInstance called\n");

    /* Create Vulkan instance via MoltenVK */
    VkResult result = pv_moltenvk_create_instance(ctx->vk, "PearVisor Guest");
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[Venus Handlers] Failed to create instance: %d\n", result);
        return -1;
    }

    /* TODO: Parse guest_id from command data and add to object table */
    /* For now, use fixed ID for testing */
    pv_venus_object_id guest_instance_id = 0x1000;
    
    pv_venus_object_add(&ctx->objects, guest_instance_id, 
                         ctx->vk->instance, PV_VENUS_OBJECT_TYPE_INSTANCE);

    ctx->commands_handled++;
    ctx->objects_created++;

    return 0;
}

/*
 * Handler: vkDestroyInstance
 */
int pv_venus_handle_vkDestroyInstance(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkDestroyInstance called\n");

    /* TODO: Parse guest_id and remove from object table */
    /* For now, just mark in stats */
    ctx->commands_handled++;
    ctx->objects_destroyed++;

    return 0;
}

/*
 * Handler: vkEnumeratePhysicalDevices
 */
int pv_venus_handle_vkEnumeratePhysicalDevices(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkEnumeratePhysicalDevices called\n");

    /* Select physical device via MoltenVK */
    VkResult result = pv_moltenvk_select_physical_device(ctx->vk);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[Venus Handlers] Failed to select physical device: %d\n", 
                result);
        return -1;
    }

    /* TODO: Parse command data and return device list to guest */
    /* For now, just add to object table */
    pv_venus_object_id guest_device_id = 0x2000;
    
    pv_venus_object_add(&ctx->objects, guest_device_id,
                         ctx->vk->physical_device, 
                         PV_VENUS_OBJECT_TYPE_PHYSICAL_DEVICE);

    ctx->commands_handled++;
    ctx->objects_created++;

    return 0;
}

/*
 * Handler: vkGetPhysicalDeviceProperties
 */
int pv_venus_handle_vkGetPhysicalDeviceProperties(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkGetPhysicalDeviceProperties called\n");
    printf("[Venus Handlers]   Device: %s\n", ctx->vk->device_properties.deviceName);

    /* TODO: Write properties back to guest shared memory */
    
    ctx->commands_handled++;
    return 0;
}

/*
 * Handler: vkGetPhysicalDeviceFeatures
 */
int pv_venus_handle_vkGetPhysicalDeviceFeatures(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkGetPhysicalDeviceFeatures called\n");

    /* TODO: Write features back to guest */
    
    ctx->commands_handled++;
    return 0;
}

/*
 * Handler: vkGetPhysicalDeviceMemoryProperties
 */
int pv_venus_handle_vkGetPhysicalDeviceMemoryProperties(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkGetPhysicalDeviceMemoryProperties called\n");
    printf("[Venus Handlers]   Memory heaps: %u\n", 
           ctx->vk->memory_properties.memoryHeapCount);

    /* TODO: Write memory properties back to guest */
    
    ctx->commands_handled++;
    return 0;
}

/*
 * Handler: vkCreateDevice
 */
int pv_venus_handle_vkCreateDevice(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkCreateDevice called\n");

    /* Create logical device via MoltenVK */
    VkResult result = pv_moltenvk_create_device(ctx->vk);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[Venus Handlers] Failed to create device: %d\n", result);
        return -1;
    }

    /* TODO: Parse guest_id from command data */
    pv_venus_object_id guest_device_id = 0x3000;
    
    pv_venus_object_add(&ctx->objects, guest_device_id,
                         ctx->vk->device, PV_VENUS_OBJECT_TYPE_DEVICE);

    ctx->commands_handled++;
    ctx->objects_created++;

    return 0;
}

/*
 * Handler: vkDestroyDevice
 */
int pv_venus_handle_vkDestroyDevice(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkDestroyDevice called\n");

    /* TODO: Parse guest_id and remove from object table */
    
    ctx->commands_handled++;
    ctx->objects_destroyed++;

    return 0;
}

/*
 * Handler: vkGetDeviceQueue
 */
int pv_venus_handle_vkGetDeviceQueue(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkGetDeviceQueue called\n");

    /* TODO: Parse queue family index and queue index from command */
    /* For now, just add graphics queue */
    pv_venus_object_id guest_queue_id = 0x4000;
    
    pv_venus_object_add(&ctx->objects, guest_queue_id,
                         ctx->vk->graphics_queue, PV_VENUS_OBJECT_TYPE_QUEUE);

    ctx->commands_handled++;
    ctx->objects_created++;

    return 0;
}

/*
 * Memory Management Handlers
 */

int pv_venus_handle_vkAllocateMemory(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkAllocateMemory called\n");

    /* TODO: Parse VkMemoryAllocateInfo from command data */
    /* For now, allocate 1MB of device memory as test */
    
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = 1024 * 1024,  // 1MB
        .memoryTypeIndex = 0,            // First memory type
    };

    VkDeviceMemory memory;
    VkResult result = vkAllocateMemory(ctx->vk->device, &alloc_info, NULL, &memory);
    
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[Venus Handlers] vkAllocateMemory failed: %d\n", result);
        return -1;
    }

    /* Track with fixed guest ID for testing */
    pv_venus_object_id guest_memory_id = 0x5000;
    pv_venus_object_add(&ctx->objects, guest_memory_id, memory,
                         PV_VENUS_OBJECT_TYPE_DEVICE_MEMORY);

    printf("[Venus Handlers]   Allocated %zu bytes of device memory\n", 
           alloc_info.allocationSize);

    ctx->commands_handled++;
    ctx->objects_created++;

    return 0;
}

int pv_venus_handle_vkFreeMemory(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkFreeMemory called\n");

    /* TODO: Parse guest memory ID and free actual memory */
    /* For now, just track statistics */

    ctx->commands_handled++;
    ctx->objects_destroyed++;

    return 0;
}

/*
 * Buffer Management Handlers
 */

int pv_venus_handle_vkCreateBuffer(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkCreateBuffer called\n");

    /* TODO: Parse VkBufferCreateInfo from command data */
    /* For now, create 64KB vertex buffer as test */
    
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = 64 * 1024,  // 64KB
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
    };

    VkBuffer buffer;
    VkResult result = vkCreateBuffer(ctx->vk->device, &buffer_info, NULL, &buffer);
    
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[Venus Handlers] vkCreateBuffer failed: %d\n", result);
        return -1;
    }

    /* Track with fixed guest ID for testing */
    pv_venus_object_id guest_buffer_id = 0x6000;
    pv_venus_object_add(&ctx->objects, guest_buffer_id, buffer,
                         PV_VENUS_OBJECT_TYPE_BUFFER);

    printf("[Venus Handlers]   Created buffer: %zu bytes\n", buffer_info.size);

    ctx->commands_handled++;
    ctx->objects_created++;

    return 0;
}

int pv_venus_handle_vkDestroyBuffer(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkDestroyBuffer called\n");

    /* TODO: Parse guest buffer ID and destroy actual buffer */
    /* For now, just track statistics */

    ctx->commands_handled++;
    ctx->objects_destroyed++;

    return 0;
}

int pv_venus_handle_vkBindBufferMemory(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkBindBufferMemory called\n");

    /* TODO: Parse buffer ID, memory ID, and offset from command data */
    /* For now, use fixed IDs from previous allocations */
    
    VkBuffer buffer = pv_venus_object_get(&ctx->objects, 0x6000);
    VkDeviceMemory memory = pv_venus_object_get(&ctx->objects, 0x5000);

    if (!buffer || !memory) {
        fprintf(stderr, "[Venus Handlers] Buffer or memory not found\n");
        return -1;
    }

    VkResult result = vkBindBufferMemory(ctx->vk->device, buffer, memory, 0);
    
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[Venus Handlers] vkBindBufferMemory failed: %d\n", result);
        return -1;
    }

    printf("[Venus Handlers]   Bound buffer to memory\n");

    ctx->commands_handled++;

    return 0;
}

/*
 * Image Management Handlers
 */

int pv_venus_handle_vkCreateImage(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkCreateImage called\n");

    /* TODO: Parse VkImageCreateInfo from command data */
    /* For now, create 512x512 RGBA8 texture as test */
    
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {
            .width = 512,
            .height = 512,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkImage image;
    VkResult result = vkCreateImage(ctx->vk->device, &image_info, NULL, &image);
    
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[Venus Handlers] vkCreateImage failed: %d\n", result);
        return -1;
    }

    /* Track with fixed guest ID for testing */
    pv_venus_object_id guest_image_id = 0x7000;
    pv_venus_object_add(&ctx->objects, guest_image_id, image,
                         PV_VENUS_OBJECT_TYPE_IMAGE);

    printf("[Venus Handlers]   Created image: %ux%u\n", 
           image_info.extent.width, image_info.extent.height);

    ctx->commands_handled++;
    ctx->objects_created++;

    return 0;
}

int pv_venus_handle_vkDestroyImage(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkDestroyImage called\n");

    /* TODO: Parse guest image ID and destroy actual image */
    /* For now, just track statistics */

    ctx->commands_handled++;
    ctx->objects_destroyed++;

    return 0;
}

int pv_venus_handle_vkBindImageMemory(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkBindImageMemory called\n");

    /* TODO: Parse image ID, memory ID, and offset from command data */
    /* For now, we'd need separate memory allocation for image */
    /* Skip actual binding in this test implementation */

    printf("[Venus Handlers]   (Binding skipped - would need separate memory allocation)\n");

    ctx->commands_handled++;

    return 0;
}

/*
 * Command Pool Management Handlers
 */

int pv_venus_handle_vkCreateCommandPool(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkCreateCommandPool called\n");

    /* TODO: Parse VkCommandPoolCreateInfo from command data */
    /* For now, create command pool for graphics queue */
    
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = ctx->vk->graphics_queue_family,
    };

    VkCommandPool command_pool;
    VkResult result = vkCreateCommandPool(ctx->vk->device, &pool_info, NULL, &command_pool);
    
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[Venus Handlers] vkCreateCommandPool failed: %d\n", result);
        return -1;
    }

    /* Track with fixed guest ID for testing */
    pv_venus_object_id guest_pool_id = 0x8000;
    pv_venus_object_add(&ctx->objects, guest_pool_id, command_pool,
                         PV_VENUS_OBJECT_TYPE_COMMAND_POOL);

    printf("[Venus Handlers]   Created command pool for queue family %u\n", 
           ctx->vk->graphics_queue_family);

    ctx->commands_handled++;
    ctx->objects_created++;

    return 0;
}

int pv_venus_handle_vkDestroyCommandPool(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkDestroyCommandPool called\n");

    /* TODO: Parse guest pool ID and destroy actual pool */

    ctx->commands_handled++;
    ctx->objects_destroyed++;

    return 0;
}

/*
 * Command Buffer Management Handlers
 */

int pv_venus_handle_vkAllocateCommandBuffers(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkAllocateCommandBuffers called\n");

    /* TODO: Parse VkCommandBufferAllocateInfo from command data */
    /* For now, allocate single primary command buffer */
    
    VkCommandPool pool = pv_venus_object_get(&ctx->objects, 0x8000);
    if (!pool) {
        fprintf(stderr, "[Venus Handlers] Command pool not found\n");
        return -1;
    }

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer command_buffer;
    VkResult result = vkAllocateCommandBuffers(ctx->vk->device, &alloc_info, &command_buffer);
    
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[Venus Handlers] vkAllocateCommandBuffers failed: %d\n", result);
        return -1;
    }

    /* Track with fixed guest ID for testing */
    pv_venus_object_id guest_cmdbuf_id = 0x9000;
    pv_venus_object_add(&ctx->objects, guest_cmdbuf_id, command_buffer,
                         PV_VENUS_OBJECT_TYPE_COMMAND_BUFFER);

    printf("[Venus Handlers]   Allocated command buffer\n");

    ctx->commands_handled++;
    ctx->objects_created++;

    return 0;
}

int pv_venus_handle_vkFreeCommandBuffers(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkFreeCommandBuffers called\n");

    /* TODO: Parse pool ID, command buffer count, and IDs */

    ctx->commands_handled++;
    ctx->objects_destroyed++;

    return 0;
}

int pv_venus_handle_vkBeginCommandBuffer(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkBeginCommandBuffer called\n");

    /* TODO: Parse guest command buffer ID and begin info */
    
    VkCommandBuffer cmd_buffer = pv_venus_object_get(&ctx->objects, 0x9000);
    if (!cmd_buffer) {
        fprintf(stderr, "[Venus Handlers] Command buffer not found\n");
        return -1;
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL,
    };

    VkResult result = vkBeginCommandBuffer(cmd_buffer, &begin_info);
    
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[Venus Handlers] vkBeginCommandBuffer failed: %d\n", result);
        return -1;
    }

    printf("[Venus Handlers]   Command buffer recording started\n");

    ctx->commands_handled++;

    return 0;
}

int pv_venus_handle_vkEndCommandBuffer(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkEndCommandBuffer called\n");

    /* TODO: Parse guest command buffer ID */
    
    VkCommandBuffer cmd_buffer = pv_venus_object_get(&ctx->objects, 0x9000);
    if (!cmd_buffer) {
        fprintf(stderr, "[Venus Handlers] Command buffer not found\n");
        return -1;
    }

    VkResult result = vkEndCommandBuffer(cmd_buffer);
    
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[Venus Handlers] vkEndCommandBuffer failed: %d\n", result);
        return -1;
    }

    printf("[Venus Handlers]   Command buffer recording finished\n");

    ctx->commands_handled++;

    return 0;
}

/*
 * Queue Operation Handlers
 */

int pv_venus_handle_vkQueueSubmit(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkQueueSubmit called\n");

    /* TODO: Parse submit info (command buffers, semaphores, fence) */
    
    VkCommandBuffer cmd_buffer = pv_venus_object_get(&ctx->objects, 0x9000);
    if (!cmd_buffer) {
        fprintf(stderr, "[Venus Handlers] Command buffer not found\n");
        return -1;
    }

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = NULL,
        .pWaitDstStageMask = NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd_buffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = NULL,
    };

    VkResult result = vkQueueSubmit(ctx->vk->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[Venus Handlers] vkQueueSubmit failed: %d\n", result);
        return -1;
    }

    printf("[Venus Handlers]   Submitted command buffer to GPU queue\n");

    ctx->commands_handled++;

    return 0;
}

int pv_venus_handle_vkQueueWaitIdle(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size)
{
    (void)header;
    (void)data;
    (void)data_size;
    
    struct pv_venus_dispatch_context *dispatch_ctx = context;
    struct pv_venus_handler_context *ctx = dispatch_ctx->user_context;

    printf("[Venus Handlers] vkQueueWaitIdle called\n");

    VkResult result = vkQueueWaitIdle(ctx->vk->graphics_queue);
    
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[Venus Handlers] vkQueueWaitIdle failed: %d\n", result);
        return -1;
    }

    printf("[Venus Handlers]   Queue idle (all GPU work completed)\n");

    ctx->commands_handled++;

    return 0;
}

/*
 * Register all handlers
 */
void pv_venus_handlers_register(
    struct pv_venus_dispatch_context *dispatch_ctx,
    struct pv_venus_handler_context *handler_ctx)
{
    /* Store handler context in dispatch context for handlers to access */
    dispatch_ctx->user_context = handler_ctx;

    /* Register instance handlers */
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkCreateInstance,
                                pv_venus_handle_vkCreateInstance);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkDestroyInstance,
                                pv_venus_handle_vkDestroyInstance);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkEnumeratePhysicalDevices,
                                pv_venus_handle_vkEnumeratePhysicalDevices);

    /* Register physical device query handlers */
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkGetPhysicalDeviceProperties,
                                pv_venus_handle_vkGetPhysicalDeviceProperties);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkGetPhysicalDeviceFeatures,
                                pv_venus_handle_vkGetPhysicalDeviceFeatures);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkGetPhysicalDeviceMemoryProperties,
                                pv_venus_handle_vkGetPhysicalDeviceMemoryProperties);

    /* Register device handlers */
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkCreateDevice,
                                pv_venus_handle_vkCreateDevice);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkDestroyDevice,
                                pv_venus_handle_vkDestroyDevice);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkGetDeviceQueue,
                                pv_venus_handle_vkGetDeviceQueue);

    /* Register memory handlers */
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkAllocateMemory,
                                pv_venus_handle_vkAllocateMemory);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkFreeMemory,
                                pv_venus_handle_vkFreeMemory);

    /* Register buffer handlers */
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkCreateBuffer,
                                pv_venus_handle_vkCreateBuffer);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkDestroyBuffer,
                                pv_venus_handle_vkDestroyBuffer);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkBindBufferMemory,
                                pv_venus_handle_vkBindBufferMemory);

    /* Register image handlers */
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkCreateImage,
                                pv_venus_handle_vkCreateImage);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkDestroyImage,
                                pv_venus_handle_vkDestroyImage);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkBindImageMemory,
                                pv_venus_handle_vkBindImageMemory);

    /* Register command pool handlers */
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkCreateCommandPool,
                                pv_venus_handle_vkCreateCommandPool);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkDestroyCommandPool,
                                pv_venus_handle_vkDestroyCommandPool);

    /* Register command buffer handlers */
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkAllocateCommandBuffers,
                                pv_venus_handle_vkAllocateCommandBuffers);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkFreeCommandBuffers,
                                pv_venus_handle_vkFreeCommandBuffers);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkBeginCommandBuffer,
                                pv_venus_handle_vkBeginCommandBuffer);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkEndCommandBuffer,
                                pv_venus_handle_vkEndCommandBuffer);

    /* Register queue operation handlers */
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkQueueSubmit,
                                pv_venus_handle_vkQueueSubmit);
    pv_venus_dispatch_register(dispatch_ctx, PV_VK_COMMAND_vkQueueWaitIdle,
                                pv_venus_handle_vkQueueWaitIdle);

    printf("[Venus Handlers] Registered 25 command handlers\n");
}
