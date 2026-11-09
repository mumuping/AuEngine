#pragma once

// General-purpose helper macros.

// ---- Token pasting / stringizing -----------------------------------------------------------------

#define AU_STRINGIFY_IMPL(x) #x
#define AU_STRINGIFY(x) AU_STRINGIFY_IMPL(x)

#define AU_CONCAT_IMPL(a, b) a##b
#define AU_CONCAT(a, b) AU_CONCAT_IMPL(a, b)

// Unique identifier for macro-generated locals (e.g. scope guards).
#define AU_UNIQUE_NAME(prefix) AU_CONCAT(prefix, __COUNTER__)

// ---- Special members (when NonCopyable inheritance is undesirable) -------------------------------

// NOLINTBEGIN(bugprone-macro-parentheses)
#define AU_DISABLE_COPY(TypeName)                  \
    TypeName(const TypeName&)            = delete; \
    TypeName& operator=(const TypeName&) = delete

#define AU_DISABLE_MOVE(TypeName)             \
    TypeName(TypeName&&)            = delete; \
    TypeName& operator=(TypeName&&) = delete
// NOLINTEND(bugprone-macro-parentheses)

#define AU_DISABLE_COPY_MOVE(TypeName) \
    AU_DISABLE_COPY(TypeName);         \
    AU_DISABLE_MOVE(TypeName)

// ---- Attributes / codegen hints ------------------------------------------------------------------

#ifdef _MSC_VER
    #define AU_FORCEINLINE __forceinline
    #define AU_NOINLINE __declspec(noinline)
    #define AU_RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
    #define AU_FORCEINLINE inline __attribute__((always_inline))
    #define AU_NOINLINE __attribute__((noinline))
    #define AU_RESTRICT __restrict__
#else
    #define AU_FORCEINLINE inline
    #define AU_NOINLINE
    #define AU_RESTRICT
#endif

// Branch hints. Prefer C++20 [[likely]] / [[unlikely]] on statements when writing plain code;
// these wrap builtins for expression-style use: if (AU_LIKELY(cond)).
#if defined(__clang__) || defined(__GNUC__)
    #define AU_LIKELY(x) (__builtin_expect(!!(x), 1))
    #define AU_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
    #define AU_LIKELY(x) (!!(x))
    #define AU_UNLIKELY(x) (!!(x))
#endif

#ifdef _MSC_VER
    #define AU_UNREACHABLE() __assume(0)
#elif defined(__GNUC__) || defined(__clang__)
    #define AU_UNREACHABLE() __builtin_unreachable()
#else
    #define AU_UNREACHABLE() ((void)0)
#endif

// ---- Misc utilities ------------------------------------------------------------------------------

#define AU_UNUSED(x) ((void)(x))

// Bit helper for flag enums / masks.
#define AU_BIT(n) (1u << (n))

// Extent of a C array. Prefer std::size / std::span for containers.
#define AU_ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
