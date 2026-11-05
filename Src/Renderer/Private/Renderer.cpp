#include "Renderer/Renderer.h"

#include "Core/Log.h"

namespace Au
{
    Renderer::Renderer() = default;

    Renderer::~Renderer()
    {
        Shutdown();
    }

    bool Renderer::Initialize(RHI::ERHIBackend backend)
    {
        m_Device = CreateRHIDevice(backend);
        if (!m_Device)
        {
            AU_LOG_ERROR("Renderer: failed to create {} device", RHI::ToString(backend));
            return false;
        }
        AU_LOG_INFO("Renderer: {} device on '{}'", RHI::ToString(m_Device->GetBackend()), m_Device->GetAdapterName());
        return true;
    }

    void Renderer::Shutdown()
    {
        m_Device.reset();
    }

    RHI::IRHIDevice* Renderer::GetDevice() const
    {
        return m_Device.get();
    }
} // namespace Au
