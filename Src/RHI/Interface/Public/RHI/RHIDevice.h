#pragma once

#include "RHI/RHIAPI.h"

#include "Core/Types.h"

#include <optional>
#include <string_view>

namespace Au::RHI
{
    enum class ERHIBackend : u8
    {
        Vulkan,
        D3D12,
    };

    AU_RHI_API std::string_view ToString(ERHIBackend backend);
    // Case-insensitive, accepts the names returned by ToString().
    AU_RHI_API std::optional<ERHIBackend> ParseBackend(std::string_view name);

    class AU_RHI_API IRHIDevice
    {
    public:
        virtual ~IRHIDevice();

        IRHIDevice(const IRHIDevice&)            = delete;
        IRHIDevice& operator=(const IRHIDevice&) = delete;
        IRHIDevice(IRHIDevice&&)                 = delete;
        IRHIDevice& operator=(IRHIDevice&&)      = delete;

        [[nodiscard]] virtual ERHIBackend      GetBackend() const     = 0;
        [[nodiscard]] virtual std::string_view GetAdapterName() const = 0;

    protected:
        IRHIDevice() = default;
    };
} // namespace Au::RHI
