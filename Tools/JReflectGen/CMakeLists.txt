cmake_minimum_required(VERSION 3.15)

# This subdir builds the Rust reflection generator using cargo.
project(JReflectGenTool NONE)

# Where cargo will output binaries
set(JREFLECTGEN_WORKDIR ${CMAKE_CURRENT_LIST_DIR})

# Build the Rust exe (Release is typical even for Debug builds, generator is a tool)
add_custom_command(
        OUTPUT ${JREFLECTGEN_WORKDIR}/target/release/jreflectgen${CMAKE_EXECUTABLE_SUFFIX}
        COMMAND cargo build --release
        WORKING_DIRECTORY ${JREFLECTGEN_WORKDIR}
        COMMENT "Building JReflectGen (cargo --release)"
        VERBATIM
)

add_custom_target(JReflectGenBuild
        DEPENDS ${JREFLECTGEN_WORKDIR}/target/release/jreflectgen${CMAKE_EXECUTABLE_SUFFIX}
)

# Export absolute path for parent projects
set(JREFLECTGEN_EXE
        ${JREFLECTGEN_WORKDIR}/target/release/jreflectgen${CMAKE_EXECUTABLE_SUFFIX}
        PARENT_SCOPE
)