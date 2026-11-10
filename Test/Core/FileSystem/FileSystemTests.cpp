#include "Core/FileSystem/FileSystem.h"
#include "Core/FileSystem/MemoryFileSystem.h"
#include "Core/FileSystem/NativeFileSystem.h"
#include "Core/FileSystem/Path.h"
#include "Core/FileSystem/VirtualFileSystem.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

    [[nodiscard]] Au::Fs::Path MustNormalize(std::string_view utf8) {
        Au::Result<Au::Fs::Path> path = Au::Fs::Path::Normalize(utf8);
        EXPECT_TRUE(path) << (path ? "" : path.error().Message());
        return path ? std::move(*path) : Au::Fs::Path{};
    }

    [[nodiscard]] std::filesystem::path MakeTempDir(std::string_view suite) {
        const ::testing::TestInfo* const info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::filesystem::path            dir =
            std::filesystem::temp_directory_path() / "AuEngine" / "FileSystemTests" / std::string(suite) / info->name();
        std::error_code error;
        std::filesystem::remove_all(dir, error);
        std::filesystem::create_directories(dir, error);
        return dir;
    }

}  // namespace

TEST(PathTest, NormalizeAcceptsAbsoluteAndCollapses) {
    EXPECT_EQ(MustNormalize("/game/Config/Engine.ini").View(), "/game/Config/Engine.ini");
    EXPECT_EQ(MustNormalize("/game//Config/./Engine.ini").View(), "/game/Config/Engine.ini");
    EXPECT_EQ(MustNormalize("/game/../user/a").View(), "/user/a");
    EXPECT_EQ(MustNormalize("/").View(), "/");
}

TEST(PathTest, NormalizeRejectsRelativeAndEscape) {
    const Au::Result<Au::Fs::Path> relative = Au::Fs::Path::Normalize("game/a");
    ASSERT_FALSE(relative);
    EXPECT_NE(relative.error().Message().find("absolute"), std::string::npos);

    const Au::Result<Au::Fs::Path> escape = Au::Fs::Path::Normalize("/../x");
    ASSERT_FALSE(escape);
    EXPECT_NE(escape.error().Message().find("escapes"), std::string::npos);

    const Au::Result<Au::Fs::Path> backslash = Au::Fs::Path::Normalize("/game\\a");
    ASSERT_FALSE(backslash);
}

TEST(PathTest, JoinParentFilenameAndMountPrefix) {
    const Au::Fs::Path game = MustNormalize("/game");
    const Au::Fs::Path file = MustNormalize("/game/Config/Engine.ini");

    EXPECT_EQ(file.Parent().View(), "/game/Config");
    EXPECT_EQ(file.Filename().View(), "Engine.ini");

    Au::Result<Au::Fs::Path> joined = game.Join("Config/Engine.ini");
    ASSERT_TRUE(joined);
    EXPECT_EQ(joined->View(), "/game/Config/Engine.ini");

    EXPECT_TRUE(game.IsMountPrefixOf(file));
    EXPECT_TRUE(game.IsMountPrefixOf(MustNormalize("/GAME/x")));
    EXPECT_FALSE(game.IsMountPrefixOf(MustNormalize("/gamesplay/x")));

    Au::Result<Au::Fs::Path> stripped = game.StripMountPrefix(file);
    ASSERT_TRUE(stripped);
    EXPECT_EQ(stripped->View(), "/Config/Engine.ini");
}

TEST(MemoryFileSystemTest, WriteReadStatEnumerateAndRemove) {
    Au::Fs::MemoryFileSystem memory;

    const Au::Fs::Path dir  = MustNormalize("/data");
    const Au::Fs::Path file = MustNormalize("/data/hello.txt");

    ASSERT_TRUE(memory.CreateDirectories(dir));
    ASSERT_TRUE(Au::Fs::WriteText(memory, file, "hello"));

    Au::Result<Au::Fs::FileStat> stat = memory.Stat(file);
    ASSERT_TRUE(stat);
    EXPECT_TRUE(stat->Exists);
    EXPECT_TRUE(stat->IsRegularFile);
    EXPECT_EQ(stat->Size, 5U);

    Au::Result<std::string> text = Au::Fs::ReadText(memory, file);
    ASSERT_TRUE(text);
    EXPECT_EQ(*text, "hello");

    Au::Result<std::vector<Au::Fs::Path>> children = memory.Enumerate(dir);
    ASSERT_TRUE(children);
    ASSERT_EQ(children->size(), 1U);
    EXPECT_EQ((*children)[0].View(), "/data/hello.txt");

    ASSERT_TRUE(memory.Remove(file));
    Au::Result<Au::Fs::FileStat> missing = memory.Stat(file);
    ASSERT_TRUE(missing);
    EXPECT_FALSE(missing->Exists);
}

