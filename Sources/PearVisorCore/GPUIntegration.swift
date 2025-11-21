//
//  GPUIntegration.swift
//  PearVisorCore
//
//  Bridges Virtualization.framework with PearVisor GPU subsystem
//  Connects Venus protocol stack to guest VMs
//

import Foundation
import Virtualization

/// Integrates PearVisor GPU subsystem with VZ.framework
public class GPUIntegration {

    // MARK: - Properties

    private var venusContext: OpaquePointer?
    private var ringBuffer: OpaquePointer?
    private var sharedMemoryRegion: UnsafeMutableRawPointer?
    private var sharedMemorySize: Int = 4 * 1024 * 1024 // 4MB default
    private var isRunning = false

    public let vmID: UUID

    // MARK: - Initialization

    public init(vmID: UUID) {
        self.vmID = vmID
        print("[GPUIntegration] Initialized for VM: \(vmID)")
    }

    deinit {
        cleanup()
    }

    // MARK: - GPU Device Configuration

    /// Creates VZ graphics device configuration with Venus support
    public func createGraphicsDevice() throws -> VZVirtioGraphicsDeviceConfiguration {
        print("[GPUIntegration] Creating VirtioGraphicsDevice with Venus protocol support")

        let graphicsDevice = VZVirtioGraphicsDeviceConfiguration()

        // Configure scanout (display output)
        graphicsDevice.scanouts = [
            VZVirtioGraphicsScanoutConfiguration(
                widthInPixels: 1920,
                heightInPixels: 1080
            )
        ]

        print("[GPUIntegration] Graphics device configured: 1920x1080")
        return graphicsDevice
    }

    // MARK: - Venus Integration

    /// Initialize Venus protocol handler and ring buffer
    public func initializeVenus() throws {
        print("[GPUIntegration] Initializing Venus protocol stack")

        // Allocate shared memory region for ring buffer
        sharedMemoryRegion = allocateSharedMemory(size: sharedMemorySize)
        guard sharedMemoryRegion != nil else {
            throw GPUIntegrationError.sharedMemoryAllocationFailed
        }
        print("[GPUIntegration] Allocated shared memory: \(sharedMemorySize) bytes")

        // Initialize Venus ring buffer (call into C subsystem)
        ringBuffer = pv_venus_ring_create_from_memory(
            sharedMemoryRegion,
            UInt32(sharedMemorySize)
        )

        guard ringBuffer != nil else {
            deallocateSharedMemory()
            throw GPUIntegrationError.ringBufferInitializationFailed
        }
        print("[GPUIntegration] Venus ring buffer created")

        // Initialize Venus handler context (MoltenVK + dispatch)
        venusContext = pv_venus_init()
        guard venusContext != nil else {
            pv_venus_ring_destroy(ringBuffer)
            deallocateSharedMemory()
            throw GPUIntegrationError.venusInitializationFailed
        }
        print("[GPUIntegration] Venus handler context initialized")

        // Start ring buffer processing
        let result = pv_venus_integration_start(ringBuffer, venusContext)
        guard result == 0 else {
            pv_venus_cleanup(venusContext)
            pv_venus_ring_destroy(ringBuffer)
            deallocateSharedMemory()
            throw GPUIntegrationError.ringBufferStartFailed
        }

        isRunning = true
        print("[GPUIntegration] Venus protocol stack operational")
    }

    /// Stop Venus protocol handler
    public func stopVenus() {
        guard isRunning else { return }

        print("[GPUIntegration] Stopping Venus protocol stack")

        if let ring = ringBuffer {
            pv_venus_integration_stop(ring)
            pv_venus_ring_destroy(ring)
            ringBuffer = nil
        }

        if let ctx = venusContext {
            pv_venus_cleanup(ctx)
            venusContext = nil
        }

        deallocateSharedMemory()
        isRunning = false

        print("[GPUIntegration] Venus protocol stack stopped")
    }

    // MARK: - Shared Memory Management

    private func allocateSharedMemory(size: Int) -> UnsafeMutableRawPointer? {
        // Allocate page-aligned memory for VM shared region
        var ptr: UnsafeMutableRawPointer?
        let result = posix_memalign(&ptr, 4096, size)

        guard result == 0, let memory = ptr else {
            print("[GPUIntegration] Failed to allocate shared memory: \(result)")
            return nil
        }

        // Zero out memory
        memset(memory, 0, size)

        return memory
    }

    private func deallocateSharedMemory() {
        if let memory = sharedMemoryRegion {
            free(memory)
            sharedMemoryRegion = nil
            print("[GPUIntegration] Shared memory deallocated")
        }
    }

