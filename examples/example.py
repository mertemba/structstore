from lib import structstore
from lib import structstore_example


def __main__():
    settings = structstore_example.Settings()
    print(type(settings))
    print(settings.__slots__)
    print(settings.num)
    settings.str = "foobar"
    print(settings.to_yaml())

    state = structstore.StructStore()
    state.add_int('num')
    state.add_str('mystr')
    state.add_bool('flag')

    print(type(state))
    print(state.__slots__)
    print(state.num)
    print(state.to_yaml())

    shmem = structstore.StructStoreShared("/dyn_shdata_store")
    shstore = shmem.get_store()
    shstore.add_int('num')
    shstore.add_str('mystr')
    shstore.add_bool('flag')
    print(shstore.to_dict())


if __name__ == '__main__':
    __main__()
