#include "Core/Memory/Allocator.h"
#include "Core/Memory/ArenaAllocator.h"
#include "Core/Memory/Memory.h"
#include "Core/Memory/NewDelete.h"
#include "Core/Memory/PoolAllocator.h"
#include "Core/Memory/RefCounter.h"
#include "Core/Memory/RefPtr.h"
#include "Core/Memory/UniquePtr.h"

#include <atomic>
#include <cstdint>
#include <gtest/gtest.h>
#include <thread>
#include <utility>
#include <vector>

namespace {

    struct Tracker {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,readability-identifier-naming)
        static std::atomic<int> s_Live;

        Tracker() { s_Live.fetch_add(1, std::memory_order_relaxed); }

        ~Tracker() { s_Live.fetch_sub(1, std::memory_order_relaxed); }
    };

    // NOLINTNEXTLINE(readability-identifier-naming)
    std::atomic<int> Tracker::s_Live{0};

    // CRTP root. Virtual dtor so further derived types still run their destructors when
    // RefCounter<SharedObject>::Destroy calls object->~SharedObject().
    class SharedObject : public Au::Memory::RefCounter<SharedObject> {
      public:
        explicit SharedObject(int value) : Value(value) { s_Live.fetch_add(1, std::memory_order_relaxed); }

        virtual ~SharedObject() { s_Live.fetch_sub(1, std::memory_order_relaxed); }

        int Value = 0;

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,readability-identifier-naming)
        static std::atomic<int> s_Live;
    };

    // NOLINTNEXTLINE(readability-identifier-naming)
    std::atomic<int> SharedObject::s_Live{0};

    class DerivedShared : public SharedObject {
      public:
        explicit DerivedShared(int value) : SharedObject(value) {}
    };

    // RefCounter need not be the left-most base; CRTP Free uses static_cast<T*>.
    struct SideBase {
        int Side = 11;
    };

    // NOLINTNEXTLINE(misc-multiple-inheritance)
    class OffsetShared : public SideBase, public Au::Memory::RefCounter<OffsetShared> {
      public:
        explicit OffsetShared(int value) : Value(value) { s_Live.fetch_add(1, std::memory_order_relaxed); }

        ~OffsetShared() { s_Live.fetch_sub(1, std::memory_order_relaxed); }

        int Value = 0;

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,readability-identifier-naming)
        static std::atomic<int> s_Live;
    };

    // NOLINTNEXTLINE(readability-identifier-naming)
    std::atomic<int> OffsetShared::s_Live{0};

}  // namespace

TEST(MemoryHeapTest, AllocateFreeAndReallocate) {
    void* memory = Au::Memory::Allocate(128, 16, Au::Memory::EMemoryTag::General);
    ASSERT_NE(memory, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(memory) % 16U, 0U);

    memory = Au::Memory::Reallocate(memory, 256, 16, Au::Memory::EMemoryTag::General);
    ASSERT_NE(memory, nullptr);
    Au::Memory::Free(memory);
}

TEST(MemoryHeapTest, NewDeleteAndArray) {
    Tracker::s_Live = 0;

    auto* single = Au::Memory::New<Tracker>();
    EXPECT_EQ(Tracker::s_Live.load(), 1);
    Au::Memory::Delete(single);
    EXPECT_EQ(Tracker::s_Live.load(), 0);

    auto* many = Au::Memory::NewArray<Tracker>(4);
    EXPECT_EQ(Tracker::s_Live.load(), 4);
    Au::Memory::DeleteArray(many);
    EXPECT_EQ(Tracker::s_Live.load(), 0);
}

TEST(MemoryUniquePtrTest, MakeUniqueDestroys) {
    Tracker::s_Live = 0;
    {
        const Au::UniquePtr<Tracker> ptr = Au::MakeUnique<Tracker>();
        ASSERT_NE(ptr, nullptr);
        EXPECT_EQ(Tracker::s_Live.load(), 1);
    }
    EXPECT_EQ(Tracker::s_Live.load(), 0);
}

