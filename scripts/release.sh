#!/bin/sh -e

test -f pyproject.toml
poetry export -f requirements.txt --output requirements.txt
