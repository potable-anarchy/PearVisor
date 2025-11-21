/*
 * PearVisor - virglrenderer Integration Header
 * 
 * Public API for virglrenderer integration with Venus protocol
 */

#ifndef PV_VIRGL_H
#define PV_VIRGL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize virglrenderer with Venus support
 * 
 * Returns: 0 on success, negative error code on failure
 */
int pv_virgl_init(void);

/*
 * Create a Venus rendering context
 * 
 * @ctx_id: Unique context identifier
 * Returns: 0 on success, negative error code on failure
 */
int pv_virgl_create_venus_context(uint32_t ctx_id);

/*
 * Destroy a rendering context
 * 
 * @ctx_id: Context identifier to destroy
 */
void pv_virgl_destroy_context(uint32_t ctx_id);

/*
 * Cleanup virglrenderer
 */
void pv_virgl_cleanup(void);

/*
 * Run basic functionality test
 * 
 * Returns: 0 on success, negative error code on failure
 */
int pv_virgl_test(void);

#ifdef __cplusplus
}
#endif

#endif /* PV_VIRGL_H */
