__version__ = '0.2.2'

import pathlib

from ._structstore_py import *


def cmake_dir() -> str:
    path = pathlib.Path(__file__).parent.absolute() / 'cmake'
    if path.exists():
        return str(path)
    path = path.parent.parent.parent.parent / 'cmake' / 'structstore'
    assert path.exists(), path
    return str(path)
