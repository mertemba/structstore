cmake_minimum_required(VERSION 3.10)
project(structstore_examples)

find_package(yaml-cpp REQUIRED)

add_executable(example example.cpp)
target_compile_features(example PUBLIC cxx_std_17)
target_include_directories(example PRIVATE ${STRUCTSTORE_INC_DIRS})
target_link_libraries(example PRIVATE
        ${YAML_CPP_LIBRARIES})
