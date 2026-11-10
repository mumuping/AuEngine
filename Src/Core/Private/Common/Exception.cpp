#include "Core/Common/Exception.h"

namespace Au {

    Exception::Exception(Error error) noexcept : m_Error(std::move(error)) {
    }

    Exception::~Exception() = default;

    const char* Exception::what() const noexcept {
        return m_Error.Message().c_str();
    }

    void Throw(Error error) {
        throw Exception(std::move(error));
    }

}  // namespace Au
