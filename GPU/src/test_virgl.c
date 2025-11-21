/*
 * PearVisor - virglrenderer Test Program
 * 
 * Simple test to verify virglrenderer with Venus support works
 */

#include <stdio.h>
#include "pv_virgl.h"

int main(int argc, char **argv)
{
    printf("PearVisor virglrenderer Venus Integration Test\n");
    printf("===============================================\n\n");

    int ret = pv_virgl_test();
    
    if (ret == 0) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Tests failed with error: %d\n", ret);
        return 1;
    }
}
