/*
 * PearVisor - Venus Protocol Implementation
 */

#include "pv_venus_protocol.h"
#include <stdio.h>
#include <string.h>

/*
 * Command name lookup table (for debugging)
 */
static const struct {
    uint32_t id;
    const char *name;
} command_names[] = {
    /* Instance */
    {PV_VK_COMMAND_vkCreateInstance, "vkCreateInstance"},
    {PV_VK_COMMAND_vkDestroyInstance, "vkDestroyInstance"},
    {PV_VK_COMMAND_vkEnumeratePhysicalDevices, "vkEnumeratePhysicalDevices"},
    
    /* Physical Device */
    {PV_VK_COMMAND_vkGetPhysicalDeviceFeatures, "vkGetPhysicalDeviceFeatures"},
    {PV_VK_COMMAND_vkGetPhysicalDeviceProperties, "vkGetPhysicalDeviceProperties"},
    {PV_VK_COMMAND_vkGetPhysicalDeviceQueueFamilyProperties, "vkGetPhysicalDeviceQueueFamilyProperties"},
    {PV_VK_COMMAND_vkGetPhysicalDeviceMemoryProperties, "vkGetPhysicalDeviceMemoryProperties"},
    
    /* Device */
    {PV_VK_COMMAND_vkCreateDevice, "vkCreateDevice"},
    {PV_VK_COMMAND_vkDestroyDevice, "vkDestroyDevice"},
    {PV_VK_COMMAND_vkGetDeviceQueue, "vkGetDeviceQueue"},
    
    /* Queue */
    {PV_VK_COMMAND_vkQueueSubmit, "vkQueueSubmit"},
    {PV_VK_COMMAND_vkQueueWaitIdle, "vkQueueWaitIdle"},
    {PV_VK_COMMAND_vkDeviceWaitIdle, "vkDeviceWaitIdle"},
    
    /* Memory */
    {PV_VK_COMMAND_vkAllocateMemory, "vkAllocateMemory"},
    {PV_VK_COMMAND_vkFreeMemory, "vkFreeMemory"},
    {PV_VK_COMMAND_vkBindBufferMemory, "vkBindBufferMemory"},
    {PV_VK_COMMAND_vkBindImageMemory, "vkBindImageMemory"},
    {PV_VK_COMMAND_vkGetBufferMemoryRequirements, "vkGetBufferMemoryRequirements"},
    {PV_VK_COMMAND_vkGetImageMemoryRequirements, "vkGetImageMemoryRequirements"},
    
    /* Sync */
    {PV_VK_COMMAND_vkCreateFence, "vkCreateFence"},
    {PV_VK_COMMAND_vkWaitForFences, "vkWaitForFences"},
    {PV_VK_COMMAND_vkCreateSemaphore, "vkCreateSemaphore"},
    
    /* Buffers */
    {PV_VK_COMMAND_vkCreateBuffer, "vkCreateBuffer"},
    {PV_VK_COMMAND_vkDestroyBuffer, "vkDestroyBuffer"},
    
    /* Images */
    {PV_VK_COMMAND_vkCreateImage, "vkCreateImage"},
    {PV_VK_COMMAND_vkDestroyImage, "vkDestroyImage"},
    
    /* Command Buffers */
    {PV_VK_COMMAND_vkCreateCommandPool, "vkCreateCommandPool"},
    {PV_VK_COMMAND_vkAllocateCommandBuffers, "vkAllocateCommandBuffers"},
    {PV_VK_COMMAND_vkBeginCommandBuffer, "vkBeginCommandBuffer"},
    {PV_VK_COMMAND_vkEndCommandBuffer, "vkEndCommandBuffer"},
};

#define NUM_COMMAND_NAMES (sizeof(command_names) / sizeof(command_names[0]))

/*
 * Get command name for debugging
 */
const char *pv_venus_command_name(uint32_t command_id)
{
    for (size_t i = 0; i < NUM_COMMAND_NAMES; i++) {
        if (command_names[i].id == command_id) {
            return command_names[i].name;
        }
    }
    
    static char unknown[64];
    snprintf(unknown, sizeof(unknown), "Unknown(0x%x)", command_id);
    return unknown;
}

/*
 * Validate command header
 */
int pv_venus_validate_command_header(const struct pv_venus_command_header *header)
{
    if (!header) {
        return -1;
    }
    
    /* Check command size is reasonable */
    if (header->command_size < sizeof(struct pv_venus_command_header)) {
        fprintf(stderr, "[Venus Protocol] Invalid command size: %u (too small)\n",
                header->command_size);
        return -1;
    }
    
    if (header->command_size > 1024 * 1024) {  /* 1MB max */
        fprintf(stderr, "[Venus Protocol] Invalid command size: %u (too large)\n",
                header->command_size);
        return -1;
    }
    
    /* Check command ID is in valid range */
    if (header->command_id >= PV_VENUS_MAX_COMMAND_ID) {
        fprintf(stderr, "[Venus Protocol] Invalid command ID: %u\n",
                header->command_id);
        return -1;
    }
    
    return 0;
}
