#!/bin/bash -ex

srcdir="$PWD"
builddir="/tmp/structstore_build"
mkdir "$builddir"
cd "$builddir"
cp "$srcdir/PKGBUILD" ./
mkdir src
rsync -a --exclude build "$srcdir" src/

makepkg --noextract --holdver

namcap structstore-*.pkg.tar.zst | tee -a namcap.log | (! grep -v ' W: ')
namcap structstore_py-*.pkg.tar.zst | tee -a namcap.log | (! grep -v ' W: ')
cat namcap.log
