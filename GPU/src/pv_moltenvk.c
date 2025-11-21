/*
 * PearVisor - MoltenVK Wrapper Implementation
 */

#include "pv_moltenvk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* MoltenVK portability extension flag (may not be in older headers) */
#ifndef VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
#define VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR 0x00000001
#endif

/*
 * Initialize MoltenVK context
 */
struct pv_moltenvk_context *pv_moltenvk_init(void)
{
    struct pv_moltenvk_context *ctx = calloc(1, sizeof(*ctx));
    if (!ctx) {
        fprintf(stderr, "[MoltenVK] Failed to allocate context\n");
        return NULL;
    }

    printf("[MoltenVK] Context initialized\n");
    return ctx;
}

/*
 * Cleanup MoltenVK context
 */
void pv_moltenvk_cleanup(struct pv_moltenvk_context *ctx)
{
    if (!ctx) {
        return;
    }

    /* Destroy device */
    if (ctx->device_created && ctx->device) {
        vkDestroyDevice(ctx->device, NULL);
        printf("[MoltenVK] Destroyed device\n");
    }

    /* Destroy instance */
    if (ctx->instance_created && ctx->instance) {
        vkDestroyInstance(ctx->instance, NULL);
        printf("[MoltenVK] Destroyed instance\n");
    }

    /* Free queue families */
    if (ctx->queue_families) {
        free(ctx->queue_families);
    }

    free(ctx);
    printf("[MoltenVK] Context cleaned up\n");
}

/*
 * Create Vulkan instance
 */
VkResult pv_moltenvk_create_instance(
    struct pv_moltenvk_context *ctx,
    const char *app_name)
{
    if (!ctx || ctx->instance_created) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    printf("[MoltenVK] Creating Vulkan instance...\n");

    /* Application info */
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = app_name ? app_name : "PearVisor Guest",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "PearVisor",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    /* Enable portability enumeration for MoltenVK */
    const char *extensions[] = {
        "VK_KHR_portability_enumeration",
    };

    /* Instance create info */
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = extensions,
    };

    /* Create instance */
    VkResult result = vkCreateInstance(&create_info, NULL, &ctx->instance);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[MoltenVK] Failed to create instance: %d\n", result);
        return result;
    }

    ctx->instance_created = true;
    printf("[MoltenVK] Instance created successfully\n");

    return VK_SUCCESS;
}

/*
 * Enumerate and select physical device
 */
VkResult pv_moltenvk_select_physical_device(
    struct pv_moltenvk_context *ctx)
{
    if (!ctx || !ctx->instance_created) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    printf("[MoltenVK] Enumerating physical devices...\n");

    /* Get device count */
    uint32_t device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(ctx->instance, &device_count, NULL);
    if (result != VK_SUCCESS || device_count == 0) {
        fprintf(stderr, "[MoltenVK] No physical devices found\n");
        return VK_ERROR_DEVICE_LOST;
    }

    printf("[MoltenVK] Found %u physical device(s)\n", device_count);

    /* Get devices */
    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    if (!devices) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    result = vkEnumeratePhysicalDevices(ctx->instance, &device_count, devices);
    if (result != VK_SUCCESS) {
        free(devices);
        return result;
    }

    /* Select first device (Apple Silicon GPU) */
    ctx->physical_device = devices[0];
    free(devices);

    /* Query device properties */
    vkGetPhysicalDeviceProperties(ctx->physical_device, &ctx->device_properties);
    vkGetPhysicalDeviceFeatures(ctx->physical_device, &ctx->device_features);
    vkGetPhysicalDeviceMemoryProperties(ctx->physical_device, &ctx->memory_properties);

    /* Query queue families */
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device,
                                              &ctx->queue_family_count, NULL);
    
    if (ctx->queue_family_count > 0) {
        ctx->queue_families = malloc(sizeof(VkQueueFamilyProperties) * 
                                      ctx->queue_family_count);
        if (ctx->queue_families) {
            vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device,
                                                      &ctx->queue_family_count,
                                                      ctx->queue_families);
        }
    }

    printf("[MoltenVK] Selected device: %s\n", ctx->device_properties.deviceName);
    printf("[MoltenVK] Driver version: %u.%u.%u\n",
           VK_VERSION_MAJOR(ctx->device_properties.driverVersion),
           VK_VERSION_MINOR(ctx->device_properties.driverVersion),
           VK_VERSION_PATCH(ctx->device_properties.driverVersion));

    return VK_SUCCESS;
}

