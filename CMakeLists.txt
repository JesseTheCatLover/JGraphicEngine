cmake_minimum_required(VERSION 3.15)
project(JGraphicEngine)
include(FetchContent)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(APPLE)
    add_compile_definitions(JENGINE_PLATFORM_MACOS=1)
elseif(WIN32)
    add_compile_definitions(JENGINE_PLATFORM_WINDOWS=1)
elseif(UNIX)
    add_compile_definitions(JENGINE_PLATFORM_LINUX=1)
endif()

# Fetch Dependencies
FetchContent_Declare(
        glfw
        GIT_TAG 3.4
        GIT_SHALLOW TRUE
        GIT_REPOSITORY https://github.com/glfw/glfw.git
)

FetchContent_Declare(
        assimp
        GIT_TAG v5.4.3
        GIT_SHALLOW TRUE
        GIT_REPOSITORY https://github.com/assimp/assimp.git
)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "Treat warnings as errors" FORCE)

FetchContent_Declare(
        glm
        GIT_TAG CMakeVersionFix
        GIT_SHALLOW TRUE
        GIT_REPOSITORY https://github.com/JesseTheCatLover/glm.git
)

FetchContent_Declare(
        json
        GIT_TAG v3.11.3
        GIT_REPOSITORY https://github.com/nlohmann/json.git
)

FetchContent_MakeAvailable(glfw assimp glm json)

# Add submodules
# --- ThirdParty libraries ---
add_subdirectory(ThirdParty)
# --- Engine modules
add_subdirectory(Source/Engine)
add_subdirectory(Source/EditorRuntime)
add_subdirectory(Source/Editor)
add_subdirectory(Source/Launch) # Executable is defined in this module

# --- Engine directory macro ---
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Engine PUBLIC ENGINE_DIRECTORY="${CMAKE_SOURCE_DIR}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(Engine PUBLIC ENGINE_DIRECTORY="${CMAKE_SOURCE_DIR}")
endif()

# Engine internals folder
set_target_properties(JApplication Engine EditorRuntime PROPERTIES
        FOLDER "EngineLibs"
        EXCLUDE_FROM_ALL TRUE
)

# Compiler warnings for best portability across compilers
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    target_compile_options(Engine PRIVATE
            -Wextra            # Enable extra warnings not included in -Wall
            -Wpedantic         # Enforce strict ISO C++ compliance
            -Werror=implicit-function-declaration  # Catch missing includes / transitive includes
    )
elseif (MSVC)
    # MSVC equivalent flags (to enforce similar checks on Windows)
    target_compile_options(Engine PRIVATE
        /permissive-  # Enforce standard-compliant C++
        /EHsc         # Standard exception handling
        )
endif()