    /// Get shared memory pointer for virtio-gpu device
    public func getSharedMemoryRegion() -> (pointer: UnsafeMutableRawPointer, size: Int)? {
        guard let memory = sharedMemoryRegion else { return nil }
        return (memory, sharedMemorySize)
    }

    // MARK: - virtio-gpu Device Handling

    /// Handle virtio-gpu command from guest
    /// This will be called by VZ.framework when guest writes to virtio-gpu device
    public func handleVirtioCommand(_ command: Data) {
        // Guest writes to virtio-gpu control queue end up here
        // For Venus, we mostly care about the 3D command queue

        print("[GPUIntegration] Received virtio command: \(command.count) bytes")

        // Commands are already in ring buffer (guest writes directly)
        // Just trigger the decoder to process available commands
        if let ring = ringBuffer {
            pv_venus_ring_notify(ring)
        }
    }

    // MARK: - Statistics

    /// Get Venus protocol statistics
    public func getStatistics() -> VenusStatistics {
        guard let ctx = venusContext else {
            return VenusStatistics(
                commandsProcessed: 0,
                objectsCreated: 0,
                errorsEncountered: 0,
                ringBufferUtilization: 0.0
            )
        }

        let stats = pv_venus_get_stats(ctx)

        return VenusStatistics(
            commandsProcessed: UInt64(stats.commands_handled),
            objectsCreated: UInt64(stats.objects_created),
            errorsEncountered: UInt64(stats.errors),
            ringBufferUtilization: pv_venus_ring_utilization(ringBuffer)
        )
    }

    // MARK: - Cleanup

    private func cleanup() {
        stopVenus()
    }
}

// MARK: - Venus Statistics

public struct VenusStatistics {
    public let commandsProcessed: UInt64
    public let objectsCreated: UInt64
    public let errorsEncountered: UInt64
    public let ringBufferUtilization: Double

    public var description: String {
        """
        Venus Statistics:
          Commands Processed: \(commandsProcessed)
          Objects Created: \(objectsCreated)
          Errors: \(errorsEncountered)
          Ring Buffer: \(String(format: "%.1f%%", ringBufferUtilization * 100))
        """
    }
}

// MARK: - GPU Integration Errors

public enum GPUIntegrationError: Error, LocalizedError {
    case sharedMemoryAllocationFailed
    case ringBufferInitializationFailed
    case venusInitializationFailed
    case ringBufferStartFailed
    case notInitialized

    public var errorDescription: String? {
        switch self {
        case .sharedMemoryAllocationFailed:
            return "Failed to allocate shared memory for GPU communication"
        case .ringBufferInitializationFailed:
            return "Failed to initialize Venus ring buffer"
        case .venusInitializationFailed:
            return "Failed to initialize Venus protocol handler"
        case .ringBufferStartFailed:
            return "Failed to start ring buffer processing thread"
        case .notInitialized:
            return "GPU integration not initialized"
        }
    }
}

// MARK: - C Interop Functions (to be implemented in GPU subsystem)

// These functions will be implemented in the C GPU subsystem
// and exposed via bridging header

@_silgen_name("pv_venus_ring_create_from_memory")
func pv_venus_ring_create_from_memory(
    _ memory: UnsafeMutableRawPointer?,
    _ size: UInt32
) -> OpaquePointer?

@_silgen_name("pv_venus_ring_destroy")
func pv_venus_ring_destroy(_ ring: OpaquePointer?)

@_silgen_name("pv_venus_integration_start")
func pv_venus_integration_start(_ ring: OpaquePointer?, _ context: OpaquePointer?) -> Int32

@_silgen_name("pv_venus_integration_stop")
func pv_venus_integration_stop(_ ring: OpaquePointer?)

@_silgen_name("pv_venus_ring_notify")
func pv_venus_ring_notify(_ ring: OpaquePointer?)

@_silgen_name("pv_venus_ring_utilization")
func pv_venus_ring_utilization(_ ring: OpaquePointer?) -> Double

@_silgen_name("pv_venus_init")
func pv_venus_init() -> OpaquePointer?

@_silgen_name("pv_venus_cleanup")
func pv_venus_cleanup(_ context: OpaquePointer?)

@_silgen_name("pv_venus_get_stats")
func pv_venus_get_stats(_ context: OpaquePointer?) -> pv_venus_stats

// Venus statistics structure (matches C struct)
struct pv_venus_stats {
    var commands_handled: UInt32
    var objects_created: UInt32
    var errors: UInt32
    var _padding: UInt32
}
