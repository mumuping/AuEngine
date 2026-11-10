#include "Core/FileSystem/NativeFileSystem.h"

#include "Core/Common/Assert.h"
#include "Core/Common/Error.h"

#include <algorithm>
#include <fstream>
#include <utility>

namespace Au::Fs {
    namespace {

        [[nodiscard]] std::filesystem::path ToFilesystemPath(std::string_view utf8) {
            return {std::u8string(utf8.begin(), utf8.end())};
        }

        [[nodiscard]] std::string ToUtf8(const std::filesystem::path& path) {
            const std::u8string utf8 = path.u8string();
            return {utf8.begin(), utf8.end()};
        }

        [[nodiscard]] std::filesystem::path CanonicalOrNormal(const std::filesystem::path& path) {
            std::error_code             error;
            const std::filesystem::path canonical = std::filesystem::weakly_canonical(path, error);
            return error ? path.lexically_normal() : canonical;
        }

        [[nodiscard]] bool IsWithinRoot(const std::filesystem::path& root, const std::filesystem::path& candidate) {
            const std::filesystem::path relative = candidate.lexically_relative(root);
            if (relative.empty()) {
                return candidate == root;
            }
            return std::ranges::all_of(relative, [](const std::filesystem::path& part) { return part != ".."; });
        }

        [[nodiscard]] Result<std::filesystem::path>
        MapUnderRoot(const std::filesystem::path& root, const Path& virtualPath) {
            if (!virtualPath.IsAbsolute()) {
                return Unexpected(Error::Format({}, "native fs path must be absolute: '{}'", virtualPath.View()));
            }

            std::filesystem::path combined = root;
            if (!virtualPath.IsRoot()) {
                combined /= ToFilesystemPath(virtualPath.View().substr(1));
            }

            std::filesystem::path normalized = CanonicalOrNormal(combined);
            if (!IsWithinRoot(root, normalized)) {
                return Unexpected(Error::Format({}, "path escapes native root: '{}'", virtualPath.View()));
            }
            return normalized;
        }

        [[nodiscard]] Status EnsureWritable(bool readOnly, const Path& path) {
            if (readOnly) {
                return Unexpected(Error::Format({}, "read-only mount: '{}'", path.View()));
            }
            return {};
        }

        [[nodiscard]] bool ModeRequiresWrite(EFileMode mode) noexcept {
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

        class NativeFile final : public IFile {
          public:
            NativeFile(std::fstream stream, EFileAccess access) : m_Stream(std::move(stream)), m_Access(access) {
                AU_ENSURE(m_Stream.good());
                const std::streampos end = m_Stream.seekg(0, std::ios::end).tellg();
                m_Size                   = end < 0 ? 0 : static_cast<u64>(end);
                m_Stream.clear();
                m_Stream.seekg(0, std::ios::beg);
                m_Position = 0;
            }

            Result<usize> Read(std::span<u8> buffer) override {
                if (m_Access == EFileAccess::Write) {
                    return Unexpected(Error::Make("file not opened for reading"));
                }
                m_Stream.clear();
                m_Stream.seekg(static_cast<std::streamoff>(m_Position));
                m_Stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
                const std::streamsize got = m_Stream.gcount();
                if (m_Stream.bad()) {
                    return Unexpected(Error::Make("native file read failed"));
                }
                m_Stream.clear();
                m_Position += static_cast<u64>(got);
                return static_cast<usize>(got);
            }

            Result<usize> Write(std::span<const u8> buffer) override {
                if (m_Access == EFileAccess::Read) {
                    return Unexpected(Error::Make("file not opened for writing"));
                }
                m_Stream.clear();
                m_Stream.seekp(static_cast<std::streamoff>(m_Position));
                m_Stream.write(
                    reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size())
                );
                if (!m_Stream.good()) {
                    return Unexpected(Error::Make("native file write failed"));
                }
                m_Position += static_cast<u64>(buffer.size());
                m_Size = std::max(m_Position, m_Size);
                return buffer.size();
            }

            Status Seek(i64 offset, ESeekOrigin origin) override {
                i64 target = 0;
                switch (origin) {
                    case ESeekOrigin::Begin:
                        target = offset;
                        break;
                    case ESeekOrigin::Current:
                        target = static_cast<i64>(m_Position) + offset;
                        break;
                    case ESeekOrigin::End:
                        target = static_cast<i64>(m_Size) + offset;
                        break;
                }
                if (target < 0) {
                    return Unexpected(Error::Make("seek before beginning of file"));
                }
                m_Position = static_cast<u64>(target);
                return {};
            }

            [[nodiscard]] u64 Tell() const override { return m_Position; }

            [[nodiscard]] u64 Size() const override { return m_Size; }

          private:
            std::fstream m_Stream;
            EFileAccess  m_Access   = EFileAccess::Read;
            u64          m_Position = 0;
            u64          m_Size     = 0;
        };

