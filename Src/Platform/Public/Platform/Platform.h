#pragma once

#include "Platform/PlatformAPI.h"

#include <string_view>

#if AU_PLATFORM_WINDOWS
    #define AU_DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
    #define AU_DEBUG_BREAK() __builtin_trap()
#else
    #error "Unsupported platform"
#endif

namespace Au::Platform
{
    AU_PLATFORM_API std::string_view GetPlatformName();
} // namespace Au::Platform
