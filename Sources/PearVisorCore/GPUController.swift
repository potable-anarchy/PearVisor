//
//  GPUController.swift
//  PearVisorCore
//
//  GPU passthrough controller and Metal bridge
//

import Foundation
import Metal

/// Manages GPU passthrough and performance monitoring
public class GPUController {
    public static let shared = GPUController()

    private let metalDevice: MTLDevice?
    private var isInitialized = false

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

        // TODO: Initialize virtio-gpu device
        // TODO: Initialize virglrenderer
        // TODO: Initialize MoltenVK bridge

        isInitialized = true
        print("GPU passthrough initialized successfully")
    }

    // MARK: - GPU Passthrough

    public func attachGPUToVM(_ vmID: UUID) throws {
        guard isInitialized else {
            throw GPUError.notInitialized
        }

        print("Attaching GPU to VM: \(vmID)")

        // TODO: Create virtio-gpu device for VM
        // TODO: Configure shared memory
        // TODO: Start Venus protocol handler
    }

    public func detachGPUFromVM(_ vmID: UUID) {
        print("Detaching GPU from VM: \(vmID)")

        // TODO: Stop Venus protocol handler
        // TODO: Cleanup shared memory
        // TODO: Destroy virtio-gpu device
    }

    // MARK: - Performance Monitoring

    public func getGPUUsage(for vmID: UUID) -> GPUMetrics {
        // TODO: Implement real GPU metrics collection
        return GPUMetrics(
            utilization: 0.0,
            memoryUsed: 0,
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
            vendorID: 0, // Not exposed by Metal API
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
