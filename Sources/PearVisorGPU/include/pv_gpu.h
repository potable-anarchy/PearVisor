//
//  pv_gpu.h
//  PearVisorGPU
//
//  C/C++ GPU subsystem interface
//

#ifndef PV_GPU_H
#define PV_GPU_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// MARK: - Types

typedef struct pv_gpu_device pv_gpu_device_t;
typedef struct pv_gpu_context pv_gpu_context_t;

typedef enum {
    PV_GPU_OK = 0,
    PV_GPU_ERROR_INIT_FAILED = -1,
    PV_GPU_ERROR_DEVICE_NOT_FOUND = -2,
    PV_GPU_ERROR_VULKAN_FAILED = -3,
    PV_GPU_ERROR_METAL_FAILED = -4,
    PV_GPU_ERROR_OUT_OF_MEMORY = -5,
} pv_gpu_error_t;

typedef struct {
    uint32_t vendor_id;
    uint32_t device_id;
    char name[256];
    uint64_t vram_size;
    bool supports_vulkan;
    bool supports_metal;
} pv_gpu_info_t;

// MARK: - Initialization

/**
 * Initialize the GPU subsystem
 * @return PV_GPU_OK on success, error code otherwise
 */
pv_gpu_error_t pv_gpu_init(void);

/**
 * Cleanup and shutdown the GPU subsystem
 */
void pv_gpu_shutdown(void);

// MARK: - Device Management

/**
 * Get GPU device information
 * @param info Output GPU information structure
 * @return PV_GPU_OK on success, error code otherwise
 */
pv_gpu_error_t pv_gpu_get_info(pv_gpu_info_t *info);

/**
 * Create a GPU device for a VM
 * @param vm_id UUID of the virtual machine
 * @param device Output device handle
 * @return PV_GPU_OK on success, error code otherwise
 */
pv_gpu_error_t pv_gpu_create_device(const uint8_t vm_id[16], pv_gpu_device_t **device);

/**
 * Destroy a GPU device
 * @param device Device handle to destroy
 */
void pv_gpu_destroy_device(pv_gpu_device_t *device);

// MARK: - Venus Protocol

/**
 * Start Venus protocol handler for a device
 * @param device GPU device handle
 * @return PV_GPU_OK on success, error code otherwise
 */
pv_gpu_error_t pv_gpu_start_venus(pv_gpu_device_t *device);

/**
 * Stop Venus protocol handler
 * @param device GPU device handle
 */
void pv_gpu_stop_venus(pv_gpu_device_t *device);

// MARK: - MoltenVK Bridge

/**
 * Initialize MoltenVK bridge
 * @return PV_GPU_OK on success, error code otherwise
 */
pv_gpu_error_t pv_gpu_init_moltenvk(void);

/**
 * Shutdown MoltenVK bridge
 */
void pv_gpu_shutdown_moltenvk(void);

// MARK: - Performance

/**
 * Get GPU utilization (0.0 - 1.0)
 * @param device GPU device handle
 * @return Utilization percentage
 */
double pv_gpu_get_utilization(pv_gpu_device_t *device);

/**
 * Get GPU memory usage in bytes
 * @param device GPU device handle
 * @return Memory usage in bytes
 */
uint64_t pv_gpu_get_memory_usage(pv_gpu_device_t *device);

#ifdef __cplusplus
}
#endif

#endif // PV_GPU_H
