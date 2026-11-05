set(_au_rhi_backends "")
if(AU_RHI_VULKAN)
    list(APPEND _au_rhi_backends Au::RHIVulkan)
endif()
if(AU_RHI_D3D12)
    list(APPEND _au_rhi_backends Au::RHID3D12)
endif()

au_add_module(Renderer
    PUBLIC_DEPS     Au::RHI
    PRIVATE_DEPS    ${_au_rhi_backends}
    PRIVATE_DEFINES AU_RHI_VULKAN=$<BOOL:${AU_RHI_VULKAN}>
                    AU_RHI_D3D12=$<BOOL:${AU_RHI_D3D12}>
)
