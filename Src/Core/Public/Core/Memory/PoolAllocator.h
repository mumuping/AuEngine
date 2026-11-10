#pragma once

#include "Core/Common/Macros.h"
#include "Core/Common/Types.h"
#include "Core/CoreAPI.h"
#include "Core/Memory/IAllocator.h"
#include "Core/Memory/MemoryTag.h"

namespace Au::Memory {

    // Fixed-size block pool. Allocate requests larger than the block size fail via AU_ENSURE.
    class AU_CORE_API PoolAllocator final : public IAllocator {
      public:
        explicit PoolAllocator(usize blockSize, usize blocksPerChunk = 64, EMemoryTag tag = EMemoryTag::General);
        ~PoolAllocator() override;

        AU_DISABLE_COPY_MOVE(PoolAllocator);

        [[nodiscard]] void* Allocate(usize size, usize alignment) override;
        void                Free(void* ptr) noexcept override;

        [[nodiscard]] EMemoryTag GetTag() const noexcept override;
        [[nodiscard]] usize      GetBlockSize() const noexcept;
        [[nodiscard]] usize      GetAllocatedBlocks() const noexcept;

      private:
        struct Chunk;
        union FreeNode;

        void Grow();

        FreeNode*  m_FreeList        = nullptr;
        Chunk*     m_Chunks          = nullptr;
        usize      m_BlockSize       = 0;
        usize      m_BlocksPerChunk  = 0;
        usize      m_AllocatedBlocks = 0;
        EMemoryTag m_Tag             = EMemoryTag::General;
    };

}  // namespace Au::Memory
