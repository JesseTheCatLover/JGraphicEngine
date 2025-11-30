# EngineEditor/CMakeLists.txt

# Collect all EngineEditor files
file(GLOB_RECURSE ENGINE_EDITOR_SRC
        "Private/*.cpp" "Private/**/*.cpp"
        "Public/*.h"    "Public/**/*.h"
)

add_library(EngineEditor STATIC ${ENGINE_EDITOR_SRC})

# Include directories
target_include_directories(EngineEditor
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# EngineEditor depends on Engine
target_link_libraries(EngineEditor
        PUBLIC Engine
)