/*
 * Create logical device
 */
VkResult pv_moltenvk_create_device(
    struct pv_moltenvk_context *ctx)
{
    if (!ctx || !ctx->physical_device || ctx->device_created) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    printf("[MoltenVK] Creating logical device...\n");

    /* Find graphics queue family */
    uint32_t graphics_family = 0;
    bool found = false;
    
    for (uint32_t i = 0; i < ctx->queue_family_count; i++) {
        if (ctx->queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_family = i;
            found = true;
            break;
        }
    }

    if (!found) {
        fprintf(stderr, "[MoltenVK] No graphics queue family found\n");
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    ctx->graphics_queue_family = graphics_family;
    ctx->compute_queue_family = graphics_family;  /* Same on Apple Silicon */
    ctx->transfer_queue_family = graphics_family;

    /* Queue create info */
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = graphics_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    /* Device create info */
    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .pEnabledFeatures = &ctx->device_features,
    };

    /* Create device */
    VkResult result = vkCreateDevice(ctx->physical_device, &create_info, 
                                      NULL, &ctx->device);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[MoltenVK] Failed to create device: %d\n", result);
        return result;
    }

    ctx->device_created = true;

    /* Get queue handles */
    vkGetDeviceQueue(ctx->device, graphics_family, 0, &ctx->graphics_queue);
    ctx->compute_queue = ctx->graphics_queue;
    ctx->transfer_queue = ctx->graphics_queue;

    printf("[MoltenVK] Device created successfully\n");
    printf("[MoltenVK] Graphics queue family: %u\n", graphics_family);

    return VK_SUCCESS;
}

/*
 * Print Vulkan information
 */
void pv_moltenvk_print_info(const struct pv_moltenvk_context *ctx)
{
    if (!ctx) {
        return;
    }

    printf("\n=== MoltenVK/Vulkan Info ===\n");

    if (ctx->instance_created) {
        printf("Instance: Created\n");
    }

    if (ctx->physical_device) {
        printf("\nPhysical Device:\n");
        printf("  Name: %s\n", ctx->device_properties.deviceName);
        printf("  Type: ");
        switch (ctx->device_properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                printf("Integrated GPU\n");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                printf("Discrete GPU\n");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                printf("Virtual GPU\n");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                printf("CPU\n");
                break;
            default:
                printf("Other\n");
        }
        
        printf("  API Version: %u.%u.%u\n",
               VK_VERSION_MAJOR(ctx->device_properties.apiVersion),
               VK_VERSION_MINOR(ctx->device_properties.apiVersion),
               VK_VERSION_PATCH(ctx->device_properties.apiVersion));
        
        printf("  Driver Version: %u.%u.%u\n",
               VK_VERSION_MAJOR(ctx->device_properties.driverVersion),
               VK_VERSION_MINOR(ctx->device_properties.driverVersion),
               VK_VERSION_PATCH(ctx->device_properties.driverVersion));
        
        printf("  Vendor ID: 0x%04X\n", ctx->device_properties.vendorID);
        printf("  Device ID: 0x%04X\n", ctx->device_properties.deviceID);
        
        printf("\nMemory:\n");
        printf("  Heaps: %u\n", ctx->memory_properties.memoryHeapCount);
        for (uint32_t i = 0; i < ctx->memory_properties.memoryHeapCount; i++) {
            printf("    Heap %u: %llu MB\n", i,
                   ctx->memory_properties.memoryHeaps[i].size / (1024 * 1024));
        }
        
        printf("  Types: %u\n", ctx->memory_properties.memoryTypeCount);
        
        printf("\nQueue Families: %u\n", ctx->queue_family_count);
        for (uint32_t i = 0; i < ctx->queue_family_count; i++) {
            printf("  Family %u: %u queues, flags: ", i,
                   ctx->queue_families[i].queueCount);
            
            if (ctx->queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                printf("GRAPHICS ");
            if (ctx->queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                printf("COMPUTE ");
            if (ctx->queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
                printf("TRANSFER ");
            printf("\n");
        }
    }

    if (ctx->device_created) {
        printf("\nLogical Device: Created\n");
        printf("  Graphics Queue Family: %u\n", ctx->graphics_queue_family);
    }

    printf("\n");
}
