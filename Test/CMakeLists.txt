au_add_test(CoreTests
    SOURCES Core/Log/LogTests.cpp Core/Common/ErrorHandlingTests.cpp Core/FileSystem/FileSystemTests.cpp
            Core/Memory/MemoryTests.cpp
    DEPS    Au::Core
)
