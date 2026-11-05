#include "Core/Memory.h"

#include <mimalloc.h>

namespace Au::Memory
{
    void* Allocate(std::size_t size, std::size_t alignment)
    {
        return mi_malloc_aligned(size, alignment);
    }

    void Free(void* ptr)
    {
        mi_free(ptr);
    }

    int GetAllocatorVersion()
    {
        return mi_version();
    }
} // namespace Au::Memory
