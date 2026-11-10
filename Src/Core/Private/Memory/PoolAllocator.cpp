#include "Core/Memory/PoolAllocator.h"

#include "Core/Common/Assert.h"
#include "Core/Math/Alignment.h"
#include "Core/Memory/Memory.h"

#include <algorithm>

namespace Au::Memory {

    union PoolAllocator::FreeNode {
        FreeNode* Next;
        alignas(std::max_align_t) u8 Storage[1];
    };

    struct PoolAllocator::Chunk {
        Chunk* Next = nullptr;
    };

    PoolAllocator::PoolAllocator(usize blockSize, usize blocksPerChunk, EMemoryTag tag)
        : m_BlocksPerChunk(blocksPerChunk), m_Tag(tag) {
        AU_ENSURE(blockSize > 0);
        AU_ENSURE(blocksPerChunk > 0);

        const usize minimum = std::max(blockSize, sizeof(FreeNode));
        m_BlockSize         = Math::AlignUp(minimum, alignof(std::max_align_t));
    }

    PoolAllocator::~PoolAllocator() {
        Chunk* chunk = m_Chunks;
        while (chunk != nullptr) {
            Chunk* const next = chunk->Next;
            ::Au::Memory::Free(chunk);
            chunk = next;
        }
        m_Chunks          = nullptr;
        m_FreeList        = nullptr;
        m_AllocatedBlocks = 0;
    }

    void* PoolAllocator::Allocate(usize size, usize alignment) {
        AU_ENSURE(size > 0);
        AU_ENSURE(Math::IsPowerOfTwo(alignment));
        AU_ENSURE_MSG(
            size <= m_BlockSize && alignment <= alignof(std::max_align_t),
            "PoolAllocator block too small (requested size={}, align={}, block={})",
            size,
            alignment,
            m_BlockSize
        );

        if (m_FreeList == nullptr) {
            Grow();
        }

        FreeNode* const node = m_FreeList;
        m_FreeList           = node->Next;
        ++m_AllocatedBlocks;
        return node;
    }

    void PoolAllocator::Free(void* ptr) noexcept {
        if (ptr == nullptr) {
            return;
        }

        auto* const node = static_cast<FreeNode*>(ptr);
        node->Next       = m_FreeList;
        m_FreeList       = node;
        AU_ASSERT(m_AllocatedBlocks > 0);
        --m_AllocatedBlocks;
    }

    EMemoryTag PoolAllocator::GetTag() const noexcept {
        return m_Tag;
    }

    usize PoolAllocator::GetBlockSize() const noexcept {
        return m_BlockSize;
    }

    usize PoolAllocator::GetAllocatedBlocks() const noexcept {
        return m_AllocatedBlocks;
    }

    void PoolAllocator::Grow() {
        const usize bytes  = sizeof(Chunk) + (m_BlockSize * m_BlocksPerChunk);
        void* const memory = ::Au::Memory::Allocate(bytes, alignof(Chunk), m_Tag);

        auto* const chunk = static_cast<Chunk*>(memory);
        chunk->Next       = m_Chunks;
        m_Chunks          = chunk;

        auto* const storage = reinterpret_cast<u8*>(chunk + 1);
        for (usize index = 0; index < m_BlocksPerChunk; ++index) {
            auto* const node = reinterpret_cast<FreeNode*>(storage + (index * m_BlockSize));
            node->Next       = m_FreeList;
            m_FreeList       = node;
        }
    }

}  // namespace Au::Memory
