cmake_minimum_required(VERSION 3.10)
project(structstore VERSION 0.2.0)

# build options

option(STRUCTSTORE_WITH_PYTHON
    "Build StructStore with Python bindings" OFF)
option(STRUCTSTORE_WITH_PY_BUILD_CMAKE
    "Build StructStore with py-build-cmake" OFF)

if(${STRUCTSTORE_WITH_PY_BUILD_CMAKE})
    if(NOT ${STRUCTSTORE_WITH_PYTHON})
    message(FATAL_ERROR "STRUCTSTORE_WITH_PY_BUILD_CMAKE enabled "
        "but STRUCTSTORE_WITH_PYTHON disabled.")
    endif()
endif()

# dependencies

find_package(yaml-cpp REQUIRED)

if(${STRUCTSTORE_WITH_PYTHON})
    find_package(Python 3.9 COMPONENTS Interpreter Development REQUIRED)
    execute_process(
            COMMAND "${Python_EXECUTABLE}" -c "import nanobind; print(nanobind.cmake_dir())"
            OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE NB_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${NB_DIR}")
    find_package(nanobind CONFIG REQUIRED)
endif()

# source files

file(GLOB STRUCTSTORE_INC_FILES
        ${PROJECT_SOURCE_DIR}/include/*.hpp)

set(STRUCTSTORE_SRC_FILES
        ${PROJECT_SOURCE_DIR}/src/stst_containers.cpp
        ${PROJECT_SOURCE_DIR}/src/stst_field.cpp
        ${PROJECT_SOURCE_DIR}/src/stst_shared.cpp
        ${PROJECT_SOURCE_DIR}/src/stst_structstore.cpp
        ${PROJECT_SOURCE_DIR}/src/stst_typing.cpp)

# targets that will be linked against: library

add_library(structstore_lib SHARED
    ${STRUCTSTORE_INC_FILES}
    ${STRUCTSTORE_SRC_FILES})
set_target_properties(structstore_lib PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
    ARCHIVE_OUTPUT_NAME structstore)
set(STRUCTSTORE_DEP_TARGETS structstore_lib)

# targets that will not be linked against: none

set(STRUCTSTORE_NONDEP_TARGETS "")


# python specifics

if(${STRUCTSTORE_WITH_PY_BUILD_CMAKE})
    set(PY_VERSION_SUFFIX "")
    set(PY_FULL_VERSION ${PROJECT_VERSION}${PY_VERSION_SUFFIX})

    # make sure that the Python and CMake versions match
    if(NOT "${PY_BUILD_CMAKE_PACKAGE_VERSION}" MATCHES "^${PY_FULL_VERSION}$")
        message(FATAL_ERROR "Version number does not match "
                "(${PY_BUILD_CMAKE_PACKAGE_VERSION} - ${PY_FULL_VERSION}).")
    endif()

    # set variables

    set(STRUCTSTORE_INSTALL_DIR ${PY_BUILD_CMAKE_MODULE_NAME})
endif()

if(${STRUCTSTORE_WITH_PYTHON})
    # targets that will be linked against: python bindings helper library

    add_library(structstore_py_lib SHARED
            ${STRUCTSTORE_INC_FILES}
            ${PROJECT_SOURCE_DIR}/src/stst_bindings.cpp)
    set_target_properties(structstore_py_lib PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
            ARCHIVE_OUTPUT_NAME structstore)
    target_link_libraries(structstore_py_lib PUBLIC
            structstore_lib
            nanobind-static)
    list(APPEND STRUCTSTORE_DEP_TARGETS structstore_py_lib)

    # targets that will not be linked against: default python bindings

    nanobind_add_module(structstore_py src/structstore_py.cpp NB_STATIC)
    target_link_libraries(structstore_py PUBLIC structstore_py_lib)
    set_target_properties(structstore_py PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
            LIBRARY_OUTPUT_NAME _structstore_py)
    target_compile_definitions(structstore_py PRIVATE
            MODULE_NAME=$<TARGET_FILE_BASE_NAME:structstore_py>
            VERSION_INFO="${PY_FULL_VERSION}"
    )
    # hide all symbols by default(including external libraries on Linux)
    set_target_properties(structstore_py PROPERTIES
            CXX_VISIBILITY_PRESET "hidden"
            VISIBILITY_INLINES_HIDDEN true)
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        target_link_options(structstore_py PRIVATE "LINKER:--exclude-libs,ALL")
    endif()
    set_target_properties(structstore_py PROPERTIES INSTALL_RPATH "\\\$ORIGIN")
    list(APPEND STRUCTSTORE_NONDEP_TARGETS structstore_py_lib)
endif()

foreach(TARGET ${STRUCTSTORE_DEP_TARGETS} ${STRUCTSTORE_NONDEP_TARGETS})
    target_compile_features(${TARGET} PUBLIC cxx_std_17)
    target_compile_options(${TARGET} PUBLIC -fPIC)
    target_compile_options(${TARGET} PUBLIC -Wall -Wextra -pedantic)
    target_include_directories(${TARGET} PRIVATE 
            ${YAML_CPP_INCLUDE_DIRS}
            ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(${TARGET} PUBLIC
            ${YAML_CPP_LIBRARIES}
            rt)
endforeach()

# install

install(TARGETS ${STRUCTSTORE_DEP_TARGETS} nanobind-static
        EXPORT StructStoreTargets
        COMPONENT install_modules
        DESTINATION ${STRUCTSTORE_INSTALL_DIR})
install(TARGETS ${STRUCTSTORE_NONDEP_TARGETS}
        COMPONENT install_modules
        DESTINATION ${STRUCTSTORE_INSTALL_DIR})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include
        COMPONENT install_modules
        DESTINATION ${STRUCTSTORE_INSTALL_DIR})

include(cmake/NanobindStubgen.cmake)
nanobind_stubgen(structstore_py)
nanobind_stubgen_install(structstore_py ${STRUCTSTORE_INSTALL_DIR})

# inspired by https://stackoverflow.com/a/49857699
export(TARGETS ${STRUCTSTORE_DEP_TARGETS} nanobind-static
        FILE "${PROJECT_BINARY_DIR}/StructStoreTargets.cmake")
export(PACKAGE StructStore)
configure_file(${PROJECT_SOURCE_DIR}/cmake/StructStoreConfig.cmake.in
        "${PROJECT_BINARY_DIR}/StructStoreConfig.cmake" @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/cmake/StructStoreConfigVersion.cmake.in
        "${PROJECT_BINARY_DIR}/StructStoreConfigVersion.cmake" @ONLY)
install(FILES
        "${PROJECT_BINARY_DIR}/StructStoreConfig.cmake"
        "${PROJECT_BINARY_DIR}/StructStoreConfigVersion.cmake"
        COMPONENT install_modules
        DESTINATION ${STRUCTSTORE_INSTALL_DIR}/cmake)
install(EXPORT StructStoreTargets
        COMPONENT install_modules
        DESTINATION ${STRUCTSTORE_INSTALL_DIR}/cmake)
install(FILES
        "${PROJECT_SOURCE_DIR}/LICENSE"
        COMPONENT install_modules
        DESTINATION ${STRUCTSTORE_INSTALL_DIR}/license)
