#include "Platform/Platform.h"

namespace Au::Platform
{
    std::string_view GetPlatformName()
    {
#if AU_PLATFORM_WINDOWS
        return "Windows";
#elif AU_PLATFORM_LINUX
        return "Linux";
#endif
    }
} // namespace Au::Platform
