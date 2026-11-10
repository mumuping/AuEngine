#pragma once

#include "Core/Common/Error.h"
#include "Core/Common/Exception.h"

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace Au {

    struct UnexpectT {
        explicit UnexpectT() = default;
    };

    inline constexpr UnexpectT g_Unexpect{};

    template <typename E>
    class Unexpected {
      public:
        static_assert(!std::is_same_v<E, void>, "Unexpected<void> is not supported");

        constexpr explicit Unexpected(const E& error) : m_Error(error) {}

        constexpr explicit Unexpected(E&& error) : m_Error(std::move(error)) {}

        template <typename... Args>
        constexpr explicit Unexpected(std::in_place_t /*unused*/, Args&&... args)
            : m_Error(std::forward<Args>(args)...) {}

        // NOLINTBEGIN(readability-identifier-naming)
        [[nodiscard]] constexpr E& error() & noexcept { return m_Error; }

        [[nodiscard]] constexpr const E& error() const& noexcept { return m_Error; }

        [[nodiscard]] constexpr E&& error() && noexcept { return std::move(m_Error); }

        [[nodiscard]] constexpr const E&& error() const&& noexcept { return std::move(m_Error); }

        // NOLINTEND(readability-identifier-naming)

      private:
        E m_Error;
    };

    template <typename E>
    Unexpected(E) -> Unexpected<E>;

    namespace Detail {

        template <typename E>
        [[noreturn]] inline void ThrowExpectedError(E&& error) {
            if constexpr (std::is_same_v<std::decay_t<E>, Error>) {
                Throw(std::forward<E>(error));
            } else {
                Throw(Error::Make("bad Expected access"));
            }
        }

    }  // namespace Detail

    // API names intentionally mirror std::expected (has_value / value / error / value_or).
    // NOLINTBEGIN(readability-identifier-naming)
    template <typename T, typename E = Error>
    class Expected {
      public:
        static_assert(!std::is_same_v<T, UnexpectT>, "T must not be UnexpectT");
        static_assert(!std::is_same_v<T, std::in_place_t>, "T must not be in_place_t");
        static_assert(!std::is_reference_v<T>, "Expected does not support reference types");
        static_assert(!std::is_reference_v<E>, "Expected does not support reference error types");

        using value_type = T;
        using error_type = E;

        constexpr Expected()
            requires std::is_default_constructible_v<T>
            : m_HasValue(true) {
            new (ValuePtr()) T();
        }

        constexpr Expected(const Expected& other) : m_HasValue(other.m_HasValue) {
            if (m_HasValue) {
                new (ValuePtr()) T(*other.ValuePtr());
            } else {
                new (ErrorPtr()) E(*other.ErrorPtr());
            }
        }

        constexpr Expected(
            Expected&& other
        ) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_constructible_v<E>)
            : m_HasValue(other.m_HasValue) {
            if (m_HasValue) {
                new (ValuePtr()) T(std::move(*other.ValuePtr()));
            } else {
                new (ErrorPtr()) E(std::move(*other.ErrorPtr()));
            }
        }

        template <typename U = T>
            requires(
                !std::is_same_v<std::decay_t<U>, Expected> && !std::is_same_v<std::decay_t<U>, Unexpected<E>> &&
                !std::is_same_v<std::decay_t<U>, std::in_place_t> && std::is_constructible_v<T, U>
            )
        constexpr explicit(!std::is_convertible_v<U, T>) Expected(U&& value) : m_HasValue(true) {
            new (ValuePtr()) T(std::forward<U>(value));
        }

        template <typename... Args>
            requires std::is_constructible_v<T, Args...>
        constexpr explicit Expected(std::in_place_t /*unused*/, Args&&... args) : m_HasValue(true) {
            new (ValuePtr()) T(std::forward<Args>(args)...);
        }

        template <typename G = E>
            requires std::is_constructible_v<E, const G&>
        constexpr explicit(!std::is_convertible_v<const G&, E>) Expected(const Unexpected<G>& unexpected) {
            new (ErrorPtr()) E(unexpected.error());
        }

        template <typename G = E>
            requires std::is_constructible_v<E, G>
        constexpr explicit(!std::is_convertible_v<G, E>) Expected(Unexpected<G>&& unexpected) {
            new (ErrorPtr()) E(std::move(unexpected).error());
        }

        template <typename... Args>
            requires std::is_constructible_v<E, Args...>
        constexpr explicit Expected(UnexpectT /*unused*/, Args&&... args) {
            new (ErrorPtr()) E(std::forward<Args>(args)...);
        }

        ~Expected() { Destroy(); }

        Expected& operator=(const Expected& other) {
            if (this == &other) {
                return *this;
            }
            if (m_HasValue && other.m_HasValue) {
                *ValuePtr() = *other.ValuePtr();
            } else if (!m_HasValue && !other.m_HasValue) {
                *ErrorPtr() = *other.ErrorPtr();
            } else {
                Destroy();
                m_HasValue = other.m_HasValue;
                if (m_HasValue) {
                    new (ValuePtr()) T(*other.ValuePtr());
                } else {
                    new (ErrorPtr()) E(*other.ErrorPtr());
                }
            }
            return *this;
        }

        Expected& operator=(Expected&& other) noexcept(
            std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T> &&
            std::is_nothrow_move_assignable_v<E> && std::is_nothrow_move_constructible_v<E>
        ) {
            if (this == &other) {
                return *this;
            }
            if (m_HasValue && other.m_HasValue) {
                *ValuePtr() = std::move(*other.ValuePtr());
            } else if (!m_HasValue && !other.m_HasValue) {
                *ErrorPtr() = std::move(*other.ErrorPtr());
            } else {
                Destroy();
                m_HasValue = other.m_HasValue;
                if (m_HasValue) {
                    new (ValuePtr()) T(std::move(*other.ValuePtr()));
                } else {
                    new (ErrorPtr()) E(std::move(*other.ErrorPtr()));
                }
            }
            return *this;
        }

        [[nodiscard]] constexpr bool has_value() const noexcept { return m_HasValue; }

        [[nodiscard]] constexpr explicit operator bool() const noexcept { return m_HasValue; }

        [[nodiscard]] constexpr T* operator->() noexcept { return ValuePtr(); }

        [[nodiscard]] constexpr const T* operator->() const noexcept { return ValuePtr(); }

        [[nodiscard]] constexpr T& operator*() & noexcept { return *ValuePtr(); }

        [[nodiscard]] constexpr const T& operator*() const& noexcept { return *ValuePtr(); }

        [[nodiscard]] constexpr T&& operator*() && noexcept { return std::move(*ValuePtr()); }

        [[nodiscard]] constexpr const T&& operator*() const&& noexcept { return std::move(*ValuePtr()); }

        [[nodiscard]] constexpr T& value() & {
            if (!m_HasValue) {
                Detail::ThrowExpectedError(*ErrorPtr());
            }
            return *ValuePtr();
        }

        [[nodiscard]] constexpr const T& value() const& {
            if (!m_HasValue) {
                Detail::ThrowExpectedError(*ErrorPtr());
            }
            return *ValuePtr();
        }

        [[nodiscard]] constexpr T&& value() && {
            if (!m_HasValue) {
                Detail::ThrowExpectedError(std::move(*ErrorPtr()));
            }
            return std::move(*ValuePtr());
        }

        [[nodiscard]] constexpr const T&& value() const&& {
            if (!m_HasValue) {
                Detail::ThrowExpectedError(std::move(*ErrorPtr()));
            }
            return std::move(*ValuePtr());
        }

        [[nodiscard]] constexpr E& error() & noexcept { return *ErrorPtr(); }

        [[nodiscard]] constexpr const E& error() const& noexcept { return *ErrorPtr(); }

        [[nodiscard]] constexpr E&& error() && noexcept { return std::move(*ErrorPtr()); }

        [[nodiscard]] constexpr const E&& error() const&& noexcept { return std::move(*ErrorPtr()); }

        template <typename U>
        [[nodiscard]] constexpr T value_or(U&& defaultValue) const& {
            return m_HasValue ? *ValuePtr() : static_cast<T>(std::forward<U>(defaultValue));
        }

        template <typename U>
        [[nodiscard]] constexpr T value_or(U&& defaultValue) && {
            return m_HasValue ? std::move(*ValuePtr()) : static_cast<T>(std::forward<U>(defaultValue));
        }

      private:
        void Destroy() {
            if (m_HasValue) {
                ValuePtr()->~T();
            } else {
                ErrorPtr()->~E();
            }
        }

        [[nodiscard]] T* ValuePtr() noexcept { return reinterpret_cast<T*>(m_Storage); }

        [[nodiscard]] const T* ValuePtr() const noexcept { return reinterpret_cast<const T*>(m_Storage); }

        [[nodiscard]] E* ErrorPtr() noexcept { return reinterpret_cast<E*>(m_Storage); }

        [[nodiscard]] const E* ErrorPtr() const noexcept { return reinterpret_cast<const E*>(m_Storage); }

        static constexpr std::size_t s_StorageSize  = (std::max)(sizeof(T), sizeof(E));
        static constexpr std::size_t s_StorageAlign = (std::max)(alignof(T), alignof(E));

        alignas(s_StorageAlign) unsigned char m_Storage[s_StorageSize]{};
        bool m_HasValue = false;
    };

    template <typename E>
    class Expected<void, E> {
      public:
        static_assert(!std::is_reference_v<E>, "Expected does not support reference error types");

        using value_type = void;
        using error_type = E;

        constexpr Expected() noexcept : m_HasValue(true) {}

        constexpr Expected(const Expected& other) : m_HasValue(other.m_HasValue) {
            if (!m_HasValue) {
                new (ErrorPtr()) E(*other.ErrorPtr());
            }
        }

        constexpr Expected(Expected&& other) noexcept(std::is_nothrow_move_constructible_v<E>)
            : m_HasValue(other.m_HasValue) {
            if (!m_HasValue) {
                new (ErrorPtr()) E(std::move(*other.ErrorPtr()));
            }
        }

        constexpr explicit Expected(std::in_place_t /*unused*/) noexcept : m_HasValue(true) {}

        template <typename G = E>
            requires std::is_constructible_v<E, const G&>
        constexpr explicit(!std::is_convertible_v<const G&, E>) Expected(const Unexpected<G>& unexpected) {
            new (ErrorPtr()) E(unexpected.error());
        }

        template <typename G = E>
            requires std::is_constructible_v<E, G>
        constexpr explicit(!std::is_convertible_v<G, E>) Expected(Unexpected<G>&& unexpected) {
            new (ErrorPtr()) E(std::move(unexpected).error());
        }

        template <typename... Args>
            requires std::is_constructible_v<E, Args...>
        constexpr explicit Expected(UnexpectT /*unused*/, Args&&... args) {
            new (ErrorPtr()) E(std::forward<Args>(args)...);
        }

        ~Expected() { Destroy(); }

        Expected& operator=(const Expected& other) {
            if (this == &other) {
                return *this;
            }
            if (!m_HasValue && !other.m_HasValue) {
                *ErrorPtr() = *other.ErrorPtr();
            } else {
                Destroy();
                m_HasValue = other.m_HasValue;
                if (!m_HasValue) {
                    new (ErrorPtr()) E(*other.ErrorPtr());
                }
            }
            return *this;
        }

        Expected& operator=(
            Expected&& other
        ) noexcept(std::is_nothrow_move_assignable_v<E> && std::is_nothrow_move_constructible_v<E>) {
            if (this == &other) {
                return *this;
            }
            if (!m_HasValue && !other.m_HasValue) {
                *ErrorPtr() = std::move(*other.ErrorPtr());
            } else {
                Destroy();
                m_HasValue = other.m_HasValue;
                if (!m_HasValue) {
                    new (ErrorPtr()) E(std::move(*other.ErrorPtr()));
                }
            }
            return *this;
        }

        [[nodiscard]] constexpr bool has_value() const noexcept { return m_HasValue; }

        [[nodiscard]] constexpr explicit operator bool() const noexcept { return m_HasValue; }

        constexpr void operator*() const noexcept {}

        constexpr void value() const {
            if (!m_HasValue) {
                Detail::ThrowExpectedError(*ErrorPtr());
            }
        }

        [[nodiscard]] constexpr E& error() & noexcept { return *ErrorPtr(); }

        [[nodiscard]] constexpr const E& error() const& noexcept { return *ErrorPtr(); }

        [[nodiscard]] constexpr E&& error() && noexcept { return std::move(*ErrorPtr()); }

        [[nodiscard]] constexpr const E&& error() const&& noexcept { return std::move(*ErrorPtr()); }

      private:
        void Destroy() {
            if (!m_HasValue) {
                ErrorPtr()->~E();
            }
        }

        [[nodiscard]] E* ErrorPtr() noexcept { return reinterpret_cast<E*>(m_Storage); }

        [[nodiscard]] const E* ErrorPtr() const noexcept { return reinterpret_cast<const E*>(m_Storage); }

        alignas(E) unsigned char m_Storage[sizeof(E)]{};
        bool m_HasValue = false;
    };

    // NOLINTEND(readability-identifier-naming)

    template <typename T>
    using Result = Expected<T, Error>;

    using Status = Expected<void, Error>;

}  // namespace Au
