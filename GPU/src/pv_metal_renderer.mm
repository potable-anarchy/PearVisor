//
//  pv_metal_renderer.mm
//  PearVisorGPU
//
//  Simple Metal renderer for GPU passthrough
//

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include "../include/pv_gpu.h"

// MARK: - Metal Renderer Structure

struct pv_metal_renderer {
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    id<MTLTexture> renderTarget;
    
    // Render state
    bool initialized;
    uint32_t width;
    uint32_t height;
    
    // Statistics
    uint64_t frames_rendered;
};

// MARK: - Initialization

pv_metal_renderer_t* pv_metal_renderer_create(uint32_t width, uint32_t height) {
    pv_metal_renderer_t* renderer = new pv_metal_renderer;
    if (!renderer) {
        return nullptr;
    }
    
    // Get default Metal device
    renderer->device = MTLCreateSystemDefaultDevice();
    if (!renderer->device) {
        NSLog(@"PearVisor GPU: Failed to create Metal device");
        delete renderer;
        return nullptr;
    }
    
    NSLog(@"PearVisor GPU: Metal device: %@", renderer->device.name);
    
    // Create command queue
    renderer->commandQueue = [renderer->device newCommandQueue];
    if (!renderer->commandQueue) {
        NSLog(@"PearVisor GPU: Failed to create command queue");
        delete renderer;
        return nullptr;
    }
    
    // Set dimensions
    renderer->width = width;
    renderer->height = height;
    renderer->initialized = true;
    renderer->frames_rendered = 0;
    
    // Create render target texture
    MTLTextureDescriptor* textureDescriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
        width:width
        height:height
        mipmapped:NO];
    
    textureDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    
    renderer->renderTarget = [renderer->device newTextureWithDescriptor:textureDescriptor];
    if (!renderer->renderTarget) {
        NSLog(@"PearVisor GPU: Failed to create render target");
        delete renderer;
        return nullptr;
    }
    
    NSLog(@"PearVisor GPU: Metal renderer initialized (%u x %u)", width, height);
    
    return renderer;
}

void pv_metal_renderer_destroy(pv_metal_renderer_t* renderer) {
    if (!renderer) {
        return;
    }
    
    NSLog(@"PearVisor GPU: Destroying Metal renderer");
    
    // Release Metal objects (ARC will handle it)
    renderer->renderTarget = nil;
    renderer->commandQueue = nil;
    renderer->device = nil;
    
    delete renderer;
}

// MARK: - Rendering

pv_gpu_error_t pv_metal_renderer_clear(pv_metal_renderer_t* renderer, float r, float g, float b, float a) {
    if (!renderer || !renderer->initialized) {
        return PV_GPU_ERROR_INIT_FAILED;
    }
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [renderer->commandQueue commandBuffer];
    if (!commandBuffer) {
        return PV_GPU_ERROR_METAL_FAILED;
    }
    
    // Create render pass descriptor
    MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    renderPassDescriptor.colorAttachments[0].texture = renderer->renderTarget;
    renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(r, g, b, a);
    renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    
    // Create render command encoder
    id<MTLRenderCommandEncoder> renderEncoder =
        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    
    if (!renderEncoder) {
        return PV_GPU_ERROR_METAL_FAILED;
    }
    
    // End encoding
    [renderEncoder endEncoding];
    
    // Commit command buffer
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    
    renderer->frames_rendered++;
    
    return PV_GPU_OK;
}

pv_gpu_error_t pv_metal_renderer_present(pv_metal_renderer_t* renderer) {
    if (!renderer || !renderer->initialized) {
        return PV_GPU_ERROR_INIT_FAILED;
    }
    
    // TODO: Present to display
    // For now, just mark frame as rendered
    renderer->frames_rendered++;
    
    return PV_GPU_OK;
}

// MARK: - Accessors

id<MTLTexture> pv_metal_renderer_get_texture(pv_metal_renderer_t* renderer) {
    if (!renderer) {
        return nil;
    }
    return renderer->renderTarget;
}

id<MTLDevice> pv_metal_renderer_get_device(pv_metal_renderer_t* renderer) {
    if (!renderer) {
        return nil;
    }
    return renderer->device;
}

uint64_t pv_metal_renderer_get_frame_count(pv_metal_renderer_t* renderer) {
    if (!renderer) {
        return 0;
    }
    return renderer->frames_rendered;
}

// MARK: - C API Bridge

extern "C" {
    pv_gpu_error_t pv_gpu_init_metal_renderer(void** renderer, uint32_t width, uint32_t height) {
        pv_metal_renderer_t* r = pv_metal_renderer_create(width, height);
        if (!r) {
            return PV_GPU_ERROR_INIT_FAILED;
        }
        *renderer = r;
        return PV_GPU_OK;
    }
    
    void pv_gpu_destroy_metal_renderer(void* renderer) {
        pv_metal_renderer_destroy((pv_metal_renderer_t*)renderer);
    }
    
    pv_gpu_error_t pv_gpu_metal_clear(void* renderer, float r, float g, float b, float a) {
        return pv_metal_renderer_clear((pv_metal_renderer_t*)renderer, r, g, b, a);
    }
    
    pv_gpu_error_t pv_gpu_metal_present(void* renderer) {
        return pv_metal_renderer_present((pv_metal_renderer_t*)renderer);
    }
}
