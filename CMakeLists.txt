cmake_minimum_required(VERSION 3.10)
project(structstore)

find_package(pybind11 REQUIRED)
find_package(yaml-cpp REQUIRED)

set(STRUCTSTORE_INC_DIRS
        ${YAML_CPP_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/src)

pybind11_add_module(structstore src/structstore_pybind.cpp)
target_compile_features(structstore PUBLIC cxx_std_17)
target_compile_options(structstore PUBLIC -Wall -Wextra -pedantic)
target_include_directories(structstore PRIVATE ${STRUCTSTORE_INC_DIRS})
target_link_libraries(structstore PRIVATE
        ${YAML_CPP_LIBRARIES})
set_target_properties(structstore PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(examples)
