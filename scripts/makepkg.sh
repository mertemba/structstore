#!/bin/bash -e

mkdir build_pkg
cd build_pkg
cp ../PKGBUILD ./
mkdir src
rsync -a --exclude build_pkg .. src/structstore
rm -rf src/structstore/build
makepkg --noextract --holdver
