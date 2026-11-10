#include "Core/FileSystem/FileSystem.h"

#include "Core/Common/Error.h"

namespace Au::Fs {

    Result<std::vector<u8>> ReadAll(IFileSystem& fileSystem, const Path& path) {
        Result<FilePtr> file = fileSystem.Open(path, EFileAccess::Read, EFileMode::OpenExisting);
        if (!file) {
            return Unexpected(file.error());
        }

        const u64       size = (*file)->Size();
        std::vector<u8> buffer(static_cast<usize>(size));
        usize           offset = 0;
        while (offset < buffer.size()) {
            Result<usize> read = (*file)->Read(std::span<u8>(buffer.data() + offset, buffer.size() - offset));
            if (!read) {
                return Unexpected(read.error());
            }
            if (*read == 0) {
                break;
            }
            offset += *read;
        }
        buffer.resize(offset);
        return buffer;
    }

    Result<std::string> ReadText(IFileSystem& fileSystem, const Path& path) {
        Result<std::vector<u8>> bytes = ReadAll(fileSystem, path);
        if (!bytes) {
            return Unexpected(bytes.error());
        }
        return std::string(bytes->begin(), bytes->end());
    }

    Status WriteAll(IFileSystem& fileSystem, const Path& path, std::span<const u8> data) {
        Result<FilePtr> file = fileSystem.Open(path, EFileAccess::Write, EFileMode::CreateAlways);
        if (!file) {
            return Unexpected(file.error());
        }

        usize offset = 0;
        while (offset < data.size()) {
            Result<usize> written = (*file)->Write(data.subspan(offset));
            if (!written) {
                return Unexpected(written.error());
            }
            if (*written == 0) {
                return Unexpected(Error::Make("write returned zero bytes"));
            }
            offset += *written;
        }
        return {};
    }

    Status WriteText(IFileSystem& fileSystem, const Path& path, std::string_view text) {
        return WriteAll(fileSystem, path, std::span<const u8>(reinterpret_cast<const u8*>(text.data()), text.size()));
    }

}  // namespace Au::Fs
