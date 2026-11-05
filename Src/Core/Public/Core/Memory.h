#pragma once

#include "Core/CoreAPI.h"

#include <cstddef>

namespace Au::Memory
{
    AU_CORE_API void* Allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t));
    AU_CORE_API void  Free(void* ptr);

    AU_CORE_API int GetAllocatorVersion();
} // namespace Au::Memory
