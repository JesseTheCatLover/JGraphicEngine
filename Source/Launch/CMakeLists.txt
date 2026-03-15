# Launch/CMakeLists.txt

# Collect launch source files
file(GLOB_RECURSE LAUNCH_SRC CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Private/**/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Private/**/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Public/**/*.h"
)

# JApplication static library
add_library(JApplication STATIC ${LAUNCH_SRC})

# Include directories
target_include_directories(JApplication
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# Link dependencies
target_link_libraries(JApplication
        PUBLIC Engine
        PUBLIC Editor
)

# Editor executable
add_executable(JEditor
        EditorEntry.cpp
)
target_link_libraries(JEditor PRIVATE JApplication)
set_target_properties(JEditor PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Game executable
add_executable(JGame
        GameEntry.cpp
)

target_link_libraries(JGame PRIVATE JApplication)
set_target_properties(JGame PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Universal launcher
add_executable(JEngine
        ApplicationEntry.cpp
)

target_link_libraries(JEngine PRIVATE JApplication)

# Output directory (bin/)
set_target_properties(JEngine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Executables folder (these should show in the IDE)
set_property(TARGET JEditor JGame JEngine PROPERTY FOLDER "Executables")