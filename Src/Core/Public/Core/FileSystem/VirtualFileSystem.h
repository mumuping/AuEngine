#pragma once

#include "Core/Common/Macros.h"
#include "Core/CoreAPI.h"
#include "Core/FileSystem/IFileSystem.h"

#include <memory>

namespace Au::Fs {

    class AU_CORE_API VirtualFileSystem final : public IFileSystem {
      public:
        AU_DISABLE_COPY_MOVE(VirtualFileSystem);

        VirtualFileSystem();
        ~VirtualFileSystem() override;

        // Larger priority wins for reads. Writes use the highest-priority writable mount whose prefix matches.
        Status Mount(Path mountPoint, std::unique_ptr<IFileSystem> backend, int priority, bool readOnly);
        Status Unmount(const Path& mountPoint);

        [[nodiscard]] Result<FileStat> Stat(const Path& path) const override;

        Result<std::unique_ptr<IFile>>          Open(const Path& path, EFileAccess access, EFileMode mode) override;
        Status                                  Remove(const Path& path) override;
        Status                                  CreateDirectories(const Path& path) override;
        [[nodiscard]] Result<std::vector<Path>> Enumerate(const Path& directory) const override;

      private:
        struct Impl;
        std::unique_ptr<Impl> m_Impl;
    };

}  // namespace Au::Fs
