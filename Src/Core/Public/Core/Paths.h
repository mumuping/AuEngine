#pragma once

#include "Core/CoreAPI.h"

#include <filesystem>
#include <string>

namespace Au::Paths
{
    // Resolves the engine directories once at startup. Content/ is searched in this order:
    //   1. --content=<dir> on the command line, or the AU_CONTENT_DIR environment variable
    //   2. <executable dir>/Content
    //   3. the source-tree Content/ (non-Release builds only)
    // Returns false and logs the searched locations if none of them exists.
    // Calling the getters below without Initialize() resolves with no command-line arguments.
    AU_CORE_API bool Initialize(int argc = 0, const char* const* argv = nullptr);

    AU_CORE_API const std::filesystem::path& GetExecutableDir();
    AU_CORE_API const std::filesystem::path& GetContentDir();
    // Logs, caches and user settings. Source-tree Saved/ in non-Release builds, <executable dir>/Saved otherwise.
    AU_CORE_API const std::filesystem::path& GetSavedDir();

    AU_CORE_API std::string ToUtf8(const std::filesystem::path& path);
} // namespace Au::Paths
