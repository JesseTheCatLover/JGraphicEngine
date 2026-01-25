# Engine/CMakeLists.txt

# ---------------- Reflection CodeGen (JReflectGen) ----------------

# Where generated files will be written
set(JREFLECT_GEN_DIR ${CMAKE_BINARY_DIR}/generated)

# JReflectGen executable path should be provided by the Tools project
if (NOT DEFINED JREFLECTGEN_EXE)
    message(FATAL_ERROR "JREFLECTGEN_EXE is not set. Add Tools/JReflectGen and export JREFLECTGEN_EXE.")
endif()

# 1) Discover reflected headers automatically by scanning for ".refl.generated.h"
file(GLOB_RECURSE ENGINE_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h"  "${CMAKE_CURRENT_SOURCE_DIR}/Public/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.hpp"
)

set(JREFLECT_HEADERS "")
foreach(H ${ENGINE_HEADERS})
    file(STRINGS "${H}" _refl_include REGEX "\\.refl\\.generated\\.h\"")
    if (_refl_include)
        list(APPEND JREFLECT_HEADERS "${H}")
    endif()
endforeach()

# Make CMake reconfigure when headers change (helps with GLOB + Ninja)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${ENGINE_HEADERS})

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
    string(JOIN ";" JREFLECT_HEADERS_ARG ${JREFLECT_HEADERS})

    add_custom_command(
            OUTPUT ${JREFLECT_GEN_DIR}/reflection_manifest.txt
            COMMAND ${JREFLECTGEN_EXE}
            --out ${JREFLECT_GEN_DIR}
            --headers "${JREFLECT_HEADERS_ARG}"
            DEPENDS ${JREFLECT_HEADERS} ${JREFLECTGEN_EXE}
            BYPRODUCTS ${JREFLECT_BYPRODUCTS}
            COMMENT "Generating reflection (.refl.generated.h / .refl.gen.cpp)"
            VERBATIM
    )

    add_custom_target(EngineReflectionGen
            DEPENDS ${JREFLECT_GEN_DIR}/reflection_manifest.txt
    )
endif()

# Collect private source files
file(GLOB_RECURSE ENGINE_SRC
        "Private/*.cpp" "Private/**/*.cpp"
        "Private/*.h"   "Private/**/*.h"  # add headers for IDE visibility
)

# Create static library
add_library(Engine STATIC ${ENGINE_SRC})

add_dependencies(Engine EngineReflectionGen)

# So headers can do: #include "JActor.refl.generated.h"
target_include_directories(Engine PRIVATE
        ${JREFLECT_GEN_DIR}
)

# Compile all generated .cpp files
# (JREFLECT_GEN_CPP is defined only if headers were found)
if (DEFINED JREFLECT_GEN_CPP)
    target_sources(Engine PRIVATE ${JREFLECT_GEN_CPP})
endif()

# Include directories
target_include_directories(Engine
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public   # exposed to users of the module
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private  # only for Engine implementation
)

# Define available surface backends
target_compile_definitions(Engine PRIVATE
        HAVE_GLFW          # Enable GLFW backend
)

# Define available rendering backends
target_compile_definitions(Engine PRIVATE
        HAVE_OPENGL          # Enable OpenGL backend
        # HAVE_VULKAN         # Enable Vulkan backend
        # HAVE_METAL          # Enable Metal backend (for macOS)
        # HAVE_DIRECTX        # Enable DirectX backend (for Windows)
)

# Link dependencies
target_link_libraries(Engine
        PUBLIC glad stb glfw assimp glm nlohmann_json
)
