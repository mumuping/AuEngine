#include "Core/Memory.h"
#include "Core/Paths.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

TEST(Paths, ExecutableDirectoryExists)
{
    ASSERT_TRUE(Au::Paths::Initialize());
    EXPECT_TRUE(fs::is_directory(Au::Paths::GetExecutableDir()));
}

TEST(Paths, ContentDirectoryContainsEngineConfig)
{
    ASSERT_TRUE(Au::Paths::Initialize());
    EXPECT_TRUE(fs::is_regular_file(Au::Paths::GetContentDir() / "Config" / "Engine.ini"));
}

TEST(Paths, SavedDirectoryIsCreated)
{
    ASSERT_TRUE(Au::Paths::Initialize());
    EXPECT_TRUE(fs::is_directory(Au::Paths::GetSavedDir()));
}

TEST(Memory, AlignedAllocation)
{
    constexpr std::size_t alignment = 64;
    void* ptr = Au::Memory::Allocate(256, alignment);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(ptr) % alignment, 0U);
    Au::Memory::Free(ptr);
}

TEST(TestData, SampleFileIsReadable)
{
    std::ifstream file(fs::path(u8"" AU_TEST_DATA_DIR) / "Sample.txt");
    ASSERT_TRUE(file.is_open());
    std::string line;
    std::getline(file, line);
    EXPECT_EQ(line, "AuEngine test data");
}
