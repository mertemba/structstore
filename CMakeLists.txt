cmake_minimum_required(VERSION 3.10)
project(structstore VERSION 0.4.0)

# build options

option(BUILD_WITH_PYTHON
    "Build StructStore with Python bindings" OFF)
option(BUILD_WITH_PY_BUILD_CMAKE
    "Build StructStore with py-build-cmake" OFF)
option(BUILD_WITH_TESTING
    "Build StructStore with testing enabled" OFF)
option(BUILD_WITH_COVERAGE
    "Build StructStore with coverage analysis enabled" OFF)
option(BUILD_WITH_SANITIZER
    "Build StructStore with address sanitizer enabled" OFF)

if(${BUILD_WITH_PYTHON})
    message(STATUS "Building with Python bindings")
endif()
if(${BUILD_WITH_PY_BUILD_CMAKE})
    message(STATUS "Building with py-build-cmake")
endif()

if(${BUILD_WITH_PY_BUILD_CMAKE})
    if(NOT ${BUILD_WITH_PYTHON})
        message(FATAL_ERROR "BUILD_WITH_PY_BUILD_CMAKE enabled "
            "but BUILD_WITH_PYTHON disabled.")
    endif()
endif()

if(${BUILD_WITH_PY_BUILD_CMAKE})
    if(NOT "${CMAKE_INSTALL_PREFIX}" MATCHES "")
        message(FATAL_ERROR "BUILD_WITH_PY_BUILD_CMAKE enabled "
            "but CMAKE_INSTALL_PREFIX set.")
    endif()
endif()

# dependencies

include(CheckCCompilerFlag)
include(GenerateExportHeader)
include(GNUInstallDirs)

find_package(yaml-cpp REQUIRED)

