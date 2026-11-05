#include "Application/Application.h"

#include "ConfigFile.h"

#include "Core/Log.h"
#include "Core/Paths.h"
#include "Platform/Platform.h"

#include <optional>
#include <string_view>
#include <utility>

namespace Au
{
    namespace
    {
        constexpr std::string_view GetBuildConfigName()
        {
#if AU_DEBUG
            return "Debug";
#elif AU_DEVELOPMENT
            return "Development";
#else
            return "Release";
#endif
        }

        std::optional<std::string_view> FindArgument(int argc, const char* const* argv, std::string_view prefix)
        {
            for (int i = 1; i < argc; ++i)
            {
                const std::string_view arg = argv[i];
                if (arg.starts_with(prefix))
                {
                    return arg.substr(prefix.size());
                }
            }
            return std::nullopt;
        }

        RHI::ERHIBackend SelectBackend(int argc, const char* const* argv, const ConfigFile& config)
        {
            std::optional<std::string> requested;
            if (const auto arg = FindArgument(argc, argv, "--rhi="))
            {
                requested = std::string(*arg);
            }
            else
            {
                requested = config.Get("Renderer", "RHI");
            }

            if (requested)
            {
                const auto backend = RHI::ParseBackend(*requested);
                if (backend && IsRHIBackendAvailable(*backend))
                {
                    return *backend;
                }
                AU_LOG_WARN("RHI '{}' is not available in this build, using {}", *requested,
                            RHI::ToString(GetDefaultRHIBackend()));
            }
            return GetDefaultRHIBackend();
        }
    } // namespace

    Application::Application(ApplicationDesc desc)
        : m_Desc(std::move(desc))
    {
    }

    Application::~Application() = default;

    int Application::Run(int argc, char** argv)
    {
        if (!Paths::Initialize(argc, argv))
        {
            AU_LOG_CRITICAL("{}: startup failed, Content directory not found", m_Desc.Name);
            return 1;
        }

        Log::Initialize(Paths::GetSavedDir() / "Logs" / (m_Desc.Name + ".log"));
        AU_LOG_INFO("{} starting ({}, {} build)", m_Desc.Name, Platform::GetPlatformName(), GetBuildConfigName());
        AU_LOG_INFO("  Executable: {}", Paths::ToUtf8(Paths::GetExecutableDir()));
        AU_LOG_INFO("  Content:    {}", Paths::ToUtf8(Paths::GetContentDir()));
        AU_LOG_INFO("  Saved:      {}", Paths::ToUtf8(Paths::GetSavedDir()));

        m_AssetManager = std::make_unique<AssetManager>(Paths::GetContentDir());

        ConfigFile config;
        if (const auto text = m_AssetManager->ReadText("Config/Engine.ini"))
        {
            config = ConfigFile::Parse(*text);
        }
        else
        {
            AU_LOG_WARN("Config/Engine.ini not found, using defaults");
        }

        m_Renderer = std::make_unique<Renderer>();
        if (!m_Renderer->Initialize(SelectBackend(argc, argv, config)))
        {
            AU_LOG_CRITICAL("{}: renderer initialization failed", m_Desc.Name);
            Log::Shutdown();
            return 1;
        }

        if (!OnInitialize())
        {
            AU_LOG_CRITICAL("{}: OnInitialize failed", m_Desc.Name);
            Log::Shutdown();
            return 1;
        }

        // The main loop arrives together with the Platform window layer.

        OnShutdown();
        m_Renderer->Shutdown();
        AU_LOG_INFO("{} shut down", m_Desc.Name);
        Log::Shutdown();
        return 0;
    }
} // namespace Au
