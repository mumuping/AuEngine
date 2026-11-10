#pragma once

#include "Core/Memory/MemoryTag.h"
#include "Core/Memory/NewDelete.h"

#include <memory>
#include <type_traits>
#include <utility>

namespace Au::Memory {

    template <typename T>
    struct MemoryDeleter {
        constexpr MemoryDeleter() noexcept = default;

        template <typename U>
            requires std::is_convertible_v<U*, T*>
        // NOLINTNEXTLINE(cppcoreguidelines-explicit-constructor,misc-explicit-constructor,google-explicit-constructor)
        MemoryDeleter(const MemoryDeleter<U>& /*unused*/) noexcept {}

        void operator()(T* ptr) const noexcept { Delete(ptr); }
    };

    template <typename T>
    using UniquePtr = std::unique_ptr<T, MemoryDeleter<T>>;

    template <typename T, typename... Args>
    [[nodiscard]] UniquePtr<T> MakeUnique(EMemoryTag tag, Args&&... args) {
        return UniquePtr<T>(New<T>(tag, std::forward<Args>(args)...));
    }

    template <typename T, typename... Args>
        requires(!std::is_same_v<std::decay_t<Args>, EMemoryTag> && ...)
    [[nodiscard]] UniquePtr<T> MakeUnique(Args&&... args) {
        return MakeUnique<T>(EMemoryTag::General, std::forward<Args>(args)...);
    }

}  // namespace Au::Memory

namespace Au {

    template <typename T>
    using UniquePtr = Memory::UniquePtr<T>;

    template <typename T, typename... Args>
    [[nodiscard]] Memory::UniquePtr<T> MakeUnique(Args&&... args) {
        return Memory::MakeUnique<T>(std::forward<Args>(args)...);
    }

}  // namespace Au
