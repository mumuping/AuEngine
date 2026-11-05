set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# The mimalloc DLL always loads mimalloc-redirect.dll and tries to replace the CRT malloc, which fails
# (and prints errors) when it is loaded through an engine DLL. The static library never redirects, so
# mimalloc is only used through explicit Au::Memory calls.
if(PORT STREQUAL "mimalloc")
    set(VCPKG_LIBRARY_LINKAGE static)
endif()
