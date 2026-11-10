#pragma once

#include "Core/Common/Expected.h"
#include "Core/Common/Types.h"
#include "Core/FileSystem/FileTypes.h"
#include "Core/Memory/UniquePtr.h"

#include <span>

namespace Au::Fs {

    class IFile {
      public:
        virtual ~IFile() = default;

        virtual Result<usize>     Read(std::span<u8> buffer)           = 0;
        virtual Result<usize>     Write(std::span<const u8> buffer)    = 0;
        virtual Status            Seek(i64 offset, ESeekOrigin origin) = 0;
        [[nodiscard]] virtual u64 Tell() const                         = 0;
        [[nodiscard]] virtual u64 Size() const                         = 0;
    };

    using FilePtr = UniquePtr<IFile>;

}  // namespace Au::Fs
