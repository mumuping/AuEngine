#include "Core/Log.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <exception>
#include <memory>

#if AU_PLATFORM_WINDOWS
    #include <Windows.h>
#endif

namespace Au::Log
{
    namespace
    {
        std::shared_ptr<spdlog::logger> CreateLogger()
        {
#if AU_PLATFORM_WINDOWS
            SetConsoleOutputCP(CP_UTF8);
#endif
            auto logger = std::make_shared<spdlog::logger>("Au", std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
            logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
#if AU_DEBUG
            logger->set_level(spdlog::level::debug);
#else
            logger->set_level(spdlog::level::info);
#endif
            logger->flush_on(spdlog::level::warn);
            return logger;
        }

        std::shared_ptr<spdlog::logger>& GetLoggerPtr()
        {
            static std::shared_ptr<spdlog::logger> s_Logger = CreateLogger();
            return s_Logger;
        }
    } // namespace

    void Initialize(const std::filesystem::path& logFile)
    {
        spdlog::logger& logger = GetLogger();
        try
        {
            std::filesystem::create_directories(logFile.parent_path());
            auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile.string(), true);
            fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            logger.sinks().push_back(std::move(fileSink));
        }
        catch (const std::exception& e)
        {
            logger.warn("Failed to open log file: {}", e.what());
        }
    }

    void Shutdown()
    {
        GetLogger().flush();
    }

    spdlog::logger& GetLogger()
    {
        return *GetLoggerPtr();
    }
} // namespace Au::Log
