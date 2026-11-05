add_subdirectory(Interface)

if(AU_RHI_VULKAN)
    add_subdirectory(Vulkan)
endif()

if(AU_RHI_D3D12)
    add_subdirectory(D3D12)
endif()
