cmake_minimum_required(VERSION 3.10)
project(structstore_examples)

find_package(pybind11 REQUIRED)
find_package(yaml-cpp REQUIRED)

add_executable(example example.cpp)
target_compile_features(example PUBLIC cxx_std_17)
target_include_directories(example PRIVATE ${STRUCTSTORE_INC_DIRS})
target_link_libraries(example PRIVATE
        structstore_lib
        ${YAML_CPP_LIBRARIES})

add_executable(example_dyn example_dyn.cpp)
target_compile_features(example_dyn PUBLIC cxx_std_17)
target_include_directories(example_dyn PRIVATE ${STRUCTSTORE_INC_DIRS})
target_link_libraries(example_dyn PRIVATE
        structstore_lib
        ${YAML_CPP_LIBRARIES})

pybind11_add_module(structstore_example example_pybind.cpp)
target_compile_features(structstore_example PUBLIC cxx_std_17)
target_include_directories(structstore_example PRIVATE ${STRUCTSTORE_INC_DIRS})
target_link_libraries(structstore_example PRIVATE
        structstore_lib
        structstore_python_lib
        ${YAML_CPP_LIBRARIES})
set_target_properties(structstore_example PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
