#pragma once

// Ownership / allocation decision tree:
//
// Need a heap object?
//   ├─ Single owner / transferable     → UniquePtr + MakeUnique
//   ├─ Shared across systems, type OK  → inherit RefCounter<T> + RefPtr
//   ├─ Frame / bulk temporary          → ArenaAllocator (raw ptr or arena index)
//   ├─ Many fixed-size homogeneous     → PoolAllocator
//   └─ Third-party requires shared_ptr → std::shared_ptr (boundary only)
// Borrowed, not responsible for delete → T*
//
// Do not mix RefPtr and std::shared_ptr on the same object.
// Prefer unique ownership; do not promote std::shared_ptr in public engine APIs.

#include "Core/Common/Types.h"
#include "Core/CoreAPI.h"
#include "Core/Memory/MemoryTag.h"

#include <cstddef>

namespace Au::Memory {

    struct MemoryStats {
        usize LiveBytes          = 0;
        usize LiveCount          = 0;
        usize TotalAllocateCalls = 0;
        usize TotalFreeCalls     = 0;
        usize TagBytes[static_cast<usize>(EMemoryTag::Count)]{};
        usize TagAllocCalls[static_cast<usize>(EMemoryTag::Count)]{};
    };

    // Raw heap (mimalloc). OOM is fatal via AU_ENSURE.
    [[nodiscard]] AU_CORE_API void*
    Allocate(usize size, usize alignment = alignof(std::max_align_t), EMemoryTag tag = EMemoryTag::General);

    [[nodiscard]] AU_CORE_API void* Reallocate(
        void* ptr, usize newSize, usize alignment = alignof(std::max_align_t), EMemoryTag tag = EMemoryTag::General
    );

    AU_CORE_API void Free(void* ptr) noexcept;

    [[nodiscard]] AU_CORE_API MemoryStats GetStats() noexcept;
    AU_CORE_API void                      ResetStats() noexcept;

}  // namespace Au::Memory
