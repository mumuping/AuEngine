#pragma once

#include "Core/Common/Macros.h"
#include "Core/Common/Types.h"
#include "Core/Memory/Memory.h"

#include <atomic>
#include <type_traits>

namespace Au::Memory {

    // Intrusive reference count via CRTP: `class Foo : public RefCounter<Foo>`.
    template <typename T>
    class RefCounter {
      public:
        AU_DISABLE_COPY_MOVE(RefCounter);

        void AddRef() const noexcept { m_RefCount.fetch_add(1, std::memory_order_relaxed); }

        void Release() const noexcept {
            if (m_RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
                const_cast<RefCounter*>(this)->Destroy();
            }
        }

        [[nodiscard]] i32 GetRefCount() const noexcept { return m_RefCount.load(std::memory_order_relaxed); }

      private:
        friend T;

        RefCounter() { static_assert(std::is_base_of_v<RefCounter<T>, T>, "T must inherit RefCounter<T>"); }

        ~RefCounter() = default;

        void Destroy() {
            T* const object = static_cast<T*>(this);
            object->~T();
            Free(object);
        }

        mutable std::atomic<i32> m_RefCount{0};
    };

}  // namespace Au::Memory
