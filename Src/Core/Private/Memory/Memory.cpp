#include "Core/Memory/Memory.h"

#include "Core/Common/Assert.h"
#include "Core/Math/Alignment.h"

#include <array>
#include <atomic>
#include <mimalloc.h>

namespace Au::Memory {
    namespace {

        [[nodiscard]] constexpr bool IsTrackingEnabled() noexcept {
#if (defined(AU_DEBUG) && AU_DEBUG) || (defined(AU_DEVELOPMENT) && AU_DEVELOPMENT)
            return true;
#else
            return false;
#endif
        }

        constexpr usize g_TagCount = static_cast<usize>(EMemoryTag::Count);

        struct StatsState {
            std::atomic<usize>                         LiveBytes{0};
            std::atomic<usize>                         LiveCount{0};
            std::atomic<usize>                         TotalAllocateCalls{0};
            std::atomic<usize>                         TotalFreeCalls{0};
            std::array<std::atomic<usize>, g_TagCount> TagBytes{};
            std::array<std::atomic<usize>, g_TagCount> TagAllocCalls{};
        };

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        StatsState g_Stats{};

        void RecordAllocate(usize size, EMemoryTag tag) noexcept {
            if (!IsTrackingEnabled()) {
                return;
            }

            const auto tagIndex = static_cast<usize>(tag);
            if (tagIndex >= g_TagCount) {
                return;
            }

            g_Stats.LiveBytes.fetch_add(size, std::memory_order_relaxed);
            g_Stats.LiveCount.fetch_add(1, std::memory_order_relaxed);
            g_Stats.TotalAllocateCalls.fetch_add(1, std::memory_order_relaxed);
            g_Stats.TagBytes[tagIndex].fetch_add(size, std::memory_order_relaxed);
            g_Stats.TagAllocCalls[tagIndex].fetch_add(1, std::memory_order_relaxed);
        }

        void RecordFree(usize usableSize) noexcept {
            if (!IsTrackingEnabled()) {
                return;
            }

            g_Stats.LiveBytes.fetch_sub(usableSize, std::memory_order_relaxed);
            g_Stats.LiveCount.fetch_sub(1, std::memory_order_relaxed);
            g_Stats.TotalFreeCalls.fetch_add(1, std::memory_order_relaxed);
        }

    }  // namespace

    void* Allocate(usize size, usize alignment, EMemoryTag tag) {
        AU_ENSURE(size > 0);
        AU_ENSURE(Math::IsPowerOfTwo(alignment));
        AU_ENSURE(static_cast<usize>(tag) < g_TagCount);

        void* const memory = mi_malloc_aligned(size, alignment);
        AU_ENSURE_MSG(memory != nullptr, "Au::Memory::Allocate failed (size={}, align={})", size, alignment);

        RecordAllocate(mi_usable_size(memory), tag);
        return memory;
    }

    void* Reallocate(void* ptr, usize newSize, usize alignment, EMemoryTag tag) {
        AU_ENSURE(newSize > 0);
        AU_ENSURE(Math::IsPowerOfTwo(alignment));
        AU_ENSURE(static_cast<usize>(tag) < g_TagCount);

        if (ptr != nullptr && IsTrackingEnabled()) {
            RecordFree(mi_usable_size(ptr));
        }

        void* const memory = mi_realloc_aligned(ptr, newSize, alignment);
        AU_ENSURE_MSG(memory != nullptr, "Au::Memory::Reallocate failed (size={}, align={})", newSize, alignment);

        RecordAllocate(mi_usable_size(memory), tag);
        return memory;
    }

    void Free(void* ptr) noexcept {
        if (ptr == nullptr) {
            return;
        }

        RecordFree(mi_usable_size(ptr));
        mi_free(ptr);
    }

    MemoryStats GetStats() noexcept {
        MemoryStats stats;
        stats.LiveBytes          = g_Stats.LiveBytes.load(std::memory_order_relaxed);
        stats.LiveCount          = g_Stats.LiveCount.load(std::memory_order_relaxed);
        stats.TotalAllocateCalls = g_Stats.TotalAllocateCalls.load(std::memory_order_relaxed);
        stats.TotalFreeCalls     = g_Stats.TotalFreeCalls.load(std::memory_order_relaxed);

        for (usize index = 0; index < g_TagCount; ++index) {
            stats.TagBytes[index]      = g_Stats.TagBytes[index].load(std::memory_order_relaxed);
            stats.TagAllocCalls[index] = g_Stats.TagAllocCalls[index].load(std::memory_order_relaxed);
        }
        return stats;
    }

    void ResetStats() noexcept {
        g_Stats.LiveBytes.store(0, std::memory_order_relaxed);
        g_Stats.LiveCount.store(0, std::memory_order_relaxed);
        g_Stats.TotalAllocateCalls.store(0, std::memory_order_relaxed);
        g_Stats.TotalFreeCalls.store(0, std::memory_order_relaxed);

        for (usize index = 0; index < g_TagCount; ++index) {
            g_Stats.TagBytes[index].store(0, std::memory_order_relaxed);
            g_Stats.TagAllocCalls[index].store(0, std::memory_order_relaxed);
        }
    }

}  // namespace Au::Memory
