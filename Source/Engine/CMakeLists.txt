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

# Define available surface backends
# (You can toggle these on/off depending on platform or build type)
target_compile_definitions(Engine PRIVATE
        HAVE_GLFW          # Enable GLFW backend
)

# Define available rendering backends
# (You can toggle these on/off depending on platform or build type)
target_compile_definitions(Engine PRIVATE
        HAVE_OPENGL          # Enable OpenGL backend
        # HAVE_VULKAN         # Enable Vulkan backend
        # HAVE_METAL          # Enable Metal backend (for macOS)
        # HAVE_DIRECTX        # Enable DirectX backend (for Windows)
)

# Link dependencies
target_link_libraries(Engine
        PUBLIC glad stb glfw assimp glm nlohmann_json::nlohmann_json
)
