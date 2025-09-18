# This file is part of the StructStore library.
# Copyright (C) 2022-2025 Max Mertens
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License v3.0
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU General Lesser Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.10)
project(structstore_tests)

# build options

option(BUILD_WITH_PYTHON
    "Build StructStore tests with Python bindings" OFF)

# paths

set(STRUCTSTORE_TESTS_DIR ${PROJECT_SOURCE_DIR})

# dependencies

include(FetchContent)

if(${BUILD_WITH_PYTHON})
    message(STATUS "Building with Python bindings")
    find_package(Python 3.10 COMPONENTS Interpreter Development REQUIRED)

    execute_process(
            COMMAND "${Python_EXECUTABLE}" -c "import structstore; print(structstore.cmake_dir())"
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE STRUCTSTORE_DIR
            RESULT_VARIABLE STATUS)
    if(STATUS AND NOT STATUS EQUAL 0)
        message(FATAL_ERROR "getting structstore path failed with exit code ${STATUS}")
    endif()
    list(APPEND CMAKE_PREFIX_PATH "${STRUCTSTORE_DIR}")
endif()

find_package(StructStore REQUIRED)
if(${BUILD_WITH_PYTHON})
    find_package(StructStorePy REQUIRED)

    include(${STRUCTSTORE_CMAKE_DIR}/StructStoreBindings.cmake)
endif()

# define tests

include(${PROJECT_SOURCE_DIR}/DefineTests.cmake)
