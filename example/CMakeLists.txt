set(CPLUS_EXAMPLE_NAME demo)
set(CPLUS_EXAMPLE_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cplus)
set(CPLUS_EXAMPLE_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(CPLUS_EXAMPLE_GENERATED_C ${CPLUS_EXAMPLE_GENERATED_DIR}/${CPLUS_EXAMPLE_NAME}.c)
set(CPLUS_EXAMPLE_GENERATED_H ${CPLUS_EXAMPLE_GENERATED_DIR}/${CPLUS_EXAMPLE_NAME}.h)

find_program(CPLUS_EXECUTABLE
    NAMES ${CMAKE_PROJECT_NAME}
    REQUIRED
)

add_custom_command(
    OUTPUT
        ${CPLUS_EXAMPLE_GENERATED_C}
        ${CPLUS_EXAMPLE_GENERATED_H}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CPLUS_EXAMPLE_GENERATED_DIR}
    COMMAND ${CPLUS_EXECUTABLE} -o ${CPLUS_EXAMPLE_GENERATED_DIR} ${CPLUS_EXAMPLE_INPUT_DIR}
    DEPENDS
        ${CPLUS_EXAMPLE_INPUT_DIR}/demo.hp
        ${CPLUS_EXAMPLE_INPUT_DIR}/demo.cp
    COMMENT "Transpiling example c+ sources"
    VERBATIM
)

add_custom_target(example_transpile
    DEPENDS
        ${CPLUS_EXAMPLE_GENERATED_C}
        ${CPLUS_EXAMPLE_GENERATED_H}
)

add_executable(cplus_example_app
    src/main.c
    ${CPLUS_EXAMPLE_GENERATED_C}
)

add_dependencies(cplus_example_app example_transpile)

target_include_directories(cplus_example_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CPLUS_EXAMPLE_GENERATED_DIR}
)

target_compile_options(cplus_example_app PRIVATE
    -include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/cplus_types.h
)