        [[nodiscard]] std::ios::openmode BuildOpenMode(EFileAccess access, EFileMode mode) {
            // NOLINTBEGIN(bugprone-signed-bitwise) -- std::ios::openmode is a signed STL flag enum
            std::ios::openmode flags = std::ios::binary;
            switch (access) {
                case EFileAccess::Read:
                    flags |= std::ios::in;
                    break;
                case EFileAccess::Write:
                    flags |= std::ios::out;
                    break;
                case EFileAccess::ReadWrite:
                    flags |= std::ios::in | std::ios::out;
                    break;
            }
            switch (mode) {
                case EFileMode::OpenExisting:
                    break;
                case EFileMode::CreateNew:
                    flags |= std::ios::out;
                    break;
                case EFileMode::CreateAlways:
                case EFileMode::Truncate:
                    flags |= std::ios::out | std::ios::trunc;
                    break;
                case EFileMode::Append:
                    flags |= std::ios::out | std::ios::app;
                    break;
            }
            // NOLINTEND(bugprone-signed-bitwise)
            return flags;
        }

    }  // namespace

    struct NativeFileSystem::Impl {
        std::filesystem::path Root;
        bool                  ReadOnly = false;
    };

    NativeFileSystem::NativeFileSystem(const std::filesystem::path& root, bool readOnly)
        : m_Impl(MakeUnique<Impl>(Memory::EMemoryTag::FileSystem)) {
        std::error_code error;
        std::filesystem::create_directories(root, error);
        m_Impl->Root     = CanonicalOrNormal(root);
        m_Impl->ReadOnly = readOnly;
    }

    NativeFileSystem::~NativeFileSystem() = default;

    const std::filesystem::path& NativeFileSystem::GetRoot() const noexcept {
        return m_Impl->Root;
    }

    bool NativeFileSystem::IsReadOnly() const noexcept {
        return m_Impl->ReadOnly;
    }

    Result<FileStat> NativeFileSystem::Stat(const Path& path) const {
        const Result<std::filesystem::path> mapped = MapUnderRoot(m_Impl->Root, path);
        if (!mapped) {
            return Unexpected(mapped.error());
        }

        std::error_code error;
        if (!std::filesystem::exists(*mapped, error)) {
            if (error) {
                return Unexpected(Error::FromStdError(error));
            }
            return FileStat{};
        }

        FileStat stat;
        stat.Exists      = true;
        stat.IsDirectory = std::filesystem::is_directory(*mapped, error);
        if (error) {
            return Unexpected(Error::FromStdError(error));
        }
        stat.IsRegularFile = std::filesystem::is_regular_file(*mapped, error);
        if (error) {
            return Unexpected(Error::FromStdError(error));
        }
        if (stat.IsRegularFile) {
            stat.Size = static_cast<u64>(std::filesystem::file_size(*mapped, error));
            if (error) {
                return Unexpected(Error::FromStdError(error));
            }
        }
        return stat;
    }

