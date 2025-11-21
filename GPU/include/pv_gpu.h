/*
 * PearVisor GPU Subsystem Header
 * 
 * Main API for GPU virtualization
 */

#ifndef PV_GPU_H
#define PV_GPU_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes */
typedef enum {
    PV_GPU_OK = 0,
    PV_GPU_ERROR_INIT_FAILED = -1,
    PV_GPU_ERROR_OUT_OF_MEMORY = -2,
    PV_GPU_ERROR_INVALID_PARAM = -3,
} pv_gpu_error_t;

/* GPU info structure */
typedef struct {
    char name[256];
    uint32_t vendor_id;
    uint32_t device_id;
    uint64_t vram_size;
    bool supports_vulkan;
    bool supports_metal;
} pv_gpu_info_t;

/* GPU device structure */
struct pv_gpu_device {
    uint8_t vm_id[16];
    void *venus_ctx;
    void *moltenvk_ctx;
    bool initialized;
};

typedef struct pv_gpu_device pv_gpu_device_t;

/* Initialization */
pv_gpu_error_t pv_gpu_init(void);
void pv_gpu_shutdown(void);

/* Device management */
pv_gpu_error_t pv_gpu_get_info(pv_gpu_info_t *info);
pv_gpu_error_t pv_gpu_create_device(const uint8_t vm_id[16], pv_gpu_device_t **device);
void pv_gpu_destroy_device(pv_gpu_device_t *device);

/* Venus protocol */
pv_gpu_error_t pv_gpu_start_venus(pv_gpu_device_t *device);
void pv_gpu_stop_venus(pv_gpu_device_t *device);

/* MoltenVK bridge */
pv_gpu_error_t pv_gpu_init_moltenvk(void);
void pv_gpu_shutdown_moltenvk(void);

/* Performance */
double pv_gpu_get_utilization(pv_gpu_device_t *device);
uint64_t pv_gpu_get_memory_usage(pv_gpu_device_t *device);

#ifdef __cplusplus
}
#endif

#endif /* PV_GPU_H */
