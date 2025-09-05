cmake_minimum_required(VERSION 3.10)
project(JGraphicEngine)
include(FetchContent)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# ENGINE_DIRECTORY for builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DENGINE_DIRECTORY="${CMAKE_SOURCE_DIR}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DENGINE_DIRECTORY=".")
endif()

set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "Treat warnings as errors" FORCE)

# Fetch dependencies
include(FetchContent)

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

FetchContent_MakeAvailable(glfw assimp glm)

# Gather source files
file(GLOB_RECURSE ENGINE_SRC
        "${CMAKE_SOURCE_DIR}/Source/Core/*.cpp"
        "${CMAKE_SOURCE_DIR}/Source/Rendering/*.cpp"
        "${CMAKE_SOURCE_DIR}/Source/Scene/*.cpp"
        "${CMAKE_SOURCE_DIR}/LibSrc/*.cpp"
        "${CMAKE_SOURCE_DIR}/LibSrc/*.c"
)

# Create executable
add_executable(JGraphicEngine ${ENGINE_SRC} Main.cpp)

# Include directories
target_include_directories(JGraphicEngine PRIVATE
        "${CMAKE_SOURCE_DIR}/Source"
        "${CMAKE_SOURCE_DIR}/LibSrc"
        "${CMAKE_SOURCE_DIR}/Include"
)

# Link libraries
target_link_libraries(JGraphicEngine PRIVATE glfw assimp glm)
