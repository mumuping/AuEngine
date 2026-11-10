#include "Core/Memory/HeapAllocator.h"

#include "Core/Memory/Memory.h"

namespace Au::Memory {

    HeapAllocator::HeapAllocator(EMemoryTag tag) noexcept : m_Tag(tag) {
    }

    void* HeapAllocator::Allocate(usize size, usize alignment) {
        return ::Au::Memory::Allocate(size, alignment, m_Tag);
    }

    void HeapAllocator::Free(void* ptr) noexcept {
        ::Au::Memory::Free(ptr);
    }

    EMemoryTag HeapAllocator::GetTag() const noexcept {
        return m_Tag;
    }

    HeapAllocator& GetHeapAllocator() noexcept {
        static HeapAllocator s_HeapAllocator{EMemoryTag::General};
        return s_HeapAllocator;
    }

}  // namespace Au::Memory
