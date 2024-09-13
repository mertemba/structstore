#!/bin/bash -ex

srcdir="$PWD"
pkgdir="/tmp/structstore_build"
mkdir "$pkgdir"
cd "$pkgdir"

cmake "$srcdir" -GNinja -DCMAKE_BUILD_TYPE=Debug
ninja
