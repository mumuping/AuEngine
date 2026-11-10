#pragma once

#include "Core/Common/Macros.h"
#include "Core/Common/Types.h"
#include "Core/CoreAPI.h"
#include "Core/Memory/MemoryTag.h"

#include <utility>

namespace Au::Memory {

    class AU_CORE_API IAllocator {
      public:
        AU_DISABLE_COPY_MOVE(IAllocator);

        virtual ~IAllocator() = default;

        [[nodiscard]] virtual void* Allocate(usize size, usize alignment) = 0;

        // Arena-style allocators may no-op; heap/pool free the block.
        virtual void Free(void* ptr) noexcept = 0;

        [[nodiscard]] virtual EMemoryTag GetTag() const noexcept { return EMemoryTag::General; }

        template <typename T, typename... Args>
        [[nodiscard]] T* Create(Args&&... args) {
            void* const memory = Allocate(sizeof(T), alignof(T));
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            return ::new (memory) T(std::forward<Args>(args)...);
        }

        template <typename T>
        void Destroy(T* ptr) noexcept {
            if (ptr == nullptr) {
                return;
            }
            ptr->~T();
            Free(ptr);
        }

      protected:
        IAllocator() = default;
    };

}  // namespace Au::Memory
