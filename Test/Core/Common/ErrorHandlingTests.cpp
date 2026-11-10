#include "Core/Common/Assert.h"
#include "Core/Common/Error.h"
#include "Core/Common/Exception.h"
#include "Core/Common/Expected.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <system_error>

namespace {

    struct FatalCapture {
        bool                 Called = false;
        std::string          Message;
        std::source_location Location;
    };

    FatalCapture g_Capture;

    void RecordingFatalHandler(const std::source_location& location, std::string_view message) {
        g_Capture.Called   = true;
        g_Capture.Message  = std::string(message);
        g_Capture.Location = location;
    }

    class FatalHandlerGuard {
      public:
        FatalHandlerGuard() : m_Previous(Au::SetFatalHandler(&RecordingFatalHandler)) {
            g_Capture = {};
        }

        ~FatalHandlerGuard() { (void)Au::SetFatalHandler(m_Previous); }

        FatalHandlerGuard(const FatalHandlerGuard&)            = delete;
        FatalHandlerGuard& operator=(const FatalHandlerGuard&) = delete;

      private:
        Au::FatalHandler m_Previous;
    };

}  // namespace

TEST(ErrorTest, MakeStoresMessageAndLocation) {
    const Au::Error error = Au::Error::Make("boom");

    EXPECT_EQ(error.Message(), "boom");
    EXPECT_FALSE(error.HasCode());
    EXPECT_NE(error.Location().file_name(), nullptr);
    EXPECT_GT(error.Location().line(), 0U);
}

TEST(ErrorTest, FormatBuildsMessage) {
    const Au::ErrorLocation location;
    const Au::Error         error = Au::Error::Format(location, "failed with {}", 42);

    EXPECT_EQ(error.Message(), "failed with 42");
}

TEST(ErrorTest, FromStdErrorKeepsCode) {
    const std::error_code code(static_cast<int>(std::errc::no_such_file_or_directory), std::generic_category());
    const Au::Error       error = Au::Error::FromStdError(code);

    EXPECT_TRUE(error.HasCode());
    EXPECT_EQ(error.Code(), code);
    EXPECT_FALSE(error.Message().empty());
}

TEST(ExpectedTest, ResultSuccess) {
    const Au::Result<int> result = 7;

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 7);
    EXPECT_EQ(result.value(), 7);
    EXPECT_EQ(result.value_or(0), 7);
}

TEST(ExpectedTest, ResultFailureAndValueOr) {
    const Au::Result<int> result = Au::Unexpected(Au::Error::Make("nope"));

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().Message(), "nope");
    EXPECT_EQ(result.value_or(99), 99);
}

TEST(ExpectedTest, ResultValueThrowsException) {
    const Au::Result<int> result = Au::Unexpected(Au::Error::Make("explode"));

    try {
        (void)result.value();
        FAIL() << "expected Au::Exception";
    } catch (const Au::Exception& ex) {
        EXPECT_EQ(ex.GetError().Message(), "explode");
        EXPECT_STREQ(ex.what(), "explode");
    }
}

TEST(ExpectedTest, StatusSuccessAndFailure) {
    const Au::Status ok{};
    EXPECT_TRUE(ok.has_value());
    ok.value();

    const Au::Status failed = Au::Unexpected(Au::Error::Make("status-failed"));
    ASSERT_FALSE(failed);
    EXPECT_EQ(failed.error().Message(), "status-failed");

    try {
        failed.value();
        FAIL() << "expected Au::Exception";
    } catch (const Au::Exception& ex) {
        EXPECT_EQ(ex.GetError().Message(), "status-failed");
    }
}

TEST(ExceptionTest, ThrowHelper) {
    try {
        Au::Throw(Au::Error::Make("thrown"));
        FAIL() << "expected Au::Exception";
    } catch (const Au::Exception& ex) {
        EXPECT_EQ(ex.GetError().Message(), "thrown");
    }
}

TEST(AssertTest, EnsureInvokesFatalHandler) {
    const FatalHandlerGuard guard;

    AU_ENSURE(false);

    EXPECT_TRUE(g_Capture.Called);
    EXPECT_NE(g_Capture.Message.find("AU_ENSURE"), std::string::npos);
    EXPECT_GT(g_Capture.Location.line(), 0U);
}

TEST(AssertTest, EnsureMsgUsesFatalHandler) {
    const FatalHandlerGuard guard;

    AU_ENSURE_MSG(false, "ensure-{}", 3);

    EXPECT_TRUE(g_Capture.Called);
    EXPECT_EQ(g_Capture.Message, "ensure-3");
}
