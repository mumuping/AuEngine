au_add_module(RHIVulkan
    PUBLIC_DEPS     Au::RHI
    PRIVATE_DEPS    Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator
    PRIVATE_DEFINES $<$<PLATFORM_ID:Windows>:VK_USE_PLATFORM_WIN32_KHR>
    FOLDER          Engine/RHI
)
