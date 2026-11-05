include_guard(GLOBAL)

if(AU_ENABLE_ASAN AND AU_COMPILER_CLANG_CL)
    message(FATAL_ERROR "AuEngine: AU_ENABLE_ASAN is not wired up for clang-cl yet, use the msvc preset instead")
endif()

if(AU_ENABLE_UBSAN AND AU_MSVC_FRONTEND)
    message(WARNING "AuEngine: UBSan is not supported by the MSVC frontend, ignoring AU_ENABLE_UBSAN")
    set(AU_ENABLE_UBSAN OFF)
endif()

if(AU_ENABLE_ASAN AND AU_COMPILER_MSVC)
    # /RTC is incompatible with /fsanitize=address.
    foreach(_au_flags_var CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG)
        string(REGEX REPLACE "/RTC[1csu]+" "" ${_au_flags_var} "${${_au_flags_var}}")
    endforeach()

    # The ASan runtime DLL lives next to cl.exe and must sit next to every executable.
    get_filename_component(_au_cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    find_file(AU_ASAN_RUNTIME_DLL clang_rt.asan_dynamic-x86_64.dll HINTS "${_au_cl_dir}" NO_DEFAULT_PATH)
    if(NOT AU_ASAN_RUNTIME_DLL)
        message(WARNING "AuEngine: clang_rt.asan_dynamic-x86_64.dll not found next to cl.exe")
    endif()
endif()

function(au_apply_sanitizers target)
    if(AU_ENABLE_ASAN)
        if(AU_COMPILER_MSVC)
            target_compile_options(${target} PRIVATE /fsanitize=address)
            target_link_options(${target} PRIVATE /INCREMENTAL:NO)
        elseif(AU_COMPILER_GCC OR AU_COMPILER_CLANG)
            target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address)
        endif()
    endif()

    if(AU_ENABLE_UBSAN AND (AU_COMPILER_GCC OR AU_COMPILER_CLANG))
        target_compile_options(${target} PRIVATE -fsanitize=undefined -fno-sanitize-recover=undefined)
        target_link_options(${target} PRIVATE -fsanitize=undefined)
    endif()
endfunction()

# Called for executables only.
function(au_copy_sanitizer_runtime target)
    if(AU_ENABLE_ASAN AND AU_COMPILER_MSVC AND AU_ASAN_RUNTIME_DLL)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${AU_ASAN_RUNTIME_DLL}" "$<TARGET_FILE_DIR:${target}>"
            VERBATIM
        )
    endif()
endfunction()
