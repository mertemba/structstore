import unittest
from dataclasses import dataclass
from typing import List

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


class TestBytes0(unittest.TestCase):
    def test_bytes_0(self):
        shmem = structstore.StructStoreShared(
            "/dyn_shdata_store", 4096, reinit=True, cleanup=structstore.CleanupMode.ALWAYS)
        shmem.state = State(5, 3.14, 'foo', True, Substate(42), [0, 1])
        print(shmem.deepcopy())
        shmem.check()
        buf = shmem.to_bytes()
        del shmem

        shmem = structstore.StructStoreShared(
            "/dyn_shdata_store2", 4096, reinit=True, cleanup=structstore.CleanupMode.ALWAYS)
        shmem.from_bytes(buf)
        print(shmem.deepcopy())
        shmem.check()
