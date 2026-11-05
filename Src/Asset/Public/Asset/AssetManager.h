#pragma once

#include "Asset/AssetAPI.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace Au
{
    // Relative paths are UTF-8 and use '/' separators, e.g. "Config/Engine.ini".
    class AU_ASSET_API AssetManager
    {
    public:
        explicit AssetManager(std::filesystem::path contentDir);

        [[nodiscard]] const std::filesystem::path& GetContentDir() const;
        [[nodiscard]] std::filesystem::path        Resolve(std::string_view relativePath) const;
        [[nodiscard]] bool                         Exists(std::string_view relativePath) const;
        [[nodiscard]] std::optional<std::string>   ReadText(std::string_view relativePath) const;

    private:
        std::filesystem::path m_ContentDir;
    };
} // namespace Au
