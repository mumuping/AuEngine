include_guard(GLOBAL)

# ---- Global settings (affect every target, including the ones defined below) ----------------------

# Requires policy CMP0141 (CMake >= 3.25). Embedded (/Z7) is needed for compiler caches.
if(AU_ENABLE_CCACHE)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
else()
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
endif()

if(AU_ENABLE_CCACHE)
    find_program(AU_CCACHE_PROGRAM NAMES sccache ccache)
    if(AU_CCACHE_PROGRAM)
        message(STATUS "AuEngine: compiler launcher ${AU_CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER   "${AU_CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${AU_CCACHE_PROGRAM}")
    else()
        message(WARNING "AuEngine: AU_ENABLE_CCACHE is ON but neither sccache nor ccache was found")
    endif()
endif()

if(AU_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _au_ipo_supported OUTPUT _au_ipo_output LANGUAGES CXX)
    if(_au_ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    else()
        message(WARNING "AuEngine: LTO not supported: ${_au_ipo_output}")
    endif()
endif()

if(AU_ENABLE_CLANG_TIDY)
    find_program(AU_CLANG_TIDY_PROGRAM NAMES clang-tidy)
    if(NOT AU_CLANG_TIDY_PROGRAM)
        message(WARNING "AuEngine: AU_ENABLE_CLANG_TIDY is ON but clang-tidy was not found")
    endif()
    set(AU_CLANG_TIDY_COMMAND "${AU_CLANG_TIDY_PROGRAM}")
    if(AU_MSVC_FRONTEND)
        # clang-tidy (LLVM 23) drops /EHsc from cl-style command lines and then rejects every try/catch.
        list(APPEND AU_CLANG_TIDY_COMMAND "--extra-arg=/EHsc")
    elseif(AU_COMPILER_GCC)
        # GCC-only warnings such as -Wduplicated-cond are unknown to clang-tidy's frontend.
        list(APPEND AU_CLANG_TIDY_COMMAND "--extra-arg=-Wno-unknown-warning-option")
    endif()
endif()

# ---- Per-target settings --------------------------------------------------------------------------

# Applied to every engine-owned target. Third-party targets are never touched, and their headers
# come in as SYSTEM includes, so the warning levels below do not apply to them.
function(au_apply_compiler_settings target)
    target_compile_features(${target} PUBLIC cxx_std_20)

    set_target_properties(${target} PROPERTIES
        COMPILE_WARNING_AS_ERROR ${AU_WARNINGS_AS_ERRORS}
    )

    if(AU_MSVC_FRONTEND)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /utf-8
            /Zc:__cplusplus
            /bigobj
            /EHsc
            # STL members of exported classes; harmless as long as every module uses the same CRT.
            /wd4251
            /wd4275
            "$<$<CONFIG:Release,RelWithDebInfo>:/Gy;/Oi>"
        )
        target_link_options(${target} PRIVATE
            "$<$<CONFIG:Release>:/OPT:REF;/OPT:ICF>"
        )
    endif()

    if(AU_COMPILER_MSVC)
        target_compile_options(${target} PRIVATE
            /Zc:preprocessor
            /Zc:inline
            /Zc:throwingNew
        )
        if(CMAKE_GENERATOR MATCHES "Visual Studio")
            target_compile_options(${target} PRIVATE /MP)
        endif()
    endif()

    if(AU_COMPILER_CLANG_CL)
        target_compile_options(${target} PRIVATE
            -Wshadow
            -Wnon-virtual-dtor
            -Woverloaded-virtual
            -Wnull-dereference
            -Wformat=2
            -Wno-pragma-pack
        )
    endif()

    if(AU_COMPILER_GCC OR AU_COMPILER_CLANG)
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Woverloaded-virtual
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
        )
    endif()

    if(AU_COMPILER_GCC)
        target_compile_options(${target} PRIVATE
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wuseless-cast
        )
    endif()

    if(AU_PLATFORM_WINDOWS)
        target_compile_definitions(${target} PRIVATE
            AU_PLATFORM_WINDOWS=1
            NOMINMAX
            WIN32_LEAN_AND_MEAN
            UNICODE
            _UNICODE
            _CRT_SECURE_NO_WARNINGS
        )
    elseif(AU_PLATFORM_LINUX)
        target_compile_definitions(${target} PRIVATE AU_PLATFORM_LINUX=1)
    endif()

    target_compile_definitions(${target} PRIVATE
        $<$<CONFIG:Debug>:AU_DEBUG=1>
        $<$<CONFIG:RelWithDebInfo>:AU_DEVELOPMENT=1>
        $<$<CONFIG:Release,MinSizeRel>:AU_RELEASE=1>
    )

    if(AU_ENABLE_CLANG_TIDY AND AU_CLANG_TIDY_PROGRAM)
        set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${AU_CLANG_TIDY_COMMAND}")
    endif()

    au_apply_sanitizers(${target})
endfunction()
