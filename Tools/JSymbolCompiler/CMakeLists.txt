# Tools/JSymbolCompiler/CMakeLists.txt

if (NOT REDLEAF_BUILD_DOCS_SYNC)
    return()
endif()

find_package(Doxygen REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Root output folder
set(JSYMBOL_ROOT_DIR ${CMAKE_BINARY_DIR}/JSymbolCompiler)
set(JSYMBOL_GEN_DIR ${JSYMBOL_ROOT_DIR}/generated)

# Subfolders
set(DOXY_XML_DIR ${JSYMBOL_GEN_DIR}/xml)
set(SYMBOL_JSON ${JSYMBOL_GEN_DIR}/symbols.json)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/templates/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}Doxyfile
        @ONLY
)

add_custom_target(JSymbolCompiler ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${JSYMBOL_GEN_DIRR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DOXY_XML_DIR}

        # Step 1: generate xml output (run doxygen -> XML)
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile

        # Step 2: compile symbols
        COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.py
        --xml ${DOXY_XML_DIR}
        --out ${SYMBOL_JSON}

        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}

        COMMENT "Running JSymbolCompiler pipeline..."
        VERBATIM
)