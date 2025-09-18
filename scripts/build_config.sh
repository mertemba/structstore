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

! test -z "$srcdir"

builddir="$srcdir/build"
venvdir="$builddir/venv"

cmake_options=""
if [ ! -z "$BUILD_WITH_PYTHON" ]; then
    cmake_options="$cmake_options -DBUILD_WITH_PYTHON=ON"
fi
if [ ! -z "$BUILD_WITH_TESTING" ]; then
    cmake_options="$cmake_options -DBUILD_WITH_TESTING=ON"
fi
if [ ! -z "$BUILD_WITH_COVERAGE" ]; then
    cmake_options="$cmake_options -DBUILD_WITH_COVERAGE=ON"
fi
if [ ! -z "$BUILD_WITH_SANITIZER" ]; then
    cmake_options="$cmake_options -DBUILD_WITH_SANITIZER=ON"
fi
if [ ! -z "$BUILD_RELEASE" ]; then
    cmake_options="$cmake_options -DCMAKE_BUILD_TYPE=RelWithDebInfo"
else
    cmake_options="$cmake_options -DCMAKE_BUILD_TYPE=Debug"
fi