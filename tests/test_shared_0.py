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
        shmem = structstore.StructStoreShared(
            "/dyn_shdata_store", 16384, cleanup=structstore.CleanupMode.IF_LAST
        )
        shmem.revalidate()
        with shmem.write_lock():
            shmem.state = State(5, 3.14, "foo", True, Substate(42), [0, 1])
        self.assertEqual(shmem.deepcopy(), shmem.store.deepcopy())
        shmem.state = dict(foo="bar")
        shmem.check()

        shmem2 = structstore.StructStoreShared(
            "/dyn_shdata_store", 16384, cleanup=structstore.CleanupMode.IF_LAST
        )
        shmem.revalidate()
        shmem2.revalidate()
        print(f'py1')
        shmem2.state.lst = []
        print(f'py2')
        shmem2.state.lst.append(0)
        print(f'py3')
        shmem2.state.lst.append(1)
        print(f'py4')
        self.assertEqual(shmem.deepcopy(), shmem2.deepcopy())
        print(f'py5')
        shmem2.check()
        print(f'py6')
        del shmem
        print(f'py7')
        del shmem2
