from dataclasses import dataclass

from lib import structstore
from lib import structstore_example
from lib import structstore_utils


@dataclass
class Substate:
    subnum: int


@dataclass
class State:
    num: int
    mystr: str
    flag: bool
    substate: Substate


def __main__():
    settings = structstore_example.Settings()
    print(settings.num)
    print(settings.to_yaml())

    state = structstore.StructStore()
    state.add_int('num')
    state.add_str('mystr')
    state.add_bool('flag')

    shmem = structstore.StructStoreShared("/dyn_shdata_store", 16384)
    shstore = shmem.get_store()
    shstate = State(5, 'foo', True, Substate(42))
    structstore_utils.construct_from_obj(shstore, shstate)
    print(shstore.to_dict())


if __name__ == '__main__':
    __main__()
