// swift-tools-version: 5.9

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
    ],
    targets: [
        .executableTarget(
            name: "PearVisor",
            dependencies: ["PearVisorCore"],
            path: "Sources/PearVisor",
            swiftSettings: [
                .unsafeFlags(["-I", "GPU/include"])
            ],
            linkerSettings: [
                .unsafeFlags(["-L", "GPU/build"]),
                .linkedLibrary("PearVisorGPU"),
                .linkedFramework("Metal"),
                .linkedFramework("Foundation"),
                .linkedFramework("QuartzCore")
            ]
        ),
        
        .target(
            name: "PearVisorCore",
            dependencies: [],
            path: "Sources/PearVisorCore",
            swiftSettings: [
                .unsafeFlags(["-I", "GPU/include"])
            ],
            linkerSettings: [
                .unsafeFlags(["-L", "GPU/build"]),
                .linkedLibrary("PearVisorGPU"),
                .linkedFramework("Metal"),
                .linkedFramework("Foundation"),
                .linkedFramework("QuartzCore")
            ]
        ),
    ]
)
