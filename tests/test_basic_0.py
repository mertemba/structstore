import unittest
from dataclasses import dataclass
from typing import List, Dict

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
        state_copy: Dict = state.deepcopy()
        print(state_copy["vec"])
        print(state.vec.copy())
        self.assertTrue((state.vec.copy() == state_copy["vec"]).all())
        self.assertTrue(state.sub.vec.copy()[0] == state_copy["sub"]["vec"][0])
        self.assertTrue((state.mat.copy() == state_copy["mat"]).all().all())
        del state_copy["vec"]
        del state_copy["sub"]
        del state_copy["mat"]
        assert state_copy == {
            "num": 5,
            "value": 3.14,
            "mystr": "foo",
            "flag": True,
            "none": None,
            "lst": [1, 2, 3, 5, 8],
            "tuple": [0, 0],
        }

        state2 = structstore.StructStore()
        state2.state = state.deepcopy()
        assert state2.state == state
        state.sub.vec.copy()[0] += 1.0
        assert state2.state != state

        shmem = structstore.StructStoreShared("/dyn_shdata_store", 16384)
        with shmem.lock():
            shmem.state = State(5, 3.14, "foo", True, Substate(42), [0, 1])
        shmem_copy = shmem.deepcopy()
