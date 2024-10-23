#!/bin/bash -ex

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

cmake "$srcdir/tests" -GNinja -DCMAKE_BUILD_TYPE=Debug $cmake_options
ninja
ctest --output-on-failure
if [ ! -z "$BUILD_WITH_PYTHON" ]; then
    # add test Python libs to search path
    export PYTHONPATH="$PYTHONPATH:$builddir"
    pytest "$srcdir/tests"
fi
