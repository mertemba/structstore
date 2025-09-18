#!/bin/bash -ex

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

srcdir="$PWD"

# this defines builddir and cmake_options
source "$srcdir/scripts/build_config.sh"

mkdir -p "$builddir"
cd "$builddir"

if [ ! -z "$BUILD_WITH_VENV" ]; then
    source "$srcdir/scripts/setup_venv.sh"
    if [ ! -z "$INSTALL_PY_WHEEL" ]; then
        pip install "$srcdir"/dist/structstore-*.whl
    fi
    if [ ! -z "$INSTALL_PY_TARBALL" ]; then
        pip install "$srcdir"/dist/structstore-*.tar.gz
    fi
fi

cmake "$srcdir/tests" -GNinja $cmake_options
ninja
ctest --output-on-failure
if [ ! -z "$BUILD_WITH_PYTHON" ]; then
    # add test Python libs to search path
    export PYTHONPATH="$PYTHONPATH:$builddir"
    pytest "$srcdir/tests"
fi
