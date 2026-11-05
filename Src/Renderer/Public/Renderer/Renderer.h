#pragma once

#include "Renderer/RendererAPI.h"

#include "RHI/RHIDevice.h"

#include <memory>

namespace Au
{
    // Backends compiled into this build are selected by AU_RHI_VULKAN / AU_RHI_D3D12.
    AU_RENDERER_API bool                        IsRHIBackendAvailable(RHI::ERHIBackend backend);
    AU_RENDERER_API RHI::ERHIBackend            GetDefaultRHIBackend();
    AU_RENDERER_API std::unique_ptr<RHI::IRHIDevice> CreateRHIDevice(RHI::ERHIBackend backend);

    class AU_RENDERER_API Renderer
    {
    public:
        Renderer();
        ~Renderer();

        Renderer(const Renderer&)            = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&)                 = delete;
        Renderer& operator=(Renderer&&)      = delete;

        bool Initialize(RHI::ERHIBackend backend);
        void Shutdown();

        [[nodiscard]] RHI::IRHIDevice* GetDevice() const;

    private:
        std::unique_ptr<RHI::IRHIDevice> m_Device;
    };
} // namespace Au
