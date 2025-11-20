//
//  GPUBenchmark.swift
//  PearVisor
//
//  Comprehensive GPU performance benchmarking
//

import Foundation
import Metal

// MARK: - C API Imports

@_silgen_name("pv_gpu_init")
func pv_gpu_init() -> Int32

@_silgen_name("pv_gpu_create_device")
func pv_gpu_create_device(_ vmID: UnsafePointer<UInt8>, _ device: UnsafeMutablePointer<OpaquePointer?>) -> Int32

@_silgen_name("pv_gpu_destroy_device")
func pv_gpu_destroy_device(_ device: OpaquePointer?)

@_silgen_name("pv_gpu_init_metal_renderer")
func pv_gpu_init_metal_renderer(_ renderer: UnsafeMutablePointer<UnsafeMutableRawPointer?>, _ width: UInt32, _ height: UInt32) -> Int32

@_silgen_name("pv_gpu_destroy_metal_renderer")
func pv_gpu_destroy_metal_renderer(_ renderer: UnsafeMutableRawPointer?)

@_silgen_name("pv_gpu_metal_clear")
func pv_gpu_metal_clear(_ renderer: UnsafeMutableRawPointer?, _ r: Float, _ g: Float, _ b: Float, _ a: Float) -> Int32

@_silgen_name("pv_gpu_metal_present")
func pv_gpu_metal_present(_ renderer: UnsafeMutableRawPointer?) -> Int32

// MARK: - Benchmark Results

struct BenchmarkResults {
    let testName: String
    let iterations: Int
    let totalTimeMs: Double
    let avgTimeUs: Double
    let minTimeUs: Double
    let maxTimeUs: Double
    let fps: Double
    
    func print() {
        Swift.print("\n[\(testName)]")
        Swift.print("  Iterations: \(iterations)")
        Swift.print("  Total Time: \(String(format: "%.2f", totalTimeMs)) ms")
        Swift.print("  Average: \(String(format: "%.2f", avgTimeUs)) μs")
        Swift.print("  Min: \(String(format: "%.2f", minTimeUs)) μs")
        Swift.print("  Max: \(String(format: "%.2f", maxTimeUs)) μs")
        Swift.print("  FPS: \(String(format: "%.1f", fps))")
    }
}

// MARK: - Benchmarking Functions

func measureTime(_ block: () -> Void) -> Double {
    let start = CFAbsoluteTimeGetCurrent()
    block()
    let end = CFAbsoluteTimeGetCurrent()
    return (end - start) * 1_000_000 // Convert to microseconds
}

func runBenchmark(name: String, iterations: Int, warmup: Int = 10, block: () -> Void) -> BenchmarkResults {
    // Warmup
    for _ in 0..<warmup {
        block()
    }
    
    var times: [Double] = []
    times.reserveCapacity(iterations)
    
    // Actual benchmark
    for _ in 0..<iterations {
        let time = measureTime(block)
        times.append(time)
    }
    
    let totalTimeUs = times.reduce(0, +)
    let avgTimeUs = totalTimeUs / Double(iterations)
    let minTimeUs = times.min() ?? 0
    let maxTimeUs = times.max() ?? 0
    let fps = 1_000_000 / avgTimeUs // FPS from microseconds
    
    return BenchmarkResults(
        testName: name,
        iterations: iterations,
        totalTimeMs: totalTimeUs / 1000.0,
        avgTimeUs: avgTimeUs,
        minTimeUs: minTimeUs,
        maxTimeUs: maxTimeUs,
        fps: fps
    )
}

// MARK: - Native Metal Benchmark

