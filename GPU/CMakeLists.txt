# CMakeLists.txt for PearVisor GPU Subsystem

cmake_minimum_required(VERSION 3.25)
project(PearVisorGPU VERSION 0.1.0 LANGUAGES C CXX)

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

# virglrenderer paths
set(VIRGL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Submodules/virglrenderer/build-macos/install)
include_directories(${VIRGL_DIR}/include)
link_directories(${VIRGL_DIR}/lib)

# Source files
set(GPU_SOURCES
    src/pv_gpu.c
    src/pv_virgl.c
    src/pv_venus_ring.c
    src/pv_venus_protocol.c
    src/pv_venus_decoder.c
    src/pv_moltenvk.c
    src/pv_venus_handlers.c
    src/pv_venus_integration.c
)

# Create static library
add_library(PearVisorGPU STATIC ${GPU_SOURCES})

# Find Vulkan
find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

# Link virglrenderer and Vulkan
target_link_libraries(PearVisorGPU virglrenderer ${Vulkan_LIBRARIES})

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

# Test executables
add_executable(test_virgl src/test_virgl.c)
target_link_libraries(test_virgl PearVisorGPU)

add_executable(test_venus_ring src/test_venus_ring.c)
target_link_libraries(test_venus_ring PearVisorGPU)

add_executable(test_venus_decoder src/test_venus_decoder.c)
target_link_libraries(test_venus_decoder PearVisorGPU)

add_executable(test_moltenvk src/test_moltenvk.c)
target_link_libraries(test_moltenvk PearVisorGPU)

add_executable(test_venus_handlers src/test_venus_handlers.c)
target_link_libraries(test_venus_handlers PearVisorGPU)

add_executable(test_resource_handlers src/test_resource_handlers.c)
target_link_libraries(test_resource_handlers PearVisorGPU)

add_executable(test_command_buffers src/test_command_buffers.c)
target_link_libraries(test_command_buffers PearVisorGPU)

add_executable(test_venus_integration src/test_venus_integration.c)
target_link_libraries(test_venus_integration PearVisorGPU)

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
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
