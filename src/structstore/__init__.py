__version__ = '0.1.6'

import pathlib

try:
    from ._structstore_bindings import *
except ImportError:
    from install._structstore_bindings import *


def cmake_dir() -> str:
    return str(pathlib.Path(__file__).parent.absolute() / 'cmake')
