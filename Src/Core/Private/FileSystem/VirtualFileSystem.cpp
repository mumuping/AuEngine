#include "Core/FileSystem/VirtualFileSystem.h"

#include "Core/Common/Assert.h"
#include "Core/Common/Error.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace Au::Fs {
    namespace {

        struct MountEntry {
            Path                         MountPoint;
            std::unique_ptr<IFileSystem> Backend;
            int                          Priority = 0;
            bool                         ReadOnly = false;
        };

        [[nodiscard]] bool IsWriteOperation(EFileAccess access, EFileMode mode) noexcept {
            if (access != EFileAccess::Read) {
                return true;
            }
            switch (mode) {
                case EFileMode::OpenExisting:
                    return false;
                case EFileMode::CreateNew:
                case EFileMode::CreateAlways:
                case EFileMode::Append:
                case EFileMode::Truncate:
                    return true;
            }
            return false;
        }

    }  // namespace

    struct VirtualFileSystem::Impl {
        std::vector<MountEntry> Mounts;

        void SortMounts() {
            std::ranges::stable_sort(Mounts, [](const MountEntry& lhs, const MountEntry& rhs) {
                return lhs.Priority > rhs.Priority;
            });
        }

        [[nodiscard]] std::vector<const MountEntry*> MatchingMounts(const Path& path) const {
            std::vector<const MountEntry*> matches;
            for (const MountEntry& entry : Mounts) {
                if (entry.MountPoint.IsMountPrefixOf(path)) {
                    matches.push_back(&entry);
                }
            }
            return matches;
        }
    };

    VirtualFileSystem::VirtualFileSystem() : m_Impl(std::make_unique<Impl>()) {
    }

    VirtualFileSystem::~VirtualFileSystem() = default;

    Status
    VirtualFileSystem::Mount(Path mountPoint, std::unique_ptr<IFileSystem> backend, int priority, bool readOnly) {
        AU_ENSURE(backend != nullptr);
        if (!mountPoint.IsAbsolute()) {
            return Unexpected(Error::Format({}, "mount point must be absolute: '{}'", mountPoint.View()));
        }

        MountEntry entry;
        entry.MountPoint = std::move(mountPoint);
        entry.Backend    = std::move(backend);
        entry.Priority   = priority;
        entry.ReadOnly   = readOnly;
        m_Impl->Mounts.push_back(std::move(entry));
        m_Impl->SortMounts();
        return {};
    }

    Status VirtualFileSystem::Unmount(const Path& mountPoint) {
        const auto eraseFrom =
            std::remove_if(m_Impl->Mounts.begin(), m_Impl->Mounts.end(), [&](const MountEntry& entry) {
                return entry.MountPoint.EqualsIgnoreCase(mountPoint);
            });
        if (eraseFrom == m_Impl->Mounts.end()) {
            return Unexpected(Error::Format({}, "mount not found: '{}'", mountPoint.View()));
        }
        m_Impl->Mounts.erase(eraseFrom, m_Impl->Mounts.end());
        return {};
    }

    Result<FileStat> VirtualFileSystem::Stat(const Path& path) const {
        const std::vector<const MountEntry*> matches = m_Impl->MatchingMounts(path);
        if (matches.empty()) {
            return Unexpected(Error::Format({}, "no mount for path: '{}'", path.View()));
        }

        for (const MountEntry* entry : matches) {
            Result<Path> relative = entry->MountPoint.StripMountPrefix(path);
            if (!relative) {
                continue;
            }
            Result<FileStat> stat = entry->Backend->Stat(*relative);
            if (!stat) {
                continue;
            }
            if (stat->Exists) {
                return stat;
            }
        }
        return FileStat{};
    }

    Result<std::unique_ptr<IFile>> VirtualFileSystem::Open(const Path& path, EFileAccess access, EFileMode mode) {
        const bool                             writing = IsWriteOperation(access, mode);
        const std::vector<const MountEntry*> matches = m_Impl->MatchingMounts(path);
        if (matches.empty()) {
            return Unexpected(Error::Format({}, "no mount for path: '{}'", path.View()));
        }

        if (writing) {
            for (const MountEntry* entry : matches) {
                if (entry->ReadOnly) {
                    continue;
                }
                Result<Path> relative = entry->MountPoint.StripMountPrefix(path);
                if (!relative) {
                    return Unexpected(relative.error());
                }
                return entry->Backend->Open(*relative, access, mode);
            }
            return Unexpected(Error::Format({}, "no writable mount for path: '{}'", path.View()));
        }

        for (const MountEntry* entry : matches) {
            Result<Path> relative = entry->MountPoint.StripMountPrefix(path);
            if (!relative) {
                continue;
            }
            Result<FileStat> stat = entry->Backend->Stat(*relative);
            if (!stat) {
                continue;
            }
            if (!stat->Exists || !stat->IsRegularFile) {
                continue;
            }
            return entry->Backend->Open(*relative, access, mode);
        }
        return Unexpected(Error::Format({}, "file not found: '{}'", path.View()));
    }

    Status VirtualFileSystem::Remove(const Path& path) {
        const std::vector<const MountEntry*> matches = m_Impl->MatchingMounts(path);
        if (matches.empty()) {
            return Unexpected(Error::Format({}, "no mount for path: '{}'", path.View()));
        }
        for (const MountEntry* entry : matches) {
            if (entry->ReadOnly) {
                continue;
            }
            Result<Path> relative = entry->MountPoint.StripMountPrefix(path);
            if (!relative) {
                return Unexpected(relative.error());
            }
            return entry->Backend->Remove(*relative);
        }
        return Unexpected(Error::Format({}, "no writable mount for path: '{}'", path.View()));
    }

    Status VirtualFileSystem::CreateDirectories(const Path& path) {
        const std::vector<const MountEntry*> matches = m_Impl->MatchingMounts(path);
        if (matches.empty()) {
            return Unexpected(Error::Format({}, "no mount for path: '{}'", path.View()));
        }
        for (const MountEntry* entry : matches) {
            if (entry->ReadOnly) {
                continue;
            }
            Result<Path> relative = entry->MountPoint.StripMountPrefix(path);
            if (!relative) {
                return Unexpected(relative.error());
            }
            return entry->Backend->CreateDirectories(*relative);
        }
        return Unexpected(Error::Format({}, "no writable mount for path: '{}'", path.View()));
    }

    Result<std::vector<Path>> VirtualFileSystem::Enumerate(const Path& directory) const {
        const std::vector<const MountEntry*> matches = m_Impl->MatchingMounts(directory);
        if (matches.empty()) {
            return Unexpected(Error::Format({}, "no mount for path: '{}'", directory.View()));
        }

        // First period: enumerate only the highest-priority matching mount.
        const MountEntry* entry    = matches.front();
        Result<Path>      relative = entry->MountPoint.StripMountPrefix(directory);
        if (!relative) {
            return Unexpected(relative.error());
        }

        Result<std::vector<Path>> children = entry->Backend->Enumerate(*relative);
        if (!children) {
            return children;
        }

        // Backend children are absolute within the backend root ("/a.txt").
        // Rebase them onto the VFS mount point ("/game" + "a.txt" -> "/game/a.txt").
        std::vector<Path> rebased;
        rebased.reserve(children->size());
        for (const Path& child : *children) {
            if (entry->MountPoint.IsRoot()) {
                rebased.push_back(child);
                continue;
            }
            if (child.IsRoot()) {
                rebased.push_back(entry->MountPoint);
                continue;
            }
            Result<Path> joined = entry->MountPoint.Join(child.View().substr(1));
            if (!joined) {
                return Unexpected(joined.error());
            }
            rebased.push_back(std::move(*joined));
        }
        return rebased;
    }

}  // namespace Au::Fs
