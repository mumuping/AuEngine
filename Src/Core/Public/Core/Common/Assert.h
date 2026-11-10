#pragma once

#include "Core/Common/Macros.h"
#include "Core/CoreAPI.h"

#include <fmt/format.h>
#include <source_location>
#include <string_view>

namespace Au {

    // Invoked when AU_ASSERT / AU_ENSURE fails. Default logs (if available) then aborts.
    using FatalHandler = void (*)(const std::source_location& location, std::string_view message);

    // Returns the previous handler. Pass nullptr to restore the default handler.
    [[nodiscard]] AU_CORE_API FatalHandler SetFatalHandler(FatalHandler handler) noexcept;
    [[nodiscard]] AU_CORE_API FatalHandler GetFatalHandler() noexcept;

    namespace Detail {

        // Used by AU_ASSERT / AU_ENSURE. Do not call from engine code.
        AU_CORE_API void FailAssert(const std::source_location& location, std::string_view message);

    }  // namespace Detail

}  // namespace Au

// ---- Ensure: always active --------------------------------------------------------------------

#define AU_ENSURE(condition)                                                                        \
    do {                                                                                            \
        if (AU_UNLIKELY(!(condition))) { /* NOLINT(readability-simplify-boolean-expr) */            \
            ::Au::Detail::FailAssert(std::source_location::current(), "AU_ENSURE(" #condition ")"); \
        }                                                                                           \
    } while (0)

#define AU_ENSURE_MSG(condition, ...)                                                            \
    do {                                                                                         \
        if (AU_UNLIKELY(!(condition))) { /* NOLINT(readability-simplify-boolean-expr) */         \
            ::Au::Detail::FailAssert(std::source_location::current(), fmt::format(__VA_ARGS__)); \
        }                                                                                        \
    } while (0)

// ---- Assert: Debug only -----------------------------------------------------------------------

#if defined(AU_DEBUG) && AU_DEBUG

    #define AU_ASSERT(condition) AU_ENSURE(condition)
    #define AU_ASSERT_MSG(condition, ...) AU_ENSURE_MSG(condition, __VA_ARGS__)

#else

    #define AU_ASSERT(condition) ((void)0)
    #define AU_ASSERT_MSG(condition, ...) ((void)0)

#endif
