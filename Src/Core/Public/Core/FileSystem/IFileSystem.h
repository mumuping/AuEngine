#pragma once

#include "Core/Common/Expected.h"
#include "Core/FileSystem/FileTypes.h"
#include "Core/FileSystem/IFile.h"
#include "Core/FileSystem/Path.h"
#include "Core/Memory/UniquePtr.h"

#include <vector>

namespace Au::Fs {

    class IFileSystem {
      public:
        virtual ~IFileSystem() = default;

        // Not-found yields a successful FileStat with Exists == false.
        [[nodiscard]] virtual Result<FileStat> Stat(const Path& path) const = 0;

        virtual Result<FilePtr>                         Open(const Path& path, EFileAccess access, EFileMode mode) = 0;
        virtual Status                                  Remove(const Path& path)                                   = 0;
        virtual Status                                  CreateDirectories(const Path& path)                        = 0;
        [[nodiscard]] virtual Result<std::vector<Path>> Enumerate(const Path& directory) const                     = 0;
    };

    using FileSystemPtr = UniquePtr<IFileSystem>;

}  // namespace Au::Fs
