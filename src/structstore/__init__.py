__version__ = '0.2.0'

import pathlib

from ._structstore_py import *


def cmake_dir() -> str:
    return str(pathlib.Path(__file__).parent.absolute() / 'cmake')
