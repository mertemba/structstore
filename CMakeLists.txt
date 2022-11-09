cmake_minimum_required(VERSION 3.10)
project(structstore)

find_package(pybind11 REQUIRED)
find_package(yaml-cpp REQUIRED)

set(STRUCTSTORE_INC_DIRS
        ${YAML_CPP_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/src)

add_library(structstore_lib STATIC
        src/serialization.cpp
        src/structstore.cpp)
set_property(TARGET structstore_lib PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_features(structstore_lib PUBLIC cxx_std_17)
target_include_directories(structstore_lib PRIVATE ${STRUCTSTORE_INC_DIRS})
target_link_libraries(structstore_lib PRIVATE ${YAML_CPP_LIBRARIES})

add_library(structstore_python_lib STATIC
        src/structstore_pybind.cpp)
set_property(TARGET structstore_python_lib PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_features(structstore_python_lib PUBLIC cxx_std_17)
target_include_directories(structstore_python_lib PRIVATE
        ${STRUCTSTORE_INC_DIRS}
        ${PYTHON_INCLUDE_DIRS})
target_link_libraries(structstore_python_lib PRIVATE
        structstore_lib
        ${YAML_CPP_LIBRARIES})

pybind11_add_module(structstore src/structstore_dyn_pybind.cpp)
target_compile_features(structstore PUBLIC cxx_std_17)
target_include_directories(structstore PRIVATE ${STRUCTSTORE_INC_DIRS})
target_link_libraries(structstore PRIVATE
        structstore_python_lib
        ${YAML_CPP_LIBRARIES})
set_target_properties(structstore PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(examples)
