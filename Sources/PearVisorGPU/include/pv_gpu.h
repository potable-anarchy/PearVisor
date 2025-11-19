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
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// MARK: - Forward Declarations

typedef struct pv_gpu_device pv_gpu_device_t;
typedef struct pv_metal_renderer pv_metal_renderer_t;

// MARK: - Error Codes

typedef enum {
    PV_GPU_OK = 0,
    PV_GPU_ERROR_INIT_FAILED = -1,
    PV_GPU_ERROR_DEVICE_NOT_FOUND = -2,
    PV_GPU_ERROR_VULKAN_FAILED = -3,
    PV_GPU_ERROR_METAL_FAILED = -4,
    PV_GPU_ERROR_OUT_OF_MEMORY = -5,
} pv_gpu_error_t;

// MARK: - Info Structures

typedef struct {
    uint32_t vendor_id;
    uint32_t device_id;
    char name[256];
    uint64_t vram_size;
    bool supports_vulkan;
    bool supports_metal;
} pv_gpu_info_t;

// MARK: - Initialization

pv_gpu_error_t pv_gpu_init(void);
void pv_gpu_shutdown(void);

// MARK: - Device Management

pv_gpu_error_t pv_gpu_get_info(pv_gpu_info_t *info);
pv_gpu_error_t pv_gpu_create_device(const uint8_t vm_id[16], pv_gpu_device_t **device);
void pv_gpu_destroy_device(pv_gpu_device_t *device);

// MARK: - Command Processing

pv_gpu_error_t pv_gpu_start_venus(pv_gpu_device_t *device);
void pv_gpu_stop_venus(pv_gpu_device_t *device);
pv_gpu_error_t pv_gpu_submit_command(pv_gpu_device_t* device, const void* cmd, size_t cmd_size);
pv_gpu_error_t pv_gpu_present_frame(pv_gpu_device_t* device);

// MARK: - Metal Renderer

pv_gpu_error_t pv_gpu_init_metal_renderer(void** renderer, uint32_t width, uint32_t height);
void pv_gpu_destroy_metal_renderer(void* renderer);
pv_gpu_error_t pv_gpu_metal_clear(void* renderer, float r, float g, float b, float a);
pv_gpu_error_t pv_gpu_metal_present(void* renderer);

// MARK: - Performance

double pv_gpu_get_utilization(pv_gpu_device_t *device);
uint64_t pv_gpu_get_memory_usage(pv_gpu_device_t *device);

#ifdef __cplusplus
}
#endif

#endif // PV_GPU_H
