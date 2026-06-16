# BuildSystem/RedleafDependencies.cmake

include(FetchContent)

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
