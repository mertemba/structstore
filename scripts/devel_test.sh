#!/bin/bash -ex

srcdir="$PWD"

# this defines builddir and cmake_options
source "$srcdir/scripts/build_config.sh"

test -d build
source "$venvdir/bin/activate"

# this is necessary because Python confuses ASan
export ASAN_OPTIONS=verify_asan_link_order=0

# build, just to be sure everything is up-to-date
cmake --build "$builddir"
cmake --install "$builddir"

# add test Python libs to search path
export PYTHONPATH="$PYTHONPATH:$builddir"

# test
ctest --test-dir build --output-on-failure
timeout 5 pytest -vv "$srcdir/tests"

# coverage analysis
gcovr --cobertura-pretty --exclude-unreachable-branches --print-summary \
    -o "$builddir/coverage.xml" --filter src --txt - --txt-metric line
mkdir -p "$builddir/coverage_details"
gcovr --html-details --exclude-unreachable-branches \
    -o "$builddir/coverage_details/coverage.html" --filter src
