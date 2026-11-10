#pragma once

#include "Core/Common/Macros.h"
#include "Core/CoreAPI.h"
#include "Core/FileSystem/IFileSystem.h"
#include "Core/Memory/UniquePtr.h"

namespace Au::Fs {

    class AU_CORE_API VirtualFileSystem final : public IFileSystem {
      public:
        AU_DISABLE_COPY_MOVE(VirtualFileSystem);

        VirtualFileSystem();
        ~VirtualFileSystem() override;

        // Larger priority wins for reads. Writes use the highest-priority writable mount whose prefix matches.
        Status Mount(Path mountPoint, FileSystemPtr backend, int priority, bool readOnly);
        Status Unmount(const Path& mountPoint);

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
