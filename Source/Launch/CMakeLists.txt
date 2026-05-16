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
        PUBLIC imgui
)

# Final executable output layout:
# <EngineRoot>/Binaries/<Executable>
set(JENGINE_BIN_DIR "${CMAKE_SOURCE_DIR}/Binaries")

# Editor executable
add_executable(JEditor
        EditorEntry.cpp
)
target_link_libraries(JEditor PRIVATE JApplication)

# Game executable
add_executable(JGame
        GameEntry.cpp
)
target_link_libraries(JGame PRIVATE JApplication)

# Universal launcher
add_executable(JEngine
        ApplicationEntry.cpp
)
target_link_libraries(JEngine PRIVATE JApplication)

# Apply output directory to all executables
set(JENGINE_EXECUTABLES JEditor JGame JEngine)

foreach(tgt IN LISTS JENGINE_EXECUTABLES)
    set_target_properties(${tgt} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${JENGINE_BIN_DIR}"
    )

    if (MSVC)
        target_link_libraries(${tgt} PRIVATE Engine)
        target_link_options(${tgt} PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE_NAME:Engine>")
    elseif (APPLE)
        target_link_options(${tgt} PRIVATE
                -Wl,-force_load,$<TARGET_FILE:Engine>
        )
    else()
        target_link_options(${tgt} PRIVATE
                -Wl,--whole-archive,$<TARGET_FILE:Engine>
                -Wl,--no-whole-archive
        )
    endif()
endforeach()

# Executables folder (these should show in the IDE)
set_property(TARGET JEditor JGame JEngine PROPERTY FOLDER "Executables")