# BuildSystem/RedleafUtilities.cmake

function(redleaf_set_folder TARGET_NAME FOLDER_NAME)

    set_target_properties(
            ${TARGET_NAME}
            PROPERTIES
            FOLDER ${FOLDER_NAME}
    )

endfunction()

function(redleaf_force_link TARGET LIBRARY)

    if (MSVC)

        target_link_options(
                ${TARGET}
                PRIVATE
                "/WHOLEARCHIVE:$<TARGET_FILE_NAME:${LIBRARY}>"
        )

    elseif (APPLE)

        target_link_options(
                ${TARGET}
                PRIVATE
                -Wl,-force_load,$<TARGET_FILE:${LIBRARY}>
        )

    else()

        target_link_options(
                ${TARGET}
                PRIVATE
                -Wl,--whole-archive,$<TARGET_FILE:${LIBRARY}>
                -Wl,--no-whole-archive
        )

    endif()

endfunction()