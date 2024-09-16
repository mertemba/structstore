#!/bin/bash -ex

srcdir="$PWD"
pkgdir="/tmp/structstore_build"
mkdir "$pkgdir"
cd "$pkgdir"
cp "$srcdir/PKGBUILD" ./
mkdir src
rsync -a --exclude build "$srcdir" src/

makepkg --noextract --holdver

namcap structstore-*.pkg.tar.zst | tee -a namcap.log | (! grep -v ' W: ')
namcap structstore_py-*.pkg.tar.zst | tee -a namcap.log | (! grep -v ' W: ')
cat namcap.log
