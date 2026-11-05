#include "Core/Paths.h"

#include "Core/Log.h"

#include <cstdlib>
#include <optional>
#include <string_view>
#include <system_error>

#if AU_PLATFORM_WINDOWS
    #include <Windows.h>
    #include <shellapi.h>
#endif

namespace Au::Paths
{
    namespace fs = std::filesystem;

    namespace
    {
        struct PathState
        {
            fs::path ExecutableDir;
            fs::path ContentDir;
            fs::path SavedDir;
            bool     Initialized = false;
        };

        PathState& GetState()
        {
            static PathState s_State;
            return s_State;
        }

        fs::path QueryExecutablePath()
        {
#if AU_PLATFORM_WINDOWS
            std::wstring buffer(MAX_PATH, L'\0');
            for (;;)
            {
                const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
                if (length == 0)
                {
                    return {};
                }
                if (length < buffer.size())
                {
                    buffer.resize(length);
                    return {buffer};
                }
                buffer.resize(buffer.size() * 2);
            }
#else
            std::error_code error;
            const fs::path path = fs::read_symlink("/proc/self/exe", error);
            return error ? fs::path() : path;
#endif
        }

        std::optional<fs::path> FindContentArgument([[maybe_unused]] int argc,
                                                    [[maybe_unused]] const char* const* argv)
        {
#if AU_PLATFORM_WINDOWS
            // argv is encoded in the ANSI code page; the wide command line preserves non-ASCII paths.
            constexpr std::wstring_view prefix = L"--content=";
            int count = 0;
            LPWSTR* wideArgs = CommandLineToArgvW(GetCommandLineW(), &count);
            if (wideArgs == nullptr)
            {
                return std::nullopt;
            }
            std::optional<fs::path> result;
            for (int i = 1; i < count; ++i)
            {
                const std::wstring_view arg = wideArgs[i];
                if (arg.starts_with(prefix))
                {
                    result = fs::path(arg.substr(prefix.size()));
                }
            }
            LocalFree(static_cast<HLOCAL>(wideArgs));
            return result;
#else
            constexpr std::string_view prefix = "--content=";
            std::optional<fs::path> result;
            for (int i = 1; i < argc && argv != nullptr; ++i)
            {
                const std::string_view arg = argv[i];
                if (arg.starts_with(prefix))
                {
                    result = fs::path(arg.substr(prefix.size()));
                }
            }
            return result;
#endif
        }

        std::optional<fs::path> FindContentEnvironment()
        {
#if AU_PLATFORM_WINDOWS
            const DWORD size = GetEnvironmentVariableW(L"AU_CONTENT_DIR", nullptr, 0);
            if (size == 0)
            {
                return std::nullopt;
            }
            std::wstring value(size, L'\0');
            const DWORD written = GetEnvironmentVariableW(L"AU_CONTENT_DIR", value.data(), size);
            value.resize(written);
            return value.empty() ? std::nullopt : std::optional<fs::path>(fs::path(value));
#else
            const char* value = std::getenv("AU_CONTENT_DIR");
            if (value == nullptr || *value == '\0')
            {
                return std::nullopt;
            }
            return fs::path(value);
#endif
        }

        bool IsDirectory(const fs::path& path)
        {
            std::error_code error;
            return fs::is_directory(path, error);
        }

        fs::path Normalize(const fs::path& path)
        {
            std::error_code error;
            const fs::path result = fs::weakly_canonical(path, error);
            return error ? path.lexically_normal() : result;
        }

        void EnsureInitialized()
        {
            if (!GetState().Initialized)
            {
                Initialize();
            }
        }
    } // namespace

    bool Initialize(int argc, const char* const* argv)
    {
        PathState& state    = GetState();
        state.Initialized   = true;
        state.ExecutableDir = QueryExecutablePath().parent_path();

#ifdef AU_SOURCE_SAVED_DIR
        state.SavedDir = fs::path(u8"" AU_SOURCE_SAVED_DIR);
#else
        state.SavedDir = state.ExecutableDir / "Saved";
#endif
        state.SavedDir = Normalize(state.SavedDir);
        std::error_code error;
        fs::create_directories(state.SavedDir, error);

        std::optional<fs::path> overridePath = FindContentArgument(argc, argv);
        const char*             overrideName = "--content";
        if (!overridePath)
        {
            overridePath = FindContentEnvironment();
            overrideName = "AU_CONTENT_DIR";
        }

        if (overridePath)
        {
            if (!IsDirectory(*overridePath))
            {
                AU_LOG_ERROR("Content directory from {} does not exist: {}", overrideName, ToUtf8(*overridePath));
                state.ContentDir.clear();
                return false;
            }
            state.ContentDir = Normalize(*overridePath);
            AU_LOG_DEBUG("Content directory from {}: {}", overrideName, ToUtf8(state.ContentDir));
            return true;
        }

        const fs::path besideExecutable = state.ExecutableDir / "Content";
        if (IsDirectory(besideExecutable))
        {
            state.ContentDir = Normalize(besideExecutable);
            return true;
        }

#ifdef AU_SOURCE_CONTENT_DIR
        const fs::path sourceContent = fs::path(u8"" AU_SOURCE_CONTENT_DIR);
        if (IsDirectory(sourceContent))
        {
            state.ContentDir = Normalize(sourceContent);
            return true;
        }
#endif

        state.ContentDir.clear();
        AU_LOG_ERROR("Could not locate the Content directory. Searched:");
        AU_LOG_ERROR("  {}", ToUtf8(besideExecutable));
#ifdef AU_SOURCE_CONTENT_DIR
        AU_LOG_ERROR("  {}", ToUtf8(sourceContent));
#endif
        AU_LOG_ERROR("Pass --content=<dir> or set the AU_CONTENT_DIR environment variable.");
        return false;
    }

    const fs::path& GetExecutableDir()
    {
        EnsureInitialized();
        return GetState().ExecutableDir;
    }

    const fs::path& GetContentDir()
    {
        EnsureInitialized();
        return GetState().ContentDir;
    }

    const fs::path& GetSavedDir()
    {
        EnsureInitialized();
        return GetState().SavedDir;
    }

    std::string ToUtf8(const fs::path& path)
    {
        const std::u8string utf8 = path.u8string();
        return {utf8.begin(), utf8.end()};
    }
} // namespace Au::Paths
