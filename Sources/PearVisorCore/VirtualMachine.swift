//
//  VirtualMachine.swift
//  PearVisorCore
//
//  Core virtual machine model and lifecycle management
//

import Foundation
import Virtualization

/// Represents a virtual machine instance
public class VirtualMachine: Identifiable, ObservableObject {
    public let id: UUID
    public let name: String
    public let guestOS: String
    public let configuration: VMConfiguration

    @Published public private(set) var state: VMState = .stopped
    @Published public private(set) var cpuUsage: Double = 0.0
    @Published public private(set) var memoryUsage: UInt64 = 0
    @Published public private(set) var gpuUsage: Double = 0.0

    private var vzVirtualMachine: VZVirtualMachine?
    private var vmDirectory: URL

    public var isRunning: Bool {
        state == .running
    }

    public init(id: UUID = UUID(), name: String, guestOS: String, configuration: VMConfiguration) {
        self.id = id
        self.name = name
        self.guestOS = guestOS
        self.configuration = configuration

        // Create VM directory
        let containerPath = NSSearchPathForDirectoriesInDomains(.applicationSupportDirectory, .userDomainMask, true).first!
        let baseDir = URL(fileURLWithPath: containerPath).appendingPathComponent("PearVisor/VMs")
        self.vmDirectory = baseDir.appendingPathComponent(id.uuidString)

        // Create directory if needed
        try? FileManager.default.createDirectory(at: vmDirectory, withIntermediateDirectories: true)
    }

    // MARK: - VM Lifecycle

    public func start() async throws {
        guard state == .stopped else {
            throw VMError.invalidState("VM must be stopped to start")
        }

        print("Starting VM: \(name)")
        state = .starting

        do {
            // Build VZ configuration
            let vzConfig = try VZConfigurator.buildConfiguration(from: configuration, vmDirectory: vmDirectory, vmID: id)

            // Create VZ virtual machine
            let vm = VZVirtualMachine(configuration: vzConfig)
            self.vzVirtualMachine = vm

            // Start the VM
            try await vm.start()

            state = .running
            print("VM started successfully: \(name)")
        } catch {
            state = .error
            print("Failed to start VM: \(error)")
            throw VMError.startupFailed(error.localizedDescription)
        }
    }

    public func stop() async throws {
        guard let vm = vzVirtualMachine else {
            throw VMError.invalidState("VM not initialized")
        }

        guard state == .running || state == .paused else {
            throw VMError.invalidState("VM must be running or paused to stop")
        }

        print("Stopping VM: \(name)")
        state = .stopping

        do {
            // Try graceful shutdown first
            if vm.canRequestStop {
                try await vm.requestStop()
            } else {
                // Force stop
                try await vm.stop()
            }

            state = .stopped
            vzVirtualMachine = nil
            print("VM stopped successfully: \(name)")
        } catch {
            state = .error
            print("Failed to stop VM: \(error)")
            throw VMError.shutdownFailed(error.localizedDescription)
        }
    }

    public func pause() async throws {
        guard let vm = vzVirtualMachine else {
            throw VMError.invalidState("VM not initialized")
        }

        guard state == .running else {
            throw VMError.invalidState("Cannot pause VM that is not running")
        }

        guard vm.canPause else {
            throw VMError.invalidState("VM does not support pausing")
        }

        print("Pausing VM: \(name)")

        do {
            try await vm.pause()
            state = .paused
            print("VM paused successfully: \(name)")
        } catch {
            print("Failed to pause VM: \(error)")
            throw error
        }
    }

    public func resume() async throws {
        guard let vm = vzVirtualMachine else {
            throw VMError.invalidState("VM not initialized")
        }

        guard state == .paused else {
            throw VMError.invalidState("Cannot resume VM that is not paused")
        }

        guard vm.canResume else {
            throw VMError.invalidState("VM does not support resuming")
        }

        print("Resuming VM: \(name)")

        do {
            try await vm.resume()
            state = .running
            print("VM resumed successfully: \(name)")
        } catch {
            print("Failed to resume VM: \(error)")
            throw error
        }
    }

    public func reset() async throws {
        print("Resetting VM: \(name)")

        if state == .running || state == .paused {
            try await stop()
        }

        try await start()
    }

    // MARK: - Snapshots

    public func createSnapshot(name: String) async throws {
        print("Creating snapshot: \(name)")
        // TODO: Implement snapshots with VZVirtualMachine.saveRestorationState
    }

    public func restoreSnapshot(id: UUID) async throws {
        print("Restoring snapshot: \(id)")
        // TODO: Implement snapshot restoration
    }

    // MARK: - VM Information

    public func getVMDirectory() -> URL {
        return vmDirectory
    }
}

// MARK: - VM State

public enum VMState: String {
    case stopped
    case starting
    case running
    case paused
    case stopping
    case error
}

// MARK: - VM Configuration

public struct VMConfiguration: Codable {
    public let name: String
    public let guestOS: String
    public let cpuCount: Int
    public let memorySize: UInt64
    public var diskSize: UInt64 = 64 * 1024 * 1024 * 1024 // 64GB default
    public var enableGPU: Bool = true
    public var enableSharedDirectories: Bool = true
    public var networkMode: NetworkMode = .nat

    public init(
        name: String,
        guestOS: String,
        cpuCount: Int,
        memorySize: UInt64,
        diskSize: UInt64 = 64 * 1024 * 1024 * 1024,
        enableGPU: Bool = true,
        enableSharedDirectories: Bool = true,
        networkMode: NetworkMode = .nat
    ) {
        self.name = name
        self.guestOS = guestOS
        self.cpuCount = cpuCount
        self.memorySize = memorySize
        self.diskSize = diskSize
        self.enableGPU = enableGPU
        self.enableSharedDirectories = enableSharedDirectories
        self.networkMode = networkMode
    }
}

public enum NetworkMode: String, Codable {
    case nat
    case bridged
    case hostOnly
}

// MARK: - VM Errors

public enum VMError: Error, LocalizedError {
    case invalidConfiguration(String)
    case invalidState(String)
    case startupFailed(String)
    case shutdownFailed(String)
    case gpuNotAvailable
    case diskImageNotFound

    public var errorDescription: String? {
        switch self {
        case .invalidConfiguration(let msg):
            return "Invalid configuration: \(msg)"
        case .invalidState(let msg):
            return "Invalid state: \(msg)"
        case .startupFailed(let msg):
            return "Startup failed: \(msg)"
        case .shutdownFailed(let msg):
            return "Shutdown failed: \(msg)"
        case .gpuNotAvailable:
            return "GPU acceleration is not available"
        case .diskImageNotFound:
            return "Disk image not found"
        }
    }
}

// MARK: - Hashable & Equatable

extension VirtualMachine: Hashable {
    public static func == (lhs: VirtualMachine, rhs: VirtualMachine) -> Bool {
        lhs.id == rhs.id
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine(id)
    }
}
