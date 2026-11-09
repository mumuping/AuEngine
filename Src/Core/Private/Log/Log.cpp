#include "Core/Log/Log.h"

#include <cassert>
#include <mutex>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace Au {
    namespace {
        constexpr std::size_t g_AsyncQueueSize      = 8192;
        constexpr std::size_t g_AsyncThreadCount    = 1;
        constexpr std::size_t g_RotatingMaxFileSize = 5ULL * 1024ULL * 1024ULL;
        constexpr std::size_t g_RotatingMaxFiles    = 3;

        spdlog::level::level_enum GetDefaultLevel() noexcept {
#if defined(AU_DEBUG) && AU_DEBUG
            return spdlog::level::trace;
#elif defined(AU_DEVELOPMENT) && AU_DEVELOPMENT
            return spdlog::level::debug;
#else
            return spdlog::level::info;
#endif
        }

        [[nodiscard]] spdlog::level::level_enum ToSpdLevel(ELogLevel level) noexcept {
            switch (level) {
                case ELogLevel::Trace:
                    return spdlog::level::trace;
                case ELogLevel::Debug:
                    return spdlog::level::debug;
                case ELogLevel::Info:
                    return spdlog::level::info;
                case ELogLevel::Warn:
                    return spdlog::level::warn;
                case ELogLevel::Error:
                    return spdlog::level::err;
                case ELogLevel::Critical:
                    return spdlog::level::critical;
                case ELogLevel::Off:
                    return spdlog::level::off;
            }
            return spdlog::level::info;
        }

        [[nodiscard]] ELogLevel FromSpdLevel(spdlog::level::level_enum level) noexcept {
            switch (level) {
                case spdlog::level::trace:
                    return ELogLevel::Trace;
                case spdlog::level::debug:
                    return ELogLevel::Debug;
                case spdlog::level::info:
                    return ELogLevel::Info;
                case spdlog::level::warn:
                    return ELogLevel::Warn;
                case spdlog::level::err:
                    return ELogLevel::Error;
                case spdlog::level::critical:
                    return ELogLevel::Critical;
                case spdlog::level::off:
                    return ELogLevel::Off;
                case spdlog::level::n_levels:
                    break;
            }
            return ELogLevel::Info;
        }
    }  // namespace

    struct Log::Impl {
        std::vector<spdlog::sink_ptr>                                    Sinks;
        std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> Loggers;
        mutable std::mutex                                               Mutex;
        spdlog::level::level_enum                                        Level = spdlog::level::info;

        [[nodiscard]] std::shared_ptr<spdlog::logger> CreateCategoryLogger(const char* name) {
            auto logger = std::make_shared<spdlog::async_logger>(
                name, Sinks.begin(), Sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block
            );
            logger->set_level(Level);
            logger->flush_on(spdlog::level::err);
            spdlog::register_logger(logger);
            return logger;
        }

        [[nodiscard]] spdlog::logger& GetOrCreateCategoryLogger(const LogCategory& category) {
            assert(category.Name != nullptr && category.Name[0] != '\0');

            const std::scoped_lock lock(Mutex);
            if (const auto it = Loggers.find(category.Name); it != Loggers.end()) {
                return *it->second;
            }

            std::shared_ptr<spdlog::logger> logger = CreateCategoryLogger(category.Name);
            spdlog::logger&                 ref    = *logger;
            Loggers.emplace(category.Name, std::move(logger));
            return ref;
        }
    };

    AU_DEFINE_LOG_CATEGORY(LogCore);

    Log::Log(const std::filesystem::path& logFile) : m_Impl(std::make_unique<Impl>()) {
        if (const std::filesystem::path parent = logFile.parent_path(); !parent.empty()) {
            std::error_code error;
            std::filesystem::create_directories(parent, error);
        }

        spdlog::init_thread_pool(g_AsyncQueueSize, g_AsyncThreadCount);

        const auto          consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        const std::u8string logFileU8   = logFile.u8string();
        const auto          fileSink    = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            std::string(logFileU8.begin(), logFileU8.end()), g_RotatingMaxFileSize, g_RotatingMaxFiles
        );

        // Console: colorized, short. File: ISO timestamp + source location for post-mortem.
        consoleSink->set_pattern("%^[%T] [%l] [%n] %v%$");
        fileSink->set_pattern("[%Y-%m-%d %T.%e] [%l] [%n] [%s:%#] %v");

        m_Impl->Sinks.push_back(consoleSink);
        m_Impl->Sinks.push_back(fileSink);

        m_Impl->Level = GetDefaultLevel();
        (void)m_Impl->GetOrCreateCategoryLogger(LogCore);
    }

    Log::~Log() {
        {
            const std::scoped_lock lock(m_Impl->Mutex);
            for (auto& [name, logger] : m_Impl->Loggers) {
                if (logger) {
                    logger->flush();
                    spdlog::drop(name);
                }
            }
            m_Impl->Loggers.clear();
            m_Impl->Sinks.clear();
        }
        spdlog::shutdown();
    }

    void Log::Writev(
        const LogCategory&          category,
        ELogLevel                   level,
        const std::source_location& location,
        fmt::string_view            format,
        fmt::format_args            args
    ) {
        fmt::memory_buffer buffer;
        fmt::vformat_to(fmt::appender(buffer), format, args);

        spdlog::logger& logger = m_Impl->GetOrCreateCategoryLogger(category);
        logger.log(
            spdlog::source_loc{
                location.file_name(),
                static_cast<int>(location.line()),
                location.function_name(),
            },
            ToSpdLevel(level),
            spdlog::string_view_t{buffer.data(), buffer.size()}
        );
    }

    void Log::SetLevel(ELogLevel level) {
        const spdlog::level::level_enum spdLevel = ToSpdLevel(level);
        const std::scoped_lock          lock(m_Impl->Mutex);
        m_Impl->Level = spdLevel;
        for (auto& [_, logger] : m_Impl->Loggers) {
            if (logger) {
                logger->set_level(spdLevel);
            }
        }
    }

    ELogLevel Log::GetLevel() const noexcept {
        const std::scoped_lock lock(m_Impl->Mutex);
        return FromSpdLevel(m_Impl->Level);
    }

    void Log::SetCategoryLevel(const LogCategory& category, ELogLevel level) {
        spdlog::logger& logger = m_Impl->GetOrCreateCategoryLogger(category);
        logger.set_level(ToSpdLevel(level));
    }

    ELogLevel Log::GetCategoryLevel(const LogCategory& category) {
        const spdlog::logger& logger = m_Impl->GetOrCreateCategoryLogger(category);
        return FromSpdLevel(logger.level());
    }
}  // namespace Au
