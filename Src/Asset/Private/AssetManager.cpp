#include "Asset/AssetManager.h"

#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

namespace Au
{
    AssetManager::AssetManager(std::filesystem::path contentDir)
        : m_ContentDir(std::move(contentDir))
    {
    }

    const std::filesystem::path& AssetManager::GetContentDir() const
    {
        return m_ContentDir;
    }

    std::filesystem::path AssetManager::Resolve(std::string_view relativePath) const
    {
        return (m_ContentDir / std::u8string(relativePath.begin(), relativePath.end())).lexically_normal();
    }

    bool AssetManager::Exists(std::string_view relativePath) const
    {
        std::error_code error;
        return std::filesystem::is_regular_file(Resolve(relativePath), error);
    }

    std::optional<std::string> AssetManager::ReadText(std::string_view relativePath) const
    {
        std::ifstream file(Resolve(relativePath), std::ios::binary);
        if (!file)
        {
            return std::nullopt;
        }
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }
} // namespace Au
