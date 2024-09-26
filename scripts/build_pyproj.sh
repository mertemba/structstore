#!/bin/bash -ex

srcdir="$PWD"
builddir="$srcdir/build"
mkdir "$builddir"
cd "$builddir"

venvdir="$builddir/venv"
python -m venv "$venvdir"
source "$venvdir/bin/activate"
pip install -r "$srcdir/requirements.txt"

pyproject-build "$srcdir"
