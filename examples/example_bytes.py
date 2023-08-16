from dataclasses import dataclass
from typing import List

from lib import structstore


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


def __main__():
    shmem = structstore.StructStoreShared(
        "/dyn_shdata_store", 2048, reinit=True, cleanup=structstore.CleanupMode.ALWAYS)
    shmem.state = State(5, 3.14, 'foo', True, Substate(42), [0, 1])
    print(shmem.deepcopy())
    buf = shmem.to_bytes()
    addr = shmem.addr()
    del shmem

    shmem = structstore.StructStoreShared(
        "/dyn_shdata_store2", 2048, reinit=True, cleanup=structstore.CleanupMode.ALWAYS,
        target_addr=addr)
    shmem.from_bytes(buf)
    print(shmem.deepcopy())


if __name__ == '__main__':
    __main__()
