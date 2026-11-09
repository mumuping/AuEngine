# All third-party packages are located here, at the top-level scope, so that their imported
# targets are visible to every subdirectory. Packages come from the vcpkg manifest (vcpkg.json).
include_guard(GLOBAL)

# ---- Core -----------------------------------------------------------------------------------------
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

find_package(mimalloc CONFIG REQUIRED)
if(TARGET mimalloc-static)
    add_library(ThirdParty::mimalloc ALIAS mimalloc-static)
else()
    add_library(ThirdParty::mimalloc ALIAS mimalloc)
endif()

# ---- Vulkan ---------------------------------------------------------------------------------------
if(AU_RHI_VULKAN)
    # Headers and loader come from the Vulkan SDK (located through the VULKAN_SDK environment variable).
    find_package(Vulkan)
    if(NOT Vulkan_FOUND)
        message(FATAL_ERROR "AuEngine: Vulkan SDK not found. Install it from https://vulkan.lunarg.com and make "
                            "sure VULKAN_SDK is set, or configure with -DAU_RHI_VULKAN=OFF")
    endif()
    find_package(VulkanMemoryAllocator CONFIG REQUIRED)
endif()

# ---- D3D12 ----------------------------------------------------------------------------------------
if(AU_RHI_D3D12)
    find_package(directx-headers CONFIG REQUIRED)
    find_package(D3D12MemoryAllocator CONFIG REQUIRED)
endif()

# ---- Tests ----------------------------------------------------------------------------------------
if(AU_BUILD_TESTS)
    find_package(GTest CONFIG REQUIRED)
    include(GoogleTest)
endif()
