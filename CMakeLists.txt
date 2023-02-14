cmake_minimum_required(VERSION 3.10)
project(structstore)

include(FetchContent)

FetchContent_Declare(
    pybind
    GIT_REPOSITORY "https://github.com/pybind/pybind11"
    GIT_TAG "v2.10.0"
)

message(STATUS "Loading pybind ...")
FetchContent_MakeAvailable(pybind)

find_package(yaml-cpp REQUIRED)

set(STRUCTSTORE_INC_DIRS
        ${YAML_CPP_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/src)

pybind11_add_module(structstore src/structstore_pybind.cpp)
target_compile_features(structstore PUBLIC cxx_std_17)
target_compile_options(structstore PUBLIC -Wall -Wextra -pedantic)
target_include_directories(structstore PRIVATE ${STRUCTSTORE_INC_DIRS})
target_link_libraries(structstore PRIVATE
        ${YAML_CPP_LIBRARIES}
        rt)
set_target_properties(structstore PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(examples)
