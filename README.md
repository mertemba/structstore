# StructStore

Structured object storage, dynamically typed, to be shared between processes.
C++17 library and Python bindings.

## Usage examples

### C++

```c++
#include <structstore/structstore.hpp>
namespace stst = structstore;

int main() {
    stst::StructStore store;
    int& num = store["num"];
    num = 5;
    stst::List& list = store["list"];
    list.push_back(5);
    list.push_back(42);
    for (int& i: list) {
        ++i;
    }
    stst::List& strlist = store["strlist"];
    strlist.push_back((const char*) "foo");
    std::cout << "store: " << store << std::endl;
    return 0;
}
```

Creating C++ structs from nested structures with containers is also possible:

```c++
struct Subsettings {
    stst::StructStore& store;
    
    int& subnum = store["subnum"] = 42;
    stst::string& substr = store["substr"] = (const char*) "bar";
    
    explicit Subsettings(stst::StructStore& store) : store(store) {}
};

struct Settings {
    stst::StructStore& store;
    
    int& num = store["num"] = 5;
    bool& flag = store["flag"] = true;
    stst::string& str = store["str"] = "foo";
    Subsettings subsettings{store.substore("subsettings")};
    
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

Creating non-intrusive (de)serialization for existing structs is also possible,
see file `examples/example2.cpp`.

### Python

```python
state = structstore.StructStore()
state.num = 5
state.value = 3.14
state.mystr = 'foo'
state.flag = True
state.lst = [1, 2, 3, 5, 8]
import numpy as np
state.vec = np.array([[1.0, 2.0], [3.0, 4.0]])
print(state.deepcopy())
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
    lst: List[int]

store = structstore.StructStore()
store.state = State(5, 'foo', True, Substate(42), [0, 1])
print(store.deepcopy())
```

### Shared structure in C++

```c++
stst::StructStoreShared shdata_store("/shdata_store");
std::cout << "shared data: " << *shdata_store << std::endl;
shdata_store[H("num")] = 53; // compile-time string hashing

// usage with a struct as above:
stst::StructStoreShared shsettings_store("/shsettings_store");
Settings shsettings{*shsettings_store};
shsettings.num = 42;
std::cout << "settings struct: " << *shsettings_store << std::endl;
```

### Shared structure in Python

```python
shmem = structstore.StructStoreShared("/shdata_store")
shmem.state = State(5, 'foo', True, Substate(42), [0, 1])
print(shmem.deepcopy())
```

## Implementation details

Dynamic structures (such as the internal field map and any containers) use an
arena allocator with a memory region residing along the StructStore itself.
Thus, the whole structure including dynamic structures with pointers can be
mmap'ed by several processes.

## Limitations

* The library currently only supports the following types: int, double, string,
  bool, list, NumPy float64 vectors, 2D NumPy float64 arrays, nested structures.
* The arena memory region currently has a fixed size, i.e. at some point,
  additional allocations will throw an exception.
* Shared memory is mmap'ed to the same address in all processes (using
  MAP_FIXED_NOREPLACE), this might fail when the memory region is already
  reserved in a process.
* Opening shared memory multiple times (e.g. in separate threads) from one
  process is currently not supported.
* Locking a shared structure (or parts of it) currently has only basic support.

## License

This repo is released under the BSD 3-Clause License. See LICENSE file for
details.
