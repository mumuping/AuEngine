#include "Core/Memory/ArenaAllocator.h"

#include "Core/Common/Assert.h"
#include "Core/Math/Alignment.h"
#include "Core/Memory/Memory.h"

#include <algorithm>

namespace Au::Memory {

    struct ArenaAllocator::Block {
        Block* Next     = nullptr;
        usize  Capacity = 0;
        usize  Offset   = 0;
        u8*    Data     = nullptr;
    };

    ArenaAllocator::ArenaAllocator(usize blockSize, EMemoryTag tag) : m_BlockSize(blockSize), m_Tag(tag) {
        AU_ENSURE(blockSize > 0);
    }

    ArenaAllocator::~ArenaAllocator() {
        ReleaseBlocks();
    }

    void* ArenaAllocator::Allocate(usize size, usize alignment) {
        AU_ENSURE(size > 0);
        AU_ENSURE(Math::IsPowerOfTwo(alignment));

        if (m_Current != nullptr) {
            const usize alignedOffset = Math::AlignUp(m_Current->Offset, alignment);
            if (alignedOffset + size <= m_Current->Capacity) {
                void* const result = m_Current->Data + alignedOffset;
                m_Current->Offset  = alignedOffset + size;
                m_Allocated += size;
                return result;
            }
        }

        Block* const block         = AllocateBlock(size + alignment);
        const usize  alignedOffset = Math::AlignUp(block->Offset, alignment);
        AU_ENSURE(alignedOffset + size <= block->Capacity);

        void* const result = block->Data + alignedOffset;
        block->Offset      = alignedOffset + size;
        m_Allocated += size;
        return result;
    }

    void ArenaAllocator::Free(void* /*ptr*/) noexcept {
        // Linear allocator: individual frees are no-ops. Use Reset().
    }

    void ArenaAllocator::Reset() noexcept {
        for (Block* block = m_Blocks; block != nullptr; block = block->Next) {
            block->Offset = 0;
        }
        m_Current   = m_Blocks;
        m_Allocated = 0;
    }

    EMemoryTag ArenaAllocator::GetTag() const noexcept {
        return m_Tag;
    }

    usize ArenaAllocator::GetBlockSize() const noexcept {
        return m_BlockSize;
    }

    usize ArenaAllocator::GetAllocatedBytes() const noexcept {
        return m_Allocated;
    }

    ArenaAllocator::Block* ArenaAllocator::AllocateBlock(usize minimumSize) {
        const usize capacity = std::max(m_BlockSize, minimumSize);
        void* const memory   = ::Au::Memory::Allocate(sizeof(Block) + capacity, alignof(Block), m_Tag);

        auto* const block = static_cast<Block*>(memory);
        block->Next       = m_Blocks;
        block->Capacity   = capacity;
        block->Offset     = 0;
        block->Data       = reinterpret_cast<u8*>(block + 1);

        m_Blocks  = block;
        m_Current = block;
        return block;
    }

    void ArenaAllocator::ReleaseBlocks() noexcept {
        Block* block = m_Blocks;
        while (block != nullptr) {
            Block* const next = block->Next;
            ::Au::Memory::Free(block);
            block = next;
        }
        m_Blocks    = nullptr;
        m_Current   = nullptr;
        m_Allocated = 0;
    }

}  // namespace Au::Memory
