/*
 * PearVisor - MoltenVK Test
 * 
 * Test program to verify MoltenVK integration
 */

#include <stdio.h>
#include "pv_moltenvk.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    printf("=== PearVisor MoltenVK Integration Test ===\n\n");

    /* Test 1: Create context */
    printf("--- Test 1: Create Context ---\n");
    struct pv_moltenvk_context *ctx = pv_moltenvk_init();
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        return 1;
    }

    /* Test 2: Create Vulkan instance */
    printf("\n--- Test 2: Create Vulkan Instance ---\n");
    VkResult result = pv_moltenvk_create_instance(ctx, "PearVisor Test");
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create instance: %d\n", result);
        pv_moltenvk_cleanup(ctx);
        return 1;
    }

    /* Test 3: Select physical device */
    printf("\n--- Test 3: Select Physical Device ---\n");
    result = pv_moltenvk_select_physical_device(ctx);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to select physical device: %d\n", result);
        pv_moltenvk_cleanup(ctx);
        return 1;
    }

    /* Test 4: Create logical device */
    printf("\n--- Test 4: Create Logical Device ---\n");
    result = pv_moltenvk_create_device(ctx);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create device: %d\n", result);
        pv_moltenvk_cleanup(ctx);
        return 1;
    }

    /* Test 5: Print Vulkan info */
    printf("\n--- Test 5: Vulkan Information ---\n");
    pv_moltenvk_print_info(ctx);

    /* Cleanup */
    printf("--- Cleanup ---\n");
    pv_moltenvk_cleanup(ctx);

    printf("\n=== All Tests Passed! ===\n");
    return 0;
}
