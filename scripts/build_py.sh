#!/bin/bash -ex

srcdir="$PWD"
pkgdir="/tmp/structstore_build"
mkdir "$pkgdir"
cd "$pkgdir"

venvdir="$pkgdir/venv"
python -m venv "$venvdir"
source "$venvdir/bin/activate"
pip install -r "$srcdir/requirements.txt"

pyproject-build "$srcdir"
