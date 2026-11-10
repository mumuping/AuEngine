#pragma once

#include "Core/Common/Types.h"

namespace Au::Fs {

    enum class EFileAccess : u8 {
        Read      = 0,
        Write     = 1,
        ReadWrite = 2,
    };

    enum class EFileMode : u8 {
        OpenExisting = 0,
        CreateNew    = 1,
        CreateAlways = 2,
        Append       = 3,
        Truncate     = 4,
    };

    enum class ESeekOrigin : u8 {
        Begin   = 0,
        Current = 1,
        End     = 2,
    };

    struct FileStat {
        bool Exists        = false;
        bool IsDirectory   = false;
        bool IsRegularFile = false;
        u64  Size          = 0;
    };

}  // namespace Au::Fs
