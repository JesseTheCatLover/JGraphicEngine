# Tools/JDocGen/CMakeLists.txt

set(JDOCGEN_OUTPUT_DIR
        ${CMAKE_BINARY_DIR}/Documentation)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        @ONLY)

add_custom_target(
        GenerateDocs
        COMMAND ${DOXYGEN_EXECUTABLE}
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMENT "Generating API documentation...")

set_target_properties(
        GenerateDocs
        PROPERTIES
        FOLDER "Tools"
)
