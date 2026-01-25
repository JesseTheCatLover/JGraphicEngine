cmake_minimum_required(VERSION 3.16)

# Build the Rust generator using cargo (cross-platform)
add_custom_target(JReflectGenBuild ALL
        COMMAND cargo build --release
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

# Path to the built exe
if (WIN32)
    set(JREFLECTGEN_EXE ${CMAKE_CURRENT_LIST_DIR}/target/release/jreflectgen.exe)
else()
    set(JREFLECTGEN_EXE ${CMAKE_CURRENT_LIST_DIR}/target/release/jreflectgen)
endif()

set(JREFLECTGEN_EXE ${JREFLECTGEN_EXE} PARENT_SCOPE)