# CMakeLists.txt for PearVisor GPU Subsystem

cmake_minimum_required(VERSION 3.25)
project(PearVisorGPU VERSION 0.1.0 LANGUAGES C CXX OBJCXX)

# C/C++ Standards
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(GPU_SOURCES
    src/pv_gpu.c
    src/pv_virtio_gpu.c
    src/pv_metal_renderer.mm
)

# Create static library
add_library(PearVisorGPU STATIC ${GPU_SOURCES})

# macOS frameworks (Metal, IOKit, etc.)
if(APPLE)
    find_library(METAL_FRAMEWORK Metal)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore)
    
    target_link_libraries(PearVisorGPU
        ${METAL_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
    )
endif()

# Enable Objective-C++ for .mm files
set_source_files_properties(src/pv_metal_renderer.mm PROPERTIES COMPILE_FLAGS "-x objective-c++")

# Installation
install(TARGETS PearVisorGPU
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include/PearVisorGPU
    FILES_MATCHING PATTERN "*.h"
)

# Print configuration
message(STATUS "PearVisor GPU Subsystem Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  Metal Framework: ${METAL_FRAMEWORK}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
