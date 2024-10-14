#!/bin/bash -ex

srcdir="$PWD"

# this defines builddir and cmake_options
source "$srcdir/scripts/build_config.sh"

! test -d "$builddir"

mkdir "$builddir"
source "$srcdir/scripts/setup_venv.sh"
ln -sf "$builddir/compile_commands.json" "$srcdir/"

cmake -B "$builddir" -S "$srcdir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DBUILD_WITH_PYTHON=ON -DBUILD_WITH_TESTING=ON -DBUILD_WITH_COVERAGE=ON \
    -DCMAKE_COLOR_DIAGNOSTICS=ON -DCMAKE_INSTALL_PREFIX:PATH="build/venv"
