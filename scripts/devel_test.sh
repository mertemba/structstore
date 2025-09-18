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

test -d build
source "$venvdir/bin/activate"

# this is necessary because Python confuses ASan
export ASAN_OPTIONS=verify_asan_link_order=0

# build, just to be sure everything is up-to-date
cmake --build "$builddir" || cmake --build "$builddir" -j1
cmake --install "$builddir" >/dev/null

# add test Python libs to search path
export PYTHONPATH="$PYTHONPATH:$builddir"

# test
ctest --test-dir build --output-on-failure
timeout 5 pytest -vvs "$srcdir/tests"

# coverage analysis
gcovr --cobertura-pretty --exclude-unreachable-branches --print-summary \
    -o "$builddir/coverage.xml" --filter src --txt - --txt-metric line
mkdir -p "$builddir/coverage_details"
gcovr --html-details --exclude-unreachable-branches \
    -o "$builddir/coverage_details/coverage.html" --filter src
