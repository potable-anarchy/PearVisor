/*
 * PearVisor - MoltenVK Wrapper
 * 
 * Simplified interface for MoltenVK/Vulkan initialization and management
 */

#ifndef PV_MOLTENVK_H
#define PV_MOLTENVK_H

#include <vulkan/vulkan.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * MoltenVK context
 * 
 * Holds all Vulkan state for PearVisor
 */
struct pv_moltenvk_context {
    /* Vulkan instance */
    VkInstance instance;
    bool instance_created;
    
    /* Physical device (Apple Silicon GPU) */
    VkPhysicalDevice physical_device;
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    VkPhysicalDeviceMemoryProperties memory_properties;
    uint32_t queue_family_count;
    VkQueueFamilyProperties *queue_families;
    
    /* Logical device */
    VkDevice device;
    bool device_created;
    
    /* Queues */
    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue transfer_queue;
    uint32_t graphics_queue_family;
    uint32_t compute_queue_family;
    uint32_t transfer_queue_family;
};

/*
 * Initialize MoltenVK context
 * 
 * Returns: Allocated context, or NULL on failure
 */
struct pv_moltenvk_context *pv_moltenvk_init(void);

/*
 * Cleanup MoltenVK context
 */
void pv_moltenvk_cleanup(struct pv_moltenvk_context *ctx);

/*
 * Create Vulkan instance
 * 
 * @ctx: MoltenVK context
 * @app_name: Application name (for debugging)
 * Returns: VK_SUCCESS or error code
 */
VkResult pv_moltenvk_create_instance(
    struct pv_moltenvk_context *ctx,
    const char *app_name
);

/*
 * Enumerate and select physical device
 * 
 * @ctx: MoltenVK context
 * Returns: VK_SUCCESS or error code
 */
VkResult pv_moltenvk_select_physical_device(
    struct pv_moltenvk_context *ctx
);

/*
 * Create logical device
 * 
 * @ctx: MoltenVK context
 * Returns: VK_SUCCESS or error code
 */
VkResult pv_moltenvk_create_device(
    struct pv_moltenvk_context *ctx
);

/*
 * Print Vulkan information (for debugging)
 */
void pv_moltenvk_print_info(const struct pv_moltenvk_context *ctx);

#ifdef __cplusplus
}
#endif

#endif /* PV_MOLTENVK_H */
