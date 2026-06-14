cmake_minimum_required(VERSION 3.15)
project(RedleafEngine VERSION 0.1 LANGUAGES C CXX)

set(REDLEAF_DOC_VERSION "Alpha")

include(FetchContent)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Platform defines
if(APPLE)
    add_compile_definitions(JENGINE_PLATFORM_MACOS=1)
elseif(WIN32)
    add_compile_definitions(JENGINE_PLATFORM_WINDOWS=1)
elseif(UNIX)
    add_compile_definitions(JENGINE_PLATFORM_LINUX=1)
endif()

# Engine development features
option(JENGINE_WITH_SOURCE_ASSETS "Enable source asset pipeline" ON)

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

# --- ThirdParty libraries ---
add_subdirectory(ThirdParty)

# --- Tools ---
add_subdirectory(Tools/JReflectGen) # (must come BEFORE Engine so JREFLECTGEN_EXE is exported)

# --- Engine modules ---
add_subdirectory(Source/Engine)

if(JENGINE_WITH_SOURCE_ASSETS) # Engine configuration options
    target_compile_definitions(Engine PUBLIC JENGINE_WITH_SOURCE_ASSETS=1)
endif()

add_subdirectory(Source/EditorRuntime)
add_subdirectory(Source/Editor)
add_subdirectory(Source/Launch) # Executable is defined here

# --- Engine directory macro ---
# Do NOT use CMAKE_BUILD_TYPE (breaks in VS/Xcode multi-config). Just define it always:
target_compile_definitions(Engine PUBLIC
        ENGINE_DIRECTORY="$<SHELL_PATH:${CMAKE_SOURCE_DIR}>"
)

# Engine internals folder (IDE organization)
set_target_properties(JApplication Engine EditorRuntime PROPERTIES
        FOLDER "EngineLibs"
        EXCLUDE_FROM_ALL TRUE
)

# Compiler warnings for best portability across compilers
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    target_compile_options(Engine PRIVATE
            -Wextra
            -Wpedantic
            -Werror=implicit-function-declaration
    )
elseif (MSVC)
    target_compile_options(Engine PRIVATE
            /permissive-
            /EHsc
    )
endif()