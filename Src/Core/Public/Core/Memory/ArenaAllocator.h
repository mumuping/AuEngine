#pragma once

#include "Core/Common/Macros.h"
#include "Core/Common/Types.h"
#include "Core/CoreAPI.h"
#include "Core/Memory/IAllocator.h"
#include "Core/Memory/MemoryTag.h"

namespace Au::Memory {

    // Linear bump allocator. Free is a no-op; call Reset() to reclaim all blocks.
    // Do not manage Arena objects with RefPtr — lifetime is tied to the arena.
    class AU_CORE_API ArenaAllocator final : public IAllocator {
      public:
        explicit ArenaAllocator(usize blockSize = 64ULL * 1024ULL, EMemoryTag tag = EMemoryTag::Temporary);
        ~ArenaAllocator() override;

        AU_DISABLE_COPY_MOVE(ArenaAllocator);

        [[nodiscard]] void* Allocate(usize size, usize alignment) override;
        void                Free(void* ptr) noexcept override;

        void Reset() noexcept;

        [[nodiscard]] EMemoryTag GetTag() const noexcept override;
        [[nodiscard]] usize      GetBlockSize() const noexcept;
        [[nodiscard]] usize      GetAllocatedBytes() const noexcept;

      private:
        struct Block;

        [[nodiscard]] Block* AllocateBlock(usize minimumSize);
        void                 ReleaseBlocks() noexcept;

        Block*     m_Current   = nullptr;
        Block*     m_Blocks    = nullptr;
        usize      m_BlockSize = 0;
        usize      m_Allocated = 0;
        EMemoryTag m_Tag       = EMemoryTag::Temporary;
    };

}  // namespace Au::Memory
