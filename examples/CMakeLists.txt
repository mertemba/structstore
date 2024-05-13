cmake_minimum_required(VERSION 3.10)
project(structstore_examples)

find_package(Python 3.9 COMPONENTS Interpreter Development REQUIRED)

execute_process(
        COMMAND "${Python_EXECUTABLE}" -c "import structstore; print(structstore.cmake_dir())"
        OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE STRUCTSTORE_DIR)
list(APPEND CMAKE_PREFIX_PATH "${STRUCTSTORE_DIR}")
find_package(StructStore REQUIRED)

foreach(EXAMPLE_TARGET example example2)
    add_executable(${EXAMPLE_TARGET} ${EXAMPLE_TARGET}.cpp)
    target_compile_features(${EXAMPLE_TARGET} PUBLIC cxx_std_17)
    target_compile_options(${EXAMPLE_TARGET} PUBLIC -Wall -Wextra -pedantic)
    target_include_directories(${EXAMPLE_TARGET} PRIVATE ${STRUCTSTORE_INCLUDE_DIRS})
    target_link_libraries(${EXAMPLE_TARGET} PRIVATE
            ${STRUCTSTORE_LIBRARIES}
            ${YAML_CPP_LIBRARIES}
            rt)
endforeach()