    Result<FilePtr> NativeFileSystem::Open(const Path& path, EFileAccess access, EFileMode mode) {
        const bool writing = access != EFileAccess::Read || ModeRequiresWrite(mode);
        if (writing) {
            if (Status writable = EnsureWritable(m_Impl->ReadOnly, path); !writable) {
                return Unexpected(writable.error());
            }
        }
        if (ModeRequiresWrite(mode) && access == EFileAccess::Read) {
            return Unexpected(Error::Make("create/truncate/append requires write access"));
        }

        const Result<std::filesystem::path> mapped = MapUnderRoot(m_Impl->Root, path);
        if (!mapped) {
            return Unexpected(mapped.error());
        }

        std::error_code error;
        const bool      exists = std::filesystem::exists(*mapped, error);
        if (error) {
            return Unexpected(Error::FromStdError(error));
        }

        switch (mode) {
            case EFileMode::OpenExisting:
            case EFileMode::Truncate:
                if (!exists) {
                    return Unexpected(Error::Format({}, "file not found: '{}'", path.View()));
                }
                break;
            case EFileMode::CreateNew:
                if (exists) {
                    return Unexpected(Error::Format({}, "file already exists: '{}'", path.View()));
                }
                break;
            case EFileMode::CreateAlways:
            case EFileMode::Append:
                break;
        }

        if (writing) {
            const std::filesystem::path parent = mapped->parent_path();
            if (!parent.empty()) {
                std::filesystem::create_directories(parent, error);
                if (error) {
                    return Unexpected(Error::FromStdError(error));
                }
            }
        }

        if (mode == EFileMode::CreateNew) {
            // NOLINTNEXTLINE(bugprone-signed-bitwise) -- std::ios::openmode is a signed STL flag enum
            std::ofstream createStream(*mapped, std::ios::binary | std::ios::out);
            if (!createStream) {
                return Unexpected(Error::Format({}, "failed to create native file '{}'", ToUtf8(*mapped)));
            }
            createStream.close();
        }

        const EFileMode openMode = (mode == EFileMode::CreateNew) ? EFileMode::OpenExisting : mode;
        std::fstream    stream(*mapped, BuildOpenMode(access, openMode));
        if (!stream) {
            return Unexpected(Error::Format({}, "failed to open native file '{}'", ToUtf8(*mapped)));
        }

        auto file = MakeUnique<NativeFile>(Memory::EMemoryTag::FileSystem, std::move(stream), access);
        if (mode == EFileMode::Append) {
            if (Status seekStatus = file->Seek(0, ESeekOrigin::End); !seekStatus) {
                return Unexpected(seekStatus.error());
            }
        }
        return FilePtr(std::move(file));
    }

    Status NativeFileSystem::Remove(const Path& path) {
        if (Status writable = EnsureWritable(m_Impl->ReadOnly, path); !writable) {
            return writable;
        }
        const Result<std::filesystem::path> mapped = MapUnderRoot(m_Impl->Root, path);
        if (!mapped) {
            return Unexpected(mapped.error());
        }
        std::error_code error;
        const bool      removed = std::filesystem::remove(*mapped, error);
        if (error) {
            return Unexpected(Error::FromStdError(error));
        }
        if (!removed) {
            return Unexpected(Error::Format({}, "path not found: '{}'", path.View()));
        }
        return {};
    }

    Status NativeFileSystem::CreateDirectories(const Path& path) {
        if (Status writable = EnsureWritable(m_Impl->ReadOnly, path); !writable) {
            return writable;
        }
        const Result<std::filesystem::path> mapped = MapUnderRoot(m_Impl->Root, path);
        if (!mapped) {
            return Unexpected(mapped.error());
        }
        std::error_code error;
        std::filesystem::create_directories(*mapped, error);
        if (error) {
            return Unexpected(Error::FromStdError(error));
        }
        return {};
    }

    Result<std::vector<Path>> NativeFileSystem::Enumerate(const Path& directory) const {
        const Result<std::filesystem::path> mapped = MapUnderRoot(m_Impl->Root, directory);
        if (!mapped) {
            return Unexpected(mapped.error());
        }

        std::error_code error;
        if (!std::filesystem::is_directory(*mapped, error)) {
            if (error) {
                return Unexpected(Error::FromStdError(error));
            }
            return Unexpected(Error::Format({}, "directory not found: '{}'", directory.View()));
        }

        std::vector<Path> children;
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(*mapped, error)) {
            if (error) {
                return Unexpected(Error::FromStdError(error));
            }
            const std::string name = ToUtf8(entry.path().filename());
            Result<Path> joined = directory.IsRoot() ? Path::Normalize(std::string("/") + name) : directory.Join(name);
            if (!joined) {
                return Unexpected(joined.error());
            }
            children.push_back(std::move(*joined));
        }
        if (error) {
            return Unexpected(Error::FromStdError(error));
        }
        return children;
    }

}  // namespace Au::Fs
