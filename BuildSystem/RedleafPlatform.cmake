# BuildSystem/RedleafPlatform.cmake

# Platform defines
if(APPLE)
    add_compile_definitions(JENGINE_PLATFORM_MACOS=1)
elseif(WIN32)
    add_compile_definitions(JENGINE_PLATFORM_WINDOWS=1)
elseif(UNIX)
    add_compile_definitions(JENGINE_PLATFORM_LINUX=1)
endif()

# Compiler warnings for best portability across compilers
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    add_compile_options(
            -Wextra
            -Wpedantic
            -Werror=implicit-function-declaration
    )
elseif (MSVC)
    add_compile_options(
            /permissive-
            /EHsc
    )
endif()