if(${BUILD_WITH_PYTHON})
    find_package(Python 3.10 COMPONENTS Interpreter Development REQUIRED)
    execute_process(
            COMMAND "${Python_EXECUTABLE}" -c "import nanobind; print(nanobind.cmake_dir())"
            OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE NB_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${NB_DIR}")
    find_package(nanobind CONFIG REQUIRED)
    include(cmake/NanobindStubgen.cmake)
endif()

# source files

file(GLOB STRUCTSTORE_INC_FILES
        ${PROJECT_SOURCE_DIR}/src/include/*.hpp)

set(STRUCTSTORE_SRC_FILES
        ${PROJECT_SOURCE_DIR}/src/mini_malloc.cpp
        ${PROJECT_SOURCE_DIR}/src/stst_alloc.cpp
        ${PROJECT_SOURCE_DIR}/src/stst_callstack.cpp
        ${PROJECT_SOURCE_DIR}/src/stst_containers.cpp
        ${PROJECT_SOURCE_DIR}/src/stst_field.cpp
        ${PROJECT_SOURCE_DIR}/src/stst_fieldmap.cpp
        ${PROJECT_SOURCE_DIR}/src/stst_lock.cpp
        ${PROJECT_SOURCE_DIR}/src/stst_shared.cpp
        ${PROJECT_SOURCE_DIR}/src/stst_structstore.cpp
        ${PROJECT_SOURCE_DIR}/src/stst_typing.cpp
        ${PROJECT_SOURCE_DIR}/src/stst_utils.cpp)

# library target

add_library(structstore_lib SHARED
    ${STRUCTSTORE_INC_FILES}
    ${STRUCTSTORE_SRC_FILES})
set_target_properties(structstore_lib PROPERTIES
    LIBRARY_OUTPUT_NAME structstore)
set(STRUCTSTORE_LIB_TARGETS structstore_lib)

set(STRUCTSTORE_PY_TARGETS "")

# python specifics

if(${BUILD_WITH_PY_BUILD_CMAKE})
    set(PY_VERSION_SUFFIX "")
    set(PY_FULL_VERSION ${PROJECT_VERSION}${PY_VERSION_SUFFIX})

    # make sure that the Python and CMake versions match
    if(NOT "${PY_BUILD_CMAKE_PACKAGE_VERSION}" MATCHES "^${PY_FULL_VERSION}$")
        message(FATAL_ERROR "Version number does not match "
                "(${PY_BUILD_CMAKE_PACKAGE_VERSION} - ${PY_FULL_VERSION}).")
    endif()
endif()

if(${BUILD_WITH_PYTHON})
    # targets that will be linked against: python bindings helper library

    add_library(structstore_py_lib SHARED
            ${STRUCTSTORE_INC_FILES}
            ${PROJECT_SOURCE_DIR}/src/stst_py.cpp)
    set_target_properties(structstore_py_lib PROPERTIES
            LIBRARY_OUTPUT_NAME structstore_py)
    target_link_libraries(structstore_py_lib
            PUBLIC structstore_lib
            PRIVATE nanobind-static)
    list(APPEND STRUCTSTORE_PY_TARGETS structstore_py_lib)

    # targets that will not be linked against: default python bindings

    include(cmake/StructStoreBindings.cmake)

    add_structstore_binding(structstore_py src/structstore_py.cpp)
    list(APPEND STRUCTSTORE_PY_TARGETS structstore_py)

    # mark nanobind headers as system includes to suppress warnings
    get_target_property(NB_INC_DIRS nanobind-static INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(structstore_py_lib SYSTEM PRIVATE ${NB_INC_DIRS})
endif()

foreach(TARGET ${STRUCTSTORE_LIB_TARGETS} ${STRUCTSTORE_PY_TARGETS})
    target_compile_features(${TARGET} PUBLIC cxx_std_17)
    target_compile_options(${TARGET} PUBLIC -fPIC)
    target_compile_options(${TARGET} PUBLIC -Wall -Wextra -pedantic -Werror)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${TARGET} PUBLIC -Wno-class-memaccess)
        target_compile_options(${TARGET} PUBLIC -Wno-dangling-reference)
    endif()
    # target_include_directories() is called later
    target_link_libraries(${TARGET} PUBLIC
            ${YAML_CPP_LIBRARIES}
            rt)
endforeach()

# coverage

if(${BUILD_WITH_COVERAGE})
    foreach(TARGET ${STRUCTSTORE_LIB_TARGETS} ${STRUCTSTORE_PY_TARGETS})
        target_compile_options(${TARGET} PRIVATE --coverage -fprofile-arcs -ftest-coverage -g -O0 -fno-lto)
        target_link_libraries(${TARGET} PRIVATE gcov -fprofile-arcs -ftest-coverage -fno-lto)
    endforeach()
endif()

# sanitizer

if(${BUILD_WITH_SANITIZER})
    foreach(TARGET ${STRUCTSTORE_LIB_TARGETS} ${STRUCTSTORE_PY_TARGETS})
        target_compile_options(${TARGET} PRIVATE -fsanitize=address)
        target_link_libraries(${TARGET} PRIVATE -fsanitize=address)
    endforeach()
endif()

# install

set(STRUCTSTORE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/structstore)
set(STRUCTSTORE_INSTALL_LIBDIR_PY ${CMAKE_INSTALL_LIBDIR}/python3.${Python_VERSION_MINOR}/site-packages/structstore)
set(STRUCTSTORE_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR})
set(STRUCTSTORE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/structstore)
set(STRUCTSTORE_INSTALL_LICENSEDIR ${CMAKE_INSTALL_DATADIR}/licenses/structstore)
set(STRUCTSTORE_INSTALL_LICENSEDIR_PY ${CMAKE_INSTALL_DATADIR}/licenses/structstore_py)

if(${BUILD_WITH_PYTHON})
    set_target_properties(structstore_py PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${STRUCTSTORE_INSTALL_LIBDIR})
endif()

if(${BUILD_WITH_PY_BUILD_CMAKE})
    set(STRUCTSTORE_INSTALL_LIBDIR ${PY_BUILD_CMAKE_MODULE_NAME})
    set(STRUCTSTORE_INSTALL_LIBDIR_PY ${PY_BUILD_CMAKE_MODULE_NAME})
    set(STRUCTSTORE_INSTALL_INCLUDEDIR ${PY_BUILD_CMAKE_MODULE_NAME}/include)
    set(STRUCTSTORE_INSTALL_CMAKEDIR ${PY_BUILD_CMAKE_MODULE_NAME}/cmake)
    set(STRUCTSTORE_INSTALL_LICENSEDIR ${PY_BUILD_CMAKE_MODULE_NAME})
    set(STRUCTSTORE_INSTALL_LICENSEDIR_PY ${PY_BUILD_CMAKE_MODULE_NAME})
    set_target_properties(structstore_py PROPERTIES INSTALL_RPATH "\\\$ORIGIN")
endif()

install(TARGETS structstore_lib
        EXPORT StructStoreTargets
        COMPONENT base
        DESTINATION ${STRUCTSTORE_INSTALL_LIBDIR})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/include/structstore
        COMPONENT base
        DESTINATION ${STRUCTSTORE_INSTALL_INCLUDEDIR})

if(${BUILD_WITH_PYTHON})
    install(TARGETS structstore_py_lib
            EXPORT StructStorePyTargets
            COMPONENT python
            DESTINATION ${STRUCTSTORE_INSTALL_LIBDIR})
    install(TARGETS structstore_py
            COMPONENT python
            DESTINATION ${STRUCTSTORE_INSTALL_LIBDIR_PY})
    install(FILES ${PROJECT_SOURCE_DIR}/src/structstore/__init__.py
            COMPONENT python
            DESTINATION ${STRUCTSTORE_INSTALL_LIBDIR_PY})

    if(NOT ${BUILD_WITH_SANITIZER})
        nanobind_stubgen_install(structstore_py ${STRUCTSTORE_INSTALL_LIBDIR_PY})
    endif()
endif()

# add include directories after install directories are set

foreach(TARGET ${STRUCTSTORE_LIB_TARGETS} ${STRUCTSTORE_PY_TARGETS})
    target_include_directories(${TARGET}
            PUBLIC
            "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/include>"
            "$<INSTALL_INTERFACE:${STRUCTSTORE_INSTALL_INCLUDEDIR}>")
endforeach()

# inspired by https://stackoverflow.com/a/49857699
export(PACKAGE StructStore)
configure_file(${PROJECT_SOURCE_DIR}/cmake/StructStoreConfig.cmake.in
        "${PROJECT_BINARY_DIR}/StructStoreConfig.cmake" @ONLY)
install(FILES
        "${PROJECT_BINARY_DIR}/StructStoreConfig.cmake"
        COMPONENT base
        DESTINATION ${STRUCTSTORE_INSTALL_CMAKEDIR})
install(EXPORT StructStoreTargets
        COMPONENT base
        DESTINATION ${STRUCTSTORE_INSTALL_CMAKEDIR})
install(FILES
        "${PROJECT_SOURCE_DIR}/LICENSE"
        COMPONENT base
        DESTINATION ${STRUCTSTORE_INSTALL_LICENSEDIR})

if(${BUILD_WITH_PYTHON})
    export(PACKAGE StructStorePy)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/StructStorePyConfig.cmake.in
            "${PROJECT_BINARY_DIR}/StructStorePyConfig.cmake" @ONLY)
    install(FILES
            "${PROJECT_BINARY_DIR}/StructStorePyConfig.cmake"
            COMPONENT python
            DESTINATION ${STRUCTSTORE_INSTALL_CMAKEDIR})
    install(EXPORT StructStorePyTargets
            COMPONENT python
            DESTINATION ${STRUCTSTORE_INSTALL_CMAKEDIR})
    install(FILES
            "${PROJECT_SOURCE_DIR}/LICENSE"
            COMPONENT python
            DESTINATION ${STRUCTSTORE_INSTALL_LICENSEDIR_PY})
endif()

# tests

if(${BUILD_WITH_TESTING})
    # set(STRUCTSTORE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/include)
    set(STRUCTSTORE_TESTS_DIR ${PROJECT_SOURCE_DIR}/tests)
    include(${PROJECT_SOURCE_DIR}/tests/DefineTests.cmake)
endif()
