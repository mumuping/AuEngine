#pragma once

#include "Core/Common/Error.h"
#include "Core/CoreAPI.h"

#include <exception>

namespace Au {

    // Cross-layer failure carrying a rich Error. Prefer Expected/Result for recoverable paths.
    class AU_CORE_API Exception : public std::exception {
      public:
        explicit Exception(Error error) noexcept;

        Exception(const Exception&)            = default;
        Exception& operator=(const Exception&) = default;
        Exception(Exception&&)                 = default;
        Exception& operator=(Exception&&)      = default;

        ~Exception() override;

        [[nodiscard]] const char* what() const noexcept override;

        [[nodiscard]] const Error& GetError() const noexcept { return m_Error; }

      private:
        Error m_Error;
    };

    [[noreturn]] AU_CORE_API void Throw(Error error);

}  // namespace Au
