#!/bin/bash -ex

srcdir="$PWD"

# this defines builddir and cmake_options
source "$srcdir/scripts/build_config.sh"

test -d build
source "$venvdir/bin/activate"

# build, just to be sure everything is up-to-date
cmake --build "$builddir"
cmake --install "$builddir"

# test
ctest --test-dir build --output-on-failure
export PYTHONPATH="$PYTHONPATH:$srcdir/install/lib/python3.12/site-packages"
pytest "$srcdir/tests"

# coverage analysis
gcovr --cobertura-pretty --exclude-unreachable-branches --print-summary \
    -o "$builddir/coverage.xml" --filter src --txt - --txt-metric line
mkdir -p "$builddir/coverage_details"
gcovr --html-details --exclude-unreachable-branches \
    -o "$builddir/coverage_details/coverage.html" --filter src
