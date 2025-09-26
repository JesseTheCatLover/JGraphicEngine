cmake_minimum_required(VERSION 3.15)
project(JGraphicEngine)
include(FetchContent)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "Treat warnings as errors" FORCE)

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

FetchContent_Declare(
        glm
        GIT_TAG cmake-fix
        GIT_SHALLOW TRUE
        GIT_REPOSITORY https://github.com/Saman-Safaei-Dev/glm.git
)

FetchContent_Declare(
        json
        GIT_TAG v3.11.3 # pick latest
        GIT_REPOSITORY https://github.com/nlohmann/json.git
)

FetchContent_MakeAvailable(glfw assimp glm json)

# Add submodules
# --- ThirdParty libraries ---
add_subdirectory(ThirdParty)
# --- Engine modules
add_subdirectory(Source/Engine)
add_subdirectory(Source/Editor)

# Create executable and link
add_executable(JGraphicEngine Main.cpp)
target_link_libraries(JGraphicEngine PRIVATE Engine Editor)

# --- Engine directory macro ---
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Engine PUBLIC ENGINE_DIRECTORY="${CMAKE_SOURCE_DIR}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(Engine PUBLIC ENGINE_DIRECTORY="${CMAKE_SOURCE_DIR}")
endif()

