# BuildSystem/RedleafModule.cmake

function(redleaf_add_module)

    cmake_parse_arguments(
            MODULE
            ""
            "NAME"
            "PUBLIC_DEPS;PRIVATE_DEPS"
            ${ARGN}
    )

    if(NOT MODULE_NAME)
        message(FATAL_ERROR
                "redleaf_add_module: NAME is required"
        )
    endif()

    set(MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

    file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS
            "${MODULE_DIR}/Private/*.cpp"
            "${MODULE_DIR}/Private/*.c"
            "${MODULE_DIR}/Private/*.h"
            "${MODULE_DIR}/Private/*.hpp"

            "${MODULE_DIR}/Public/*.h"
            "${MODULE_DIR}/Public/*.hpp"
    )

    add_library(${MODULE_NAME} STATIC ${MODULE_SOURCES})

    target_include_directories(${MODULE_NAME}
            PUBLIC ${MODULE_DIR}/Public
            PRIVATE ${MODULE_DIR}/Private
    )

    target_link_libraries(${MODULE_NAME}
            PUBLIC ${MODULE_PUBLIC_DEPS}
            PRIVATE ${MODULE_PRIVATE_DEPS}
    )

endfunction()

function(redleaf_grant_private_access)

    cmake_parse_arguments(
            ACCESS
            ""
            "TARGET"
            "MODULES"
            ${ARGN}
    )

    if(NOT ACCESS_TARGET)
        message(FATAL_ERROR
                "redleaf_grant_private_access: TARGET is required"
        )
    endif()

    foreach(MODULE ${ACCESS_MODULES})

        target_include_directories(
                ${ACCESS_TARGET}
                PRIVATE
                ${CMAKE_SOURCE_DIR}/Source/${MODULE}/Private
        )

    endforeach()

endfunction()