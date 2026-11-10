#pragma once

#include "Core/Common/Assert.h"
#include "Core/Memory/MemoryTag.h"
#include "Core/Memory/NewDelete.h"

#include <cstddef>
#include <type_traits>
#include <utility>

namespace Au::Memory {

    template <typename T>
    class RefPtr {
        static_assert(
            requires(const T& object) {
                object.AddRef();
                object.Release();
                object.GetRefCount();
            }, "RefPtr<T> requires an intrusive refcount API (e.g. inherit RefCounter<T> or a refcounted base)"
        );

      public:
        using ElementType = T;

        constexpr RefPtr() noexcept = default;

        // NOLINTNEXTLINE(cppcoreguidelines-explicit-constructor,misc-explicit-constructor,google-explicit-constructor)
        constexpr RefPtr(std::nullptr_t) noexcept {}

        RefPtr(const RefPtr& other) noexcept : m_Ptr(other.m_Ptr) { AddRef(); }

        template <typename U>
            requires std::is_convertible_v<U*, T*>
        // NOLINTNEXTLINE(cppcoreguidelines-explicit-constructor,misc-explicit-constructor,google-explicit-constructor)
        RefPtr(const RefPtr<U>& other) noexcept : m_Ptr(other.Get()) {
            AddRef();
        }

        RefPtr(RefPtr&& other) noexcept : m_Ptr(other.m_Ptr) { other.m_Ptr = nullptr; }

        template <typename U>
            requires std::is_convertible_v<U*, T*>
        // NOLINTNEXTLINE(cppcoreguidelines-explicit-constructor,misc-explicit-constructor,google-explicit-constructor,cppcoreguidelines-rvalue-reference-param-not-moved)
        RefPtr(RefPtr<U>&& other) noexcept : m_Ptr(other.Detach()) {}

        ~RefPtr() { Release(); }

        RefPtr& operator=(const RefPtr& other) noexcept {
            if (this != &other) {
                Release();
                m_Ptr = other.m_Ptr;
                AddRef();
            }
            return *this;
        }

        template <typename U>
            requires std::is_convertible_v<U*, T*>
        RefPtr& operator=(const RefPtr<U>& other) noexcept {
            Release();
            m_Ptr = other.Get();
            AddRef();
            return *this;
        }

        RefPtr& operator=(RefPtr&& other) noexcept {
            if (this != &other) {
                Release();
                m_Ptr       = other.m_Ptr;
                other.m_Ptr = nullptr;
            }
            return *this;
        }

        template <typename U>
            requires std::is_convertible_v<U*, T*>
        // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
        RefPtr& operator=(RefPtr<U>&& other) noexcept {
            Release();
            m_Ptr = other.Detach();
            return *this;
        }

        RefPtr& operator=(std::nullptr_t) noexcept {
            Reset();
            return *this;
        }

        void Reset() noexcept {
            Release();
            m_Ptr = nullptr;
        }

        [[nodiscard]] T* Get() const noexcept { return m_Ptr; }

        [[nodiscard]] T& operator*() const noexcept {
            AU_ASSERT(m_Ptr != nullptr);
            return *m_Ptr;
        }

        [[nodiscard]] T* operator->() const noexcept {
            AU_ASSERT(m_Ptr != nullptr);
            return m_Ptr;
        }

        [[nodiscard]] explicit operator bool() const noexcept { return m_Ptr != nullptr; }

        friend void Swap(RefPtr& left, RefPtr& right) noexcept {
            using std::swap;
            swap(left.m_Ptr, right.m_Ptr);
        }

      private:
        template <typename U>
        friend class RefPtr;

        template <typename U, typename... Args>
        friend RefPtr<U> MakeRef(EMemoryTag tag, Args&&... args);

        // Only MakeRef may wrap a raw owning pointer.
        explicit RefPtr(T* ptr) noexcept : m_Ptr(ptr) { AddRef(); }

        // Used by converting move; not part of the public ownership API.
        [[nodiscard]] T* Detach() noexcept {
            T* const ptr = m_Ptr;
            m_Ptr        = nullptr;
            return ptr;
        }

        void AddRef() const noexcept {
            if (m_Ptr != nullptr) {
                m_Ptr->AddRef();
            }
        }

        void Release() const noexcept {
            if (m_Ptr != nullptr) {
                m_Ptr->Release();
            }
        }

        T* m_Ptr = nullptr;
    };

    template <typename T, typename U>
    [[nodiscard]] bool operator==(const RefPtr<T>& left, const RefPtr<U>& right) noexcept {
        return left.Get() == right.Get();
    }

    template <typename T, typename U>
    [[nodiscard]] bool operator!=(const RefPtr<T>& left, const RefPtr<U>& right) noexcept {
        return !(left == right);
    }

    template <typename T>
    [[nodiscard]] bool operator==(const RefPtr<T>& left, std::nullptr_t) noexcept {
        return left.Get() == nullptr;
    }

    template <typename T>
    [[nodiscard]] bool operator==(std::nullptr_t, const RefPtr<T>& right) noexcept {
        return right.Get() == nullptr;
    }

    template <typename T>
    [[nodiscard]] bool operator!=(const RefPtr<T>& left, std::nullptr_t) noexcept {
        return left.Get() != nullptr;
    }

    template <typename T>
    [[nodiscard]] bool operator!=(std::nullptr_t, const RefPtr<T>& right) noexcept {
        return right.Get() != nullptr;
    }

    template <typename T, typename... Args>
    [[nodiscard]] RefPtr<T> MakeRef(EMemoryTag tag, Args&&... args) {
        return RefPtr<T>(New<T>(tag, std::forward<Args>(args)...));
    }

    template <typename T, typename... Args>
        requires(!std::is_same_v<std::decay_t<Args>, EMemoryTag> && ...)
    [[nodiscard]] RefPtr<T> MakeRef(Args&&... args) {
        return MakeRef<T>(EMemoryTag::General, std::forward<Args>(args)...);
    }

}  // namespace Au::Memory

namespace Au {

    template <typename T>
    using RefPtr = Memory::RefPtr<T>;

    template <typename T, typename... Args>
    [[nodiscard]] Memory::RefPtr<T> MakeRef(Args&&... args) {
        return Memory::MakeRef<T>(std::forward<Args>(args)...);
    }

}  // namespace Au
