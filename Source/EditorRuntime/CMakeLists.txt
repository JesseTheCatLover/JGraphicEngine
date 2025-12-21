# EditorRuntime/CMakeLists.txt

# Collect all EditorRuntime files
file(GLOB_RECURSE EDITOR_RUNTIME_SRC
        "Private/*.cpp" "Private/**/*.cpp"
        "Public/*.h"    "Public/**/*.h"
)

add_library(EditorRuntime STATIC ${EDITOR_RUNTIME_SRC})

# Include directories
target_include_directories(EditorRuntime
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_SOURCE_DIR}/Source/Engine/Public
        ${CMAKE_SOURCE_DIR}/Source/Engine/Private
        ${CMAKE_SOURCE_DIR}/Source/Editor/Public
        ${CMAKE_SOURCE_DIR}/Source/Editor/Private
)

# EditorRuntime depends on Engine and Editor
target_link_libraries(EditorRuntime
        PUBLIC Engine
        PRIVATE Editor
)
