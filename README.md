# StructStore

Library for introspectable structures for C++ and Python, optionally dynamic,
optionally shared between processes.

This library does not use RTTI or compile-time reflection. Instead, it relies on
CRTP and a user-defined function to register fields.

## Usage examples

### Static structures

C++:

```c++
#include <structstore.hpp>
namespace stst = structstore;

struct Settings : stst::StructStore<Settings> {
    int num = 5;
    bool flag = true;

    void list_fields() {
        register_field("num", num);
        register_field("flag", flag);
    }
};

int main() {
    Settings settings;
    std::cout << "settings: " << settings << std::endl;
    std::cout << YAML::Dump(to_yaml(settings)) << std::endl;
    return 0;
}
```

Nested static structures and dynamic containers are also possible:

```c++
struct Subsettings : stst::StructStore<Subsettings> {
    int subnum = 42;
    // stst::string is an std::basic_string using a custom arena allocator
    stst::string substr = {"bar", alloc};

    Subsettings() : StructStore<Subsettings>() {}

    void list_fields() {
        register_field("subnum", subnum);
        register_field("substr", substr);
    }
};

struct Settings : stst::StructStore<Settings> {
    int num = 5;
    bool flag = true;
    stst::string str = {"foo", alloc};
    Subsettings subsettings{};

    void list_fields() {
        register_field("num", num);
        register_field("flag", flag);
        register_field("str", str);
        register_field("subsettings", subsettings);
    }
};
```

Python binding from C++:
```c++
#include <structstore_pybind.hpp>
PYBIND11_MODULE(structstore_example, m) {
    stst::register_pystruct<Subsettings>(m, "Subsettings", nullptr);
    stst::register_pystruct<Settings>(m, "Settings", nullptr);
}
```

Usage in Python:
```python
settings = structstore_example.Settings()
print(f'num is {settings.num}')
print(settings.to_dict())
```

### Dynamic structures

C++:

```c++
stst::StructStoreDyn state;
int& num = state.add_field<int>("num");
num = 5;
auto& substate = state.add_field<stst::StructStoreDyn<0>>("substate");
substate.add_field<int>("subnum") = 77;
std::cout << "complete state: " << state << std::endl;
```

Python:

```python
state = structstore.StructStore()
state.add_int('num')
state.add_str('mystr')
state.add_bool('flag')
print(state.num)
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
shstate = State(5, 'foo', True, Substate(42))
structstore_utils.construct_from_obj(store, state)
print(store.to_dict())
```

### Shared structure

C++:

```c++
stst::StructStoreShared<Settings> shdata("/shdata_store");
std::cout << "shared num: " << shdata->num << std::endl;
std::cout << "shared data: " << *shdata << std::endl;
```

Python:

```python
shmem = structstore.StructStoreShared("/dyn_shdata_store")
shstore = shmem.get_store()
shstate = State(5, 'foo', True, Substate(42))
structstore_utils.construct_from_obj(shstore, shstate)
print(shstore.to_dict())
```

## Implementation details

Dynamic structures (such as the internal field map and any containers) use an
arena allocator with a memory region residing within StructStore itself.
Thus, the whole structure including dynamic structures with pointers can be
mmap'ed by several processes.

## Limitations

* The library currently only supports the following types: int, string, bool,
  nested structure.
* The arena memory region currently has a compile-time fixed size, i.e. at some 
  point, additional allocations will throw an exception.
* Shared memory is mmap'ed to the same address in all processes (using
  MAP_FIXED_NOREPLACE), this might fail when the memory region is already
  reserved in a process.
* Opening shared memory multiple times (e.g. in separate threads) from one
  process is currently not supported.
* Locking a shared structure (or parts of it) is currently not supported.

## License

This repo is released under the BSD 3-Clause License. See LICENSE file for
details.
