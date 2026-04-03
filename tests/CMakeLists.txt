set(CPLUS_FIXTURE_LOCK "${CMAKE_PROJECT_NAME}_fixture_output")

function(add_transpile_pass_test name input_dir expected_dir)
    add_test(
        NAME ${name}
        COMMAND bash
            ${CMAKE_CURRENT_SOURCE_DIR}/ctest_fixture.sh
            $<TARGET_FILE:${CPLUS_BINARY_TARGET}>
            pass
            ${CMAKE_CURRENT_SOURCE_DIR}/${input_dir}
            ${CMAKE_CURRENT_SOURCE_DIR}/${expected_dir}
            ${CMAKE_CURRENT_BINARY_DIR}/tmp/${name}
    )
    set_tests_properties(${name} PROPERTIES
        LABELS "e2e;pass"
        RESOURCE_LOCK ${CPLUS_FIXTURE_LOCK}
    )
endfunction()

function(add_transpile_fail_test name input_dir expected_dir)
    add_test(
        NAME ${name}
        COMMAND bash
            ${CMAKE_CURRENT_SOURCE_DIR}/ctest_fixture.sh
            $<TARGET_FILE:${CPLUS_BINARY_TARGET}>
            fail
            ${CMAKE_CURRENT_SOURCE_DIR}/${input_dir}
            ${CMAKE_CURRENT_SOURCE_DIR}/${expected_dir}
            ${CMAKE_CURRENT_BINARY_DIR}/tmp/${name}
    )
    set_tests_properties(${name} PROPERTIES
        LABELS "e2e;fail"
        RESOURCE_LOCK ${CPLUS_FIXTURE_LOCK}
    )
endfunction()

add_transpile_pass_test(smoke_basic smoke/basic/input smoke/basic/expected)
set_tests_properties(smoke_basic PROPERTIES LABELS "e2e;pass;smoke")

add_transpile_pass_test(interface_fulfillment_basic interface-fulfillment/basic/input interface-fulfillment/basic/expected)
add_transpile_pass_test(maybe_safety_basic maybe-safety/basic/input maybe-safety/basic/expected)
add_transpile_fail_test(maybe_safety_invalid maybe-safety/invalid/input maybe-safety/invalid/expected)
add_transpile_fail_test(namespace_top_level_invalid namespace-top-level/invalid/input namespace-top-level/invalid/expected)
add_transpile_fail_test(forbidden_constructs_invalid forbidden-constructs/invalid/input forbidden-constructs/invalid/expected)
add_transpile_pass_test(body_parsing_basic body-parsing/basic/input body-parsing/basic/expected)
add_transpile_fail_test(body_parsing_invalid body-parsing/invalid/input body-parsing/invalid/expected)
add_transpile_pass_test(example_transpile_basic example-transpile/basic/input example-transpile/basic/expected)
add_transpile_pass_test(board_serial_example examples/board-serial/basic/input examples/board-serial/basic/expected)
add_transpile_pass_test(out_of_class_methods_basic out-of-class-methods/basic/input out-of-class-methods/basic/expected)
add_transpile_pass_test(out_of_class_method_calls_basic out-of-class-method-calls/basic/input out-of-class-method-calls/basic/expected)
add_transpile_pass_test(out_of_class_object_method_calls_basic out-of-class-object-method-calls/basic/input out-of-class-object-method-calls/basic/expected)
add_transpile_pass_test(out_of_class_field_access_basic out-of-class-field-access/basic/input out-of-class-field-access/basic/expected)
add_transpile_pass_test(private_methods_basic private-methods/basic/input private-methods/basic/expected)
add_transpile_pass_test(static_vars_basic static-vars/basic/input static-vars/basic/expected)
add_transpile_pass_test(inject_bind_basic inject-bind/basic/input inject-bind/basic/expected)
add_transpile_fail_test(inject_bind_invalid inject-bind/invalid/input inject-bind/invalid/expected)
add_transpile_pass_test(member_subobjects_basic member-subobjects/basic/input member-subobjects/basic/expected)
add_transpile_fail_test(member_subobjects_invalid member-subobjects/invalid/input member-subobjects/invalid/expected)
add_transpile_pass_test(member_initializers_basic member-initializers/basic/input member-initializers/basic/expected)
add_transpile_fail_test(member_initializers_invalid member-initializers/invalid/input member-initializers/invalid/expected)
add_transpile_pass_test(member_initializer_overloads_basic member-initializer-overloads/basic/input member-initializer-overloads/basic/expected)
add_transpile_fail_test(member_initializer_overloads_invalid member-initializer-overloads/invalid/input member-initializer-overloads/invalid/expected)
add_transpile_pass_test(raii_return_basic raii-return/basic/input raii-return/basic/expected)
add_transpile_pass_test(raii_return_value_basic raii-return-value/basic/input raii-return-value/basic/expected)
add_transpile_pass_test(raii_fallthrough_basic raii-fallthrough/basic/input raii-fallthrough/basic/expected)
add_transpile_pass_test(raii_nested_return_basic raii-nested-return/basic/input raii-nested-return/basic/expected)
add_transpile_pass_test(raii_nested_return_value_basic raii-nested-return-value/basic/input raii-nested-return-value/basic/expected)

if(CPLUS_ENABLE_GTEST)
    set(CPLUS_GTEST_HINTS
        /opt/homebrew/opt/googletest
        /usr/local/opt/googletest
    )

    find_package(GTest CONFIG QUIET HINTS ${CPLUS_GTEST_HINTS})
    if(NOT GTest_FOUND)
        find_package(GTest QUIET HINTS ${CPLUS_GTEST_HINTS})
    endif()

    if(GTest_FOUND)
        set(CPLUS_UNIT_TEST_TARGET "${CMAKE_PROJECT_NAME}_unit_tests")

        add_executable(${CPLUS_UNIT_TEST_TARGET}
            unit/strings_test.cpp
        )

        target_link_libraries(${CPLUS_UNIT_TEST_TARGET}
            PRIVATE
                ${CPLUS_LIBRARY_TARGET}
                GTest::gtest_main
        )

        add_test(NAME ${CPLUS_UNIT_TEST_TARGET} COMMAND $<TARGET_FILE:${CPLUS_UNIT_TEST_TARGET}>)
        set_tests_properties(${CPLUS_UNIT_TEST_TARGET} PROPERTIES LABELS "unit;gtest")
    else()
        message(STATUS "GTest not found; skipping GoogleTest unit tests")
    endif()
endif()
