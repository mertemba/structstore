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

test ! -d "$builddir"

mkdir "$builddir"
source "$srcdir/scripts/setup_venv.sh"
ln -sf "$builddir/compile_commands.json" "$srcdir/"

cmake -B "$builddir" -S "$srcdir" -G Ninja $cmake_options \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DBUILD_WITH_PYTHON=ON -DBUILD_WITH_TESTING=ON \
    -DCMAKE_COLOR_DIAGNOSTICS=ON -DCMAKE_INSTALL_PREFIX:PATH="build/venv"
