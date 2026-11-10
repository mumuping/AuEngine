#pragma once

#include "Core/Common/Expected.h"
#include "Core/CoreAPI.h"
#include "Core/FileSystem/IFileSystem.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Au::Fs {

    [[nodiscard]] AU_CORE_API Result<std::vector<u8>> ReadAll(IFileSystem& fileSystem, const Path& path);
    [[nodiscard]] AU_CORE_API Result<std::string> ReadText(IFileSystem& fileSystem, const Path& path);
    [[nodiscard]] AU_CORE_API Status WriteAll(IFileSystem& fileSystem, const Path& path, std::span<const u8> data);
    [[nodiscard]] AU_CORE_API Status WriteText(IFileSystem& fileSystem, const Path& path, std::string_view text);

}  // namespace Au::Fs
