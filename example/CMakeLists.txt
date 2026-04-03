cmake_minimum_required(VERSION 3.20)
project(cp-example)

set(CPLUS_EXECUTABLE "" CACHE FILEPATH "Path to the installed cplus transpiler")

if(CPLUS_EXECUTABLE STREQUAL "")
    unset(CPLUS_EXECUTABLE CACHE)
    find_program(CPLUS_EXECUTABLE
        NAMES cplus
        HINTS
            $ENV{HOME}/.local/bin
            /usr/local/bin
            /opt/homebrew/bin
        REQUIRED
    )
endif()

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

add_custom_command(
    OUTPUT
        ${GENERATED_DIR}/demo.c
        ${GENERATED_DIR}/demo.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND ${CPLUS_EXECUTABLE} -o ${GENERATED_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/demo.hp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/demo.cp
    COMMENT "Transpiling example c+ sources"
    VERBATIM
)

add_executable(cplus_example_app
    src/main.c
    ${GENERATED_DIR}/demo.h
    ${GENERATED_DIR}/demo.c
)

set_source_files_properties(
    ${GENERATED_DIR}/demo.c
    ${GENERATED_DIR}/demo.h
    PROPERTIES GENERATED TRUE
)

set_source_files_properties(
    src/main.c
    PROPERTIES OBJECT_DEPENDS "${GENERATED_DIR}/demo.h"
)

target_include_directories(cplus_example_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GENERATED_DIR}
)

target_compile_options(cplus_example_app PRIVATE
    -include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cplus_types.h
)
