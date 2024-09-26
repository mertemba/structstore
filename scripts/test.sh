#!/bin/bash -ex

srcdir="$PWD"

# this defines pkgdir and cmake_options
source scripts/build_config.sh

mkdir -p "$pkgdir"
cd "$pkgdir"

if [ ! -z "$BUILD_WITH_VENV" ]; then
    venvdir="$pkgdir/venv"
    python -m venv "$venvdir"
    source "$venvdir/bin/activate"
    pip install -r "$srcdir/requirements.txt"
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
    pytest "$srcdir/tests"
fi
