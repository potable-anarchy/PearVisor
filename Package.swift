// swift-tools-version: 5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "PearVisor",
    platforms: [
        .macOS(.v15)
    ],
    products: [
        // Main application
        .executable(
            name: "PearVisor",
            targets: ["PearVisor"]
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

        // Core VM management library
        .target(
            name: "PearVisorCore",
            dependencies: ["PearVisorGPU"],
            path: "Sources/PearVisorCore"
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
