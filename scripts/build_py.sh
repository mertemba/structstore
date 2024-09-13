#!/bin/bash -e

mkdir build_py
cd build_py

venvdir="$PWD/venv"
python -m venv "${venvdir}"
source "${venvdir}/bin/activate"
pip install -r ../requirements.txt

pyproject-build ..
