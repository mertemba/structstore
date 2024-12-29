enable_testing()
find_package(GTest REQUIRED)

set(TEST_LIB_TARGETS "")
list(APPEND TEST_LIB_TARGETS mystruct0)
list(APPEND TEST_LIB_TARGETS mystruct1)

foreach(TEST_LIB_TARGET ${TEST_LIB_TARGETS})
    add_library(${TEST_LIB_TARGET} SHARED ${STRUCTSTORE_TESTS_DIR}/${TEST_LIB_TARGET}.cpp)
    target_link_libraries(${TEST_LIB_TARGET} PRIVATE structstore_lib)
endforeach()

set(TEST_TARGETS "")
list(APPEND TEST_TARGETS test_alloc)
list(APPEND TEST_TARGETS test_basic_0)
list(APPEND TEST_TARGETS test_basic_1)
list(APPEND TEST_TARGETS test_mystruct0)
list(APPEND TEST_TARGETS test_mystruct1)
list(APPEND TEST_TARGETS test_utils)

foreach(TEST_TARGET ${TEST_TARGETS})
    add_executable(${TEST_TARGET} ${STRUCTSTORE_TESTS_DIR}/${TEST_TARGET}.cpp)
    add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})
endforeach()

if(${BUILD_WITH_PYTHON})
    add_structstore_binding(mystruct0_py ${STRUCTSTORE_TESTS_DIR}/mystruct0_py.cpp)
    target_link_libraries(mystruct0_py PRIVATE ${TEST_LIB_TARGETS})
    list(APPEND TEST_TARGETS mystruct0_py)
endif()

foreach(TEST_TARGET ${TEST_TARGETS})
    target_link_libraries(${TEST_TARGET} PRIVATE
        structstore_lib
        ${GTEST_BOTH_LIBRARIES}
        ${TEST_LIB_TARGETS})
endforeach()

foreach(TEST_TARGET ${TEST_TARGETS} ${TEST_LIB_TARGETS})
    target_compile_features(${TEST_TARGET} PUBLIC cxx_std_17)
    target_compile_options(${TEST_TARGET} PUBLIC -Wall -Wextra -pedantic -Werror)
    target_include_directories(${TEST_TARGET} PRIVATE ${STRUCTSTORE_INCLUDE_DIR})
    if(${BUILD_WITH_SANITIZER})
        target_compile_options(${TEST_TARGET} PRIVATE -fsanitize=address)
        target_link_libraries(${TEST_TARGET} PRIVATE -fsanitize=address)
    endif()
    if(${BUILD_WITH_COVERAGE})
        target_compile_options(${TEST_TARGET} PRIVATE --coverage -fprofile-arcs -ftest-coverage -g -O0 -fno-lto)
        target_link_libraries(${TEST_TARGET} PRIVATE  gcov -fprofile-arcs -ftest-coverage -fno-lto)
    endif()
endforeach()
