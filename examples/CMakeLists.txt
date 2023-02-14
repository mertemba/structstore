cmake_minimum_required(VERSION 3.10)
project(structstore_examples)

find_package(yaml-cpp REQUIRED)

foreach(EXAMPLE_TARGET example example2)
add_executable(${EXAMPLE_TARGET} ${EXAMPLE_TARGET}.cpp)
target_compile_features(${EXAMPLE_TARGET} PUBLIC cxx_std_17)
target_compile_options(${EXAMPLE_TARGET} PUBLIC -Wall -Wextra -pedantic)
target_include_directories(${EXAMPLE_TARGET} PRIVATE ${STRUCTSTORE_INC_DIRS})
target_link_libraries(${EXAMPLE_TARGET} PRIVATE
        ${YAML_CPP_LIBRARIES}
        rt)
endforeach()
