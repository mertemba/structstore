import unittest
from dataclasses import dataclass
from typing import List

import numpy as np

import structstore


@dataclass
class Substate:
    subnum: int


@dataclass
class State:
    num: int
    value: float
    mystr: str
    flag: bool
    substate: Substate
    lst: List[int]


class TestBasic0(unittest.TestCase):
    def test_basic_0(self):
        state = structstore.StructStore()
        state.num = 5
        state.value = 3.14
        state.mystr = "foo"
        state.flag = True
        state.none = None
        state.lst = [1, 2, 3, 5, 8]
        state.tuple = (0.0, 0.0)
        state.vec = np.ndarray(shape=(0, 2), dtype=np.float64)
        state.sub = dict(vec=np.array([1.0, 2.0, 3.0, 4.0], dtype=np.float32))
        state.mat = np.array([[1.0, 2.0], [3.0, 4.0]])
        print(state.deepcopy())

        shmem = structstore.StructStoreShared("/dyn_shdata_store", 16384)
        with shmem.lock():
            shmem.state = State(5, 3.14, "foo", True, Substate(42), [0, 1])
        print(shmem.deepcopy())

        shmem2 = structstore.StructStoreShared("/dyn_settings")
        print(shmem2.deepcopy())
