#!/bin/bash -ex

srcdir="$PWD"
pkgdir="/tmp/structstore_build"

BUILD_WITH_TESTING=1 "$srcdir"/scripts/build.sh
(cd "$pkgdir" && ctest --output-on-failure)

if [ ! -z "$BUILD_WITH_PYTHON" ]; then
    # build wheel
    rm -rf "$pkgdir"
    "$srcdir"/scripts/build_py.sh
    test -f "$srcdir"/dist/structstore-*.whl

    # install wheel
    venvdir="$pkgdir/venv"
    python -m venv "$venvdir"
    source "$venvdir/bin/activate"
    pip install -r "$srcdir/requirements.txt"
    pip install "$srcdir"/dist/structstore-*.whl
    pytest "$srcdir/tests"
fi
