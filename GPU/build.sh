#!/bin/bash
# Simple build script for GPU subsystem

set -e

echo "Building PearVisor GPU Subsystem..."
echo ""

mkdir -p build

# Compile C files
echo "Compiling virtio-gpu device..."
clang -c -o build/pv_virtio_gpu.o src/pv_virtio_gpu.c -Iinclude -Wall -O2

# Compile Objective-C++ files
echo "Compiling Metal renderer..."
clang++ -c -o build/pv_metal_renderer.o src/pv_metal_renderer.mm -Iinclude -Wall -O2 \
    -framework Metal -framework Foundation -framework QuartzCore -std=c++17

# Create static library
echo "Creating static library..."
ar rcs build/libPearVisorGPU.a build/pv_virtio_gpu.o build/pv_metal_renderer.o

echo ""
echo "✓ Build complete!"
echo "  Library: build/libPearVisorGPU.a"
ls -lh build/libPearVisorGPU.a
