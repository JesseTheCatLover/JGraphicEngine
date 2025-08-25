cmake_minimum_required(VERSION 3.10)
project(JGraphicEngine)
include(FetchContent)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Define ENGINE_DIRECTORY for different builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DENGINE_DIRECTORY="${CMAKE_SOURCE_DIR}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DENGINE_DIRECTORY=".")
endif()

set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "Treat warnings as errors" FORCE)

# Thanks to Saman, assimp is finally built and linked.

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

add_executable(
  JGraphicEngine
  Main.cpp
  LibSrc/gl.c
  LibSrc/stb_image.cpp
  Source/JShader.cpp
  Source/Settings.cpp
  Source/JTexture.cpp
  Source/JCamera.cpp
  Source/JMesh.cpp
  Source/JModel.cpp
)

target_link_libraries(JGraphicEngine PRIVATE glfw assimp glm)
target_include_directories(JGraphicEngine PRIVATE "${CMAKE_SOURCE_DIR}/Include")
