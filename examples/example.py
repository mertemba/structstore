from dataclasses import dataclass

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


def __main__():
    state = structstore.StructStore()
    state.num = 5
    state.value = 3.14
    state.mystr = 'foo'
    state.flag = True
    print(state.to_dict())

    shmem = structstore.StructStoreShared("/dyn_shdata_store", 16384)
    shstore = shmem.get_store()
    shstore.state = State(5, 3.14, 'foo', True, Substate(42))
    print(shstore.to_dict())

    shmem2 = structstore.StructStoreShared("/dyn_settings")
    settings = shmem2.get_store()
    print(settings.to_dict())


if __name__ == '__main__':
    __main__()
