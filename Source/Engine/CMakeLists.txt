# Engine/CMakeLists.txt

# Collect private source files
file(GLOB_RECURSE ENGINE_SRC
        "Private/*.cpp" "Private/**/*.cpp"
        "Private/*.h"   "Private/**/*.h"  # add headers for IDE visibility
)

# Create static library
add_library(Engine STATIC ${ENGINE_SRC})

# Include directories
target_include_directories(Engine
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public   # exposed to users of the module
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private  # only for Engine implementation
)

# Link dependencies
target_link_libraries(Engine
        PUBLIC glad stb glfw assimp glm
)
