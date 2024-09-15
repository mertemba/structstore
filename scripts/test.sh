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
    if [ -f "$srcdir"/dist/structstore-*.whl ]; then
        pip install "$srcdir"/dist/structstore-*.whl
    fi
fi

cmake "$srcdir/tests" -GNinja -DCMAKE_BUILD_TYPE=Debug $cmake_options
ninja
ctest --output-on-failure
