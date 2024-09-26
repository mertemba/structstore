#!/bin/bash -ex

srcdir="$PWD"

# this defines pkgdir and cmake_options
source scripts/build_config.sh

mkdir -p "$pkgdir"
cd "$pkgdir"

if [ ! -z "$BUILD_WITH_PYTHON" ]; then
    venvdir="$pkgdir/venv"
    python -m venv "$venvdir"
    source "$venvdir/bin/activate"
    pip install -r "$srcdir/requirements.txt"
fi

cmake "$srcdir" -GNinja -DCMAKE_BUILD_TYPE=Debug $cmake_options
ninja
