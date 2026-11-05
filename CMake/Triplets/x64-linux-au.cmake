set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

# Already static on Linux; kept explicit so both platforms state the same mimalloc policy as
# x64-windows-au.cmake (static library, no malloc override).
if(PORT STREQUAL "mimalloc")
    set(VCPKG_LIBRARY_LINKAGE static)
endif()
