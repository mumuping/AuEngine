#pragma once

#include "Core/Common/Types.h"
#include "Core/CoreAPI.h"

#include <string_view>

namespace Au {

    using TypeKeyID = u32;

    // Invalid type key ID.
    constexpr TypeKeyID g_InvalidTypeKeyID = 0;  // NOLINT(clang-diagnostic-unused-const-variable)

    // Interns a unique name into a process-wide dense id. Same name always returns the same id
    // (including across shared-library boundaries, as long as this function is called in Core).
    AU_CORE_API TypeKeyID InternTypeKey(std::string_view uniqueName);

    template <typename T>
    constexpr std::string_view TypeKeyName() {
#ifdef _MSC_VER
        return __FUNCSIG__;
#else
        return __PRETTY_FUNCTION__;
#endif
    }

    // Local static only caches the Core-assigned id; identity comes from InternTypeKey.
    template <typename T>
    TypeKeyID TypeKeyTag() {
        static const TypeKeyID id = InternTypeKey(TypeKeyName<T>());
        return id;
    }

}  // namespace Au
