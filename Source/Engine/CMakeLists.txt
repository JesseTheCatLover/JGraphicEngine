# Engine/CMakeLists.txt
# ---------------- Engine Target ----------------

# Collect engine source files
file(GLOB_RECURSE ENGINE_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Private/**/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Private/**/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Public/**/*.h"
)

# Create static library
add_library(Engine STATIC ${ENGINE_SRC})

# Include directories
target_include_directories(Engine
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# Define available surface backends
target_compile_definitions(Engine PRIVATE
        HAVE_GLFW
)

# Define available rendering backends
target_compile_definitions(Engine PRIVATE
        HAVE_OPENGL
        # HAVE_VULKAN
        # HAVE_METAL
        # HAVE_DIRECTX
)

# Link dependencies
target_link_libraries(Engine
        PUBLIC glad stb glfw assimp glm nlohmann_json
)

# ---------------- Reflection CodeGen (JReflectGen) ----------------

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Where generated files will be written
set(JREFLECT_GEN_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${JREFLECT_GEN_DIR})

# JReflectGen executable path should be provided by the Tools project
if (NOT DEFINED JREFLECTGEN_EXE)
    message(FATAL_ERROR "JReflectGen executable is not set. Ensure add_subdirectory(Tools/JReflectGen) is called before Source/Engine.")
endif()

# 1) Discover reflected headers automatically by scanning for ".generated.h" include
file(GLOB_RECURSE ENGINE_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h"  "${CMAKE_CURRENT_SOURCE_DIR}/Public/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.hpp"
)
list(REMOVE_DUPLICATES ENGINE_HEADERS)

set(JREFLECT_HEADERS "")
foreach(H ${ENGINE_HEADERS})
    file(STRINGS "${H}" _refl_include REGEX "\\.generated\\.h\"")
    if (_refl_include)
        list(APPEND JREFLECT_HEADERS "${H}")
    endif()
endforeach()
list(REMOVE_DUPLICATES JREFLECT_HEADERS)

# If no reflected headers found, still create a no-op target
if (NOT JREFLECT_HEADERS)
    add_custom_target(EngineReflectionGen)
else()
    set(JREFLECT_BYPRODUCTS "")
    set(JREFLECT_GEN_CPP "")

    foreach(H ${JREFLECT_HEADERS})
        get_filename_component(STEM "${H}" NAME_WE)

        list(APPEND JREFLECT_BYPRODUCTS
                ${JREFLECT_GEN_DIR}/${STEM}.generated.h
                ${JREFLECT_GEN_DIR}/${STEM}.refl.gen.cpp
        )
        list(APPEND JREFLECT_GEN_CPP
                ${JREFLECT_GEN_DIR}/${STEM}.refl.gen.cpp
        )
    endforeach()

    # Run ONCE, but only rebuild changed headers (driver decides)
    set(JREFLECT_STAMP ${JREFLECT_GEN_DIR}/jreflectgen.stamp)

    add_custom_command(
            OUTPUT ${JREFLECT_STAMP}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${JREFLECT_GEN_DIR}
            COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/Tools/JReflectGen/run_incremental.py
            --jreflectgen ${JREFLECTGEN_EXE}
            --out ${JREFLECT_GEN_DIR}
            ${JREFLECT_HEADERS}
            COMMAND ${CMAKE_COMMAND} -E touch ${JREFLECT_STAMP}
            DEPENDS ${JREFLECT_HEADERS} ${JREFLECTGEN_EXE}
            BYPRODUCTS ${JREFLECT_BYPRODUCTS}
            COMMENT "Compiling..."
            VERBATIM
    )

    add_custom_target(EngineReflectionGen DEPENDS ${JREFLECT_STAMP})
endif()
# 2) Wire it into Engine build
add_dependencies(Engine EngineReflectionGen)

# So headers can do: #include "JActor.generated.h"
target_include_directories(Engine
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Public ${JREFLECT_GEN_DIR}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# Compile all generated .cpp files
if (JREFLECT_GEN_CPP)
    set_source_files_properties(${JREFLECT_GEN_CPP} PROPERTIES GENERATED TRUE)
    target_sources(Engine PRIVATE ${JREFLECT_GEN_CPP})
endif()

