#pragma once

#include "Core/Common/Assert.h"
#include "Core/Common/Types.h"
#include "Core/Math/Alignment.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/MemoryTag.h"

#include <type_traits>
#include <utility>

namespace Au::Memory {
    namespace Detail {

        struct ArrayHeader {
            usize Count = 0;
        };

        [[nodiscard]] inline usize ArrayPadding(usize alignment) noexcept {
            return Math::AlignUp(sizeof(ArrayHeader), alignment);
        }

    }  // namespace Detail

    template <typename T, typename... Args>
    [[nodiscard]] T* New(EMemoryTag tag, Args&&... args) {
        static_assert(!std::is_array_v<T>, "Use NewArray for arrays");
        void* const memory = Allocate(sizeof(T), alignof(T), tag);
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        return ::new (memory) T(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
        requires(!std::is_same_v<std::decay_t<Args>, EMemoryTag> && ...)
    [[nodiscard]] T* New(Args&&... args) {
        return New<T>(EMemoryTag::General, std::forward<Args>(args)...);
    }

    template <typename T>
    void Delete(T* ptr) noexcept {
        if (ptr == nullptr) {
            return;
        }
        ptr->~T();
        Free(ptr);
    }

    template <typename T>
    [[nodiscard]] T* NewArray(usize count, EMemoryTag tag = EMemoryTag::General) {
        AU_ENSURE(count > 0);
        const usize padding = Detail::ArrayPadding(alignof(T));
        void* const raw     = Allocate(padding + (sizeof(T) * count), alignof(T), tag);

        auto* const header = static_cast<Detail::ArrayHeader*>(raw);
        header->Count      = count;

        T* const elements = reinterpret_cast<T*>(static_cast<u8*>(raw) + padding);
        for (usize index = 0; index < count; ++index) {
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            ::new (elements + index) T();
        }
        return elements;
    }

    template <typename T>
    void DeleteArray(T* ptr) noexcept {
        if (ptr == nullptr) {
            return;
        }

        const usize                padding = Detail::ArrayPadding(alignof(T));
        auto* const                raw     = reinterpret_cast<u8*>(ptr) - padding;
        const Detail::ArrayHeader* header  = reinterpret_cast<Detail::ArrayHeader*>(raw);

        for (usize index = header->Count; index > 0; --index) {
            ptr[index - 1].~T();
        }
        Free(raw);
    }

}  // namespace Au::Memory
