#pragma once

#include "Core/Common/Expected.h"
#include "Core/CoreAPI.h"

#include <string>
#include <string_view>

namespace Au::Fs {

    // Absolute UTF-8 virtual path using '/' separators only (e.g. "/game/Config/Engine.ini").
    // Construct trusted values directly; parse untrusted input with Normalize().
    class AU_CORE_API Path {
      public:
        Path() = default;

        explicit Path(std::string alreadyNormalizedUtf8);

        [[nodiscard]] static Result<Path> Normalize(std::string_view utf8);

        [[nodiscard]] std::string_view View() const noexcept { return m_Value; }

        [[nodiscard]] const std::string& String() const noexcept { return m_Value; }

        [[nodiscard]] bool Empty() const noexcept { return m_Value.empty(); }

        [[nodiscard]] bool IsAbsolute() const noexcept;

        [[nodiscard]] bool IsRoot() const noexcept { return m_Value == "/"; }

        [[nodiscard]] Path Parent() const;

        [[nodiscard]] Path Filename() const;

        [[nodiscard]] Result<Path> Join(std::string_view child) const;

        [[nodiscard]] bool EqualsIgnoreCase(const Path& other) const noexcept;

        // True when this path is a prefix of other as a mount point ("/game" matches "/game" and "/game/...").
        [[nodiscard]] bool IsMountPrefixOf(const Path& other) const noexcept;

        // other with this mount prefix removed, always absolute for the backend ("/game/a" -> "/a").
        [[nodiscard]] Result<Path> StripMountPrefix(const Path& other) const;

        [[nodiscard]] friend bool operator==(const Path& lhs, const Path& rhs) noexcept {
            return lhs.m_Value == rhs.m_Value;
        }

        [[nodiscard]] friend bool operator!=(const Path& lhs, const Path& rhs) noexcept { return !(lhs == rhs); }

        [[nodiscard]] friend bool operator<(const Path& lhs, const Path& rhs) noexcept {
            return lhs.m_Value < rhs.m_Value;
        }

      private:
        std::string m_Value;
    };

}  // namespace Au::Fs