func benchmarkNativeMetal(iterations: Int) -> BenchmarkResults {
    guard let device = MTLCreateSystemDefaultDevice() else {
        fatalError("Metal device not available")
    }
    
    guard let commandQueue = device.makeCommandQueue() else {
        fatalError("Failed to create command queue")
    }
    
    let textureDescriptor = MTLTextureDescriptor.texture2DDescriptor(
        pixelFormat: .bgra8Unorm,
        width: 1920,
        height: 1080,
        mipmapped: false
    )
    textureDescriptor.usage = [.renderTarget, .shaderRead]
    
    guard let texture = device.makeTexture(descriptor: textureDescriptor) else {
        fatalError("Failed to create texture")
    }
    
    return runBenchmark(name: "Native Metal Clear", iterations: iterations) {
        guard let commandBuffer = commandQueue.makeCommandBuffer() else { return }
        
        let renderPassDescriptor = MTLRenderPassDescriptor()
        renderPassDescriptor.colorAttachments[0].texture = texture
        renderPassDescriptor.colorAttachments[0].loadAction = .clear
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColor(red: 1.0, green: 0.0, blue: 0.0, alpha: 1.0)
        renderPassDescriptor.colorAttachments[0].storeAction = .store
        
        guard let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor) else { return }
        renderEncoder.endEncoding()
        
        commandBuffer.commit()
        commandBuffer.waitUntilCompleted()
    }
}

// MARK: - PearVisor GPU Benchmark

func benchmarkPearVisorGPU(iterations: Int) -> BenchmarkResults {
    var renderer: UnsafeMutableRawPointer?
    let result = pv_gpu_init_metal_renderer(&renderer, 1920, 1080)
    guard result == 0, let metalRenderer = renderer else {
        fatalError("Failed to initialize Metal renderer")
    }
    
    let benchmarkResult = runBenchmark(name: "PearVisor GPU Clear", iterations: iterations) {
        pv_gpu_metal_clear(metalRenderer, 1.0, 0.0, 0.0, 1.0)
        pv_gpu_metal_present(metalRenderer)
    }
    
    pv_gpu_destroy_metal_renderer(metalRenderer)
    return benchmarkResult
}

// MARK: - Device Creation Benchmark

func benchmarkDeviceCreation(iterations: Int) -> BenchmarkResults {
    let vmID = UUID()
    
    return runBenchmark(name: "GPU Device Creation", iterations: iterations, warmup: 2) {
        var devicePtr: OpaquePointer?
        withUnsafeBytes(of: vmID.uuid) { uuidBytes in
            _ = pv_gpu_create_device(
                uuidBytes.baseAddress!.assumingMemoryBound(to: UInt8.self),
                &devicePtr
            )
        }
        if let device = devicePtr {
            pv_gpu_destroy_device(device)
        }
    }
}

// MARK: - Main Benchmark

