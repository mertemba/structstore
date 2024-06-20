cmake_minimum_required(VERSION 3.10)
project(structstore VERSION 0.1.9)

set(PY_VERSION_SUFFIX "")
set(PY_FULL_VERSION ${PROJECT_VERSION}${PY_VERSION_SUFFIX})

# make sure that the Python and CMake versions match
if (DEFINED PY_BUILD_CMAKE_PACKAGE_VERSION)
    if (NOT "${PY_BUILD_CMAKE_PACKAGE_VERSION}" MATCHES "^${PY_FULL_VERSION}$")
        message(FATAL_ERROR "Version number does not match "
                "(${PY_BUILD_CMAKE_PACKAGE_VERSION} - ${PY_FULL_VERSION}).")
    endif ()
endif ()

# dependencies

find_package(Python 3.9 COMPONENTS Interpreter Development REQUIRED)

execute_process(
        COMMAND "${Python_EXECUTABLE}" -c "import nanobind; print(nanobind.cmake_dir())"
        OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE NB_DIR)
list(APPEND CMAKE_PREFIX_PATH "${NB_DIR}")
find_package(nanobind CONFIG REQUIRED)

find_package(yaml-cpp REQUIRED)

# source files

file(GLOB STRUCTSTORE_INC_FILES
        ${PROJECT_SOURCE_DIR}/include/*.hpp)

set(STRUCTSTORE_SRC_FILES
        ${PROJECT_SOURCE_DIR}/src/structstore.cpp
        ${PROJECT_SOURCE_DIR}/src/structstore_containers.cpp
        ${PROJECT_SOURCE_DIR}/src/structstore_field.cpp
        ${PROJECT_SOURCE_DIR}/src/structstore_serialization.cpp
        ${PROJECT_SOURCE_DIR}/src/structstore_shared.cpp)

set(STRUCTSTORE_INC_DIRS
        ${YAML_CPP_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include)

# targets: library and python bindings

add_library(structstore_lib STATIC
        ${STRUCTSTORE_INC_FILES}
        ${STRUCTSTORE_SRC_FILES})
set_target_properties(structstore_lib PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
        ARCHIVE_OUTPUT_NAME structstore)

nanobind_add_module(structstore_bindings src/structstore_bindings.cpp)
target_link_libraries(structstore_bindings PRIVATE structstore_lib)
set_target_properties(structstore_bindings PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
        LIBRARY_OUTPUT_NAME _structstore_bindings)
target_compile_definitions(structstore_bindings PRIVATE
        MODULE_NAME=$<TARGET_FILE_BASE_NAME:structstore_bindings>
        VERSION_INFO="${PY_FULL_VERSION}"
)
# hide all symbols by default (including external libraries on Linux)
set_target_properties(structstore_bindings PROPERTIES
        CXX_VISIBILITY_PRESET "hidden"
        VISIBILITY_INLINES_HIDDEN true)
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_options(structstore_bindings PRIVATE "LINKER:--exclude-libs,ALL")
endif ()

foreach (TARGET structstore_lib structstore_bindings)
    target_compile_features(${TARGET} PUBLIC cxx_std_17)
    target_compile_options(${TARGET} PUBLIC -fPIC)
    target_compile_options(${TARGET} PUBLIC -Wall -Wextra -pedantic)
    target_include_directories(${TARGET} PRIVATE ${STRUCTSTORE_INC_DIRS})
    target_link_libraries(${TARGET} PRIVATE
            ${YAML_CPP_LIBRARIES}
            rt)
endforeach ()

# install

if (DEFINED PY_BUILD_CMAKE_MODULE_NAME)
    set(STRUCTSTORE_INSTALL_DIR ${PY_BUILD_CMAKE_MODULE_NAME})
else ()
    set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
    set(STRUCTSTORE_INSTALL_DIR install)
endif ()

install(TARGETS structstore_lib
        EXPORT StructStoreTargets
        COMPONENT python_modules
        DESTINATION ${STRUCTSTORE_INSTALL_DIR})
install(TARGETS structstore_bindings
        COMPONENT python_modules
        DESTINATION ${STRUCTSTORE_INSTALL_DIR})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include
        COMPONENT python_modules
        DESTINATION ${STRUCTSTORE_INSTALL_DIR})

include(cmake/NanobindStubgen.cmake)
nanobind_stubgen(structstore_bindings)
nanobind_stubgen_install(structstore_bindings ${STRUCTSTORE_INSTALL_DIR})

# inspired by https://stackoverflow.com/a/49857699
export(TARGETS structstore_lib
        FILE "${PROJECT_BINARY_DIR}/StructStoreTargets.cmake")
export(PACKAGE StructStore)
configure_file(${PROJECT_SOURCE_DIR}/cmake/StructStoreConfig.cmake.in
        "${PROJECT_BINARY_DIR}/StructStoreConfig.cmake" @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/cmake/StructStoreConfigVersion.cmake.in
        "${PROJECT_BINARY_DIR}/StructStoreConfigVersion.cmake" @ONLY)
install(FILES
        "${PROJECT_BINARY_DIR}/StructStoreConfig.cmake"
        "${PROJECT_BINARY_DIR}/StructStoreConfigVersion.cmake"
        COMPONENT python_modules
        DESTINATION ${STRUCTSTORE_INSTALL_DIR}/cmake)
install(EXPORT StructStoreTargets
        COMPONENT python_modules
        DESTINATION ${STRUCTSTORE_INSTALL_DIR}/cmake)

if (NOT DEFINED PY_BUILD_CMAKE_PACKAGE_VERSION)
    set(STRUCTSTORE_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)
    set(STRUCTSTORE_LIBRARIES structstore_lib)

    add_subdirectory(examples)
endif ()
