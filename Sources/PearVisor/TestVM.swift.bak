//
//  TestVM.swift
//  PearVisor
//
//  Simple CLI test to boot a VM
//

import Foundation
import PearVisorCore

@main
struct TestVM {
    static func main() async throws {
        print("PearVisor VM Test")
        print("==================")
        print("")
        
        // Create VM configuration
        let config = VMConfiguration(
            name: "Test Ubuntu VM",
            guestOS: "Ubuntu 24.04 ARM64",
            cpuCount: 4,
            memorySize: 4 * 1024 * 1024 * 1024, // 4GB
            diskSize: 20 * 1024 * 1024 * 1024   // 20GB
        )
        
        print("Creating VM...")
        print("  Name: \(config.name)")
        print("  CPUs: \(config.cpuCount)")
        print("  Memory: \(config.memorySize / 1024 / 1024 / 1024) GB")
        print("  Disk: \(config.diskSize / 1024 / 1024 / 1024) GB")
        print("")
        
        let vm = VirtualMachine(
            name: config.name,
            guestOS: config.guestOS,
            configuration: config
        )
        
        print("VM Directory: \(vm.getVMDirectory().path)")
        print("")
        print("NOTE: For this to work, you need:")
        print("  1. vmlinuz (kernel) in VM directory")
        print("  2. initrd (initial ramdisk) in VM directory") 
        print("  3. disk.img (root filesystem) in VM directory")
        print("")
        print("You can extract kernel/initrd from Ubuntu cloud image using:")
        print("  ./GuestImages/extract-kernel.sh ubuntu-24.04-arm64.img \(vm.getVMDirectory().path)")
        print("")
        
        print("Press Enter to start VM (Ctrl+C to cancel)...")
        _ = readLine()
        
        do {
            print("Starting VM...")
            try await vm.start()
            
            print("")
            print("✓ VM started successfully!")
            print("Console output will appear below.")
            print("Press Ctrl+C to stop VM.")
            print("")
            print("----------------------------------------")
            
            // Keep running until interrupted
            try await Task.sleep(for: .seconds(999999))
            
        } catch {
            print("")
            print("✗ Failed to start VM: \(error)")
            print("")
            exit(1)
        }
    }
}
