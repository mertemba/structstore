from dataclasses import dataclass

from lib import structstore
from lib import structstore_utils


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


def __main__():
    state = structstore.StructStore()
    state.add_int('num')
    state.add_float('value')
    state.add_str('mystr')
    state.add_bool('flag')
    print(state.to_dict())

    shmem = structstore.StructStoreShared("/dyn_shdata_store", 16384)
    shstore = shmem.get_store()
    shstate = State(5, 3.14, 'foo', True, Substate(42))
    structstore_utils.construct_from_obj(shstore, shstate)
    print(shstore.to_dict())

    shmem2 = structstore.StructStoreShared("/dyn_settings")
    settings = shmem2.get_store()
    print(settings.to_dict())


if __name__ == '__main__':
    __main__()
