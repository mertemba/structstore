#!/bin/sh -e

test -f pyproject.toml
poetry export -f requirements.txt --output requirements.txt

# pacman
mkdir -p pacman
cp PKGBUILD pacman/
makepkg --printsrcinfo > pacman/.SRCINFO
