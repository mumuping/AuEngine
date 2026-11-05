#pragma once

#include "RHIVulkan/RHIVulkanAPI.h"

#include "RHI/RHIDevice.h"

#include <memory>

namespace Au::RHI
{
    // Returns nullptr if no Vulkan instance or physical device can be created.
    AU_RHIVULKAN_API std::unique_ptr<IRHIDevice> CreateVulkanDevice();
} // namespace Au::RHI
