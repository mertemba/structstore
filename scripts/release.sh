#!/bin/sh -e

# This file is part of the StructStore library.
# Copyright (C) 2022-2025 Max Mertens
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License v3.0
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU General Lesser Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

test -f pyproject.toml
poetry export -f requirements.txt --output requirements.txt

# pacman
mkdir -p pacman
cp PKGBUILD pacman/
makepkg --printsrcinfo > pacman/.SRCINFO
