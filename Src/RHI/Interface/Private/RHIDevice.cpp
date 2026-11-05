#include "RHI/RHIDevice.h"

#include <algorithm>
#include <cctype>

namespace Au::RHI
{
    namespace
    {
        bool EqualsIgnoreCase(std::string_view lhs, std::string_view rhs)
        {
            return std::ranges::equal(lhs, rhs, [](char a, char b) {
                return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
            });
        }
    } // namespace

    IRHIDevice::~IRHIDevice() = default;

    std::string_view ToString(ERHIBackend backend)
    {
        switch (backend)
        {
        case ERHIBackend::Vulkan:
            return "Vulkan";
        case ERHIBackend::D3D12:
            return "D3D12";
        }
        return "Unknown";
    }

    std::optional<ERHIBackend> ParseBackend(std::string_view name)
    {
        for (const ERHIBackend backend : {ERHIBackend::Vulkan, ERHIBackend::D3D12})
        {
            if (EqualsIgnoreCase(name, ToString(backend)))
            {
                return backend;
            }
        }
        return std::nullopt;
    }
} // namespace Au::RHI
