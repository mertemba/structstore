__version__ = '0.1.1'

import pathlib

from ._structstore_bindings import *


def cmake_dir() -> str:
    return str(pathlib.Path(__file__).parent.absolute() / 'cmake')
