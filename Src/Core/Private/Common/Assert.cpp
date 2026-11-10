#include "Core/Common/Assert.h"

#include "Core/Common/SingletonSystem.h"
#include "Core/Log/Log.h"

#include <cstdio>
#include <cstdlib>

namespace Au {
    namespace {

        [[noreturn]] void DefaultFatalHandler(const std::source_location& location, std::string_view message) {
            if (Log* const log = SingletonSystem::Get().TryGet<Log>()) {
                log->Write(LogCore, ELogLevel::Critical, location, "ASSERT FAILED: {}", message);
            } else {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
                std::fprintf(
                    stderr,
                    "ASSERT FAILED: %.*s (%s:%u %s)\n",
                    static_cast<int>(message.size()),
                    message.data(),
                    location.file_name(),
                    location.line(),
                    location.function_name()
                );
                std::fflush(stderr);
            }

            std::abort();
        }

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        FatalHandler g_FatalHandler = &DefaultFatalHandler;

    }  // namespace

    FatalHandler SetFatalHandler(FatalHandler handler) noexcept {
        FatalHandler previous = g_FatalHandler;
        g_FatalHandler        = handler != nullptr ? handler : &DefaultFatalHandler;
        return previous;
    }

    FatalHandler GetFatalHandler() noexcept {
        return g_FatalHandler;
    }

    namespace Detail {

        void FailAssert(const std::source_location& location, std::string_view message) {
            // Default handler aborts. Tests may install a recording handler that returns.
            g_FatalHandler(location, message);
        }

    }  // namespace Detail

}  // namespace Au
