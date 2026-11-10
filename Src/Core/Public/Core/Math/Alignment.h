#pragma once

#include "Core/Common/Types.h"

namespace Au::Math {

    [[nodiscard]] constexpr bool IsPowerOfTwo(usize value) noexcept {
        return value != 0 && (value & (value - 1U)) == 0;
    }

    // alignment must be a power of two.
    [[nodiscard]] constexpr usize AlignUp(usize value, usize alignment) noexcept {
        return (value + alignment - 1U) & ~(alignment - 1U);
    }

    // alignment must be a power of two.
    [[nodiscard]] constexpr usize AlignDown(usize value, usize alignment) noexcept {
        return value & ~(alignment - 1U);
    }

}  // namespace Au::Math
