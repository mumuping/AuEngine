#pragma once

#include "Core/Common/Types.h"

namespace Au::Memory {

    // Coarse allocation categories for Debug/Development accounting.
    enum class EMemoryTag : u8 {
        General = 0,
        FileSystem,
        Rendering,
        Audio,
        Physics,
        Script,
        Temporary,
        Count,
    };

}  // namespace Au::Memory
