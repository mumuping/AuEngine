#pragma once

#include "RHID3D12/RHID3D12API.h"

#include "RHI/RHIDevice.h"

#include <memory>

namespace Au::RHI
{
    // Returns nullptr if no D3D12-capable hardware adapter is available.
    AU_RHID3D12_API std::unique_ptr<IRHIDevice> CreateD3D12Device();
} // namespace Au::RHI