TEST(MemoryFileSystemTest, OpenExistingMissingReturnsError) {
    Au::Fs::MemoryFileSystem memory;
    const Au::Fs::Path       path = MustNormalize("/missing.txt");

    Au::Result<std::unique_ptr<Au::Fs::IFile>> file =
        memory.Open(path, Au::Fs::EFileAccess::Read, Au::Fs::EFileMode::OpenExisting);
    ASSERT_FALSE(file);
    EXPECT_NE(file.error().Message().find("not found"), std::string::npos);
}

TEST(NativeFileSystemTest, ReadWriteAndReadOnlyRejection) {
    const std::filesystem::path root = MakeTempDir("Native");

    Au::Fs::NativeFileSystem writable(root, false);
    const Au::Fs::Path       file = MustNormalize("/notes.txt");
    ASSERT_TRUE(Au::Fs::WriteText(writable, file, "native"));
    Au::Result<std::string> text = Au::Fs::ReadText(writable, file);
    ASSERT_TRUE(text);
    EXPECT_EQ(*text, "native");

    Au::Fs::NativeFileSystem readOnly(root, true);
    Au::Status               writeStatus = Au::Fs::WriteText(readOnly, MustNormalize("/denied.txt"), "x");
    ASSERT_FALSE(writeStatus);
    EXPECT_NE(writeStatus.error().Message().find("read-only"), std::string::npos);
}

TEST(VirtualFileSystemTest, OverlayPriorityAndWritableRouting) {
    auto base    = std::make_unique<Au::Fs::MemoryFileSystem>();
    auto overlay = std::make_unique<Au::Fs::MemoryFileSystem>();
    auto saved   = std::make_unique<Au::Fs::MemoryFileSystem>();

    Au::Fs::MemoryFileSystem& baseRef    = *base;
    Au::Fs::MemoryFileSystem& overlayRef = *overlay;
    Au::Fs::MemoryFileSystem& savedRef   = *saved;

    ASSERT_TRUE(Au::Fs::WriteText(baseRef, MustNormalize("/Config/Engine.ini"), "base"));
    ASSERT_TRUE(Au::Fs::WriteText(overlayRef, MustNormalize("/Config/Engine.ini"), "overlay"));

    Au::Fs::VirtualFileSystem vfs;
    ASSERT_TRUE(vfs.Mount(MustNormalize("/game"), std::move(base), 0, true));
    ASSERT_TRUE(vfs.Mount(MustNormalize("/game"), std::move(overlay), 10, true));
    ASSERT_TRUE(vfs.Mount(MustNormalize("/user"), std::move(saved), 0, false));

    Au::Result<std::string> text = Au::Fs::ReadText(vfs, MustNormalize("/game/Config/Engine.ini"));
    ASSERT_TRUE(text);
    EXPECT_EQ(*text, "overlay");

    Au::Status denied = Au::Fs::WriteText(vfs, MustNormalize("/game/Config/Engine.ini"), "nope");
    ASSERT_FALSE(denied);
    EXPECT_NE(denied.error().Message().find("writable"), std::string::npos);

    ASSERT_TRUE(Au::Fs::WriteText(vfs, MustNormalize("/user/save.dat"), "slot1"));
    Au::Result<std::string> savedText = Au::Fs::ReadText(savedRef, MustNormalize("/save.dat"));
    ASSERT_TRUE(savedText);
    EXPECT_EQ(*savedText, "slot1");

    Au::Result<Au::Fs::FileStat> missingMount = vfs.Stat(MustNormalize("/other/x"));
    ASSERT_FALSE(missingMount);
    EXPECT_NE(missingMount.error().Message().find("no mount"), std::string::npos);
}

TEST(VirtualFileSystemTest, CaseInsensitiveMountAndUnmount) {
    auto                      memory    = std::make_unique<Au::Fs::MemoryFileSystem>();
    Au::Fs::MemoryFileSystem& memoryRef = *memory;
    ASSERT_TRUE(Au::Fs::WriteText(memoryRef, MustNormalize("/A.txt"), "ok"));

    Au::Fs::VirtualFileSystem vfs;
    ASSERT_TRUE(vfs.Mount(MustNormalize("/Game"), std::move(memory), 0, false));

    Au::Result<std::string> text = Au::Fs::ReadText(vfs, MustNormalize("/game/A.txt"));
    ASSERT_TRUE(text);
    EXPECT_EQ(*text, "ok");

    ASSERT_TRUE(vfs.Unmount(MustNormalize("/GAME")));
    const Au::Status remountMissing = vfs.Unmount(MustNormalize("/Game"));
    ASSERT_FALSE(remountMissing);
}
