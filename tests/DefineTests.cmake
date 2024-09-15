enable_testing()
find_package(GTest REQUIRED)

set(TEST_TARGETS "")
list(APPEND TEST_TARGETS test_basic_0)
list(APPEND TEST_TARGETS test_basic_1)

foreach(TEST_TARGET ${TEST_TARGETS})
    add_executable(${TEST_TARGET} ${STRUCTSTORE_TESTS_DIR}/${TEST_TARGET}.cpp)
    target_compile_features(${TEST_TARGET} PUBLIC cxx_std_17)
    target_compile_options(${TEST_TARGET} PUBLIC -Wall -Wextra -pedantic)
    target_include_directories(${TEST_TARGET} PRIVATE ${STRUCTSTORE_INCLUDE_DIR})
    target_link_libraries(${TEST_TARGET} PRIVATE structstore_lib)
    add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})
endforeach()

# nanobind_add_module(example_bindings example_bindings.cpp)
# install(TARGETS example_bindings
#         COMPONENT python_modules
#         DESTINATION ${PROJECT_SOURCE_DIR})
