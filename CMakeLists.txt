cmake_minimum_required(VERSION 3.10)
project(OpenGLProject)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)

# Include directories for GLFW and GLAD
include_directories(Libs/glfw/include)
include_directories(Libs/glad/include)
include_directories(Libs/glm)

# Add the GLFW library
add_subdirectory(Libs/glfw)

# Add GLAD source files
add_library(glad STATIC ${CMAKE_SOURCE_DIR}/Libs/glad/src/glad.c)
add_library(ProjectLibrary Source/JShader.cpp
        Source/JShader.h Source/Settings.h Source/Settings.cpp
        Source/ThirdParty/stb_image.h
        Source/JTexture.cpp
        Source/JTexture.h
        Source/JCamera.cpp
        Source/JCamera.h)

file(GLOB Source

        "Source/*.h"
        "Source/*.cpp"
        )

add_library(STB_IMAGE "Source/ThirdParty/stb_image.cpp")
set(LIBS ${LIBS} STB_IMAGE)

# Create an executable
add_executable(OpenGLProject Main.cpp)

# Link GLFW, GLAD, and other necessary libraries
target_link_libraries(OpenGLProject ProjectLibrary glfw glad ${CMAKE_DL_LIBS})
target_link_libraries(OpenGLProject ${LIBS})