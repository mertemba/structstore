# StructStore

Header-only C++ library and Python bindings for introspectable structures,
optionally shared between processes.

This library does not use RTTI, CRTP or other types of reflection. Instead, it
relies on user-provided function calls to register fields.

## Usage examples

### C++

```c++
#include <structstore.hpp>
namespace stst = structstore;

int main() {
    stst::StructStore store;
    int& num = store.get<int>("num");
    num = 5;
    std::cout << "store: " << store << std::endl;
    return 0;
}
```

Creating C++ structs from nested structures with containers is also possible:

```c++
struct Subsettings {
    stst::StructStore& store;
    
    int& subnum = store.get<int>("subnum") = 42;
    stst::string& substr = store.get<stst::string>("substr") = "bar";
    
    explicit Subsettings(stst::StructStore& store) : store(store) {}
};

struct Settings {
    stst::StructStore& store;
    
    int& num = store.get<int>("num") = 5;
    bool& flag = store.get<bool>("flag") = true;
    stst::string& str = store.get<stst::string>("str") = "foo";
    Subsettings subsettings{store.get<stst::StructStore>("subsettings")};
    
    explicit Settings(stst::StructStore& store) : store(store) {}
};

int main() {
    stst::StructStore store;
    Settings settings{store};
    settings.num = 42;
    std::cout << "settings: " << store << std::endl;
    return 0;
}
```

### Python

```python
state = structstore.StructStore()
state.add_int('num')
state.add_str('mystr')
state.add_bool('flag')
print(state.to_dict())
```

In Python, dynamic structures can be automatically constructed from classes and
dataclasses:

```python
class Substate:
    def __init__(self, subnum: int):
        self.subnum = subnum

@dataclass
class State:
    num: int
    mystr: str
    flag: bool
    substate: Substate

store = structstore.StructStore()
state = State(5, 'foo', True, Substate(42))
structstore_utils.construct_from_obj(store, state)
print(store.to_dict())
```

### Shared structure in C++

```c++
stst::StructStoreShared shdata_store("/shdata_store");
std::cout << "shared data: " << *shdata_store << std::endl;
shdata_store[H("num")] = 53;

// usage with a struct as above:
stst::StructStoreShared shsettings_store("/shsettings_store");
Settings shsettings{*shsettings_store};
shsettings.num = 42;
std::cout << "settings struct: " << *shsettings_store << std::endl;
```

### Shared structure in Python

```python
shmem = structstore.StructStoreShared("/shdata_store")
shstore = shmem.get_store()
shstate = State(5, 'foo', True, Substate(42))
structstore_utils.construct_from_obj(shstore, shstate)
print(shstore.to_dict())
```

## Implementation details

Dynamic structures (such as the internal field map and any containers) use an
arena allocator with a memory region residing along the StructStore itself.
Thus, the whole structure including dynamic structures with pointers can be
mmap'ed by several processes.

## Limitations

* The library currently only supports the following types: int, string, bool,
  nested structure.
* The arena memory region currently has a fixed size, i.e. at some point,
  additional allocations will throw an exception.
* Shared memory is mmap'ed to the same address in all processes (using
  MAP_FIXED_NOREPLACE), this might fail when the memory region is already
  reserved in a process.
* Opening shared memory multiple times (e.g. in separate threads) from one
  process is currently not supported.
* Locking a shared structure (or parts of it) is currently not supported.

## License

This repo is released under the BSD 3-Clause License. See LICENSE file for
details.
