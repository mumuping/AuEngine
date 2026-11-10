#pragma once

#include "Core/Common/Assert.h"
#include "Core/Memory/HeapAllocator.h"
#include "Core/Memory/IAllocator.h"

#include <limits>

namespace Au::Memory {

    // STL-compatible allocator that forwards to an IAllocator backend (defaults to heap).
    template <typename T>
    class Allocator {
      public:
        using value_type = T;

        Allocator() noexcept : m_Backend(&GetHeapAllocator()) {}

        // NOLINTNEXTLINE(cppcoreguidelines-explicit-constructor,misc-explicit-constructor)
        Allocator(IAllocator& backend) noexcept : m_Backend(&backend) {}

        template <typename U>
        // NOLINTNEXTLINE(cppcoreguidelines-explicit-constructor,misc-explicit-constructor)
        Allocator(const Allocator<U>& other) noexcept : m_Backend(other.Backend()) {}

        // NOLINTNEXTLINE(readability-identifier-naming)
        [[nodiscard]] T* allocate(usize count) {
            AU_ENSURE(count <= (std::numeric_limits<usize>::max() / sizeof(T)));
            return static_cast<T*>(m_Backend->Allocate(sizeof(T) * count, alignof(T)));
        }

        // NOLINTNEXTLINE(readability-identifier-naming)
        void deallocate(T* ptr, usize /*count*/) noexcept { m_Backend->Free(ptr); }

        [[nodiscard]] IAllocator* Backend() const noexcept { return m_Backend; }

        template <typename U>
        [[nodiscard]] bool operator==(const Allocator<U>& other) const noexcept {
            return m_Backend == other.Backend();
        }

        template <typename U>
        [[nodiscard]] bool operator!=(const Allocator<U>& other) const noexcept {
            return !(*this == other);
        }

      private:
        IAllocator* m_Backend = nullptr;
    };

}  // namespace Au::Memory
