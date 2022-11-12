from lib import structstore


def construct_from_obj(store: structstore.StructStore, obj):
    assert isinstance(store, structstore.StructStore) or isinstance(store, structstore.SubStructStore)
    for name, value in obj.__dict__.items():
        if name[0] == '_':
            continue
        if isinstance(value, bool):
            store.add_bool(name)
        elif isinstance(value, int):
            store.add_int(name)
        elif isinstance(value, float):
            store.add_float(name)
        elif isinstance(value, str):
            store.add_str(name)
        elif hasattr(value, '__dict__'):
            substore = store.add_store(name)
            construct_from_obj(substore, value)
            continue
        else:
            raise RuntimeError(f'field {name} has unknown type {type(value)}')
        setattr(store, name, value)
