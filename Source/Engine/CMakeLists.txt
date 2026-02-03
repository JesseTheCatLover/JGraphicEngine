cmake_minimum_required(VERSION 3.15)

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

# Where generated files will be written
set(JREFLECT_GEN_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${JREFLECT_GEN_DIR})

# JReflectGen executable path should be provided by the Tools project
if (NOT DEFINED JREFLECTGEN_EXE)
    message(FATAL_ERROR "JREFLECTGEN_EXE is not set. Ensure add_subdirectory(Tools/JReflectGen) is called before Source/Engine.")
endif()

# 1) Discover reflected headers automatically by scanning for ".refl.generated.h"
file(GLOB_RECURSE ENGINE_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h"  "${CMAKE_CURRENT_SOURCE_DIR}/Public/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.hpp"
)

# Make CMake reconfigure when headers change (helps with GLOB + Ninja)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${ENGINE_HEADERS})

set(JREFLECT_HEADERS "")
foreach(H ${ENGINE_HEADERS})
    file(STRINGS "${H}" _refl_include REGEX "\\.refl\\.generated\\.h\"")
    if (_refl_include)
        list(APPEND JREFLECT_HEADERS "${H}")
    endif()
endforeach()

# If no reflected headers found, still create a no-op target
if (NOT JREFLECT_HEADERS)
    add_custom_target(EngineReflectionGen)
else()
    # 2) Compute expected outputs (typename-only)
    set(JREFLECT_BYPRODUCTS "")
    set(JREFLECT_GEN_CPP "")

    foreach(H ${JREFLECT_HEADERS})
        get_filename_component(STEM "${H}" NAME_WE)

        list(APPEND JREFLECT_BYPRODUCTS
                ${JREFLECT_GEN_DIR}/${STEM}.refl.generated.h
                ${JREFLECT_GEN_DIR}/${STEM}.refl.gen.cpp
        )

        list(APPEND JREFLECT_GEN_CPP
                ${JREFLECT_GEN_DIR}/${STEM}.refl.gen.cpp
        )
    endforeach()

    # 3) Run generator once for all headers
    # NOTE: This can hit command-line length limits on Windows.
    # Recommended future improvement: use a response file (--headers-rsp).
    string(JOIN ";" JREFLECT_HEADERS_ARG ${JREFLECT_HEADERS})

    add_custom_command(
            OUTPUT ${JREFLECT_GEN_DIR}/reflection_manifest.txt
            COMMAND ${JREFLECTGEN_EXE}
            --out ${JREFLECT_GEN_DIR}
            --headers "${JREFLECT_HEADERS_ARG}"
            DEPENDS
            ${JREFLECT_HEADERS}
            JReflectGenBuild
            BYPRODUCTS ${JREFLECT_BYPRODUCTS}
            COMMENT "Generating reflection (.refl.generated.h / .refl.gen.cpp)"
            VERBATIM
    )

    add_custom_target(EngineReflectionGen
            DEPENDS ${JREFLECT_GEN_DIR}/reflection_manifest.txt
    )
endif()

# 4) Wire it into Engine build
add_dependencies(Engine EngineReflectionGen)

# So headers can do: #include "JActor.refl.generated.h"
target_include_directories(Engine PRIVATE ${JREFLECT_GEN_DIR})

# Compile all generated .cpp files
if (JREFLECT_GEN_CPP)
    set_source_files_properties(${JREFLECT_GEN_CPP} PROPERTIES GENERATED TRUE)
    target_sources(Engine PRIVATE ${JREFLECT_GEN_CPP})
endif()