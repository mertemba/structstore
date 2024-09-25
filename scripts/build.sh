#!/bin/bash -ex

srcdir="$PWD"
pkgdir="/tmp/structstore_build"
mkdir "$pkgdir"
cd "$pkgdir"

cmake_options=""
if [ ! -z "$BUILD_WITH_PYTHON" ]; then
    cmake_options="$cmake_options -DBUILD_WITH_PYTHON=ON"
    venvdir="$pkgdir/venv"
    python -m venv "$venvdir"
    source "$venvdir/bin/activate"
    pip install -r "$srcdir/requirements.txt"
fi
if [ ! -z "$BUILD_WITH_TESTING" ]; then
    cmake_options="$cmake_options -DBUILD_WITH_TESTING=ON"
fi
if [ ! -z "$BUILD_WITH_COVERAGE" ]; then
    cmake_options="$cmake_options -DBUILD_WITH_COVERAGE=ON"
fi

cmake "$srcdir" -GNinja -DCMAKE_BUILD_TYPE=Debug $cmake_options
ninja
