# Editor/CMakeLists.txt

# Collect private source files
file(GLOB_RECURSE EDITOR_SRC
        "Private/*.cpp" "Private/**/*.cpp"
        "Private/*.h"   "Private/**/*.h"  # add headers for IDE visibility
)

# Create static library
add_library(Editor STATIC ${EDITOR_SRC})

# Include directories
target_include_directories(Editor
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public   # exposed to users of the module
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private  # only for Editor implementation
)

# Link dependencies
target_link_libraries(Editor
        PUBLIC EngineEditor glfw imgui  # expose EngineEditor and ImGui includes to users
)

# Editor library folder
set_target_properties(Editor PROPERTIES
        FOLDER "EditorLibs"
        EXCLUDE_FROM_ALL TRUE
)