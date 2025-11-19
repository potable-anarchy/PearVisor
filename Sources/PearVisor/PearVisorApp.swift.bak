//
//  PearVisorApp.swift
//  PearVisor
//
//  Created by PearVisor Contributors
//  Copyright © 2025 PearVisor Contributors. All rights reserved.
//

import SwiftUI
import PearVisorCore

@main
struct PearVisorApp: App {
    @StateObject private var appState = AppState()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(appState)
                .frame(minWidth: 800, minHeight: 600)
        }
        .commands {
            CommandGroup(replacing: .newItem) {
                Button("New Virtual Machine...") {
                    appState.showNewVMWizard = true
                }
                .keyboardShortcut("n", modifiers: .command)
            }
        }

        Settings {
            SettingsView()
                .environmentObject(appState)
        }
    }
}

/// Global application state
@MainActor
class AppState: ObservableObject {
    @Published var virtualMachines: [VirtualMachine] = []
    @Published var selectedVM: VirtualMachine?
    @Published var showNewVMWizard = false

    init() {
        loadVirtualMachines()
    }

    private func loadVirtualMachines() {
        // TODO: Load VMs from disk
        print("Loading virtual machines...")
    }

    func createVirtualMachine(configuration: VMConfiguration) {
        print("Creating VM: \(configuration.name)")
        // TODO: Implement VM creation
    }

    func deleteVirtualMachine(_ vm: VirtualMachine) {
        print("Deleting VM: \(vm.name)")
        // TODO: Implement VM deletion
    }
}
