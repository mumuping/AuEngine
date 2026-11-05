include_guard(GLOBAL)

include(GenerateExportHeader)

set(AU_SOURCE_GLOB_PATTERNS *.h *.hpp *.inl *.cpp *.c)

# Collects sources under <dir>/Public and <dir>/Private. CONFIGURE_DEPENDS re-runs the glob on every
# build, so new files are picked up without touching CMakeLists.txt.
function(_au_collect_sources out_var dir)
    set(_patterns "")
    foreach(_sub IN ITEMS Public Private)
        foreach(_pattern IN LISTS AU_SOURCE_GLOB_PATTERNS)
            list(APPEND _patterns "${dir}/${_sub}/${_pattern}")
        endforeach()
    endforeach()
    file(GLOB_RECURSE _sources CONFIGURE_DEPENDS ${_patterns})
    set(${out_var} ${_sources} PARENT_SCOPE)
endfunction()

function(_au_resolve_sources out_var)
    if(ARGN)
        set(${out_var} ${ARGN} PARENT_SCOPE)
    else()
        _au_collect_sources(_sources "${CMAKE_CURRENT_SOURCE_DIR}")
        if(NOT _sources)
            message(FATAL_ERROR "AuEngine: no sources found under ${CMAKE_CURRENT_SOURCE_DIR}/{Public,Private}")
        endif()
        set(${out_var} ${_sources} PARENT_SCOPE)
    endif()
endfunction()

function(_au_setup_ide target folder)
    get_target_property(_sources ${target} SOURCES)
    set(_local_sources "")
    foreach(_source IN LISTS _sources)
        cmake_path(ABSOLUTE_PATH _source BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE _abs)
        cmake_path(IS_PREFIX CMAKE_CURRENT_SOURCE_DIR "${_abs}" NORMALIZE _is_local)
        if(_is_local)
            list(APPEND _local_sources "${_abs}")
        endif()
    endforeach()
    if(_local_sources)
        source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${_local_sources})
    endif()
    set_target_properties(${target} PROPERTIES FOLDER "${folder}")
endfunction()

# Copies every runtime DLL the target depends on next to it (Windows only).
function(_au_copy_runtime_dlls target)
    if(NOT WIN32)
        return()
    endif()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "$<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:${target}>>,${CMAKE_COMMAND};-E;copy_if_different;$<TARGET_RUNTIME_DLLS:${target}>;$<TARGET_FILE_DIR:${target}>,${CMAKE_COMMAND};-E;true>"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endfunction()

