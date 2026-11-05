include_guard(GLOBAL)

# ---- Platform -------------------------------------------------------------------------------------
set(AU_PLATFORM_WINDOWS OFF)
set(AU_PLATFORM_LINUX   OFF)

if(WIN32)
    set(AU_PLATFORM_WINDOWS ON)
    set(AU_PLATFORM_NAME "Windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(AU_PLATFORM_LINUX ON)
    set(AU_PLATFORM_NAME "Linux")
else()
    message(FATAL_ERROR "AuEngine: unsupported platform '${CMAKE_SYSTEM_NAME}'")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "AuEngine: only 64-bit targets are supported")
endif()

# ---- Compiler -------------------------------------------------------------------------------------
# AU_MSVC_FRONTEND covers both cl and clang-cl, which share the MSVC command-line syntax.
set(AU_COMPILER_MSVC     OFF)
set(AU_COMPILER_CLANG_CL OFF)
set(AU_COMPILER_CLANG    OFF)
set(AU_COMPILER_GCC      OFF)
set(AU_MSVC_FRONTEND     OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(AU_COMPILER_MSVC ON)
    set(AU_MSVC_FRONTEND ON)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(AU_COMPILER_CLANG_CL ON)
    set(AU_MSVC_FRONTEND ON)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(AU_COMPILER_CLANG ON)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(AU_COMPILER_GCC ON)
else()
    message(WARNING "AuEngine: untested compiler '${CMAKE_CXX_COMPILER_ID}'")
endif()

# ---- Option validation ----------------------------------------------------------------------------
if(AU_RHI_D3D12 AND NOT AU_PLATFORM_WINDOWS)
    message(WARNING "AuEngine: D3D12 is only available on Windows, disabling AU_RHI_D3D12")
    set(AU_RHI_D3D12 OFF CACHE BOOL "Build the D3D12 RHI backend" FORCE)
endif()

if(NOT AU_RHI_VULKAN AND NOT AU_RHI_D3D12)
    message(FATAL_ERROR "AuEngine: at least one RHI backend (AU_RHI_VULKAN / AU_RHI_D3D12) must be enabled")
endif()

message(STATUS "AuEngine: platform=${AU_PLATFORM_NAME} compiler=${CMAKE_CXX_COMPILER_ID} "
               "${CMAKE_CXX_COMPILER_VERSION} (frontend: ${CMAKE_CXX_COMPILER_FRONTEND_VARIANT})")
message(STATUS "AuEngine: shared=${AU_BUILD_SHARED} vulkan=${AU_RHI_VULKAN} d3d12=${AU_RHI_D3D12} "
               "tests=${AU_BUILD_TESTS} editor=${AU_BUILD_EDITOR}")
