// swift-tools-version: 5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "PearVisor",
    platforms: [
        .macOS(.v14)
    ],
    products: [
        // Main application
        .executable(
            name: "PearVisor",
            targets: ["PearVisor"]
        ),
        // VM testing tool
        .executable(
            name: "pearvisor-test",
            targets: ["PearVisorTest"]
        ),
        // Core library (can be used by other tools)
        .library(
            name: "PearVisorCore",
            targets: ["PearVisorCore"]
        ),
    ],
    dependencies: [
        // Add external dependencies here if needed
    ],
    targets: [
        // Main application target
        .executableTarget(
            name: "PearVisor",
            dependencies: ["PearVisorCore", "PearVisorGPU"],
            path: "Sources/PearVisor",
            resources: [
                .process("Resources")
            ]
        ),

        // VM testing tool
        .executableTarget(
            name: "PearVisorTest",
            dependencies: ["PearVisorCore"],
            path: "Sources/PearVisorTest"
        ),

        // Core VM management library
        .target(
            name: "PearVisorCore",
            dependencies: ["PearVisorGPU"],
            path: "Sources/PearVisorCore",
            linkerSettings: [
                .unsafeFlags([
                    "-L/Users/brad/code/PearVisor/GPU/build",
                    "-L/opt/homebrew/lib",
                    "-lPearVisorGPU",
                    "-lvulkan"
                ])
            ]
        ),

        // GPU subsystem Swift wrapper
        .target(
            name: "PearVisorGPU",
            dependencies: [],
            path: "Sources/PearVisorGPU",
            publicHeadersPath: "include"
        ),

        // Tests
        .testTarget(
            name: "PearVisorTests",
            dependencies: ["PearVisor"],
            path: "Tests/PearVisorTests"
        ),
        .testTarget(
            name: "PearVisorCoreTests",
            dependencies: ["PearVisorCore"],
            path: "Tests/PearVisorCoreTests"
        ),
    ]
)
