//
//  GPUController.swift
//  PearVisorCore
//
//  GPU passthrough controller with real Metal bridge
//

import Foundation
import Metal

/// Manages GPU passthrough and performance monitoring
public class GPUController {
    public static let shared = GPUController()
    
    private let metalDevice: MTLDevice?
    private var isInitialized = false
    
    // C GPU devices (one per VM)
    private var gpuDevices: [UUID: OpaquePointer] = [:]
    
    public var isGPUAvailable: Bool {
        metalDevice != nil
    }
    
    public var gpuName: String {
        metalDevice?.name ?? "No GPU"
    }
    
    private init() {
        metalDevice = MTLCreateSystemDefaultDevice()
        print("GPU Controller initialized: \(gpuName)")
    }
    
    // MARK: - GPU Initialization
    
    public func initialize() throws {
        guard let device = metalDevice else {
            throw GPUError.deviceNotAvailable
        }
        
        print("Initializing GPU passthrough with Metal device: \(device.name)")
        
        // Initialize C GPU subsystem
        let result = pv_gpu_init()
        guard result == 0 else { // PV_GPU_OK
            throw GPUError.metalError("Failed to initialize GPU subsystem: \(result)")
        }
        
        isInitialized = true
        print("GPU passthrough initialized successfully")
    }
    
    // MARK: - GPU Passthrough
    
    public func attachGPUToVM(_ vmID: UUID) throws -> OpaquePointer {
        guard isInitialized else {
            throw GPUError.notInitialized
        }
        
        print("Attaching GPU to VM: \(vmID)")
        
        // Create C GPU device for this VM
        var devicePtr: OpaquePointer?
        let uuid = vmID.uuid
        
        withUnsafeBytes(of: uuid) { uuidBytes in
            let result = pv_gpu_create_device(
                uuidBytes.baseAddress!.assumingMemoryBound(to: UInt8.self),
                &devicePtr
            )
            
            if result != 0 { // PV_GPU_OK
                print("Failed to create GPU device: \(result)")
            }
        }
        
        guard let device = devicePtr else {
            throw GPUError.attachmentFailed("Failed to create GPU device")
        }
        
        // Store device for this VM
        gpuDevices[vmID] = device
        
        // Start command processing
        let result = pv_gpu_start_venus(device)
        guard result == 0 else {
            throw GPUError.attachmentFailed("Failed to start GPU processing: \(result)")
        }
        
        print("GPU attached successfully to VM: \(vmID)")
        return device
    }
    
    public func detachGPUFromVM(_ vmID: UUID) {
        guard let device = gpuDevices[vmID] else {
            print("No GPU device found for VM: \(vmID)")
            return
        }
        
        print("Detaching GPU from VM: \(vmID)")
        
        // Stop command processing
        pv_gpu_stop_venus(device)
        
        // Destroy device
        pv_gpu_destroy_device(device)
        
        // Remove from tracking
        gpuDevices.removeValue(forKey: vmID)
        
        print("GPU detached from VM: \(vmID)")
    }
    
    // MARK: - Performance Monitoring
    
    public func getGPUUsage(for vmID: UUID) -> GPUMetrics {
        guard let device = gpuDevices[vmID] else {
            return GPUMetrics(
                utilization: 0.0,
                memoryUsed: 0,
                memoryTotal: metalDevice?.recommendedMaxWorkingSetSize ?? 0,
                temperature: 0.0,
                powerUsage: 0.0
            )
        }
        
        let utilization = pv_gpu_get_utilization(device)
        let memoryUsed = pv_gpu_get_memory_usage(device)
        
        return GPUMetrics(
            utilization: utilization,
            memoryUsed: memoryUsed,
            memoryTotal: metalDevice?.recommendedMaxWorkingSetSize ?? 0,
            temperature: 0.0,
            powerUsage: 0.0
        )
    }
    
    // MARK: - GPU Information
    
    public func getGPUInfo() -> GPUInfo {
        guard let device = metalDevice else {
            return GPUInfo(
                name: "No GPU",
                vendorID: 0,
                isLowPower: false,
                isRemovable: false,
                recommendedMaxWorkingSetSize: 0,
                supportsRaytracing: false
            )
        }
        
        return GPUInfo(
            name: device.name,
            vendorID: 0,
            isLowPower: device.isLowPower,
            isRemovable: device.isRemovable,
            recommendedMaxWorkingSetSize: device.recommendedMaxWorkingSetSize,
            supportsRaytracing: device.supportsRaytracing
        )
    }
}

// MARK: - GPU Metrics

public struct GPUMetrics {
    public let utilization: Double // 0.0 - 1.0
    public let memoryUsed: UInt64
    public let memoryTotal: UInt64
    public let temperature: Double
    public let powerUsage: Double
    
    public var memoryUsagePercentage: Double {
        guard memoryTotal > 0 else { return 0.0 }
        return Double(memoryUsed) / Double(memoryTotal)
    }
}

// MARK: - GPU Info

public struct GPUInfo {
    public let name: String
    public let vendorID: UInt32
    public let isLowPower: Bool
    public let isRemovable: Bool
    public let recommendedMaxWorkingSetSize: UInt64
    public let supportsRaytracing: Bool
    
    public var formattedMemory: String {
        let gb = Double(recommendedMaxWorkingSetSize) / 1_073_741_824
        return String(format: "%.1f GB", gb)
    }
}

// MARK: - GPU Errors

public enum GPUError: Error, LocalizedError {
    case deviceNotAvailable
    case notInitialized
    case attachmentFailed(String)
    case detachmentFailed(String)
    case vulkanError(String)
    case metalError(String)
    
    public var errorDescription: String? {
        switch self {
        case .deviceNotAvailable:
            return "GPU device not available"
        case .notInitialized:
            return "GPU controller not initialized"
        case .attachmentFailed(let msg):
            return "GPU attachment failed: \(msg)"
        case .detachmentFailed(let msg):
            return "GPU detachment failed: \(msg)"
        case .vulkanError(let msg):
            return "Vulkan error: \(msg)"
        case .metalError(let msg):
            return "Metal error: \(msg)"
        }
    }
}

// MARK: - C Interop

// Import C functions from GPU subsystem
@_silgen_name("pv_gpu_init")
func pv_gpu_init() -> Int32

@_silgen_name("pv_gpu_create_device")
func pv_gpu_create_device(_ vmID: UnsafePointer<UInt8>, _ device: UnsafeMutablePointer<OpaquePointer?>) -> Int32

@_silgen_name("pv_gpu_destroy_device")
func pv_gpu_destroy_device(_ device: OpaquePointer)

@_silgen_name("pv_gpu_start_venus")
func pv_gpu_start_venus(_ device: OpaquePointer) -> Int32

@_silgen_name("pv_gpu_stop_venus")
func pv_gpu_stop_venus(_ device: OpaquePointer)

@_silgen_name("pv_gpu_get_utilization")
func pv_gpu_get_utilization(_ device: OpaquePointer) -> Double

@_silgen_name("pv_gpu_get_memory_usage")
func pv_gpu_get_memory_usage(_ device: OpaquePointer) -> UInt64
