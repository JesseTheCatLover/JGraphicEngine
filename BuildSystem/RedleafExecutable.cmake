# BuildSystem/RedleafExecutable.cmake

function(redleaf_add_executable)

    cmake_parse_arguments(
            EXEC
            ""
            "NAME;ENTRY"
            "DEPS"
            ${ARGN}
    )

    add_executable(${EXEC_NAME} ${EXEC_ENTRY})

    target_link_libraries(${EXEC_NAME}
            PRIVATE ${EXEC_DEPS}
    )

    set_target_properties(
            ${EXEC_NAME}
            PROPERTIES RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_SOURCE_DIR}/Binaries"
    )

endfunction()