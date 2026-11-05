#pragma once

#include "Core/CoreAPI.h"

#include <spdlog/spdlog.h>

#include <filesystem>

namespace Au::Log
{
    // Adds a file sink to the engine logger. The console sink is always present, so logging works
    // even before this is called.
    AU_CORE_API void Initialize(const std::filesystem::path& logFile);
    AU_CORE_API void Shutdown();
    AU_CORE_API spdlog::logger& GetLogger();
} // namespace Au::Log

#define AU_LOG_TRACE(...)    ::Au::Log::GetLogger().trace(__VA_ARGS__)
#define AU_LOG_DEBUG(...)    ::Au::Log::GetLogger().debug(__VA_ARGS__)
#define AU_LOG_INFO(...)     ::Au::Log::GetLogger().info(__VA_ARGS__)
#define AU_LOG_WARN(...)     ::Au::Log::GetLogger().warn(__VA_ARGS__)
#define AU_LOG_ERROR(...)    ::Au::Log::GetLogger().error(__VA_ARGS__)
#define AU_LOG_CRITICAL(...) ::Au::Log::GetLogger().critical(__VA_ARGS__)
