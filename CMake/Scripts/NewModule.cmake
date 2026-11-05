# Creates a new engine module (or executable) from the templates in Templates/ and registers it
# with the build. Run from anywhere:
#
#   cmake -DNAME=Physics -P CMake/Scripts/NewModule.cmake
#   cmake -DNAME=Physics -DDEPS="Au::Core;Au::Asset" -P CMake/Scripts/NewModule.cmake
#   cmake -DNAME=Metal -DPARENT=RHI -P CMake/Scripts/NewModule.cmake      # Src/RHI/Metal, target AuRHIMetal
#   cmake -DNAME=Sandbox -DTYPE=exe -P CMake/Scripts/NewModule.cmake      # Src/Sandbox, target AuSandbox
#
# Options:
#   NAME       PascalCase name (required)
#   PARENT     existing group directory under Src/ to nest into (e.g. RHI); the module is named <PARENT><NAME>
#   TYPE       lib (default) or exe
#   DEPS       ;-separated link dependencies. Default: Au::<PARENT> or Au::Core for lib, Au::Application for exe
#   NAMESPACE  C++ namespace for the generated code (default: Au)
#   NO_TEST    ON to skip generating a unit test (libraries only)
cmake_minimum_required(VERSION 3.28)

get_filename_component(AU_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(AU_TEMPLATE_DIR "${CMAKE_CURRENT_LIST_DIR}/Templates")

# ---- Validate arguments ---------------------------------------------------------------------------
if(NOT NAME)
    message(FATAL_ERROR "NAME is required, e.g. cmake -DNAME=Physics -P CMake/Scripts/NewModule.cmake")
endif()
if(NOT NAME MATCHES "^[A-Z][A-Za-z0-9]*$")
    message(FATAL_ERROR "NAME must be PascalCase (letters and digits, starting with an uppercase letter): '${NAME}'")
endif()

if(NOT TYPE)
    set(TYPE lib)
endif()
if(NOT TYPE MATCHES "^(lib|exe)$")
    message(FATAL_ERROR "TYPE must be 'lib' or 'exe', got '${TYPE}'")
endif()

if(NOT NAMESPACE)
    set(NAMESPACE Au)
endif()

if(PARENT)
    if(NOT PARENT MATCHES "^[A-Z][A-Za-z0-9]*$")
        message(FATAL_ERROR "PARENT must be PascalCase: '${PARENT}'")
    endif()
    set(parent_dir "${AU_ROOT}/Src/${PARENT}")
    if(NOT EXISTS "${parent_dir}/CMakeLists.txt")
        message(FATAL_ERROR "PARENT directory '${PARENT}' with a CMakeLists.txt not found at ${parent_dir}")
    endif()
else()
    set(parent_dir "${AU_ROOT}/Src")
endif()

set(module_dir "${parent_dir}/${NAME}")
if(EXISTS "${module_dir}")
    message(FATAL_ERROR "${module_dir} already exists, refusing to overwrite it")
endif()

# ---- Template variables ---------------------------------------------------------------------------
set(AU_MODULE_NAME "${PARENT}${NAME}")
string(TOUPPER "${AU_MODULE_NAME}" AU_MODULE_UPPER)
set(AU_MODULE_EXPORT "AU_${AU_MODULE_UPPER}_API")
set(AU_NAMESPACE "${NAMESPACE}")
set(AU_TARGET_NAME "Au${AU_MODULE_NAME}")

if(PARENT)
    set(AU_FOLDER "Engine/${PARENT}")
else()
    set(AU_FOLDER "Engine")
endif()

if(NOT DEPS)
    if(TYPE STREQUAL "exe")
        set(DEPS Au::Application)
    elseif(PARENT)
        set(DEPS Au::${PARENT})
    else()
        set(DEPS Au::Core)
    endif()
endif()
string(REPLACE ";" " " AU_MODULE_DEPS "${DEPS}")

function(au_generate template output)
    configure_file("${AU_TEMPLATE_DIR}/${template}" "${output}" @ONLY NEWLINE_STYLE UNIX)
    file(RELATIVE_PATH relative "${AU_ROOT}" "${output}")
    message(STATUS "  created  ${relative}")
endfunction()

# Appends <line> to <file> unless it is already present.
function(au_append_line file line)
    file(READ "${file}" content)
    string(FIND "${content}" "${line}" found)
    if(NOT found EQUAL -1)
        return()
    endif()
    if(content AND NOT content MATCHES "\n$")
        file(APPEND "${file}" "\n")
    endif()
    file(APPEND "${file}" "${line}\n")
    file(RELATIVE_PATH relative "${AU_ROOT}" "${file}")
    message(STATUS "  updated  ${relative}")
endfunction()

# ---- Generate -------------------------------------------------------------------------------------
message(STATUS "Creating ${TYPE} ${AU_TARGET_NAME} in namespace ${AU_NAMESPACE}")

if(TYPE STREQUAL "lib")
    au_generate(Module.CMakeLists.txt.in "${module_dir}/CMakeLists.txt")
    au_generate(Module.h.in              "${module_dir}/Public/${AU_MODULE_NAME}/${AU_MODULE_NAME}Module.h")
    au_generate(Module.cpp.in            "${module_dir}/Private/${AU_MODULE_NAME}Module.cpp")
else()
    au_generate(Executable.CMakeLists.txt.in "${module_dir}/CMakeLists.txt")
    au_generate(Main.cpp.in                  "${module_dir}/Private/Main.cpp")
endif()

au_append_line("${parent_dir}/CMakeLists.txt" "add_subdirectory(${NAME})")

if(TYPE STREQUAL "lib" AND NOT NO_TEST)
    au_generate(ModuleTests.cpp.in "${AU_ROOT}/Test/${AU_MODULE_NAME}/${AU_MODULE_NAME}Tests.cpp")
    au_append_line("${AU_ROOT}/Test/CMakeLists.txt"
        "\nau_add_test(${AU_MODULE_NAME}Tests\n    SOURCES ${AU_MODULE_NAME}/${AU_MODULE_NAME}Tests.cpp\n    DEPS    Au::${AU_MODULE_NAME}\n)")
endif()

message(STATUS "Done. Re-run the configure step (e.g. cmake --preset msvc) to pick up ${AU_TARGET_NAME}.")
