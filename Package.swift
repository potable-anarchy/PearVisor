// swift-tools-version: 5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "PearVisor",
    platforms: [
        .macOS(.v14)
    ],
    products: [
        .executable(
            name: "PearVisor",
            targets: ["PearVisor"]
        ),
        .library(
            name: "PearVisorCore",
            targets: ["PearVisorCore"]
        ),
    ],
    dependencies: [],
    targets: [
        .executableTarget(
            name: "PearVisor",
            dependencies: ["PearVisorCore"],
            path: "Sources/PearVisor",
            resources: [
                .process("Resources")
            ]
        ),
        
        .target(
            name: "PearVisorCore",
            dependencies: ["PearVisorGPU"],
            path: "Sources/PearVisorCore",
            linkerSettings: [
                .unsafeFlags(["-L", "GPU/build"]),
                .linkedLibrary("PearVisorGPU")
            ]
        ),
        
        .systemLibrary(
            name: "PearVisorGPU",
            path: "Sources/PearVisorGPU",
            pkgConfig: nil
        ),
        
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
