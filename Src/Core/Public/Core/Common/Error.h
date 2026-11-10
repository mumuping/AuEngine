#pragma once

#include <fmt/format.h>
#include <source_location>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace Au {

    // Captures the call-site location when default-constructed as a function argument.
    // Usage: Error::Format({}, "failed: {}", reason);
    struct ErrorLocation {
        std::source_location Value = std::source_location::current();
    };

    // Rich error value for recoverable (Expected) and exceptional (Exception) paths.
    class Error {
      public:
        Error(std::string message, std::source_location location, std::error_code code = {})
            : m_Message(std::move(message)), m_Location(location), m_Code(code) {}

        [[nodiscard]] static Error
        Make(std::string_view message, const std::source_location& location = std::source_location::current()) {
            return {std::string(message), location};
        }

        template <typename... Args>
        [[nodiscard]] static Error Format(ErrorLocation location, fmt::format_string<Args...> format, Args&&... args) {
            return {fmt::format(format, std::forward<Args>(args)...), location.Value};
        }

        [[nodiscard]] static Error
        FromStdError(std::error_code code, const std::source_location& location = std::source_location::current()) {
            return {code.message(), location, code};
        }

        [[nodiscard]] const std::string& Message() const noexcept { return m_Message; }

        [[nodiscard]] const std::source_location& Location() const noexcept { return m_Location; }

        [[nodiscard]] std::error_code Code() const noexcept { return m_Code; }

        [[nodiscard]] bool HasCode() const noexcept { return static_cast<bool>(m_Code); }

      private:
        std::string          m_Message;
        std::source_location m_Location;
        std::error_code      m_Code;
    };

}  // namespace Au
