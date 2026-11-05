#include "Renderer/Renderer.h"

#include "Core/Log.h"

#if AU_RHI_VULKAN
    #include "RHIVulkan/VulkanRHI.h"
#endif
#if AU_RHI_D3D12
    #include "RHID3D12/D3D12RHI.h"
#endif

namespace Au
{
    namespace
    {
        constexpr bool HasVulkan = AU_RHI_VULKAN != 0;
        constexpr bool HasD3D12  = AU_RHI_D3D12 != 0;
    } // namespace

    bool IsRHIBackendAvailable(RHI::ERHIBackend backend)
    {
        switch (backend)
        {
        case RHI::ERHIBackend::Vulkan:
            return HasVulkan;
        case RHI::ERHIBackend::D3D12:
            return HasD3D12;
        }
        return false;
    }

    RHI::ERHIBackend GetDefaultRHIBackend()
    {
#if AU_RHI_D3D12
        return RHI::ERHIBackend::D3D12;
#else
        return RHI::ERHIBackend::Vulkan;
#endif
    }

    std::unique_ptr<RHI::IRHIDevice> CreateRHIDevice(RHI::ERHIBackend backend)
    {
        switch (backend)
        {
        case RHI::ERHIBackend::Vulkan:
#if AU_RHI_VULKAN
            return RHI::CreateVulkanDevice();
#else
            break;
#endif
        case RHI::ERHIBackend::D3D12:
#if AU_RHI_D3D12
            return RHI::CreateD3D12Device();
#else
            break;
#endif
        }
        AU_LOG_ERROR("RHI backend {} is not compiled into this build", RHI::ToString(backend));
        return nullptr;
    }
} // namespace Au