@main
struct GPUBenchmark {
    static func main() {
        print(String(repeating: "=", count: 70))
        print("PearVisor GPU Performance Benchmark")
        print(String(repeating: "=", count: 70))
        
        // System info
        guard let device = MTLCreateSystemDefaultDevice() else {
            fatalError("Metal not available")
        }
        
        print("\nSystem Information:")
        print("  GPU: \(device.name)")
        print("  Memory: \(String(format: "%.1f", Double(device.recommendedMaxWorkingSetSize) / 1_073_741_824)) GB")
        print("  Low Power: \(device.isLowPower)")
        print("  Ray Tracing: \(device.supportsRaytracing)")
        
        // Initialize GPU subsystem
        print("\nInitializing GPU subsystem...")
        let initResult = pv_gpu_init()
        guard initResult == 0 else {
            fatalError("Failed to initialize GPU subsystem")
        }
        print("✓ GPU subsystem initialized")
        
        print("\n" + String(repeating: "=", count: 70))
        print("BENCHMARKS")
        print(String(repeating: "=", count: 70))
        
        // Benchmark 1: Device creation overhead
        print("\n[1/4] Benchmarking device creation overhead...")
        let deviceResults = benchmarkDeviceCreation(iterations: 100)
        deviceResults.print()
        
        // Benchmark 2: Native Metal performance (baseline)
        print("\n[2/4] Benchmarking native Metal performance (baseline)...")
        let nativeResults = benchmarkNativeMetal(iterations: 1000)
        nativeResults.print()
        
        // Benchmark 3: PearVisor GPU performance
        print("\n[3/4] Benchmarking PearVisor GPU performance...")
        let pearVisorResults = benchmarkPearVisorGPU(iterations: 1000)
        pearVisorResults.print()
        
        // Benchmark 4: Batch rendering test
        print("\n[4/4] Benchmarking batch rendering (100 frames)...")
        let batchStart = CFAbsoluteTimeGetCurrent()
        
        var renderer: UnsafeMutableRawPointer?
        _ = pv_gpu_init_metal_renderer(&renderer, 1920, 1080)
        
        let colors: [(Float, Float, Float)] = [
            (1.0, 0.0, 0.0),  // Red
            (0.0, 1.0, 0.0),  // Green
            (0.0, 0.0, 1.0),  // Blue
            (1.0, 1.0, 0.0),  // Yellow
            (1.0, 0.0, 1.0),  // Magenta
            (0.0, 1.0, 1.0),  // Cyan
        ]
        
        for i in 0..<100 {
            let color = colors[i % colors.count]
            pv_gpu_metal_clear(renderer, color.0, color.1, color.2, 1.0)
            pv_gpu_metal_present(renderer)
        }
        
        let batchEnd = CFAbsoluteTimeGetCurrent()
        let batchTimeMs = (batchEnd - batchStart) * 1000
        let batchFps = 100.0 / (batchEnd - batchStart)
        
        print("\n[Batch Rendering - 100 frames]")
        print("  Total Time: \(String(format: "%.2f", batchTimeMs)) ms")
        print("  Average FPS: \(String(format: "%.1f", batchFps))")
        
        pv_gpu_destroy_metal_renderer(renderer)
        
        // Performance comparison
        print("\n" + String(repeating: "=", count: 70))
        print("PERFORMANCE ANALYSIS")
        print(String(repeating: "=", count: 70))
        
        let overhead = pearVisorResults.avgTimeUs - nativeResults.avgTimeUs
        let overheadPercent = (overhead / nativeResults.avgTimeUs) * 100
        let efficiency = (nativeResults.avgTimeUs / pearVisorResults.avgTimeUs) * 100
        
        print("\nCommand Latency Comparison:")
        print("  Native Metal:    \(String(format: "%8.2f", nativeResults.avgTimeUs)) μs")
        print("  PearVisor GPU:   \(String(format: "%8.2f", pearVisorResults.avgTimeUs)) μs")
        print("  Overhead:        \(String(format: "%8.2f", overhead)) μs (+\(String(format: "%.1f", overheadPercent))%)")
        print("  Efficiency:      \(String(format: "%.1f", efficiency))% of native")
        
        print("\nFramerate Comparison:")
        print("  Native Metal:    \(String(format: "%8.1f", nativeResults.fps)) FPS")
        print("  PearVisor GPU:   \(String(format: "%8.1f", pearVisorResults.fps)) FPS")
        
        print("\nDevice Creation:")
        print("  Time per device: \(String(format: "%.2f", deviceResults.avgTimeUs)) μs")
        print("  Devices/second:  \(String(format: "%.0f", 1_000_000 / deviceResults.avgTimeUs))")
        
        // Target assessment
        print("\n" + String(repeating: "=", count: 70))
        print("TARGET ASSESSMENT (Phase 1)")
        print(String(repeating: "=", count: 70))
        
        print("\nPhase 1 Goal: Prove concept with direct Metal rendering")
        print("  Current efficiency: \(String(format: "%.1f", efficiency))%")
        
        if efficiency >= 95 {
            print("  Status: ✓ EXCELLENT - Exceeds Phase 2 target!")
        } else if efficiency >= 80 {
            print("  Status: ✓ GREAT - Meets Phase 2 target early!")
        } else if efficiency >= 50 {
            print("  Status: ✓ GOOD - Solid foundation for Phase 2")
        } else {
            print("  Status: ⚠ NEEDS OPTIMIZATION")
        }
        
        print("\nPhase 2 Target: 80% native performance with Venus protocol")
        print("  Current vs target: \(String(format: "%.1f", efficiency - 80))% \(efficiency >= 80 ? "ahead" : "behind")")
        
        print("\n" + String(repeating: "=", count: 70))
        print("Benchmark complete!")
        print(String(repeating: "=", count: 70))
    }
}
