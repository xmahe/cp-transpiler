cmake_minimum_required(VERSION 3.20)

project(cplus LANGUAGES C CXX)

set(CPLUS_LIBRARY_TARGET "${CMAKE_PROJECT_NAME}_lib")
set(CPLUS_BINARY_TARGET "${CMAKE_PROJECT_NAME}")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Install path prefix" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CPLUS_ENABLE_GTEST "Build GoogleTest-based unit tests when GTest is available" ON)
option(CPLUS_BUILD_EXAMPLE "Build the example generated-C project" OFF)

add_library(${CPLUS_LIBRARY_TARGET}
    src/ast/ast.cpp
    src/driver/cli.cpp
    src/driver/driver.cpp
    src/emit/emit_c.cpp
    src/lex/lexer.cpp
    src/lower/lower.cpp
    src/lower/raii.cpp
    src/parse/body_validation.cpp
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

include(GNUInstallDirs)
install(TARGETS ${CPLUS_BINARY_TARGET}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(CPLUS_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()
