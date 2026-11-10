#pragma once

#include <algorithm>
#include <string_view>

namespace Au {

    // ASCII-only helpers for identifiers and virtual paths. Not Unicode case folding.

    [[nodiscard]] constexpr char AsciiToLower(char character) noexcept {
        if (character >= 'A' && character <= 'Z') {
            return static_cast<char>(character - 'A' + 'a');
        }
        return character;
    }

    [[nodiscard]] constexpr bool AsciiEqualsIgnoreCase(std::string_view lhs, std::string_view rhs) noexcept {
        return std::ranges::equal(lhs, rhs, [](char left, char right) noexcept {
            return AsciiToLower(left) == AsciiToLower(right);
        });
    }

    [[nodiscard]] constexpr bool AsciiStartsWithIgnoreCase(std::string_view value, std::string_view prefix) noexcept {
        if (value.size() < prefix.size()) {
            return false;
        }
        return AsciiEqualsIgnoreCase(value.substr(0, prefix.size()), prefix);
    }

}  // namespace Au
