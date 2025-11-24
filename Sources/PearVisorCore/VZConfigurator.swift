//
//  VZConfigurator.swift
//  PearVisorCore
//
//  Virtualization.framework configuration builder
//

import Foundation
import Virtualization

/// Builds VZVirtualMachineConfiguration from VMConfiguration
public class VZConfigurator {

    // MARK: - Configuration Building

    public static func buildConfiguration(from config: VMConfiguration, vmDirectory: URL, vmID: UUID) throws -> VZVirtualMachineConfiguration {
        let vzConfig = VZVirtualMachineConfiguration()

        // CPU
        vzConfig.cpuCount = config.cpuCount

        // Memory
        vzConfig.memorySize = config.memorySize

        // Boot loader
        vzConfig.bootLoader = try createBootLoader(config: config, vmDirectory: vmDirectory)

        // Storage
        vzConfig.storageDevices = try createStorageDevices(config: config, vmDirectory: vmDirectory)

        // Network
        vzConfig.networkDevices = try createNetworkDevices(config: config)

        // Serial console
        vzConfig.consoleDevices = try createConsoleDevices()

        // Entropy (random number generator)
        vzConfig.entropyDevices = [VZVirtioEntropyDeviceConfiguration()]

        // GPU (if enabled)
        if config.enableGPU {
            vzConfig.graphicsDevices = try createGraphicsDevices(vmID: vmID)
        }

        // Validate configuration
        try vzConfig.validate()

        return vzConfig
    }

    // MARK: - Boot Loader

    private static func createBootLoader(config: VMConfiguration, vmDirectory: URL) throws -> VZBootLoader {
        // Use EFI boot loader (supports ISO boot)
        let efiVariableStore = vmDirectory.appendingPathComponent("efi-variables.bin")

        // Create EFI variable store if it doesn't exist
        if !FileManager.default.fileExists(atPath: efiVariableStore.path) {
            try VZEFIVariableStore(creatingVariableStoreAt: efiVariableStore)
        }

        let variableStore = VZEFIVariableStore(url: efiVariableStore)
        let bootLoader = VZEFIBootLoader()
        bootLoader.variableStore = variableStore

        return bootLoader
    }

    // MARK: - Storage

    private static func createStorageDevices(config: VMConfiguration, vmDirectory: URL) throws -> [VZStorageDeviceConfiguration] {
        var devices: [VZStorageDeviceConfiguration] = []

        // Primary disk
        let diskURL = config.diskURL ?? vmDirectory.appendingPathComponent("disk.img")

        if !FileManager.default.fileExists(atPath: diskURL.path) {
            try createDiskImage(at: diskURL, size: config.diskSize)
        }

        let diskAttachment = try VZDiskImageStorageDeviceAttachment(url: diskURL, readOnly: false)
        let blockDevice = VZVirtioBlockDeviceConfiguration(attachment: diskAttachment)
        devices.append(blockDevice)

        // ISO CD-ROM (if specified)
        if let isoPath = config.isoPath {
            let isoURL = URL(fileURLWithPath: isoPath)
            guard FileManager.default.fileExists(atPath: isoURL.path) else {
                throw VMError.invalidConfiguration("ISO file not found: \(isoPath)")
            }

            let isoAttachment = try VZDiskImageStorageDeviceAttachment(url: isoURL, readOnly: true)
            let cdromDevice = VZVirtioBlockDeviceConfiguration(attachment: isoAttachment)
            devices.append(cdromDevice)

            print("Attached ISO: \(isoPath)")
        }

        return devices
    }

    private static func createDiskImage(at url: URL, size: UInt64) throws {
        let fd = open(url.path, O_RDWR | O_CREAT, 0o644)
        guard fd != -1 else {
            throw VMError.diskImageNotFound
        }
        defer { close(fd) }

        guard ftruncate(fd, Int64(size)) == 0 else {
            throw VMError.invalidConfiguration("Failed to create disk image")
        }

        print("Created disk image: \(url.path) (\(size / 1024 / 1024 / 1024) GB)")
    }

    // MARK: - Networking

    private static func createNetworkDevices(config: VMConfiguration) throws -> [VZNetworkDeviceConfiguration] {
        var devices: [VZNetworkDeviceConfiguration] = []

        let networkDevice = VZVirtioNetworkDeviceConfiguration()

        switch config.networkMode {
        case .nat:
            networkDevice.attachment = VZNATNetworkDeviceAttachment()
        case .bridged:
            print("Warning: Bridged networking not yet implemented, using NAT")
            networkDevice.attachment = VZNATNetworkDeviceAttachment()
        case .hostOnly:
            print("Warning: Host-only networking not yet implemented, using NAT")
            networkDevice.attachment = VZNATNetworkDeviceAttachment()
        }

        devices.append(networkDevice)
        return devices
    }

    // MARK: - Console

    private static func createConsoleDevices() throws -> [VZConsoleDeviceConfiguration] {
        var devices: [VZConsoleDeviceConfiguration] = []

        let consoleDevice = VZVirtioConsoleDeviceConfiguration()
        let consolePort = VZVirtioConsolePortConfiguration()
        consolePort.name = "console"
        consolePort.isConsole = true

        let inputFileHandle = FileHandle.standardInput
        let outputFileHandle = FileHandle.standardOutput

        consolePort.attachment = VZFileHandleSerialPortAttachment(
            fileHandleForReading: inputFileHandle,
            fileHandleForWriting: outputFileHandle
        )

        consoleDevice.ports[0] = consolePort
        devices.append(consoleDevice)

        return devices
    }

    // MARK: - Graphics

    private static var gpuIntegration: GPUIntegration?

    private static func createGraphicsDevices(vmID: UUID) throws -> [VZGraphicsDeviceConfiguration] {
        // Create GPU integration instance (manages Venus GPU virtualization)
        gpuIntegration = GPUIntegration(vmID: vmID)

        // Initialize Venus subsystem
        // This sets up:
        //   - Shared memory ring buffer for GPU commands
        //   - MoltenVK context for Metal translation
        //   - 25 Venus protocol handlers
        do {
            try gpuIntegration?.initializeVenus()
            print("Venus GPU virtualization initialized ✓")
        } catch {
            print("Warning: Failed to initialize Venus GPU: \(error)")
            print("Falling back to basic graphics device")
        }

        // Create VZ graphics device with Venus support
        let graphicsDevice = try gpuIntegration?.createGraphicsDevice() ?? createBasicGraphicsDevice()

        return [graphicsDevice]
    }

    /// Creates basic VirtIO GPU without Venus acceleration
    /// Used as fallback if Venus initialization fails
    private static func createBasicGraphicsDevice() -> VZVirtioGraphicsDeviceConfiguration {
        let graphicsDevice = VZVirtioGraphicsDeviceConfiguration()

        graphicsDevice.scanouts = [
            VZVirtioGraphicsScanoutConfiguration(
                widthInPixels: 1920,
                heightInPixels: 1080
            )
        ]

        return graphicsDevice
    }

    /// Cleanup GPU resources when VM is destroyed
    public static func cleanupGPU() {
        if let gpu = gpuIntegration {
            gpu.cleanup()
            gpuIntegration = nil
            print("Venus GPU resources cleaned up")
        }
    }
}

// MARK: - VMConfiguration Extensions

extension VMConfiguration {
    public var kernelURL: URL? { nil }
    public var initrdURL: URL? { nil }
    public var kernelCommandLine: String? { nil }
    public var diskURL: URL? { nil }
}
