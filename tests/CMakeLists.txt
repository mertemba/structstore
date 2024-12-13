cmake_minimum_required(VERSION 3.10)
project(structstore_tests)

# build options

option(BUILD_WITH_PYTHON
    "Build StructStore tests with Python bindings" OFF)

# dependencies

if(${BUILD_WITH_PYTHON})
    message(STATUS "Building with Python bindings")
    find_package(Python 3.9 COMPONENTS Interpreter Development REQUIRED)

    execute_process(
            COMMAND "${Python_EXECUTABLE}" -c "import nanobind; print(nanobind.cmake_dir())"
            OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE NB_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${NB_DIR}")
    find_package(nanobind CONFIG REQUIRED)
    include(${PROJECT_SOURCE_DIR}/../cmake/NanobindStubgen.cmake)

    execute_process(
            COMMAND "${Python_EXECUTABLE}" -c "import structstore; print(structstore.cmake_dir())"
            OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE STRUCTSTORE_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${STRUCTSTORE_DIR}")

    include(${PROJECT_SOURCE_DIR}/../cmake/StructStoreBindings.cmake)
endif()

find_package(StructStore REQUIRED)
if(${BUILD_WITH_PYTHON})
    find_package(StructStorePy REQUIRED)
endif()

# define tests

set(STRUCTSTORE_TESTS_DIR ${PROJECT_SOURCE_DIR})
include(${PROJECT_SOURCE_DIR}/DefineTests.cmake)
