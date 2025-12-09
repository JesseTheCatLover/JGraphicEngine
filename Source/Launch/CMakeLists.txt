# Launch/CMakeLists.txt

# JApplication static library
add_library(JApplication STATIC
        Private/JApplication.cpp
        Public/JApplication.h # for IDE visibility
)

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

set(JENGINE_EXECUTABLES JEditor JGame JEngine)
foreach(tgt IN LISTS JENGINE_EXECUTABLES) # TODO: We need a codegen to escape this mess....
    if (MSVC)
        # Force link all object files from Engine.lib
        target_link_libraries(${tgt} PRIVATE Engine)
        target_link_options(${tgt} PRIVATE "/WHOLEARCHIVE:Engine")
    elseif (APPLE)
        # On macOS/Clang: force_load the static lib produced by target Engine
        target_link_options(${tgt} PRIVATE
                -Wl,-force_load,$<TARGET_FILE:Engine>
        )
    else()
        # On GCC/Clang / lld
        target_link_options(${tgt} PRIVATE
                -Wl,--whole-archive,$<TARGET_FILE:Engine>
                -Wl,--no-whole-archive
        )
    endif()
endforeach()