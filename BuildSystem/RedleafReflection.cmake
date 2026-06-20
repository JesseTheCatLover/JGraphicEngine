# BuildSystem/RedleafReflection.cmake

function(redleaf_enable_reflection)

    cmake_parse_arguments(
            REFLECT
            ""
            "TARGET;MODULE_DIR"
            ""
            ${ARGN}
    )

    if(NOT REFLECT_TARGET)
        message(FATAL_ERROR "redleaf_enable_reflection: TARGET is required")
    endif()

    if(NOT REFLECT_MODULE_DIR)
        message(FATAL_ERROR "redleaf_enable_reflection: MODULE_DIR is required")
    endif()

    find_package(
            Python3
            REQUIRED
            COMPONENTS Interpreter
    )

    #
    # Validate JReflectGen
    #
    if (NOT DEFINED JREFLECTGEN_EXE)
        message(FATAL_ERROR
                "JREFLECTGEN_EXE is not set. "
                "Ensure add_subdirectory(Tools/JReflectGen) is called before reflection-enabled modules."
        )
    endif()

    #
    # Generated output directory
    #
    set(GEN_DIR
            ${CMAKE_BINARY_DIR}/generated/${REFLECT_TARGET}
    )

    file(MAKE_DIRECTORY ${GEN_DIR})

    #
    # Discover headers
    #
    file(GLOB_RECURSE MODULE_HEADERS CONFIGURE_DEPENDS
            "${REFLECT_MODULE_DIR}/Public/*.h"
            "${REFLECT_MODULE_DIR}/Public/*.hpp"
            "${REFLECT_MODULE_DIR}/Private/*.h"
            "${REFLECT_MODULE_DIR}/Private/*.hpp"
    )

    list(REMOVE_DUPLICATES MODULE_HEADERS)

    #
    # Find reflected headers
    #
    set(REFLECTED_HEADERS)

    foreach(H ${MODULE_HEADERS})

        file(
                STRINGS
                "${H}"
                REFLECTION_MARKERS
                REGEX "^[ \t]*J(CLASS|STRUCT|ENUM)[ \t]*\\("
        )

        if(REFLECTION_MARKERS)
            list(APPEND REFLECTED_HEADERS "${H}")
        endif()

    endforeach()

    list(REMOVE_DUPLICATES REFLECTED_HEADERS)

    #
    # Reflection target name
    #
    set(REFLECTION_TARGET ${REFLECT_TARGET}_ReflectionGen)

    #
    # No reflected files
    #
    if (NOT REFLECTED_HEADERS)

        add_custom_target(${REFLECTION_TARGET})

        target_include_directories(
                ${REFLECT_TARGET}
                PUBLIC
                ${GEN_DIR}
        )

        add_dependencies(
                ${REFLECT_TARGET}
                ${REFLECTION_TARGET}
        )

        return()

    endif()

    #
    # Generated files
    #
    set(REFLECT_BYPRODUCTS)
    set(REFLECT_GEN_CPP)

    foreach(H ${REFLECTED_HEADERS})

        get_filename_component(
                STEM
                "${H}"
                NAME_WE
        )

        list(APPEND REFLECT_BYPRODUCTS
                ${GEN_DIR}/${STEM}.generated.h
                ${GEN_DIR}/${STEM}.refl.gen.cpp
        )

        list(APPEND REFLECT_GEN_CPP
                ${GEN_DIR}/${STEM}.refl.gen.cpp
        )

    endforeach()

    #
    # Stamp file
    #
    set(STAMP_FILE ${GEN_DIR}/reflection.stamp)

    #
    # Generator command
    #
    add_custom_command(

            OUTPUT
            ${STAMP_FILE}

            COMMAND
            ${CMAKE_COMMAND}
            -E
            make_directory
            ${GEN_DIR}

            COMMAND
            ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/Tools/JReflectGen/run_incremental.py
            --jreflectgen
            ${JREFLECTGEN_EXE}
            --out
            ${GEN_DIR}
            ${REFLECTED_HEADERS}

            COMMAND
            ${CMAKE_COMMAND}
            -E
            touch
            ${STAMP_FILE}

            DEPENDS
            ${REFLECTED_HEADERS}
            ${JREFLECTGEN_EXE}

            BYPRODUCTS
            ${REFLECT_BYPRODUCTS}

            COMMENT
            "Generating reflection data for ${REFLECT_TARGET}"

            VERBATIM
    )

    #
    # Reflection target
    #
    add_custom_target(
            ${REFLECTION_TARGET}
            DEPENDS ${STAMP_FILE}
    )

    #
    # Ensure JReflectGen is built before this module's reflection step
    #
    add_dependencies(${REFLECTION_TARGET} JReflectGenBuild)

    #
    # Ensure reflection runs first
    #
    add_dependencies(${REFLECT_TARGET} ${REFLECTION_TARGET})

    #
    # Generated include path
    #
    target_include_directories(
            ${REFLECT_TARGET}
            PUBLIC
            ${GEN_DIR}
    )

    #
    # Generated cpp files
    #
    set_source_files_properties(
            ${REFLECT_GEN_CPP}
            PROPERTIES
            GENERATED TRUE
    )

    target_sources(
            ${REFLECT_TARGET}
            PRIVATE
            ${REFLECT_GEN_CPP}
    )

endfunction()