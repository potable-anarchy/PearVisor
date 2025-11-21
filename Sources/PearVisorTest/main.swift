//
//  VMTest.swift
//  PearVisor
//
//  Simple command-line tool to test VM creation and booting
//

import Foundation
import PearVisorCore
import Virtualization

/// Command-line VM testing tool
@available(macOS 14.0, *)
class VMTest {

    static func printUsage() {
        print("""
        PearVisor VM Test Tool

        Usage:
            pearvisor-test create <name> <iso-path>    Create and start a new VM
            pearvisor-test list                        List all VMs
            pearvisor-test start <name>                Start an existing VM
            pearvisor-test stop <name>                 Stop a running VM

        Examples:
            pearvisor-test create ubuntu-test GuestImages/ubuntu-24.04.3-live-server-arm64.iso
            pearvisor-test start ubuntu-test
        """)
    }

    static func createVM(name: String, isoPath: String) async throws {
        print("Creating VM: \(name)")
        print("ISO: \(isoPath)")

        // Verify ISO exists
        let isoURL = URL(fileURLWithPath: isoPath)
        guard FileManager.default.fileExists(atPath: isoURL.path) else {
            throw VMError.invalidConfiguration("ISO file not found: \(isoPath)")
        }

        // Create VM configuration
        let config = VMConfiguration(
            name: name,
            guestOS: "Ubuntu 24.04",
            cpuCount: 4,
            memorySize: 4 * 1024 * 1024 * 1024, // 4GB
            diskSize: 32 * 1024 * 1024 * 1024, // 32GB
            enableGPU: true,
            enableSharedDirectories: false,
            networkMode: .nat
        )

        print("Configuration:")
        print("  CPUs: \(config.cpuCount)")
        print("  Memory: \(config.memorySize / 1024 / 1024 / 1024) GB")
        print("  Disk: \(config.diskSize / 1024 / 1024 / 1024) GB")
        print("  GPU: \(config.enableGPU ? "Enabled" : "Disabled")")
        print("  Network: \(config.networkMode)")

        // Create VM instance
        let vm = VirtualMachine(
            name: name,
            guestOS: "Ubuntu 24.04",
            configuration: config
        )

        print("\nVM ID: \(vm.id)")
        print("VM Directory: ~/Library/Application Support/PearVisor/VMs/\(vm.id)")

        // Start VM
        print("\nStarting VM...")
        print("(This will open a console window)")
        print("(Press Ctrl+C to stop)")

        do {
            try await vm.start()

            print("\n✅ VM started successfully!")
            print("\nInstallation instructions:")
            print("1. Follow Ubuntu installer prompts")
            print("2. After installation completes, reboot")
            print("3. Login to guest system")
            print("4. Copy GuestImages/setup-ubuntu-venus.sh to guest")
            print("5. Run: sudo ./setup-ubuntu-venus.sh")
            print("6. Test GPU: vkcube")

            // Keep running until interrupted
            try await Task.sleep(for: .seconds(3600)) // 1 hour timeout

        } catch {
            print("\n❌ VM startup failed: \(error)")
            throw error
        }
    }

    static func listVMs() {
        print("Listing VMs...")

        let containerPath = NSSearchPathForDirectoriesInDomains(.applicationSupportDirectory, .userDomainMask, true).first!
        let baseDir = URL(fileURLWithPath: containerPath).appendingPathComponent("PearVisor/VMs")

        guard FileManager.default.fileExists(atPath: baseDir.path) else {
            print("No VMs found.")
            return
        }

        do {
            let vmDirs = try FileManager.default.contentsOfDirectory(
                at: baseDir,
                includingPropertiesForKeys: [.creationDateKey],
                options: [.skipsHiddenFiles]
            )

            if vmDirs.isEmpty {
                print("No VMs found.")
            } else {
                print("\nFound \(vmDirs.count) VM(s):\n")
                for vmDir in vmDirs {
                    let uuid = vmDir.lastPathComponent
                    let diskPath = vmDir.appendingPathComponent("disk.img")
                    let diskExists = FileManager.default.fileExists(atPath: diskPath.path)

                    print("  ID: \(uuid)")
                    print("  Path: \(vmDir.path)")
                    print("  Disk: \(diskExists ? "✓" : "✗")")
                    print()
                }
            }
        } catch {
            print("Error listing VMs: \(error)")
        }
    }

    static func run(arguments: [String]) async throws {
        guard arguments.count >= 2 else {
            printUsage()
            return
        }

        let command = arguments[1]

        switch command {
        case "create":
            guard arguments.count >= 4 else {
                print("Error: create requires <name> and <iso-path>")
                printUsage()
                return
            }
            let name = arguments[2]
            let isoPath = arguments[3]
            try await createVM(name: name, isoPath: isoPath)

        case "list":
            listVMs()

        case "start", "stop":
            print("Error: start/stop not yet implemented")
            print("Use 'create' to create and start a new VM")

        default:
            print("Error: Unknown command '\(command)'")
            printUsage()
        }
    }
}

// Main entry point
@available(macOS 14.0, *)
@main
struct PearVisorTestMain {
    static func main() async {
        do {
            let arguments = CommandLine.arguments
            try await VMTest.run(arguments: arguments)
        } catch {
            print("\nError: \(error)")
            exit(1)
        }
    }
}
