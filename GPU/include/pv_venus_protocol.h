/*
 * PearVisor - Venus Protocol Definitions
 * 
 * Venus protocol command IDs and structures
 * Based on venus-protocol from virglrenderer
 */

#ifndef PV_VENUS_PROTOCOL_H
#define PV_VENUS_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Venus Protocol Version
 */
#define PV_VENUS_PROTOCOL_VERSION 1

/*
 * Command Header
 * 
 * All Venus commands start with this header
 */
struct pv_venus_command_header {
    uint32_t command_id;      /* VkCommandTypeEXT value */
    uint32_t command_size;    /* Total size including header */
};

/*
 * Venus Command Types (VkCommandTypeEXT)
 * 
 * These are from vn_protocol_renderer_defines.h
 * We'll implement a subset initially (marked with *)
 */

/* Instance Management */
#define PV_VK_COMMAND_vkCreateInstance                         0   /* * */
#define PV_VK_COMMAND_vkDestroyInstance                        1   /* * */
#define PV_VK_COMMAND_vkEnumeratePhysicalDevices               2   /* * */

/* Physical Device Queries */
#define PV_VK_COMMAND_vkGetPhysicalDeviceFeatures              3   /* * */
#define PV_VK_COMMAND_vkGetPhysicalDeviceFormatProperties      4
#define PV_VK_COMMAND_vkGetPhysicalDeviceImageFormatProperties 5
#define PV_VK_COMMAND_vkGetPhysicalDeviceProperties            6   /* * */
#define PV_VK_COMMAND_vkGetPhysicalDeviceQueueFamilyProperties 7   /* * */
#define PV_VK_COMMAND_vkGetPhysicalDeviceMemoryProperties      8   /* * */

/* Device Management */
#define PV_VK_COMMAND_vkCreateDevice                          11   /* * */
#define PV_VK_COMMAND_vkDestroyDevice                         12   /* * */
#define PV_VK_COMMAND_vkGetDeviceQueue                        17   /* * */

/* Queue Operations */
#define PV_VK_COMMAND_vkQueueSubmit                           18   /* * */
#define PV_VK_COMMAND_vkQueueWaitIdle                         19   /* * */
#define PV_VK_COMMAND_vkDeviceWaitIdle                        20   /* * */

/* Memory Management */
#define PV_VK_COMMAND_vkAllocateMemory                        21   /* * */
#define PV_VK_COMMAND_vkFreeMemory                            22   /* * */
#define PV_VK_COMMAND_vkMapMemory                             23
#define PV_VK_COMMAND_vkUnmapMemory                           24
#define PV_VK_COMMAND_vkBindBufferMemory                      28   /* * */
#define PV_VK_COMMAND_vkBindImageMemory                       29   /* * */
#define PV_VK_COMMAND_vkGetBufferMemoryRequirements           30   /* * */
#define PV_VK_COMMAND_vkGetImageMemoryRequirements            31   /* * */

/* Synchronization */
#define PV_VK_COMMAND_vkCreateFence                           35   /* * */
#define PV_VK_COMMAND_vkDestroyFence                          36
#define PV_VK_COMMAND_vkResetFences                           37
#define PV_VK_COMMAND_vkGetFenceStatus                        38
#define PV_VK_COMMAND_vkWaitForFences                         39   /* * */
#define PV_VK_COMMAND_vkCreateSemaphore                       40   /* * */
#define PV_VK_COMMAND_vkDestroySemaphore                      41

/* Buffers */
#define PV_VK_COMMAND_vkCreateBuffer                          50   /* * */
#define PV_VK_COMMAND_vkDestroyBuffer                         51   /* * */

/* Images */
#define PV_VK_COMMAND_vkCreateImage                           54   /* * */
#define PV_VK_COMMAND_vkDestroyImage                          55   /* * */
#define PV_VK_COMMAND_vkCreateImageView                       57
#define PV_VK_COMMAND_vkDestroyImageView                      58

/* Shaders and Pipelines */
#define PV_VK_COMMAND_vkCreateShaderModule                    59
#define PV_VK_COMMAND_vkDestroyShaderModule                   60
#define PV_VK_COMMAND_vkCreateGraphicsPipelines               65
#define PV_VK_COMMAND_vkCreateComputePipelines                66
#define PV_VK_COMMAND_vkDestroyPipeline                       67

/* Command Buffers */
#define PV_VK_COMMAND_vkCreateCommandPool                     85   /* * */
#define PV_VK_COMMAND_vkDestroyCommandPool                    86
#define PV_VK_COMMAND_vkResetCommandPool                      87
#define PV_VK_COMMAND_vkAllocateCommandBuffers                88   /* * */
#define PV_VK_COMMAND_vkFreeCommandBuffers                    89
#define PV_VK_COMMAND_vkBeginCommandBuffer                    90   /* * */
#define PV_VK_COMMAND_vkEndCommandBuffer                      91   /* * */

/* Drawing Commands */
#define PV_VK_COMMAND_vkCmdBindPipeline                       93
#define PV_VK_COMMAND_vkCmdSetViewport                        94
#define PV_VK_COMMAND_vkCmdSetScissor                         95
#define PV_VK_COMMAND_vkCmdDraw                              106
#define PV_VK_COMMAND_vkCmdDrawIndexed                       107

/* Command buffer commands */
#define PV_VK_COMMAND_vkCmdCopyBuffer                        112
#define PV_VK_COMMAND_vkCmdPipelineBarrier                   126
#define PV_VK_COMMAND_vkCmdBeginRenderPass                   133
#define PV_VK_COMMAND_vkCmdEndRenderPass                     135

/*
 * Maximum command ID we support (for array bounds)
 */
#define PV_VENUS_MAX_COMMAND_ID 500

/*
 * Command handler function type
 */
typedef int (*pv_venus_command_handler_t)(
    void *context,
    const struct pv_venus_command_header *header,
    const void *data,
    size_t data_size
);

/*
 * Get command name for debugging
 */
const char *pv_venus_command_name(uint32_t command_id);

/*
 * Validate command header
 */
int pv_venus_validate_command_header(const struct pv_venus_command_header *header);

#ifdef __cplusplus
}
#endif

#endif /* PV_VENUS_PROTOCOL_H */
