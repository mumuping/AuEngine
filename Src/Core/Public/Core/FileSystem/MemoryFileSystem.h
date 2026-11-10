#pragma once

#include "Core/Common/Macros.h"
#include "Core/CoreAPI.h"
#include "Core/FileSystem/IFileSystem.h"
#include "Core/Memory/UniquePtr.h"

namespace Au::Fs {

    class AU_CORE_API MemoryFileSystem final : public IFileSystem {
      public:
        AU_DISABLE_COPY_MOVE(MemoryFileSystem);

        MemoryFileSystem();
        ~MemoryFileSystem() override;

        [[nodiscard]] Result<FileStat> Stat(const Path& path) const override;

        Result<FilePtr>                         Open(const Path& path, EFileAccess access, EFileMode mode) override;
        Status                                  Remove(const Path& path) override;
        Status                                  CreateDirectories(const Path& path) override;
        [[nodiscard]] Result<std::vector<Path>> Enumerate(const Path& directory) const override;

      private:
        struct Impl;
        UniquePtr<Impl> m_Impl;
    };

}  // namespace Au::Fs
