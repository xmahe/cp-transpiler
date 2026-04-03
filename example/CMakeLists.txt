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
file(GLOB CPLUS_EXAMPLE_INPUTS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cp"
)

if(CPLUS_EXAMPLE_INPUTS STREQUAL "")
    message(FATAL_ERROR "No c+ source files found under example/src")
endif()

set(CPLUS_EXAMPLE_OUTPUTS)
set(CPLUS_EXAMPLE_HEADERS)
foreach(input_file IN LISTS CPLUS_EXAMPLE_INPUTS)
    if(input_file MATCHES "\\.hp$")
        get_filename_component(stem "${input_file}" NAME_WE)
        list(APPEND CPLUS_EXAMPLE_OUTPUTS
            "${GENERATED_DIR}/${stem}.h"
            "${GENERATED_DIR}/${stem}.c"
        )
        list(APPEND CPLUS_EXAMPLE_HEADERS
            "${GENERATED_DIR}/${stem}.h"
        )
    endif()
endforeach()

if(CPLUS_EXAMPLE_OUTPUTS STREQUAL "")
    message(FATAL_ERROR "No c+ module headers found under example/src")
endif()

add_custom_command(
    OUTPUT ${CPLUS_EXAMPLE_OUTPUTS}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND ${CPLUS_EXECUTABLE} -o ${GENERATED_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src
    DEPENDS ${CPLUS_EXAMPLE_INPUTS}
    COMMENT "Transpiling example c+ sources"
    VERBATIM
)

add_executable(cplus_example_app
    src/main.c
    ${CPLUS_EXAMPLE_OUTPUTS}
)

set_source_files_properties(
    ${CPLUS_EXAMPLE_OUTPUTS}
    PROPERTIES GENERATED TRUE
)

set_source_files_properties(
    src/main.c
    PROPERTIES OBJECT_DEPENDS "${CPLUS_EXAMPLE_HEADERS}"
)

target_include_directories(cplus_example_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GENERATED_DIR}
)

target_compile_options(cplus_example_app PRIVATE
    -include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cplus_types.h
)
