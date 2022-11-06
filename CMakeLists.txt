cmake_minimum_required(VERSION 3.24)
project(structstore)

find_package(pybind11 REQUIRED)
find_package(yaml-cpp REQUIRED)

set(STRUCTSTORE_INC_DIR ${PROJECT_SOURCE_DIR}/src)

add_executable(example examples/example.cpp)
target_compile_features(example PUBLIC cxx_std_17)
target_include_directories(example PRIVATE
        ${YAML_CPP_INCLUDE_DIRS}
        ${STRUCTSTORE_INC_DIR})
target_link_libraries(example PRIVATE ${YAML_CPP_LIBRARIES})

add_executable(example_dyn examples/example_dyn.cpp)
target_compile_features(example_dyn PUBLIC cxx_std_17)
target_include_directories(example_dyn PRIVATE
        ${YAML_CPP_INCLUDE_DIRS}
        ${STRUCTSTORE_INC_DIR})
target_link_libraries(example_dyn PRIVATE ${YAML_CPP_LIBRARIES})

pybind11_add_module(structstore_example examples/example_pybind.cpp)
target_compile_features(structstore_example PUBLIC cxx_std_17)
target_include_directories(structstore_example PRIVATE
        ${YAML_CPP_INCLUDE_DIRS}
        ${STRUCTSTORE_INC_DIR})
target_link_libraries(structstore_example PRIVATE ${YAML_CPP_LIBRARIES})
set_target_properties(structstore_example PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

pybind11_add_module(structstore src/structstore_dyn_pybind.cpp)
target_compile_features(structstore PUBLIC cxx_std_17)
target_include_directories(structstore PRIVATE
        ${YAML_CPP_INCLUDE_DIRS}
        ${STRUCTSTORE_INC_DIR})
target_link_libraries(structstore PRIVATE ${YAML_CPP_LIBRARIES})
set_target_properties(structstore PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
