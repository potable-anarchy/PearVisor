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

    public var isRunning: Bool {
        state == .running
    }

    public init(id: UUID = UUID(), name: String, guestOS: String, configuration: VMConfiguration) {
        self.id = id
        self.name = name
        self.guestOS = guestOS
        self.configuration = configuration
    }

    // MARK: - VM Lifecycle

    public func start() async throws {
        print("Starting VM: \(name)")
        state = .starting

        // TODO: Implement VM startup with Virtualization.framework
        // For now, just simulate starting
        try await Task.sleep(for: .seconds(1))

        state = .running
    }

    public func stop() async throws {
        print("Stopping VM: \(name)")
        state = .stopping

        // TODO: Implement graceful shutdown
        try await Task.sleep(for: .seconds(1))

        state = .stopped
    }

    public func pause() async throws {
        guard state == .running else {
            throw VMError.invalidState("Cannot pause VM that is not running")
        }

        print("Pausing VM: \(name)")
        state = .paused

        // TODO: Implement VM pause
    }

    public func resume() async throws {
        guard state == .paused else {
            throw VMError.invalidState("Cannot resume VM that is not paused")
        }

        print("Resuming VM: \(name)")
        state = .running

        // TODO: Implement VM resume
    }

    public func reset() async throws {
        print("Resetting VM: \(name)")

        if state == .running {
            try await stop()
        }

        try await start()
    }

    // MARK: - Snapshots

    public func createSnapshot(name: String) async throws {
        print("Creating snapshot: \(name)")
        // TODO: Implement snapshots
    }

    public func restoreSnapshot(id: UUID) async throws {
        print("Restoring snapshot: \(id)")
        // TODO: Implement snapshot restoration
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
