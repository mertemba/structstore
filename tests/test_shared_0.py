import unittest
from dataclasses import dataclass
from typing import List, Dict

import numpy as np

import structstore


@dataclass(slots=True)
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


class TestShared0(unittest.TestCase):
    def test_shared_0(self):
        shmem = structstore.StructStoreShared("/dyn_shdata_store", 16384)
        shmem.revalidate()
        with shmem.lock():
            shmem.state = State(5, 3.14, "foo", True, Substate(42), [0, 1])
        self.assertEqual(shmem.deepcopy(), shmem.store.deepcopy())