# au_add_module(<Name>
#     [SOURCES ...]          # default: everything under Public/ and Private/
#     [PUBLIC_DEPS ...]
#     [PRIVATE_DEPS ...]
#     [PUBLIC_DEFINES ...]
#     [PRIVATE_DEFINES ...]
#     [FOLDER <ide folder>]  # default: Engine
# )
#
# Creates target Au<Name> with alias Au::<Name>. Static or shared follows AU_BUILD_SHARED.
# Generates <Name>/<Name>API.h providing AU_<NAME>_API for symbol export.
function(au_add_module name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG
        ""
        "FOLDER"
        "SOURCES;PUBLIC_DEPS;PRIVATE_DEPS;PUBLIC_DEFINES;PRIVATE_DEFINES"
    )
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "au_add_module(${name}): unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()

    set(target Au${name})
    string(TOUPPER "${name}" upper_name)
    set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/Generated")

    _au_resolve_sources(sources ${ARG_SOURCES})

    add_library(${target} ${sources})
    add_library(Au::${name} ALIAS ${target})

    generate_export_header(${target}
        BASE_NAME          AU_${upper_name}
        EXPORT_MACRO_NAME  AU_${upper_name}_API
        STATIC_DEFINE      AU_${upper_name}_STATIC_DEFINE
        EXPORT_FILE_NAME   "${generated_dir}/${name}/${name}API.h"
    )
    target_sources(${target} PRIVATE "${generated_dir}/${name}/${name}API.h")

    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "STATIC_LIBRARY")
        target_compile_definitions(${target} PUBLIC AU_${upper_name}_STATIC_DEFINE)
    endif()

    target_include_directories(${target}
        PUBLIC
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>"
            "$<BUILD_INTERFACE:${generated_dir}>"
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/Private"
    )

    target_link_libraries(${target}
        PUBLIC  ${ARG_PUBLIC_DEPS}
        PRIVATE ${ARG_PRIVATE_DEPS}
    )
    target_compile_definitions(${target}
        PUBLIC  ${ARG_PUBLIC_DEFINES}
        PRIVATE ${ARG_PRIVATE_DEFINES}
    )

    set_target_properties(${target} PROPERTIES
        CXX_VISIBILITY_PRESET     hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    au_apply_compiler_settings(${target})

    if(NOT ARG_FOLDER)
        set(ARG_FOLDER "Engine")
    endif()
    _au_setup_ide(${target} "${ARG_FOLDER}")
endfunction()

# au_add_executable(<Name>
#     [SOURCES ...]          # default: everything under Public/ and Private/
#     [PRIVATE_DEPS ...]
#     [PRIVATE_DEFINES ...]
#     [FOLDER <ide folder>]  # default: Programs
#     [WIN32]                # GUI subsystem on Windows (no console window)
# )
function(au_add_executable name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG
        "WIN32"
        "FOLDER"
        "SOURCES;PRIVATE_DEPS;PRIVATE_DEFINES"
    )
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "au_add_executable(${name}): unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()

    _au_resolve_sources(sources ${ARG_SOURCES})

    if(ARG_WIN32)
        add_executable(${name} WIN32 ${sources})
    else()
        add_executable(${name} ${sources})
    endif()

    target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")
    target_link_libraries(${name} PRIVATE ${ARG_PRIVATE_DEPS})
    target_compile_definitions(${name} PRIVATE ${ARG_PRIVATE_DEFINES})

    set_target_properties(${name} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${name}>"
    )

    au_apply_compiler_settings(${name})
    _au_copy_runtime_dlls(${name})
    au_copy_sanitizer_runtime(${name})
    add_dependencies(${name} AuCopyContent)

    if(NOT ARG_FOLDER)
        set(ARG_FOLDER "Programs")
    endif()
    _au_setup_ide(${name} "${ARG_FOLDER}")
endfunction()

# au_add_test(<Name>
#     SOURCES ...
#     [DEPS ...]
#     [DEFINES ...]
#     [FOLDER <ide folder>]  # default: Tests
# )
#
# Links GoogleTest (with gtest_main) and registers every test case with CTest.
function(au_add_test name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG
        ""
        "FOLDER"
        "SOURCES;DEPS;DEFINES"
    )
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "au_add_test(${name}): unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "au_add_test(${name}): SOURCES is required")
    endif()

    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE ${ARG_DEPS} GTest::gtest GTest::gtest_main)
    target_compile_definitions(${name} PRIVATE
        ${ARG_DEFINES}
        "AU_TEST_DATA_DIR=\"${AU_TEST_DATA_DIR}\""
    )

    au_apply_compiler_settings(${name})
    _au_copy_runtime_dlls(${name})
    au_copy_sanitizer_runtime(${name})
    add_dependencies(${name} AuCopyContent)

    set_target_properties(${name} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${name}>"
    )

    # PRE_TEST: test discovery runs at ctest time instead of as a post-build step, so a build never
    # fails just because a DLL cannot be located yet.
    gtest_discover_tests(${name}
        DISCOVERY_MODE    PRE_TEST
        WORKING_DIRECTORY "${AU_RUNTIME_OUTPUT_DIR}"
    )

    if(NOT ARG_FOLDER)
        set(ARG_FOLDER "Tests")
    endif()
    _au_setup_ide(${name} "${ARG_FOLDER}")
endfunction()
