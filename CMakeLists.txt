cmake_minimum_required(VERSION 3.20)

project(cplus LANGUAGES CXX)

set(CPLUS_LIBRARY_TARGET "${CMAKE_PROJECT_NAME}_lib")
set(CPLUS_BINARY_TARGET "${CMAKE_PROJECT_NAME}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CPLUS_ENABLE_GTEST "Build GoogleTest-based unit tests when GTest is available" ON)

add_library(${CPLUS_LIBRARY_TARGET}
    src/ast/ast.cpp
    src/driver/cli.cpp
    src/driver/driver.cpp
    src/emit/emit_c.cpp
    src/lex/lexer.cpp
    src/lower/lower.cpp
    src/lower/raii.cpp
    src/parse/parser.cpp
    src/sema/analyze.cpp
    src/sema/symbols.cpp
    src/support/diagnostics.cpp
    src/support/files.cpp
    src/support/strings.cpp
)

target_include_directories(${CPLUS_LIBRARY_TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(${CPLUS_LIBRARY_TARGET} PUBLIC cxx_std_20)

add_executable(${CPLUS_BINARY_TARGET} src/main.cpp)
target_link_libraries(${CPLUS_BINARY_TARGET} PRIVATE ${CPLUS_LIBRARY_TARGET})

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
