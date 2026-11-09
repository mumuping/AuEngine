#pragma once

#include "Core/Common/Macros.h"
#include "Core/Common/SingletonSystem.h"
#include "Core/Common/Types.h"
#include "Core/CoreAPI.h"

#include <filesystem>
#include <fmt/format.h>
#include <memory>
#include <source_location>

namespace Au {

    enum class ELogLevel : u8 {
        Trace    = 0,
        Debug    = 1,
        Info     = 2,
        Warn     = 3,
        Error    = 4,
        Critical = 5,
        Off      = 6,
    };

#if defined(AU_DEBUG) && AU_DEBUG
    inline constexpr ELogLevel g_CompileLogLevel = ELogLevel::Trace;
#elif defined(AU_DEVELOPMENT) && AU_DEVELOPMENT
    inline constexpr ELogLevel g_CompileLogLevel = ELogLevel::Debug;
#else
    inline constexpr ELogLevel g_CompileLogLevel = ELogLevel::Info;
#endif

    struct LogCategory {
        const char* Name = nullptr;
    };

    class AU_CORE_API Log {
      public:
        AU_DISABLE_COPY_MOVE(Log);

        explicit Log(const std::filesystem::path& logFile);
        ~Log();

        template <typename... Args>
        void Write(
            const LogCategory&          category,
            ELogLevel                   level,
            const std::source_location& location,
            fmt::format_string<Args...> format,
            const Args&... args
        ) {
            Writev(category, level, location, format, fmt::make_format_args(args...));
        }

        void                    SetLevel(ELogLevel level);
        [[nodiscard]] ELogLevel GetLevel() const noexcept;

        void                    SetCategoryLevel(const LogCategory& category, ELogLevel level);
        [[nodiscard]] ELogLevel GetCategoryLevel(const LogCategory& category);

      private:
        void Writev(
            const LogCategory&          category,
            ELogLevel                   level,
            const std::source_location& location,
            fmt::string_view            format,
            fmt::format_args            args
        );

        struct Impl;
        std::unique_ptr<Impl> m_Impl;
    };

    // Declared here; defined with AU_DEFINE_LOG_CATEGORY in Log.cpp.
    extern const LogCategory LogCore;  // NOLINT(readability-identifier-naming)

}  // namespace Au

// Place inside `namespace Au { ... }` (or qualify with Au::) in module headers/sources.
#define AU_DECLARE_LOG_CATEGORY_EXTERN(CategoryName) extern const ::Au::LogCategory CategoryName

#define AU_DEFINE_LOG_CATEGORY(CategoryName) \
    const ::Au::LogCategory CategoryName {   \
        #CategoryName                        \
    }

#define AU_REGISTER_LOG(logFile) AU_REGISTER_SINGLETON_IMMEDIATE(::Au::Log, logFile)

#define AU_LOG(Category, Level, ...)                                                                         \
    do {                                                                                                     \
        constexpr ::Au::ELogLevel auLogLevel = ::Au::ELogLevel::Level;                                       \
        if constexpr (static_cast<::Au::u8>(auLogLevel) >= static_cast<::Au::u8>(::Au::g_CompileLogLevel)) { \
            if (::Au::Log* const auLog = ::Au::SingletonSystem::Get().TryGet<::Au::Log>()) {                 \
                auLog->Write((Category), auLogLevel, std::source_location::current(), __VA_ARGS__);          \
            }                                                                                                \
        }                                                                                                    \
    } while (0)

#define AU_LOG_TRACE(...) AU_LOG(::Au::LogCore, Trace, __VA_ARGS__)
#define AU_LOG_DEBUG(...) AU_LOG(::Au::LogCore, Debug, __VA_ARGS__)
#define AU_LOG_INFO(...) AU_LOG(::Au::LogCore, Info, __VA_ARGS__)
#define AU_LOG_WARN(...) AU_LOG(::Au::LogCore, Warn, __VA_ARGS__)
#define AU_LOG_ERROR(...) AU_LOG(::Au::LogCore, Error, __VA_ARGS__)
#define AU_LOG_CRITICAL(...) AU_LOG(::Au::LogCore, Critical, __VA_ARGS__)
