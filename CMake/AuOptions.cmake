# Included before project(): platform variables such as WIN32 are not available yet,
# so host checks are used for defaults.
include_guard(GLOBAL)

option(AU_BUILD_SHARED        "Build engine modules as shared libraries"        OFF)
option(AU_BUILD_TESTS         "Build unit tests"                                 ON)
option(AU_BUILD_EDITOR        "Build the editor executable"                      ON)

option(AU_RHI_VULKAN          "Build the Vulkan RHI backend"                     ON)
if(CMAKE_HOST_WIN32)
    option(AU_RHI_D3D12       "Build the D3D12 RHI backend"                      ON)
else()
    option(AU_RHI_D3D12       "Build the D3D12 RHI backend"                      OFF)
endif()

option(AU_WARNINGS_AS_ERRORS  "Treat compiler warnings as errors"                OFF)
option(AU_ENABLE_LTO          "Enable link-time optimization in Release"         OFF)
option(AU_ENABLE_ASAN         "Enable AddressSanitizer"                          OFF)
option(AU_ENABLE_UBSAN        "Enable UndefinedBehaviorSanitizer (GCC/Clang)"    OFF)
option(AU_ENABLE_CLANG_TIDY   "Run clang-tidy while compiling"                   ON)
option(AU_ENABLE_CCACHE       "Use ccache/sccache as compiler launcher"          OFF)