TEST(MemoryRefPtrTest, SharedOwnershipAndPolymorphism) {
    SharedObject::s_Live = 0;
    {
        const Au::RefPtr<SharedObject> first = Au::MakeRef<DerivedShared>(42);
        EXPECT_EQ(first->Value, 42);
        EXPECT_EQ(first->GetRefCount(), 1);
        EXPECT_EQ(SharedObject::s_Live.load(), 1);

        Au::RefPtr<SharedObject> second = first;
        EXPECT_EQ(first->GetRefCount(), 2);

        const Au::RefPtr<SharedObject> moved = std::move(second);
        // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
        EXPECT_EQ(second.Get(), nullptr);
        EXPECT_EQ(moved->GetRefCount(), 2);
    }
    EXPECT_EQ(SharedObject::s_Live.load(), 0);
}

TEST(MemoryRefPtrTest, NonLeftMostRefCounterBase) {
    OffsetShared::s_Live = 0;
    {
        const Au::RefPtr<OffsetShared> ptr = Au::MakeRef<OffsetShared>(9);
        EXPECT_EQ(ptr->Value, 9);
        EXPECT_EQ(ptr->Side, 11);
        EXPECT_EQ(ptr->GetRefCount(), 1);
    }
    EXPECT_EQ(OffsetShared::s_Live.load(), 0);
}

TEST(MemoryRefPtrTest, ConcurrentAddRefRelease) {
    SharedObject::s_Live          = 0;
    Au::RefPtr<SharedObject> root = Au::MakeRef<SharedObject>(7);
    ASSERT_EQ(root->GetRefCount(), 1);

    constexpr int            kThreads    = 4;
    constexpr int            kIterations = 1000;
    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    for (int index = 0; index < kThreads; ++index) {
        workers.emplace_back([&root] {
            for (int iteration = 0; iteration < kIterations; ++iteration) {
                // Intentional copy to exercise concurrent AddRef/Release.
                // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
                const Au::RefPtr<SharedObject> local = root;
                EXPECT_NE(local.Get(), nullptr);
                EXPECT_EQ(local->Value, 7);
            }
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    EXPECT_EQ(root->GetRefCount(), 1);
    root.Reset();
    EXPECT_EQ(SharedObject::s_Live.load(), 0);
}

TEST(MemoryArenaTest, BumpAllocateAndReset) {
    Au::Memory::ArenaAllocator arena(1024);
    void* const                a = arena.Allocate(32, 8);
    void* const                b = arena.Allocate(64, 16);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(b) % 16U, 0U);
    EXPECT_GT(arena.GetAllocatedBytes(), 0U);

    arena.Reset();
    EXPECT_EQ(arena.GetAllocatedBytes(), 0U);

    void* const c = arena.Allocate(32, 8);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c, a);
}

TEST(MemoryArenaTest, CreateDestroyTrivial) {
    Au::Memory::ArenaAllocator arena;
    int* const                 value = arena.Create<int>(123);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 123);
    arena.Destroy(value);
    arena.Reset();
}

TEST(MemoryPoolTest, ReuseBlocks) {
    Au::Memory::PoolAllocator pool(32, 8);
    void* const               first = pool.Allocate(16, 8);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(pool.GetAllocatedBlocks(), 1U);

    pool.Free(first);
    EXPECT_EQ(pool.GetAllocatedBlocks(), 0U);

    void* const second = pool.Allocate(16, 8);
    EXPECT_EQ(second, first);
}

TEST(MemoryAllocatorTest, VectorUsesBackend) {
    Au::Memory::ArenaAllocator                   arena(4096);
    const Au::Memory::Allocator<int>             alloc(arena);
    std::vector<int, Au::Memory::Allocator<int>> values(alloc);

    values.push_back(1);
    values.push_back(2);
    values.push_back(3);
    ASSERT_EQ(values.size(), 3U);
    EXPECT_EQ(values[0], 1);
    EXPECT_EQ(values[2], 3);
    EXPECT_GT(arena.GetAllocatedBytes(), 0U);
}
