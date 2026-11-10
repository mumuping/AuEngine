#pragma once

#include "Core/Common/Types.h"
#include "Core/CoreAPI.h"
#include "Core/Memory/IAllocator.h"
#include "Core/Memory/MemoryTag.h"

namespace Au::Memory {

    // Global mimalloc-backed allocator.
    class AU_CORE_API HeapAllocator final : public IAllocator {
      public:
        explicit HeapAllocator(EMemoryTag tag = EMemoryTag::General) noexcept;

        [[nodiscard]] void* Allocate(usize size, usize alignment) override;
        void                Free(void* ptr) noexcept override;

        [[nodiscard]] EMemoryTag GetTag() const noexcept override;

      private:
        EMemoryTag m_Tag;
    };

    [[nodiscard]] AU_CORE_API HeapAllocator& GetHeapAllocator() noexcept;

}  // namespace Au::Memory
