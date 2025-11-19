//
//  GPUTest.swift
//  PearVisor
//
//  Simple GPU initialization test
//

import Foundation
import PearVisorCore

@main
struct GPUTest {
    static func main() async throws {
        print("PearVisor GPU Test")
        print("==================")
        print("")
        
        // Get GPU controller
        let gpu = GPUController.shared
        
        print("GPU Available: \(gpu.isGPUAvailable)")
        print("GPU Name: \(gpu.gpuName)")
        print("")
        
        // Get GPU info
        let info = gpu.getGPUInfo()
        print("GPU Information:")
        print("  Name: \(info.name)")
        print("  Low Power: \(info.isLowPower)")
        print("  Memory: \(info.formattedMemory)")
        print("  Ray Tracing: \(info.supportsRaytracing)")
        print("")
        
        // Initialize GPU subsystem
        print("Initializing GPU subsystem...")
        do {
            try gpu.initialize()
            print("✓ GPU subsystem initialized")
        } catch {
            print("✗ Failed to initialize: \(error)")
            exit(1)
        }
        print("")
        
        // Create test VM ID
        let testVMID = UUID()
        print("Creating GPU device for test VM: \(testVMID)")
        
        do {
            let device = try gpu.attachGPUToVM(testVMID)
            print("✓ GPU device created: \(device)")
            print("")
            
            // Get metrics
            let metrics = gpu.getGPUUsage(for: testVMID)
            print("GPU Metrics:")
            print("  Utilization: \(Int(metrics.utilization * 100))%")
            print("  Memory Used: \(metrics.memoryUsed / 1024 / 1024) MB")
            print("  Memory Total: \(metrics.memoryTotal / 1024 / 1024 / 1024) GB")
            print("")
            
            // Keep running for a bit
            print("GPU device running, press Ctrl+C to stop...")
            try await Task.sleep(for: .seconds(5))
            
            // Cleanup
            print("")
            print("Detaching GPU...")
            gpu.detachGPUFromVM(testVMID)
            print("✓ GPU detached")
            
        } catch {
            print("✗ Failed to attach GPU: \(error)")
            exit(1)
        }
        
        print("")
        print("✓ GPU test complete!")
    }
}
