//
//  ContentView.swift
//  PearVisor
//
//  Main application view with VM library
//

import SwiftUI
import PearVisorCore

struct ContentView: View {
    @EnvironmentObject var appState: AppState

    var body: some View {
        NavigationSplitView {
            // Sidebar: VM list
            VMListView()
                .frame(minWidth: 200)
        } detail: {
            // Detail: VM viewer or welcome screen
            if let vm = appState.selectedVM {
                VMDetailView(vm: vm)
            } else {
                WelcomeView()
            }
        }
        .sheet(isPresented: $appState.showNewVMWizard) {
            NewVMWizard()
        }
    }
}

struct VMListView: View {
    @EnvironmentObject var appState: AppState

    var body: some View {
        List(selection: $appState.selectedVM) {
            Section("Virtual Machines") {
                if appState.virtualMachines.isEmpty {
                    Text("No virtual machines")
                        .foregroundColor(.secondary)
                        .italic()
                } else {
                    ForEach(appState.virtualMachines) { vm in
                        VMListItem(vm: vm)
                    }
                }
            }
        }
        .navigationTitle("PearVisor")
        .toolbar {
            ToolbarItem(placement: .primaryAction) {
                Button {
                    appState.showNewVMWizard = true
                } label: {
                    Label("New VM", systemImage: "plus")
                }
            }
        }
    }
}

struct VMListItem: View {
    let vm: VirtualMachine

    var body: some View {
        HStack {
            Image(systemName: vm.isRunning ? "circle.fill" : "circle")
                .foregroundColor(vm.isRunning ? .green : .secondary)
                .font(.caption)

            VStack(alignment: .leading, spacing: 2) {
                Text(vm.name)
                    .font(.headline)
                Text(vm.guestOS)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
        .padding(.vertical, 4)
    }
}

struct WelcomeView: View {
    @EnvironmentObject var appState: AppState

    var body: some View {
        VStack(spacing: 20) {
            Image(systemName: "cpu")
                .font(.system(size: 64))
                .foregroundColor(.accentColor)

            Text("Welcome to PearVisor")
                .font(.largeTitle)
                .bold()

            Text("High-performance virtualization for Apple Silicon")
                .font(.title3)
                .foregroundColor(.secondary)

            Button("Create Your First Virtual Machine") {
                appState.showNewVMWizard = true
            }
            .buttonStyle(.borderedProminent)
            .controlSize(.large)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
}

struct VMDetailView: View {
    let vm: VirtualMachine

    var body: some View {
        VStack {
            Text("VM: \(vm.name)")
                .font(.largeTitle)

            Text("Status: \(vm.isRunning ? "Running" : "Stopped")")
                .foregroundColor(.secondary)

            Spacer()

            HStack(spacing: 12) {
                Button(vm.isRunning ? "Stop" : "Start") {
                    // TODO: Toggle VM state
                }
                .buttonStyle(.borderedProminent)

                Button("Settings") {
                    // TODO: Show settings
                }
            }

            Spacer()
        }
        .padding()
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
}

struct NewVMWizard: View {
    @Environment(\.dismiss) var dismiss
    @EnvironmentObject var appState: AppState
    @State private var vmName = "Ubuntu VM"
    @State private var selectedOS = "Ubuntu 24.04 ARM64"

    var body: some View {
        VStack(spacing: 20) {
            Text("Create New Virtual Machine")
                .font(.title)
                .bold()

            Form {
                TextField("Name", text: $vmName)
                Picker("Operating System", selection: $selectedOS) {
                    Text("Ubuntu 24.04 ARM64").tag("Ubuntu 24.04 ARM64")
                    Text("Fedora 40 ARM64").tag("Fedora 40 ARM64")
                    Text("Debian 12 ARM64").tag("Debian 12 ARM64")
                }
            }
            .padding()

            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .keyboardShortcut(.cancelAction)

                Spacer()

                Button("Create") {
                    let config = VMConfiguration(
                        name: vmName,
                        guestOS: selectedOS,
                        cpuCount: 4,
                        memorySize: 8 * 1024 * 1024 * 1024 // 8GB
                    )
                    appState.createVirtualMachine(configuration: config)
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .keyboardShortcut(.defaultAction)
            }
            .padding()
        }
        .frame(width: 500, height: 400)
    }
}

struct SettingsView: View {
    var body: some View {
        TabView {
            GeneralSettingsView()
                .tabItem {
                    Label("General", systemImage: "gearshape")
                }

            PerformanceSettingsView()
                .tabItem {
                    Label("Performance", systemImage: "speedometer")
                }
        }
        .frame(width: 500, height: 400)
    }
}

struct GeneralSettingsView: View {
    var body: some View {
        Form {
            Section("Virtual Machines") {
                Text("Default VM storage location")
                Text("~/Library/Containers/PearVisor/VMs")
                    .foregroundColor(.secondary)
            }
        }
        .padding()
    }
}

struct PerformanceSettingsView: View {
    var body: some View {
        Form {
            Section("GPU Acceleration") {
                Toggle("Enable GPU passthrough", isOn: .constant(true))
                Toggle("Use MoltenVK optimization", isOn: .constant(true))
            }
        }
        .padding()
    }
}

// Preview removed - requires Xcode 15+ with macro support
// To preview, open Package.swift in Xcode and use the built-in preview
