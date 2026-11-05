cmake_minimum_required(VERSION 3.28)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(AuOptions)

if(AU_BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()
if(AU_RHI_VULKAN)
    list(APPEND VCPKG_MANIFEST_FEATURES "vulkan")
endif()
if(AU_RHI_D3D12)
    list(APPEND VCPKG_MANIFEST_FEATURES "d3d12")
endif()

project(AuEngine
    VERSION 0.1.0
    DESCRIPTION "AuEngine 3D rendering engine"
    LANGUAGES C CXX
)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "AuEngine: in-source builds are not allowed, use a preset (cmake --preset msvc)")
endif()

# ---- Global settings ------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS ${AU_BUILD_SHARED})

set(AU_RUNTIME_OUTPUT_DIR "${PROJECT_BINARY_DIR}/Bin/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${AU_RUNTIME_OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${AU_RUNTIME_OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/Lib/$<CONFIG>")
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

include(AuPlatform)
include(AuSanitizers)
include(AuCompilerSettings)
include(AuDependencies)
include(AuContent)
include(AuTargets)

# ---- Targets --------------------------------------------------------------------------------------
add_subdirectory(Src)

if(AU_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Test)
endif()
