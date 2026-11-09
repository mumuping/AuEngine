#include "Core/Log/Log.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>

namespace {

    AU_DEFINE_LOG_CATEGORY(LogUnitTest);  // NOLINT(readability-identifier-naming)

    [[nodiscard]] std::filesystem::path MakeTestLogDir() {
        const ::testing::TestInfo* const info = ::testing::UnitTest::GetInstance()->current_test_info();
        return std::filesystem::temp_directory_path() / "AuEngine" / "LogTests" / info->test_suite_name() /
               info->name();
    }

    [[nodiscard]] std::string ReadTextFile(const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        EXPECT_TRUE(input.good()) << path;
        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }

    class LogTest : public ::testing::Test {
      protected:
        void SetUp() override {
            m_Dir = MakeTestLogDir();
            std::error_code error;
            std::filesystem::remove_all(m_Dir, error);
            std::filesystem::create_directories(m_Dir, error);
            m_LogFile = m_Dir / "test.log";
        }

        void TearDown() override {
            AU_SHUTDOWN_SINGLETONS();
            std::error_code error;
            std::filesystem::remove_all(m_Dir, error);
        }

        std::filesystem::path m_Dir;
        std::filesystem::path m_LogFile;
    };

}  // namespace

TEST_F(LogTest, DefaultLevelMatchesBuildConfig) {
    Au::Log log(m_LogFile);

#if defined(AU_DEBUG) && AU_DEBUG
    EXPECT_EQ(log.GetLevel(), Au::ELogLevel::Trace);
#elif defined(AU_DEVELOPMENT) && AU_DEVELOPMENT
    EXPECT_EQ(log.GetLevel(), Au::ELogLevel::Debug);
#else
    EXPECT_EQ(log.GetLevel(), Au::ELogLevel::Info);
#endif
    EXPECT_EQ(log.GetCategoryLevel(Au::LogCore), log.GetLevel());
}

TEST_F(LogTest, SetLevelUpdatesExistingAndFutureCategories) {
    Au::Log log(m_LogFile);

    log.SetLevel(Au::ELogLevel::Warn);
    EXPECT_EQ(log.GetLevel(), Au::ELogLevel::Warn);
    EXPECT_EQ(log.GetCategoryLevel(Au::LogCore), Au::ELogLevel::Warn);

    EXPECT_EQ(log.GetCategoryLevel(LogUnitTest), Au::ELogLevel::Warn);
}

TEST_F(LogTest, SetCategoryLevelOverridesOnlyThatCategory) {
    Au::Log log(m_LogFile);

    log.SetLevel(Au::ELogLevel::Info);
    log.SetCategoryLevel(LogUnitTest, Au::ELogLevel::Error);

    EXPECT_EQ(log.GetLevel(), Au::ELogLevel::Info);
    EXPECT_EQ(log.GetCategoryLevel(Au::LogCore), Au::ELogLevel::Info);
    EXPECT_EQ(log.GetCategoryLevel(LogUnitTest), Au::ELogLevel::Error);
}

TEST_F(LogTest, WritePersistsFormattedMessageToFile) {
    constexpr std::string_view kMarker = "au-log-write-marker-42";

    {
        Au::Log log(m_LogFile);
        log.SetLevel(Au::ELogLevel::Trace);
        log.Write(Au::LogCore, Au::ELogLevel::Info, std::source_location::current(), "hello {} {}", kMarker, 7);
    }

    ASSERT_TRUE(std::filesystem::exists(m_LogFile));
    const std::string contents = ReadTextFile(m_LogFile);
    EXPECT_NE(contents.find(std::string(kMarker)), std::string::npos);
    EXPECT_NE(contents.find("hello "), std::string::npos);
    EXPECT_NE(contents.find('7'), std::string::npos);
    EXPECT_NE(contents.find("LogCore"), std::string::npos);
}

TEST_F(LogTest, WriteToCustomCategoryAppearsInFile) {
    constexpr std::string_view kMarker = "au-log-category-marker";

    {
        Au::Log log(m_LogFile);
        log.SetLevel(Au::ELogLevel::Trace);
        log.Write(LogUnitTest, Au::ELogLevel::Warn, std::source_location::current(), "{}", kMarker);
    }

    const std::string contents = ReadTextFile(m_LogFile);
    EXPECT_NE(contents.find(std::string(kMarker)), std::string::npos);
    EXPECT_NE(contents.find("LogUnitTest"), std::string::npos);
}

TEST_F(LogTest, MacroWritesWhenLogIsRegistered) {
    constexpr std::string_view kMarker = "au-log-macro-marker";

    AU_REGISTER_LOG(m_LogFile);
    AU_GET_SINGLETON(Au::Log).SetLevel(Au::ELogLevel::Trace);

    AU_LOG_INFO("macro {}", kMarker);
    AU_LOG(LogUnitTest, Error, "category-macro {}", kMarker);

    AU_SHUTDOWN_SINGLETONS();

    const std::string contents = ReadTextFile(m_LogFile);
    EXPECT_NE(contents.find(std::string(kMarker)), std::string::npos);
    EXPECT_NE(contents.find("LogCore"), std::string::npos);
    EXPECT_NE(contents.find("LogUnitTest"), std::string::npos);
}

TEST_F(LogTest, MacroIsNoOpWhenLogIsNotRegistered) {
    EXPECT_EQ(Au::SingletonSystem::Get().TryGet<Au::Log>(), nullptr);
    EXPECT_NO_THROW(AU_LOG_INFO("should be ignored {}", 123));
    EXPECT_FALSE(std::filesystem::exists(m_LogFile));
}

TEST_F(LogTest, CreatesParentDirectoriesForLogFile) {
    const std::filesystem::path nested = m_Dir / "nested" / "deep" / "engine.log";
    {
        Au::Log log(nested);
        log.Write(Au::LogCore, Au::ELogLevel::Info, std::source_location::current(), "nested-ok");
    }

    EXPECT_TRUE(std::filesystem::exists(nested));
    EXPECT_NE(ReadTextFile(nested).find("nested-ok"), std::string::npos);
}
