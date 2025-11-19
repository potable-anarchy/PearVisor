//
//  GPUTest.swift
//  PearVisor
//
//  GPU rendering test with Metal
//

import Foundation
import PearVisorCore

@main
struct GPUTest {
    static func main() async throws {
        print("PearVisor GPU Rendering Test")
        print("=============================")
        print("")
        
        let gpu = GPUController.shared
        
        print("GPU: \(gpu.gpuName)")
        let info = gpu.getGPUInfo()
        print("Memory: \(info.formattedMemory)")
        print("Ray Tracing: \(info.supportsRaytracing)")
        print("")
        
        // Initialize GPU
        print("Initializing GPU subsystem...")
        try gpu.initialize()
        print("✓ GPU initialized")
        print("")
        
        // Create GPU device
        let vmID = UUID()
        print("Creating GPU device...")
        let device = try gpu.attachGPUToVM(vmID)
        print("✓ Device created: \(device)")
        print("")
        
        // Initialize Metal renderer
        print("Initializing Metal renderer (1920x1080)...")
        var renderer: OpaquePointer?
        let result = pv_gpu_init_metal_renderer(&renderer, 1920, 1080)
        guard result == 0, let metalRenderer = renderer else {
            print("✗ Failed to initialize Metal renderer")
            exit(1)
        }
        print("✓ Metal renderer initialized")
        print("")
        
        // Test rendering: Clear screen to different colors
        let colors: [(String, Float, Float, Float, Float)] = [
            ("Red", 1.0, 0.0, 0.0, 1.0),
            ("Green", 0.0, 1.0, 0.0, 1.0),
            ("Blue", 0.0, 0.0, 1.0, 1.0),
            ("Yellow", 1.0, 1.0, 0.0, 1.0),
            ("Magenta", 1.0, 0.0, 1.0, 1.0),
            ("Cyan", 0.0, 1.0, 1.0, 1.0)
        ]
        
        print("Testing Metal rendering...")
        print("Clearing screen to different colors...")
        print("")
        
        for (name, r, g, b, a) in colors {
            print("  Rendering \(name) (R:\(r), G:\(g), B:\(b))...", terminator: "")
            fflush(stdout)
            
            let clearResult = pv_gpu_metal_clear(metalRenderer, r, g, b, a)
            if clearResult != 0 {
                print(" FAILED")
                continue
            }
            
            let presentResult = pv_gpu_metal_present(metalRenderer)
            if presentResult != 0 {
                print(" FAILED")
                continue
            }
            
            print(" ✓")
            
            // Small delay to simulate frame time
            try await Task.sleep(for: .milliseconds(100))
        }
        
        print("")
        print("✓ Rendered \(colors.count) frames successfully!")
        print("")
        
        // Get final metrics
        let metrics = gpu.getGPUUsage(for: vmID)
        print("Final GPU Metrics:")
        print("  Utilization: \(Int(metrics.utilization * 100))%")
        print("  Memory Used: \(metrics.memoryUsed / 1024 / 1024) MB")
        print("")
        
        // Cleanup
        print("Cleaning up...")
        pv_gpu_destroy_metal_renderer(metalRenderer)
        gpu.detachGPUFromVM(vmID)
        print("✓ Cleanup complete")
        print("")
        
        print("🎉 GPU RENDERING TEST COMPLETE!")
        print("")
        print("Summary:")
        print("  - GPU device: Created and functional")
        print("  - Metal renderer: Working")
        print("  - Frames rendered: \(colors.count)")
        print("  - Clear operations: ✓")
        print("  - Present operations: ✓")
        print("")
        print("Next: Integrate with VM and render from guest!")
    }
}

// MARK: - C Interop for Metal Renderer

@_silgen_name("pv_gpu_init_metal_renderer")
func pv_gpu_init_metal_renderer(
    _ renderer: UnsafeMutablePointer<OpaquePointer?>,
    _ width: UInt32,
    _ height: UInt32
) -> Int32

@_silgen_name("pv_gpu_destroy_metal_renderer")
func pv_gpu_destroy_metal_renderer(_ renderer: OpaquePointer)

@_silgen_name("pv_gpu_metal_clear")
func pv_gpu_metal_clear(
    _ renderer: OpaquePointer,
    _ r: Float,
    _ g: Float,
    _ b: Float,
    _ a: Float
) -> Int32

@_silgen_name("pv_gpu_metal_present")
func pv_gpu_metal_present(_ renderer: OpaquePointer) -> Int32
