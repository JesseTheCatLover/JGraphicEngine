cmake_minimum_required(VERSION 3.10)
project(JGraphicEngine)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)

# Define ENGINE_DIRECTORY for different builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DENGINE_DIRECTORY="${CMAKE_SOURCE_DIR}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DENGINE_DIRECTORY=".")
endif()

# Include directories for GLFW, GLAD, GLM, and Assimp
include_directories(3rdPartyLibs/glfw/include)
include_directories(3rdPartyLibs/glad/include)
include_directories(3rdPartyLibs/glm)
include_directories(3rdPartyLibs/assimp/include)

# Add the GLFW library
add_subdirectory(3rdPartyLibs/glfw)

# Add GLAD source files
add_library(glad STATIC ${CMAKE_SOURCE_DIR}/3rdPartyLibs/glad/src/glad.c)

# Add STB library
add_library(STB_IMAGE "Source/ThirdParty/stb_image.cpp")

# Add Assimp library
#set(ASSIMP_LIBRARY ${CMAKE_SOURCE_DIR}/3rdPartyLibs/assimp/lib/assimp.pc)

# Create libraries and executable
add_library(ProjectLibrary Source/JShader.cpp
        Source/JShader.h Source/Settings.h Source/Settings.cpp
        Source/ThirdParty/stb_image.h
        Source/JTexture.cpp
        Source/JTexture.h
        Source/JCamera.cpp
        Source/JCamera.h
        Source/JMesh.cpp
        Source/JMesh.h
        Source/JModel.cpp
        Source/JModel.h
)

add_executable(JGraphicEngine Main.cpp)

# Link GLFW, GLAD, Assimp, and other necessary libraries
target_link_libraries(JGraphicEngine ProjectLibrary glfw glad ${CMAKE_DL_LIBS} ${ASSIMP_LIBRARY} STB_IMAGE)
